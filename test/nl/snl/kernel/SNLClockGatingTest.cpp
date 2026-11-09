// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "gtest/gtest.h"
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLClock.h"
#include "NLReset.h"
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
#include "SNLAttributes.h"

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
  SNLInstance* fusedGate(SNLDesign* model) {
    for (auto* instance : model->getInstances())
      if (NLDB0::isClockGate(instance->getModel())) return instance;
    ADD_FAILURE() << "Missing fused gate";
    return nullptr;
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

TEST_F(SNLClockGatingTest, sharedWrapperOnlyClockOccurrenceIsSpecialized) {
  auto* model = wrapper();
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* data = port(top, "data", SNLTerm::Direction::Input)->getNet();
  auto* output = net(top, "gated");
  auto* clockInstance = instantiate(model, "clock_use", clock, output);
  const auto id = clockInstance->getID();
  SNLAttributes::addAttribute(clockInstance, SNLAttribute(NLName("keep"), SNLAttributeValue("true")));
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
  EXPECT_EQ(clockInstance, inferred);
  EXPECT_EQ(1u, SNLAttributes::getAttributes(inferred).size());
  EXPECT_FALSE(inferred->getModel()->isPrimitive());
  EXPECT_NE(model, inferred->getModel());
  EXPECT_NE(nullptr, fusedGate(inferred->getModel()));
  EXPECT_NE(nullptr, model->getInstance(NLName("storage")));
  EXPECT_EQ(model, top->getInstance(NLName("data_use"))->getModel());
  EXPECT_EQ(clock, inferred->getInstTerm(inferred->getModel()->getScalarTerm(NLName("source")))->getNet());
  EXPECT_EQ(output, inferred->getInstTerm(inferred->getModel()->getScalarTerm(NLName("result")))->getNet());
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
  EXPECT_EQ(1u, SNLClockGating::infer(top));
  EXPECT_EQ(model, top->getInstance(NLName("first"))->getModel());
  EXPECT_EQ(model, top->getInstance(NLName("second"))->getModel());
  EXPECT_NE(nullptr, fusedGate(model));
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
  EXPECT_EQ(model, top->getInstance(NLName("clock_use"))->getModel());
  EXPECT_NE(nullptr, fusedGate(model));
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
  EXPECT_EQ(model, instance->getModel());
  auto* primitive = fusedGate(model);
  ASSERT_NE(nullptr, primitive);
  EXPECT_EQ(enable, instance->getInstTerm(model->getScalarTerm(NLName("request")))->getNet());
  EXPECT_EQ(model->getScalarTerm(NLName("request"))->getNet(),
    primitive->getInstTerm(NLDB0::getClockGateEnable())->getNet());
  EXPECT_TRUE(primitive->getInstTerm(NLDB0::getClockGateTestEnable())->getNet()->isAssign0());
}

TEST_F(SNLClockGatingTest, qualifiedInstancesShareOneSpecializedWrapper) {
  auto* model = wrapper();
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* firstNet = net(top, "first_net");
  auto* secondNet = net(top, "second_net");
  auto* first = instantiate(model, "first", clock, firstNet);
  auto* second = instantiate(model, "second", clock, secondNet);
  auto* data = instantiate(model, "data_use", port(top, "data", SNLTerm::Direction::Input)->getNet(),
    port(top, "result", SNLTerm::Direction::Output)->getNet());
  sink(firstNet, "first_flop");
  sink(secondNet, "second_flop");
  auto* firstPin = first->getInstTerm(model->getScalarTerm(NLName("source")));
  SNLDesign::create(library, NLName("gate_wrapper__clock_gate"));
  EXPECT_EQ(1u, SNLClockGating::infer(top));
  auto* specialized = first->getModel();
  EXPECT_EQ(NLName("gate_wrapper__clock_gate_1"), specialized->getName());
  EXPECT_EQ(specialized, second->getModel());
  EXPECT_NE(model, specialized);
  EXPECT_EQ(model, data->getModel());
  EXPECT_EQ(2u, specialized->getSlaveInstances().size());
  EXPECT_EQ(1u, model->getSlaveInstances().size());
  EXPECT_EQ(first, top->getInstance(NLName("first")));
  EXPECT_EQ(firstPin, first->getInstTerm(specialized->getScalarTerm(NLName("source"))));
  EXPECT_EQ(clock, firstPin->getNet());
  EXPECT_NE(nullptr, fusedGate(specialized));
  EXPECT_NE(nullptr, model->getInstance(NLName("storage")));
  EXPECT_EQ(0u, SNLClockGating::infer(top));
}

TEST_F(SNLClockGatingTest, specializedWrapperPreservesClockResetAndSourceMetadata) {
  auto* model = wrapper();
  auto* primary = NLClock::createPrimary(model, NLName("source"), 12, "ns",
    {model->getScalarTerm(NLName("source"))});
  primary->setWaveform(1, 5);
  NLClock::createGenerated(model, NLName("output"), primary,
    model->getScalarTerm(NLName("source")), 2, 1, true,
    {model->getScalarTerm(NLName("result"))});
  NLReset::create(model, NLName("reset"), {model->getScalarTerm(NLName("request"))},
    SNLDesignModeling::SNLActiveLevel::Low);
  auto* storage = model->getInstance(NLName("storage"));
  auto* dataPin = storage->getInstTerm(NLDB0::getDLatchData());
  SNLAttributes::addAttribute(dataPin, SNLAttribute(NLName("tag"), SNLAttributeValue("preserved")));
  SNLRTLInfos::create(dataPin)->setInfo(NLName("source_tag"), "preserved");
  // Keep a primitive pin with metadata after fusion, rather than only the
  // latch pin that will be removed in the specialized model.
  auto* inverter = *model->getInstances().begin();
  auto* pin = *inverter->getInstTerms().begin();
  SNLAttributes::addAttribute(pin, SNLAttribute(NLName("tag"), SNLAttributeValue("preserved")));
  SNLRTLInfos::create(pin)->setInfo(NLName("source_tag"), "preserved");
  auto pinID = pin->getBitTerm()->getID();
  auto inverterID = inverter->getID();
  auto* clock = port(top, "clock", SNLTerm::Direction::Input)->getNet();
  auto* output = net(top, "gated");
  auto* instance = instantiate(model, "clock_use", clock, output);
  instantiate(model, "data_use", port(top, "data", SNLTerm::Direction::Input)->getNet(),
    port(top, "result", SNLTerm::Direction::Output)->getNet());
  sink(output);
  EXPECT_EQ(1u, SNLClockGating::infer(top));
  auto* clone = instance->getModel();
  ASSERT_NE(model, clone);
  ASSERT_EQ(2u, clone->getClocks().size());
  for (auto* declaration : clone->getClocks()) {
    EXPECT_EQ(clone, declaration->getDesign());
    EXPECT_EQ(12 * (declaration->getKind() == NLClock::Kind::Primary ? 1 : 2), declaration->getPeriod());
    for (auto* source : declaration->getSources())
      EXPECT_EQ(clone, static_cast<SNLBitTerm*>(source)->getDesign());
    if (declaration->getKind() == NLClock::Kind::Generated) {
      EXPECT_EQ(clone, declaration->getMaster()->getDesign());
      EXPECT_EQ(1, declaration->getMaster()->getRiseAt());
      EXPECT_EQ(5, declaration->getMaster()->getFallAt());
    }
  }
  ASSERT_EQ(1u, clone->getResets().size());
  EXPECT_EQ(clone, (*clone->getResets().begin())->getDesign());
  auto* copied = clone->getInstance(inverterID)->getInstTermByFlatID(pinID);
  EXPECT_EQ(1u, SNLAttributes::getAttributes(copied).size());
  ASSERT_NE(nullptr, copied->getRTLInfos());
  EXPECT_EQ("preserved", copied->getRTLInfos()->getInfo(NLName("source_tag")));
}
