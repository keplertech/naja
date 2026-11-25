// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <algorithm>
#include <set>
#include "NajaLog.h"
#include "NLUniverse.h"

#include "SNLBitNet.h"
#include "SNLBundleTerm.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

#include "SNLLibertyConstructor.h"
#include "SNLLibertyConstructorException.h"
#include "SNLVRLConstructor.h"
#include "NLBitVecDynamic.h"
#include "SNLDesignModeling.h"
using ::testing::ElementsAre;

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*  library_;
};

TEST_F(SNLLibertyConstructorTest1, testBusses) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bus_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("bus_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("ram"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBusTerms().size());
  auto clk = design->getScalarTerm(NLName("clk"));
  ASSERT_NE(nullptr, clk);
  EXPECT_EQ(SNLTerm::Direction::Input, clk->getDirection());
  auto rd_out = design->getBusTerm(NLName("rd_out"));
  ASSERT_NE(nullptr, rd_out);
  EXPECT_EQ(SNLTerm::Direction::Output, rd_out->getDirection());
  EXPECT_EQ(14, rd_out->getMSB());
  EXPECT_EQ(0, rd_out->getLSB());
}

TEST_F(SNLLibertyConstructorTest1, testBusDirectionInheritedFromChildPins) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bus_direction_from_pins.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("test_gpio_bus_direction_bug"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("PADCELL_DRV_BUG"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBusTerms().size());
  auto drv = design->getBusTerm(NLName("DRV"));
  ASSERT_NE(nullptr, drv);
  EXPECT_EQ(SNLTerm::Direction::Input, drv->getDirection());
  EXPECT_EQ(1, drv->getMSB());
  EXPECT_EQ(0, drv->getLSB());
}

TEST_F(SNLLibertyConstructorTest1, testBusDirectionInheritedSkippingChildWithoutDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bus_direction_from_partial_pins.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("test_gpio_bus_direction_partial"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("PADCELL_DRV_PARTIAL"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBusTerms().size());
  auto drv = design->getBusTerm(NLName("DRV"));
  ASSERT_NE(nullptr, drv);
  EXPECT_EQ(SNLTerm::Direction::Input, drv->getDirection());
  EXPECT_EQ(1, drv->getMSB());
  EXPECT_EQ(0, drv->getLSB());
}

TEST_F(SNLLibertyConstructorTest1, testBundleTermsIssue120) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bundle_issue_120.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("example"), library_->getName());
  EXPECT_EQ(1, library_->getSNLDesigns().size());

  auto design = library_->getSNLDesign(NLName("cell_def"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(2, design->getBundleTerms().size());

  auto ck = design->getScalarTerm(NLName("CK"));
  auto se = design->getScalarTerm(NLName("SE"));
  auto si = design->getScalarTerm(NLName("SI"));
  auto d = design->getBundleTerm(NLName("D"));
  auto qn = design->getBundleTerm(NLName("QN"));
  ASSERT_NE(nullptr, ck);
  ASSERT_NE(nullptr, se);
  ASSERT_NE(nullptr, si);
  ASSERT_NE(nullptr, d);
  ASSERT_NE(nullptr, qn);

  EXPECT_THAT(
    std::vector<SNLTerm*>(design->getTerms().begin(), design->getTerms().end()),
    ElementsAre(ck, se, si, d, qn));

  auto d0 = design->getScalarTerm(NLName("D0"));
  auto d1 = design->getScalarTerm(NLName("D1"));
  auto qn0 = design->getScalarTerm(NLName("QN0"));
  auto qn1 = design->getScalarTerm(NLName("QN1"));
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, d1);
  ASSERT_NE(nullptr, qn0);
  ASSERT_NE(nullptr, qn1);

  EXPECT_EQ(d, d0->getBundleOwner());
  EXPECT_EQ(d, d1->getBundleOwner());
  EXPECT_EQ(qn, qn0->getBundleOwner());
  EXPECT_EQ(qn, qn1->getBundleOwner());
  EXPECT_EQ(d0, d->getMember(0));
  EXPECT_EQ(d1, d->getMember(1));
  EXPECT_EQ(qn0, qn->getMember(0));
  EXPECT_EQ(qn1, qn->getMember(1));
  EXPECT_EQ(d->getFlatID(), d0->getFlatID());
  EXPECT_EQ(qn->getFlatID(), qn0->getFlatID());

  EXPECT_THAT(
    std::vector<SNLBitTerm*>(design->getBitTerms().begin(), design->getBitTerms().end()),
    ElementsAre(ck, se, si, d0, d1, qn0, qn1));
}

TEST_F(SNLLibertyConstructorTest1, testBundleDirectionInheritedByMembers) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bundle_direction_inherited.lib"));
  constructor.construct(testPath);

  auto design = library_->getSNLDesign(NLName("cell_def"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getBundleTerms().size());

  auto d = design->getBundleTerm(NLName("D"));
  ASSERT_NE(nullptr, d);
  EXPECT_EQ(SNLTerm::Direction::Input, d->getDirection());

  auto d0 = design->getScalarTerm(NLName("D0"));
  auto d1 = design->getScalarTerm(NLName("D1"));
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, d1);
  EXPECT_EQ(SNLTerm::Direction::Input, d0->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, d1->getDirection());
  EXPECT_EQ(d, d0->getBundleOwner());
  EXPECT_EQ(d, d1->getBundleOwner());
}

TEST_F(SNLLibertyConstructorTest1, testBundleBusDirectionInheritedByMember) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bundle_bus_direction_inherited.lib"));
  constructor.construct(testPath);

  auto design = library_->getSNLDesign(NLName("cell_def"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getBundleTerms().size());

  auto d = design->getBundleTerm(NLName("D"));
  auto d0 = design->getBusTerm(NLName("D0"));
  ASSERT_NE(nullptr, d);
  ASSERT_NE(nullptr, d0);
  EXPECT_EQ(SNLTerm::Direction::Input, d->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, d0->getDirection());
  EXPECT_EQ(1, d0->getMSB());
  EXPECT_EQ(0, d0->getLSB());
  EXPECT_EQ(d, d0->getBundleOwner());
  EXPECT_EQ(d0, d->getMember(0));
}

TEST_F(SNLLibertyConstructorTest1, testBundleTermsIssue120VerilogIntegration) {
  SNLLibertyConstructor libertyConstructor(library_);
  std::filesystem::path libertyPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bundle_issue_120.lib"));
  libertyConstructor.construct(libertyPath);

  auto cellDef = library_->getSNLDesign(NLName("cell_def"));
  ASSERT_NE(nullptr, cellDef);

  auto rtlLibrary = NLLibrary::create(library_->getDB(), NLName("RTL"));
  SNLVRLConstructor verilogConstructor(rtlLibrary);
  std::filesystem::path verilogPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("..")
      / std::filesystem::path("verilog")
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("bundle_issue_120.v"));

  EXPECT_NO_THROW(verilogConstructor.parse(verilogPath));
  verilogConstructor.setFirstPass(false);
  EXPECT_NO_THROW(verilogConstructor.parse(verilogPath));

  auto top = rtlLibrary->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  ASSERT_EQ(1, top->getInstances().size());
  auto instance = top->getInstance(NLName("u_cell"));
  ASSERT_NE(nullptr, instance);
  EXPECT_EQ(cellDef, instance->getModel());

  auto ck = cellDef->getScalarTerm(NLName("CK"));
  auto d0 = cellDef->getScalarTerm(NLName("D0"));
  auto d1 = cellDef->getScalarTerm(NLName("D1"));
  auto qn0 = cellDef->getScalarTerm(NLName("QN0"));
  auto qn1 = cellDef->getScalarTerm(NLName("QN1"));
  ASSERT_NE(nullptr, ck);
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, d1);
  ASSERT_NE(nullptr, qn0);
  ASSERT_NE(nullptr, qn1);

  EXPECT_EQ(static_cast<SNLBitNet*>(top->getScalarNet(NLName("clock_net"))), instance->getInstTerm(ck)->getNet());
  EXPECT_EQ(static_cast<SNLBitNet*>(top->getScalarNet(NLName("data_in_1"))), instance->getInstTerm(d0)->getNet());
  EXPECT_EQ(static_cast<SNLBitNet*>(top->getScalarNet(NLName("data_in_0"))), instance->getInstTerm(d1)->getNet());
  EXPECT_NE(nullptr, instance->getInstTerm(qn0)->getNet());
  EXPECT_NE(nullptr, instance->getInstTerm(qn1)->getNet());
  EXPECT_NE(instance->getInstTerm(qn0)->getNet(), instance->getInstTerm(qn1)->getNet());
  EXPECT_EQ(7, instance->getInstTerms().size());
}

TEST_F(SNLLibertyConstructorTest1, testBufferFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("buffer_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("buffer_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("buffer"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(NLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(1, tt.size());
  EXPECT_THAT(tt.getDependencies(), ::testing::ElementsAre(1));
  EXPECT_TRUE(NLBitVecDynamic(0b10, 2) == tt.bits());
  EXPECT_TRUE(SNLDesignModeling::isBuf(design));
}

TEST_F(SNLLibertyConstructorTest1, testInvFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("inv_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("inv_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("inv"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(NLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(1, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b01, 2) == tt.bits());
  EXPECT_TRUE(SNLDesignModeling::isInv(design));
}

TEST_F(SNLLibertyConstructorTest1, testAnd2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("and2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("and2_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("and2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b1000, 4) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testAnd4Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("and4_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("and4_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("and4"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto i2 = design->getScalarTerm(NLName("I2"));
  ASSERT_NE(nullptr, i2);
  EXPECT_EQ(SNLTerm::Direction::Input, i2->getDirection());
  auto i3 = design->getScalarTerm(NLName("I3"));
  ASSERT_NE(nullptr, i3);
  EXPECT_EQ(SNLTerm::Direction::Input, i3->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(4, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0x8000, 16) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testOr2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("or2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("or2_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("or2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b1110, 4) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testXor2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("xor2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("xor2_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("xor2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b0110, 4) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testGate0Function) {
  //function: "!(A | (B1 & B2))";
  //order 0: A 1: B1 2: B2
  //Expected Truth Table: tt(3, 0x15);

  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("gates_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("gates_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto gate0 = library_->getSNLDesign(NLName("gate0"));
  ASSERT_NE(nullptr, gate0);
  EXPECT_EQ(4, gate0->getTerms().size());
  EXPECT_EQ(4, gate0->getScalarTerms().size());
  EXPECT_TRUE(gate0->getBusTerms().empty());
  auto a = gate0->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto b1 = gate0->getScalarTerm(NLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  auto b2 = gate0->getScalarTerm(NLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  auto z = gate0->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(gate0);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(3, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0x15, 8) == tt.bits());

  auto gate1 = library_->getSNLDesign(NLName("gate0"));
  ASSERT_NE(nullptr, gate1);
  EXPECT_EQ(4, gate1->getTerms().size());
  EXPECT_EQ(4, gate1->getScalarTerms().size());
  EXPECT_TRUE(gate1->getBusTerms().empty());
  a = gate1->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  b1 = gate1->getScalarTerm(NLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  b2 = gate1->getScalarTerm(NLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  z = gate1->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  tt = SNLDesignModeling::getTruthTable(gate1);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(3, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0x15, 8) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testLogic01Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("logic01_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("logic01_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto logic0 = library_->getSNLDesign(NLName("logic0"));
  ASSERT_NE(nullptr, logic0);
  EXPECT_EQ(1, logic0->getTerms().size());
  EXPECT_EQ(1, logic0->getScalarTerms().size());
  EXPECT_TRUE(logic0->getBusTerms().empty());
  auto z = logic0->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(logic0);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(0, tt.size());
  EXPECT_TRUE(0b0 == tt.bits().operator uint64_t());
  EXPECT_TRUE(SNLDesignModeling::isConst0(logic0));

  auto logic1 = library_->getSNLDesign(NLName("logic1"));
  ASSERT_NE(nullptr, logic1);
  EXPECT_EQ(1, logic1->getTerms().size());
  EXPECT_EQ(1, logic1->getScalarTerms().size());
  EXPECT_TRUE(logic1->getBusTerms().empty());
  z = logic1->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  tt = SNLDesignModeling::getTruthTable(logic1);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(0, tt.size());
  EXPECT_EQ(0b1, tt.bits().operator uint64_t());
  EXPECT_TRUE(SNLDesignModeling::isConst1(logic1));
}

TEST_F(SNLLibertyConstructorTest1, testBufZFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bufz_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("bufz_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("bufz"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(NLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto en = design->getScalarTerm(NLName("EN"));
  ASSERT_NE(nullptr, en);
  EXPECT_EQ(SNLTerm::Direction::Input, en->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest1, testOAI222Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("OAI222_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("OAI222_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("OAI222"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(7, design->getTerms().size());
  EXPECT_EQ(7, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto a1 = design->getScalarTerm(NLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(NLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b1 = design->getScalarTerm(NLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  auto b2 = design->getScalarTerm(NLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  auto c1 = design->getScalarTerm(NLName("C1"));
  ASSERT_NE(nullptr, c1);
  EXPECT_EQ(SNLTerm::Direction::Input, c1->getDirection());
  auto c2 = design->getScalarTerm(NLName("C2"));
  ASSERT_NE(nullptr, c2);
  EXPECT_EQ(SNLTerm::Direction::Input, c2->getDirection());
  auto zn = design->getScalarTerm(NLName("ZN"));
  ASSERT_NE(nullptr, zn);
  EXPECT_EQ(SNLTerm::Direction::Output, zn->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(6, tt.size());
  uint64_t result = 0x111f111f111fffff;
  EXPECT_TRUE(NLBitVecDynamic(result, 64) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testFA_X1Function) {
  //This test tests a multiple output term primitive.
  //The primitive is a full adder with 3 inputs and 2 outputs.
  
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("FA_X1.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("FA_X1_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("FA_X1"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto a = design->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto b = design->getScalarTerm(NLName("B"));
  ASSERT_NE(nullptr, b);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto ci = design->getScalarTerm(NLName("CI"));
  ASSERT_NE(nullptr, ci);
  EXPECT_EQ(SNLTerm::Direction::Input, ci->getDirection());
  auto co = design->getScalarTerm(NLName("CO"));
  ASSERT_NE(nullptr, co);
  EXPECT_EQ(SNLTerm::Direction::Output, co->getDirection());
  auto s = design->getScalarTerm(NLName("S"));
  ASSERT_NE(nullptr, s);
  EXPECT_EQ(SNLTerm::Direction::Output, s->getDirection());
  // Check the truth table per output
  auto tt_co = SNLDesignModeling::getTruthTable(design, co->getFlatID());
  EXPECT_TRUE(tt_co.isInitialized());
  EXPECT_EQ(3, tt_co.size());
  auto tt_s = SNLDesignModeling::getTruthTable(design, s->getFlatID());
  EXPECT_TRUE(tt_s.isInitialized());
  EXPECT_EQ(3, tt_s.size());
  // Check the truth table for the design as a whole
  // Note: The design truth table is not initialized for multiple outputs.
  // This is a limitation of the current implementation.
  // Uncomment the following lines if you want to test the design truth table.
  //EXPECT_TRUE(SNLDesignModeling::isInitialized(design));
}

TEST_F(SNLLibertyConstructorTest1, testFF) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("FF.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("FF_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("FF"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto ck = design->getScalarTerm(NLName("CK"));
  ASSERT_NE(nullptr, ck);
  EXPECT_EQ(SNLTerm::Direction::Input, ck->getDirection());
  auto d = design->getScalarTerm(NLName("D"));
  ASSERT_NE(nullptr, d);
  EXPECT_EQ(SNLTerm::Direction::Input, d->getDirection());
  auto q = design->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, q);
  EXPECT_EQ(SNLTerm::Direction::Output, q->getDirection());
  // No truth table on seq output
  auto tt_q = SNLDesignModeling::getTruthTable(design, q->getID());
  EXPECT_FALSE(tt_q.isInitialized());
  auto inputs = SNLDesignModeling::getClockRelatedInputs(ck);
  EXPECT_EQ(1, inputs.size());
  EXPECT_EQ(d, *inputs.begin());
  auto outputs = SNLDesignModeling::getClockRelatedOutputs(ck);
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(q, *outputs.begin());
  ASSERT_TRUE(SNLDesignModeling::hasSequentialModel(design));
  const auto& model = SNLDesignModeling::getSequentialModel(design);
  ASSERT_EQ(model.states.size(), 1u);
  ASSERT_EQ(model.outputs.size(), 1u);
  EXPECT_EQ(model.clockedOn.nodes[model.clockedOn.root].term, ck);
  EXPECT_EQ(model.states[0].nextState.nodes[model.states[0].nextState.root].term, d);
  EXPECT_EQ(
      model.outputs[0].function.nodes[model.outputs[0].function.root].operation,
      SNLDesignModeling::BooleanExpression::Operator::State);
}

TEST_F(SNLLibertyConstructorTest1, testStateTableFunctionIsOpaque) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "statetable_state_function.lib";
  testing::internal::CaptureStdout();
  constructor.construct(testPath);
  naja::log::get()->flush();
  const auto diagnostics = testing::internal::GetCapturedStdout();
  EXPECT_NE(std::string::npos, diagnostics.find("cell ICG"));
  EXPECT_NE(std::string::npos, diagnostics.find("cell STATE_FUNCTION_ONLY"));
  EXPECT_NE(std::string::npos, diagnostics.find("cell STATE_RISING_OUTPUT"));
  EXPECT_NE(std::string::npos, diagnostics.find("cell STATE_BUS_TIMING"));
  EXPECT_NE(std::string::npos, diagnostics.find("cell FF_WITH_STATE_FUNCTION"));
  EXPECT_NE(
      std::string::npos,
      diagnostics.find(
          "unsupported Liberty statetable/state_function behavioral modeling"));

  auto* icg = library_->getSNLDesign(NLName("ICG"));
  ASSERT_NE(nullptr, icg);
  EXPECT_EQ(4, icg->getTerms().size());
  EXPECT_EQ(4, icg->getScalarTerms().size());
  EXPECT_TRUE(icg->getBusTerms().empty());

  auto* clk = icg->getScalarTerm(NLName("CLK"));
  auto* ena = icg->getScalarTerm(NLName("ENA"));
  auto* se = icg->getScalarTerm(NLName("SE"));
  ASSERT_NE(nullptr, clk);
  ASSERT_NE(nullptr, ena);
  ASSERT_NE(nullptr, se);
  EXPECT_EQ(SNLTerm::Direction::Input, clk->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, ena->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, se->getDirection());
  auto* gclk = icg->getScalarTerm(NLName("GCLK"));
  ASSERT_NE(nullptr, gclk);
  EXPECT_EQ(SNLTerm::Direction::Output, gclk->getDirection());

  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(icg));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(icg).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(icg, gclk->getFlatID()).isInitialized());
  EXPECT_FALSE(SNLDesignModeling::isConst0(icg));
  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(icg));
  EXPECT_TRUE(SNLDesignModeling::isSequential(icg));
  EXPECT_TRUE(SNLDesignModeling::isClock(clk));

  const auto clockRelatedInputs =
      SNLDesignModeling::getClockRelatedInputs(clk);
  const std::set<SNLBitTerm*> clockRelatedInputSet(
      clockRelatedInputs.begin(), clockRelatedInputs.end());
  EXPECT_EQ((std::set<SNLBitTerm*>{ena, se}), clockRelatedInputSet);
  for (auto* input : {ena, se}) {
    const auto relatedClocks = SNLDesignModeling::getInputRelatedClocks(input);
    ASSERT_EQ(1u, relatedClocks.size());
    EXPECT_EQ(clk, *relatedClocks.begin());
  }
  const auto gclkCombinatorialInputs =
      SNLDesignModeling::getCombinatorialInputs(gclk);
  ASSERT_EQ(1u, gclkCombinatorialInputs.size());
  EXPECT_EQ(clk, *gclkCombinatorialInputs.begin());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(gclk).empty());

  auto* stateFunctionOnly =
      library_->getSNLDesign(NLName("STATE_FUNCTION_ONLY"));
  ASSERT_NE(nullptr, stateFunctionOnly);
  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(stateFunctionOnly));
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(stateFunctionOnly).isInitialized());
  auto* stateInput = stateFunctionOnly->getScalarTerm(NLName("D"));
  auto* stateOutput = stateFunctionOnly->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, stateInput);
  ASSERT_NE(nullptr, stateOutput);
  const auto stateCombinatorialInputs =
      SNLDesignModeling::getCombinatorialInputs(stateOutput);
  ASSERT_EQ(1u, stateCombinatorialInputs.size());
  EXPECT_EQ(stateInput, *stateCombinatorialInputs.begin());

  auto* stateRisingOutput =
      library_->getSNLDesign(NLName("STATE_RISING_OUTPUT"));
  ASSERT_NE(nullptr, stateRisingOutput);
  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(stateRisingOutput));
  auto* stateClock = stateRisingOutput->getScalarTerm(NLName("CLK"));
  auto* risingOutput = stateRisingOutput->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, stateClock);
  ASSERT_NE(nullptr, risingOutput);
  EXPECT_TRUE(
      SNLDesignModeling::getCombinatorialInputs(risingOutput).empty());
  const auto risingOutputClocks =
      SNLDesignModeling::getOutputRelatedClocks(risingOutput);
  ASSERT_EQ(1u, risingOutputClocks.size());
  EXPECT_EQ(stateClock, *risingOutputClocks.begin());

  auto* stateBusTiming =
      library_->getSNLDesign(NLName("STATE_BUS_TIMING"));
  ASSERT_NE(nullptr, stateBusTiming);
  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(stateBusTiming));
  auto* stateBusData = stateBusTiming->getBusTerm(NLName("D"));
  auto* stateBusOutput = stateBusTiming->getBusTerm(NLName("Q"));
  ASSERT_NE(nullptr, stateBusData);
  ASSERT_NE(nullptr, stateBusOutput);
  const auto stateBusDataBits = stateBusData->getBits();
  const std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> expectedDataBits(
      stateBusDataBits.begin(), stateBusDataBits.end());
  ASSERT_EQ(2u, expectedDataBits.size());
  const auto stateBusOutputBits = stateBusOutput->getBits();
  ASSERT_EQ(2u, stateBusOutputBits.size());
  for (auto* outputBit : stateBusOutputBits) {
    const auto combinatorialInputs =
        SNLDesignModeling::getCombinatorialInputs(outputBit);
    const std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> actualInputs(
        combinatorialInputs.begin(), combinatorialInputs.end());
    EXPECT_EQ(expectedDataBits, actualInputs);
  }

  auto* ffWithStateFunction =
      library_->getSNLDesign(NLName("FF_WITH_STATE_FUNCTION"));
  ASSERT_NE(nullptr, ffWithStateFunction);
  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(ffWithStateFunction));
  EXPECT_TRUE(SNLDesignModeling::hasSequentialModel(ffWithStateFunction));
  auto* ffClk = ffWithStateFunction->getScalarTerm(NLName("CLK"));
  auto* ffData = ffWithStateFunction->getScalarTerm(NLName("D"));
  auto* ffOutput = ffWithStateFunction->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, ffClk);
  ASSERT_NE(nullptr, ffData);
  ASSERT_NE(nullptr, ffOutput);
  const auto ffInputClocks =
      SNLDesignModeling::getInputRelatedClocks(ffData);
  const auto ffOutputClocks =
      SNLDesignModeling::getOutputRelatedClocks(ffOutput);
  ASSERT_EQ(1u, ffInputClocks.size());
  ASSERT_EQ(1u, ffOutputClocks.size());
  EXPECT_EQ(ffClk, *ffInputClocks.begin());
  EXPECT_EQ(ffClk, *ffOutputClocks.begin());
}

TEST_F(SNLLibertyConstructorTest1,
       testStateTableTimingRejectsOutputRelatedPin) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "errors" / "timing_only_related_output_error.lib";

  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "Combinational timing `related_pin` `Q` for term `Q` "
            "resolves to an output term"));
  }
}

TEST_F(SNLLibertyConstructorTest1,
       testStateTableTimingRejectsUnknownRelatedPin) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "errors" /
      "timing_only_unknown_related_pin_error.lib";

  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "Combinational timing `related_pin` `MISSING` for term `Q` "
            "does not resolve to an input or inout term"));
  }
}

TEST_F(SNLLibertyConstructorTest1, testFFScanModel) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "FF_scan.lib";
  constructor.construct(testPath);
  auto* design = library_->getSNLDesign(NLName("FFSCAN"));
  ASSERT_NE(nullptr, design);
  auto* ck = design->getScalarTerm(NLName("CK"));
  auto* d = design->getScalarTerm(NLName("D"));
  auto* se = design->getScalarTerm(NLName("SE"));
  auto* si = design->getScalarTerm(NLName("SI"));
  auto* qn = design->getScalarTerm(NLName("QN"));

  ASSERT_TRUE(SNLDesignModeling::hasSequentialModel(design));
  const auto& model = SNLDesignModeling::getSequentialModel(design);
  ASSERT_EQ(model.states.size(), 1u);
  ASSERT_EQ(model.outputs.size(), 1u);
  std::set<SNLBitTerm*> nextStateTerms;
  for (const auto& node : model.states[0].nextState.nodes) {
    if (node.operation == SNLDesignModeling::BooleanExpression::Operator::Term) {
      nextStateTerms.insert(node.term);
    }
  }
  EXPECT_EQ(nextStateTerms, (std::set<SNLBitTerm*>{d, se, si}));
  EXPECT_EQ(SNLDesignModeling::SNLActiveLevel::High, SNLDesignModeling::getActiveLevel(se));
  EXPECT_EQ(SNLDesignModeling::getTermRole(d),
            SNLDesignModeling::SNLTermRole::DataInput);
  EXPECT_EQ(SNLDesignModeling::getTermRole(se),
            SNLDesignModeling::SNLTermRole::ScanEnable);
  EXPECT_EQ(SNLDesignModeling::getTermRole(si),
            SNLDesignModeling::SNLTermRole::ScanInput);
  EXPECT_EQ(SNLDesignModeling::getClockRelatedInputs(ck).size(), 3u);
  EXPECT_EQ(SNLDesignModeling::getClockRelatedOutputs(ck).size(), 1u);
  EXPECT_NE(qn, nullptr);
}

TEST_F(SNLLibertyConstructorTest1, testFFSequentialModelCoverage) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "FF_sequential_coverage.lib";
  constructor.construct(testPath);

  using Value = SNLDesignModeling::SequentialState::ClearPresetValue;
  const std::vector<std::pair<const char*, Value>> expectedValues {
      {"FF_CLEAR_PRESET_L", Value::Zero},
      {"FF_VALUE_H", Value::One},
      {"FF_VALUE_N", Value::Hold},
      {"FF_VALUE_T", Value::Toggle},
      {"FF_VALUE_UNKNOWN", Value::Unknown}};
  for (const auto& [name, expected] : expectedValues) {
    auto* design = library_->getSNLDesign(NLName(name));
    ASSERT_NE(nullptr, design);
    ASSERT_TRUE(SNLDesignModeling::hasSequentialModel(design));
    const auto& model = SNLDesignModeling::getSequentialModel(design);
    ASSERT_EQ(1u, model.states.size());
    EXPECT_EQ(expected, model.states[0].clearPresetValue);
  }

  auto* controlled =
      library_->getSNLDesign(NLName("FF_CLEAR_PRESET_L"));
  const auto& controlledModel =
      SNLDesignModeling::getSequentialModel(controlled);
  EXPECT_TRUE(controlledModel.states[0].clear.has_value());
  EXPECT_TRUE(controlledModel.states[0].preset.has_value());
  ASSERT_EQ(2u, controlledModel.outputs.size());
  auto* clock = controlled->getScalarTerm(NLName("CK"));
  EXPECT_EQ(3u, SNLDesignModeling::getClockRelatedInputs(clock).size());
  EXPECT_EQ(2u, SNLDesignModeling::getClockRelatedOutputs(clock).size());

  auto* incomplete =
      library_->getSNLDesign(NLName("FF_MISSING_NEXT_STATE"));
  ASSERT_NE(nullptr, incomplete);
  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(incomplete));
}

TEST_F(SNLLibertyConstructorTest1, testMultipleFFGroups) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "multiple_ff_groups.lib";

  EXPECT_NO_THROW(constructor.construct(testPath));
  EXPECT_NE(nullptr, library_->getSNLDesign(NLName("INV")));
  auto* multiClock = library_->getSNLDesign(NLName("badcell"));
  ASSERT_NE(nullptr, multiClock);
  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(multiClock));

  auto* multiState = library_->getSNLDesign(NLName("MULTI_STATE_FF"));
  ASSERT_NE(nullptr, multiState);
  ASSERT_TRUE(SNLDesignModeling::hasSequentialModel(multiState));
  const auto& model = SNLDesignModeling::getSequentialModel(multiState);
  ASSERT_EQ(2u, model.states.size());
  ASSERT_EQ(2u, model.outputs.size());
  EXPECT_TRUE(std::any_of(
      model.states[1].nextState.nodes.begin(),
      model.states[1].nextState.nodes.end(),
      [](const auto& node) {
        return node.operation ==
                   SNLDesignModeling::BooleanExpression::Operator::State &&
               node.state == 0;
      }));
}

TEST_F(SNLLibertyConstructorTest1, testLatch) {
  using Operator = SNLDesignModeling::BooleanExpression::Operator;
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "latch.lib";

  EXPECT_NO_THROW(constructor.construct(testPath));
  auto* latch = library_->getSNLDesign(NLName("DLATCH_LOW"));
  ASSERT_NE(nullptr, latch);
  ASSERT_TRUE(SNLDesignModeling::hasSequentialModel(latch));
  const auto& model = SNLDesignModeling::getSequentialModel(latch);
  EXPECT_EQ(SNLDesignModeling::SequentialModel::Kind::Latch, model.kind);
  ASSERT_EQ(1u, model.states.size());
  ASSERT_EQ(2u, model.outputs.size());

  // `enable : "!CLK"` is the control expression, `data_in : "D"` the data.
  auto* clk = latch->getScalarTerm(NLName("CLK"));
  auto* d = latch->getScalarTerm(NLName("D"));
  ASSERT_NE(nullptr, clk);
  ASSERT_NE(nullptr, d);
  const auto& enableRoot = model.clockedOn.nodes.at(model.clockedOn.root);
  EXPECT_EQ(Operator::Not, enableRoot.operation);
  EXPECT_TRUE(std::any_of(
      model.clockedOn.nodes.begin(), model.clockedOn.nodes.end(),
      [clk](const auto& node) {
        return node.operation == Operator::Term && node.term == clk;
      }));
  const auto& dataRoot =
      model.states[0].nextState.nodes.at(model.states[0].nextState.root);
  EXPECT_EQ(Operator::Term, dataRoot.operation);
  EXPECT_EQ(d, dataRoot.term);
  auto* qn = latch->getScalarTerm(NLName("QN"));
  ASSERT_NE(nullptr, qn);
  const auto outputIt = std::find_if(
      model.outputs.begin(), model.outputs.end(),
      [qn](const auto& output) { return output.term == qn; });
  ASSERT_NE(model.outputs.end(), outputIt);
  EXPECT_EQ(Operator::Not,
            outputIt->function.nodes.at(outputIt->function.root).operation);
}

TEST_F(SNLLibertyConstructorTest1, testFFBank) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "ff_bank.lib";

  EXPECT_NO_THROW(constructor.construct(testPath));
  auto* bank = library_->getSNLDesign(NLName("DFF2_BANK"));
  ASSERT_NE(nullptr, bank);
  ASSERT_TRUE(SNLDesignModeling::hasSequentialModel(bank));
  const auto& model = SNLDesignModeling::getSequentialModel(bank);
  ASSERT_EQ(2u, model.states.size());
  ASSERT_EQ(2u, model.outputs.size());

  // Bank bit 1 samples bundle member D1.
  auto* d1 = bank->getScalarTerm(NLName("D1"));
  ASSERT_NE(nullptr, d1);
  EXPECT_TRUE(std::any_of(
      model.states[1].nextState.nodes.begin(),
      model.states[1].nextState.nodes.end(),
      [d1](const auto& node) {
        return node.operation ==
                   SNLDesignModeling::BooleanExpression::Operator::Term &&
               node.term == d1;
      }));

  // Bundle member QN1 carries `function : "IQN"` from the bundle: state 1.
  auto* qn1 = bank->getScalarTerm(NLName("QN1"));
  ASSERT_NE(nullptr, qn1);
  const auto outputIt = std::find_if(
      model.outputs.begin(), model.outputs.end(),
      [qn1](const auto& output) { return output.term == qn1; });
  ASSERT_NE(model.outputs.end(), outputIt);
  EXPECT_TRUE(std::any_of(
      outputIt->function.nodes.begin(),
      outputIt->function.nodes.end(),
      [](const auto& node) {
        return node.operation ==
                   SNLDesignModeling::BooleanExpression::Operator::State &&
               node.state == 1;
      }));
}

TEST_F(SNLLibertyConstructorTest1, testFFBankMalformedStaysLoadable) {
  // A bank with too few arguments, a non-numeric or zero width, or an
  // expression that does not resolve gets no sequential model but keeps the
  // cell loadable; none of these is a library error.
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "ff_bank_malformed.lib";

  EXPECT_NO_THROW(constructor.construct(testPath));
  for (const char* cellName : {"BANK_TOO_FEW_ARGS", "BANK_BAD_WIDTH",
                               "BANK_ZERO_WIDTH", "BANK_BAD_EXPRESSION"}) {
    auto* cell = library_->getSNLDesign(NLName(cellName));
    ASSERT_NE(nullptr, cell) << cellName;
    EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(cell)) << cellName;
    EXPECT_NE(nullptr, cell->getScalarTerm(NLName("Q1"))) << cellName;
  }
}

TEST_F(SNLLibertyConstructorTest1, testSequentialModelErrorHasCellContext) {
  SNLLibertyConstructor constructor(library_);
  const auto testPath = std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "errors" / "unknown_sequential_state.lib";

  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "line 2, cell `BROKEN_FF`"));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find("unknown_sequential_state.lib"));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "Invalid `next_state` expression for `ff (IQ, IQN)` at line 5"));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "Scalar term `UNKNOWN_STATE` referenced at character 1 "
            "was not found in the cell interface."));
  }
}

TEST_F(SNLLibertyConstructorTest1, testScanMetadata) {
  using Modeling = SNLDesignModeling;
  using Role = Modeling::SNLTermRole;
  using Level = Modeling::SNLActiveLevel;
  std::vector<std::string> warnings;
  auto sink = naja::log::addCallbackSink([&](auto, std::string_view message) {
    warnings.emplace_back(message);
  }, spdlog::level::warn);
  SNLLibertyConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks" / "tests" / "scan_metadata.lib");
  naja::log::removeSink(sink);
  for (const auto& name : {"scan_high", "scan_low", "agree", "conflict", "missing",
                          "nonmux", "inverted", "plain", "contradiction", "mixed_polarity",
                          "metadata_only", "unmarked_mux"}) {
    SCOPED_TRACE(name);
    auto* design = library_->getSNLDesign(NLName(name));
    ASSERT_NE(nullptr, design);
    EXPECT_EQ(5u, design->getTerms().size());
    auto* a = design->getScalarTerm(NLName("a"));
    auto* b = design->getScalarTerm(NLName("b"));
    auto* c = design->getScalarTerm(NLName("c"));
    auto* k = design->getScalarTerm(NLName("k"));
    auto* z = design->getScalarTerm(NLName("z"));
    const std::string cell(name);
    bool unmarked = cell == "plain" || cell == "unmarked_mux";
    EXPECT_EQ(unmarked ? Role::Other : Role::DataInput, Modeling::getTermRole(a));
    bool noScan = unmarked || cell == "conflict";
    EXPECT_EQ(noScan ? Role::Other : Role::ScanInput, Modeling::getTermRole(b));
    EXPECT_EQ(noScan ? Role::Other : Role::ScanEnable, Modeling::getTermRole(c));
    auto level = noScan || cell == "nonmux" || cell == "mixed_polarity" ? Level::NA :
        cell == "scan_low" || cell == "contradiction" ? Level::Low : Level::High;
    EXPECT_EQ(level, Modeling::getActiveLevel(c));
    EXPECT_EQ(Level::NA, Modeling::getResetActiveLevel(c));
    ASSERT_TRUE(Modeling::hasSequentialModel(design));
    const auto& model = Modeling::getSequentialModel(design);
    ASSERT_EQ(1u, model.states.size());
    ASSERT_EQ(1u, model.outputs.size());
    EXPECT_EQ(k, model.clockedOn.nodes[model.clockedOn.root].term);
    EXPECT_EQ(Modeling::BooleanExpression::Operator::State,
        model.outputs[0].function.nodes[model.outputs[0].function.root].operation);
    auto terms = std::set<SNLBitTerm*>{};
    for (const auto& node : model.states[0].nextState.nodes) {
      if (node.term) terms.insert(node.term);
    }
    EXPECT_EQ(cell == "plain" ? (std::set<SNLBitTerm*>{a}) :
        (std::set<SNLBitTerm*>{a, b, c}), terms);
    EXPECT_THAT(Modeling::getOutputRelatedClocks(z), ElementsAre(k));
    EXPECT_TRUE(Modeling::getCombinatorialInputs(z).empty());
    auto* owner = SNLDesign::create(
        NLLibrary::create(library_->getDB()), NLName("owner_" + cell));
    auto* instance = SNLInstance::create(owner, design);
    EXPECT_EQ(level, Modeling::getActiveLevel(instance->getInstTerm(c)));
  }
  auto* testOnly = library_->getSNLDesign(NLName("test_only"));
  ASSERT_NE(nullptr, testOnly);
  EXPECT_EQ(2u, testOnly->getTerms().size());
  EXPECT_FALSE(Modeling::hasSequentialModel(testOnly));
  EXPECT_TRUE(Modeling::isBuf(testOnly));
  EXPECT_EQ(Role::DataInput, Modeling::getTermRole(testOnly->getScalarTerm(NLName("a"))));
  auto warned = [&](const std::string& fragment) {
    return std::any_of(warnings.begin(), warnings.end(), [&](const auto& warning) {
      return warning.find(fragment) != std::string::npos;
    });
  };
  EXPECT_TRUE(warned("cell conflict pin b"));
  EXPECT_TRUE(warned("cell conflict pin c"));
  EXPECT_TRUE(warned("cell missing test_cell pin absent"));
  EXPECT_TRUE(warned("cell nonmux pin c"));
  EXPECT_TRUE(warned("cell contradiction pin c"));
}
