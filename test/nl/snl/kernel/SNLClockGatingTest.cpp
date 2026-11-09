// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "gtest/gtest.h"
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLClock.h"
#include "SNLClockGating.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLRTLInfos.h"

using namespace naja::NL;

TEST(SNLClockGatingStandaloneTest, accessorsWithoutUniverse) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  EXPECT_EQ(nullptr, NLDB0::getOrCreateClockGate());
  EXPECT_EQ(nullptr, NLDB0::getClockGateClock());
  EXPECT_EQ(nullptr, NLDB0::getClockGateEnable());
  EXPECT_EQ(nullptr, NLDB0::getClockGateTestEnable());
  EXPECT_EQ(nullptr, NLDB0::getClockGateOutput());
}

class SNLClockGatingTest : public ::testing::Test {
 protected:
  NLLibrary* library {};
  SNLDesign* top {};
  void SetUp() override {
    auto* universe = NLUniverse::create();
    auto* db = NLDB::create(universe);
    library = NLLibrary::create(db, NLName("designs"));
    top = SNLDesign::create(library, NLName("top"));
    db->setTopDesign(top);
  }
  void TearDown() override { NLUniverse::get()->destroy(); }
  SNLScalarNet* net(SNLDesign* design, const char* name) {
    return SNLScalarNet::create(design, NLName(name));
  }
  SNLScalarTerm* port(SNLDesign* design, const char* name, SNLTerm::Direction direction) {
    auto* term = SNLScalarTerm::create(design, direction, NLName(name));
    term->setNet(net(design, name));
    return term;
  }
  SNLInstance* gate(SNLDesign* design, NLDB0::GateType type,
      SNLBitNet* first, SNLBitNet* second, SNLBitNet* out) {
    auto* model = type.isNInput() ? NLDB0::getOrCreateNInputGate(type, second ? 2 : 1)
      : NLDB0::getOrCreateNOutputGate(type, 1);
    auto* instance = SNLInstance::create(design, model);
    for (auto* pin : instance->getInstTerms()) {
      if (pin->getDirection() == SNLTerm::Direction::Output) pin->setNet(out);
      else { pin->setNet(first); first = second; }
    }
    return instance;
  }
  SNLDesign* wrapper(bool inverted = true, bool exposeState = false, bool includeTest = true) {
    auto* design = SNLDesign::create(library, NLName("gate_wrapper"));
    auto* clk = port(design, "source", SNLTerm::Direction::Input);
    auto* en = port(design, "request", SNLTerm::Direction::Input);
    auto* test = includeTest ? port(design, "test", SNLTerm::Direction::Input) : nullptr;
    auto* out = port(design, "result", SNLTerm::Direction::Output);
    auto* control = net(design, "control");
    auto* data = includeTest ? net(design, "combined") : en->getNet();
    auto* held = net(design, "held");
    gate(design, inverted ? NLDB0::GateType::Not : NLDB0::GateType::Buf,
      clk->getNet(), nullptr, control);
    if (test) gate(design, NLDB0::GateType::Or, en->getNet(), test->getNet(), data);
    auto* latch = SNLInstance::create(design, NLDB0::getDLatch(), NLName("storage"));
    latch->setTermNet(NLDB0::getDLatchEnable(), control);
    latch->setTermNet(NLDB0::getDLatchData(), data);
    latch->setTermNet(NLDB0::getDLatchOutput(), held);
    gate(design, NLDB0::GateType::And, held, clk->getNet(), out->getNet());
    if (exposeState) SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("state"))->setNet(held);
    return design;
  }
  SNLInstance* instantiate(SNLDesign* wrapper, const char* name, SNLBitNet* clk, SNLBitNet* output) {
    auto* instance = SNLInstance::create(top, wrapper, NLName(name));
    instance->setTermNet(wrapper->getScalarTerm(NLName("source")), clk);
    instance->setTermNet(wrapper->getScalarTerm(NLName("request")), net(top, (std::string(name)+"_en").c_str()));
    instance->setTermNet(wrapper->getScalarTerm(NLName("test")), net(top, (std::string(name)+"_test").c_str()));
    instance->setTermNet(wrapper->getScalarTerm(NLName("result")), output);
    return instance;
  }
  void sink(SNLBitNet* clock, const char* name = "flop") {
    auto* instance = SNLInstance::create(top, NLDB0::getDFF(), NLName(name));
    instance->setTermNet(NLDB0::getDFFClock(), clock);
    instance->setTermNet(NLDB0::getDFFData(), net(top, (std::string(name)+"_d").c_str()));
    instance->setTermNet(NLDB0::getDFFOutput(), net(top, (std::string(name)+"_q").c_str()));
  }
};

TEST_F(SNLClockGatingTest, canonicalContractAndTiming) {
  auto* model = NLDB0::getOrCreateClockGate();
  ASSERT_NE(nullptr, model);
  EXPECT_TRUE(NLDB0::isClockGate(model));
  EXPECT_EQ(model, NLDB0::getOrCreateClockGate());
  EXPECT_FALSE(NLDB0::isClockGate(NLDB0::getDLatch()));
  EXPECT_EQ(4u, model->getBitTerms().size());
  EXPECT_EQ(NLName("naja_clock_gate"), model->getName());
  using M = SNLDesignModeling;
  EXPECT_TRUE(M::isSequential(model));
  EXPECT_EQ(0u, M::getTruthTableCount(model));
  EXPECT_EQ(M::SNLTermRole::Clock, M::getTermRole(NLDB0::getClockGateClock()));
  EXPECT_EQ(M::SNLTermRole::Enable, M::getTermRole(NLDB0::getClockGateEnable()));
  EXPECT_EQ(M::SNLTermRole::ScanEnable, M::getTermRole(NLDB0::getClockGateTestEnable()));
  const auto& sequence = M::getSequentialModel(model);
  EXPECT_EQ(M::SequentialModel::Kind::Latch, sequence.kind);
  ASSERT_EQ(1u, sequence.states.size());
  ASSERT_EQ(1u, sequence.outputs.size());
  EXPECT_EQ(M::BooleanExpression::Operator::Not, sequence.clockedOn.nodes[sequence.clockedOn.root].operation);
  EXPECT_EQ(M::BooleanExpression::Operator::Or,
    sequence.states[0].nextState.nodes[sequence.states[0].nextState.root].operation);
  EXPECT_EQ(M::BooleanExpression::Operator::And,
    sequence.outputs[0].function.nodes[sequence.outputs[0].function.root].operation);
  EXPECT_EQ(2u, M::getClockRelatedInputs(NLDB0::getClockGateClock()).size());
  EXPECT_EQ(1u, M::getClockRelatedOutputs(NLDB0::getClockGateClock()).size());
}

TEST_F(SNLClockGatingTest, sharedWrapperOnlyClockOccurrenceIsReplaced) {
  auto* model = wrapper();
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* data = port(top, "data", SNLTerm::Direction::Input)->getNet();
  auto* output = net(top, "gated");
  auto* clockInstance = instantiate(model, "clock_use", clock, output);
  const auto id = clockInstance->getID();
  auto* metadata = SNLRTLInfos::create(clockInstance);
  metadata->setSourceLoc({NLName("synthetic.sv"), 3, 3, 1, 8});
  metadata->setInfo(NLName("origin_tag"), "synthetic");
  SNLSourceReference reference {{NLName("synthetic.sv"), 2, 4, 1, 9},
    NLName("test"), NLName("SystemVerilog"), NLName("RTL")};
  metadata->setSourceDeclaration(reference);
  metadata->addSourceOrigin(reference);
  auto* dataOutput = port(top, "data_result", SNLTerm::Direction::Output)->getNet();
  instantiate(model, "data_use", data, dataOutput);
  sink(output);
  EXPECT_EQ(1u, SNLClockGating::infer(top));
  auto* inferred = top->getInstance(NLName("clock_use"));
  ASSERT_NE(nullptr, inferred);
  EXPECT_EQ(id, inferred->getID());
  ASSERT_NE(nullptr, inferred->getRTLInfos());
  EXPECT_EQ("synthetic", inferred->getRTLInfos()->getInfo(NLName("origin_tag")));
  EXPECT_EQ(reference, *inferred->getRTLInfos()->getSourceDeclaration());
  EXPECT_EQ(std::vector<SNLSourceReference>{reference}, inferred->getRTLInfos()->getSourceOrigins());
  ASSERT_TRUE(inferred->getRTLInfos()->hasSourceLoc());
  EXPECT_EQ(3u, inferred->getRTLInfos()->getSourceLoc()->line);
  EXPECT_TRUE(NLDB0::isClockGate(inferred->getModel()));
  EXPECT_EQ(model, top->getInstance(NLName("data_use"))->getModel());
  EXPECT_EQ(clock, inferred->getInstTerm(NLDB0::getClockGateClock())->getNet());
  EXPECT_EQ(output, inferred->getInstTerm(NLDB0::getClockGateOutput())->getNet());
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, dataLatchEnableIsNotClockEvidence) {
  auto* model = wrapper();
  auto* output = net(top, "condition");
  instantiate(model, "data_use", port(top, "data", SNLTerm::Direction::Input)->getNet(), output);
  auto* latch = SNLInstance::create(top, NLDB0::getDLatch(), NLName("ordinary_latch"));
  latch->setTermNet(NLDB0::getDLatchEnable(), output);
  latch->setTermNet(NLDB0::getDLatchData(), net(top, "d"));
  latch->setTermNet(NLDB0::getDLatchOutput(), net(top, "q"));
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, mixedDataAndClockConsumersAreNotReplaced) {
  auto* model = wrapper();
  auto* output = net(top, "mixed");
  instantiate(model, "mixed_use", port(top, "clock", SNLTerm::Direction::Input)->getNet(), output);
  sink(output);
  auto* dataSink = SNLInstance::create(top, NLDB0::getDFF(), NLName("data_sink"));
  dataSink->setTermNet(NLDB0::getDFFData(), output);
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, wrongPhaseIsRejected) {
  auto* model = wrapper(false);
  auto* output = net(top, "gated");
  instantiate(model, "gate", port(top, "clock", SNLTerm::Direction::Input)->getNet(), output);
  sink(output);
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, observableHeldStateIsRejected) {
  auto* model = wrapper(true, true);
  auto* output = net(top, "gated");
  instantiate(model, "gate", port(top, "clock", SNLTerm::Direction::Input)->getNet(), output);
  sink(output);
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, declaredClockAllowsLatchOnlyClockNetwork) {
  auto* model = wrapper();
  auto* input = port(top, "clock", SNLTerm::Direction::Input);
  auto* output = net(top, "gated");
  instantiate(model, "gate", input->getNet(), output);
  auto* latch = SNLInstance::create(top, NLDB0::getDLatch(), NLName("clocked_storage"));
  latch->setTermNet(NLDB0::getDLatchEnable(), output);
  latch->setTermNet(NLDB0::getDLatchData(), net(top, "d"));
  latch->setTermNet(NLDB0::getDLatchOutput(), net(top, "q"));
  NLClock::createPrimary(top, NLName("root"), 10, "ns", {input});
  EXPECT_EQ(1u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, multipleClockGateOutputDriversAreRejected) {
  auto* model = wrapper();
  auto* output = net(top, "gated");
  auto* instance = instantiate(model, "gate", port(top, "clock", SNLTerm::Direction::Input)->getNet(), output);
  auto* extra = gate(top, NLDB0::GateType::Buf, net(top, "other"), nullptr, output);
  (void)instance; (void)extra;
  sink(output);
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, cascadedGatesAreRecognizedTogether) {
  auto* model = wrapper();
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* middle = net(top, "middle");
  auto* output = net(top, "gated");
  instantiate(model, "first", clock, middle);
  instantiate(model, "second", middle, output);
  sink(output);
  EXPECT_EQ(2u, SNLClockGating::infer(top));
  EXPECT_TRUE(NLDB0::isClockGate(top->getInstance(NLName("first"))->getModel()));
  EXPECT_TRUE(NLDB0::isClockGate(top->getInstance(NLName("second"))->getModel()));
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, sharedHierarchyWithoutCandidatesIsUnchanged) {
  auto* child = SNLDesign::create(library);
  auto* input = port(child, "input", SNLTerm::Direction::Input);
  auto* output = port(child, "output", SNLTerm::Direction::Output);
  gate(child, NLDB0::GateType::Not, input->getNet(), nullptr, output->getNet());
  // A compact model DAG represents over a million leaf occurrences. With no
  // latch/AND candidate, inference must not need that expanded clock network.
  for (size_t level = 0; level < 20; ++level) {
    auto* parent = level == 19 ? top : SNLDesign::create(library);
    auto* parentInput = port(parent, "input", SNLTerm::Direction::Input);
    auto* parentOutput = port(parent, "output", SNLTerm::Direction::Output);
    auto* middle = net(parent, "middle");
    auto* first = SNLInstance::create(parent, child);
    auto* second = SNLInstance::create(parent, child);
    first->setTermNet(input, parentInput->getNet());
    first->setTermNet(output, middle);
    second->setTermNet(input, middle);
    second->setTermNet(output, parentOutput->getNet());
    child = parent;
    input = parentInput;
    output = parentOutput;
  }
  NLClock::createPrimary(top, NLName("source"), 10, "ns", {input});
  SNLClockGating::Hooks hooks;
  const auto unexpected = [](SNLInstance*) { ADD_FAILURE() << "Unexpected replacement"; };
  hooks.beforeReplace = unexpected;
  hooks.afterReplace = unexpected;
  hooks.beforeRemove = unexpected;
  EXPECT_EQ(0u, SNLClockGating::infer(top, hooks));
  EXPECT_EQ(2u, top->getInstances().size());
  EXPECT_EQ(3u, top->getBitNets().size());
  EXPECT_EQ(1u, top->getClocks().size());
}

TEST_F(SNLClockGatingTest, candidateInLaterRootIsStillRecognized) {
  auto* firstRoot = SNLDesign::create(library);
  auto* model = wrapper();
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* output = net(top, "gated");
  instantiate(model, "clock_use", clock, output);
  sink(output);
  EXPECT_EQ(1u, SNLClockGating::infer(std::vector<SNLDesign*>{firstRoot, top}));
  EXPECT_TRUE(NLDB0::isClockGate(top->getInstance(NLName("clock_use"))->getModel()));
}

TEST_F(SNLClockGatingTest, sharedOwnerWithClockAndDataContextsRemainsStructural) {
  auto* model = wrapper();
  auto* root = top;
  top = SNLDesign::create(library, NLName("owner"));
  auto* source = port(top, "source", SNLTerm::Direction::Input);
  auto* output = port(top, "result", SNLTerm::Direction::Output);
  instantiate(model, "shared", source->getNet(), output->getNet());
  auto* owner = top;
  top = root;
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* data = port(top, "data", SNLTerm::Direction::Input)->getNet();
  auto* gated = net(top, "gated");
  auto* result = port(top, "data_result", SNLTerm::Direction::Output)->getNet();
  for (bool clockUse : {true, false}) {
    auto* instance = SNLInstance::create(top, owner);
    instance->setTermNet(source, clockUse ? clock : data);
    instance->setTermNet(output, clockUse ? gated : result);
  }
  sink(gated);
  EXPECT_EQ(0u, SNLClockGating::infer(top));
  EXPECT_EQ(model, owner->getInstance(NLName("shared"))->getModel());
}

TEST_F(SNLClockGatingTest, declaredClockCanReplaceLocalPairWithoutWrapper) {
  top = wrapper();
  auto* output = top->getScalarTerm(NLName("result"));
  NLClock::createPrimary(top, NLName("generated"), 12.0, "ns", {output});
  EXPECT_EQ(1u, SNLClockGating::infer(top));
  size_t gates = 0, latches = 0;
  for (auto* instance : top->getInstances()) {
    gates += NLDB0::isClockGate(instance->getModel());
    latches += NLDB0::isDLatch(instance->getModel());
    if (NLDB0::isClockGate(instance->getModel())) {
      EXPECT_TRUE(instance->getInstTerm(NLDB0::getClockGateTestEnable())->getNet()->isAssign0());
    }
  }
  EXPECT_EQ(1u, gates);
  EXPECT_EQ(0u, latches);
}

TEST_F(SNLClockGatingTest, twoInputWrapperTiesTestEnableLow) {
  auto* model = wrapper(true, false, false);
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* enable = port(top, "enable", SNLTerm::Direction::Input)->getNet();
  auto* output = net(top, "gated");
  auto* instance = SNLInstance::create(top, model, NLName("clock_use"));
  instance->setTermNet(model->getScalarTerm(NLName("source")), clock);
  instance->setTermNet(model->getScalarTerm(NLName("request")), enable);
  instance->setTermNet(model->getScalarTerm(NLName("result")), output);
  sink(output);
  EXPECT_EQ(1u, SNLClockGating::infer(top));
  instance = top->getInstance(NLName("clock_use"));
  EXPECT_TRUE(NLDB0::isClockGate(instance->getModel()));
  EXPECT_EQ(enable, instance->getInstTerm(NLDB0::getClockGateEnable())->getNet());
  EXPECT_TRUE(instance->getInstTerm(NLDB0::getClockGateTestEnable())->getNet()->isAssign0());
}
