// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NajaDumpableProperty.h"

#include "SNLDesignModeling.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
using namespace naja::NL;

class SNLDesignModelingTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignModelingTest0, canonicalMemoryTermRolesMatchInterface) {
  NLUniverse::create();
  using Modeling = SNLDesignModeling;
  using Role = Modeling::SNLTermRole;
  for (auto mode : {NLDB0::MemoryResetMode::None, NLDB0::MemoryResetMode::AsyncLow,
                    NLDB0::MemoryResetMode::AsyncHigh, NLDB0::MemoryResetMode::SyncLow,
                    NLDB0::MemoryResetMode::SyncHigh}) {
    for (bool independent : {false, true}) {
      if (independent && mode != NLDB0::MemoryResetMode::None) continue;
      for (size_t writes : {0u, 3u}) {
        if (!writes && (independent || mode != NLDB0::MemoryResetMode::None)) continue;
        auto* memory = NLDB0::getOrCreateMemory({7, 5, 3, 2, writes, mode, independent});
        const auto interface = Modeling::getMemoryInterface(memory);
        auto check = [](const auto& terms, Role role) {
          for (const auto* term : terms) EXPECT_EQ(role, Modeling::getTermRole(term));
        };
        if (interface.clock) EXPECT_EQ(Role::Clock, Modeling::getTermRole(interface.clock));
        if (interface.reset) {
          auto role = Role::Other;
          if (mode == NLDB0::MemoryResetMode::AsyncLow || mode == NLDB0::MemoryResetMode::AsyncHigh)
            role = Role::AsyncReset;
          if (mode == NLDB0::MemoryResetMode::SyncLow || mode == NLDB0::MemoryResetMode::SyncHigh)
            role = Role::SyncReset;
          EXPECT_EQ(role, Modeling::getTermRole(interface.reset));
        }
        for (const auto& port : interface.readPorts) {
          check(port.address, Role::MemoryReadAddress);
          check(port.data, Role::MemoryReadData);
        }
        for (const auto& port : interface.writePorts) {
          check(port.address, Role::MemoryWriteAddress);
          check(port.data, Role::MemoryWriteData);
          check(port.mask, Role::MemoryWriteEnable);
          check(port.enables, Role::MemoryWriteEnable);
          if (port.clock) EXPECT_EQ(Role::Clock, Modeling::getTermRole(port.clock));
        }
      }
    }
  }
}

TEST_F(SNLDesignModelingTest0, testCombinatorial) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto lut = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("LUT"));
  auto luti0 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I0"));
  auto luti1 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I1"));
  auto luti2 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I2"));
  auto luti3 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I3"));
  auto luto = SNLScalarTerm::create(lut, SNLTerm::Direction::Output, NLName("O"));
  EXPECT_FALSE(SNLDesignModeling::hasModeling(lut));
  SNLDesignModeling::addCombinatorialArcs({luti0, luti1, luti2, luti3}, {luto});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(lut));
  EXPECT_FALSE(SNLDesignModeling::isSequential(lut));
  auto lutIns0 = SNLInstance::create(top, lut, NLName("ins0"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(luto).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(luto).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(luto).begin(),
      SNLDesignModeling::getCombinatorialInputs(luto).end()),
    ElementsAre(luti0, luti1, luti2, luti3));
  //lutIns0/luto
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luto)).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).begin(),
      SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).end()),
    ElementsAre(
      lutIns0->getInstTerm(luti0),
      lutIns0->getInstTerm(luti1),
      lutIns0->getInstTerm(luti2),
      lutIns0->getInstTerm(luti3)));
  
  //luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti0).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti0).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti0).end()),
    ElementsAre(luto));
  //lutIns0/luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luti0)).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).begin(),
      SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).end()),
    ElementsAre(lutIns0->getInstTerm(luto)));

  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti1).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti1).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti1).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti2).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti2).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti2).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti3).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti3).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti3).end()),
    ElementsAre(luto));
}

TEST_F(SNLDesignModelingTest0, testSequential) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("REG"));
  auto regD = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("D"));
  auto regQ = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("Q"));
  auto regC = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("C"));

  EXPECT_FALSE(SNLDesignModeling::hasModeling(reg));
  SNLDesignModeling::addInputsToClockArcs({regD}, regC);
  SNLDesignModeling::addClockToOutputsArcs(regC, {regQ});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(reg));
  EXPECT_TRUE(SNLDesignModeling::isSequential(reg));

  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(regD).empty());

  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedInputs(regC).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedInputs(regC).begin(),
      SNLDesignModeling::getClockRelatedInputs(regC).end()),
    ElementsAre(regD));
  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedOutputs(regC).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedOutputs(regC).begin(),
      SNLDesignModeling::getClockRelatedOutputs(regC).end()),
    ElementsAre(regQ));
  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(regD).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getInputRelatedClocks(regD).begin(),
      SNLDesignModeling::getInputRelatedClocks(regD).end()),
    ElementsAre(regC));
  EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(regQ).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getOutputRelatedClocks(regQ).begin(),
      SNLDesignModeling::getOutputRelatedClocks(regQ).end()),
    ElementsAre(regC));

  auto regIns = SNLInstance::create(top, reg, NLName("regIns"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regD)).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regQ)).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regD)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regQ)).empty());

  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).begin(),
      SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).end()),
    ElementsAre(regIns->getInstTerm(regD)));
  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).begin(),
      SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).end()),
    ElementsAre(regIns->getInstTerm(regQ)));
  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).begin(),
      SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).end()),
    ElementsAre(regIns->getInstTerm(regC)));
  EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).begin(),
      SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).end()),
    ElementsAre(regIns->getInstTerm(regC)));
}

TEST_F(SNLDesignModelingTest0, testExplicitTermRolesAndSafeDefaults) {
  using Role = SNLDesignModeling::SNLTermRole;
  using Level = SNLDesignModeling::SNLActiveLevel;
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* designs = NLLibrary::create(db);
  auto* top = SNLDesign::create(designs, NLName("top"));
  auto* reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("REG"));
  auto* clock = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("CLK"));
  auto* data = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("DIN"));
  auto* reset = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("RESET_B"));
  auto* set = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("SET"));
  auto* syncReset = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("SYNC_RESET"));
  auto* syncSet = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("SYNC_SET_B"));
  auto* output = SNLScalarTerm::create(reg, SNLTerm::Direction::Output, NLName("DOUT"));
  auto* other = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("UNMODELED"));

  SNLDesignModeling::setTermRole(clock, Role::Clock);
  SNLDesignModeling::setTermRole(data, Role::DataInput);
  SNLDesignModeling::setTermRole(reset, Role::AsyncReset, Level::Low);
  SNLDesignModeling::setTermRole(set, Role::AsyncSet, Level::High);
  SNLDesignModeling::setTermRole(syncReset, Role::SyncReset, Level::High);
  SNLDesignModeling::setTermRole(syncSet, Role::SyncSet, Level::Low);
  SNLDesignModeling::setTermRole(output, Role::DataOutput);

  EXPECT_TRUE(SNLDesignModeling::isClock(clock));
  EXPECT_TRUE(SNLDesignModeling::isDataInput(data));
  EXPECT_TRUE(SNLDesignModeling::isAsyncReset(reset));
  EXPECT_TRUE(SNLDesignModeling::isAsyncSet(set));
  EXPECT_TRUE(SNLDesignModeling::isSyncReset(syncReset));
  EXPECT_TRUE(SNLDesignModeling::isSyncSet(syncSet));
  EXPECT_TRUE(SNLDesignModeling::isReset(reset));
  EXPECT_TRUE(SNLDesignModeling::isReset(syncReset));
  EXPECT_EQ(Level::Low, SNLDesignModeling::getResetActiveLevel(reset));
  EXPECT_EQ(Level::High, SNLDesignModeling::getResetActiveLevel(set));
  EXPECT_EQ(Level::High, SNLDesignModeling::getResetActiveLevel(syncReset));
  EXPECT_EQ(Level::Low, SNLDesignModeling::getResetActiveLevel(syncSet));
  EXPECT_TRUE(SNLDesignModeling::isDataOutput(output));
  EXPECT_EQ(Role::Other, SNLDesignModeling::getTermRole(other));
  EXPECT_EQ(Level::NA, SNLDesignModeling::getResetActiveLevel(other));

  EXPECT_THAT(std::vector(SNLDesignModeling::getClockTerms(reg).begin(),
                          SNLDesignModeling::getClockTerms(reg).end()),
              ElementsAre(clock));
  EXPECT_THAT(std::vector(SNLDesignModeling::getAsyncResetTerms(reg).begin(),
                          SNLDesignModeling::getAsyncResetTerms(reg).end()),
              ElementsAre(reset));
  EXPECT_THAT(std::vector(SNLDesignModeling::getAsyncSetTerms(reg).begin(),
                          SNLDesignModeling::getAsyncSetTerms(reg).end()),
              ElementsAre(set));
  EXPECT_THAT(std::vector(SNLDesignModeling::getSyncResetTerms(reg).begin(),
                          SNLDesignModeling::getSyncResetTerms(reg).end()),
              ElementsAre(syncReset));
  EXPECT_THAT(std::vector(SNLDesignModeling::getSyncSetTerms(reg).begin(),
                          SNLDesignModeling::getSyncSetTerms(reg).end()),
              ElementsAre(syncSet));
  EXPECT_THAT(std::vector(SNLDesignModeling::getDataInputTerms(reg).begin(),
                          SNLDesignModeling::getDataInputTerms(reg).end()),
              ElementsAre(data));
  EXPECT_THAT(std::vector(SNLDesignModeling::getOutputTerms(reg).begin(),
                          SNLDesignModeling::getOutputTerms(reg).end()),
              ElementsAre(output));

  auto* instance = SNLInstance::create(top, reg, NLName("reg"));
  EXPECT_TRUE(SNLDesignModeling::isClock(instance->getInstTerm(clock)));
  EXPECT_TRUE(SNLDesignModeling::isAsyncReset(instance->getInstTerm(reset)));
  EXPECT_TRUE(SNLDesignModeling::isAsyncSet(instance->getInstTerm(set)));
  EXPECT_TRUE(SNLDesignModeling::isSyncReset(instance->getInstTerm(syncReset)));
  EXPECT_TRUE(SNLDesignModeling::isSyncSet(instance->getInstTerm(syncSet)));
  EXPECT_TRUE(SNLDesignModeling::isReset(instance->getInstTerm(reset)));
  EXPECT_TRUE(SNLDesignModeling::isReset(instance->getInstTerm(syncReset)));
  EXPECT_TRUE(SNLDesignModeling::isDataInput(instance->getInstTerm(data)));
  EXPECT_TRUE(SNLDesignModeling::isDataOutput(instance->getInstTerm(output)));
  EXPECT_EQ(Level::Low,
            SNLDesignModeling::getResetActiveLevel(instance->getInstTerm(reset)));

  auto* gate = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("GATE"));
  auto* gateInput = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("A"));
  EXPECT_EQ(Role::Other, SNLDesignModeling::getTermRole(gateInput));
  EXPECT_TRUE(SNLDesignModeling::getClockTerms(gate).empty());
  EXPECT_TRUE(SNLDesignModeling::getAsyncResetTerms(gate).empty());
  EXPECT_TRUE(SNLDesignModeling::getSyncResetTerms(gate).empty());
  EXPECT_TRUE(SNLDesignModeling::getSyncSetTerms(gate).empty());
  EXPECT_TRUE(SNLDesignModeling::getDataInputTerms(gate).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputTerms(gate).empty());
}

TEST_F(SNLDesignModelingTest0, testReadOnlyMemoryInterfaceIsCombinatorial) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* library = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("lookup"));
  auto* address = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("A"));
  auto* data = SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("Y"));
  SNLDesignModeling::MemoryInterface memory;
  memory.width = 1; memory.depth = 2; memory.abits = 1;
  memory.readPorts.push_back({{address}, {data}, {}});
  ASSERT_TRUE(memory.isValid());
  SNLDesignModeling::setMemoryInterface(model, memory);
  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(model));
  EXPECT_FALSE(SNLDesignModeling::isSequential(model));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::MemoryReadAddress, SNLDesignModeling::getTermRole(address));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::MemoryReadData, SNLDesignModeling::getTermRole(data));
}

TEST_F(SNLDesignModelingTest0, testManualMemoryInterfaceSurvivesOnInstances) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM1P"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto ce = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CE"));
  auto we = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE"));
  auto addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("ADDR"));
  auto wdata = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("WDATA"));
  auto wmask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("WMASK"));
  auto rdata = SNLBusTerm::create(mem, SNLTerm::Direction::Output, 3, 0, NLName("RDATA"));

  SNLDesignModeling::BitTerms readDataBits;
  SNLDesignModeling::BitTerms updateInputs;
  for (int bit = 0; bit <= 3; ++bit) {
    readDataBits.push_back(static_cast<SNLBitTerm*>(rdata->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(wdata->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(wmask->getBit(bit)));
    if (bit <= 1) {
      updateInputs.push_back(static_cast<SNLBitTerm*>(addr->getBit(bit)));
    }
  }
  updateInputs.push_back(ce);
  updateInputs.push_back(we);

  SNLDesignModeling::addClockToOutputsArcs(clk, readDataBits);
  SNLDesignModeling::addInputsToClockArcs(updateInputs, clk);

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 4;
  interface.depth = 4;
  interface.abits = 2;
  interface.clock = clk;
  SNLDesignModeling::MemoryReadPort readPort;
  readPort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(0)));
  readPort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(1)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(0)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(1)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(2)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(3)));
  interface.readPorts.push_back(readPort);

  SNLDesignModeling::MemoryWritePort writePort;
  writePort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(0)));
  writePort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(1)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(0)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(1)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(2)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(3)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(0)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(1)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(2)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(3)));
  writePort.enables.push_back(ce);
  writePort.enables.push_back(we);
  interface.writePorts.push_back(writePort);
  SNLDesignModeling::setMemoryInterface(mem, interface);

  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(mem));
  const auto modelInterface = SNLDesignModeling::getMemoryInterface(mem);
  EXPECT_EQ(4u, modelInterface.width);
  EXPECT_EQ(4u, modelInterface.depth);
  EXPECT_EQ(2u, modelInterface.abits);

  auto inst = SNLInstance::create(top, mem, NLName("mem0"));
  // Guard the connected-instance path: once a memory interface is projected to
  // a concrete instance, fully wired ports must remain visible.
  auto* clkNet = SNLScalarNet::create(top, NLName("clk"));
  auto* ceNet = SNLScalarNet::create(top, NLName("ce"));
  auto* weNet = SNLScalarNet::create(top, NLName("we"));
  inst->getInstTerm(clk)->setNet(clkNet);
  inst->getInstTerm(ce)->setNet(ceNet);
  inst->getInstTerm(we)->setNet(weNet);
  for (int bit = 0; bit <= 1; ++bit) {
    auto* addrNet =
        SNLScalarNet::create(top, NLName("addr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(addr->getBit(bit)))->setNet(addrNet);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* wdataNet =
        SNLScalarNet::create(top, NLName("wdata_" + std::to_string(bit)));
    auto* wmaskNet =
        SNLScalarNet::create(top, NLName("wmask_" + std::to_string(bit)));
    auto* rdataNet =
        SNLScalarNet::create(top, NLName("rdata_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(wdata->getBit(bit)))->setNet(wdataNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(wmask->getBit(bit)))->setNet(wmaskNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(rdata->getBit(bit)))->setNet(rdataNet);
  }
  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(inst->getModel()));
  const auto instInterface = SNLDesignModeling::getMemoryInterface(inst);
  EXPECT_EQ(1u, instInterface.readPorts.size());
  EXPECT_EQ(1u, instInterface.writePorts.size());
  EXPECT_EQ(4u, instInterface.readPorts.front().data.size());
  EXPECT_EQ(4u, instInterface.writePorts.front().mask.size());
  EXPECT_EQ(2u, instInterface.writePorts.front().enables.size());

  for (int bit = 0; bit <= 3; ++bit) {
    auto* instReadBit =
        inst->getInstTerm(static_cast<SNLBitTerm*>(rdata->getBit(bit)));
    EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(instReadBit).size());
    EXPECT_EQ(inst->getInstTerm(clk),
              *SNLDesignModeling::getOutputRelatedClocks(instReadBit).begin());
  }
}

TEST_F(SNLDesignModelingTest0,
       testMemoryInterfaceProvidesSequentialRelationsWithoutExplicitArcs) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM1P"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto rst = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("RST"));
  auto we = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE"));
  auto addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("ADDR"));
  auto wdata = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("WDATA"));
  auto wmask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("WMASK"));
  auto wextra = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 0, 0, NLName("WEXTRA"));
  auto rdata = SNLBusTerm::create(mem, SNLTerm::Direction::Output, 1, 0, NLName("RDATA"));

  auto* addr0 = static_cast<SNLBitTerm*>(addr->getBit(0));
  auto* addr1 = static_cast<SNLBitTerm*>(addr->getBit(1));
  auto* wdata0 = static_cast<SNLBitTerm*>(wdata->getBit(0));
  auto* wdata1 = static_cast<SNLBitTerm*>(wdata->getBit(1));
  auto* wmask0 = static_cast<SNLBitTerm*>(wmask->getBit(0));
  auto* wmask1 = static_cast<SNLBitTerm*>(wmask->getBit(1));
  auto* wextra0 = static_cast<SNLBitTerm*>(wextra->getBit(0));
  auto* rdata0 = static_cast<SNLBitTerm*>(rdata->getBit(0));
  auto* rdata1 = static_cast<SNLBitTerm*>(rdata->getBit(1));

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 2;
  interface.depth = 4;
  interface.abits = 2;
  interface.resetMode = SNLDesignModeling::MemoryResetMode::AsyncHigh;
  interface.clock = clk;
  interface.reset = rst;
  interface.readPorts.push_back(
      {.address = {addr0, addr1}, .data = {rdata0, rdata1}, .enables = {we}});
  interface.writePorts.push_back(
      {.address = {addr0, addr1},
       .data = {wdata0, wdata1},
       .mask = {wmask0, wmask1},
       .enables = {we},
       .extraWriteInputs = {{wextra0}}});
  SNLDesignModeling::setMemoryInterface(mem, interface);

  // The memory interface itself is enough to describe the sequential timing
  // surface. SEC relies on these generic relations so memory read data is a
  // state output and write/read controls are next-state dependencies without
  // hardcoded primitive pin names.
  EXPECT_TRUE(SNLDesignModeling::hasModeling(mem));
  EXPECT_TRUE(SNLDesignModeling::isSequential(mem));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::Clock,
            SNLDesignModeling::getTermRole(clk));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::AsyncReset,
            SNLDesignModeling::getTermRole(rst));
  EXPECT_EQ(SNLDesignModeling::SNLActiveLevel::High,
            SNLDesignModeling::getResetActiveLevel(rst));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::MemoryReadAddress,
            SNLDesignModeling::getTermRole(addr0));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::MemoryReadData,
            SNLDesignModeling::getTermRole(rdata0));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::MemoryWriteData,
            SNLDesignModeling::getTermRole(wdata0));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::MemoryWriteEnable,
            SNLDesignModeling::getTermRole(wmask0));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::Enable,
            SNLDesignModeling::getTermRole(we));
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::Other,
            SNLDesignModeling::getTermRole(wextra0));
  EXPECT_TRUE(SNLDesignModeling::isClock(clk));
  EXPECT_TRUE(SNLDesignModeling::isAsyncReset(rst));
  EXPECT_TRUE(SNLDesignModeling::isReset(rst));
  EXPECT_TRUE(SNLDesignModeling::isEnable(we));
  EXPECT_TRUE(SNLDesignModeling::isDataInput(wdata1));
  EXPECT_TRUE(SNLDesignModeling::isDataOutput(rdata1));
  EXPECT_THAT(std::vector(SNLDesignModeling::getClockTerms(mem).begin(),
                          SNLDesignModeling::getClockTerms(mem).end()),
              ElementsAre(clk));
  EXPECT_THAT(std::vector(SNLDesignModeling::getAsyncResetTerms(mem).begin(),
                          SNLDesignModeling::getAsyncResetTerms(mem).end()),
              ElementsAre(rst));
  EXPECT_THAT(std::vector(SNLDesignModeling::getDataInputTerms(mem).begin(),
                          SNLDesignModeling::getDataInputTerms(mem).end()),
              UnorderedElementsAre(wdata0, wdata1));
  EXPECT_THAT(std::vector(SNLDesignModeling::getOutputTerms(mem).begin(),
                          SNLDesignModeling::getOutputTerms(mem).end()),
              UnorderedElementsAre(rdata0, rdata1));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getClockRelatedOutputs(clk).begin(),
                  SNLDesignModeling::getClockRelatedOutputs(clk).end()),
      UnorderedElementsAre(rdata0, rdata1));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getClockRelatedInputs(clk).begin(),
                  SNLDesignModeling::getClockRelatedInputs(clk).end()),
      UnorderedElementsAre(
          rst, we, addr0, addr1, wdata0, wdata1, wmask0, wmask1, wextra0));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getOutputRelatedClocks(rdata0).begin(),
                  SNLDesignModeling::getOutputRelatedClocks(rdata0).end()),
      ElementsAre(clk));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getInputRelatedClocks(wdata1).begin(),
                  SNLDesignModeling::getInputRelatedClocks(wdata1).end()),
      ElementsAre(clk));
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(wdata0).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(wdata0).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(rdata0).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(wdata0).empty());

  auto inst = SNLInstance::create(top, mem, NLName("mem0"));
  size_t netIndex = 0;
  for (auto* term : std::vector<SNLBitTerm*>{
           clk, rst, we, addr0, addr1, wdata0, wdata1, wmask0, wmask1, rdata0,
           wextra0, rdata1}) {
    auto* net = SNLScalarNet::create(
        top, NLName(std::string("n") + std::to_string(netIndex++)));
    inst->getInstTerm(term)->setNet(net);
  }

  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getClockRelatedOutputs(inst->getInstTerm(clk)).begin(),
          SNLDesignModeling::getClockRelatedOutputs(inst->getInstTerm(clk)).end()),
      UnorderedElementsAre(inst->getInstTerm(rdata0), inst->getInstTerm(rdata1)));
  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getClockRelatedInputs(inst->getInstTerm(clk)).begin(),
          SNLDesignModeling::getClockRelatedInputs(inst->getInstTerm(clk)).end()),
      UnorderedElementsAre(
          inst->getInstTerm(rst), inst->getInstTerm(we),
          inst->getInstTerm(addr0), inst->getInstTerm(addr1),
          inst->getInstTerm(wdata0), inst->getInstTerm(wdata1),
          inst->getInstTerm(wmask0), inst->getInstTerm(wmask1),
          inst->getInstTerm(wextra0)));
  EXPECT_TRUE(
      SNLDesignModeling::getClockRelatedInputs(inst->getInstTerm(wdata0)).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getClockRelatedOutputs(inst->getInstTerm(wdata0)).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(rdata0)).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getOutputRelatedClocks(inst->getInstTerm(wdata0)).empty());
  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(wmask0)).begin(),
          SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(wmask0)).end()),
      ElementsAre(inst->getInstTerm(clk)));
  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(wextra0)).begin(),
          SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(wextra0)).end()),
      ElementsAre(inst->getInstTerm(clk)));
  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getOutputRelatedClocks(inst->getInstTerm(rdata1)).begin(),
          SNLDesignModeling::getOutputRelatedClocks(inst->getInstTerm(rdata1)).end()),
      ElementsAre(inst->getInstTerm(clk)));
  EXPECT_TRUE(SNLDesignModeling::isClock(inst->getInstTerm(clk)));
  EXPECT_TRUE(SNLDesignModeling::isAsyncReset(inst->getInstTerm(rst)));
  EXPECT_TRUE(SNLDesignModeling::isReset(inst->getInstTerm(rst)));
  EXPECT_TRUE(SNLDesignModeling::isEnable(inst->getInstTerm(we)));
  EXPECT_TRUE(SNLDesignModeling::isDataInput(inst->getInstTerm(wdata0)));
  EXPECT_TRUE(SNLDesignModeling::isDataOutput(inst->getInstTerm(rdata0)));

  using Mode = SNLDesignModeling::MemoryResetMode;
  using Role = SNLDesignModeling::SNLTermRole;
  using Level = SNLDesignModeling::SNLActiveLevel;
  struct ResetCase { Mode mode; Role role; Level level; };
  for (const auto& resetCase : {
           ResetCase{Mode::None, Role::Other, Level::NA},
           ResetCase{Mode::AsyncLow, Role::AsyncReset, Level::Low},
           ResetCase{Mode::AsyncHigh, Role::AsyncReset, Level::High},
           ResetCase{Mode::SyncLow, Role::SyncReset, Level::Low},
           ResetCase{Mode::SyncHigh, Role::SyncReset, Level::High}}) {
    SCOPED_TRACE(static_cast<int>(resetCase.mode));
    interface.resetMode = resetCase.mode;
    SNLDesignModeling::setMemoryInterface(mem, interface);
    EXPECT_EQ(resetCase.role, SNLDesignModeling::getTermRole(rst));
    EXPECT_EQ(resetCase.level, SNLDesignModeling::getResetActiveLevel(rst));
    EXPECT_EQ(resetCase.role, SNLDesignModeling::getTermRole(inst->getInstTerm(rst)));
    EXPECT_EQ(resetCase.level,
              SNLDesignModeling::getResetActiveLevel(inst->getInstTerm(rst)));
    EXPECT_EQ(resetCase.role != Role::Other, SNLDesignModeling::isReset(rst));
    EXPECT_EQ(resetCase.role == Role::SyncReset, SNLDesignModeling::isSyncReset(rst));
  }

  auto plain = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("PLAIN"));
  auto plainIn = SNLScalarTerm::create(
      plain, SNLTerm::Direction::Input, NLName("I"));
  auto plainOut = SNLScalarTerm::create(
      plain, SNLTerm::Direction::Output, NLName("O"));
  auto plainInst = SNLInstance::create(top, plain, NLName("plain0"));
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(plainIn).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(plainIn).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(plainIn).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(plainOut).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getClockRelatedInputs(plainInst->getInstTerm(plainIn)).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getClockRelatedOutputs(plainInst->getInstTerm(plainIn)).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getInputRelatedClocks(plainInst->getInstTerm(plainIn)).empty());
  EXPECT_TRUE(
      SNLDesignModeling::getOutputRelatedClocks(plainInst->getInstTerm(plainOut)).empty());
}

TEST_F(SNLDesignModelingTest0,
       testInstanceMemoryInterfaceDropsDisconnectedPorts) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM2W"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto rst = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("RST"));
  auto raddr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("RADDR"));
  auto rdata = SNLBusTerm::create(mem, SNLTerm::Direction::Output, 3, 0, NLName("RDATA"));
  auto w0addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("W0_ADDR"));
  auto w0data = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W0_DATA"));
  auto w0mask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W0_MASK"));
  auto we0 = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE0"));
  auto w1addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("W1_ADDR"));
  auto w1data = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W1_DATA"));
  auto w1mask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W1_MASK"));
  auto we1 = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE1"));
  auto w1extra = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("W1_EXTRA"));

  SNLDesignModeling::BitTerms readDataBits;
  SNLDesignModeling::BitTerms updateInputs;
  for (int bit = 0; bit <= 3; ++bit) {
    readDataBits.push_back(static_cast<SNLBitTerm*>(rdata->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w0data->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w0mask->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w1data->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w1mask->getBit(bit)));
    if (bit <= 1) {
      updateInputs.push_back(static_cast<SNLBitTerm*>(raddr->getBit(bit)));
      updateInputs.push_back(static_cast<SNLBitTerm*>(w0addr->getBit(bit)));
      updateInputs.push_back(static_cast<SNLBitTerm*>(w1addr->getBit(bit)));
    }
  }
  updateInputs.push_back(we0);
  updateInputs.push_back(we1);

  SNLDesignModeling::addClockToOutputsArcs(clk, readDataBits);
  SNLDesignModeling::addInputsToClockArcs(updateInputs, clk);

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 4;
  interface.depth = 4;
  interface.abits = 2;
  interface.clock = clk;
  interface.reset = rst;
  interface.readPorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(raddr->getBit(0)),
                   static_cast<SNLBitTerm*>(raddr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(rdata->getBit(0)),
                static_cast<SNLBitTerm*>(rdata->getBit(1)),
                static_cast<SNLBitTerm*>(rdata->getBit(2)),
                static_cast<SNLBitTerm*>(rdata->getBit(3))}});
  interface.writePorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(w0addr->getBit(0)),
                   static_cast<SNLBitTerm*>(w0addr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(w0data->getBit(0)),
                static_cast<SNLBitTerm*>(w0data->getBit(1)),
                static_cast<SNLBitTerm*>(w0data->getBit(2)),
                static_cast<SNLBitTerm*>(w0data->getBit(3))},
       .mask = {static_cast<SNLBitTerm*>(w0mask->getBit(0)),
                static_cast<SNLBitTerm*>(w0mask->getBit(1)),
                static_cast<SNLBitTerm*>(w0mask->getBit(2)),
                static_cast<SNLBitTerm*>(w0mask->getBit(3))},
       .enables = {we0}});
  interface.writePorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(w1addr->getBit(0)),
                   static_cast<SNLBitTerm*>(w1addr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(w1data->getBit(0)),
                static_cast<SNLBitTerm*>(w1data->getBit(1)),
                static_cast<SNLBitTerm*>(w1data->getBit(2)),
                static_cast<SNLBitTerm*>(w1data->getBit(3))},
       .mask = {static_cast<SNLBitTerm*>(w1mask->getBit(0)),
                static_cast<SNLBitTerm*>(w1mask->getBit(1)),
                static_cast<SNLBitTerm*>(w1mask->getBit(2)),
                static_cast<SNLBitTerm*>(w1mask->getBit(3))},
       .enables = {we1},
       .extraWriteInputs = {{w1extra}}});
  SNLDesignModeling::setMemoryInterface(mem, interface);

  auto inst = SNLInstance::create(top, mem, NLName("mem0"));
  const auto disconnectedInterface = SNLDesignModeling::getMemoryInterface(inst);
  EXPECT_EQ(nullptr, disconnectedInterface.clock);
  EXPECT_EQ(nullptr, disconnectedInterface.reset);
  EXPECT_TRUE(disconnectedInterface.readPorts.empty());
  EXPECT_TRUE(disconnectedInterface.writePorts.empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(inst->getInstTerm(clk)).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(inst->getInstTerm(clk)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(we0)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(
      inst->getInstTerm(rdata->getBit(0))).empty());

  auto* clkNet = SNLScalarNet::create(top, NLName("clk"));
  inst->getInstTerm(clk)->setNet(clkNet);
  for (int bit = 0; bit <= 1; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("raddr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(raddr->getBit(bit)))->setNet(net);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("rdata_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(rdata->getBit(bit)))->setNet(net);
  }

  for (int bit = 0; bit <= 1; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("w0addr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(w0addr->getBit(bit)))->setNet(net);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* dataNet =
        SNLScalarNet::create(top, NLName("w0data_" + std::to_string(bit)));
    auto* maskNet =
        SNLScalarNet::create(top, NLName("w0mask_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(w0data->getBit(bit)))->setNet(dataNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(w0mask->getBit(bit)))->setNet(maskNet);
  }
  auto* we0Net = SNLScalarNet::create(top, NLName("we0"));
  inst->getInstTerm(we0)->setNet(we0Net);
  for (int bit = 0; bit <= 1; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("w1addr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(w1addr->getBit(bit)))->setNet(net);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* dataNet =
        SNLScalarNet::create(top, NLName("w1data_" + std::to_string(bit)));
    auto* maskNet =
        SNLScalarNet::create(top, NLName("w1mask_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(w1data->getBit(bit)))->setNet(dataNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(w1mask->getBit(bit)))->setNet(maskNet);
  }
  auto* we1Net = SNLScalarNet::create(top, NLName("we1"));
  inst->getInstTerm(we1)->setNet(we1Net);

  // Guard a real SEC regression: instance-level memory consumers should only
  // see connected ports, otherwise disconnected formal write ports become
  // fake no-driver dependencies downstream.
  const auto instInterface = SNLDesignModeling::getMemoryInterface(inst);
  ASSERT_EQ(1u, instInterface.readPorts.size());
  ASSERT_EQ(1u, instInterface.writePorts.size());
  EXPECT_EQ(1u, instInterface.writePorts.front().enables.size());
  EXPECT_EQ(we0, instInterface.writePorts.front().enables.front());
  EXPECT_THAT(
      std::vector(instInterface.writePorts.front().address.begin(),
                  instInterface.writePorts.front().address.end()),
      ElementsAre(static_cast<SNLBitTerm*>(w0addr->getBit(0)),
                  static_cast<SNLBitTerm*>(w0addr->getBit(1))));
}

TEST_F(SNLDesignModelingTest0, testMemoryInterfaceValidationErrors) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM1P"));
  auto other = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("OTHER"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto addr = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("ADDR"));
  auto wdata = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WDATA"));
  auto we = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE"));
  auto rdata = SNLScalarTerm::create(mem, SNLTerm::Direction::Output, NLName("RDATA"));
  auto otherTerm = SNLScalarTerm::create(other, SNLTerm::Direction::Input, NLName("OTHER"));

  auto makeInterface = [&]() {
    SNLDesignModeling::MemoryInterface interface;
    interface.width = 1;
    interface.depth = 2;
    interface.abits = 1;
    interface.clock = clk;
    interface.readPorts.push_back({.address = {addr}, .data = {rdata}});
    interface.writePorts.push_back(
        {.address = {addr}, .data = {wdata}, .enables = {we}});
    return interface;
  };

  EXPECT_THROW(
      SNLDesignModeling::setMemoryInterface(top, makeInterface()),
      NLException);
  EXPECT_THROW(
      SNLDesignModeling::setMemoryInterface(
          mem, SNLDesignModeling::MemoryInterface{}),
      NLException);

  auto invalidClock = makeInterface();
  invalidClock.clock = otherTerm;
  EXPECT_THROW(SNLDesignModeling::setMemoryInterface(mem, invalidClock), NLException);

  auto invalidWriteClock = makeInterface();
  invalidWriteClock.writePorts.front().clock = otherTerm;
  EXPECT_THROW(
      SNLDesignModeling::setMemoryInterface(mem, invalidWriteClock),
      NLException);
  EXPECT_FALSE(SNLDesignModeling::hasMemoryInterface(mem));

  auto invalidReset = makeInterface();
  invalidReset.reset = otherTerm;
  EXPECT_THROW(SNLDesignModeling::setMemoryInterface(mem, invalidReset), NLException);

  auto invalidReadAddress = makeInterface();
  invalidReadAddress.readPorts.front().address = {otherTerm};
  EXPECT_THROW(
      SNLDesignModeling::setMemoryInterface(mem, invalidReadAddress),
      NLException);

  auto invalidExtraWrite = makeInterface();
  invalidExtraWrite.writePorts.front().extraWriteInputs.push_back({otherTerm});
  EXPECT_THROW(
      SNLDesignModeling::setMemoryInterface(mem, invalidExtraWrite),
      NLException);

  EXPECT_THROW(
      SNLDesignModeling::getMemoryInterface(static_cast<const SNLDesign*>(nullptr)),
      NLException);
  EXPECT_FALSE(SNLDesignModeling::hasMemoryInterface(static_cast<const SNLDesign*>(nullptr)));
  EXPECT_THROW(SNLDesignModeling::getMemoryInterface(top), NLException);
  EXPECT_THROW(SNLDesignModeling::getMemoryInterface(static_cast<SNLInstance*>(nullptr)), NLException);
}

TEST_F(SNLDesignModelingTest0, testCombiWithParameter) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto gate = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("LUT"));
  auto mode = SNLParameter::create(gate, NLName("MODE"), SNLParameter::Type::String, "NORMAL");
  auto luti0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("I0"));
  auto luti1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("I1"));
  auto luto0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, NLName("O0"));
  auto luto1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::setParameter(gate, "MODE", "NORMAL");
  //no parameter arg so default mode
  SNLDesignModeling::addCombinatorialArcs({luti0}, {luto0});
  SNLDesignModeling::addCombinatorialArcs({luti1}, {luto1});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti1}, {luto0});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti0}, {luto1});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(gate));

  //Default parameter
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti1).size());
  EXPECT_EQ(luto0, *SNLDesignModeling::getCombinatorialOutputs(luti0).begin());
  EXPECT_EQ(luto1, *SNLDesignModeling::getCombinatorialOutputs(luti1).begin());

  //instance with no parameter => default parameter
  auto ins0 = SNLInstance::create(top, gate, NLName("ins0")); 
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).size());
  EXPECT_EQ(ins0->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins0->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).begin());

  //instance with default parameter value
  auto ins1 = SNLInstance::create(top, gate, NLName("ins1")); 
  SNLInstParameter::create(ins1, mode, "NORMAL");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).size());
  EXPECT_EQ(ins1->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins1->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).begin());

  //instance with non default parameter value
  auto ins2 = SNLInstance::create(top, gate, NLName("ins2")); 
  SNLInstParameter::create(ins2, mode, "CROSS");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).size());
  EXPECT_EQ(ins2->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins2->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).begin());

  //Error for non existing parameter
  auto ins3 = SNLInstance::create(top, gate, NLName("ins3")); 
  SNLInstParameter::create(ins3, mode, "UNKNOWN");
  EXPECT_THROW(SNLDesignModeling::getCombinatorialOutputs(ins3->getInstTerm(luti0)), NLException);
}

TEST_F(SNLDesignModelingTest0, testErrors0) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto designs = NLLibrary::create(db);
  //not a primitive
  auto design = SNLDesign::create(designs, NLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("C"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({designD}, designC), NLException);
}

TEST_F(SNLDesignModelingTest0, testErrors1) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("C"));
  auto designA = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("A"));
  auto designB = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("B"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({}, designC), NLException);
  EXPECT_THROW(SNLDesignModeling::addClockToOutputsArcs(designC, {}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({}, {designB}), NLException);

  auto design1 = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design1"));
  auto design1A = SNLScalarTerm::create(design1, SNLTerm::Direction::Input, NLName("A"));
  auto design1B = SNLScalarTerm::create(design1, SNLTerm::Direction::Output, NLName("B"));
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {design1B}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA, design1A}, {design1B}), NLException);
}

TEST_F(SNLDesignModelingTest0, testNonExistingParameterError) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto prim = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("prim"));
  EXPECT_THROW(SNLDesignModeling::setParameter(prim, "MODE", "NORMAL"), NLException);
}

TEST_F(SNLDesignModelingTest0, testGetCombiDepsFromTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  //set truth table
  std::vector<u_int64_t> deps;
  deps.push_back(3);
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, deps));
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1, deps)), NLException);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_EQ(inputArcs.size(), 2);
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_EQ(outputArcs.size(), 1);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  auto insInputArcs = SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o));
  EXPECT_EQ(insInputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insInputArcs.begin(), insInputArcs.end()),
    ElementsAre(ins0->getInstTerm(i0), ins0->getInstTerm(i1)));
  auto insOutputArcs = SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0));
  EXPECT_EQ(insOutputArcs.size(), 1);
  EXPECT_THAT(
    std::vector(insOutputArcs.begin(), insOutputArcs.end()),
    ElementsAre(ins0->getInstTerm(o)));
}

TEST_F(SNLDesignModelingTest0, testGetCombiDepsFromTTs) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  auto o1 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O1"));
  //set truth table
  std::vector<u_int64_t> deps;
  deps.push_back(3);
  SNLDesignModeling::setTruthTables(design, { SNLTruthTable(2, 0x5, deps) , SNLTruthTable(2, 0x5, deps)});
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1, deps)), NLException);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_EQ(inputArcs.size(), 2);
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_EQ(outputArcs.size(), 2);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  auto insInputArcs = SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o));
  EXPECT_EQ(insInputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insInputArcs.begin(), insInputArcs.end()),
    ElementsAre(ins0->getInstTerm(i0), ins0->getInstTerm(i1)));
  auto insOutputArcs = SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0));
  EXPECT_EQ(insOutputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insOutputArcs.begin(), insOutputArcs.end()),
    ElementsAre(ins0->getInstTerm(o), ins0->getInstTerm(o1)));
}

TEST_F(SNLDesignModelingTest0, testNoDepsFromTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  auto o1 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O1"));
  // create a top
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  // create an instance of the design
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  std::vector<u_int64_t> deps;
  EXPECT_THROW(
      SNLDesignModeling::setTruthTables(
          design, {SNLTruthTable(2, 0x5, deps), SNLTruthTable(2, 0x5, deps)}),
      NLException);
  auto property = naja::NajaDumpableProperty::create(
      static_cast<naja::NajaObject*>(design), "SNLDesignTruthTableProperty");
  property->addUInt64Value(2);
  property->addUInt64Value(0x5);
  property->addUInt64Value(0);
  property->addUInt64Value(2);
  property->addUInt64Value(0x5);
  property->addUInt64Value(0);
  // Test all inputs are returned for combinatorial quarie for each output
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o1).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o1)).size(), 2);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 2);
}

TEST_F(SNLDesignModelingTest0, testNoDepsFromSingleTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  // create a top
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  // create an instance of the design
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  //set truth table
  std::vector<u_int64_t> deps;
  deps.push_back(1);
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, deps));
  // Test all inputs are returned for combinatorial quarie for each output
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 1);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 0);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 0);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_THAT(
    std::vector(inputArcs.begin(), inputArcs.end()),
    ElementsAre(i0));
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_THAT(
    std::vector(outputArcs.begin(), outputArcs.end()),
    ElementsAre(o));
  auto ouputArcsI1 = SNLDesignModeling::getCombinatorialOutputs(i1);
  EXPECT_TRUE(ouputArcsI1.empty());
}

TEST_F(SNLDesignModelingTest0, testNoDepsFromSingleTTWithEmptyDeps) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  // create a top
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  // create an instance of the design
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  //set truth table
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 2);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 1);
  std::vector<u_int64_t> deps;
  EXPECT_THROW(
      SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, deps)),
      NLException);
  auto property = naja::NajaDumpableProperty::create(
      static_cast<naja::NajaObject*>(design), "SNLDesignTruthTableProperty");
  property->addUInt64Value(2);
  property->addUInt64Value(0x5);
  property->addUInt64Value(0);
  // Test all inputs are returned for combinatorial quarie for each output
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 2);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 1);
}

TEST_F(SNLDesignModelingTest0, testSequentialModelAPI) {
  using Expression = SNLDesignModeling::BooleanExpression;
  using Model = SNLDesignModeling::SequentialModel;

  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* designs = NLLibrary::create(db);
  auto* top = SNLDesign::create(designs, NLName("top"));
  auto* primitive = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("FF"));
  auto* clock = SNLScalarTerm::create(
      primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto* output = SNLScalarTerm::create(
      primitive, SNLTerm::Direction::Output, NLName("Q"));

  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(nullptr));
  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(primitive));
  EXPECT_FALSE(SNLDesignModeling::isSequential(primitive));
  EXPECT_THROW(
      SNLDesignModeling::getSequentialModel(nullptr), NLException);
  EXPECT_THROW(
      SNLDesignModeling::getSequentialModel(primitive), NLException);

  Model invalidModel;
  EXPECT_THROW(
      SNLDesignModeling::setSequentialModel(primitive, invalidModel),
      NLException);

  Model model;
  model.clockedOn.root = model.clockedOn.addTerm(clock);
  SNLDesignModeling::SequentialState state;
  const auto constant0 = state.nextState.addConstant(false);
  const auto constant1 = state.nextState.addConstant(true);
  state.nextState.root = state.nextState.addOperation(
      Expression::Operator::Or, {constant0, constant1});
  model.states.push_back(std::move(state));
  SNLDesignModeling::SequentialOutput sequentialOutput;
  sequentialOutput.term = output;
  sequentialOutput.function.root = sequentialOutput.function.addState(0);
  model.outputs.push_back(std::move(sequentialOutput));
  ASSERT_TRUE(model.isValid());

  EXPECT_THROW(
      SNLDesignModeling::setSequentialModel(nullptr, model), NLException);
  EXPECT_THROW(
      SNLDesignModeling::setSequentialModel(top, model), NLException);
  SNLDesignModeling::setSequentialModel(primitive, model);

  EXPECT_TRUE(SNLDesignModeling::hasSequentialModel(primitive));
  EXPECT_TRUE(SNLDesignModeling::isSequential(primitive));
  const auto& stored = SNLDesignModeling::getSequentialModel(primitive);
  ASSERT_EQ(3u, stored.states[0].nextState.nodes.size());
  EXPECT_FALSE(stored.states[0].nextState.nodes[constant0].constant);
  EXPECT_TRUE(stored.states[0].nextState.nodes[constant1].constant);
  EXPECT_EQ(Expression::Operator::State,
            stored.outputs[0].function.nodes[0].operation);
}

TEST_F(SNLDesignModelingTest0, testParameterizedSequentialModel) {
  using Modeling = SNLDesignModeling;
  using Expression = Modeling::BooleanExpression;
  using Op = Expression::Operator;
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* designs = NLLibrary::create(db);
  auto* top = SNLDesign::create(designs, NLName("top"));
  auto* primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("PARAM_DFF"));
  auto input = [primitive](const char* name) {
    return SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName(name));
  };
  auto* data = input("I");
  auto* clock = input("CK");
  auto* load = input("L");
  auto* reset = input("R");
  auto* output = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("O"));
  std::vector<std::string> names {"FALLING_EDGE", "USE_RESET", "USE_ENABLE", "SYNC_RESET", "RESET_VALUE"};
  std::vector<SNLParameter*> parameters;
  for (const auto& name : names) {
    parameters.push_back(SNLParameter::create(primitive, NLName(name), SNLParameter::Type::Binary, "0"));
  }
  Modeling::addClockToOutputsArcs(clock, {output});
  Modeling::addInputsToClockArcs({data, load, reset}, clock);
  Modeling::SequentialModelTable models;
  // Cover all 32 configurations, including falling edges.
  for (uint64_t bits = 0; bits < 32; ++bits) {
    std::vector<uint64_t> values;
    for (size_t i = 0; i < 5; ++i) values.push_back((bits >> i) & 1);
    Modeling::SequentialModel model;
    auto clk = model.clockedOn.addTerm(clock);
    model.clockedOn.root = values[0] ? model.clockedOn.addOperation(Op::Not, {clk}) : clk;
    Modeling::SequentialState state;
    auto& next = state.nextState;
    auto d = next.addTerm(data);
    next.root = d;
    if (values[2]) {
      auto l = next.addTerm(load);
      auto q = next.addState(0);
      auto nl = next.addOperation(Op::Not, {l});
      auto ld = next.addOperation(Op::And, {l, d});
      auto hold = next.addOperation(Op::And, {nl, q});
      next.root = next.addOperation(Op::Or, {ld, hold});
    }
    if (values[1]) {
      if (values[3]) {
        auto r = next.addTerm(reset);
        auto nr = next.addOperation(Op::Not, {r});
        auto update = next.addOperation(Op::And, {nr, next.root});
        next.root = values[4] ? next.addOperation(Op::Or, {r, update}) : update;
      } else {
        Expression control;
        control.root = control.addTerm(reset);
        if (values[4]) state.preset = control;
        else state.clear = control;
      }
    }
    model.states.push_back(state);
    Expression q;
    q.root = q.addState(0);
    model.outputs.push_back({output, q});
    models.emplace(values, model);
  }
  Modeling::setSequentialModelFromParameters(primitive, names, models);
  EXPECT_TRUE(Modeling::hasSequentialModel(primitive));
  EXPECT_TRUE(Modeling::hasSequentialModelFromParameters(primitive));
  EXPECT_TRUE(Modeling::isSequential(primitive));
  EXPECT_THROW(Modeling::getSequentialModel(primitive), NLException);
  EXPECT_THROW(Modeling::getSequentialModel(static_cast<const SNLInstance*>(nullptr)), NLException);
  auto* instance = SNLInstance::create(top, primitive, NLName("default"));
  auto* defaultModel = &Modeling::getSequentialModel(instance);
  EXPECT_EQ(defaultModel, &Modeling::getSequentialModel(instance));
  EXPECT_FALSE(defaultModel->states[0].clear);
  EXPECT_EQ(Op::Term, defaultModel->states[0].nextState.nodes[0].operation);
  std::vector<SNLInstParameter*> overrides;
  for (auto* parameter : parameters) overrides.push_back(SNLInstParameter::create(instance, parameter, "0"));
  for (const auto& [values, ignored] : models) {
    for (size_t i = 0; i < values.size(); ++i) overrides[i]->setValue(values[i] ? "1'b1" : "1'b0");
    const auto& model = Modeling::getSequentialModel(instance);
    EXPECT_EQ(&model, &Modeling::getSequentialModel(instance));
    EXPECT_EQ(values[0] ? Op::Not : Op::Term, model.clockedOn.nodes[model.clockedOn.root].operation);
    EXPECT_EQ(bool(values[1] && !values[3] && !values[4]), model.states[0].clear.has_value());
    EXPECT_EQ(bool(values[1] && !values[3] && values[4]), model.states[0].preset.has_value());
    for (unsigned assignment = 0; assignment < 16; ++assignment) {
      bool d = assignment & 1, l = assignment & 2, r = assignment & 4, q = assignment & 8;
      auto evaluate = [&](const auto& self, const Expression& expression, size_t id) -> bool {
        const auto& node = expression.nodes[id];
        switch (node.operation) {
          case Op::Constant: return node.constant;
          case Op::Term: return node.term == data ? d : node.term == load ? l : r;
          case Op::State: return q;
          case Op::Not: return !self(self, expression, node.operands[0]);
          case Op::And: return self(self, expression, node.operands[0]) && self(self, expression, node.operands[1]);
          case Op::Or: return self(self, expression, node.operands[0]) || self(self, expression, node.operands[1]);
          case Op::Xor: return self(self, expression, node.operands[0]) != self(self, expression, node.operands[1]);
        }
        return false;
      };
      bool expected = values[2] && !l ? q : d;
      if (values[1] && values[3] && r) expected = values[4];
      const auto& next = model.states[0].nextState;
      EXPECT_EQ(expected, evaluate(evaluate, next, next.root));
      if (model.states[0].clear) EXPECT_EQ(r, evaluate(evaluate, *model.states[0].clear, model.states[0].clear->root));
      if (model.states[0].preset) EXPECT_EQ(r, evaluate(evaluate, *model.states[0].preset, model.states[0].preset->root));
    }
  }
  // Timing arcs stay conservative even when L or R is unused.
  EXPECT_EQ(3, Modeling::getClockRelatedInputs(instance->getInstTerm(clock)).size());
  EXPECT_EQ(1, Modeling::getOutputRelatedClocks(instance->getInstTerm(output)).size());
  auto* cloned = top->clone(NLName("clone"));
  EXPECT_EQ(&Modeling::getSequentialModel(instance), &Modeling::getSequentialModel(cloned->getInstance(NLName("default"))));
  for (auto* override : overrides) override->destroy();
  EXPECT_EQ(defaultModel, &Modeling::getSequentialModel(instance));
  auto* required = SNLParameter::create(primitive, NLName("required"), SNLParameter::Type::Boolean);
  Modeling::setSequentialModelFromParameters(primitive, {"required"}, {{{0}, models.begin()->second}});
  EXPECT_THROW(Modeling::getSequentialModel(instance), NLException);
  auto* override = SNLInstParameter::create(instance, required, "0");
  EXPECT_NO_THROW(Modeling::getSequentialModel(instance));
  override->setValue("1");
  EXPECT_THROW(Modeling::getSequentialModel(instance), NLException);
  override->setValue("1'bx");
  EXPECT_THROW(Modeling::getSequentialModel(instance), NLException);
  override->destroy();
  EXPECT_THROW(Modeling::getSequentialModel(instance), NLException);
  EXPECT_THROW(Modeling::setSequentialModelFromParameters(primitive, {"absent"}, models), NLException);
  EXPECT_THROW(Modeling::setSequentialModelFromParameters(primitive, {"required", "required"}, models), NLException);
  EXPECT_THROW(Modeling::setSequentialModelFromParameters(primitive, {"required"}, {}), NLException);
  auto invalid = models.begin()->second;
  auto* foreign = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("foreign"));
  invalid.clockedOn.nodes[0].term = foreign;
  EXPECT_THROW(Modeling::setSequentialModelFromParameters(primitive, {"required"}, {{{0}, invalid}}), NLException);
  auto checkDiagnostic = [&](const Modeling::SequentialModel& model,
      const std::vector<std::string>& details) {
    try {
      Modeling::setSequentialModelFromParameters(primitive, {"required"}, {{{0}, model}});
      FAIL() << "Invalid model declaration unexpectedly accepted";
    } catch (const NLException& error) {
      for (const auto& detail : details) {
        EXPECT_THAT(error.getReason(), ::testing::HasSubstr(detail));
      }
    }
  };
  checkDiagnostic(invalid, {"PARAM_DFF", "required=0", "clocked_on", "nodes[0]", "foreign", "belongs to"});
  invalid = models.begin()->second;
  invalid.states[0].nextState.root = Expression::InvalidNode;
  checkDiagnostic(invalid, {"states[0].next_state", "root node index", "node array of size 1"});
  invalid = models.begin()->second;
  invalid.states[0].nextState.nodes[0].operation = Op::State;
  invalid.states[0].nextState.nodes[0].state = 7;
  checkDiagnostic(invalid, {"states[0].next_state", "nodes[0]", "state index 7", "states array of size 1"});
  invalid = models.begin()->second;
  invalid.clockedOn.nodes[0].operation = Op::Not;
  invalid.clockedOn.nodes[0].operands = {0};
  checkDiagnostic(invalid, {"clocked_on", "nodes[0]", "operands[0]=0", "index < 0", "cyclic/forward"});
  invalid = models.begin()->second;
  invalid.kind = static_cast<Modeling::SequentialModel::Kind>(99);
  checkDiagnostic(invalid, {"required=0", "invalid kind enum value 99", "FlipFlop or Latch"});
  invalid = models.begin()->second;
  invalid.clockedOn.nodes[0].operation = static_cast<Op>(99);
  checkDiagnostic(invalid, {"clocked_on", "nodes[0]", "invalid operator enum value 99"});
  for (auto operation : {Op::Term, Op::Not, Op::And}) {
    invalid = models.begin()->second;
    invalid.clockedOn.nodes[0].operation = operation;
    invalid.clockedOn.nodes[0].operands = operation == Op::Term ? std::vector<size_t>{0} : std::vector<size_t>{};
    checkDiagnostic(invalid, {"clocked_on", "nodes[0]", "operands; expected"});
  }
  invalid = models.begin()->second;
  invalid.states[0].clearPresetValue = static_cast<Modeling::SequentialState::ClearPresetValue>(99);
  checkDiagnostic(invalid, {"states[0].clear_preset_value", "invalid enum value 99", "Zero, One, Hold, Toggle, or Unknown"});
  invalid = models.begin()->second;
  invalid.outputs[0].term = foreign;
  checkDiagnostic(invalid, {"outputs[0]", "foreign", "belongs to", "destination primitive"});
  invalid.outputs[0].term = data;
  checkDiagnostic(invalid, {"outputs[0]", "expected direction Output"});
  auto* stringSelector = SNLParameter::create(primitive, NLName("string_selector"), SNLParameter::Type::String, "zero");
  try {
    Modeling::setSequentialModelFromParameters(primitive, {stringSelector->getName().getString()}, {{{0}, models.begin()->second}});
    FAIL() << "String selector unexpectedly accepted";
  } catch (const NLException& error) {
    EXPECT_THAT(error.getReason(), ::testing::HasSubstr("parameters[0] <string_selector>"));
    EXPECT_THAT(error.getReason(), ::testing::HasSubstr("type String is unsupported; expected Binary, Boolean, or Decimal"));
  }
  // Failed declarations preserve the previously installed table.
  EXPECT_TRUE(Modeling::hasSequentialModelFromParameters(primitive));
  auto* restored = SNLInstParameter::create(instance, required, "0");
  EXPECT_EQ(Op::Term, Modeling::getSequentialModel(instance).states[0].nextState.nodes[0].operation);
  restored->destroy();
  Modeling::setSequentialModel(primitive, models.begin()->second);
  EXPECT_FALSE(Modeling::hasSequentialModelFromParameters(primitive));
  EXPECT_EQ(&Modeling::getSequentialModel(primitive), &Modeling::getSequentialModel(instance));
  auto* noModel = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("NO_MODEL"));
  auto* unmodeledInstance = SNLInstance::create(top, noModel, NLName("unmodeled"));
  try {
    Modeling::getSequentialModel(unmodeledInstance);
    FAIL() << "Instance without a sequential model unexpectedly accepted";
  } catch (const NLException& error) {
    EXPECT_THAT(error.getReason(), ::testing::HasSubstr("unmodeled"));
    EXPECT_THAT(error.getReason(), ::testing::HasSubstr("NO_MODEL"));
    EXPECT_THAT(error.getReason(), ::testing::HasSubstr("has no sequential model; attach a static model or parameter table first"));
  }
}

TEST_F(SNLDesignModelingTest0, testMultiOutputGatePredicates) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("multi"));
  SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I"));
  SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::setTruthTables(design, {SNLTruthTable(1, 0b10, {1}), SNLTruthTable(1, 0b01, {1})});
  auto parameterDesign = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("parameters"));
  auto parameterInput = SNLScalarTerm::create(parameterDesign, SNLTerm::Direction::Input, NLName("I"));
  auto parameterOutput0 = SNLScalarTerm::create(parameterDesign, SNLTerm::Direction::Output, NLName("O0"));
  auto parameterOutput1 = SNLScalarTerm::create(parameterDesign, SNLTerm::Direction::Output, NLName("O1"));
  auto init = SNLParameter::create(parameterDesign, NLName("INIT"), SNLParameter::Type::Binary, "2'b10");
  SNLDesignModeling::setTruthTableFromParameter(parameterDesign, parameterOutput0, {parameterInput}, init);
  SNLDesignModeling::setTruthTableFromParameter(parameterDesign, parameterOutput1, {parameterInput}, init);
  const std::vector<bool (*)(const SNLDesign*)> predicates = {
    SNLDesignModeling::isConst0, SNLDesignModeling::isConst1, SNLDesignModeling::isConst,
    SNLDesignModeling::isInv, SNLDesignModeling::isBuf, SNLDesignModeling::isAnd,
    SNLDesignModeling::isNand, SNLDesignModeling::isOr, SNLDesignModeling::isNor,
    SNLDesignModeling::isXor, SNLDesignModeling::isXnor, SNLDesignModeling::isMux,
    SNLDesignModeling::isSequential
  };
  for (auto model : {NLDB0::getFA(), NLDB0::getOrCreateTableSelect({2, 2, 1}), design, parameterDesign}) {
    EXPECT_THROW(SNLDesignModeling::getTruthTable(model), NLException);
    for (auto predicate : predicates) {
      EXPECT_NO_THROW(EXPECT_FALSE(predicate(model)));
    }
  }
}

TEST_F(SNLDesignModelingTest0, testParameterizedTermRoles) {
  using Modeling = SNLDesignModeling;
  using Role = Modeling::SNLTermRole;
  using Level = Modeling::SNLActiveLevel;
  auto* universe = NLUniverse::create();
  auto* db = NLDB::create(universe);
  auto* prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* designs = NLLibrary::create(db);
  auto* top = SNLDesign::create(designs, NLName("top"));
  auto* model = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("TEST_DFF"));
  auto* reset = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("R"));
  auto* load = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("L"));
  auto* clock = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("CK"));
  auto* mode = SNLParameter::create(model, NLName("mode"), SNLParameter::Type::Decimal, "0");
  Modeling::setTermRole(reset, Role::AsyncReset, Level::Low);
  Modeling::setTermRole(load, Role::Enable);
  Modeling::setTermRole(clock, Role::Clock);
  Modeling::TermRoleTable table {
    {{0}, {{reset, {}}, {load, {}}}},
    {{1}, {{reset, {Role::AsyncReset, Level::High}}, {load, {Role::Enable, Level::NA}}}},
    {{2}, {{reset, {Role::SyncReset, Level::Low}}, {load, {}}}},
    {{3}, {{reset, {Role::AsyncSet, Level::High}}, {load, {}}}},
    {{4}, {{reset, {Role::SyncSet, Level::Low}}, {load, {}}}}
  };
  auto* instance = SNLInstance::create(top, model, NLName("ff"));
  auto* r = instance->getInstTerm(reset);
  EXPECT_EQ(Role::AsyncReset, Modeling::getTermRole(r));
  EXPECT_FALSE(Modeling::hasRolesFromParameters(model));
  Modeling::setRolesFromParameters(model, {"mode"}, table);
  EXPECT_TRUE(Modeling::hasRolesFromParameters(model));
  EXPECT_EQ(Role::Other, Modeling::getTermRole(r));
  EXPECT_FALSE(Modeling::isEnable(instance->getInstTerm(load)));
  auto* override = SNLInstParameter::create(instance, mode, "0");
  for (const auto& [key, roles] : table) {
    override->setValue(std::to_string(key[0]));
    EXPECT_EQ(roles.at(reset).role, Modeling::getTermRole(r));
    EXPECT_EQ(roles.at(reset).activeLevel, Modeling::getResetActiveLevel(r));
    EXPECT_TRUE(Modeling::isClock(instance->getInstTerm(clock)));
  }
  override->setValue("5");
  EXPECT_THROW(Modeling::getTermRole(r), NLException);
  override->destroy();
  EXPECT_EQ(Role::Other, Modeling::getTermRole(r));
  EXPECT_EQ(Role::AsyncReset, Modeling::getTermRole(reset));
  EXPECT_THROW(Modeling::setRolesFromParameters(nullptr, {"mode"}, table), NLException);
  EXPECT_THROW(Modeling::setRolesFromParameters(top, {"mode"}, table), NLException);
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {}, table), NLException);
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"mode"}, {}), NLException);
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"absent"}, table), NLException);
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"mode", "mode"}, table), NLException);
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"mode", "extra"}, table), NLException);
  auto invalid = table;
  invalid.begin()->second[reset] = {static_cast<Role>(999), Level::NA};
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"mode"}, invalid), NLException);
  invalid.begin()->second[reset] = {Role::Other, static_cast<Level>(999)};
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"mode"}, invalid), NLException);
  invalid.clear();
  invalid[{0}][nullptr] = {};
  EXPECT_THROW(Modeling::setRolesFromParameters(model, {"mode"}, invalid), NLException);
  EXPECT_EQ(Role::Other, Modeling::getTermRole(r));
  EXPECT_FALSE(Modeling::hasRolesFromParameters(nullptr));
}
