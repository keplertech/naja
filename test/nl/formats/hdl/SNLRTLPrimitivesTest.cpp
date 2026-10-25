// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLInstParameter.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLRTLPrimitives.h"

using namespace naja::NL;

class SNLRTLPrimitivesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto* db = NLDB::create(NLUniverse::create());
      library_ = NLLibrary::create(db, NLLibrary::Type::Standard);
      design_ = SNLDesign::create(library_, NLName("top"));
    }
    void TearDown() override { NLUniverse::get()->destroy(); }
    NLLibrary* library_ {};
    SNLDesign* design_ {};
};

TEST_F(SNLRTLPrimitivesTest, ROMStoresInstanceContentsAndPreservesBitOrder) {
  auto* zero = SNLScalarNet::create(design_); zero->setType(SNLNet::Type::Assign0);
  auto* one = SNLScalarNet::create(design_); one->setType(SNLNet::Type::Assign1);
  auto* x = SNLScalarNet::create(design_); x->setType(SNLNet::Type::AssignX);
  auto* z = SNLScalarNet::create(design_); z->setType(SNLNet::Type::AssignZ);
  auto* address = SNLBusNet::create(design_, 4, 5);
  auto* output = SNLBusNet::create(design_, 7, 9);
  SNLRTLPrimitives::Bits data{one,zero,one, zero,one,one, one,one,zero};
  SNLRTLPrimitives::Bits select{address->getBit(5),address->getBit(4)};
  auto* rom = SNLRTLPrimitives::createROM(design_, data, select, output);
  const NLDB0::MemorySignature expected{3,3,2,1,0};
  EXPECT_EQ(expected, NLDB0::getMemorySignature(rom));
  EXPECT_EQ("9'h0f5", rom->getInstParameter(NLName("INIT"))->getValue());
  EXPECT_EQ("1", rom->getInstParameter(NLName("INIT_ENABLE"))->getValue());
  EXPECT_EQ(address->getBit(5), rom->getInstTerm(NLDB0::getMemoryReadAddress(rom->getModel())->getBit(0))->getNet());
  EXPECT_EQ(output->getBit(9), rom->getInstTerm(NLDB0::getMemoryReadData(rom->getModel())->getBit(0))->getNet());
  data[0] = x; data[1] = z;
  auto* other = SNLRTLPrimitives::createROM(design_, data, select, SNLBusNet::create(design_,2,0));
  EXPECT_EQ(rom->getModel(), other->getModel());
  EXPECT_EQ("9'b0111101zx", other->getInstParameter(NLName("INIT"))->getValue());
  const auto count = design_->getInstances().size();
  data[0] = address->getBit(4);
  EXPECT_THROW(SNLRTLPrimitives::createROM(design_, data, select, output), NLException);
  EXPECT_EQ(count, design_->getInstances().size());
}

TEST_F(SNLRTLPrimitivesTest, MuxPreservesReorderedInputsAndAscendingOutput) {
  auto* select = SNLScalarNet::create(design_);
  auto* a = SNLBusNet::create(design_, 7, 4);
  auto* b = SNLBusNet::create(design_, 2, 5);
  auto* out = SNLBusNet::create(design_, 10, 13);
  SNLRTLPrimitives::Bits aBits {a->getBit(6), a->getBit(4), a->getBit(7), a->getBit(5)};
  SNLRTLPrimitives::Bits bBits {b->getBit(5), b->getBit(4), b->getBit(3), b->getBit(2)};
  auto* instance = SNLRTLPrimitives::createMux(design_, select, aBits, bBits, out);
  auto* model = NLDB0::getOrCreateMux2(4);
  ASSERT_EQ(model, instance->getModel());
  EXPECT_EQ(select, instance->getInstTerm(NLDB0::getMux2Select(model))->getNet());
  for (NLID::Bit bit = 0; bit < 4; ++bit) {
    EXPECT_EQ(aBits[bit], instance->getInstTerm(NLDB0::getMux2InputA(model)->getBit(bit))->getNet());
    EXPECT_EQ(bBits[bit], instance->getInstTerm(NLDB0::getMux2InputB(model)->getBit(bit))->getNet());
    EXPECT_EQ(out->getBit(13 - bit),
      instance->getInstTerm(NLDB0::getMux2Output(model)->getBit(bit))->getNet());
  }
}

TEST_F(SNLRTLPrimitivesTest, MuxAcceptsScalarOutputAndConstantInput) {
  auto* select = SNLScalarNet::create(design_);
  auto* a = SNLScalarNet::create(design_);
  a->setType(SNLNet::Type::Assign0);
  auto* b = SNLScalarNet::create(design_);
  auto* out = SNLScalarNet::create(design_);
  auto* instance = SNLRTLPrimitives::createMux(design_, select, {a}, {b}, out);
  EXPECT_EQ(a, instance->getInstTerm(NLDB0::getMux2InputA()->getBit(0))->getNet());
  EXPECT_EQ(out, instance->getInstTerm(NLDB0::getMux2Output()->getBit(0))->getNet());
}

TEST_F(SNLRTLPrimitivesTest, DFFAcceptsOneBitBusesWithoutFrontendState) {
  auto* clock = SNLScalarNet::create(design_);
  auto* data = SNLBusNet::create(design_, 7, 7);
  auto* out = SNLBusNet::create(design_, -2, -2);
  auto* instance = SNLRTLPrimitives::createDFF(design_, clock, data, out);
  EXPECT_EQ(NLDB0::getDFF(), instance->getModel());
  EXPECT_EQ(clock, instance->getInstTerm(NLDB0::getDFFClock())->getNet());
  EXPECT_EQ(data->getBit(7), instance->getInstTerm(NLDB0::getDFFData())->getNet());
  EXPECT_EQ(out->getBit(-2), instance->getInstTerm(NLDB0::getDFFOutput())->getNet());
}

TEST_F(SNLRTLPrimitivesTest, DFFEUsesCanonicalActiveHighEnableModel) {
  auto* clock = SNLScalarNet::create(design_);
  auto* data = SNLScalarNet::create(design_);
  auto* enable = SNLScalarNet::create(design_);
  auto* out = SNLScalarNet::create(design_);
  auto* instance = SNLRTLPrimitives::createDFFE(
      design_, clock, data, enable, out);
  EXPECT_EQ(NLDB0::getDFFE(), instance->getModel());
  EXPECT_EQ(clock, instance->getInstTerm(NLDB0::getDFFEClock())->getNet());
  EXPECT_EQ(data, instance->getInstTerm(NLDB0::getDFFEData())->getNet());
  EXPECT_EQ(enable, instance->getInstTerm(NLDB0::getDFFEEnable())->getNet());
  EXPECT_EQ(out, instance->getInstTerm(NLDB0::getDFFEOutput())->getNet());
}

TEST_F(SNLRTLPrimitivesTest, DFFSRUsesCanonicalSynchronousResetModel) {
  auto* clock = SNLScalarNet::create(design_);
  auto* data = SNLScalarNet::create(design_);
  auto* reset = SNLScalarNet::create(design_);
  auto* out = SNLScalarNet::create(design_);
  auto* instance = SNLRTLPrimitives::createDFFSR(
      design_, clock, data, reset, out);
  EXPECT_EQ(NLDB0::getDFFSR(), instance->getModel());
  EXPECT_EQ(clock, instance->getInstTerm(NLDB0::getDFFSRClock())->getNet());
  EXPECT_EQ(data, instance->getInstTerm(NLDB0::getDFFSRData())->getNet());
  EXPECT_EQ(reset, instance->getInstTerm(NLDB0::getDFFSRReset())->getNet());
  EXPECT_EQ(out, instance->getInstTerm(NLDB0::getDFFSROutput())->getNet());
}

TEST_F(SNLRTLPrimitivesTest, DFFSREUsesCanonicalResetAndEnableModel) {
  auto* clock = SNLScalarNet::create(design_);
  auto* data = SNLScalarNet::create(design_);
  auto* enable = SNLScalarNet::create(design_);
  auto* reset = SNLScalarNet::create(design_);
  auto* out = SNLScalarNet::create(design_);
  auto* instance = SNLRTLPrimitives::createDFFSRE(
      design_, clock, data, enable, reset, out);
  auto* model = NLDB0::getDFFSRE();
  EXPECT_EQ(model, instance->getModel());
  EXPECT_EQ(clock, instance->getInstTerm(model->getScalarTerm(NLName("C")))->getNet());
  EXPECT_EQ(data, instance->getInstTerm(model->getScalarTerm(NLName("D")))->getNet());
  EXPECT_EQ(enable, instance->getInstTerm(model->getScalarTerm(NLName("E")))->getNet());
  EXPECT_EQ(reset, instance->getInstTerm(model->getScalarTerm(NLName("R")))->getNet());
  EXPECT_EQ(out, instance->getInstTerm(model->getScalarTerm(NLName("Q")))->getNet());
}

TEST_F(SNLRTLPrimitivesTest, GatesUseCanonicalModelsAndCallerOutput) {
  auto* a = SNLScalarNet::create(design_);
  auto* b = SNLScalarNet::create(design_);
  auto* andOut = SNLScalarNet::create(design_);
  auto* notOut = SNLScalarNet::create(design_);
  auto* andInstance = SNLRTLPrimitives::createGate(
      design_, SNLRTLPrimitives::GateKind::And, {a, b}, andOut);
  auto* andModel = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2);
  EXPECT_EQ(andInstance->getModel(), andModel);
  EXPECT_EQ(andInstance->getInstTerm(
                NLDB0::getGateNTerms(andModel)->getBitAtPosition(0))->getNet(), a);
  EXPECT_EQ(andInstance->getInstTerm(
                NLDB0::getGateNTerms(andModel)->getBitAtPosition(1))->getNet(), b);
  EXPECT_EQ(andInstance->getInstTerm(NLDB0::getGateSingleTerm(andModel))->getNet(), andOut);

  auto* notInstance = SNLRTLPrimitives::createGate(
      design_, SNLRTLPrimitives::GateKind::Not, {andOut}, notOut);
  auto* notModel = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1);
  EXPECT_EQ(notInstance->getModel(), notModel);
  EXPECT_EQ(notInstance->getInstTerm(NLDB0::getGateSingleTerm(notModel))->getNet(), andOut);
  EXPECT_EQ(notInstance->getInstTerm(
                NLDB0::getGateNTerms(notModel)->getBitAtPosition(0))->getNet(), notOut);
}

TEST_F(SNLRTLPrimitivesTest, BitwiseGateMapsRightmostBitsToHardwarePositionZero) {
  auto* input = SNLBusNet::create(design_, -1, 2);
  auto* output = SNLBusNet::create(design_, 5, 2);
  const auto instances = SNLRTLPrimitives::createBitwiseGate(
      design_, SNLRTLPrimitives::GateKind::Buf, {input}, output);
  ASSERT_EQ(instances.size(), 4);
  auto* model = NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Buf, 1);
  for (size_t position = 0; position < instances.size(); ++position) {
    auto* instance = instances[position];
    EXPECT_EQ(instance->getModel(), model);
    EXPECT_EQ(instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet(),
              input->getBitAtPosition(3 - position));
    EXPECT_EQ(instance->getInstTerm(
                  NLDB0::getGateNTerms(model)->getBitAtPosition(0))->getNet(),
              output->getBitAtPosition(3 - position));
  }
}

TEST_F(SNLRTLPrimitivesTest, RejectsInvalidInputsBeforeCreatingInstances) {
  auto* bit = SNLScalarNet::create(design_);
  auto* bus = SNLBusNet::create(design_, 1, 0);
  auto* other = SNLDesign::create(library_, NLName("other"));
  auto* foreign = SNLScalarNet::create(other);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {}, {}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {bit, bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, nullptr, {bit}, {bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {nullptr}, {bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {foreign}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {bit}, bus), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {bit}, foreign), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(nullptr, bit, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(design_, bit, bus, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(design_, foreign, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(design_, bit, bit, nullptr), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFE(nullptr, bit, bit, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFE(design_, bit, bus, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFE(design_, bit, bit, foreign, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFE(design_, bit, bit, bit, nullptr), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSR(nullptr, bit, bit, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSR(design_, bit, bus, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSR(design_, bit, bit, foreign, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSR(design_, bit, bit, bit, nullptr), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSRE(nullptr, bit, bit, bit, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSRE(design_, bit, bus, bit, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSRE(design_, bit, bit, foreign, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSRE(design_, bit, bit, bit, foreign, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFFSRE(design_, bit, bit, bit, bit, nullptr), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createGate(
      design_, SNLRTLPrimitives::GateKind::And, {}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createGate(
      design_, SNLRTLPrimitives::GateKind::Not, {bit, bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createGate(
      design_, SNLRTLPrimitives::GateKind::And, {bit, foreign}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createGate(
      design_, SNLRTLPrimitives::GateKind::And, {bit}, bus), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createGate(
      design_, static_cast<SNLRTLPrimitives::GateKind>(-1), {bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createBitwiseGate(
      design_, SNLRTLPrimitives::GateKind::And, {bus, bit}, bus), NLException);
  EXPECT_TRUE(design_->getInstances().empty());
}

TEST_F(SNLRTLPrimitivesTest, AssignRejectsInvalidNetsBeforeCreatingInstances) {
  auto* input = SNLBusNet::create(design_, 3, 0);
  auto* output = SNLBusNet::create(design_, 0, 3);
  auto* scalar = SNLScalarNet::create(design_);
  auto* other = SNLDesign::create(library_, NLName("other"));
  auto* foreign = SNLBusNet::create(other, 3, 0);
  EXPECT_THROW(SNLRTLPrimitives::createAssign(design_, nullptr, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createAssign(design_, input, nullptr), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createAssign(design_, scalar, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createAssign(design_, foreign, output), NLException);
  EXPECT_TRUE(design_->getInstances().empty());
  const auto instances = SNLRTLPrimitives::createAssign(design_, input, output);
  ASSERT_EQ(instances.size(), 4);
  for (size_t position = 0; position < instances.size(); ++position) {
    EXPECT_EQ(instances[position]->getModel(), NLDB0::getAssign());
    EXPECT_EQ(instances[position]->getInstTerm(NLDB0::getAssignInput())->getNet(), input->getBit(position));
    EXPECT_EQ(instances[position]->getInstTerm(NLDB0::getAssignOutput())->getNet(), output->getBit(3 - position));
  }
}

TEST_F(SNLRTLPrimitivesTest, VectorUpdateFactoryPreservesBitOrderAndValidatesInputs) {
  auto* base = SNLBusNet::create(design_, 7, 0);
  auto* data = SNLBusNet::create(design_, 2, 0);
  auto* offset = SNLBusNet::create(design_, 39, 0);
  auto* enable = SNLScalarNet::create(design_);
  auto* output = SNLBusNet::create(design_, 0, 7);
  SNLRTLPrimitives::Bits b, d, a;
  for (unsigned bit = 0; bit < 8; ++bit) b.push_back(base->getBit(bit));
  for (unsigned bit = 0; bit < 3; ++bit) d.push_back(data->getBit(bit));
  for (unsigned bit = 0; bit < 40; ++bit) a.push_back(offset->getBit(bit));
  auto* instance = SNLRTLPrimitives::createVectorUpdate(design_, b, d, a, true, enable, output);
  auto* model = instance->getModel();
  EXPECT_EQ((NLDB0::VectorUpdateSignature{8, 3, 40, true}), NLDB0::getVectorUpdateSignature(instance));
  for (unsigned bit = 0; bit < 8; ++bit) {
    EXPECT_EQ(b[bit], instance->getInstTerm(NLDB0::getVectorUpdateBase(model)->getBit(bit))->getNet());
    EXPECT_EQ(output->getBit(7-bit),
        instance->getInstTerm(NLDB0::getVectorUpdateOutput(model)->getBit(bit))->getNet());
  }
  const auto count = design_->getInstances().size();
  EXPECT_THROW(SNLRTLPrimitives::createVectorUpdate(design_, {}, d, a, true, enable, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createVectorUpdate(design_, b, {}, a, true, enable, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createVectorUpdate(design_, b, d, {}, true, enable, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createVectorUpdate(design_, b, d, a, true, nullptr, output), NLException);
  auto foreign = b;
  auto* other = SNLDesign::create(library_, NLName("other"));
  foreign[0] = SNLScalarNet::create(other);
  EXPECT_THROW(SNLRTLPrimitives::createVectorUpdate(design_, foreign, d, a, true, enable, output), NLException);
  EXPECT_EQ(count, design_->getInstances().size());
  SNLInstParameter::create(instance, model->getParameter(NLName("SIGNED")), "0");
  EXPECT_THROW(NLDB0::getVectorUpdateSignature(instance), NLException);
}

TEST_F(SNLRTLPrimitivesTest, VectorSelectFactoryPreservesBitOrderAndValidatesInputs) {
  auto* data = SNLBusNet::create(design_, 7, 0);
  auto* offset = SNLBusNet::create(design_, 79, 0);
  auto* output = SNLBusNet::create(design_, 0, 2);
  SNLRTLPrimitives::Bits d, a;
  for (unsigned bit = 0; bit < 8; ++bit) d.push_back(data->getBit(bit));
  for (unsigned bit = 0; bit < 80; ++bit) a.push_back(offset->getBit(bit));
  auto* instance = SNLRTLPrimitives::createVectorSelect(design_, d, a, true, output);
  auto* model = instance->getModel();
  EXPECT_EQ((NLDB0::VectorSelectSignature{8, 3, 80, true}), NLDB0::getVectorSelectSignature(instance));
  for (unsigned bit = 0; bit < 3; ++bit)
    EXPECT_EQ(output->getBit(2-bit),
        instance->getInstTerm(NLDB0::getVectorSelectOutput(model)->getBit(bit))->getNet());
  const auto count = design_->getInstances().size();
  EXPECT_THROW(SNLRTLPrimitives::createVectorSelect(design_, {}, a, true, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createVectorSelect(design_, d, {}, true, output), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createVectorSelect(design_, d, a, true, nullptr), NLException);
  auto* other = SNLDesign::create(library_, NLName("other_select"));
  d[0] = SNLScalarNet::create(other);
  EXPECT_THROW(SNLRTLPrimitives::createVectorSelect(design_, d, a, true, output), NLException);
  EXPECT_EQ(count, design_->getInstances().size());
  SNLInstParameter::create(instance, model->getParameter(NLName("SIGNED")), "0");
  EXPECT_THROW(NLDB0::getVectorSelectSignature(instance), NLException);
}
