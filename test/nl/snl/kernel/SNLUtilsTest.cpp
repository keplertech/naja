// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesignModeling.h"
#include "SNLUtils.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"
#include "SNLRTLInfos.h"
using namespace naja::NL;

class SNLUtilsTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB*  db_;
};

TEST_F(SNLUtilsTest, testSimpleHierarchy) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  
  auto top = SNLDesign::create(library, NLName("top"));
  auto level0 = SNLDesign::create(library, NLName("level0"));
  auto level2 = SNLDesign::create(library, NLName("level2"));
  auto level1 = SNLDesign::create(library, NLName("level1"));
  auto level0Ins = SNLInstance::create(level1, level0, NLName("level0"));
  auto level1Ins = SNLInstance::create(level2, level1, NLName("level1"));
  auto level2Ins = SNLInstance::create(top, level2, NLName("level2"));

  SNLUtils::SortedDesigns sortedDesigns;
  SNLUtils::getDesignsSortedByHierarchicalLevel(top, sortedDesigns);
  EXPECT_THAT(sortedDesigns, ElementsAre(
    SNLUtils::DesignLevel(level0, 0),
    SNLUtils::DesignLevel(level1, 1),
    SNLUtils::DesignLevel(level2, 2),
    SNLUtils::DesignLevel(top, 3)
  ));
}

TEST_F(SNLUtilsTest, testDoubleHierarchy) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  
  auto top = SNLDesign::create(library, NLName("top"));
  auto level21 = SNLDesign::create(library, NLName("level20"));
  auto level0 = SNLDesign::create(library, NLName("level0"));
  auto level20 = SNLDesign::create(library, NLName("level21"));
  auto level1 = SNLDesign::create(library, NLName("level1"));
  auto level20_1Ins = SNLInstance::create(level20, level1, NLName("level1"));
  auto level21_1Ins = SNLInstance::create(level21, level1, NLName("level1"));
  auto level0Ins = SNLInstance::create(level1, level0, NLName("level0"));
  auto level20Ins = SNLInstance::create(top, level20, NLName("level20"));
  auto level21Ins = SNLInstance::create(top, level21, NLName("level21"));

  SNLUtils::SortedDesigns sortedDesigns;
  SNLUtils::getDesignsSortedByHierarchicalLevel(top, sortedDesigns);
  EXPECT_THAT(sortedDesigns, ElementsAre(
    SNLUtils::DesignLevel(level0, 0),
    SNLUtils::DesignLevel(level1, 1),
    SNLUtils::DesignLevel(level21, 2),
    SNLUtils::DesignLevel(level20, 2), //sorted by SNLID
    SNLUtils::DesignLevel(top, 3)
  ));
}

TEST_F(SNLUtilsTest, testReachableInstanceCountNullTop) {
  auto count = SNLUtils::countReachableInstances(nullptr);
  EXPECT_EQ(0u, count.totalInstances);
  EXPECT_EQ(0u, count.leafInstances);
  EXPECT_EQ(0u, count.foldedTotalInstances);
  EXPECT_EQ(0u, count.foldedLeafInstances);
  EXPECT_EQ(0u, count.reachableModels);
}

TEST_F(SNLUtilsTest, testReachableInstanceCountSharedModel) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto primitives = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("PRIMS"));

  auto primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("leaf"));
  auto top = SNLDesign::create(library, NLName("top"));
  auto child = SNLDesign::create(library, NLName("child"));
  SNLInstance::create(child, primitive, NLName("leaf"));
  SNLInstance::create(top, child, NLName("child0"));
  SNLInstance::create(top, child, NLName("child1"));

  auto count = SNLUtils::countReachableInstances(top);
  EXPECT_EQ(4u, count.totalInstances);
  EXPECT_EQ(2u, count.leafInstances);
  EXPECT_EQ(3u, count.foldedTotalInstances);
  EXPECT_EQ(1u, count.foldedLeafInstances);
  EXPECT_EQ(3u, count.reachableModels);
}

TEST_F(SNLUtilsTest, testPrepareForConcurrentAccess) {
  auto library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto primitives = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("PRIMS"));

  auto primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("buf"));
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("i"));
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("o"));
  SNLDesignModeling::setTruthTable(primitive, SNLTruthTable::Buf());

  auto top = SNLDesign::create(library, NLName("top"));
  auto child = SNLDesign::create(library, NLName("child"));
  SNLInstance::create(child, primitive, NLName("primitive"));
  SNLInstance::create(top, child, NLName("child"));

  EXPECT_NO_THROW(SNLUtils::prepareForConcurrentAccess(top));
}

TEST_F(SNLUtilsTest, testPrepareForConcurrentAccessTableSelect) {
  auto library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto top = SNLDesign::create(library, NLName("top"));
  auto tableSelect = NLDB0::getOrCreateTableSelect({2, 2, 1});
  ASSERT_NE(tableSelect, nullptr);
  SNLInstance::create(top, tableSelect, NLName("select"));

  EXPECT_NO_THROW(SNLUtils::prepareForConcurrentAccess(top));

  size_t outputCount = 0;
  for (auto* term: tableSelect->getBitTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Input) {
      continue;
    }
    const auto truthTable =
        SNLDesignModeling::getTruthTable(tableSelect, term->getOrderID());
    EXPECT_TRUE(truthTable.isInitialized());
    EXPECT_EQ(SNLTruthTable::GenericType::TABLE_SELECT,
              truthTable.getGenericType());
    EXPECT_EQ(3u, truthTable.size());
    EXPECT_EQ(1u, truthTable.getTableSelectAddressSize());
    EXPECT_EQ(2u, truthTable.getTableSelectDepth());
    ++outputCount;
  }
  EXPECT_EQ(2u, outputCount);
}

TEST_F(SNLUtilsTest, testPrepareForConcurrentAccessMemory) {
  NLDB0::MemorySignature signature;
  signature.width = 8;
  signature.depth = 16;
  signature.abits = 4;
  signature.readPorts = 1;
  signature.writePorts = 1;
  auto* memory = NLDB0::getOrCreateMemory(signature);
  ASSERT_NE(memory, nullptr);

  EXPECT_NO_THROW(SNLUtils::prepareForConcurrentAccess(memory));
}

TEST_F(SNLUtilsTest, removeUnconsumedMuxConesPreservesExternalAndStateUses) {
  auto* library = db_->getLibrary(NLName("MYLIB"));
  auto* top = SNLDesign::create(library, NLName("top"));
  auto* a = SNLBusNet::create(top, 1, 0, NLName("a"));
  auto* b = SNLBusNet::create(top, 1, 0, NLName("b"));
  auto* select = SNLScalarNet::create(top, NLName("select"));
  auto* model = NLDB0::getOrCreateMux2(2);
  const auto makeMux = [&](const char* name, SNLNet* input, SNLNet* output,
                           SNLBitNet* selector) {
    auto* instance = SNLInstance::create(top, model, NLName(name));
    instance->setTermNet(NLDB0::getMux2InputA(model), input);
    instance->setTermNet(NLDB0::getMux2InputB(model), b);
    instance->setTermNet(NLDB0::getMux2Select(model), selector);
    instance->setTermNet(NLDB0::getMux2Output(model), output);
    return instance;
  };
  auto* dead0 = SNLBusNet::create(top, 1, 0, NLName("dead0"));
  auto* dead1 = SNLBusNet::create(top, 1, 0, NLName("dead1"));
  auto* dead2 = SNLBusNet::create(top, 1, 0, NLName("dead2"));
  makeMux("dead0", a, dead0, select);
  makeMux("dead1", dead0, dead1, select);
  makeMux("dead2", a, dead2, dead0->getBit(0));
  // A disconnected pure combinational cycle is also unobservable.
  auto* cycle0 = SNLBusNet::create(top, 1, 0, NLName("cycle0"));
  auto* cycle1 = SNLBusNet::create(top, 1, 0, NLName("cycle1"));
  makeMux("cycle0", cycle1, cycle0, select);
  makeMux("cycle1", cycle0, cycle1, select);

  auto* selectValue = SNLBusNet::create(top, 1, 0, NLName("select_value"));
  auto* output = SNLBusNet::create(top, 1, 0, NLName("output"));
  auto* liveSelect = makeMux("live_select", a, selectValue, select);
  auto* liveOutput = makeMux("live_output", a, output, selectValue->getBit(1));
  // Any observed output bit keeps the whole mux and its select cone.
  SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("y"))->setNet(output->getBit(0));

  auto* stateData = SNLBusNet::create(top, 1, 0, NLName("state_data"));
  auto* liveState = makeMux("live_state", a, stateData, select);
  auto* flop = SNLInstance::create(top, NLDB0::getDFF(), NLName("flop"));
  flop->setTermNet(NLDB0::getDFFData(), stateData->getBit(0));
  auto* external = SNLBusNet::create(top, 1, 0, NLName("external"));
  auto* liveInout = makeMux("live_inout", a, external, select);
  auto* blackbox = SNLDesign::create(library, SNLDesign::Type::UserBlackBox, NLName("box"));
  auto* io = SNLScalarTerm::create(blackbox, SNLTerm::Direction::InOut, NLName("io"));
  SNLInstance::create(top, blackbox, NLName("box"))->setTermNet(io, external->getBit(1));
  auto* inputValue = SNLBusNet::create(top, 1, 0, NLName("input_value"));
  auto* liveInput = makeMux("live_input", a, inputValue, select);
  SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("i"))->setNet(inputValue->getBit(0));

  const auto nets = top->getNets().size();
  EXPECT_EQ(0u, SNLUtils::removeUnconsumedMuxes(nullptr));
  EXPECT_EQ(5u, SNLUtils::removeUnconsumedMuxes(top));
  EXPECT_EQ(nets, top->getNets().size());
  EXPECT_EQ(liveSelect, top->getInstance(NLName("live_select")));
  EXPECT_EQ(liveOutput, top->getInstance(NLName("live_output")));
  EXPECT_EQ(liveState, top->getInstance(NLName("live_state")));
  EXPECT_EQ(liveInout, top->getInstance(NLName("live_inout")));
  EXPECT_EQ(liveInput, top->getInstance(NLName("live_input")));
  EXPECT_EQ(stateData->getBit(0), flop->getInstTerm(NLDB0::getDFFData())->getNet());
  EXPECT_EQ(0u, SNLUtils::removeUnconsumedMuxes(top));
}

namespace {
struct RepeatedMuxFixture {
  SNLDesign* design;
  SNLDesign* model;
  SNLBusNet *a, *b, *temporary, *output;
  SNLScalarNet *s, *t;
  SNLInstance *inner, *outer;
};

RepeatedMuxFixture makeRepeatedMuxFixture(NLLibrary* library, bool reverseOrder = false) {
  RepeatedMuxFixture f {};
  f.design = SNLDesign::create(library);
  f.model = NLDB0::getOrCreateMux2(3);
  f.a = SNLBusNet::create(f.design, 2, 0, NLName("a"));
  f.b = SNLBusNet::create(f.design, 2, 0, NLName("b"));
  f.temporary = SNLBusNet::create(f.design, 2, 0, NLName("temporary"));
  f.output = SNLBusNet::create(f.design, 2, 0, NLName("output"));
  f.s = SNLScalarNet::create(f.design, NLName("s"));
  f.t = SNLScalarNet::create(f.design, NLName("t"));
  if (reverseOrder) f.outer = SNLInstance::create(f.design, f.model, NLName("outer"));
  f.inner = SNLInstance::create(f.design, f.model, NLName("inner"));
  if (!reverseOrder) f.outer = SNLInstance::create(f.design, f.model, NLName("outer"));
  for (auto* instance : {f.inner, f.outer}) {
    instance->setTermNet(NLDB0::getMux2InputA(f.model), instance == f.inner ? f.a : f.temporary);
    instance->setTermNet(NLDB0::getMux2InputB(f.model), f.b);
    instance->setTermNet(NLDB0::getMux2Select(f.model), instance == f.inner ? f.s : f.t);
    instance->setTermNet(NLDB0::getMux2Output(f.model), instance == f.inner ? f.temporary : f.output);
  }
  SNLBusTerm::create(f.design, SNLTerm::Direction::Output, 2, 0, NLName("y"))->setNet(f.output);
  return f;
}
}

TEST_F(SNLUtilsTest, mergeRepeatedMuxSelectionsPreservesPinsAndOrigins) {
  auto f = makeRepeatedMuxFixture(db_->getLibrary(NLName("MYLIB")));
  const SNLSourceReference first {{NLName("source.sv"), 11, 11, 2, 8},
    NLName("slang"), NLName("systemverilog"), NLName("rtl")};
  const SNLSourceReference second {{NLName("source.sv"), 23, 23, 2, 8},
    NLName("slang"), NLName("systemverilog"), NLName("rtl")};
  SNLRTLInfos::create(f.inner)->addSourceOrigin(first);
  SNLRTLInfos::create(f.outer)->addSourceOrigin(second);
  EXPECT_EQ(0u, SNLUtils::mergeRepeatedMuxSelections(nullptr));
  EXPECT_EQ(1u, SNLUtils::mergeRepeatedMuxSelections(f.design));
  EXPECT_EQ(nullptr, f.design->getInstance(NLName("inner")));
  EXPECT_EQ(f.outer, f.design->getInstance(NLName("outer")));
  for (NLID::Bit bit = 0; bit < 3; ++bit) {
    EXPECT_EQ(f.a->getBit(bit), f.outer->getInstTerm(NLDB0::getMux2InputA(f.model)->getBit(bit))->getNet());
    EXPECT_EQ(f.b->getBit(bit), f.outer->getInstTerm(NLDB0::getMux2InputB(f.model)->getBit(bit))->getNet());
    EXPECT_EQ(f.output->getBit(bit), f.outer->getInstTerm(NLDB0::getMux2Output(f.model)->getBit(bit))->getNet());
  }
  EXPECT_THAT(f.outer->getRTLInfos()->getSourceOrigins(), ElementsAre(second, first));
  auto* select = f.outer->getInstTerm(NLDB0::getMux2Select(f.model))->getNet();
  SNLInstance* gate = nullptr;
  for (auto* term : select->getInstTerms())
    if (term->getDirection() == SNLTerm::Direction::Output) gate = term->getInstance();
  ASSERT_NE(nullptr, gate);
  EXPECT_EQ("or", NLDB0::getGateName(gate->getModel()));
  EXPECT_EQ(f.s, gate->getInstTerm(NLDB0::getGateNTerms(gate->getModel())->getBit(0))->getNet());
  EXPECT_EQ(f.t, gate->getInstTerm(NLDB0::getGateNTerms(gate->getModel())->getBit(1))->getNet());
  EXPECT_THAT(gate->getRTLInfos()->getSourceOrigins(), ElementsAre(second, first));
  EXPECT_THAT(select->getRTLInfos()->getSourceOrigins(), ElementsAre(second, first));
  EXPECT_EQ(0u, SNLUtils::mergeRepeatedMuxSelections(f.design));
}

TEST_F(SNLUtilsTest, mergeRepeatedMuxSelectionsRetainsNonExclusiveAndDifferentValues) {
  for (const auto* variation : {"port", "data_use", "select_use", "other_driver", "other_consumer",
                               "permutation", "partial", "different", "feedback", "disconnected"}) {
    SCOPED_TRACE(variation);
    auto f = makeRepeatedMuxFixture(db_->getLibrary(NLName("MYLIB")));
    const std::string kind(variation);
    auto* a = NLDB0::getMux2InputA(f.model);
    if (kind == "port") {
      SNLScalarTerm::create(f.design, SNLTerm::Direction::Output)->setNet(f.temporary->getBit(0));
    } else if (kind == "data_use" || kind == "select_use") {
      f.outer->getInstTerm(kind == "data_use" ? static_cast<SNLBitTerm*>(NLDB0::getMux2InputB(f.model)->getBit(0)) :
        static_cast<SNLBitTerm*>(NLDB0::getMux2Select(f.model)))->setNet(f.temporary->getBit(0));
    } else if (kind == "other_driver") {
      auto* driver = SNLInstance::create(f.design, NLDB0::getAssign());
      driver->setTermNet(NLDB0::getAssignInput(), f.s);
      driver->setTermNet(NLDB0::getAssignOutput(), f.temporary->getBit(0));
    } else if (kind == "other_consumer") {
      auto* consumer = SNLInstance::create(f.design, NLDB0::getAssign());
      consumer->setTermNet(NLDB0::getAssignInput(), f.temporary->getBit(1));
      consumer->setTermNet(NLDB0::getAssignOutput(), SNLScalarNet::create(f.design));
    } else if (kind == "permutation") {
      f.outer->getInstTerm(a->getBit(0))->setNet(f.temporary->getBit(1));
      f.outer->getInstTerm(a->getBit(1))->setNet(f.temporary->getBit(0));
    } else if (kind == "partial") {
      f.outer->getInstTerm(a->getBit(2))->setNet(f.a->getBit(2));
    } else if (kind == "different") {
      f.outer->getInstTerm(NLDB0::getMux2InputB(f.model)->getBit(2))->setNet(f.a->getBit(2));
    } else if (kind == "feedback") {
      f.inner->getInstTerm(a->getBit(0))->setNet(f.output->getBit(0));
    } else {
      f.inner->getInstTerm(a->getBit(2))->setNet(nullptr);
    }
    EXPECT_EQ(0u, SNLUtils::mergeRepeatedMuxSelections(f.design));
    EXPECT_EQ(f.inner, f.design->getInstance(NLName("inner")));
  }
}

TEST_F(SNLUtilsTest, mergeRepeatedMuxSelectionsHandlesChainsAndEqualSelectors) {
  auto reversed = makeRepeatedMuxFixture(db_->getLibrary(NLName("MYLIB")), true);
  EXPECT_EQ(1u, SNLUtils::mergeRepeatedMuxSelections(reversed.design));
  auto f = makeRepeatedMuxFixture(db_->getLibrary(NLName("MYLIB")));
  f.outer->setTermNet(NLDB0::getMux2Select(f.model), f.s);
  EXPECT_EQ(1u, SNLUtils::mergeRepeatedMuxSelections(f.design));
  EXPECT_EQ(f.s, f.outer->getInstTerm(NLDB0::getMux2Select(f.model))->getNet());
  EXPECT_EQ(1u, f.design->getInstances().size());
  auto* third = SNLInstance::create(f.design, f.model);
  third->setTermNet(NLDB0::getMux2InputA(f.model), f.output);
  third->setTermNet(NLDB0::getMux2InputB(f.model), f.b);
  third->setTermNet(NLDB0::getMux2Select(f.model), f.t);
  auto* output = SNLBusNet::create(f.design, 2, 0);
  third->setTermNet(NLDB0::getMux2Output(f.model), output);
  f.design->getBusTerm(NLName("y"))->setNet(output);
  EXPECT_EQ(1u, SNLUtils::mergeRepeatedMuxSelections(f.design));
}

TEST_F(SNLUtilsTest, mergeRepeatedMuxSelectionsHandlesCommonFalseValue) {
  auto f = makeRepeatedMuxFixture(db_->getLibrary(NLName("MYLIB")));
  auto* a = NLDB0::getMux2InputA(f.model);
  auto* b = NLDB0::getMux2InputB(f.model);
  f.inner->setTermNet(a, f.b);
  f.inner->setTermNet(b, f.a);
  f.outer->setTermNet(a, f.b);
  f.outer->setTermNet(b, f.temporary);
  EXPECT_EQ(1u, SNLUtils::mergeRepeatedMuxSelections(f.design));
  auto* select = f.outer->getInstTerm(NLDB0::getMux2Select(f.model))->getNet();
  SNLInstance* gate = nullptr;
  for (auto* term : select->getInstTerms())
    if (term->getDirection() == SNLTerm::Direction::Output) gate = term->getInstance();
  ASSERT_NE(nullptr, gate);
  EXPECT_EQ("and", NLDB0::getGateName(gate->getModel()));
  for (NLID::Bit bit = 0; bit < 3; ++bit) {
    EXPECT_EQ(f.b->getBit(bit), f.outer->getInstTerm(a->getBit(bit))->getNet());
    EXPECT_EQ(f.a->getBit(bit), f.outer->getInstTerm(b->getBit(bit))->getNet());
  }
}
