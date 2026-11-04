// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <algorithm>
#include <sstream>
#include <limits>

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLClock.h"
#include "NLException.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"
using namespace naja::NL;

class NLClockTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLName("MYLIB"));
      top_ = SNLDesign::create(library_, NLName("TOP"));
      clk_ = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("clk"));
      clk2_ = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("clk2"));
      model_ = SNLDesign::create(library_, NLName("MODEL"));
      modelClk_ = SNLScalarTerm::create(model_, SNLTerm::Direction::Input, NLName("clk"));
      instance_ = SNLInstance::create(top_, model_, NLName("inst"));
      other_ = SNLDesign::create(library_, NLName("OTHER"));
      otherClk_ = SNLScalarTerm::create(other_, SNLTerm::Direction::Input, NLName("clk"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    NLDB*           db_        {nullptr};
    NLLibrary*      library_   {nullptr};
    SNLDesign*      top_       {nullptr};
    SNLScalarTerm*  clk_       {nullptr};
    SNLScalarTerm*  clk2_      {nullptr};
    SNLDesign*      model_     {nullptr};
    SNLScalarTerm*  modelClk_  {nullptr};
    SNLInstance*    instance_  {nullptr};
    SNLDesign*      other_     {nullptr};
    SNLScalarTerm*  otherClk_  {nullptr};
};

TEST_F(NLClockTest, testPrimaryClock) {
  NLClock* clock = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});

  EXPECT_EQ(NLClock::Kind::Primary, clock->getKind());
  EXPECT_EQ(NLName("sys"), clock->getName());
  EXPECT_EQ(top_, clock->getDesign());
  EXPECT_DOUBLE_EQ(10.0, clock->getPeriod());
  EXPECT_DOUBLE_EQ(0.1, clock->getFrequency());
  EXPECT_EQ("ns", clock->getTimeUnit());
  //Default waveform: 50% duty cycle.
  EXPECT_DOUBLE_EQ(0.0, clock->getRiseAt());
  EXPECT_DOUBLE_EQ(5.0, clock->getFallAt());
  EXPECT_EQ(nullptr, clock->getMaster());
  EXPECT_EQ(clock, clock->getRootClock());
  EXPECT_TRUE(clock->isSynchronousWith(clock));
  EXPECT_EQ(1u, clock->getSources().size());
  EXPECT_EQ(clk_, *clock->getSources().begin());
}

TEST_F(NLClockTest, testWaveform) {
  NLClock* clock = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  clock->setWaveform(2.0, 7.0);
  EXPECT_DOUBLE_EQ(2.0, clock->getRiseAt());
  EXPECT_DOUBLE_EQ(7.0, clock->getFallAt());
  EXPECT_THROW(clock->setWaveform(5.0, 2.0), NLException);
  EXPECT_THROW(clock->setWaveform(0.0, 11.0), NLException);
  //A rejected waveform leaves the previous one in place.
  EXPECT_DOUBLE_EQ(7.0, clock->getFallAt());
}

TEST_F(NLClockTest, testGeneratedClockPeriodAndSynchrony) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* divided = NLClock::createGenerated(top_, NLName("div2"), master, clk_, 2, 1, false, {clk2_});

  EXPECT_EQ(NLClock::Kind::Generated, divided->getKind());
  EXPECT_EQ(master, divided->getMaster());
  EXPECT_EQ(clk_, divided->getMasterSource());
  EXPECT_DOUBLE_EQ(20.0, divided->getPeriod());
  EXPECT_DOUBLE_EQ(0.05, divided->getFrequency());
  EXPECT_EQ(master, divided->getRootClock());
  EXPECT_TRUE(divided->isSynchronousWith(master));
  EXPECT_TRUE(master->isSynchronousWith(divided));
}

TEST_F(NLClockTest, testGeneratedClockMultiply) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* multiplied = NLClock::createGenerated(top_, NLName("mul2"), master, clk_, 1, 2, true, {clk2_});
  EXPECT_DOUBLE_EQ(5.0, multiplied->getPeriod());
  EXPECT_TRUE(multiplied->isInverted());
  EXPECT_EQ(1u, multiplied->getDivideBy());
  EXPECT_EQ(2u, multiplied->getMultiplyBy());
}

TEST_F(NLClockTest, testGeneratedWaveformsFollowDivisionAndMultiplication) {
  auto* master = NLClock::createPrimary(top_, NLName("root"), 12.0, "ns", {clk_});
  master->setWaveform(3.0, 7.0);
  struct Case { unsigned divide; unsigned multiply; double period; double high; };
  for (const auto& entry : {Case{1, 1, 12.0, 4.0}, Case{2, 1, 24.0, 12.0},
      Case{3, 1, 36.0, 12.0}, Case{6, 1, 72.0, 36.0},
      Case{1, 2, 6.0, 2.0}, Case{3, 2, 18.0, 6.0}, Case{2, 3, 8.0, 4.0}}) {
    for (bool invert : {false, true}) {
      auto name = "g" + std::to_string(entry.divide) + "_" + std::to_string(entry.multiply)
        + (invert ? "_inv" : "");
      auto* clock = NLClock::createGenerated(top_, NLName(name), master, clk_,
        entry.divide, entry.multiply, invert, {clk2_, clk_});
      EXPECT_DOUBLE_EQ(entry.period, clock->getPeriod());
      EXPECT_EQ("ns", clock->getTimeUnit());
      EXPECT_DOUBLE_EQ(3.0 + (invert ? entry.high : 0.0), clock->getRiseAt());
      EXPECT_DOUBLE_EQ(3.0 + (invert ? entry.period : entry.high), clock->getFallAt());
      EXPECT_EQ(master, clock->getMaster());
      EXPECT_EQ(clk_, clock->getMasterSource());
      EXPECT_EQ(NLName(name), clock->getName());
      EXPECT_EQ(top_, clock->getDesign());
      const auto sources = clock->getSources();
      EXPECT_EQ((std::vector<NLObject*>{clk2_, clk_}),
        (std::vector<NLObject*>(sources.begin(), sources.end())));
      EXPECT_THROW(clock->setWaveform(0.0, entry.period / 2), NLException);
    }
  }
}

TEST_F(NLClockTest, testGeneratedChainsObserveMasterWaveformChanges) {
  auto* root = NLClock::createPrimary(top_, NLName("root"), 12.0, "ps", {clk_});
  root->setWaveform(3.0, 7.0);
  auto* inverted = NLClock::createGenerated(top_, NLName("inverted"), root, clk_, 1, 1, true, {clk2_});
  auto* divided = NLClock::createGenerated(top_, NLName("divided"), inverted, clk2_, 3, 1, false, {clk_});
  auto* multiplied = NLClock::createGenerated(top_, NLName("multiplied"), divided, clk_, 1, 2, true, {clk2_});
  EXPECT_DOUBLE_EQ(7.0, inverted->getRiseAt());
  EXPECT_DOUBLE_EQ(15.0, inverted->getFallAt()); // inversion wraps beyond the period
  EXPECT_DOUBLE_EQ(7.0, divided->getRiseAt());
  EXPECT_DOUBLE_EQ(31.0, divided->getFallAt());
  EXPECT_DOUBLE_EQ(18.0, multiplied->getPeriod());
  EXPECT_DOUBLE_EQ(19.0, multiplied->getRiseAt());
  EXPECT_DOUBLE_EQ(25.0, multiplied->getFallAt());
  EXPECT_EQ(root, multiplied->getRootClock());
  EXPECT_TRUE(multiplied->isSynchronousWith(root));
  EXPECT_EQ("ps", multiplied->getTimeUnit());

  root->setWaveform(1.0, 9.0);
  EXPECT_DOUBLE_EQ(9.0, inverted->getRiseAt());
  EXPECT_DOUBLE_EQ(13.0, inverted->getFallAt());
  EXPECT_DOUBLE_EQ(9.0, divided->getRiseAt());
  EXPECT_DOUBLE_EQ(21.0, divided->getFallAt());
  EXPECT_DOUBLE_EQ(15.0, multiplied->getRiseAt());
  EXPECT_DOUBLE_EQ(27.0, multiplied->getFallAt());
  EXPECT_DOUBLE_EQ(18.0, multiplied->getPeriod());
  EXPECT_DOUBLE_EQ(1.0 / 18.0, multiplied->getFrequency());
  // A permitted full-period primary high pulse becomes a zero-width inverted pulse.
  root->setWaveform(0.0, 12.0);
  EXPECT_DOUBLE_EQ(12.0, inverted->getRiseAt());
  EXPECT_DOUBLE_EQ(12.0, inverted->getFallAt());
  EXPECT_DOUBLE_EQ(12.0, divided->getFallAt());
}

TEST_F(NLClockTest, testEvenDividerRetainsHalfDutyAfterMasterChanges) {
  auto* root = NLClock::createPrimary(top_, NLName("root"), 12.0, "ns", {clk_});
  auto* divided = NLClock::createGenerated(top_, NLName("divided"), root, clk_, 2, 1, false, {clk2_});
  root->setWaveform(2.0, 5.0);
  EXPECT_DOUBLE_EQ(2.0, divided->getRiseAt());
  EXPECT_DOUBLE_EQ(14.0, divided->getFallAt());
  root->setWaveform(4.0, 11.0);
  EXPECT_DOUBLE_EQ(4.0, divided->getRiseAt());
  EXPECT_DOUBLE_EQ(16.0, divided->getFallAt());
}

TEST_F(NLClockTest, testDerivedRangeValidationBeforeRegistration) {
  auto* root = NLClock::createPrimary(top_, NLName("root"), std::numeric_limits<double>::max(), "ns", {clk_});
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("overflow"), root, clk_, 2, 1, false, {clk2_}), NLException);
  EXPECT_EQ(1u, top_->getClocks().size());
  // Equal factors preserve a representable period without an overflowing P*D intermediate.
  auto* balanced = NLClock::createGenerated(top_, NLName("balanced"), root, clk_, 3, 3, false, {clk2_});
  EXPECT_DOUBLE_EQ(root->getPeriod(), balanced->getPeriod());
}

TEST_F(NLClockTest, testUnrelatedClocksAreNotSynchronous) {
  NLClock* sys = NLClock::createPrimary(top_, NLName("sys"), 20.0, "ns", {clk_});
  NLClock* other = NLClock::createPrimary(top_, NLName("other"), 20.0, "ns", {clk2_});
  //Same period, different roots: not synchronous.
  EXPECT_FALSE(sys->isSynchronousWith(other));
  EXPECT_FALSE(sys->isSynchronousWith(nullptr));
}

TEST_F(NLClockTest, testGeneratedClockOfGeneratedClockKeepsRoot) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* first = NLClock::createGenerated(top_, NLName("g1"), master, clk_, 2, 1, false, {clk2_});
  NLClock* second = NLClock::createGenerated(top_, NLName("g2"), first, clk2_, 2, 1, false, {clk_});
  EXPECT_DOUBLE_EQ(40.0, second->getPeriod());
  EXPECT_EQ(master, second->getRootClock());
  EXPECT_TRUE(second->isSynchronousWith(master));
}

TEST_F(NLClockTest, testRejectsInvalidArguments) {
  EXPECT_THROW(NLClock::createPrimary(top_, NLName(""), 10.0, "ns", {clk_}), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 0.0, "ns", {clk_}), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "", {clk_}), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {}), NLException);
  EXPECT_THROW(NLClock::createPrimary(nullptr, NLName("sys"), 10.0, "ns", {clk_}), NLException);
}

TEST_F(NLClockTest, testRejectsInstanceTerminalSource) {
  SNLInstTerm* instTerm = instance_->getInstTerm(modelClk_);
  ASSERT_NE(nullptr, instTerm);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {instTerm}), NLException);
}

TEST_F(NLClockTest, testRejectsTerminalOfAnotherDesign) {
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {otherClk_}), NLException);
}

TEST_F(NLClockTest, testRejectsNonTerminalSource) {
  try {
    NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {top_});
    FAIL() << "Expected a non-terminal source to be rejected";
  } catch (const NLException& error) {
    EXPECT_NE(std::string::npos, std::string(error.what()).find("is not a top-level terminal"));
    EXPECT_NE(std::string::npos, std::string(error.what()).find(top_->getDescription()));
  }
  EXPECT_TRUE(top_->getClocks().empty());
}

TEST_F(NLClockTest, testPrimaryClockDescriptionAndDebugDump) {
  const auto* clock = NLClock::createPrimary(top_, NLName("sys"), 12.0, "ns", {clk_, clk2_});
  EXPECT_STREQ("NLClock", clock->getTypeName());
  EXPECT_EQ("sys", clock->getString());
  EXPECT_EQ("NLClock sys (period 12.000000 ns)", clock->getDescription());
  std::ostringstream stream;
  clock->debugDump(3, false, stream);
  EXPECT_EQ("   NLClock sys (period 12.000000 ns)\n"
            "     kind: primary\n"
            "     waveform: 0 6\n"
            "     source: " + clk_->getDescription() + "\n"
            "     source: " + clk2_->getDescription() + "\n", stream.str());
}

TEST_F(NLClockTest, testGeneratedClockDebugDump) {
  auto* master = NLClock::createPrimary(top_, NLName("sys"), 12.0, "ps", {clk_});
  for (bool invert : {false, true}) {
    const std::string name = invert ? "inverted" : "plain";
    auto* clock = NLClock::createGenerated(top_, NLName(name), master, clk_, 3, 2, invert, {clk2_});
    EXPECT_THROW(clock->setWaveform(1.0, 8.0), NLException);
    EXPECT_STREQ("NLClock", clock->getTypeName());
    EXPECT_EQ(name, clock->getString());
    EXPECT_EQ("NLClock " + name + " (period 18.000000 ps)", clock->getDescription());
    std::ostringstream stream;
    clock->debugDump(0, true, stream);
    EXPECT_EQ("NLClock " + name + " (period 18.000000 ps)\n"
              "  kind: generated\n"
              "  waveform: " + std::string(invert ? "9 18\n" : "0 9\n")
              + "  source: " + clk2_->getDescription() + "\n"
              "  master: sys divide 3 multiply 2" + (invert ? " inverted\n" : "\n"), stream.str());
  }
}

TEST_F(NLClockTest, testRejectsDuplicateNameInDesign) {
  NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 20.0, "ns", {clk2_}), NLException);
  //Same name in another design is fine.
  EXPECT_NO_THROW(NLClock::createPrimary(other_, NLName("sys"), 20.0, "ns", {otherClk_}));
}

TEST_F(NLClockTest, testRejectsInvalidGeneratedClock) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), nullptr, clk_, 2, 1, false, {clk2_}), NLException);
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), master, clk_, 0, 1, false, {clk2_}), NLException);
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), master, clk_, 1, 0, false, {clk2_}), NLException);
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), master, clk_, 2, 1, false, {}), NLException);

  NLClock* foreignMaster = NLClock::createPrimary(other_, NLName("foreign"), 10.0, "ns", {otherClk_});
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), foreignMaster, clk_, 2, 1, false, {clk2_}), NLException);
}

TEST_F(NLClockTest, testDesignOwnsClocks) {
  NLClock* sys = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* other = NLClock::createPrimary(other_, NLName("other"), 20.0, "ns", {otherClk_});

  EXPECT_EQ(1u, top_->getClocks().size());
  EXPECT_EQ(sys, *top_->getClocks().begin());
  EXPECT_EQ(1u, other_->getClocks().size());

  auto all = db_->getClocks();
  EXPECT_EQ(2u, all.size());
  EXPECT_NE(all.end(), std::find(all.begin(), all.end(), sys));
  EXPECT_NE(all.end(), std::find(all.begin(), all.end(), other));
}

TEST_F(NLClockTest, testDestroyingClockRemovesItFromDesign) {
  NLClock* sys = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  sys->destroy();
  EXPECT_TRUE(top_->getClocks().empty());
  EXPECT_TRUE(db_->getClocks().empty());
}

TEST_F(NLClockTest, testDestroyingDesignDestroysItsClocks) {
  NLClock::createPrimary(other_, NLName("other"), 20.0, "ns", {otherClk_});
  NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});

  other_->destroy();

  auto all = db_->getClocks();
  ASSERT_EQ(1u, all.size());
  EXPECT_EQ(NLName("sys"), (*all.begin())->getName());
}
