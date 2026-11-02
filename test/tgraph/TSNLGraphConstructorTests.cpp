// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <stdexcept>
#include <vector>

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLInstParameter.h"
#include "SNLParameter.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "TClockPropagator.h"
#include "TSNLGraphConstructor.h"

using namespace naja::NL;
using namespace naja::TG;

class TSNLGraphConstructorTests: public ::testing::Test {
  protected:
    void SetUp() override {
      auto* db = NLDB::create(NLUniverse::create());
      library_ = NLLibrary::create(db);
      top_ = SNLDesign::create(library_, NLName("top"));
    }
    void TearDown() override { NLUniverse::get()->destroy(); }
    SNLScalarTerm* port(SNLDesign* design, const char* name, SNLTerm::Direction direction) {
      return SNLScalarTerm::create(design, direction, NLName(name));
    }
    void connect(SNLDesign* design, std::initializer_list<SNLNetComponent*> terms) {
      auto* net = SNLScalarNet::create(design);
      for (auto* term : terms) term->setNet(net);
    }
    SNLInstTerm* term(SNLInstance* instance, const char* name) {
      return instance->getInstTerm(instance->getModel()->getScalarTerm(NLName(name)));
    }
    TGraph::EdgeId edge(const TGraph& graph, TGraph::NodeId from, TGraph::NodeId to) {
      for (const auto& e : graph.getEdgesStorage()) {
        if (e.isAlive() && e.getSource() == from && e.getTarget() == to) return e.getIndex();
      }
      return TGraph::kInvalidEdge;
    }
    void expectFailure(TSNLGraphConstructor& constructor, const std::string& reason) {
      try {
        constructor.construct();
        FAIL() << "Expected construction to fail: " << reason;
      } catch (const NLException& error) {
        EXPECT_NE(std::string::npos, std::string(error.what()).find(reason)) << error.what();
      }
      EXPECT_THROW(constructor.getGraph(), NLException);
    }
    SNLDesign* modeledFlop() {
      using Model = SNLDesignModeling;
      auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
      auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("flop"));
      auto* clock = port(model, "C", SNLTerm::Direction::Input);
      auto* data = port(model, "D", SNLTerm::Direction::Input);
      auto* output = port(model, "Q", SNLTerm::Direction::Output);
      Model::SequentialModel sequence;
      sequence.clockedOn.root = sequence.clockedOn.addTerm(clock);
      sequence.states.emplace_back();
      auto& next = sequence.states.front().nextState;
      next.root = next.addTerm(data);
      sequence.outputs.push_back({output, {}});
      auto& function = sequence.outputs.front().function;
      function.root = function.addState(0);
      Model::setSequentialModel(model, sequence);
      Model::setTermRole(clock, Model::SNLTermRole::Clock);
      Model::setTermRole(data, Model::SNLTermRole::DataInput);
      Model::setTermRole(output, Model::SNLTermRole::DataOutput);
      Model::addInputsToClockArcs({data}, clock);
      Model::addClockToOutputsArcs(clock, {output});
      return model;
    }
    struct GateParts {
      SNLInstance* latch;
      SNLInstance* inverter;
      SNLInstance* conjunction;
      SNLScalarTerm* clockPort;
      SNLScalarTerm* enablePort;
      SNLScalarTerm* outputPort;
      SNLInstTerm* clock;
      SNLInstTerm* heldInput;
      SNLInstTerm* output;
      SNLInstTerm* latchGate;
      SNLInstTerm* data;
      SNLInstTerm* held;
      SNLInstTerm* invInput;
      SNLInstTerm* invOutput;
    };
    GateParts decomposedGate(SNLDesign* design, bool swapInputs = false) {
      auto* latch = SNLInstance::create(design, NLDB0::getDLatch(), NLName("state"));
      auto* invModel = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
      auto* inverter = SNLInstance::create(design, invModel, NLName("inverse"));
      auto* andModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
      auto* conjunction = SNLInstance::create(design, andModel, NLName("conjunction"));
      auto* root = port(design, "root", SNLTerm::Direction::Input);
      auto* enable = port(design, "control", SNLTerm::Direction::Input);
      auto* output = port(design, "result", SNLTerm::Direction::Output);
      auto* c = conjunction->getInstTerm(NLDB0::getGateNTerms(andModel)->getBit(swapInputs ? 1 : 0));
      auto* h = conjunction->getInstTerm(NLDB0::getGateNTerms(andModel)->getBit(swapInputs ? 0 : 1));
      auto* g = conjunction->getInstTerm(NLDB0::getGateSingleTerm(andModel));
      auto* latchGate = latch->getInstTerm(NLDB0::getDLatchEnable());
      auto* data = latch->getInstTerm(NLDB0::getDLatchData());
      auto* held = latch->getInstTerm(NLDB0::getDLatchOutput());
      auto* invInput = inverter->getInstTerm(NLDB0::getGateSingleTerm(invModel));
      auto* invOutput = inverter->getInstTerm(NLDB0::getGateNTerms(invModel)->getBit(0));
      connect(design, {root, c, invInput});
      connect(design, {invOutput, latchGate});
      connect(design, {enable, data});
      connect(design, {held, h});
      connect(design, {g, output});
      return {latch, inverter, conjunction, root, enable, output, c, h, g, latchGate, data, held, invInput, invOutput};
    }
    NLLibrary* library_ {nullptr};
    SNLDesign* top_ {nullptr};
};

TEST_F(TSNLGraphConstructorTests, registerTransferThroughCombinationalCell) {
  auto* clockA = port(top_, "clock_a", SNLTerm::Direction::Input);
  auto* clockB = port(top_, "clock_b", SNLTerm::Direction::Input);
  auto* input = port(top_, "input", SNLTerm::Direction::Input);
  auto* output = port(top_, "output", SNLTerm::Direction::Output);
  auto* launch = SNLInstance::create(top_, NLDB0::getDFF(), NLName("launch"));
  auto* capture = SNLInstance::create(top_, NLDB0::getDFF(), NLName("capture"));
  auto* buffer = SNLInstance::create(top_, NLDB0::getAssign(), NLName("buffer"));
  connect(top_, {clockA, term(launch, "C")});
  connect(top_, {clockB, term(capture, "C")});
  connect(top_, {input, term(launch, "D")});
  connect(top_, {term(launch, "Q"), buffer->getInstTerm(NLDB0::getAssignInput())});
  connect(top_, {buffer->getInstTerm(NLDB0::getAssignOutput()), term(capture, "D")});
  connect(top_, {term(capture, "Q"), output});

  TSNLGraphConstructor constructor(top_);
  EXPECT_THROW(constructor.getGraph(), NLException);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* component) { return constructor.getNode(SNLOccurrence(component)); };
  EXPECT_EQ(2u, graph.getCaptureEdgesCount());
  EXPECT_EQ(0u, graph.getClocksCount());
  EXPECT_TRUE(graph.getNode(id(term(launch, "Q")))->isFlopDataOutput());
  EXPECT_TRUE(graph.getNode(id(term(capture, "D")))->isFlopDataInput());
  EXPECT_TRUE(graph.getNode(id(buffer->getInstTerm(NLDB0::getAssignInput())))->isCombinationalReceiver());
  auto cq = edge(graph, id(term(launch, "C")), id(term(launch, "Q")));
  ASSERT_NE(TGraph::kInvalidEdge, cq);
  EXPECT_EQ(TSNLEdgeProxy::Kind::ClockToOutput,
    dynamic_cast<const TSNLEdgeProxy*>(graph.getEdge(cq)->getProxy())->getKind());
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(term(launch, "D")), id(term(launch, "Q"))));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(term(capture, "C")), id(term(capture, "D"))));

  auto a = graph.createClock("a", clockA, id(clockA));
  auto b = graph.createClock("b", clockB, id(clockB));
  TClockPropagator::run(graph);
  auto data = edge(graph, id(buffer->getInstTerm(NLDB0::getAssignOutput())), id(term(capture, "D")));
  ASSERT_NE(TGraph::kInvalidEdge, data);
  EXPECT_TRUE(graph.getEdge(data)->hasFromClock(a));
  EXPECT_TRUE(graph.getEdge(data)->hasToClock(b));
  EXPECT_TRUE(graph.getEdge(data)->hasClockCrossing());
  EXPECT_TRUE(graph.getEdge(cq)->getToClocks().none());
  const auto* proxy = dynamic_cast<const TSNLNodeProxy*>(graph.getNode(id(term(capture, "D")))->getProxy());
  ASSERT_NE(nullptr, proxy);
  EXPECT_EQ(SNLOccurrence(term(capture, "D")), proxy->getOccurrence());
}

TEST_F(TSNLGraphConstructorTests, sharedHierarchyPreservesOccurrencesAndPortAliases) {
  auto* block = SNLDesign::create(library_, NLName("block"));
  auto* in = port(block, "in", SNLTerm::Direction::Input);
  auto* out = port(block, "out", SNLTerm::Direction::Output);
  auto* buffer = SNLInstance::create(block, NLDB0::getAssign(), NLName("buffer"));
  connect(block, {in, buffer->getInstTerm(NLDB0::getAssignInput())});
  connect(block, {buffer->getInstTerm(NLDB0::getAssignOutput()), out});
  auto* left = SNLInstance::create(top_, block, NLName("left"));
  auto* right = SNLInstance::create(top_, block, NLName("right"));
  auto* root = port(top_, "root", SNLTerm::Direction::Input);
  auto* sink = port(top_, "sink", SNLTerm::Direction::Output);
  connect(top_, {root, left->getInstTerm(in)});
  connect(top_, {left->getInstTerm(out), right->getInstTerm(in)});
  connect(top_, {right->getInstTerm(out), sink});
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto leftPin = SNLOccurrence(SNLPath(left), buffer->getInstTerm(NLDB0::getAssignInput()));
  auto rightPin = SNLOccurrence(SNLPath(right), buffer->getInstTerm(NLDB0::getAssignInput()));
  EXPECT_NE(constructor.getNode(leftPin), constructor.getNode(rightPin));
  EXPECT_EQ(constructor.getNode(SNLOccurrence(left->getInstTerm(in))),
    constructor.getNode(SNLOccurrence(SNLPath(left), in)));
  auto clk = graph.createClock("clk", root, constructor.getNode(SNLOccurrence(root)));
  TClockPropagator::run(graph);
  auto sinkId = constructor.getNode(SNLOccurrence(sink));
  ASSERT_EQ(1u, graph.getNode(sinkId)->getInDegree());
  EXPECT_TRUE(graph.getEdge(graph.getNode(sinkId)->getInEdges().front())->hasFromClock(clk));
  EXPECT_EQ(TGraph::kInvalidNode, constructor.getNode(SNLOccurrence(in)));
}

TEST_F(TSNLGraphConstructorTests, busFlopsAndControlRoles) {
  auto* wideModel = NLDB0::getOrCreateDFF(3);
  auto* wide = SNLInstance::create(top_, wideModel, NLName("wide"));
  auto* enabled = SNLInstance::create(top_, NLDB0::getDFFRE(), NLName("enabled"));
  auto* synchronous = SNLInstance::create(top_, NLDB0::getDFFSR(), NLName("synchronous"));
  auto* syncSet = SNLInstance::create(top_, NLDB0::getDFFSS(), NLName("sync_set"));
  auto* asyncSet = SNLInstance::create(top_, NLDB0::getDFFS(), NLName("async_set"));
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  for (auto* bit : wideModel->getBusTerm(NLName("D"))->getBits()) {
    auto id = constructor.getNode(SNLOccurrence(wide->getInstTerm(bit)));
    EXPECT_TRUE(graph.getNode(id)->isFlopDataInput());
    EXPECT_EQ(1u, graph.getNode(id)->getCaptureInEdges().size());
  }
  auto check = [&](SNLInstance* instance, const char* name, TNode::PinRole role, size_t captures) {
    auto id = constructor.getNode(SNLOccurrence(term(instance, name)));
    EXPECT_EQ(role, graph.getNode(id)->getPinRole());
    EXPECT_EQ(captures, graph.getNode(id)->getCaptureInEdges().size());
  };
  check(enabled, "E", TNode::PinRole::Enable, 1);
  check(enabled, "R", TNode::PinRole::AsyncReset, 0);
  check(synchronous, "R", TNode::PinRole::SyncReset, 1);
  check(syncSet, "S", TNode::PinRole::SyncSet, 1);
  check(asyncSet, "S", TNode::PinRole::AsyncSet, 0);
}

TEST_F(TSNLGraphConstructorTests, parameterizedCombinationalArcsUseInstanceValues) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("selector"));
  auto* a = port(model, "a", SNLTerm::Direction::Input);
  auto* b = port(model, "b", SNLTerm::Direction::Input);
  auto* q = port(model, "q", SNLTerm::Direction::Output);
  SNLParameter::create(model, NLName("SELECT"), SNLParameter::Type::Decimal, "0");
  SNLDesignModeling::setParameter(model, "SELECT", "0");
  SNLDesignModeling::addCombinatorialArcs("0", {a}, {q});
  SNLDesignModeling::addCombinatorialArcs("1", {b}, {q});
  auto* first = SNLInstance::create(top_, model, NLName("first"));
  auto* second = SNLInstance::create(top_, model, NLName("second"));
  SNLInstParameter::create(second, model->getParameter(NLName("SELECT")), "1");
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto id = [&](SNLInstance* instance, SNLBitTerm* bit) {
    return constructor.getNode(SNLOccurrence(instance->getInstTerm(bit)));
  };
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(first, a), id(first, q)));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(first, b), id(first, q)));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(second, b), id(second, q)));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(second, a), id(second, q)));
}

TEST_F(TSNLGraphConstructorTests, unsupportedModelsFailWithoutReplacingPreviousGraph) {
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto* previous = &constructor.getGraph();
  auto* unknown = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("unknown"));
  auto* instance = SNLInstance::create(top_, unknown, NLName("instance"));
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
  instance->destroy();
  instance = SNLInstance::create(top_, NLDB0::getDLatch(), NLName("latch"));
  EXPECT_THROW(constructor.construct(), NLException);
  instance->destroy();
  auto* io = port(top_, "io", SNLTerm::Direction::InOut);
  EXPECT_THROW(constructor.construct(), NLException);
  io->destroy();
  auto* a = port(top_, "a", SNLTerm::Direction::Input);
  auto* b = port(top_, "b", SNLTerm::Direction::Input);
  connect(top_, {a, b});
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
}

TEST_F(TSNLGraphConstructorTests, recursiveHierarchyAndInvalidTopAreRejected) {
  TSNLGraphConstructor nullConstructor(nullptr);
  EXPECT_THROW(nullConstructor.construct(), NLException);
  TSNLGraphConstructor leafConstructor(NLDB0::getDFF());
  EXPECT_THROW(leafConstructor.construct(), NLException);
  auto* child = SNLDesign::create(library_, NLName("child"));
  SNLInstance::create(top_, child, NLName("child"));
  SNLInstance::create(child, top_, NLName("recursive"));
  TSNLGraphConstructor constructor(top_);
  EXPECT_THROW(constructor.construct(), NLException);
}

TEST_F(TSNLGraphConstructorTests, constantsAndUnconnectedPinsDoNotInventDependencies) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("constant"));
  port(model, "unused", SNLTerm::Direction::Input);
  port(model, "value", SNLTerm::Direction::Output);
  SNLDesignModeling::setTruthTable(model, SNLTruthTable(0, 1, {}));
  auto* instance = SNLInstance::create(top_, model, NLName("constant"));
  auto* output = port(top_, "out", SNLTerm::Direction::Output);
  connect(top_, {term(instance, "value"), output});
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  EXPECT_EQ(3u, graph.getNodesCount());
  EXPECT_EQ(1u, graph.getEdgesCount());
  auto unused = constructor.getNode(SNLOccurrence(term(instance, "unused")));
  EXPECT_EQ(0u, graph.getNode(unused)->getOutDegree());
  EXPECT_NO_THROW(TClockPropagator::run(graph));
}

TEST_F(TSNLGraphConstructorTests, mixedSequentialOutputsAreRejected) {
  auto* model = NLDB0::getDFF();
  auto sequence = SNLDesignModeling::getSequentialModel(model);
  sequence.outputs.front().function = {};
  sequence.outputs.front().function.root =
    sequence.outputs.front().function.addTerm(NLDB0::getDFFData());
  SNLDesignModeling::setSequentialModel(model, sequence);
  SNLInstance::create(top_, model, NLName("mixed"));
  TSNLGraphConstructor constructor(top_);
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_THROW(constructor.getGraph(), NLException);
}

TEST_F(TSNLGraphConstructorTests, invertedClockAndStateOutputPreserveTimingEdges) {
  using Model = SNLDesignModeling;
  using Operator = Model::BooleanExpression::Operator;
  auto* model = NLDB0::getDFF();
  auto sequence = Model::getSequentialModel(model);
  sequence.clockedOn.root = sequence.clockedOn.addOperation(Operator::Not, {sequence.clockedOn.root});
  auto& output = sequence.outputs.front().function;
  output.root = output.addOperation(Operator::Not, {output.root});
  Model::setSequentialModel(model, sequence);
  auto* instance = SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  const auto& constConstructor = constructor;
  EXPECT_THROW(constConstructor.getGraph(), NLException);
  constructor.construct();
  const auto& graph = constConstructor.getGraph();
  EXPECT_EQ(&constructor.getGraph(), &graph);
  auto clock = constructor.getNode(SNLOccurrence(term(instance, "C")));
  auto data = constructor.getNode(SNLOccurrence(term(instance, "D")));
  auto q = constructor.getNode(SNLOccurrence(term(instance, "Q")));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, clock, q));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, data, q));
  EXPECT_EQ(1u, graph.getNode(data)->getCaptureInEdges().size());
}

TEST_F(TSNLGraphConstructorTests, sequentialTimingWithoutExplicitModelIsRejected) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("legacy"));
  auto* clock = port(model, "clock", SNLTerm::Direction::Input);
  auto* data = port(model, "data", SNLTerm::Direction::Input);
  SNLDesignModeling::addInputsToClockArcs({data}, clock);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "sequential cell has no explicit flip-flop/latch model");
}

TEST_F(TSNLGraphConstructorTests, compositeClockExpressionIsRejected) {
  using Model = SNLDesignModeling;
  auto* model = NLDB0::getDFF();
  auto sequence = Model::getSequentialModel(model);
  auto data = sequence.clockedOn.addTerm(NLDB0::getDFFData());
  sequence.clockedOn.root = sequence.clockedOn.addOperation(
    Model::BooleanExpression::Operator::And, {sequence.clockedOn.root, data});
  Model::setSequentialModel(model, sequence);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "unsupported clock expression or missing clock role");
}

TEST_F(TSNLGraphConstructorTests, missingClockRoleIsRejected) {
  SNLDesignModeling::setTermRole(NLDB0::getDFFClock(), SNLDesignModeling::SNLTermRole::DataInput);
  SNLInstance::create(top_, NLDB0::getDFF(), NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "unsupported clock expression or missing clock role");
}

TEST_F(TSNLGraphConstructorTests, unknownPinRoleIsRejected) {
  auto* model = NLDB0::getDFF();
  port(model, "extra", SNLTerm::Direction::Input);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "unsupported or inconsistent pin role");
}

TEST_F(TSNLGraphConstructorTests, clockArcToInputIsRejected) {
  auto* model = modeledFlop();
  SNLDesignModeling::addClockToOutputsArcs(
    model->getScalarTerm(NLName("C")), {model->getScalarTerm(NLName("D"))});
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "clock-to-output arc targets a non-output role");
}

TEST_F(TSNLGraphConstructorTests, clockArcWithoutStateOutputIsRejected) {
  auto* model = NLDB0::getDFF();
  auto* extra = port(model, "extra", SNLTerm::Direction::Output);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataOutput);
  SNLDesignModeling::addClockToOutputsArcs(NLDB0::getDFFClock(), {extra});
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "clock-to-output arc has no sequential state output");
}

TEST_F(TSNLGraphConstructorTests, capturePinWithoutTimingRelationshipIsRejected) {
  auto* model = modeledFlop();
  auto* extra = port(model, "extra", SNLTerm::Direction::Input);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataInput);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "missing sequential timing relationship");
}

TEST_F(TSNLGraphConstructorTests, outputWithoutDependencyIsRejected) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("partial"));
  auto* input = port(model, "input", SNLTerm::Direction::Input);
  auto* output = port(model, "output", SNLTerm::Direction::Output);
  SNLDesignModeling::addCombinatorialArcs({input}, {output});
  port(model, "extra", SNLTerm::Direction::Output);
  SNLInstance::create(top_, model, NLName("buffer"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "output has no dependency model");
}

TEST_F(TSNLGraphConstructorTests, combinationalArcFromOutputIsRejected) {
  auto* model = NLDB0::getAssign();
  SNLDesignModeling::addCombinatorialArcs({NLDB0::getAssignOutput()}, {NLDB0::getAssignOutput()});
  SNLInstance::create(top_, model, NLName("buffer"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "combinational arc source is not an input");
}

TEST_F(TSNLGraphConstructorTests, recursiveHierarchyBelowTopIsRejected) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* descendant = SNLDesign::create(library_, NLName("descendant"));
  SNLInstance::create(top_, child, NLName("child"));
  SNLInstance::create(child, descendant, NLName("descendant"));
  SNLInstance::create(descendant, child, NLName("recursive"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "recursive hierarchy");
}

TEST_F(TSNLGraphConstructorTests, clockControlFeedbackSharesStructureWithDataFanout) {
  auto* root = port(top_, "root", SNLTerm::Direction::Input);
  auto* input = port(top_, "input", SNLTerm::Direction::Input);
  auto* output = port(top_, "output", SNLTerm::Direction::Output);
  auto* state = SNLInstance::create(top_, NLDB0::getDFF(), NLName("state"));
  auto* sink = SNLInstance::create(top_, NLDB0::getDFF(), NLName("sink"));
  auto* shared = SNLInstance::create(top_, NLDB0::getAssign(), NLName("shared"));
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("and"));
  auto* source = port(model, "source", SNLTerm::Direction::Input);
  auto* control = port(model, "control", SNLTerm::Direction::Input);
  auto* result = port(model, "result", SNLTerm::Direction::Output);
  SNLDesignModeling::setTruthTable(model, SNLTruthTable(2, 8, SNLTruthTable::fullDependencies(2)));
  SNLDesignModeling::addCombinatorialArcs({source, control}, {result});
  auto* gate = SNLInstance::create(top_, model, NLName("gate"));
  auto* sharedIn = shared->getInstTerm(NLDB0::getAssignInput());
  auto* sharedOut = shared->getInstTerm(NLDB0::getAssignOutput());
  connect(top_, {root, term(gate, "source")});
  connect(top_, {term(gate, "result"), term(state, "C"), term(sink, "C")});
  connect(top_, {input, term(state, "D")});
  connect(top_, {term(state, "Q"), sharedIn});
  connect(top_, {sharedOut, term(gate, "control"), term(sink, "D"), output});

  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* component) { return constructor.getNode(SNLOccurrence(component)); };
  //One node per terminal, even though the shared output is used for both
  //clock control and ordinary data. This is a topology test, not an ICG model.
  EXPECT_EQ(14u, graph.getNodesCount());
  EXPECT_EQ(13u, graph.getEdgesCount());
  EXPECT_EQ(2u, graph.getCaptureEdgesCount());
  EXPECT_EQ(3u, graph.getNode(id(sharedOut))->getOutDegree());
  for (auto* reader : {term(gate, "control"), term(sink, "D")}) {
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(sharedOut), id(reader)));
  }
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(sharedOut), id(output)));

  const std::vector<SNLNetComponent*> cycle = {
    term(gate, "control"), term(gate, "result"), term(state, "C"),
    term(state, "Q"), sharedIn, sharedOut};
  const std::vector<TSNLEdgeProxy::Kind> kinds = {
    TSNLEdgeProxy::Kind::Combinational, TSNLEdgeProxy::Kind::Connectivity,
    TSNLEdgeProxy::Kind::ClockToOutput, TSNLEdgeProxy::Kind::Connectivity,
    TSNLEdgeProxy::Kind::Combinational, TSNLEdgeProxy::Kind::Connectivity};
  for (size_t i = 0; i < cycle.size(); ++i) {
    auto eid = edge(graph, id(cycle[i]), id(cycle[(i + 1) % cycle.size()]));
    ASSERT_NE(TGraph::kInvalidEdge, eid);
    auto* proxy = dynamic_cast<const TSNLEdgeProxy*>(graph.getEdge(eid)->getProxy());
    ASSERT_NE(nullptr, proxy);
    EXPECT_EQ(kinds[i], proxy->getKind());
  }

  //The ordinary graph has a cycle; the connectivity/combinational view does
  //not. Filtering traversal keeps every physical edge and its identity intact.
  auto processed = [&](bool includeClockToOutput) {
    std::vector<size_t> indegree(graph.getNodesStorage().size(), 0);
    auto follows = [&](const TEdge& candidate) {
      const auto* proxy = static_cast<const TSNLEdgeProxy*>(candidate.getProxy());
      return includeClockToOutput || proxy->getKind() != TSNLEdgeProxy::Kind::ClockToOutput;
    };
    for (const auto& candidate : graph.getEdgesStorage()) {
      if (follows(candidate)) ++indegree[candidate.getTarget()];
    }
    std::vector<TGraph::NodeId> ready;
    for (const auto& node : graph.getNodesStorage()) {
      if (indegree[node.getIndex()] == 0) ready.push_back(node.getIndex());
    }
    for (size_t i = 0; i < ready.size(); ++i) {
      for (auto eid : graph.getNode(ready[i])->getOutEdges()) {
        const auto& candidate = *graph.getEdge(eid);
        if (follows(candidate) && --indegree[candidate.getTarget()] == 0) {
          ready.push_back(candidate.getTarget());
        }
      }
    }
    return ready.size();
  };
  EXPECT_LT(processed(true), graph.getNodesCount());
  EXPECT_EQ(graph.getNodesCount(), processed(false));
  for (const auto& candidate : graph.getEdgesStorage()) EXPECT_FALSE(candidate.isDisabled());
  EXPECT_EQ(14u, graph.getNodesCount());
  EXPECT_EQ(13u, graph.getEdgesCount());
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGatePreservesPhysicalStructure) {
  for (bool swapped : {false, true}) {
    auto* child = SNLDesign::create(library_);
    auto parts = decomposedGate(child, swapped);
    auto* wrapper = SNLInstance::create(top_, child);
    TSNLGraphConstructor constructor(top_);
    EXPECT_THROW(constructor.getClockGates(), NLException);
    constructor.construct();
    auto& graph = constructor.getGraph();
    auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(SNLPath(wrapper), pin)); };
    ASSERT_EQ(1u, constructor.getClockGates().size());
    const auto& gate = constructor.getClockGates().front();
    EXPECT_EQ(id(parts.clock), gate.clock);
    EXPECT_EQ(id(parts.data), gate.enable);
    EXPECT_EQ(id(parts.held), gate.heldEnable);
    EXPECT_EQ(id(parts.latchGate), gate.latchGate);
    EXPECT_EQ(id(parts.output), gate.output);
    EXPECT_EQ(11u, graph.getNodesCount());
    EXPECT_EQ(10u, graph.getEdgesCount());
    EXPECT_EQ(1u, graph.getCaptureEdgesCount());
    EXPECT_TRUE(graph.getNode(gate.heldEnable)->isLatchDataOutput());
    EXPECT_TRUE(graph.getNode(gate.enable)->isLatchDataInput());
    auto* proxy = dynamic_cast<const TSNLEdgeProxy*>(graph.getEdge(gate.clockPassEdge)->getProxy());
    ASSERT_NE(nullptr, proxy);
    EXPECT_EQ(TSNLEdgeProxy::Kind::ClockPass, proxy->getKind());
    EXPECT_EQ(gate.clock, graph.getEdge(gate.clockPassEdge)->getSource());
    EXPECT_EQ(gate.output, graph.getEdge(gate.clockPassEdge)->getTarget());
    auto heldArc = edge(graph, id(parts.heldInput), id(parts.output));
    ASSERT_NE(TGraph::kInvalidEdge, heldArc);
    EXPECT_EQ(TSNLEdgeProxy::Kind::Combinational,
      static_cast<const TSNLEdgeProxy*>(graph.getEdge(heldArc)->getProxy())->getKind());
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, gate.latchGate, gate.heldEnable));
    EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, gate.enable, gate.heldEnable));
    const auto* capture = graph.getCaptureEdge(0);
    EXPECT_EQ(TCaptureEdge::Kind::ClockGateControl, capture->getKind());
    EXPECT_EQ(gate.clock, capture->getSource());
    EXPECT_EQ(gate.enable, capture->getTarget());
    auto a = graph.createClock("a", nullptr, id(parts.clockPort));
    auto result = TClockPropagator::run(graph);
    EXPECT_TRUE(result.clockOrigins[gate.output].test(a));
    EXPECT_TRUE(result.clockOrigins[gate.heldEnable].none());
    EXPECT_TRUE(graph.getEdge(heldArc)->hasFromClock(a));
    for (const auto& candidate : graph.getEdgesStorage()) EXPECT_FALSE(candidate.isDisabled());
    EXPECT_EQ(3u, child->getInstances().size());
    wrapper->destroy();
  }
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateKeepsSharedDataFanout) {
  auto parts = decomposedGate(top_);
  auto* receiver = SNLInstance::create(top_, NLDB0::getDFF(), NLName("receiver"));
  auto* heldOutput = port(top_, "held", SNLTerm::Direction::Output);
  heldOutput->setNet(parts.held->getNet());
  term(receiver, "C")->setNet(parts.clockPort->getNet());
  term(receiver, "D")->setNet(parts.output->getNet());
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  ASSERT_EQ(1u, constructor.getClockGates().size());
  auto gate = constructor.getClockGates().front();
  EXPECT_EQ(15u, graph.getNodesCount());
  EXPECT_EQ(14u, graph.getEdgesCount());
  EXPECT_EQ(2u, graph.getCaptureEdgesCount());
  EXPECT_EQ(2u, graph.getNode(gate.heldEnable)->getOutDegree());
  EXPECT_EQ(2u, graph.getNode(gate.output)->getOutDegree());
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, gate.heldEnable, constructor.getNode(SNLOccurrence(heldOutput))));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, gate.output, constructor.getNode(SNLOccurrence(term(receiver, "D")))));
  EXPECT_EQ(4u, top_->getInstances().size());
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateAcrossSharedHierarchyAndBuffers) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto parts = decomposedGate(child);
  auto* shared = SNLInstance::create(child, NLDB0::getAssign(), NLName("buffer"));
  auto* bufferIn = shared->getInstTerm(NLDB0::getAssignInput());
  auto* bufferOut = shared->getInstTerm(NLDB0::getAssignOutput());
  bufferIn->setNet(parts.held->getNet());
  connect(child, {bufferOut, parts.heldInput});
  auto* left = SNLInstance::create(top_, child, NLName("left"));
  auto* right = SNLInstance::create(top_, child, NLName("right"));
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  ASSERT_EQ(2u, constructor.getClockGates().size());
  EXPECT_EQ(26u, constructor.getGraph().getNodesCount());
  EXPECT_NE(constructor.getClockGates()[0].heldEnable, constructor.getClockGates()[1].heldEnable);
  for (auto* wrapper : {left, right}) {
    EXPECT_EQ(constructor.getNode(SNLOccurrence(wrapper->getInstTerm(parts.clockPort))),
      constructor.getNode(SNLOccurrence(SNLPath(wrapper), parts.clockPort)));
  }
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateRejectsWrongPhaseOrSource) {
  for (size_t variant = 0; variant < 5; ++variant) {
    SCOPED_TRACE(variant);
    auto* child = SNLDesign::create(library_);
    auto parts = decomposedGate(child);
    auto* wrapper = SNLInstance::create(top_, child);
    if (variant == 0) parts.latchGate->setNet(parts.clockPort->getNet()); //Transparent high.
    if (variant == 1) parts.invInput->setNet(parts.enablePort->getNet()); //Different source.
    if (variant == 2) parts.heldInput->setNet(parts.enablePort->getNet()); //No held state.
    if (variant == 3) parts.invInput->setNet(parts.invOutput->getNet()); //Inverter cycle.
    if (variant == 4) {
      auto* invModel = parts.inverter->getModel();
      auto* second = SNLInstance::create(child, invModel);
      auto* input = second->getInstTerm(NLDB0::getGateSingleTerm(invModel));
      auto* output = second->getInstTerm(NLDB0::getGateNTerms(invModel)->getBit(0));
      input->setNet(parts.held->getNet());
      connect(child, {output, parts.heldInput}); //Inverted held state.
    }
    TSNLGraphConstructor constructor(top_);
    expectFailure(constructor, "latch analysis is unsupported");
    wrapper->destroy();
  }
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateFailurePreservesPreviousResult) {
  auto parts = decomposedGate(top_);
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto* previous = &constructor.getGraph();
  auto gate = constructor.getClockGates().front();
  parts.invInput->setNet(parts.enablePort->getNet());
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
  ASSERT_EQ(1u, constructor.getClockGates().size());
  EXPECT_EQ(gate.clock, constructor.getClockGates().front().clock);
  EXPECT_EQ(gate.enable, constructor.getClockGates().front().enable);
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateTracingIsIterative) {
  auto parts = decomposedGate(top_);
  auto* previous = parts.invOutput;
  constexpr size_t depth = 6000;
  for (size_t i = 0; i < depth; ++i) {
    auto* buffer = SNLInstance::create(top_, NLDB0::getAssign());
    auto* input = buffer->getInstTerm(NLDB0::getAssignInput());
    auto* output = buffer->getInstTerm(NLDB0::getAssignOutput());
    input->setNet(previous->getNet());
    connect(top_, {output});
    previous = output;
  }
  parts.latchGate->setNet(previous->getNet());
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  ASSERT_EQ(1u, constructor.getClockGates().size());
  EXPECT_EQ(11u + 2 * depth, constructor.getGraph().getNodesCount());
  EXPECT_EQ(10u + 2 * depth, constructor.getGraph().getEdgesCount());
  auto& graph = constructor.getGraph();
  auto c = graph.createClock("root", parts.clockPort, constructor.getNode(SNLOccurrence(parts.clockPort)));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.clockOrigins[constructor.getClockGates().front().output].test(c));
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateRejectsOtherLatchModels) {
  auto* custom = modeledFlop();
  auto model = SNLDesignModeling::getSequentialModel(custom);
  model.kind = SNLDesignModeling::SequentialModel::Kind::Latch;
  SNLDesignModeling::setSequentialModel(custom, model);
  auto* instance = SNLInstance::create(top_, custom);
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "latch analysis is unsupported");
  instance->destroy();
  instance = SNLInstance::create(top_, NLDB0::getOrCreateDLatch(2));
  expectFailure(constructor, "latch analysis is unsupported");
  instance->destroy();
  //Do not trust the canonical name alone if the live model was modified.
  auto* canonical = NLDB0::getDLatch();
  model = SNLDesignModeling::getSequentialModel(canonical);
  model.clockedOn.root = model.clockedOn.addOperation(
    SNLDesignModeling::BooleanExpression::Operator::Not, {model.clockedOn.root});
  SNLDesignModeling::setSequentialModel(canonical, model);
  SNLInstance::create(top_, canonical);
  expectFailure(constructor, "latch analysis is unsupported");
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateRejectsAmbiguousHeldInputs) {
  auto parts = decomposedGate(top_);
  auto* second = SNLInstance::create(top_, NLDB0::getDLatch());
  auto* inverse = SNLInstance::create(top_, parts.inverter->getModel());
  auto* invInput = inverse->getInstTerm(NLDB0::getGateSingleTerm(inverse->getModel()));
  auto* invOutput = inverse->getInstTerm(NLDB0::getGateNTerms(inverse->getModel())->getBit(0));
  invInput->setNet(parts.held->getNet());
  second->getInstTerm(NLDB0::getDLatchData())->setNet(parts.enablePort->getNet());
  connect(top_, {invOutput, second->getInstTerm(NLDB0::getDLatchEnable())});
  connect(top_, {second->getInstTerm(NLDB0::getDLatchOutput()), parts.clock, parts.invInput});
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "ambiguous clock-gate composition");
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateSeparatesControlAndDataPropagation) {
  auto parts = decomposedGate(top_);
  auto* otherClock = port(top_, "other", SNLTerm::Direction::Input);
  auto* enableSource = SNLInstance::create(top_, NLDB0::getDFF(), NLName("source"));
  auto* receiver = SNLInstance::create(top_, NLDB0::getDFF(), NLName("receiver"));
  auto* dataReceiver = SNLInstance::create(top_, NLDB0::getDFF(), NLName("data_receiver"));
  auto* orModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2);
  auto* merge = SNLInstance::create(top_, orModel, NLName("merge"));
  auto* left = merge->getInstTerm(NLDB0::getGateNTerms(orModel)->getBit(0));
  auto* right = merge->getInstTerm(NLDB0::getGateNTerms(orModel)->getBit(1));
  auto* merged = merge->getInstTerm(NLDB0::getGateSingleTerm(orModel));
  connect(top_, {otherClock, term(enableSource, "C"), term(dataReceiver, "C")});
  term(enableSource, "D")->setNet(parts.enablePort->getNet());
  term(receiver, "D")->setNet(parts.enablePort->getNet());
  term(receiver, "C")->setNet(parts.output->getNet());
  term(dataReceiver, "D")->setNet(parts.output->getNet());
  connect(top_, {term(enableSource, "Q"), left});
  connect(top_, {term(receiver, "Q"), right});
  connect(top_, {merged, parts.data});
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
  auto a = graph.createClock("a", parts.clockPort, id(parts.clockPort));
  auto b = graph.createClock("b", otherClock, id(otherClock));
  auto result = TClockPropagator::run(graph);
  const auto& gate = constructor.getClockGates().front();
  for (auto n : {gate.output, id(term(receiver, "C"))}) {
    EXPECT_EQ(1u, result.clockOrigins[n].count());
    EXPECT_TRUE(result.clockOrigins[n].test(a));
    EXPECT_FALSE(result.clockOrigins[n].test(b));
  }
  for (auto n : {gate.heldEnable, id(term(receiver, "Q")), id(term(enableSource, "Q"))}) {
    EXPECT_TRUE(result.clockOrigins[n].none());
  }
  auto sourceData = edge(graph, id(term(enableSource, "Q")), id(left));
  auto feedbackData = edge(graph, id(term(receiver, "Q")), id(right));
  auto outputAsData = edge(graph, gate.output, id(term(dataReceiver, "D")));
  ASSERT_NE(TGraph::kInvalidEdge, sourceData);
  ASSERT_NE(TGraph::kInvalidEdge, feedbackData);
  ASSERT_NE(TGraph::kInvalidEdge, outputAsData);
  EXPECT_TRUE(graph.getEdge(sourceData)->hasFromClock(b));
  EXPECT_TRUE(graph.getEdge(sourceData)->getToClocks().none());
  EXPECT_TRUE(graph.getEdge(sourceData)->getClockGateControlClocks().test(a));
  EXPECT_TRUE(graph.getEdge(sourceData)->hasClockGateControlCrossing());
  EXPECT_TRUE(graph.getEdge(feedbackData)->hasFromClock(a));
  EXPECT_FALSE(graph.getEdge(feedbackData)->hasClockGateControlCrossing());
  EXPECT_TRUE(graph.getEdge(outputAsData)->hasFromClock(a));
  EXPECT_TRUE(graph.getEdge(outputAsData)->hasToClock(b));
  EXPECT_TRUE(graph.getEdge(outputAsData)->getClockGateControlClocks().none());
  EXPECT_TRUE(graph.getEdge(outputAsData)->hasClockCrossing());
  // The held-enable data arc remains a data dependency when G is used as data.
  auto heldArc = edge(graph, id(parts.heldInput), gate.output);
  EXPECT_TRUE(graph.getEdge(heldArc)->hasFromClock(a));
  EXPECT_TRUE(graph.getEdge(heldArc)->hasToClock(b));
  for (const auto& e : graph.getEdgesStorage()) EXPECT_FALSE(e.isDisabled());
  auto again = TClockPropagator::run(graph);
  EXPECT_EQ(result.clockOrigins, again.clockOrigins);
}
