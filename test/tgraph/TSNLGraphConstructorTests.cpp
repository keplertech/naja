// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <unistd.h>
#include <random>
#include <stdexcept>
#include <vector>

#include "DNL.h"
#include "NLDB.h"
#include "NLClock.h"
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
#include "SNLBitNet.h"
#include "SNLScalarTerm.h"
#include "SNLSVConstructor.h"
#include "TClockPropagator.h"
#include "TConstantPropagator.h"
#include <sstream>
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
    void TearDown() override {
      naja::DNL::destroy();
      NLUniverse::get()->destroy();
    }
    void construct(TSNLGraphConstructor& constructor, SNLDesign* top = nullptr) {
      naja::DNL::destroy();
      NLUniverse::get()->setTopDesign(top ? top : top_);
      constructor.construct(*naja::DNL::get());
    }
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
    void expectCoverage(TSNLGraphConstructor& constructor, const std::string& reason) {
      ASSERT_NO_THROW(construct(constructor));
      const auto& report = constructor.getCoverageReport();
      ASSERT_FALSE(report.empty());
      EXPECT_TRUE(std::any_of(report.begin(), report.end(), [&](const auto& entry) {
        return entry.detail.find(reason) != std::string::npos;
      })) << "Missing coverage diagnostic: " << reason;
    }
    void loadSV(const std::string& source) {
      auto pattern = (std::filesystem::temp_directory_path() / "naja-tgraph-XXXXXX.sv").string();
      int fd = mkstemps(pattern.data(), 3);
      ASSERT_GE(fd, 0);
      close(fd);
      struct Cleanup {
        std::string path;
        ~Cleanup() { std::filesystem::remove(path); }
      } cleanup{pattern};
      { std::ofstream file(pattern); file << source; }
      top_->destroy();
      top_ = nullptr;
      SNLSVConstructor::ConstructOptions options;
      options.blackboxUnknownModules = true;
      options.diagnosticsReportPath.reset();
      SNLSVConstructor(library_).construct(std::filesystem::path(pattern), options);
      top_ = library_->getSNLDesign(NLName("loaded"));
      ASSERT_NE(nullptr, top_);
    }
    SNLDesign* modeledFlop(bool inouts = false) {
      using Model = SNLDesignModeling;
      auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
      auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("flop"));
      auto* clock = port(model, "C", inouts ? SNLTerm::Direction::InOut : SNLTerm::Direction::Input);
      auto* data = port(model, "D", inouts ? SNLTerm::Direction::InOut : SNLTerm::Direction::Input);
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
    SNLInstance* toggler(SNLDesign* design, SNLBitNet* clockNet) {
      auto* flop = SNLInstance::create(design, NLDB0::getDFF());
      auto* model = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
      auto* inverter = SNLInstance::create(design, model);
      term(flop, "C")->setNet(clockNet);
      connect(design, {term(flop, "Q"), inverter->getInstTerm(NLDB0::getGateSingleTerm(model))});
      connect(design, {inverter->getInstTerm(NLDB0::getGateNTerms(model)->getBit(0)), term(flop, "D")});
      return flop;
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

  TSNLGraphConstructor constructor;
  EXPECT_THROW(constructor.getGraph(), NLException);
  construct(constructor);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLInstance* instance, SNLBitTerm* bit) {
    return constructor.getNode(SNLOccurrence(instance->getInstTerm(bit)));
  };
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(first, a), id(first, q)));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(first, b), id(first, q)));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(second, b), id(second, q)));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(second, a), id(second, q)));
}

TEST_F(TSNLGraphConstructorTests, unsupportedModelsAreReportedAndInvalidDNLPreservesResult) {
  using Reason = TSNLGraphConstructor::CoverageReason;
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  auto* unknown = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("unknown"));
  auto* instance = SNLInstance::create(top_, unknown, NLName("instance"));
  ASSERT_NO_THROW(construct(constructor));
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(Reason::Blackbox, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, constructor.getCoverageReport().front().treatment);
  instance->destroy();
  instance = SNLInstance::create(top_, NLDB0::getDLatch(), NLName("latch"));
  ASSERT_NO_THROW(construct(constructor));
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(Reason::UnsupportedLatch, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Approximated, constructor.getCoverageReport().front().treatment);
  instance->destroy();
  auto* io = port(top_, "io", SNLTerm::Direction::InOut);
  ASSERT_NO_THROW(construct(constructor));
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(Reason::InOut, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Bidirectional, constructor.getCoverageReport().front().treatment);
  io->destroy();
  auto* a = port(top_, "a", SNLTerm::Direction::Input);
  auto* b = port(top_, "b", SNLTerm::Direction::Input);
  connect(top_, {a, b});
  ASSERT_NO_THROW(construct(constructor));
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(Reason::MultipleDrivers, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::AllDrivers, constructor.getCoverageReport().front().treatment);
  auto* previous = &constructor.getGraph();
  naja::DNL::DNLFull unprocessed(top_);
  EXPECT_THROW(constructor.construct(unprocessed), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(Reason::MultipleDrivers, constructor.getCoverageReport().front().reason);
}

TEST_F(TSNLGraphConstructorTests, invalidTopAndUnprocessedDNLAreRejected) {
  TSNLGraphConstructor constructor;
  EXPECT_THROW(constructor.construct(), NLException); //No universe top.
  naja::DNL::DNLFull nullDNL(nullptr);
  EXPECT_THROW(constructor.construct(nullDNL), NLException);
  naja::DNL::DNLFull leafDNL(NLDB0::getDFF());
  EXPECT_THROW(constructor.construct(leafDNL), NLException);
  naja::DNL::DNLFull unprocessed(top_);
  EXPECT_THROW(constructor.construct(unprocessed), NLException);
  EXPECT_THROW(constructor.getGraph(), NLException);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  EXPECT_EQ(3u, graph.getNodesCount());
  EXPECT_EQ(1u, graph.getEdgesCount());
  auto unused = constructor.getNode(SNLOccurrence(term(instance, "unused")));
  EXPECT_EQ(0u, graph.getNode(unused)->getOutDegree());
  EXPECT_NO_THROW(TClockPropagator::run(graph));
}

TEST_F(TSNLGraphConstructorTests, mixedSequentialOutputsAreReportedOpaque) {
  auto* model = NLDB0::getDFF();
  auto sequence = SNLDesignModeling::getSequentialModel(model);
  sequence.outputs.front().function = {};
  sequence.outputs.front().function.root =
    sequence.outputs.front().function.addTerm(NLDB0::getDFFData());
  SNLDesignModeling::setSequentialModel(model, sequence);
  SNLInstance::create(top_, model, NLName("mixed"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "mixed combinational/sequential output is unsupported");
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, constructor.getCoverageReport().front().treatment);
  EXPECT_EQ(0u, constructor.getGraph().getEdgesCount());
  EXPECT_EQ(0u, constructor.getGraph().getCaptureEdgesCount());
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
  TSNLGraphConstructor constructor;
  const auto& constConstructor = constructor;
  EXPECT_THROW(constConstructor.getGraph(), NLException);
  construct(constructor);
  const auto& graph = constConstructor.getGraph();
  EXPECT_EQ(&constructor.getGraph(), &graph);
  auto clock = constructor.getNode(SNLOccurrence(term(instance, "C")));
  auto data = constructor.getNode(SNLOccurrence(term(instance, "D")));
  auto q = constructor.getNode(SNLOccurrence(term(instance, "Q")));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, clock, q));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, data, q));
  EXPECT_EQ(1u, graph.getNode(data)->getCaptureInEdges().size());
}

TEST_F(TSNLGraphConstructorTests, sequentialTimingWithoutExplicitModelIsReported) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("legacy"));
  auto* clock = port(model, "clock", SNLTerm::Direction::Input);
  auto* data = port(model, "data", SNLTerm::Direction::Input);
  SNLDesignModeling::addInputsToClockArcs({data}, clock);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "sequential cell has no explicit flip-flop/latch model");
}

TEST_F(TSNLGraphConstructorTests, compositeClockExpressionIsReported) {
  using Model = SNLDesignModeling;
  auto* model = NLDB0::getDFF();
  auto sequence = Model::getSequentialModel(model);
  auto data = sequence.clockedOn.addTerm(NLDB0::getDFFData());
  sequence.clockedOn.root = sequence.clockedOn.addOperation(
    Model::BooleanExpression::Operator::And, {sequence.clockedOn.root, data});
  Model::setSequentialModel(model, sequence);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "unsupported clock expression or missing clock role");
}

TEST_F(TSNLGraphConstructorTests, missingClockRoleIsReported) {
  SNLDesignModeling::setTermRole(NLDB0::getDFFClock(), SNLDesignModeling::SNLTermRole::DataInput);
  SNLInstance::create(top_, NLDB0::getDFF(), NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "unsupported clock expression or missing clock role");
}

TEST_F(TSNLGraphConstructorTests, unknownPinRoleIsReported) {
  auto* model = NLDB0::getDFF();
  port(model, "extra", SNLTerm::Direction::Input);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "unsupported or inconsistent pin role");
}

TEST_F(TSNLGraphConstructorTests, clockArcToInputIsReported) {
  auto* model = modeledFlop();
  SNLDesignModeling::addClockToOutputsArcs(
    model->getScalarTerm(NLName("C")), {model->getScalarTerm(NLName("D"))});
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "clock-to-output arc targets a non-output role");
}

TEST_F(TSNLGraphConstructorTests, clockArcWithoutStateOutputIsReported) {
  auto* model = NLDB0::getDFF();
  auto* extra = port(model, "extra", SNLTerm::Direction::Output);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataOutput);
  SNLDesignModeling::addClockToOutputsArcs(NLDB0::getDFFClock(), {extra});
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "clock-to-output arc has no sequential state output");
}

TEST_F(TSNLGraphConstructorTests, capturePinWithoutTimingRelationshipIsReported) {
  auto* model = modeledFlop();
  auto* extra = port(model, "extra", SNLTerm::Direction::Input);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataInput);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "missing sequential timing relationship");
}

TEST_F(TSNLGraphConstructorTests, outputWithoutDependencyIsReported) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("partial"));
  auto* input = port(model, "input", SNLTerm::Direction::Input);
  auto* output = port(model, "output", SNLTerm::Direction::Output);
  SNLDesignModeling::addCombinatorialArcs({input}, {output});
  port(model, "extra", SNLTerm::Direction::Output);
  SNLInstance::create(top_, model, NLName("buffer"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "output has no dependency model");
}

TEST_F(TSNLGraphConstructorTests, combinationalArcFromOutputIsReported) {
  auto* model = NLDB0::getAssign();
  SNLDesignModeling::addCombinatorialArcs({NLDB0::getAssignOutput()}, {NLDB0::getAssignOutput()});
  SNLInstance::create(top_, model, NLName("buffer"));
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "combinational arc source is not an input");
}

TEST_F(TSNLGraphConstructorTests, unprocessedDNLDoesNotTriggerImplicitConstruction) {
  NLUniverse::get()->setTopDesign(top_);
  naja::DNL::DNLFull unprocessed(top_);
  TSNLGraphConstructor constructor;
  EXPECT_FALSE(naja::DNL::isCreated());
  try {
    constructor.construct(unprocessed);
    FAIL() << "Unprocessed DNL must fail";
  } catch (const NLException& error) {
    EXPECT_NE(std::string::npos, std::string(error.what()).find("expected a processed DNL"));
  }
  EXPECT_FALSE(naja::DNL::isCreated());
  EXPECT_THROW(constructor.getGraph(), NLException);
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

  TSNLGraphConstructor constructor;
  construct(constructor);
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
    TSNLGraphConstructor constructor;
    EXPECT_THROW(constructor.getClockGates(), NLException);
    construct(constructor);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
  ASSERT_EQ(2u, constructor.getClockGates().size());
  EXPECT_EQ(26u, constructor.getGraph().getNodesCount());
  EXPECT_NE(constructor.getClockGates()[0].heldEnable, constructor.getClockGates()[1].heldEnable);
  for (auto* wrapper : {left, right}) {
    EXPECT_EQ(constructor.getNode(SNLOccurrence(wrapper->getInstTerm(parts.clockPort))),
      constructor.getNode(SNLOccurrence(SNLPath(wrapper), parts.clockPort)));
  }
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateReportsWrongPhaseOrSource) {
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
    TSNLGraphConstructor constructor;
    expectCoverage(constructor, "latch analysis is unsupported");
    wrapper->destroy();
  }
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateRebuildUpdatesCoverageAndPreservesInvalidDNLResult) {
  auto parts = decomposedGate(top_);
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto gate = constructor.getClockGates().front();
  auto* clockNet = parts.invInput->getNet();
  parts.invInput->setNet(parts.enablePort->getNet());
  ASSERT_NO_THROW(construct(constructor));
  EXPECT_TRUE(constructor.getClockGates().empty());
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Approximated, constructor.getCoverageReport().front().treatment);
  parts.invInput->setNet(clockNet);
  ASSERT_NO_THROW(construct(constructor));
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  auto* previous = &constructor.getGraph();
  naja::DNL::DNLFull unprocessed(top_);
  EXPECT_THROW(constructor.construct(unprocessed), NLException);
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
  TSNLGraphConstructor constructor;
  construct(constructor);
  ASSERT_EQ(1u, constructor.getClockGates().size());
  EXPECT_EQ(11u + 2 * depth, constructor.getGraph().getNodesCount());
  EXPECT_EQ(10u + 2 * depth, constructor.getGraph().getEdgesCount());
  auto& graph = constructor.getGraph();
  auto c = graph.createClock("root", parts.clockPort, constructor.getNode(SNLOccurrence(parts.clockPort)));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.clockOrigins[constructor.getClockGates().front().output].test(c));
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateApproximatesOtherLatchModels) {
  auto* custom = modeledFlop();
  auto model = SNLDesignModeling::getSequentialModel(custom);
  model.kind = SNLDesignModeling::SequentialModel::Kind::Latch;
  SNLDesignModeling::setSequentialModel(custom, model);
  auto* instance = SNLInstance::create(top_, custom);
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "latch modeled as level-sensitive state without time borrowing");
  instance->destroy();
  instance = SNLInstance::create(top_, NLDB0::getOrCreateDLatch(2));
  expectCoverage(constructor, "latch modeled as level-sensitive state without time borrowing");
  instance->destroy();
  //Do not trust the canonical name alone if the live model was modified.
  auto* canonical = NLDB0::getDLatch();
  model = SNLDesignModeling::getSequentialModel(canonical);
  model.clockedOn.root = model.clockedOn.addOperation(
    SNLDesignModeling::BooleanExpression::Operator::Not, {model.clockedOn.root});
  SNLDesignModeling::setSequentialModel(canonical, model);
  SNLInstance::create(top_, canonical);
  expectCoverage(constructor, "latch modeled as level-sensitive state without time borrowing");
}

TEST_F(TSNLGraphConstructorTests, decomposedClockGateReportsAmbiguousHeldInputs) {
  auto parts = decomposedGate(top_);
  auto* second = SNLInstance::create(top_, NLDB0::getDLatch());
  auto* inverse = SNLInstance::create(top_, parts.inverter->getModel());
  auto* invInput = inverse->getInstTerm(NLDB0::getGateSingleTerm(inverse->getModel()));
  auto* invOutput = inverse->getInstTerm(NLDB0::getGateNTerms(inverse->getModel())->getBit(0));
  invInput->setNet(parts.held->getNet());
  second->getInstTerm(NLDB0::getDLatchData())->setNet(parts.enablePort->getNet());
  connect(top_, {invOutput, second->getInstTerm(NLDB0::getDLatchEnable())});
  connect(top_, {second->getInstTerm(NLDB0::getDLatchOutput()), parts.clock, parts.invInput});
  TSNLGraphConstructor constructor;
  expectCoverage(constructor, "ambiguous clock-gate composition");
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
  TSNLGraphConstructor constructor;
  construct(constructor);
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

TEST_F(TSNLGraphConstructorTests, dividerChainsPreserveDomainsAndDataFanout) {
  for (bool foreign : {false, true}) {
    for (unsigned stages : {1u, 2u}) {
      auto* design = SNLDesign::create(library_);
      auto* root = port(design, "root", SNLTerm::Direction::Input);
      auto* other = port(design, "other", SNLTerm::Direction::Input);
      auto* input = port(design, "input", SNLTerm::Direction::Input);
      auto* source = SNLInstance::create(design, NLDB0::getDFF(), NLName("source"));
      auto* sink = SNLInstance::create(design, NLDB0::getDFF(), NLName("sink"));
      auto* fanout = SNLInstance::create(design, NLDB0::getDFF(), NLName("fanout"));
      connect(design, {foreign ? other : root, term(source, "C")});
      connect(design, {input, term(source, "D")});
      connect(design, {term(source, "Q"), term(sink, "D")});
      if (root->getNet()) term(fanout, "C")->setNet(root->getNet());
      else connect(design, {root, term(fanout, "C")});
      SNLNetComponent* last = root;
      std::vector<SNLInstance*> dividers;
      for (unsigned i = 0; i < stages; ++i) {
        auto* divider = SNLInstance::create(design, NLDB0::getDFF());
        auto* invModel = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
        auto* inv = SNLInstance::create(design, invModel);
        term(divider, "C")->setNet(last->getNet());
        if (!last->getNet()) connect(design, {last, term(divider, "C")});
        connect(design, {term(divider, "Q"), inv->getInstTerm(NLDB0::getGateSingleTerm(invModel))});
        connect(design, {inv->getInstTerm(NLDB0::getGateNTerms(invModel)->getBit(0)), term(divider, "D")});
        last = term(divider, "Q");
        dividers.push_back(divider);
      }
      term(sink, "C")->setNet(last->getNet());
      term(fanout, "D")->setNet(last->getNet());
      TSNLGraphConstructor constructor;
      construct(constructor, design);
      auto& graph = constructor.getGraph();
      auto id = [&](SNLNetComponent* component) { return constructor.getNode(SNLOccurrence(component)); };
      auto a = graph.createClock("a", root, id(root));
      auto b = graph.createClock("b", other, id(other));
      auto result = TClockPropagator::run(graph);
      EXPECT_TRUE(result.unclockedFlopClockPins.empty());
      ASSERT_EQ(stages, result.generatedClockStateArcs.size());
      for (auto* divider : dividers) {
        EXPECT_TRUE(result.clockOrigins[id(term(divider, "C"))].test(a));
        EXPECT_TRUE(result.clockOrigins[id(term(divider, "Q"))].test(a));
      }
      EXPECT_EQ(1u, result.clockOrigins[id(term(sink, "C"))].count());
      EXPECT_TRUE(result.clockOrigins[id(term(sink, "C"))].test(a));
      auto* data = graph.getEdge(edge(graph, id(term(source, "Q")), id(term(sink, "D"))));
      EXPECT_TRUE(data->hasFromClock(foreign ? b : a));
      EXPECT_TRUE(data->hasToClock(a));
      EXPECT_EQ(foreign, data->hasClockCrossing());
      EXPECT_EQ(foreign ? 1 : 0, std::count_if(graph.getEdgesStorage().begin(),
        graph.getEdgesStorage().end(), [](const auto& edge) { return edge.hasClockCrossing(); }));
      auto* shared = graph.getEdge(edge(graph, id(last), id(term(fanout, "D"))));
      EXPECT_TRUE(shared->hasFromClock(a));
      EXPECT_TRUE(shared->hasToClock(a));
      EXPECT_FALSE(shared->hasClockCrossing());
    }
  }
}

TEST_F(TSNLGraphConstructorTests, plainGateFeedbackAndForeignEnableKeepWaveformsSeparate) {
  for (bool foreign : {false, true}) {
    auto* design = SNLDesign::create(library_);
    auto* root = port(design, "root", SNLTerm::Direction::Input);
    auto* other = port(design, "other", SNLTerm::Direction::Input);
    auto* input = port(design, "input", SNLTerm::Direction::Input);
    std::vector<SNLInstance*> flops;
    for (unsigned i = 0; i < 4; ++i) flops.push_back(SNLInstance::create(design, NLDB0::getDFF()));
    auto* andModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
    auto* gate = SNLInstance::create(design, andModel);
    auto* orModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 3);
    auto* control = SNLInstance::create(design, orModel);
    auto* clockIn = gate->getInstTerm(NLDB0::getGateNTerms(andModel)->getBit(0));
    auto* enableIn = gate->getInstTerm(NLDB0::getGateNTerms(andModel)->getBit(1));
    auto* gated = gate->getInstTerm(NLDB0::getGateSingleTerm(andModel));
    connect(design, {root, term(flops[0], "C"), term(flops[1], "C")});
    if (foreign) connect(design, {other, clockIn});
    else clockIn->setNet(root->getNet());
    connect(design, {gated, term(flops[2], "C"), term(flops[3], "C")});
    for (unsigned i = 0; i < 3; ++i) {
      connect(design, {term(flops[i], "Q"), control->getInstTerm(NLDB0::getGateNTerms(orModel)->getBit(i))});
    }
    connect(design, {control->getInstTerm(NLDB0::getGateSingleTerm(orModel)), enableIn});
    connect(design, {input, term(flops[0], "D"), term(flops[1], "D"), term(flops[2], "D")});
    term(flops[3], "D")->setNet(term(flops[2], "Q")->getNet());
    TSNLGraphConstructor constructor;
    construct(constructor, design);
    auto& graph = constructor.getGraph();
    auto id = [&](SNLNetComponent* component) { return constructor.getNode(SNLOccurrence(component)); };
    auto a = graph.createClock("a", root, id(root));
    auto b = graph.createClock("b", other, id(other));
    auto result = TClockPropagator::run(graph);
    EXPECT_TRUE(constructor.getClockGates().empty());
    EXPECT_TRUE(result.unclockedFlopClockPins.empty());
    EXPECT_TRUE(result.generatedClockStateArcs.empty());
    for (unsigned i = 0; i < flops.size(); ++i) {
      auto n = id(term(flops[i], "C"));
      EXPECT_EQ(1u, result.clockOrigins[n].count());
      EXPECT_TRUE(result.clockOrigins[n].test(foreign && i >= 2 ? b : a));
    }
    for (const auto& edge : graph.getEdgesStorage()) {
      EXPECT_FALSE(edge.isDisabled());
      EXPECT_FALSE(edge.hasClockCrossing());
    }
    // Replay the constructor's complete physical topology with shuffled edge
    // creation; propagation must not rely on constructor insertion order.
    std::mt19937 random(731);
    std::vector<TGraph::EdgeId> order(graph.getEdgesCount());
    std::iota(order.begin(), order.end(), 0);
    for (unsigned trial = 0; trial < 32; ++trial) {
      std::shuffle(order.begin(), order.end(), random);
      TGraph replay;
      for (const auto& node : graph.getNodesStorage()) {
        auto n = replay.createNode(nullptr);
        replay.getNode(n)->setCellKind(node.getCellKind());
        replay.getNode(n)->setPinRole(node.getPinRole());
      }
      for (auto i : order) {
        const auto& edge = *graph.getEdge(i);
        auto n = replay.createEdge(edge.getSource(), edge.getTarget(), nullptr);
        replay.getEdge(n)->setKind(edge.getKind());
      }
      for (const auto& capture : graph.getCaptureEdgesStorage()) {
        replay.createCaptureEdge(capture.getSource(), capture.getTarget(), capture.getKind());
      }
      replay.createClock("a", nullptr, id(root));
      replay.createClock("b", nullptr, id(other));
      auto shuffled = TClockPropagator::run(replay);
      EXPECT_EQ(result.clockOrigins, shuffled.clockOrigins);
      EXPECT_EQ(result.unclockedFlopClockPins, shuffled.unclockedFlopClockPins);
      EXPECT_TRUE(shuffled.generatedClockStateArcs.empty());
      for (const auto& edge : replay.getEdgesStorage()) {
        auto original = this->edge(graph, edge.getSource(), edge.getTarget());
        ASSERT_NE(TGraph::kInvalidEdge, original);
        EXPECT_EQ(graph.getEdge(original)->getFromClocks(), edge.getFromClocks());
        EXPECT_EQ(graph.getEdge(original)->getToClocks(), edge.getToClocks());
        EXPECT_EQ(graph.getEdge(original)->getClockGateControlClocks(), edge.getClockGateControlClocks());
        EXPECT_FALSE(edge.isDisabled());
      }
    }
  }
}

TEST_F(TSNLGraphConstructorTests, dividerOccurrencesHaveIndependentOrigins) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* root = port(child, "root", SNLTerm::Direction::Input);
  auto* data = port(child, "data", SNLTerm::Direction::Input);
  auto* output = port(child, "output", SNLTerm::Direction::Output);
  auto* divider = SNLInstance::create(child, NLDB0::getDFF(), NLName("divider"));
  auto* sink = SNLInstance::create(child, NLDB0::getDFF(), NLName("sink"));
  connect(child, {root, term(divider, "C")});
  auto* invModel = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
  auto* inv = SNLInstance::create(child, invModel);
  connect(child, {term(divider, "Q"), term(sink, "C"), inv->getInstTerm(NLDB0::getGateSingleTerm(invModel))});
  connect(child, {inv->getInstTerm(NLDB0::getGateNTerms(invModel)->getBit(0)), term(divider, "D")});
  connect(child, {data, term(sink, "D")});
  connect(child, {term(sink, "Q"), output});
  auto* first = SNLInstance::create(top_, child, NLName("first"));
  auto* second = SNLInstance::create(top_, child, NLName("second"));
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLInstance* wrapper, SNLNetComponent* component) {
    return constructor.getNode(SNLOccurrence(SNLPath(wrapper), component));
  };
  auto a = graph.createClock("a", nullptr, id(first, root));
  auto b = graph.createClock("b", nullptr, id(second, root));
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(2u, result.generatedClockStateArcs.size());
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  for (auto* wrapper : {first, second}) {
    auto n = id(wrapper, term(sink, "C"));
    EXPECT_EQ(1u, result.clockOrigins[n].count());
    EXPECT_TRUE(result.clockOrigins[n].test(wrapper == first ? a : b));
  }
}

TEST_F(TSNLGraphConstructorTests, combinationalDataCycleRejectedWithoutChangingEdges) {
  auto* model = NLDB0::getAssign();
  auto* first = SNLInstance::create(top_, model);
  auto* second = SNLInstance::create(top_, model);
  connect(top_, {first->getInstTerm(NLDB0::getAssignOutput()), second->getInstTerm(NLDB0::getAssignInput())});
  connect(top_, {second->getInstTerm(NLDB0::getAssignOutput()), first->getInstTerm(NLDB0::getAssignInput())});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  EXPECT_THROW(TClockPropagator::run(graph), std::logic_error);
  for (const auto& edge : graph.getEdgesStorage()) EXPECT_FALSE(edge.isDisabled());
}

TEST_F(TSNLGraphConstructorTests, foreignPlainGateEnableDoesNotPolluteDirectDomain) {
  auto* a = port(top_, "a", SNLTerm::Direction::Input);
  auto* b = port(top_, "b", SNLTerm::Direction::Input);
  auto* en = port(top_, "en", SNLTerm::Direction::Input);
  auto* data = port(top_, "data", SNLTerm::Direction::Input);
  auto* source = SNLInstance::create(top_, NLDB0::getDFF());
  auto* middle = SNLInstance::create(top_, NLDB0::getDFF());
  auto* sink = SNLInstance::create(top_, NLDB0::getDFF());
  auto* model = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  auto* gate = SNLInstance::create(top_, model);
  connect(top_, {a, term(source, "C")});
  connect(top_, {en, term(source, "D")});
  connect(top_, {b, term(sink, "C"), gate->getInstTerm(NLDB0::getGateNTerms(model)->getBit(0))});
  connect(top_, {term(source, "Q"), gate->getInstTerm(NLDB0::getGateNTerms(model)->getBit(1))});
  connect(top_, {gate->getInstTerm(NLDB0::getGateSingleTerm(model)), term(middle, "C")});
  connect(top_, {data, term(middle, "D")});
  connect(top_, {term(middle, "Q"), term(sink, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* component) { return constructor.getNode(SNLOccurrence(component)); };
  auto ca = graph.createClock("a", a, id(a));
  auto cb = graph.createClock("b", b, id(b));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_TRUE(result.generatedClockStateArcs.empty());
  EXPECT_EQ(std::vector<TEdge::EdgeId>({edge(graph, id(term(source, "C")), id(term(source, "Q")))}),
    result.controlStateArcs);
  EXPECT_TRUE(result.clockOrigins[id(term(source, "C"))].test(ca));
  for (auto* flop : {middle, sink}) {
    const auto& mask = result.clockOrigins[id(term(flop, "C"))];
    EXPECT_EQ(1u, mask.count());
    EXPECT_TRUE(mask.test(cb));
    EXPECT_FALSE(mask.test(ca));
  }
  auto* transfer = graph.getEdge(edge(graph, id(term(middle, "Q")), id(term(sink, "D"))));
  EXPECT_TRUE(transfer->hasFromClock(cb));
  EXPECT_TRUE(transfer->hasToClock(cb));
  EXPECT_FALSE(transfer->hasClockCrossing());
}

TEST_F(TSNLGraphConstructorTests, muxDataArmsCarryGeneratedClocksAndSelectIsControl) {
  // primary/primary, divided/primary, and primary/primary with a state select.
  for (unsigned mode = 0; mode < 3; ++mode) {
    for (unsigned width : {1u, 3u}) {
      auto* design = SNLDesign::create(library_);
      auto* aPort = port(design, "a", SNLTerm::Direction::Input);
      auto* bPort = port(design, "b", SNLTerm::Direction::Input);
      auto* selectPort = port(design, "select", SNLTerm::Direction::Input);
      auto* source = SNLInstance::create(design, NLDB0::getDFF());
      auto* sink = SNLInstance::create(design, NLDB0::getDFF());
      connect(design, {aPort});
      connect(design, {bPort});
      term(source, "C")->setNet(mode == 0 ? aPort->getNet() : bPort->getNet());
      auto* divider = toggler(design, aPort->getNet());
      auto* model = NLDB0::getOrCreateMux2(width);
      auto* mux = SNLInstance::create(design, model);
      auto* left = mux->getInstTerm(NLDB0::getMux2InputA(model)->getBit(0));
      auto* right = mux->getInstTerm(NLDB0::getMux2InputB(model)->getBit(0));
      auto* select = mux->getInstTerm(NLDB0::getMux2Select(model));
      auto* out = mux->getInstTerm(NLDB0::getMux2Output(model)->getBit(0));
      left->setNet(mode == 1 ? term(divider, "Q")->getNet() : aPort->getNet());
      right->setNet(bPort->getNet());
      if (mode == 2) select->setNet(term(divider, "Q")->getNet());
      else connect(design, {selectPort, select});
      connect(design, {out, term(sink, "C")});
      connect(design, {term(source, "Q"), term(sink, "D")});
      TSNLGraphConstructor constructor;
      construct(constructor, design);
      auto& graph = constructor.getGraph();
      auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
      auto a = graph.createClock("a", aPort, id(aPort));
      auto b = graph.createClock("b", bPort, id(bPort));
      EXPECT_EQ(TEdge::Kind::ClockPass, graph.getEdge(edge(graph, id(left), id(out)))->getKind());
      EXPECT_EQ(TEdge::Kind::ClockPass, graph.getEdge(edge(graph, id(right), id(out)))->getKind());
      EXPECT_EQ(TEdge::Kind::DataOnly, graph.getEdge(edge(graph, id(select), id(out)))->getKind());
      auto result = TClockPropagator::run(graph);
      auto state = edge(graph, id(term(divider, "C")), id(term(divider, "Q")));
      EXPECT_EQ(mode == 1 ? std::vector<TEdge::EdgeId>{state} : std::vector<TEdge::EdgeId>{},
        result.generatedClockStateArcs);
      EXPECT_EQ(mode == 2 ? std::vector<TEdge::EdgeId>{state} : std::vector<TEdge::EdgeId>{},
        result.controlStateArcs);
      EXPECT_TRUE(result.unclockedFlopClockPins.empty());
      const auto& clocks = result.clockOrigins[id(term(sink, "C"))];
      EXPECT_EQ(2u, clocks.count());
      EXPECT_TRUE(clocks.test(a));
      EXPECT_TRUE(clocks.test(b));
      EXPECT_EQ(mode == 1, result.clockOrigins[id(term(divider, "Q"))].any());
      auto* data = graph.getEdge(edge(graph, id(term(source, "Q")), id(term(sink, "D"))));
      EXPECT_EQ(1u, data->getFromClocks().count());
      EXPECT_TRUE(data->hasFromClock(mode == 0 ? a : b));
      EXPECT_EQ(clocks, data->getToClocks());
      EXPECT_TRUE(data->hasClockCrossing());
      if (mode == 1 && width == 1) {
        std::mt19937 random(184);
        std::vector<TEdge::EdgeId> order(graph.getEdgesCount());
        std::iota(order.begin(), order.end(), 0);
        for (unsigned trial = 0; trial < 32; ++trial) {
          std::shuffle(order.begin(), order.end(), random);
          TGraph replay;
          for (const auto& node : graph.getNodesStorage()) {
            auto n = replay.createNode(nullptr);
            replay.getNode(n)->setCellKind(node.getCellKind());
            replay.getNode(n)->setPinRole(node.getPinRole());
          }
          std::vector<TEdge::EdgeId> mapping(order.size());
          for (auto original : order) {
            const auto* arc = graph.getEdge(original);
            auto copy = replay.createEdge(arc->getSource(), arc->getTarget(), nullptr);
            replay.getEdge(copy)->setKind(arc->getKind());
            mapping[original] = copy;
          }
          for (const auto& capture : graph.getCaptureEdgesStorage()) {
            replay.createCaptureEdge(capture.getSource(), capture.getTarget(), capture.getKind());
          }
          replay.createClock("a", nullptr, id(aPort));
          replay.createClock("b", nullptr, id(bPort));
          auto shuffled = TClockPropagator::run(replay);
          EXPECT_EQ(result.clockOrigins, shuffled.clockOrigins);
          EXPECT_EQ(result.unclockedFlopClockPins, shuffled.unclockedFlopClockPins);
          EXPECT_EQ(std::vector<TEdge::EdgeId>{mapping[state]}, shuffled.generatedClockStateArcs);
          EXPECT_EQ(result.controlStateArcs, shuffled.controlStateArcs);
          for (const auto& arc : graph.getEdgesStorage()) {
            auto* copy = replay.getEdge(mapping[arc.getIndex()]);
            EXPECT_EQ(arc.getFromClocks(), copy->getFromClocks());
            EXPECT_EQ(arc.getToClocks(), copy->getToClocks());
            EXPECT_EQ(arc.getClockGateControlClocks(), copy->getClockGateControlClocks());
            EXPECT_FALSE(copy->isDisabled());
          }
        }
      }
    }
  }
}

TEST_F(TSNLGraphConstructorTests, unclassifiedDividerMergesRemainUnclocked) {
  for (bool exclusiveOr : {false, true}) {
    auto* design = SNLDesign::create(library_);
    auto* aPort = port(design, "a", SNLTerm::Direction::Input);
    auto* bPort = port(design, "b", SNLTerm::Direction::Input);
    connect(design, {aPort});
    connect(design, {bPort});
    auto* first = toggler(design, aPort->getNet());
    auto* second = exclusiveOr ? toggler(design, bPort->getNet()) : SNLInstance::create(design, NLDB0::getDFF());
    if (!exclusiveOr) {
      term(second, "C")->setNet(aPort->getNet());
      auto* control = port(design, "control", SNLTerm::Direction::Input);
      connect(design, {control, term(second, "D")});
      connect(design, {term(second, "Q")});
    }
    auto* model = NLDB0::getOrCreateNInputGate(exclusiveOr ? NLDB0::GateType::Xor : NLDB0::GateType::And, 2);
    auto* merge = SNLInstance::create(design, model);
    auto* sink = SNLInstance::create(design, NLDB0::getDFF());
    merge->getInstTerm(NLDB0::getGateNTerms(model)->getBit(0))->setNet(term(first, "Q")->getNet());
    merge->getInstTerm(NLDB0::getGateNTerms(model)->getBit(1))->setNet(term(second, "Q")->getNet());
    connect(design, {merge->getInstTerm(NLDB0::getGateSingleTerm(model)), term(sink, "C")});
    TSNLGraphConstructor constructor;
    construct(constructor, design);
    auto& graph = constructor.getGraph();
    auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
    graph.createClock("a", aPort, id(aPort));
    graph.createClock("b", bPort, id(bPort));
    auto result = TClockPropagator::run(graph);
    EXPECT_EQ(std::vector<TGraph::NodeId>{id(term(sink, "C"))}, result.unclockedFlopClockPins);
    EXPECT_TRUE(result.clockOrigins[id(term(sink, "C"))].none());
    EXPECT_TRUE(result.generatedClockStateArcs.empty());
    EXPECT_TRUE(result.controlStateArcs.empty());
  }
}

TEST_F(TSNLGraphConstructorTests, dividedClockFeedsRecognizedLatchGate) {
  auto parts = decomposedGate(top_);
  auto* aPort = port(top_, "a", SNLTerm::Direction::Input);
  auto* bPort = port(top_, "b", SNLTerm::Direction::Input);
  connect(top_, {aPort});
  auto* divider = toggler(top_, aPort->getNet());
  // Preserve all physical gate wiring; drive the former root net from Q.
  for (auto* pin : {parts.clock, parts.invInput}) pin->setNet(term(divider, "Q")->getNet());
  parts.clockPort->setNet(nullptr);
  auto* enable = SNLInstance::create(top_, NLDB0::getDFF());
  auto* sink = SNLInstance::create(top_, NLDB0::getDFF());
  connect(top_, {bPort, term(enable, "C")});
  term(enable, "Q")->setNet(parts.enablePort->getNet());
  parts.enablePort->setNet(nullptr);
  term(sink, "C")->setNet(parts.output->getNet());
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
  auto a = graph.createClock("a", aPort, id(aPort));
  auto b = graph.createClock("b", bPort, id(bPort));
  auto result = TClockPropagator::run(graph);
  ASSERT_EQ(1u, constructor.getClockGates().size());
  EXPECT_EQ(1u, result.generatedClockStateArcs.size());
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_EQ(1u, result.clockOrigins[id(term(sink, "C"))].count());
  EXPECT_TRUE(result.clockOrigins[id(term(sink, "C"))].test(a));
  auto* control = graph.getEdge(edge(graph, id(term(enable, "Q")), id(parts.data)));
  EXPECT_TRUE(control->hasFromClock(b));
  EXPECT_TRUE(control->getClockGateControlClocks().test(a));
  EXPECT_TRUE(control->hasClockGateControlCrossing());
}

TEST_F(TSNLGraphConstructorTests, dividerDataFanoutCrossesOnlyAtForeignCapture) {
  auto* aPort = port(top_, "a", SNLTerm::Direction::Input);
  auto* bPort = port(top_, "b", SNLTerm::Direction::Input);
  auto* source = SNLInstance::create(top_, NLDB0::getDFF());
  auto* sink = SNLInstance::create(top_, NLDB0::getDFF());
  auto* fanout = SNLInstance::create(top_, NLDB0::getDFF());
  connect(top_, {aPort, term(source, "C")});
  connect(top_, {bPort, term(fanout, "C")});
  auto* divider = toggler(top_, aPort->getNet());
  term(sink, "C")->setNet(term(divider, "Q")->getNet());
  term(fanout, "D")->setNet(term(divider, "Q")->getNet());
  connect(top_, {term(source, "Q"), term(sink, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
  auto a = graph.createClock("a", aPort, id(aPort));
  auto b = graph.createClock("b", bPort, id(bPort));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_EQ(1u, result.generatedClockStateArcs.size());
  EXPECT_EQ(1u, result.clockOrigins[id(term(sink, "C"))].count());
  EXPECT_TRUE(result.clockOrigins[id(term(sink, "C"))].test(a));
  auto crossing = edge(graph, id(term(divider, "Q")), id(term(fanout, "D")));
  EXPECT_TRUE(graph.getEdge(crossing)->hasFromClock(a));
  EXPECT_TRUE(graph.getEdge(crossing)->hasToClock(b));
  for (const auto& arc : graph.getEdgesStorage()) EXPECT_EQ(arc.getIndex() == crossing, arc.hasClockCrossing());
}

TEST_F(TSNLGraphConstructorTests, generatedClockStateFeedbackUsesFixedPoint) {
  auto* root = port(top_, "root", SNLTerm::Direction::Input);
  connect(top_, {root});
  auto* first = toggler(top_, root->getNet());
  auto* second = toggler(top_, term(first, "Q")->getNet());
  auto* sink = SNLInstance::create(top_, NLDB0::getDFF());
  auto* source = SNLInstance::create(top_, NLDB0::getDFF());
  term(sink, "C")->setNet(term(second, "Q")->getNet());
  term(source, "C")->setNet(root->getNet());
  connect(top_, {term(source, "Q"), term(sink, "D")});
  auto* model = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2);
  auto* merge = SNLInstance::create(top_, model);
  // Replace first's feedback with Q1 xor ~Q2. The old inverter remains physical.
  auto* invModel = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
  auto* inverse = SNLInstance::create(top_, invModel);
  inverse->getInstTerm(NLDB0::getGateSingleTerm(invModel))->setNet(term(second, "Q")->getNet());
  merge->getInstTerm(NLDB0::getGateNTerms(model)->getBit(0))->setNet(term(first, "Q")->getNet());
  connect(top_, {inverse->getInstTerm(NLDB0::getGateNTerms(invModel)->getBit(0)),
    merge->getInstTerm(NLDB0::getGateNTerms(model)->getBit(1))});
  connect(top_, {merge->getInstTerm(NLDB0::getGateSingleTerm(model)), term(first, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
  auto a = graph.createClock("a", root, id(root));
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(2u, result.generatedClockStateArcs.size());
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  for (auto* flop : {first, second, sink, source}) {
    EXPECT_EQ(1u, result.clockOrigins[id(term(flop, "C"))].count());
    EXPECT_TRUE(result.clockOrigins[id(term(flop, "C"))].test(a));
  }
  for (const auto& arc : graph.getEdgesStorage()) {
    EXPECT_FALSE(arc.hasClockCrossing());
    EXPECT_FALSE(arc.isDisabled());
  }
}

TEST_F(TSNLGraphConstructorTests, sharedDividerOccurrencesCaptureOppositeDomains) {
  auto* child = SNLDesign::create(library_);
  auto* root = port(child, "root", SNLTerm::Direction::Input);
  auto* data = port(child, "data", SNLTerm::Direction::Input);
  auto* output = port(child, "out", SNLTerm::Direction::Output);
  connect(child, {root});
  auto* divider = toggler(child, root->getNet());
  auto* sink = SNLInstance::create(child, NLDB0::getDFF());
  term(sink, "C")->setNet(term(divider, "Q")->getNet());
  connect(child, {data, term(sink, "D")});
  connect(child, {term(sink, "Q"), output});
  std::vector<SNLInstance*> wrappers, sources;
  std::vector<SNLScalarTerm*> roots;
  for (unsigned i = 0; i < 2; ++i) {
    wrappers.push_back(SNLInstance::create(top_, child));
    sources.push_back(SNLInstance::create(top_, NLDB0::getDFF()));
    roots.push_back(SNLScalarTerm::create(top_, SNLTerm::Direction::Input));
    connect(top_, {roots.back(), wrappers.back()->getInstTerm(root), term(sources.back(), "C")});
  }
  for (unsigned i = 0; i < 2; ++i) connect(top_, {term(sources[1-i], "Q"), wrappers[i]->getInstTerm(data)});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto a = graph.createClock("a", roots[0], constructor.getNode(SNLOccurrence(roots[0])));
  auto b = graph.createClock("b", roots[1], constructor.getNode(SNLOccurrence(roots[1])));
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(2u, result.generatedClockStateArcs.size());
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  for (unsigned i = 0; i < 2; ++i) {
    auto c = constructor.getNode(SNLOccurrence(SNLPath(wrappers[i]), term(sink, "C")));
    auto d = constructor.getNode(SNLOccurrence(SNLPath(wrappers[i]), term(sink, "D")));
    EXPECT_EQ(1u, result.clockOrigins[c].count());
    EXPECT_TRUE(result.clockOrigins[c].test(i == 0 ? a : b));
    ASSERT_EQ(1u, graph.getNode(d)->getInEdges().size());
    auto* transfer = graph.getEdge(*graph.getNode(d)->getInEdges().begin());
    EXPECT_TRUE(transfer->hasFromClock(i == 0 ? b : a));
    EXPECT_TRUE(transfer->hasToClock(i == 0 ? a : b));
    EXPECT_TRUE(transfer->hasClockCrossing());
  }
}

TEST_F(TSNLGraphConstructorTests, nestedClockEnableFeedbackKeepsOneDomain) {
  auto* root = port(top_, "root", SNLTerm::Direction::Input);
  auto* input = port(top_, "input", SNLTerm::Direction::Input);
  std::vector<SNLInstance*> flops;
  for (unsigned i = 0; i < 4; ++i) flops.push_back(SNLInstance::create(top_, NLDB0::getDFF()));
  auto* andModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  auto* orModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2);
  auto* wakeModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 3);
  auto* gate = SNLInstance::create(top_, andModel);
  auto* enable = SNLInstance::create(top_, andModel);
  auto* busy = SNLInstance::create(top_, orModel);
  auto* wake = SNLInstance::create(top_, wakeModel);
  auto in = [&](SNLInstance* cell, unsigned bit) {
    return cell->getInstTerm(NLDB0::getGateNTerms(cell->getModel())->getBit(bit));
  };
  auto out = [&](SNLInstance* cell) { return cell->getInstTerm(NLDB0::getGateSingleTerm(cell->getModel())); };
  connect(top_, {root, term(flops[0], "C"), term(flops[1], "C"), in(gate, 0)});
  connect(top_, {out(gate), term(flops[2], "C"), term(flops[3], "C")});
  connect(top_, {term(flops[0], "Q"), in(enable, 0)});
  connect(top_, {term(flops[1], "Q"), in(busy, 1)});
  connect(top_, {term(flops[2], "Q"), in(wake, 2), term(flops[3], "D")});
  connect(top_, {out(wake), in(busy, 0)});
  connect(top_, {out(busy), in(enable, 1)});
  connect(top_, {out(enable), in(gate, 1)});
  connect(top_, {input, in(wake, 0), in(wake, 1), term(flops[0], "D"), term(flops[1], "D"), term(flops[2], "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
  auto a = graph.createClock("a", root, id(root));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_TRUE(result.generatedClockStateArcs.empty());
  std::vector<TEdge::EdgeId> controls;
  for (unsigned i = 0; i < 3; ++i) controls.push_back(edge(graph, id(term(flops[i], "C")), id(term(flops[i], "Q"))));
  std::sort(controls.begin(), controls.end());
  EXPECT_EQ(controls, result.controlStateArcs);
  for (auto* flop : flops) {
    EXPECT_EQ(1u, result.clockOrigins[id(term(flop, "C"))].count());
    EXPECT_TRUE(result.clockOrigins[id(term(flop, "C"))].test(a));
  }
  std::mt19937 random(643);
  std::vector<TEdge::EdgeId> order(graph.getEdgesCount());
  std::iota(order.begin(), order.end(), 0);
  for (unsigned trial = 0; trial < 32; ++trial) {
    std::shuffle(order.begin(), order.end(), random);
    TGraph replay;
    for (const auto& node : graph.getNodesStorage()) {
      auto n = replay.createNode(nullptr);
      replay.getNode(n)->setCellKind(node.getCellKind());
      replay.getNode(n)->setPinRole(node.getPinRole());
    }
    for (auto original : order) {
      auto* arc = graph.getEdge(original);
      auto copy = replay.createEdge(arc->getSource(), arc->getTarget(), nullptr);
      replay.getEdge(copy)->setKind(arc->getKind());
    }
    for (const auto& capture : graph.getCaptureEdgesStorage()) replay.createCaptureEdge(capture.getSource(), capture.getTarget());
    replay.createClock("a", nullptr, id(root));
    auto shuffled = TClockPropagator::run(replay);
    EXPECT_EQ(result.clockOrigins, shuffled.clockOrigins);
    EXPECT_TRUE(shuffled.generatedClockStateArcs.empty());
    EXPECT_TRUE(shuffled.unclockedFlopClockPins.empty());
    EXPECT_EQ(3u, shuffled.controlStateArcs.size());
    EXPECT_TRUE(std::is_sorted(shuffled.controlStateArcs.begin(), shuffled.controlStateArcs.end()));
    for (const auto& arc : replay.getEdgesStorage()) {
      EXPECT_FALSE(arc.hasClockCrossing());
      EXPECT_FALSE(arc.isDisabled());
    }
  }
}

TEST_F(TSNLGraphConstructorTests, gateDecomposedMuxReportsSuppressedDivider) {
  auto* aPort = port(top_, "a", SNLTerm::Direction::Input);
  auto* bPort = port(top_, "b", SNLTerm::Direction::Input);
  auto* select = port(top_, "select", SNLTerm::Direction::Input);
  connect(top_, {aPort});
  auto* divider = toggler(top_, aPort->getNet());
  auto* invModel = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
  auto* andModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  auto* orModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2);
  auto* inverse = SNLInstance::create(top_, invModel);
  auto* left = SNLInstance::create(top_, andModel);
  auto* right = SNLInstance::create(top_, andModel);
  auto* merge = SNLInstance::create(top_, orModel);
  auto* sink = SNLInstance::create(top_, NLDB0::getDFF());
  auto in = [&](SNLInstance* cell, unsigned bit) { return cell->getInstTerm(NLDB0::getGateNTerms(cell->getModel())->getBit(bit)); };
  auto out = [&](SNLInstance* cell) { return cell->getInstTerm(NLDB0::getGateSingleTerm(cell->getModel())); };
  in(left, 0)->setNet(term(divider, "Q")->getNet());
  connect(top_, {bPort, in(right, 0)});
  connect(top_, {select, in(left, 1), inverse->getInstTerm(NLDB0::getGateSingleTerm(invModel))});
  connect(top_, {inverse->getInstTerm(NLDB0::getGateNTerms(invModel)->getBit(0)), in(right, 1)});
  connect(top_, {out(left), in(merge, 0)});
  connect(top_, {out(right), in(merge, 1)});
  connect(top_, {out(merge), term(sink, "C")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* pin) { return constructor.getNode(SNLOccurrence(pin)); };
  auto a = graph.createClock("a", aPort, id(aPort));
  auto b = graph.createClock("b", bPort, id(bPort));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.generatedClockStateArcs.empty());
  EXPECT_EQ(std::vector<TEdge::EdgeId>({edge(graph, id(term(divider, "C")), id(term(divider, "Q")))}), result.controlStateArcs);
  EXPECT_EQ(1u, result.clockOrigins[id(term(sink, "C"))].count());
  EXPECT_TRUE(result.clockOrigins[id(term(sink, "C"))].test(b));
  EXPECT_FALSE(result.clockOrigins[id(term(sink, "C"))].test(a));
}

TEST_F(TSNLGraphConstructorTests, asyncMemoryPreservesWriteDomainAndReadAddressDependency) {
  auto* write = port(top_, "write", SNLTerm::Direction::Input);
  auto* read = port(top_, "read", SNLTerm::Direction::Input);
  auto* address = SNLInstance::create(top_, NLDB0::getDFF());
  auto* capture = SNLInstance::create(top_, NLDB0::getDFF());
  auto* memory = SNLInstance::create(top_, NLDB0::getOrCreateMemory({3, 8, 3, 1, 1}));
  auto* model = memory->getModel();
  auto* data = memory->getInstTerm(NLDB0::getMemoryReadData(model)->getBit(0));
  auto* addr = memory->getInstTerm(NLDB0::getMemoryReadAddress(model)->getBit(0));
  connect(top_, {write, term(memory, "CLK")});
  connect(top_, {read, term(capture, "C"), term(address, "C")});
  connect(top_, {term(address, "Q"), addr});
  connect(top_, {data, term(capture, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* t) { return constructor.getNode(SNLOccurrence(t)); };
  const auto w = graph.createClock("write", write, id(write));
  const auto r = graph.createClock("read", read, id(read));
  auto result = TClockPropagator::run(graph);
  auto transfer = edge(graph, id(data), id(term(capture, "D")));
  ASSERT_NE(TGraph::kInvalidEdge, transfer);
  EXPECT_TRUE(graph.getEdge(transfer)->getFromClocks().test(w));
  EXPECT_TRUE(graph.getEdge(transfer)->getFromClocks().test(r));
  EXPECT_EQ(1u, graph.getEdge(transfer)->getToClocks().count());
  EXPECT_TRUE(graph.getEdge(transfer)->getToClocks().test(r));
  EXPECT_TRUE(graph.getEdge(transfer)->hasClockCrossing());
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(addr), id(data)));
  for (auto* bus : {NLDB0::getMemoryWriteAddress(model), NLDB0::getMemoryWriteData(model),
      NLDB0::getMemoryWriteEnable(model)}) {
    for (auto* bit : bus->getBusBits()) {
      auto* pin = memory->getInstTerm(bit);
      ASSERT_EQ(1u, graph.getNode(id(pin))->getCaptureInEdges().size());
      auto* relationship = graph.getCaptureEdge(graph.getNode(id(pin))->getCaptureInEdges().front());
      EXPECT_EQ(id(term(memory, "CLK")), relationship->getSource());
    }
  }

}

TEST_F(TSNLGraphConstructorTests, registeredMemoryReadOnOneClockHasNoCrossing) {
  auto* clock = port(top_, "clock", SNLTerm::Direction::Input);
  auto* memory = SNLInstance::create(top_, NLDB0::getOrCreateMemory({3, 8, 3, 1, 1}));
  auto* output = SNLInstance::create(top_, NLDB0::getDFF());
  auto* data = memory->getInstTerm(NLDB0::getMemoryReadData(memory->getModel())->getBit(0));
  connect(top_, {clock, term(memory, "CLK"), term(output, "C")});
  connect(top_, {data, term(output, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* t) { return constructor.getNode(SNLOccurrence(t)); };
  auto c = graph.createClock("clock", clock, id(clock));
  TClockPropagator::run(graph);
  auto transfer = edge(graph, id(data), id(term(output, "D")));
  ASSERT_NE(TGraph::kInvalidEdge, transfer);
  EXPECT_EQ(1u, graph.getEdge(transfer)->getFromClocks().count());
  EXPECT_TRUE(graph.getEdge(transfer)->getFromClocks().test(c));
  EXPECT_EQ(graph.getEdge(transfer)->getFromClocks(), graph.getEdge(transfer)->getToClocks());
  EXPECT_FALSE(graph.getEdge(transfer)->hasClockCrossing());
  EXPECT_TRUE(constructor.getCoverageReport().empty());
}

TEST_F(TSNLGraphConstructorTests, inoutHierarchyAndMultipleDriversRetainAllConnectivity) {
  auto* io = port(top_, "io", SNLTerm::Direction::InOut);
  auto* other = port(top_, "driver", SNLTerm::Direction::Input);
  auto* sink = port(top_, "sink", SNLTerm::Direction::Output);
  auto* child = SNLDesign::create(library_);
  auto* childIO = port(child, "io", SNLTerm::Direction::InOut);
  auto* childOut = port(child, "out", SNLTerm::Direction::Output);
  connect(child, {childIO, childOut});
  auto* wrapper = SNLInstance::create(top_, child);
  connect(top_, {io, other, sink, term(wrapper, "io")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* t) { return constructor.getNode(SNLOccurrence(t)); };
  for (auto* driver : {io, other}) {
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(SNLOccurrence(driver)), id(sink)));
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(SNLOccurrence(driver)), id(term(wrapper, "io"))));
  }
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(SNLOccurrence(term(wrapper, "io"))), id(io)));
  EXPECT_EQ(id(term(wrapper, "io")), constructor.getNode(SNLOccurrence(SNLPath(wrapper), childIO)));
  EXPECT_EQ(2u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::InOut));
  EXPECT_EQ(1u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::MultipleDrivers));
  for (const auto& e : graph.getEdgesStorage()) EXPECT_NE(e.getSource(), e.getTarget());
}

TEST_F(TSNLGraphConstructorTests, opaqueBlackboxOutputCanSeedClockAndInoutLeafIsReported) {
  auto* macro = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("macro"));
  port(macro, "input", SNLTerm::Direction::Input);
  port(macro, "output", SNLTerm::Direction::Output);
  auto* source = SNLInstance::create(top_, macro);
  auto* flop = SNLInstance::create(top_, NLDB0::getDFF());
  connect(top_, {term(source, "output"), term(flop, "C")});
  auto* pad = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("pad"));
  port(pad, "io", SNLTerm::Direction::InOut);
  auto* instance = SNLInstance::create(top_, pad);
  TSNLGraphConstructor constructor;
  construct(constructor);
  ASSERT_EQ(2u, constructor.getCoverageReport().size());
  EXPECT_EQ(2u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::Blackbox));
  EXPECT_EQ(0u, constructor.getCoverageCounts().count(TSNLGraphConstructor::CoverageReason::InOut));
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* t) { return constructor.getNode(SNLOccurrence(t)); };
  auto c = graph.createClock("generated", term(source, "output"), id(term(source, "output")));
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.clockOrigins[id(term(flop, "C"))].test(c));
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_EQ(0u, graph.getNode(id(term(source, "input")))->getOutDegree());
  EXPECT_EQ(0u, graph.getNode(id(term(instance, "io")))->getOutDegree());
  source->destroy();
  instance->destroy();
  construct(constructor);
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  EXPECT_TRUE(constructor.getCoverageCounts().empty());
}

TEST_F(TSNLGraphConstructorTests, scalarAndBusPlainLatchesCaptureForeignDataBesideClockGate) {
  for (size_t width : {1u, 2u}) {
    auto* child = SNLDesign::create(library_);
    auto parts = decomposedGate(child);
    auto* foreign = port(child, "foreign", SNLTerm::Direction::Input);
    auto* source = SNLInstance::create(child, NLDB0::getDFF());
    auto* latch = SNLInstance::create(child, NLDB0::getOrCreateDLatch(width));
    auto* sink = SNLInstance::create(child, NLDB0::getDFF());
    auto* gate = latch->getInstTerm(latch->getModel()->getScalarTerm(NLName("E")));
    ASSERT_NE(nullptr, gate);
    gate->setNet(parts.clockPort->getNet());
    // Source data launches in the foreign domain, latch captures in root.
    connect(child, {foreign, term(source, "C")});
    term(sink, "C")->setNet(parts.clockPort->getNet());
    std::vector<SNLInstTerm*> dataPins, outputPins;
    for (auto* pin : latch->getInstTerms()) {
      if (SNLDesignModeling::getTermRole(pin) == SNLDesignModeling::SNLTermRole::DataInput) dataPins.push_back(pin);
      if (SNLDesignModeling::getTermRole(pin) == SNLDesignModeling::SNLTermRole::DataOutput) outputPins.push_back(pin);
    }
    connect(child, {term(source, "Q")});
    for (auto* pin : dataPins) pin->setNet(term(source, "Q")->getNet());
    connect(child, {outputPins.front(), term(sink, "D")});
    auto* wrapper = SNLInstance::create(top_, child);
    TSNLGraphConstructor constructor;
    construct(constructor);
    ASSERT_EQ(1u, constructor.getClockGates().size());
    ASSERT_EQ(1u, constructor.getCoverageReport().size());
    EXPECT_EQ(TSNLGraphConstructor::Treatment::Approximated, constructor.getCoverageReport().front().treatment);
    EXPECT_EQ(SNLOccurrence(SNLPath(wrapper), latch), constructor.getCoverageReport().front().occurrence);
    auto& graph = constructor.getGraph();
    auto id = [&](SNLNetComponent* t) { return constructor.getNode(SNLOccurrence(SNLPath(wrapper), t)); };
    auto a = graph.createClock("root", parts.clockPort, id(parts.clockPort));
    auto b = graph.createClock("foreign", foreign, id(foreign));
    TClockPropagator::run(graph);
    for (auto* pin : dataPins) {
      auto transfer = edge(graph, id(term(source, "Q")), id(pin));
      ASSERT_NE(TGraph::kInvalidEdge, transfer);
      EXPECT_TRUE(graph.getEdge(transfer)->getFromClocks().test(b));
      EXPECT_TRUE(graph.getEdge(transfer)->getToClocks().test(a));
      EXPECT_TRUE(graph.getEdge(transfer)->hasClockCrossing());
    }
    auto transfer = edge(graph, id(outputPins.front()), id(term(sink, "D")));
    ASSERT_NE(TGraph::kInvalidEdge, transfer);
    EXPECT_TRUE(graph.getEdge(transfer)->getFromClocks().test(a));
    EXPECT_FALSE(graph.getEdge(transfer)->hasClockCrossing());
    wrapper->destroy();
  }
}

TEST_F(TSNLGraphConstructorTests, incompleteLeafRollsBackInternalArcsAndPublishesCoverageAtomically) {
  auto* model = modeledFlop();
  auto* extra = port(model, "extra", SNLTerm::Direction::Input);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataInput);
  auto* instance = SNLInstance::create(top_, model);
  TSNLGraphConstructor constructor;
  EXPECT_THROW(constructor.getCoverageReport(), NLException);
  EXPECT_THROW(constructor.getCoverageCounts(), NLException);
  construct(constructor);
  EXPECT_EQ(0u, constructor.getGraph().getEdgesCount());
  EXPECT_EQ(0u, constructor.getGraph().getCaptureEdgesCount());
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(TSNLGraphConstructor::CoverageReason::IncompleteModeling, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, constructor.getCoverageReport().front().treatment);
  EXPECT_EQ("flop", constructor.getCoverageReport().front().modelName);
  auto* previous = &constructor.getGraph();
  naja::DNL::DNLFull unprocessed(top_);
  EXPECT_THROW(constructor.construct(unprocessed), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(SNLOccurrence(instance), constructor.getCoverageReport().front().occurrence);
}

TEST_F(TSNLGraphConstructorTests, independentMemoryWriteClocksLaunchEveryAsyncReadOutput) {
  NLDB0::MemorySignature signature {3, 8, 3, 2, 2};
  signature.independentWriteClocks = true;
  auto* model = NLDB0::getOrCreateMemory(signature);
  auto* memory = SNLInstance::create(top_, model);
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  auto& graph = constructor.getGraph();
  auto id = [&](SNLBitTerm* term) { return constructor.getNode(SNLOccurrence(memory->getInstTerm(term))); };
  auto* clocks = model->getBusTerm(NLName("WCLK"));
  auto a = graph.createClock("a", memory->getInstTerm(clocks->getBit(0)), id(clocks->getBit(0)));
  auto b = graph.createClock("b", memory->getInstTerm(clocks->getBit(1)), id(clocks->getBit(1)));
  std::vector<TGraph::EdgeId> observations;
  for (auto* bit : NLDB0::getMemoryReadData(model)->getBusBits()) {
    auto observer = graph.createNode(nullptr);
    observations.push_back(graph.createEdge(id(bit), observer, nullptr));
  }
  TClockPropagator::run(graph);
  for (auto e : observations) {
    EXPECT_EQ(2u, graph.getEdge(e)->getFromClocks().count());
    EXPECT_TRUE(graph.getEdge(e)->getFromClocks().test(a));
    EXPECT_TRUE(graph.getEdge(e)->getFromClocks().test(b));
  }
  for (size_t port = 0; port < 2; ++port) {
    auto* mask = memory->getInstTerm(model->getBusTerm(NLName("WMASK"))->getBit(port * 3));
    auto maskId = constructor.getNode(SNLOccurrence(mask));
    ASSERT_EQ(1u, graph.getNode(maskId)->getCaptureInEdges().size());
    EXPECT_EQ(id(clocks->getBit(port)), graph.getCaptureEdge(
      graph.getNode(maskId)->getCaptureInEdges().front())->getSource());
  }
}

TEST_F(TSNLGraphConstructorTests, memoryResetRoleMatchesFlopWithoutAsyncCapture) {
  for (auto mode : {NLDB0::MemoryResetMode::AsyncLow, NLDB0::MemoryResetMode::AsyncHigh,
      NLDB0::MemoryResetMode::SyncLow, NLDB0::MemoryResetMode::SyncHigh}) {
    NLDB0::MemorySignature signature {3, 8, 3, 1, 1};
    signature.resetMode = mode;
    auto* memory = SNLInstance::create(top_, NLDB0::getOrCreateMemory(signature));
    TSNLGraphConstructor constructor;
    construct(constructor);
    EXPECT_TRUE(constructor.getCoverageReport().empty());
    auto id = constructor.getNode(SNLOccurrence(term(memory, "RST")));
    const auto* node = constructor.getGraph().getNode(id);
    bool async = mode == NLDB0::MemoryResetMode::AsyncLow || mode == NLDB0::MemoryResetMode::AsyncHigh;
    EXPECT_EQ(async ? TNode::PinRole::AsyncReset : TNode::PinRole::SyncReset, node->getPinRole());
    EXPECT_EQ(async ? 0u : 1u, node->getCaptureInEdges().size());
    memory->destroy();
  }
}

TEST_F(TSNLGraphConstructorTests, unsupportedLatchIsOpaqueAndMissingMemoryTimingIsReported) {
  auto* model = modeledFlop();
  auto seq = SNLDesignModeling::getSequentialModel(model);
  seq.kind = SNLDesignModeling::SequentialModel::Kind::Latch;
  seq.states.front().clear = SNLDesignModeling::BooleanExpression{};
  SNLDesignModeling::setSequentialModel(model, seq);
  auto* latch = SNLInstance::create(top_, model);
  TSNLGraphConstructor constructor;
  construct(constructor);
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(TSNLGraphConstructor::CoverageReason::UnsupportedLatch, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, constructor.getCoverageReport().front().treatment);
  EXPECT_EQ(0u, constructor.getGraph().getEdgesCount());
  latch->destroy();
  // A generic interface has no read-clock field. Missing asynchronous timing
  // cannot prove a synchronous macro's read clock; report it instead.
  auto* macro = SNLDesign::create(model->getLibrary(), SNLDesign::Type::Primitive, NLName("memory"));
  auto* c = port(macro, "c", SNLTerm::Direction::Input);
  auto* a = port(macro, "a", SNLTerm::Direction::Input);
  auto* d = port(macro, "d", SNLTerm::Direction::Output);
  SNLDesignModeling::MemoryInterface memory;
  memory.width = memory.depth = memory.abits = 1;
  memory.clock = c;
  memory.readPorts.push_back({{a}, {d}, {}});
  SNLDesignModeling::setMemoryInterface(macro, memory);
  SNLInstance::create(top_, macro);
  construct(constructor);
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(TSNLGraphConstructor::CoverageReason::Memory, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, constructor.getCoverageReport().front().treatment);
  EXPECT_EQ(0u, constructor.getGraph().getEdgesCount());
}

TEST_F(TSNLGraphConstructorTests, multipleDriversDoNotProveClockGatePhase) {
  auto parts = decomposedGate(top_);
  auto* second = port(top_, "second", SNLTerm::Direction::Input);
  second->setNet(parts.clockPort->getNet());
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_TRUE(constructor.getClockGates().empty());
  EXPECT_EQ(1u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::MultipleDrivers));
  EXPECT_EQ(1u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::UnsupportedLatch));
  auto id = [&](SNLNetComponent* term) { return constructor.getNode(SNLOccurrence(term)); };
  auto& graph = constructor.getGraph();
  for (auto* input : {parts.clock, parts.invInput}) {
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(parts.clockPort), id(input)));
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(second), id(input)));
  }
  for (const auto& entry : constructor.getCoverageReport()) {
    if (entry.reason == TSNLGraphConstructor::CoverageReason::MultipleDrivers) {
      EXPECT_EQ(SNLOccurrence(parts.clockPort->getNet()), entry.occurrence);
      EXPECT_EQ(TSNLGraphConstructor::Treatment::AllDrivers, entry.treatment);
    }
  }
}

TEST_F(TSNLGraphConstructorTests, genericMemoryReadDependenciesMustMatchInterface) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("storage"));
  auto* clock = port(model, "clock", SNLTerm::Direction::Input);
  auto* address = port(model, "address", SNLTerm::Direction::Input);
  auto* input = port(model, "input", SNLTerm::Direction::Input);
  auto* output = port(model, "output", SNLTerm::Direction::Output);
  SNLDesignModeling::MemoryInterface memory;
  memory.width = memory.depth = memory.abits = 1;
  memory.clock = clock;
  memory.readPorts.push_back({{address}, {output}, {}});
  memory.writePorts.push_back({{address}, {input}, {}, {}, {}, nullptr});
  SNLDesignModeling::setMemoryInterface(model, memory);
  SNLDesignModeling::addCombinatorialArcs({address}, {output});
  auto* instance = SNLInstance::create(top_, model);
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  EXPECT_EQ(2u, constructor.getGraph().getEdgesCount()); //Address->data and clock->data.
  EXPECT_EQ(2u, constructor.getGraph().getCaptureEdgesCount());
  auto id = [&](SNLBitTerm* bit) { return constructor.getNode(SNLOccurrence(instance->getInstTerm(bit))); };
  EXPECT_NE(TGraph::kInvalidEdge, edge(constructor.getGraph(), id(address), id(output)));
  // A write-through dependency cannot be silently dropped from a generic macro.
  SNLDesignModeling::addCombinatorialArcs({input}, {output});
  construct(constructor);
  ASSERT_EQ(1u, constructor.getCoverageReport().size());
  EXPECT_EQ(TSNLGraphConstructor::CoverageReason::Memory, constructor.getCoverageReport().front().reason);
  EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, constructor.getCoverageReport().front().treatment);
  EXPECT_EQ(0u, constructor.getGraph().getEdgesCount());
  EXPECT_EQ(0u, constructor.getGraph().getCaptureEdgesCount());
}

TEST_F(TSNLGraphConstructorTests, onDemandConstructionCreatesAndReusesDNL) {
  auto* clock = port(top_, "clock", SNLTerm::Direction::Input);
  auto* flop = SNLInstance::create(top_, NLDB0::getDFF());
  connect(top_, {clock, term(flop, "C")});
  NLUniverse::get()->setTopDesign(top_);
  TSNLGraphConstructor constructor;
  EXPECT_FALSE(naja::DNL::isCreated());
  ASSERT_NO_THROW(constructor.construct());
  ASSERT_TRUE(naja::DNL::isCreated());
  const auto* dnl = naja::DNL::get();
  EXPECT_EQ(dnl->getNBterms(), constructor.getGraph().getNodesCount());
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  auto clockPin = constructor.getNode(SNLOccurrence(term(flop, "C")));
  ASSERT_NO_THROW(constructor.construct(*dnl));
  EXPECT_EQ(dnl, naja::DNL::get());
  EXPECT_EQ(clockPin, constructor.getNode(SNLOccurrence(term(flop, "C"))));
  ASSERT_NO_THROW(constructor.construct());
  EXPECT_EQ(dnl, naja::DNL::get());
  // An active DNL for another universe top must not be silently reused.
  auto* other = SNLDesign::create(library_, NLName("other"));
  NLUniverse::get()->setTopDesign(other);
  auto* previous = &constructor.getGraph();
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
}

TEST_F(TSNLGraphConstructorTests, explicitDNLSnapshotIsIndependentOfActiveDNLAndGraphOutlivesIt) {
  auto* clock = port(top_, "clock", SNLTerm::Direction::Input);
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* input = port(child, "clock", SNLTerm::Direction::Input);
  auto* flop = SNLInstance::create(child, NLDB0::getDFF());
  connect(child, {input, term(flop, "C")});
  auto* wrapper = SNLInstance::create(top_, child);
  connect(top_, {clock, wrapper->getInstTerm(input)});
  NLUniverse::get()->setTopDesign(top_);
  TSNLGraphConstructor constructor;
  {
    const naja::DNL::DNLFull snapshot(*naja::DNL::get());
    naja::DNL::destroy();
    NLUniverse::get()->setTopDesign(SNLDesign::create(library_, NLName("other")));
    ASSERT_NO_THROW(constructor.construct(snapshot));
    EXPECT_FALSE(naja::DNL::isCreated());
    EXPECT_EQ(snapshot.getNBterms(), constructor.getGraph().getNodesCount());
  }
  auto& graph = constructor.getGraph();
  auto root = constructor.getNode(SNLOccurrence(clock));
  auto pin = constructor.getNode(SNLOccurrence(SNLPath(wrapper), term(flop, "C")));
  auto c = graph.createClock("clock", clock, root);
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.clockOrigins[pin].test(c));
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  EXPECT_FALSE(naja::DNL::isCreated());
}

TEST_F(TSNLGraphConstructorTests, nestedSharedOccurrencesKeepIndependentTerminalIdentity) {
  auto* cell = SNLDesign::create(library_, NLName("cell"));
  auto* clock = port(cell, "clock", SNLTerm::Direction::Input);
  auto* flop = SNLInstance::create(cell, NLDB0::getDFF());
  connect(cell, {clock, term(flop, "C")});
  auto* branch = SNLDesign::create(library_, NLName("branch"));
  auto* input = port(branch, "clock", SNLTerm::Direction::Input);
  auto* first = SNLInstance::create(branch, cell);
  auto* second = SNLInstance::create(branch, cell);
  connect(branch, {input, first->getInstTerm(clock), second->getInstTerm(clock)});
  auto* left = SNLInstance::create(top_, branch);
  auto* right = SNLInstance::create(top_, branch);
  auto* a = port(top_, "a", SNLTerm::Direction::Input);
  auto* b = port(top_, "b", SNLTerm::Direction::Input);
  connect(top_, {a, left->getInstTerm(input)});
  connect(top_, {b, right->getInstTerm(input)});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto ca = graph.createClock("a", a, constructor.getNode(SNLOccurrence(a)));
  auto cb = graph.createClock("b", b, constructor.getNode(SNLOccurrence(b)));
  auto result = TClockPropagator::run(graph);
  std::vector<TGraph::NodeId> ids;
  for (auto* outer : {left, right}) {
    for (auto* inner : {first, second}) {
      SNLPath path(SNLPath(outer), inner);
      auto id = constructor.getNode(SNLOccurrence(path, term(flop, "C")));
      ASSERT_NE(TGraph::kInvalidNode, id);
      ids.push_back(id);
      EXPECT_EQ(1u, result.clockOrigins[id].count());
      EXPECT_TRUE(result.clockOrigins[id].test(outer == left ? ca : cb));
      EXPECT_EQ(constructor.getNode(SNLOccurrence(SNLPath(outer), inner->getInstTerm(clock))),
        constructor.getNode(SNLOccurrence(path, clock)));
    }
  }
  std::sort(ids.begin(), ids.end());
  EXPECT_EQ(ids.end(), std::unique(ids.begin(), ids.end()));
  EXPECT_TRUE(constructor.getCoverageReport().empty());
#if GTEST_HAS_DEATH_TEST
  // Re-execute in a fresh process: lookup hashes may change with addresses,
  // while observable node/edge creation and capture relationships must agree.
  auto fingerprint = [&]() {
    uint64_t digest = 1469598103934665603ULL;
    auto mix = [&](uint64_t value) { digest = (digest ^ value) * 1099511628211ULL; };
    for (const auto& node : graph.getNodesStorage()) {
      mix(node.getIndex());
      mix(static_cast<uint64_t>(node.getCellKind()));
      mix(static_cast<uint64_t>(node.getPinRole()));
    }
    for (const auto& edge : graph.getEdgesStorage()) {
      mix(edge.getSource());
      mix(edge.getTarget());
      mix(static_cast<uint64_t>(edge.getKind()));
    }
    for (const auto& capture : graph.getCaptureEdgesStorage()) {
      mix(capture.getSource());
      mix(capture.getTarget());
      mix(static_cast<uint64_t>(capture.getKind()));
    }
    return "graph=" + std::to_string(digest);
  };
  const auto previousStyle = GTEST_FLAG_GET(death_test_style);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT({
    std::fputs(fingerprint().c_str(), stderr);
    std::_Exit(0);
  }, ::testing::ExitedWithCode(0), "^" + fingerprint() + "$");
  GTEST_FLAG_SET(death_test_style, previousStyle);
#endif
}

TEST_F(TSNLGraphConstructorTests, declaredMultiSourceAndGeneratedClocksAreImported) {
  auto* roots = SNLBusTerm::create(top_, SNLTerm::Direction::Input, 1, 0, NLName("roots"));
  auto* generated = port(top_, "generated", SNLTerm::Direction::Input);
  auto* primary = NLClock::createPrimary(top_, NLName("primary"), 10.0, "ns", {roots->getBit(1), roots->getBit(0)});
  auto* divided = NLClock::createGenerated(top_, NLName("divided"), primary, roots->getBit(1), 2, 1, false, {generated});
  auto* producer = SNLInstance::create(top_, NLDB0::getDFF(), NLName("producer"));
  auto* consumer = SNLInstance::create(top_, NLDB0::getDFF(), NLName("consumer"));
  auto* other = SNLInstance::create(top_, NLDB0::getDFF(), NLName("other"));
  connect(top_, {roots->getBit(1), term(producer, "C")});
  connect(top_, {roots->getBit(0), term(consumer, "C")});
  connect(top_, {generated, term(other, "C")});
  connect(top_, {term(producer, "Q"), term(consumer, "D"), term(other, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  ASSERT_EQ(2u, graph.getClocksCount());
  EXPECT_EQ(primary, graph.getClock(0)->getNLClock());
  EXPECT_EQ(divided, graph.getClock(1)->getNLClock());
  EXPECT_EQ(primary, graph.getClock(1)->getNLClock()->getMaster());
  EXPECT_DOUBLE_EQ(20.0, graph.getClock(1)->getNLClock()->getPeriod());
  ASSERT_EQ(2u, graph.getClock(0)->getSeeds().size());
  EXPECT_EQ(constructor.getNode(SNLOccurrence(roots->getBit(0))), graph.getClock(0)->getSeeds()[1].node);
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  auto q = constructor.getNode(SNLOccurrence(term(producer, "Q")));
  auto d = constructor.getNode(SNLOccurrence(term(consumer, "D")));
  auto otherD = constructor.getNode(SNLOccurrence(term(other, "D")));
  auto* same = graph.getEdge(edge(graph, q, d));
  ASSERT_NE(nullptr, same);
  EXPECT_TRUE(same->hasFromClock(0));
  EXPECT_TRUE(same->hasToClock(0));
  EXPECT_FALSE(same->hasClockCrossing());
  auto* related = graph.getEdge(edge(graph, q, otherD));
  ASSERT_NE(nullptr, related);
  EXPECT_TRUE(related->hasFromClock(0));
  EXPECT_TRUE(related->hasToClock(1));
  // Relationship filtering is a separate step: binding does not merge clocks.
  EXPECT_TRUE(related->hasClockCrossing());
  EXPECT_TRUE(constructor.getCoverageReport().empty());
  // Rebuild imports fresh bindings, without accumulating clocks in either layer.
  construct(constructor);
  EXPECT_EQ(2u, constructor.getGraph().getClocksCount());
  EXPECT_EQ(2u, top_->getClocks().size());
  EXPECT_EQ(primary, constructor.getGraph().getClock(0)->getNLClock());
}

TEST_F(TSNLGraphConstructorTests, sharedModelClockDeclarationsHaveIndependentOccurrenceBindings) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* clock = port(child, "clock", SNLTerm::Direction::Input);
  auto* declaration = NLClock::createPrimary(child, NLName("local"), 7.0, "ns", {clock});
  auto* flop = SNLInstance::create(child, NLDB0::getDFF(), NLName("state"));
  connect(child, {clock, term(flop, "C")});
  auto* left = SNLInstance::create(top_, child, NLName("left"));
  auto* right = SNLInstance::create(top_, child, NLName("right"));
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  ASSERT_EQ(2u, graph.getClocksCount());
  auto result = TClockPropagator::run(graph);
  size_t index = 0;
  for (auto* instance : {left, right}) {
    const auto* binding = graph.getClock(index);
    ASSERT_NE(nullptr, binding);
    EXPECT_EQ(declaration, binding->getNLClock());
    auto seed = constructor.getNode(SNLOccurrence(instance->getInstTerm(clock)));
    auto pin = constructor.getNode(SNLOccurrence(SNLPath(instance), term(flop, "C")));
    EXPECT_EQ(seed, binding->getNode());
    EXPECT_EQ(1u, result.clockOrigins[pin].count());
    EXPECT_TRUE(result.clockOrigins[pin].test(index));
    ++index;
  }
  EXPECT_NE(graph.getClock(0)->getNode(), graph.getClock(1)->getNode());
  EXPECT_EQ(1u, child->getClocks().size());
  EXPECT_TRUE(top_->getClocks().empty());
#if GTEST_HAS_DEATH_TEST
  auto signature = [&]() {
    std::string text;
    for (const auto& binding : graph.getClocksStorage()) {
      text += binding.getName() + ":" + std::to_string(binding.getIndex());
      for (const auto& seed : binding.getSeeds()) text += ":" + std::to_string(seed.node);
      text += ";";
    }
    return text;
  };
  const auto previousStyle = GTEST_FLAG_GET(death_test_style);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_EXIT({
    std::fputs(signature().c_str(), stderr);
    std::_Exit(0);
  }, ::testing::ExitedWithCode(0), "^" + signature() + "$");
  GTEST_FLAG_SET(death_test_style, previousStyle);
#endif
}

TEST_F(TSNLGraphConstructorTests, unknownSharedPinsAndOccurrenceClockHaveNoArtificialCycle) {
  loadSV(R"(
    module loaded(input logic clk, rst_n, d, output logic q0, q1, q2);
      logic gclk;
      sram u0(.clk_i(clk), .rst_ni(rst_n));
      sram u1(.clk_i(clk), .rst_ni(rst_n));
      pll u_pll(.ref_i(clk), .clk_o(gclk));
      always_ff @(posedge clk) begin q0 <= d; q1 <= q0; end
      always_ff @(posedge gclk) q2 <= d;
    endmodule
  )");
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  ASSERT_EQ(3u, constructor.getCoverageReport().size());
  EXPECT_EQ(3u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::Blackbox));
  EXPECT_EQ(0u, constructor.getCoverageCounts().count(TSNLGraphConstructor::CoverageReason::MultipleDrivers));
  for (const auto& entry : constructor.getCoverageReport()) {
    EXPECT_EQ(TSNLGraphConstructor::Treatment::Opaque, entry.treatment);
    EXPECT_NE(std::string::npos, entry.detail.find("2 inout pins"));
  }
  auto* pll = top_->getInstance(NLName("u_pll"));
  ASSERT_NE(nullptr, pll);
  auto occurrence = SNLOccurrence(term(pll, "clk_o"));
  auto driver = constructor.getDriverNode(occurrence);
  auto load = constructor.getNode(occurrence);
  EXPECT_NE(driver, load);
  EXPECT_EQ(TNodeProxy::Role::Drive, graph.getNode(driver)->getProxy()->getRole());
  EXPECT_EQ(TNodeProxy::Role::Load, graph.getNode(load)->getProxy()->getRole());
  EXPECT_NE(std::string::npos, graph.getNode(driver)->getProxy()->getDescription().find("(drive)"));
  auto root = constructor.createClock("root", SNLOccurrence(top_->getScalarTerm(NLName("clk"))));
  auto generated = constructor.createClock("generated", occurrence);
  EXPECT_EQ(driver, graph.getClock(generated)->getNode());
  // Existing node-based registration also redirects a split load role.
  auto redirected = graph.createClock("redirected", term(pll, "clk_o"), load);
  EXPECT_EQ(driver, graph.getClock(redirected)->getNode());
  auto result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  size_t rootPins = 0, generatedPins = 0;
  for (const auto& node : graph.getNodesStorage()) {
    if (!node.isFlopClock()) continue;
    rootPins += result.clockOrigins[node.getIndex()].test(root);
    generatedPins += result.clockOrigins[node.getIndex()].test(generated);
  }
  EXPECT_EQ(2u, rootPins);
  EXPECT_EQ(1u, generatedPins);
  for (const char* name : {"q0", "q1", "q2"}) {
    auto* net = top_->getScalarTerm(NLName(name))->getNet();
    size_t matches = 0;
    for (const auto& arc : graph.getEdgesStorage()) {
      if (arc.getKind() != TEdge::Kind::State) continue;
      const auto* proxy = static_cast<const TSNLNodeProxy*>(graph.getNode(arc.getTarget())->getProxy());
      auto* pin = dynamic_cast<SNLInstTerm*>(proxy->getOccurrence().getObject());
      if (pin && pin->getNet() == net) {
        ++matches;
        const bool isGenerated = std::string(name) == "q2";
        EXPECT_EQ(!isGenerated, result.clockOrigins[arc.getSource()].test(root));
        EXPECT_EQ(isGenerated, result.clockOrigins[arc.getSource()].test(generated));
      }
    }
    EXPECT_EQ(1u, matches) << name;
  }
  EXPECT_EQ(6u, constructor.getConstructionStats().splitPins);
  for (const auto& e : graph.getEdgesStorage()) EXPECT_FALSE(e.isDisabled());
}

TEST_F(TSNLGraphConstructorTests, twoUnknownCellsSharingAllPinsHaveOnlyCrossRoleNetEdges) {
  loadSV(R"(
    module loaded;
      wire a, b;
      unknown_cell left(.x(a), .y(b));
      unknown_cell right(.x(a), .y(b));
    endmodule
  )");
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  auto* left = top_->getInstance(NLName("left"));
  auto* right = top_->getInstance(NLName("right"));
  ASSERT_NE(nullptr, left);
  ASSERT_NE(nullptr, right);
  EXPECT_EQ(8u, graph.getNodesCount());
  EXPECT_EQ(4u, graph.getEdgesCount());
  for (const char* name : {"x", "y"}) {
    auto a = SNLOccurrence(term(left, name));
    auto b = SNLOccurrence(term(right, name));
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(a), constructor.getNode(b)));
    EXPECT_NE(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(b), constructor.getNode(a)));
    EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(a), constructor.getNode(a)));
    EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, constructor.getDriverNode(b), constructor.getNode(b)));
  }
  EXPECT_NO_THROW(TClockPropagator::run(graph));
  EXPECT_EQ(2u, constructor.getCoverageReport().size());
}

TEST_F(TSNLGraphConstructorTests, topInoutAndOpaquePinShareNetWithoutCycle) {
  loadSV(R"(
    module loaded(inout wire io);
      unknown_cell leaf(.x(io));
    endmodule
  )");
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_EQ(4u, constructor.getGraph().getNodesCount());
  EXPECT_EQ(2u, constructor.getGraph().getEdgesCount());
  EXPECT_EQ(1u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::Blackbox));
  EXPECT_EQ(1u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::InOut));
  EXPECT_NO_THROW(TClockPropagator::run(constructor.getGraph()));
  auto* io = top_->getScalarTerm(NLName("io"));
  auto* declaration = NLClock::createPrimary(top_, NLName("external"), 10, "ns", {io});
  construct(constructor);
  EXPECT_EQ(declaration, constructor.getGraph().getClock(0)->getNLClock());
  EXPECT_EQ(constructor.getDriverNode(SNLOccurrence(io)), constructor.getGraph().getClock(0)->getNode());
  // Explicit declaration registration with sampled nodes uses driver seeds too.
  auto c = constructor.getGraph().createClock(*declaration, {constructor.getNode(SNLOccurrence(io))});
  EXPECT_EQ(constructor.getDriverNode(SNLOccurrence(io)), constructor.getGraph().getClock(c)->getNode());
}

TEST_F(TSNLGraphConstructorTests, modeledTristatePadStillDrivesSamplingFlop) {
  loadSV(R"(
    module loaded(input logic clk, oe, o, inout wire io, output logic q);
      assign io = oe ? o : 1'bz;
      always_ff @(posedge clk) q <= io;
    endmodule
  )");
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto clock = constructor.createClock("root", SNLOccurrence(top_->getScalarTerm(NLName("clk"))));
  auto result = TClockPropagator::run(constructor.getGraph());
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  size_t samplingInputs = 0;
  for (const auto& node : constructor.getGraph().getNodesStorage()) {
    if (node.isFlopDataInput()) {
      ++samplingInputs;
      EXPECT_GT(node.getInDegree(), 0u);
      const auto* proxy = static_cast<const TSNLNodeProxy*>(node.getProxy());
      auto* pin = dynamic_cast<SNLInstTerm*>(proxy->getOccurrence().getObject());
      ASSERT_NE(nullptr, pin);
      EXPECT_EQ(top_->getScalarTerm(NLName("io"))->getNet(), pin->getNet());
      EXPECT_TRUE(std::any_of(node.getInEdges().begin(), node.getInEdges().end(), [&](auto id) {
        return constructor.getGraph().getNode(constructor.getGraph().getEdge(id)->getSource())->isCombinationalDriver();
      }));
      ASSERT_EQ(1u, node.getCaptureInEdges().size());
      auto* capture = constructor.getGraph().getCaptureEdge(node.getCaptureInEdges().front());
      EXPECT_TRUE(result.clockOrigins[capture->getSource()].test(clock));
    }
  }
  EXPECT_EQ(1u, samplingInputs);
  EXPECT_EQ(0u, constructor.getCoverageCounts().count(TSNLGraphConstructor::CoverageReason::Blackbox));
}

TEST_F(TSNLGraphConstructorTests, modeledBidirectionalArcsKeepRealCycleAndStructuredDiagnostic) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("bidirectional"));
  auto* a = port(model, "a", SNLTerm::Direction::InOut);
  auto* b = port(model, "b", SNLTerm::Direction::InOut);
  SNLDesignModeling::addCombinatorialArcs({a}, {b});
  SNLDesignModeling::addCombinatorialArcs({b}, {a});
  auto* left = SNLInstance::create(top_, model, NLName("left"));
  auto* right = SNLInstance::create(top_, model, NLName("right"));
  connect(top_, {term(left, "a"), term(right, "b")});
  connect(top_, {term(left, "b"), term(right, "a")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  EXPECT_EQ(8u, graph.getNodesCount());
  EXPECT_EQ(8u, graph.getEdgesCount());
  EXPECT_EQ(4u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::InOut));
  try {
    TClockPropagator::run(graph);
    FAIL() << "Expected a real combinational cycle";
  } catch (const TClockPropagator::CycleError& error) {
    ASSERT_TRUE(graph.isNodeValid(error.getNode()));
    EXPECT_EQ(graph.getNode(error.getNode())->getProxy()->getDescription(), error.getDescription());
    EXPECT_EQ(graph.getNode(error.getNode())->getProxy()->getRole(), error.getRole());
    EXPECT_NE(TNodeProxy::Role::Single, error.getRole());
    EXPECT_NE(std::string::npos, error.getDescription().find("left"));
  }
  for (const auto& e : graph.getEdgesStorage()) EXPECT_FALSE(e.isDisabled());
}

TEST_F(TSNLGraphConstructorTests, splitHierarchyAliasesOnePairAndPassesBothDirections) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* io = port(child, "io", SNLTerm::Direction::InOut);
  auto* macro = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("opaque"));
  port(macro, "pin", SNLTerm::Direction::InOut);
  auto* inner = SNLInstance::create(child, macro, NLName("inner"));
  connect(child, {io, term(inner, "pin")});
  auto* wrapper = SNLInstance::create(top_, child, NLName("wrapper"));
  auto* outer = SNLInstance::create(top_, macro, NLName("outer"));
  connect(top_, {term(wrapper, "io"), term(outer, "pin")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto external = SNLOccurrence(term(wrapper, "io"));
  auto internal = SNLOccurrence(SNLPath(wrapper), io);
  auto innerPin = SNLOccurrence(SNLPath(wrapper), term(inner, "pin"));
  auto outerPin = SNLOccurrence(term(outer, "pin"));
  EXPECT_EQ(constructor.getNode(external), constructor.getNode(internal));
  EXPECT_EQ(constructor.getDriverNode(external), constructor.getDriverNode(internal));
  EXPECT_NE(constructor.getNode(external), constructor.getDriverNode(external));
  EXPECT_EQ(6u, constructor.getGraph().getNodesCount());
  EXPECT_EQ(6u, constructor.getGraph().getEdgesCount());
  auto c = constructor.createClock("outer", outerPin);
  auto d = constructor.createClock("inner", innerPin);
  auto result = TClockPropagator::run(constructor.getGraph());
  EXPECT_TRUE(result.clockOrigins[constructor.getNode(innerPin)].test(c));
  EXPECT_TRUE(result.clockOrigins[constructor.getNode(outerPin)].test(d));
}

TEST_F(TSNLGraphConstructorTests, splitFlopSampledRolesAndCapturesAreCountedOnce) {
  auto* model = modeledFlop(true);
  auto* flop = SNLInstance::create(top_, model, NLName("flop"));
  auto* clock = port(top_, "clock", SNLTerm::Direction::Input);
  auto* data = port(top_, "data", SNLTerm::Direction::Input);
  connect(top_, {clock, term(flop, "C")});
  connect(top_, {data, term(flop, "D")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto& graph = constructor.getGraph();
  EXPECT_EQ(2u, constructor.getConstructionStats().splitPins);
  size_t clocks = 0, inputs = 0, outputs = 0;
  for (const auto& node : graph.getNodesStorage()) {
    clocks += node.isFlopClock();
    inputs += node.isFlopDataInput();
    outputs += node.isFlopDataOutput();
  }
  EXPECT_EQ(1u, clocks);
  EXPECT_EQ(1u, inputs);
  EXPECT_EQ(1u, outputs);
  EXPECT_EQ(1u, graph.getCaptureEdgesCount());
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(std::vector<TGraph::NodeId>{constructor.getNode(SNLOccurrence(term(flop, "C")))}, result.unclockedFlopClockPins);
  EXPECT_EQ(2u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::InOut));
  EXPECT_EQ(0u, constructor.getCoverageCounts().count(TSNLGraphConstructor::CoverageReason::MultipleDrivers));
}

TEST_F(TSNLGraphConstructorTests, shuffledInoutConstructionPreservesNamedGraph) {
  auto* model = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("opaque"));
  port(model, "a", SNLTerm::Direction::InOut);
  port(model, "b", SNLTerm::Direction::InOut);
  std::vector<std::string> expected;
  for (bool reverse : {false, true}) {
    std::vector<SNLInstance*> cells;
    for (const auto* name : reverse ? std::vector<const char*>{"right", "left"} : std::vector<const char*>{"left", "right"}) {
      cells.push_back(SNLInstance::create(top_, model, NLName(name)));
    }
    for (const char* name : {"a", "b"}) connect(top_, {term(cells[0], name), term(cells[1], name)});
    TSNLGraphConstructor constructor;
    construct(constructor);
    std::vector<std::string> actual;
    const auto& graph = constructor.getGraph();
    for (const auto& node : graph.getNodesStorage()) actual.push_back(node.getProxy()->getDescription());
    for (const auto& e : graph.getEdgesStorage()) actual.push_back(
      graph.getNode(e.getSource())->getProxy()->getDescription() + " -> " +
      graph.getNode(e.getTarget())->getProxy()->getDescription());
    std::sort(actual.begin(), actual.end());
    if (reverse) EXPECT_EQ(expected, actual);
    else expected = actual;
    EXPECT_NO_THROW(TClockPropagator::run(constructor.getGraph()));
#if GTEST_HAS_DEATH_TEST
    // Re-exec verifies live IDs, role allocation and edge emission order for
    // identical ordered SNL input in a fresh process, in addition to the named
    // graph equivalence across the shuffled inputs above.
    std::string signature;
    for (const auto& node : graph.getNodesStorage()) signature +=
      std::to_string(node.getIndex()) + ":" + std::to_string(static_cast<int>(node.getProxy()->getRole())) + ";";
    for (const auto& arc : graph.getEdgesStorage()) signature +=
      std::to_string(arc.getIndex()) + ":" + std::to_string(arc.getSource()) + ":" +
      std::to_string(arc.getTarget()) + ";";
    const auto previousStyle = GTEST_FLAG_GET(death_test_style);
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_EXIT({ std::fputs(signature.c_str(), stderr); std::_Exit(0); },
      ::testing::ExitedWithCode(0), "^" + signature + "$");
    GTEST_FLAG_SET(death_test_style, previousStyle);
#endif
    naja::DNL::destroy();
    for (auto* cell : cells) cell->destroy();
  }
}

TEST_F(TSNLGraphConstructorTests, shortedHierarchicalInoutsRemainOneEquipotential) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* a = port(child, "a", SNLTerm::Direction::InOut);
  auto* b = port(child, "b", SNLTerm::Direction::InOut);
  auto* x = port(child, "x", SNLTerm::Direction::Input);
  auto* y = port(child, "y", SNLTerm::Direction::Input);
  connect(child, {a, b, x, y});
  auto* wrapper = SNLInstance::create(top_, child, NLName("wrapper"));
  auto* io = port(top_, "io", SNLTerm::Direction::InOut);
  connect(top_, {io, term(wrapper, "a"), term(wrapper, "b"), term(wrapper, "x"), term(wrapper, "y")});
  TSNLGraphConstructor constructor;
  construct(constructor);
  EXPECT_EQ(8u, constructor.getGraph().getNodesCount());
  EXPECT_EQ(18u, constructor.getGraph().getEdgesCount());
  for (const char* name : {"x", "y"}) {
    auto occurrence = SNLOccurrence(term(wrapper, name));
    EXPECT_EQ(constructor.getNode(occurrence), constructor.getDriverNode(occurrence));
  }
  EXPECT_EQ(0u, constructor.getCoverageCounts().count(TSNLGraphConstructor::CoverageReason::MultipleDrivers));
  EXPECT_NO_THROW(TClockPropagator::run(constructor.getGraph()));
}

TEST_F(TSNLGraphConstructorTests, modeledSingleDriverInoutKeepsOneNode) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("pad"));
  auto* input = port(model, "input", SNLTerm::Direction::Input);
  auto* io = port(model, "io", SNLTerm::Direction::InOut);
  SNLDesignModeling::addCombinatorialArcs({input}, {io});
  auto* cell = SNLInstance::create(top_, model, NLName("pad"));
  auto* output = port(top_, "output", SNLTerm::Direction::Output);
  connect(top_, {cell->getInstTerm(io), output});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto occurrence = SNLOccurrence(cell->getInstTerm(io));
  EXPECT_EQ(constructor.getNode(occurrence), constructor.getDriverNode(occurrence));
  EXPECT_EQ(3u, constructor.getGraph().getNodesCount());
  EXPECT_EQ(2u, constructor.getGraph().getEdgesCount());
  EXPECT_EQ(0u, constructor.getConstructionStats().splitPins);
  EXPECT_EQ(1u, constructor.getCoverageCounts().at(TSNLGraphConstructor::CoverageReason::InOut));
  EXPECT_NO_THROW(TClockPropagator::run(constructor.getGraph()));
}

TEST_F(TSNLGraphConstructorTests, ResetDeclarationAndDataView) {
  loadSV(R"(
    module loaded(input c, rst_n, d, output reg q);
      wire next_data = rst_n ? d : 1'b0;
      always @(posedge c) q <= next_data;
    endmodule
  )");
  auto* reset = NLReset::create(top_, NLName("domain"), {top_->getScalarTerm(NLName("rst_n"))},
    NLReset::ActiveLevel::Low);
  EXPECT_EQ(1, reset->getInactiveValue());
  EXPECT_EQ(NLReset::Timing::Async, reset->getAssertion());
  EXPECT_EQ(NLReset::Timing::Sync, reset->getDeassertion());
  EXPECT_EQ(top_, reset->getDesign());
  EXPECT_EQ(NLName("domain"), reset->getName());
  EXPECT_EQ(1u, top_->getResets().size());
  EXPECT_EQ(1u, reset->getSources().size());
  EXPECT_EQ("NLReset", std::string(reset->getTypeName()));
  EXPECT_EQ("domain", reset->getString());
  EXPECT_EQ("NLReset domain", reset->getDescription());
  std::ostringstream stream;
  reset->debugDump(2, true, stream);
  EXPECT_EQ("  NLReset domain\n", stream.str());
  auto* foreign = SNLDesign::create(library_, NLName("other"));
  auto* foreignSource = port(foreign, "reset", SNLTerm::Direction::Input);
  auto* source = top_->getScalarTerm(NLName("rst_n"));
  EXPECT_THROW(NLReset::create(nullptr, NLName("bad"), {source}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName(), {source}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("domain"), {source}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {nullptr}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {foreignSource}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {source, source}, NLReset::ActiveLevel::Low), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {source}, NLReset::ActiveLevel::NA), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {source}, NLReset::ActiveLevel::Low,
    static_cast<NLReset::Timing>(-1)), NLException);
  EXPECT_THROW(NLReset::create(top_, NLName("bad"), {source}, NLReset::ActiveLevel::Low,
    NLReset::Timing::Async, static_cast<NLReset::Timing>(-1)), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("clock"), 10, "ns", {source}), NLException);
  TSNLGraphConstructor constructor;
  construct(constructor);
  ASSERT_EQ(1u, constructor.getResetDomains().size());
  const auto& domain = constructor.getResetDomains()[0];
  EXPECT_EQ(reset, domain.declaration);
  ASSERT_EQ(1u, domain.sources.size());
  using V = TConstantPropagator::Value;
  auto values = TConstantPropagator::run(constructor, {{domain.sources[0], V::One}});
  EXPECT_EQ(V::One, values[domain.sources[0]]);
  size_t pruned = 0, transparent = 0;
  for (const auto& edge: constructor.getGraph().getEdgesStorage()) {
    pruned += edge.isDataDisabled();
    transparent += edge.isDataTransparent();
  }
  EXPECT_EQ(2u, pruned);
  EXPECT_EQ(1u, transparent);
  EXPECT_NO_THROW(TClockPropagator::run(constructor.getGraph()));
  EXPECT_THROW(TConstantPropagator::run(constructor, {{domain.sources[0], V::One}, {domain.sources[0], V::Zero}}), NLException);
  reset->destroy();
  EXPECT_TRUE(top_->getResets().empty());
}

TEST_F(TSNLGraphConstructorTests, ResetTimingMetadata) {
  auto* source = port(top_, "reset", SNLTerm::Direction::Input);
  for (auto assertion : {NLReset::Timing::Async, NLReset::Timing::Sync}) {
    for (auto deassertion : {NLReset::Timing::Async, NLReset::Timing::Sync}) {
      auto* reset = NLReset::create(top_, NLName("domain"), {source},
        NLReset::ActiveLevel::Low, assertion, deassertion);
      EXPECT_EQ(assertion, reset->getAssertion());
      EXPECT_EQ(deassertion, reset->getDeassertion());
      reset->destroy();
    }
  }
}

TEST_F(TSNLGraphConstructorTests, ConstantViewPreservesAssignWiresAndUnknownDrivers) {
  auto* reset = port(top_, "reset", SNLTerm::Direction::Input);
  auto* other = port(top_, "other", SNLTerm::Direction::Input);
  auto* output = port(top_, "output", SNLTerm::Direction::Output);
  auto* assign = SNLInstance::create(top_, NLDB0::getAssign());
  auto* input = assign->getInstTerm(NLDB0::getAssignInput());
  auto* result = assign->getInstTerm(NLDB0::getAssignOutput());
  connect(top_, {reset, other, input});
  connect(top_, {result, output});
  TSNLGraphConstructor constructor;
  construct(constructor);
  auto source = constructor.getNode(SNLOccurrence(reset));
  auto second = constructor.getNode(SNLOccurrence(other));
  auto target = constructor.getNode(SNLOccurrence(output));
  using V = TConstantPropagator::Value;
  auto values = TConstantPropagator::run(constructor, {{source, V::One}});
  EXPECT_EQ(V::Unknown, values[target]);
  values = TConstantPropagator::run(constructor, {{source, V::One}, {second, V::One}});
  EXPECT_EQ(V::One, values[target]);
  auto arc = edge(constructor.getGraph(), constructor.getNode(SNLOccurrence(input)),
    constructor.getNode(SNLOccurrence(result)));
  ASSERT_NE(TGraph::kInvalidEdge, arc);
  EXPECT_FALSE(constructor.getGraph().getEdge(arc)->isDataDisabled());
}
