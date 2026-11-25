// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <fstream>

#include "NLException.h"
#include "NLUniverse.h"

#include "SNLBundleTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

#include "SNLLibertyConstructor.h"
#include "SNLLibertyConstructorException.h"
#include "NLBitVecDynamic.h"
#include "SNLDesignModeling.h"
#include "SNLCapnP.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*  library_;
    NLDB* db_ = nullptr;
};

TEST_F(SNLLibertyConstructorTest2, test) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto and2 = library_->getSNLDesign(NLName("and2"));
  ASSERT_NE(nullptr, and2);
  EXPECT_EQ(3, and2->getTerms().size());
  EXPECT_EQ(3, and2->getScalarTerms().size());
  EXPECT_TRUE(and2->getBusTerms().empty());
  auto i0 = and2->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = and2->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = and2->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(and2);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(NLBitVecDynamic(0b1000, 4), tt.bits());
  //
  //EXPECT_TRUE(SNLDesignModeling::isAnd(design));

  auto ff = library_->getSNLDesign(NLName("FF"));
  ASSERT_NE(nullptr, ff);
  EXPECT_EQ(3, ff->getTerms().size());
  EXPECT_EQ(3, ff->getScalarTerms().size());
  EXPECT_TRUE(ff->getBusTerms().empty());
  auto d = ff->getScalarTerm(NLName("D"));
  ASSERT_NE(nullptr, d);
  EXPECT_EQ(SNLTerm::Direction::Input, d->getDirection());
  auto ck = ff->getScalarTerm(NLName("CK"));
  ASSERT_NE(nullptr, ck);
  EXPECT_EQ(SNLTerm::Direction::Input, ck->getDirection());
  auto q = ff->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, q);
  EXPECT_EQ(SNLTerm::Direction::Output, q->getDirection());
  auto ff_tt = SNLDesignModeling::getTruthTable(ff);
  EXPECT_FALSE(ff_tt.isInitialized());
  {
    auto clocks = SNLDesignModeling::getInputRelatedClocks(d);
    EXPECT_EQ(1, clocks.size());
  }
  {
    auto clocks = SNLDesignModeling::getOutputRelatedClocks(q);
    EXPECT_EQ(1, clocks.size());
  }
  // dump to naja if
  std::filesystem::path outPath("dump_if");
  SNLCapnP::dump(db_, outPath);
  {
    NLUniverse::get()->destroy();
    NLUniverse* universe = NLUniverse::create();
    db_ = NLDB::create(universe);
    library_ = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("MYLIB"));
    SNLLibertyConstructor constructor(library_);
    std::filesystem::path testPath(
        std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
        / std::filesystem::path("benchmarks")
        / std::filesystem::path("tests")
        / std::filesystem::path("small.lib"));
    constructor.construct(testPath);
  }
#if 0
  db_ = SNLCapnP::load(outPath, true);
  NLUniverse::get()->destroy();
  EXPECT_THROW(SNLCapnP::load(outPath, true), NLException);
  {
    NLUniverse::get()->destroy();
    NLUniverse* universe = NLUniverse::create();
    db_ = NLDB::create(universe);
  }
  EXPECT_THROW(SNLCapnP::load(outPath, true), NLException);
#endif
}

TEST_F(SNLLibertyConstructorTest2, TestGZ) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz"));
  constructor.construct(testPath);
}

TEST_F(SNLLibertyConstructorTest2, testBundleCapnpDumpUnsupported) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bundle_issue_120.lib"));
  constructor.construct(testPath);
  EXPECT_THROW(SNLCapnP::dump(db_, "bundle_dump_if"), NLException);
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryBusBitsAreMarkedSequential) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_bus_bits_are_marked_sequential.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array;
    data_type : bit;
    bit_width : 4;
    bit_from : 3;
    bit_to : 0;
    downto : true;
  }
  type (mem_addr) {
    base_type : array;
    data_type : bit;
    bit_width : 2;
    bit_from : 1;
    bit_to : 0;
    downto : true;
  }
  cell(mem) {
    memory() {
      type : ram;
      address_width : 2;
      word_width : 4;
    }
    pin(clk) {
      direction : input;
      clock : true;
    }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() {
        address : addr_in;
      }
      timing() {
        related_pin : "clk";
        timing_type : rising_edge;
      }
    }
    bus(addr_in) {
      bus_type : mem_addr;
      direction : input;
      timing() {
        related_pin : "clk";
        timing_type : setup_rising;
      }
      timing() {
        related_pin : "clk";
        timing_type : hold_rising;
      }
    }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() {
        address : addr_in;
        clocked_on : "clk";
      }
      timing() {
        related_pin : "clk";
        timing_type : setup_rising;
      }
      timing() {
        related_pin : "clk";
        timing_type : hold_rising;
      }
    }
    pin(we_in) {
      direction : input;
      timing() {
        related_pin : "clk";
        timing_type : setup_rising;
      }
      timing() {
        related_pin : "clk";
        timing_type : hold_rising;
      }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  constructor.construct(tempPath);

  auto* memory = library_->getSNLDesign(NLName("mem"));
  ASSERT_NE(nullptr, memory);

  auto* clk = memory->getScalarTerm(NLName("clk"));
  auto* rdOut = memory->getBusTerm(NLName("rd_out"));
  auto* addrIn = memory->getBusTerm(NLName("addr_in"));
  auto* wdIn = memory->getBusTerm(NLName("wd_in"));
  auto* weIn = memory->getScalarTerm(NLName("we_in"));
  ASSERT_NE(nullptr, clk);
  ASSERT_NE(nullptr, rdOut);
  ASSERT_NE(nullptr, addrIn);
  ASSERT_NE(nullptr, wdIn);
  ASSERT_NE(nullptr, weIn);

  for (auto* bit : rdOut->getBits()) {
    EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(bit).size())
        << "read-data bit " << bit->getName().getString()
        << "[" << bit->getBit() << "] should be clocked";
  }

  for (auto* bit : addrIn->getBits()) {
    EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(bit).size())
        << "address bit " << bit->getName().getString()
        << "[" << bit->getBit() << "] should be clock-related";
  }

  for (auto* bit : wdIn->getBits()) {
    EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(bit).size())
        << "write-data bit " << bit->getName().getString()
        << "[" << bit->getBit() << "] should be clock-related";
  }

  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(weIn).size());

  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(memory));
  const auto interface = SNLDesignModeling::getMemoryInterface(memory);
  EXPECT_EQ(4u, interface.width);
  EXPECT_EQ(4u, interface.depth);
  EXPECT_EQ(2u, interface.abits);
  EXPECT_EQ(clk, interface.clock);
  EXPECT_EQ(1u, interface.readPorts.size());
  EXPECT_EQ(1u, interface.writePorts.size());
  EXPECT_EQ(2u, interface.readPorts.front().address.size());
  EXPECT_EQ(4u, interface.readPorts.front().data.size());
  EXPECT_EQ(2u, interface.writePorts.front().address.size());
  EXPECT_EQ(4u, interface.writePorts.front().data.size());
  EXPECT_TRUE(interface.writePorts.front().mask.empty());
  EXPECT_EQ(1u, interface.writePorts.front().enables.size());
  EXPECT_EQ(weIn, interface.writePorts.front().enables.front());
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryInterfaceCapturesMaskAndControls) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_interface_captures_mask_and_controls.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array;
    data_type : bit;
    bit_width : 4;
    bit_from : 3;
    bit_to : 0;
    downto : true;
  }
  type (mem_addr) {
    base_type : array;
    data_type : bit;
    bit_width : 2;
    bit_from : 1;
    bit_to : 0;
    downto : true;
  }
  cell(mem) {
    memory() {
      type : ram;
      address_width : 2;
      word_width : 4;
    }
    pin(clk) { direction : input; clock : true; }
    pin(we_in) {
      direction : input;
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    pin(ce_in) {
      direction : input;
      timing() { related_pin : "clk"; timing_type : combinational; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() { address : addr_in; }
      timing() { timing_type : rising_edge; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : rising_edge; }
    }
    bus(addr_in) {
      bus_type : mem_addr;
      direction : input;
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(w_mask_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(w_aux_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  constructor.construct(tempPath);

  auto* memory = library_->getSNLDesign(NLName("mem"));
  ASSERT_NE(nullptr, memory);
  auto* weIn = memory->getScalarTerm(NLName("we_in"));
  auto* ceIn = memory->getScalarTerm(NLName("ce_in"));
  ASSERT_NE(nullptr, weIn);
  ASSERT_NE(nullptr, ceIn);

  ASSERT_TRUE(SNLDesignModeling::hasMemoryInterface(memory));
  const auto interface = SNLDesignModeling::getMemoryInterface(memory);
  ASSERT_EQ(1u, interface.writePorts.size());
  EXPECT_EQ(4u, interface.writePorts.front().data.size());
  EXPECT_EQ(4u, interface.writePorts.front().mask.size());
  ASSERT_EQ(1u, interface.writePorts.front().extraWriteInputs.size());
  EXPECT_EQ(4u, interface.writePorts.front().extraWriteInputs.front().size());
  EXPECT_EQ(2u, interface.writePorts.front().enables.size());

  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> enableTerms(
      interface.writePorts.front().enables.begin(),
      interface.writePorts.front().enables.end());
  EXPECT_EQ(1u, enableTerms.count(weIn));
  EXPECT_EQ(1u, enableTerms.count(ceIn));
}

TEST_F(SNLLibertyConstructorTest2, testLibertySequentialTimingPreservesDistinctClocks) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_distinct_sequential_clocks.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(seqlib) {
  cell(ff_multi_clock) {
    ff(IQ, IQN) { clocked_on : "clk0"; next_state : "d"; }
    pin(clk0) { direction : input; clock : true; }
    pin(clk1) { direction : input; clock : true; }
    pin(mode) { direction : input; }
    pin(d) {
      direction : input;
      timing() { related_pin : "clk0"; timing_type : setup_rising; when : "mode"; }
      timing() { related_pin : "clk0"; timing_type : hold_rising; when : "mode"; }
      timing() { related_pin : "clk0"; timing_type : setup_rising; when : "!mode"; }
      timing() { related_pin : "clk1"; timing_type : setup_rising; when : "!mode"; }
      timing() { related_pin : "clk1"; timing_type : hold_rising; when : "!mode"; }
    }
    pin(q) {
      direction : output;
      function : "IQ";
      timing() { related_pin : "clk0"; timing_type : rising_edge; when : "mode"; }
      timing() { related_pin : "clk1"; timing_type : rising_edge; when : "!mode"; }
      timing() { related_pin : "clk1"; timing_type : rising_edge; when : "mode"; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  constructor.construct(tempPath);

  auto* primitive = library_->getSNLDesign(NLName("ff_multi_clock"));
  ASSERT_NE(nullptr, primitive);
  auto* clk0 = primitive->getScalarTerm(NLName("clk0"));
  auto* clk1 = primitive->getScalarTerm(NLName("clk1"));
  auto* d = primitive->getScalarTerm(NLName("d"));
  auto* q = primitive->getScalarTerm(NLName("q"));
  ASSERT_NE(nullptr, clk0);
  ASSERT_NE(nullptr, clk1);
  ASSERT_NE(nullptr, d);
  ASSERT_NE(nullptr, q);

  const std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> expectedClocks{
      clk0, clk1};
  const auto inputRelatedClocks =
      SNLDesignModeling::getInputRelatedClocks(d);
  const std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> inputClockSet(
      inputRelatedClocks.begin(), inputRelatedClocks.end());
  EXPECT_EQ(expectedClocks, inputClockSet);
  const auto outputRelatedClocks =
      SNLDesignModeling::getOutputRelatedClocks(q);
  const std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> outputClockSet(
      outputRelatedClocks.begin(), outputRelatedClocks.end());
  EXPECT_EQ(expectedClocks, outputClockSet);

  auto* standardLibrary = NLLibrary::create(
      db_, NLLibrary::Type::Standard, NLName("standard"));
  auto* top = SNLDesign::create(standardLibrary, NLName("top"));
  auto* instance = SNLInstance::create(top, primitive, NLName("u0"));
  auto* id = instance->getInstTerm(d);
  auto* iq = instance->getInstTerm(q);
  auto* iclk0 = instance->getInstTerm(clk0);
  auto* iclk1 = instance->getInstTerm(clk1);
  const std::set<SNLInstTerm*> expectedInstanceClocks{iclk0, iclk1};
  const auto inputRelatedInstanceClocks =
      SNLDesignModeling::getInputRelatedClocks(id);
  const std::set<SNLInstTerm*> inputInstanceClockSet(
      inputRelatedInstanceClocks.begin(), inputRelatedInstanceClocks.end());
  EXPECT_EQ(expectedInstanceClocks, inputInstanceClockSet);
  const auto outputRelatedInstanceClocks =
      SNLDesignModeling::getOutputRelatedClocks(iq);
  const std::set<SNLInstTerm*> outputInstanceClockSet(
      outputRelatedInstanceClocks.begin(), outputRelatedInstanceClocks.end());
  EXPECT_EQ(expectedInstanceClocks, outputInstanceClockSet);

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest2, testLibertySequentialTimingRejectsUnknownRelatedPin) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_unknown_sequential_related_pin.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(seqlib) {
  cell(ff_bad_clock) {
    ff(IQ, IQN) { clocked_on : "clk"; next_state : "d"; }
    pin(clk) { direction : input; clock : true; }
    pin(d) {
      direction : input;
      timing() { related_pin : "missing_clk"; timing_type : setup_rising; }
    }
    pin(q) { direction : output; function : "IQ"; }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "`missing_clk` for term `d` does not resolve to a scalar input term"));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryRequiresReadAddress) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_missing_read_address.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array; data_type : bit; bit_width : 2; bit_from : 1; bit_to : 0; downto : true;
  }
  cell(mem) {
    memory() { type : ram; address_width : 1; word_width : 2; }
    pin(clk) { direction : input; clock : true; }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() {}
      timing() { related_pin : "clk"; timing_type : rising_edge; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryRequiresWriteClock) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_missing_write_clock.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array; data_type : bit; bit_width : 2; bit_from : 1; bit_to : 0; downto : true;
  }
  type (mem_addr) {
    base_type : array; data_type : bit; bit_width : 1; bit_from : 0; bit_to : 0; downto : true;
  }
  cell(mem) {
    memory() { type : ram; address_width : 1; word_width : 2; }
    pin(clk) { direction : input; clock : true; }
    bus(addr_in) { bus_type : mem_addr; direction : input; }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryRequiresBusAddresses) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_scalar_addresses.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array; data_type : bit; bit_width : 2; bit_from : 1; bit_to : 0; downto : true;
  }
  cell(mem) {
    memory() { type : ram; address_width : 1; word_width : 2; }
    pin(clk) { direction : input; clock : true; }
    pin(addr_in) {
      direction : input;
      timing() { related_pin : "clk"; timing_type : setup_rising; }
    }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() { address : addr_in; }
      timing() { related_pin : "clk"; timing_type : rising_edge; }
    }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryRequiresWriteBusAddress) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_scalar_write_address.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array; data_type : bit; bit_width : 2; bit_from : 1; bit_to : 0; downto : true;
  }
  type (mem_addr) {
    base_type : array; data_type : bit; bit_width : 1; bit_from : 0; bit_to : 0; downto : true;
  }
  cell(mem) {
    memory() { type : ram; address_width : 1; word_width : 2; }
    pin(clk) { direction : input; clock : true; }
    bus(rd_addr) { bus_type : mem_addr; direction : input; }
    pin(wr_addr) { direction : input; }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() { address : rd_addr; }
      timing() { related_pin : "clk"; timing_type : rising_edge; }
    }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : wr_addr; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest2, GatePolarityInputOrder) {
  SNLLibertyConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_LIBERTY_BENCHMARKS) /
      "benchmarks/tests/gate_polarity.lib");
  using Gate = SNLTruthTable::GenericType;
  struct Expected { const char* cell; Gate type; const char* inverted; };
  for (const auto& expected : {
      Expected{"cell0", Gate::NAND, "P"},
      Expected{"cell1", Gate::AND, "P"},
      Expected{"cell2", Gate::NOR, "R"},
      Expected{"cell3", Gate::OR, "P"}}) {
    SCOPED_TRACE(expected.cell);
    auto* cell = library_->getSNLDesign(NLName(expected.cell));
    ASSERT_NE(nullptr, cell);
    auto polarity = SNLDesignModeling::getGatePolarity(cell);
    ASSERT_TRUE(polarity);
    EXPECT_EQ(expected.type, polarity->type);
    EXPECT_EQ(std::vector<SNLBitTerm*>{cell->getScalarTerm(NLName(expected.inverted))},
        polarity->invertedInputs);
    EXPECT_FALSE(SNLDesignModeling::isAnd(cell));
    EXPECT_FALSE(SNLDesignModeling::isNand(cell));
    EXPECT_FALSE(SNLDesignModeling::isOr(cell));
    EXPECT_FALSE(SNLDesignModeling::isNor(cell));
    EXPECT_FALSE(SNLDesignModeling::isXor(cell));
    EXPECT_FALSE(SNLDesignModeling::isXnor(cell));
  }
}

namespace {
std::string writtenExpression(const SNLDesignModeling::BooleanExpression& expression, size_t id) {
  using Op = SNLDesignModeling::BooleanExpression::Operator;
  const auto& node = expression.nodes.at(id);
  if (node.operation == Op::Term) return node.term->getName().getString();
  if (node.operation == Op::Constant) return node.constant ? "1" : "0";
  std::string result = node.operation == Op::Not ? "not(" : node.operation == Op::And ? "and(" : node.operation == Op::Or ? "or(" : "xor(";
  for (size_t i = 0; i < node.operands.size(); ++i) {
    if (i) result += ",";
    result += writtenExpression(expression, node.operands[i]);
  }
  return result + ")";
}
}

TEST_F(SNLLibertyConstructorTest2, WrittenCombinationalFunctions) {
  SNLLibertyConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_LIBERTY_BENCHMARKS) / "benchmarks/tests/written_functions.lib");
  struct Expected { const char* cell; const char* output; const char* expression; uint64_t mask; };
  const Expected expected[] = {
    {"bubble", "Y", "not(and(not(P),Q))", 0xb},
    {"product", "Y", "and(P,not(Q))", 0x2},
    {"nor", "Y", "not(or(P,not(Q)))", 0x4},
    {"aoi", "Y", "not(or(and(P,Q),R))", 0x7},
    {"oai", "Y", "not(and(or(P,Q),or(R,S)))", 0x111f},
    {"parity", "Y", "xor(P,Q,R)", 0x96},
    {"buffer", "Y", "P", 0x2},
    {"inverter", "Y", "not(P)", 0x1},
    {"postfix", "Y", "or(and(P,not(Q)),R)", 0xf2},
    {"double", "Y", "not(not(P))", 0x2},
    {"constant", "Y", "and(P,1)", 0x2},
    {"pair", "Y", "and(P,Q)", 0x8},
    {"pair", "Z", "not(or(P,Q))", 0x1},
  };
  for (const auto& item : expected) {
    auto* design = library_->getSNLDesign(NLName(item.cell));
    ASSERT_NE(nullptr, design);
    auto* output = design->getScalarTerm(NLName(item.output));
    auto function = SNLDesignModeling::getFunction(output);
    ASSERT_TRUE(function) << item.cell;
    EXPECT_EQ(item.expression, writtenExpression(*function, function->root)) << item.cell;
    EXPECT_EQ(item.mask, static_cast<uint64_t>(SNLDesignModeling::getTruthTable(design, output->getOrderID()).bits())) << item.cell;
  }
}
