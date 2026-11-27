// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "DNL.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLSVConstructor.h"

using namespace naja::NL;

class SNLSVConstructorTestGenerateMemory: public ::testing::Test {
  protected:
    void SetUp() override {
      auto* db = NLDB::create(NLUniverse::create());
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }

    void TearDown() override {
      naja::DNL::destroy();
      NLUniverse::get()->destroy();
    }

    naja::DNL::DNLID sourceIso(naja::DNL::DNLID isoID) {
      auto* dnl = naja::DNL::get();
      for (size_t step = 0; step < dnl->getDNLTerms().size(); ++step) {
        if (isoID == naja::DNL::DNLID_MAX) {
          ADD_FAILURE() << "disconnected assignment chain";
          return isoID;
        }
        const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
        EXPECT_EQ(1u, iso.getDrivers().size());
        if (iso.getDrivers().size() != 1) return naja::DNL::DNLID_MAX;
        const auto& driver = dnl->getDNLTerminalFromID(*iso.getDrivers().begin());
        auto* inst = driver.getDNLInstance().getSNLInstance();
        if (driver.isTopPort() || !inst || !NLDB0::isAssign(inst->getModel())) {
          return isoID;
        }
        isoID = driver.getDNLInstance().getTerminalFromBitTerm(
          NLDB0::getAssignInput()).getIsoID();
      }
      ADD_FAILURE() << "cycle in assignment chain";
      return naja::DNL::DNLID_MAX;
    }

    void checkBanks(const std::string& body, const std::vector<std::string>& bases,
                    bool read = true) {
      const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) /
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
      std::filesystem::create_directories(directory);
      const auto source = directory / "top.sv";
      std::ofstream(source) << "module top #(parameter int N = " << bases.size() << R"()(
  input logic clk, we,
  input logic [1:0] addr,
  input logic [7:0] din [N],
  output logic [7:0] dout [N]
);
)" << body << "\nendmodule\n";
      SNLSVConstructor constructor(library_);
      ASSERT_NO_THROW(constructor.construct(source));
      auto* top = library_->getSNLDesign(NLName("top"));
      ASSERT_NE(nullptr, top);
      size_t count = 0;
      for (auto* inst : top->getInstances()) {
        if (NLDB0::isMemory(inst->getModel())) ++count;
      }
      ASSERT_EQ(bases.size(), count);
      NLUniverse::get()->setTopDesign(top);
      auto* dnl = naja::DNL::get();
      auto* output = top->getBusTerm(NLName("dout"));
      ASSERT_NE(nullptr, output);
      auto* input = top->getBusTerm(NLName("din"));
      ASSERT_NE(nullptr, input);
      for (size_t bank = 0; bank < bases.size(); ++bank) {
        SCOPED_TRACE(bases[bank]);
        auto* inst = top->getInstance(NLName(bases[bank] + "_mem"));
        ASSERT_NE(nullptr, inst);
        ASSERT_TRUE(NLDB0::isMemory(inst->getModel()));
        // Port nets must be scoped too: sharing them can silently short banks.
        for (const auto& suffix : {"mem_waddr_0", "mem_wdata_0", "mem_we_0"}) {
          EXPECT_NE(nullptr, top->getNet(NLName(bases[bank] + "_" + suffix)));
        }
        auto* writeData = NLDB0::getMemoryWriteData(inst->getModel());
        for (int bit = 0; bit < 8; ++bit) {
          const auto& sink = dnl->getTop().getChildInstance(inst).
            getTerminalFromBitTerm(writeData->getBit(bit));
          const auto& driver = dnl->getTop().getTerminalFromBitTerm(
            input->getBit(static_cast<int>((bases.size() - 1 - bank) * 8) + bit));
          EXPECT_EQ(driver.getIsoID(), sourceIso(sink.getIsoID()));
        }
        if (!read) {
          EXPECT_NE(nullptr, top->getNet(NLName(bases[bank] + "_mem_dummy_raddr")));
          EXPECT_NE(nullptr, top->getNet(NLName(bases[bank] + "_mem_dummy_rdata")));
          continue;
        }
        EXPECT_NE(nullptr, top->getNet(NLName(bases[bank] + "_mem_rdata_0")));
        auto* readData = NLDB0::getMemoryReadData(inst->getModel());
        for (int bit = 0; bit < 8; ++bit) {
          const auto& driver = dnl->getTop().getChildInstance(inst).
            getTerminalFromBitTerm(readData->getBit(bit));
          // The unpacked [N] dimension is ascending; element zero is highest.
          const auto& sink = dnl->getTop().getTerminalFromBitTerm(
            output->getBit(static_cast<int>((bases.size() - 1 - bank) * 8) + bit));
          EXPECT_EQ(driver.getIsoID(), sourceIso(sink.getIsoID()));
          const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(sink.getIsoID());
          EXPECT_EQ(1u, iso.getDrivers().size());
        }
      }
    }

    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestGenerateMemory, ForGenerate) {
  checkBanks(R"(
  genvar g;
  for (g = 0; g < N; g++) begin : gen_bank
    logic [7:0] mem [4];
    always_ff @(posedge clk) if (we) mem[addr] <= din[g];
    assign dout[g] = mem[addr];
  end
)", {"top_gen_bank_0_mem", "top_gen_bank_1_mem"});
}

TEST_F(SNLSVConstructorTestGenerateMemory, NestedGenerate) {
  checkBanks(R"(
  for (genvar i = 0; i < 2; i++) begin : gen_a
    for (genvar j = 0; j < 2; j++) begin : gen_b
      logic [7:0] mem [4];
      always_ff @(posedge clk) if (we) mem[addr] <= din[2*i+j];
      assign dout[2*i+j] = mem[addr];
    end
  end
)", {"top_gen_a_0_gen_b_0_mem", "top_gen_a_0_gen_b_1_mem",
     "top_gen_a_1_gen_b_0_mem", "top_gen_a_1_gen_b_1_mem"});
}

TEST_F(SNLSVConstructorTestGenerateMemory, IfGenerate) {
  checkBanks(R"(
  if (N == 2) begin : gen_a
    logic [7:0] mem [4];
    always_ff @(posedge clk) if (we) mem[addr] <= din[0];
    assign dout[0] = mem[addr];
  end
  if (N > 1) begin : gen_b
    logic [7:0] mem [4];
    always_ff @(posedge clk) if (we) mem[addr] <= din[1];
    assign dout[1] = mem[addr];
  end
)", {"top_gen_a_mem", "top_gen_b_mem"});
}

TEST_F(SNLSVConstructorTestGenerateMemory, TwoMemoriesPerScope) {
  checkBanks(R"(
  for (genvar g = 0; g < 2; g++) begin : gen_bank
    logic [7:0] mem [4], other [4];
    always_ff @(posedge clk) if (we) begin
      mem[addr] <= din[2*g];
      other[addr] <= din[2*g+1];
    end
    assign dout[2*g] = mem[addr];
    assign dout[2*g+1] = other[addr];
  end
)", {"top_gen_bank_0_mem", "top_gen_bank_0_other",
     "top_gen_bank_1_mem", "top_gen_bank_1_other"});
}

TEST_F(SNLSVConstructorTestGenerateMemory, WriteOnlyGenerate) {
  checkBanks(R"(
  for (genvar g = 0; g < N; g++) begin : gen_bank
    logic [7:0] mem [4];
    always_ff @(posedge clk) if (we) mem[addr] <= din[g];
    assign dout[g] = '0;
  end
)", {"top_gen_bank_0_mem", "top_gen_bank_1_mem"}, false);
}

TEST_F(SNLSVConstructorTestGenerateMemory, ModuleScopeNamesUnchanged) {
  checkBanks(R"(
  logic [7:0] mem [4];
  always_ff @(posedge clk) if (we) mem[addr] <= din[0];
  assign dout[0] = mem[addr];
)", {"mem"});
}
