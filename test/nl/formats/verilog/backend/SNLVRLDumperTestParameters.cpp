// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "NLDB0.h"
#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLParameter.h"
#include "SNLRTLInfos.h"

using namespace naja::NL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

namespace {

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

}

class SNLVRLDumperTestParameters: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      NLDB* db = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
      model_ = SNLDesign::create(library, NLName("model"));
      top_ = SNLDesign::create(library, NLName("top"));
    }

    SNLInstance* createMemoryInstance() {
      NLDB0::MemorySignature signature;
      signature.width = 8;
      signature.depth = 16;
      signature.abits = 4;
      signature.readPorts = 2;
      signature.writePorts = 2;
      signature.resetMode = NLDB0::MemoryResetMode::AsyncHigh;

      auto* memory = NLDB0::getOrCreateMemory(signature);
      if (nullptr == memory) {
        return nullptr;
      }

      auto* clk = SNLScalarNet::create(top_, NLName("clk"));
      auto* rst = SNLScalarNet::create(top_, NLName("rst"));
      auto* raddr = SNLBusNet::create(top_, 7, 0, NLName("raddr"));
      auto* rdata = SNLBusNet::create(top_, 15, 0, NLName("rdata"));
      auto* waddr = SNLBusNet::create(top_, 7, 0, NLName("waddr"));
      auto* wdata = SNLBusNet::create(top_, 15, 0, NLName("wdata"));
      auto* we = SNLBusNet::create(top_, 1, 0, NLName("we"));

      auto* ins = SNLInstance::create(top_, memory, NLName("mem0"));
      ins->setTermNet(memory->getScalarTerm(NLName("CLK")), clk);
      ins->setTermNet(memory->getScalarTerm(NLName("RST")), rst);
      ins->setTermNet(memory->getBusTerm(NLName("RADDR")), raddr);
      ins->setTermNet(memory->getBusTerm(NLName("RDATA")), rdata);
      ins->setTermNet(memory->getBusTerm(NLName("WADDR")), waddr);
      ins->setTermNet(memory->getBusTerm(NLName("WDATA")), wdata);
      ins->setTermNet(memory->getBusTerm(NLName("WE")), we);

      SNLInstParameter::create(ins, memory->getParameter(NLName("WIDTH")), "8");
      SNLInstParameter::create(ins, memory->getParameter(NLName("DEPTH")), "16");
      SNLInstParameter::create(ins, memory->getParameter(NLName("ABITS")), "4");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RD_PORTS")), "2");
      SNLInstParameter::create(ins, memory->getParameter(NLName("WR_PORTS")), "2");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RST_ENABLE")), "1");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RST_ASYNC")), "1");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RST_ACTIVE_LOW")), "0");
      SNLInstParameter::create(
        ins,
        memory->getParameter(NLName("INIT")),
        "128'h00112233445566778899AABBCCDDEEFF");
      return ins;
    }

    SNLInstance* createWideMux2Instance(size_t width = 8) {
      auto* mux2 = NLDB0::getOrCreateMux2(width);
      if (nullptr == mux2) {
        return nullptr;
      }

      auto* a = SNLBusNet::create(top_, static_cast<NLID::Bit>(width - 1), 0, NLName("mux_a"));
      auto* b = SNLBusNet::create(top_, static_cast<NLID::Bit>(width - 1), 0, NLName("mux_b"));
      auto* y = SNLBusNet::create(top_, static_cast<NLID::Bit>(width - 1), 0, NLName("mux_y"));
      auto* s = SNLScalarNet::create(top_, NLName("mux_s"));

      auto* ins = SNLInstance::create(top_, mux2, NLName("mux0"));
      ins->setTermNet(NLDB0::getMux2InputA(mux2), a);
      ins->setTermNet(NLDB0::getMux2InputB(mux2), b);
      ins->setTermNet(NLDB0::getMux2Select(mux2), s);
      ins->setTermNet(NLDB0::getMux2Output(mux2), y);
      return ins;
    }

    SNLInstance* createDivModInstance() {
      NLDB0::DivModSignature signature;
      signature.width = 8;
      signature.isSigned = true;

      auto* divmod = NLDB0::getOrCreateDivMod(signature);
      if (nullptr == divmod) {
        return nullptr;
      }

      auto* a = SNLBusNet::create(top_, 7, 0, NLName("div_a"));
      auto* b = SNLBusNet::create(top_, 7, 0, NLName("div_b"));
      auto* q = SNLBusNet::create(top_, 7, 0, NLName("div_q"));
      auto* r = SNLBusNet::create(top_, 7, 0, NLName("div_r"));

      auto* ins = SNLInstance::create(top_, divmod, NLName("div0"));
      ins->setTermNet(NLDB0::getDivModDividend(divmod), a);
      ins->setTermNet(NLDB0::getDivModDivisor(divmod), b);
      ins->setTermNet(NLDB0::getDivModQuotient(divmod), q);
      ins->setTermNet(NLDB0::getDivModRemainder(divmod), r);
      SNLInstParameter::create(ins, divmod->getParameter(NLName("WIDTH")), "8");
      SNLInstParameter::create(ins, divmod->getParameter(NLName("SIGNED")), "1");
      return ins;
    }

    SNLInstance* createTableSelectInstance() {
      NLDB0::TableSelectSignature signature;
      signature.width = 4;
      signature.depth = 3;
      signature.abits = 2;

      auto* tableSelect = NLDB0::getOrCreateTableSelect(signature);
      if (nullptr == tableSelect) {
        return nullptr;
      }

      auto* data = SNLBusNet::create(top_, 11, 0, NLName("select_data"));
      auto* address = SNLBusNet::create(top_, 1, 0, NLName("select_address"));
      auto* y = SNLBusNet::create(top_, 3, 0, NLName("select_y"));

      auto* ins = SNLInstance::create(top_, tableSelect, NLName("select0"));
      ins->setTermNet(NLDB0::getTableSelectData(tableSelect), data);
      ins->setTermNet(NLDB0::getTableSelectAddress(tableSelect), address);
      ins->setTermNet(NLDB0::getTableSelectOutput(tableSelect), y);
      SNLInstParameter::create(ins, tableSelect->getParameter(NLName("WIDTH")), "4");
      SNLInstParameter::create(ins, tableSelect->getParameter(NLName("DEPTH")), "3");
      SNLInstParameter::create(ins, tableSelect->getParameter(NLName("ABITS")), "2");
      return ins;
    }

    SNLInstance* createMixedConstBusMux2Instance() {
      auto* mux2 = NLDB0::getOrCreateMux2(4);
      if (nullptr == mux2) {
        return nullptr;
      }

      auto* const0 = SNLScalarNet::create(top_);
      const0->setType(SNLNet::Type::Assign0);
      auto* const1 = SNLScalarNet::create(top_);
      const1->setType(SNLNet::Type::Assign1);
      auto* sourceBus = SNLBusNet::create(top_, 1, 0, NLName("source_bus"));
      auto* b = SNLBusNet::create(top_, 3, 0, NLName("mux_b"));
      auto* y = SNLBusNet::create(top_, 3, 0, NLName("mux_y"));
      auto* s = SNLScalarNet::create(top_, NLName("mux_s"));

      auto* ins = SNLInstance::create(top_, mux2, NLName("mux0"));
      auto* aTerm = NLDB0::getMux2InputA(mux2);
      ins->setTermNet(aTerm, 3, 3, const1, 0, 0);
      ins->setTermNet(aTerm, 2, 2, const0, 0, 0);
      ins->setTermNet(aTerm, 1, 0, sourceBus, 1, 0);
      ins->setTermNet(NLDB0::getMux2InputB(mux2), b);
      ins->setTermNet(NLDB0::getMux2Select(mux2), s);
      ins->setTermNet(NLDB0::getMux2Output(mux2), y);
      return ins;
    }

    SNLInstance* createDFFInstance() {
      auto* dff = NLDB0::getDFF();
      if (nullptr == dff) {
        return nullptr;
      }

      auto* clk = SNLScalarNet::create(top_, NLName("clk"));
      auto* d = SNLScalarNet::create(top_, NLName("d"));
      auto* q = SNLScalarNet::create(top_, NLName("q"));

      auto* ins = SNLInstance::create(top_, dff, NLName("dff0"));
      ins->setTermNet(dff->getScalarTerm(NLName("C")), clk);
      ins->setTermNet(dff->getScalarTerm(NLName("D")), d);
      ins->setTermNet(dff->getScalarTerm(NLName("Q")), q);
      return ins;
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  top_;
    SNLDesign*  model_;
};

TEST_F(SNLVRLDumperTestParameters, test0) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  SNLParameter::create(model_, NLName("PARAM0"), SNLParameter::Type::Decimal, "8");
  auto falseParam = SNLParameter::create(model_, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, NLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  SNLParameter::create(model_, NLName("PARAM3"), SNLParameter::Type::Binary, "4'hF");
  SNLParameter::create(model_, NLName("PARAM4"), SNLParameter::Type::Binary, "4'b0011");
  SNLParameter::create(model_, NLName("PARAM5"), SNLParameter::Type::String, "HELLO");
  auto ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, falseParam, "1");
  SNLInstParameter::create(ins, trueParam, "0");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParameters0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / (top_->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testParameters0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestParameters, testErrors0) {
  ASSERT_TRUE(top_);
  SNLParameter::create(top_, NLName("PARAM"), SNLParameter::Type::Boolean, "YY");
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParametersErrors0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestParameters, testErrors1) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  auto falseParam = SNLParameter::create(model_, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, NLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  auto ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, falseParam, "Y");
  SNLInstParameter::create(ins, trueParam, "N");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParametersErrors1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestParameters, testDefaultInstanceParametersAreOmitted) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);

  auto* sameDecimal = SNLParameter::create(model_, NLName("SAME_DEC"), SNLParameter::Type::Decimal, "8");
  auto* sameBoolean = SNLParameter::create(model_, NLName("SAME_BOOL"), SNLParameter::Type::Boolean, "0");
  auto* sameString = SNLParameter::create(model_, NLName("SAME_STR"), SNLParameter::Type::String, "HELLO");
  auto* diffDecimal = SNLParameter::create(model_, NLName("DIFF_DEC"), SNLParameter::Type::Decimal, "2");

  auto* ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, sameDecimal, "8");
  SNLInstParameter::create(ins, sameBoolean, "FALSE");
  SNLInstParameter::create(ins, sameString, "HELLO");
  SNLInstParameter::create(ins, diffDecimal, "3");

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("model #("));
  EXPECT_NE(std::string::npos, dumped.find(".DIFF_DEC(3)"));
  EXPECT_EQ(std::string::npos, dumped.find(".SAME_DEC("));
  EXPECT_EQ(std::string::npos, dumped.find(".SAME_BOOL("));
  EXPECT_EQ(std::string::npos, dumped.find(".SAME_STR("));
}

TEST_F(SNLVRLDumperTestParameters, testConstantOutputNetsDrivePorts) {
  auto* scalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("constant_out"));
  auto* scalarNet = SNLScalarNet::create(top_, NLName("constant_out"));
  scalarNet->setType(SNLNet::Type::Assign1);
  scalar->setNet(scalarNet);
  auto* bus = SNLBusTerm::create(top_, SNLTerm::Direction::Output, 3, 0, NLName("constant.bus"));
  auto* busNet = SNLBusNet::create(top_, 3, 0, NLName("constant.bus"));
  bus->setNet(busNet);
  const SNLNet::Type types[]{SNLNet::Type::Assign0, SNLNet::Type::Assign1,
      SNLNet::Type::AssignX, SNLNet::Type::AssignZ};
  for (int bit = 0; bit < 4; ++bit) busNet->getBit(bit)->setType(types[bit]);
  std::ostringstream out;
  SNLVRLDumper().dumpDesign(top_, out);
  const auto dumped = out.str();
  EXPECT_NE(dumped.find("assign constant_out = 1'b1;"), std::string::npos);
  for (int bit = 0; bit < 4; ++bit)
    EXPECT_NE(dumped.find("assign \\constant.bus [" + std::to_string(bit) + "] = 1'b" + "01xz"[bit] + ";"),
              std::string::npos);
}

TEST_F(SNLVRLDumperTestParameters, testFullyUnconnectedInstancePortsAreDumped) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);

  auto* input = SNLScalarTerm::create(model_, SNLTerm::Direction::Input, NLName("data_i"));
  SNLScalarTerm::create(model_, SNLTerm::Direction::Output, NLName("empty_o"));
  SNLBusTerm::create(model_, SNLTerm::Direction::Output, 1, 0, NLName("empty_bus_o"));

  auto* data = SNLScalarNet::create(top_, NLName("data"));
  auto* ins = SNLInstance::create(top_, model_, NLName("ins"));
  ins->setTermNet(input, data);

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find(".data_i(data)"));
  EXPECT_NE(std::string::npos, dumped.find(".empty_o()"));
  EXPECT_NE(std::string::npos, dumped.find(".empty_bus_o()"));
}

TEST_F(SNLVRLDumperTestParameters, testMemoryInstanceDump) {
  ASSERT_NE(nullptr, createMemoryInstance());

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_mem #("));
  EXPECT_NE(std::string::npos, dumped.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, dumped.find(".DEPTH(16)"));
  EXPECT_NE(std::string::npos, dumped.find(".ABITS(4)"));
  EXPECT_NE(std::string::npos, dumped.find(".RD_PORTS(2)"));
  EXPECT_NE(std::string::npos, dumped.find(".WR_PORTS(2)"));
  EXPECT_NE(std::string::npos, dumped.find(".WCLK('0)"));
  EXPECT_NE(std::string::npos, dumped.find(".WMASK('0)"));
  EXPECT_NE(std::string::npos, dumped.find(".RST_ENABLE(1)"));
  EXPECT_NE(std::string::npos, dumped.find(".RST_ASYNC(1)"));
  EXPECT_EQ(std::string::npos, dumped.find("\"TRUE\""));
  EXPECT_EQ(std::string::npos, dumped.find("\"FALSE\""));
  EXPECT_EQ(std::string::npos, dumped.find(".RST_ACTIVE_LOW(0)"));
  EXPECT_NE(
    std::string::npos,
    dumped.find(".INIT(128'h00112233445566778899AABBCCDDEEFF)"));
  EXPECT_EQ(std::string::npos, dumped.find("module naja_mem #("));
  EXPECT_EQ(std::string::npos, dumped.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
  EXPECT_EQ(std::string::npos, dumped.find("if (allow_write && addr_index < DEPTH)"));
}

TEST_F(SNLVRLDumperTestParameters, testIndependentMemoryControlConnectivity) {
  NLDB0::MemorySignature signature;
  signature.width = 4;
  signature.depth = 8;
  signature.abits = 3;
  signature.readPorts = 1;
  signature.writePorts = 2;
  signature.independentWriteClocks = true;
  auto* memory = NLDB0::getOrCreateMemory(signature);
  ASSERT_NE(nullptr, memory);
  auto* instance = SNLInstance::create(top_, memory, NLName("mem0"));
  auto* clocks = SNLBusNet::create(top_, 1, 0, NLName("clocks"));
  auto* masks = SNLBusNet::create(top_, 7, 0, NLName("masks"));
  instance->setTermNet(memory->getBusTerm(NLName("WCLK")), clocks);
  instance->setTermNet(memory->getBusTerm(NLName("WMASK")), masks);

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();
  EXPECT_NE(std::string::npos, dumped.find(".WCLK(clocks)"));
  EXPECT_NE(std::string::npos, dumped.find(".WMASK(masks)"));
  EXPECT_EQ(std::string::npos, dumped.find(".WCLK('0)"));
  EXPECT_EQ(std::string::npos, dumped.find(".WMASK('0)"));
}

TEST_F(SNLVRLDumperTestParameters, testZeroMemoryInitUsesModelDefault) {
  auto* memoryInstance = createMemoryInstance();
  ASSERT_NE(nullptr, memoryInstance);
  auto* init = memoryInstance->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, init);
  init->setValue("128'b" + std::string(128, '0'));
  SNLInstParameter::create(
    memoryInstance,
    memoryInstance->getModel()->getParameter(NLName("INIT_ENABLE")),
    "1");

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find(".INIT_ENABLE(1)"));
  EXPECT_EQ(std::string::npos, dumped.find(".INIT("));
}

TEST_F(SNLVRLDumperTestParameters, testMemoryInitZeroLiteralClassification) {
  auto* memoryInstance = createMemoryInstance();
  ASSERT_NE(nullptr, memoryInstance);
  auto* init = memoryInstance->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, init);

  struct TestCase {
    const char* value;
    bool isZero;
  };
  const TestCase testCases[] = {
    {"0", true},
    {"not_a_literal", false},
    {"x'b0", false},
    {"1'sb0", true},
    {"1's", false},
    {"1'q0", false},
  };

  for (const auto& testCase : testCases) {
    SCOPED_TRACE(testCase.value);
    init->setValue(testCase.value);

    std::ostringstream out;
    SNLVRLDumper dumper;
    dumper.dumpDesign(top_, out);
    const auto dumped = out.str();

    if (testCase.isZero) {
      EXPECT_EQ(std::string::npos, dumped.find(".INIT("));
    } else {
      EXPECT_NE(
        std::string::npos,
        dumped.find(std::string(".INIT(") + testCase.value + ")"));
    }
  }
}

TEST_F(SNLVRLDumperTestParameters, testMemoryPrimitiveFileDump) {
  ASSERT_NE(nullptr, createMemoryInstance());

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testMemoryPrimitiveFileDump";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.dumpDesign(top_, outPath);

  const auto topDump = readTextFile(outPath / "top.v");
  EXPECT_NE(std::string::npos, topDump.find("naja_mem #("));
  EXPECT_NE(std::string::npos, topDump.find(".WCLK('0)"));
  EXPECT_NE(std::string::npos, topDump.find(".WMASK('0)"));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_mem #("));

  const auto primitivePath = outPath / "naja_primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitivePath));
  const auto primitiveDump = readTextFile(primitivePath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_fa("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dff #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dlatch #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffn #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffrn #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffr #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffs #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffe #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffre #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffse #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffsr #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffsrn #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffss #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffssn #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffsre #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffsrne #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffsse #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffssne #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
  EXPECT_NE(std::string::npos, primitiveDump.find("integer addr_index;"));
  EXPECT_NE(std::string::npos, primitiveDump.find("task automatic write_ports;"));
  EXPECT_NE(std::string::npos, primitiveDump.find("allow_write = WE[wp];"));
  EXPECT_NE(
    std::string::npos,
    primitiveDump.find("if (WE[later] && WADDR[later*ABITS +: ABITS] == addr_value)"));
  EXPECT_NE(std::string::npos, primitiveDump.find("if (allow_write && addr_index < DEPTH)"));
  EXPECT_NE(
    std::string::npos,
    primitiveDump.find("if (RST_ENABLE && RST_ASYNC && RST_ACTIVE_LOW)"));
  EXPECT_NE(std::string::npos, primitiveDump.find("always @(posedge CLK or negedge RST)"));
  EXPECT_NE(std::string::npos, primitiveDump.find("always @(posedge CLK or posedge RST)"));
  EXPECT_EQ(
    std::string::npos,
    primitiveDump.find("always @(posedge CLK or posedge RST or negedge RST)"));
}

TEST_F(SNLVRLDumperTestParameters, testSequentialPrimitiveFileDump) {
  ASSERT_NE(nullptr, createDFFInstance());

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testSequentialPrimitiveFileDump";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.dumpDesign(top_, outPath);

  const auto topDump = readTextFile(outPath / "top.v");
  EXPECT_NE(std::string::npos, topDump.find("naja_dff dff0 ("));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_dff #("));

  const auto primitivePath = outPath / "naja_primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitivePath));
  const auto primitiveDump = readTextFile(primitivePath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dff #("));
  EXPECT_NE(
    std::string::npos,
    primitiveDump.find("module naja_dff #(\n  parameter WIDTH = 1,\n  parameter INIT = {WIDTH{1'bx}}"));
  EXPECT_NE(std::string::npos, primitiveDump.find("initial Q = INIT;"));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dlatch #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_dffn #("));
  EXPECT_NE(
    std::string::npos,
    primitiveDump.find("module naja_dffn #(\n  parameter WIDTH = 1,\n  parameter INIT = {WIDTH{1'bx}}"));
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
}

TEST_F(SNLVRLDumperTestParameters, testWideMuxInstanceDump) {
  ASSERT_NE(nullptr, createWideMux2Instance());

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_mux2 #("));
  EXPECT_NE(std::string::npos, dumped.find(".WIDTH(8)"));
  EXPECT_EQ(std::string::npos, dumped.find("naja_mux2__w8"));
  EXPECT_EQ(std::string::npos, dumped.find("module naja_mux2 #("));
}

TEST_F(SNLVRLDumperTestParameters, testDivModInstanceAndPrimitiveFileDump) {
  ASSERT_NE(nullptr, createDivModInstance());

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testDivModInstanceAndPrimitiveFileDump";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.dumpDesign(top_, outPath);

  const auto topDump = readTextFile(outPath / "top.v");
  EXPECT_NE(std::string::npos, topDump.find("naja_divmod #("));
  EXPECT_NE(std::string::npos, topDump.find(".SIGNED(1)"));
  EXPECT_NE(std::string::npos, topDump.find(".WIDTH(8)"));
  EXPECT_EQ(std::string::npos, topDump.find("naja_divmod__s_w8"));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_divmod #("));

  const auto primitivePath = outPath / "naja_primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitivePath));
  const auto primitiveDump = readTextFile(primitivePath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_divmod #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("assign Q = $signed(A) / $signed(B)"));
  EXPECT_EQ(std::string::npos, primitiveDump.find("naja_divmod__s_w8"));
}

TEST_F(SNLVRLDumperTestParameters, testShiftModelDefaultsAndInstanceNames) {
  for (const auto& signature: std::vector<NLDB0::ShiftSignature>{
      {1, 1, false, false}, {9, 4, false, false},
      {9, 4, true, false}, {9, 4, true, true}}) {
    auto* model = NLDB0::getOrCreateShift(signature);
    auto* data = SNLBusNet::create(top_, signature.width - 1, 0, NLName("data"));
    auto* count = SNLBusNet::create(top_, signature.countWidth - 1, 0, NLName("count"));
    auto* output = SNLBusNet::create(top_, signature.width - 1, 0, NLName("result"));
    for (bool named: {false, true}) {
      auto* instance = SNLInstance::create(top_, model, named ? NLName("shift") : NLName());
      ASSERT_TRUE(instance->getInstParameters().empty());
      instance->setTermNet(NLDB0::getShiftData(model), data);
      instance->setTermNet(NLDB0::getShiftCount(model), count);
      instance->setTermNet(NLDB0::getShiftOutput(model), output);
      std::ostringstream out;
      SNLVRLDumper dumper;
      dumper.dumpDesign(top_, out);
      const auto dumped = out.str();
      const auto instanceName = named ? "shift" : "instance_" + std::to_string(instance->getID());
      const auto declaration = "naja_shift #(.WIDTH(" + std::to_string(signature.width) +
        "), .COUNT_WIDTH(" + std::to_string(signature.countWidth) + "), .RIGHT(" +
        std::to_string(signature.right) + "), .ARITHMETIC(" +
        std::to_string(signature.arithmetic) + ")) " + instanceName;
      EXPECT_NE(std::string::npos, dumped.find(declaration)) << dumped;
      EXPECT_NE(std::string::npos, dumped.find(".DATA(data)"));
      EXPECT_NE(std::string::npos, dumped.find(".COUNT(count)"));
      EXPECT_NE(std::string::npos, dumped.find(".Y(result)"));
      EXPECT_EQ(std::string::npos, dumped.find(model->getName().getString()));
      instance->destroy();
    }
    data->destroy();
    count->destroy();
    output->destroy();
  }
}

TEST_F(SNLVRLDumperTestParameters, testShiftPrimitiveFileDump) {
  auto* model = NLDB0::getOrCreateShift({9, 4, true, true});
  SNLInstance::create(top_, model, NLName("shift"));
  const auto outPath = std::filesystem::path(SNL_VRL_DUMPER_TEST_PATH) / "testShiftPrimitiveFileDump";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setTopFileName("top.v");
  dumper.dumpDesign(top_, outPath);

  const auto topDump = readTextFile(outPath / "top.v");
  EXPECT_NE(std::string::npos, topDump.find("naja_shift #(.WIDTH(9), .COUNT_WIDTH(4), .RIGHT(1), .ARITHMETIC(1)) shift"));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_shift #("));
  const auto primitivePath = outPath / "naja_primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitivePath));
  const auto primitiveDump = readTextFile(primitivePath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_shift #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("assign Y = $signed(DATA) >>> COUNT;"));
  EXPECT_NE(std::string::npos, primitiveDump.find("assign Y = DATA >> COUNT;"));
  EXPECT_NE(std::string::npos, primitiveDump.find("assign Y = DATA << COUNT;"));
}

TEST_F(SNLVRLDumperTestParameters, testDivModModelDefaultsAndInstanceOverrides) {
  for (auto width: {1u, 8u, 40u}) {
    for (bool isSigned: {false, true}) {
      NLDB0::DivModSignature signature;
      signature.width = width;
      signature.isSigned = isSigned;
      auto* model = NLDB0::getOrCreateDivMod(signature);
      auto* instance = SNLInstance::create(top_, model, NLName("divmod"));
      ASSERT_TRUE(instance->getInstParameters().empty());
      for (bool overrideParameters: {false, true}) {
        if (overrideParameters) {
          SNLInstParameter::create(instance, model->getParameter(NLName("WIDTH")),
            std::to_string(width + 1));
          SNLInstParameter::create(instance, model->getParameter(NLName("SIGNED")),
            isSigned ? "0" : "1");
        }
        std::ostringstream out;
        SNLVRLDumper dumper;
        dumper.dumpDesign(top_, out);
        const auto dumped = out.str();
        const bool emittedSignedness = isSigned != overrideParameters;
        const auto emittedWidth = overrideParameters ? width + 1 : width;
        if (emittedWidth != 1 || emittedSignedness) {
          if (emittedWidth != 1) {
            EXPECT_NE(std::string::npos, dumped.find(".WIDTH(" + std::to_string(emittedWidth) + ")"));
          } else {
            EXPECT_EQ(std::string::npos, dumped.find(".WIDTH("));
          }
          if (emittedSignedness) {
            EXPECT_NE(std::string::npos, dumped.find(".SIGNED(1)"));
          } else {
            EXPECT_EQ(std::string::npos, dumped.find(".SIGNED("));
          }
        } else {
          EXPECT_NE(std::string::npos, dumped.find("naja_divmod divmod"));
        }
      }
      instance->destroy();
    }
  }
}

TEST_F(SNLVRLDumperTestParameters, testMemoryAndTableSelectModelDefaults) {
  for (bool memory: {true, false}) {
    auto* instance = memory ? createMemoryInstance() : createTableSelectInstance();
    ASSERT_NE(nullptr, instance);
    std::vector<SNLInstParameter*> overrides;
    for (auto* parameter: instance->getInstParameters()) {
      overrides.push_back(parameter);
    }
    for (auto* parameter: overrides) {
      parameter->destroy();
    }
    ASSERT_TRUE(instance->getInstParameters().empty());
    std::ostringstream out;
    SNLVRLDumper dumper;
    dumper.dumpDesign(top_, out);
    const auto dumped = out.str();
    EXPECT_NE(std::string::npos, dumped.find(memory ? ".WIDTH(8)" : ".WIDTH(4)"));
    EXPECT_NE(std::string::npos, dumped.find(memory ? ".DEPTH(16)" : ".DEPTH(3)"));
    EXPECT_NE(std::string::npos, dumped.find(memory ? ".ABITS(4)" : ".ABITS(2)"));
    if (memory) {
      EXPECT_NE(std::string::npos, dumped.find(".RD_PORTS(2)"));
      EXPECT_NE(std::string::npos, dumped.find(".WR_PORTS(2)"));
      EXPECT_NE(std::string::npos, dumped.find(".RST_ENABLE(1)"));
      EXPECT_NE(std::string::npos, dumped.find(".RST_ASYNC(1)"));
      EXPECT_EQ(std::string::npos, dumped.find(".RST_ACTIVE_LOW("));
      EXPECT_EQ(std::string::npos, dumped.find(".INIT("));
    }
    // An override matching the generic module default must win over the model.
    SNLInstParameter::create(instance, instance->getModel()->getParameter(NLName("DEPTH")), "1");
    std::ostringstream overridden;
    dumper.dumpDesign(top_, overridden);
    EXPECT_EQ(std::string::npos, overridden.str().find(".DEPTH("));
    instance->destroy();
  }
}

TEST_F(SNLVRLDumperTestParameters, testSequentialModelDefaultsAndInitOverrides) {
  for (auto factory: {
      NLDB0::getOrCreateDFF, NLDB0::getOrCreateDFFN,
      NLDB0::getOrCreateDFFRN, NLDB0::getOrCreateDFFR,
      NLDB0::getOrCreateDFFS, NLDB0::getOrCreateDFFE,
      NLDB0::getOrCreateDFFRE, NLDB0::getOrCreateDFFSE,
      NLDB0::getOrCreateDFFSR, NLDB0::getOrCreateDFFSRN,
      NLDB0::getOrCreateDFFSS, NLDB0::getOrCreateDFFSSN,
      NLDB0::getOrCreateDFFSRE, NLDB0::getOrCreateDFFSRNE,
      NLDB0::getOrCreateDFFSSE, NLDB0::getOrCreateDFFSSNE,
      NLDB0::getOrCreateDLatch}) {
    auto* model = factory(8);
    SCOPED_TRACE(model->getName().getString());
    auto* instance = SNLInstance::create(top_, model, NLName("seq"));
    SNLVRLDumper dumper;
    std::ostringstream defaults;
    dumper.dumpDesign(top_, defaults);
    EXPECT_NE(std::string::npos, defaults.str().find(".WIDTH(8)"));
    EXPECT_EQ(std::string::npos, defaults.str().find(".INIT("));
    if (auto* init = model->getParameter(NLName("INIT"))) {
      init->destroy();
      init = SNLParameter::create(model, NLName("INIT"), SNLParameter::Type::Binary, "8'ha5");
      std::ostringstream initialized;
      dumper.dumpDesign(top_, initialized);
      EXPECT_NE(std::string::npos, initialized.str().find(".INIT(8'ha5)"));
      auto* override = SNLInstParameter::create(instance, init, NLDB0::getUndefinedDFFInitValue(8));
      std::ostringstream undefined;
      dumper.dumpDesign(top_, undefined);
      EXPECT_EQ(std::string::npos, undefined.str().find(".INIT("));
      override->setValue("8'h5a");
      std::ostringstream overridden;
      dumper.dumpDesign(top_, overridden);
      EXPECT_NE(std::string::npos, overridden.str().find(".INIT(8'h5a)"));
    }
    instance->destroy();
  }
}

TEST_F(SNLVRLDumperTestParameters, testTableSelectInstanceDump) {
  ASSERT_NE(nullptr, createTableSelectInstance());

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_table_select #("));
  EXPECT_NE(std::string::npos, dumped.find(") select0 ("));
  EXPECT_NE(std::string::npos, dumped.find(".DATA(select_data)"));
  EXPECT_NE(std::string::npos, dumped.find(".ADDR(select_address)"));
  EXPECT_NE(std::string::npos, dumped.find(".Y(select_y)"));
  EXPECT_EQ(std::string::npos, dumped.find("naja_table_select__("));
  EXPECT_EQ(std::string::npos, dumped.find("module naja_table_select #("));
}

TEST_F(SNLVRLDumperTestParameters, testTableSelectModelDump) {
  ASSERT_NE(nullptr, createTableSelectInstance());
  const auto outPath = std::filesystem::path(SNL_VRL_DUMPER_TEST_PATH) /
    "table_select_model_dump";
  std::filesystem::create_directories(outPath);
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  const auto model = readTextFile(outPath / "naja_primitives.v");
  EXPECT_NE(std::string::npos, model.find("module naja_table_select"));
  EXPECT_NE(std::string::npos, model.find("assign data[row] = DATA[row*WIDTH+bit_index];"));
  EXPECT_NE(std::string::npos, model.find(
    "assign Y[bit_index] = checked_addr < LIMIT ? data[checked_addr[INDEX_WIDTH-1:0]] : 1'bx;"));
}

TEST_F(SNLVRLDumperTestParameters, tableSelectPreservesIndependentColumnDependencies) {
  const auto directory = std::filesystem::path(SNL_VRL_DUMPER_TEST_PATH) / "table_select_columns";
  std::filesystem::create_directories(directory);
  std::vector<SNLScalarNet*> inputs;
  for (const auto* name : {"s", "a", "b", "c"}) {
    auto* net = SNLScalarNet::create(top_, NLName(name));
    SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName(name))->setNet(net);
    inputs.push_back(net);
  }
  auto* y = SNLBusNet::create(top_, 1, 0, NLName("y"));
  SNLBusTerm::create(top_, SNLTerm::Direction::Output, 1, 0, NLName("y"))->setNet(y);
  auto* model = NLDB0::getOrCreateTableSelect({2, 2, 1});
  auto* instance = SNLInstance::create(top_, model, NLName("select"));
  for (const auto& [name, value] : std::vector<std::pair<std::string, std::string>>{
      {"WIDTH", "2"}, {"DEPTH", "2"}, {"ABITS", "1"}})
    SNLInstParameter::create(instance, model->getParameter(NLName(name)), value);
  // Column zero depends only on primary inputs; column one may read column
  // zero. The graph is acyclic even though Y feeds one position of DATA.
  const std::vector<SNLBitNet*> data{inputs[1], y->getBit(0), inputs[3], inputs[2]};
  for (size_t bit = 0; bit < data.size(); ++bit)
    instance->getInstTerm(NLDB0::getTableSelectData(model)->getBit(bit))->setNet(data[bit]);
  instance->setTermNet(NLDB0::getTableSelectAddress(model), inputs[0]);
  instance->setTermNet(NLDB0::getTableSelectOutput(model), y);
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, directory);
  const auto quote = [](const std::filesystem::path& p) { return "\"" + p.string() + "\""; };
  const auto modelPath = directory / "naja_primitives.v";
  if (!std::system("command -v verilator >/dev/null 2>&1")) {
    const auto log = directory / "lint.log";
    const auto lint = "verilator --lint-only --sv --top-module top " + quote(directory / "top.v") +
      " " + quote(modelPath) + " > " + quote(log) + " 2>&1";
    EXPECT_EQ(0, std::system(lint.c_str())) << readTextFile(log);
    // A genuine bit-level loop must still be diagnosed.
    const auto loop = directory / "loop.v";
    std::ofstream file(loop);
    file << "module loop(input s, output y);\n"
         << "naja_table_select #(.WIDTH(1),.DEPTH(2),.ABITS(1)) u(.DATA({~y,~y}),.ADDR(s),.Y(y));\n"
         << "endmodule\n";
    file.close();
    const auto loopLint = "verilator --lint-only --sv --top-module loop " + quote(loop) + " " +
      quote(modelPath) + " > " + quote(log) + " 2>&1";
    EXPECT_NE(0, std::system(loopLint.c_str()));
    EXPECT_NE(std::string::npos, readTextFile(log).find("%Warning-UNOPTFLAT:"));
  }
  if (std::system("command -v iverilog >/dev/null 2>&1") ||
      std::system("command -v vvp >/dev/null 2>&1"))
    GTEST_SKIP() << "Icarus Verilog is required for four-state verification";
  const auto tbPath = directory / "tb.v";
  std::ofstream tb(tbPath);
  tb << R"(module tb;
reg [3:0] values; wire [1:0] y; reg [1:0] expected; integer i,k;
top dut(values[0],values[1],values[2],values[3],y);
initial begin
  for (i=0;i<256;i=i+1) begin
    for (k=0;k<4;k=k+1) begin
      case((i >> (2*k)) & 3)
        0: values[k]=0; 1: values[k]=1; 2: values[k]=1'bx; 3: values[k]=1'bz;
      endcase
    end
    expected='x;
    if (values[0] === 1'b0) begin expected[0]=values[1]; expected[1]=values[1]; end
    if (values[0] === 1'b1) begin expected[0]=values[3]; expected[1]=values[2]; end
    #1;
    if (y !== expected) $fatal(1,"column dependency mismatch inputs=%b actual=%b expected=%b",values,y,expected);
  end
  $finish;
end
endmodule
)";
  tb.close();
  const auto simulation = directory / "sim.vvp";
  const auto command = "iverilog -g2012 -s tb -o " + quote(simulation) + " " + quote(tbPath) +
    " " + quote(directory / "top.v") + " " + quote(modelPath);
  ASSERT_EQ(0, std::system(command.c_str()));
  ASSERT_EQ(0, std::system(("vvp " + quote(simulation)).c_str()));
}

TEST_F(SNLVRLDumperTestParameters, testRTLInfoCompactAttributesDumpExtraInfos) {
  ASSERT_TRUE(top_);
  auto* infos = SNLRTLInfos::create(top_);
  infos->setSourceLoc({NLName("top.sv"), 10, 12, 3, 9});
  infos->setInfo(NLName("custom_info"), "value");

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("(* naja_sv_src=\"top.sv:10:3-12:9\" *)"));
  EXPECT_NE(std::string::npos, dumped.find("(* custom_info=\"value\" *)"));
}

TEST_F(SNLVRLDumperTestParameters, testWideMuxMixedConstBusInputDump) {
  ASSERT_NE(nullptr, createMixedConstBusMux2Instance());

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_mux2 #("));
  EXPECT_NE(std::string::npos, dumped.find(".WIDTH(4)"));
  EXPECT_NE(std::string::npos, dumped.find(".A({2'b10, source_bus})")) << dumped;
}

TEST_F(SNLVRLDumperTestParameters, testNibbleAlignedConstantBusInputDump) {
  ASSERT_NE(nullptr, createWideMux2Instance());
  auto* a = top_->getBusNet(NLName("mux_a"));
  ASSERT_NE(nullptr, a);
  a->setType(SNLNet::Type::Assign0);

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find(".A(8'h00)")) << dumped;
  EXPECT_EQ(std::string::npos, dumped.find(".A(8'h000)")) << dumped;
}

TEST_F(SNLVRLDumperTestParameters, testUnitWidthMuxInstanceDumpOmitsDefaultWidth) {
  ASSERT_NE(nullptr, createWideMux2Instance(1));

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_mux2 mux0 ("));
  EXPECT_EQ(std::string::npos, dumped.find("naja_mux2 #("));
  EXPECT_EQ(std::string::npos, dumped.find(".WIDTH(1)"));
  EXPECT_EQ(std::string::npos, dumped.find("naja_mux2__w1"));
}

TEST_F(SNLVRLDumperTestParameters, testUnitWidthMuxExplicitDefaultWidthParameterIsOmitted) {
  auto* ins = createWideMux2Instance(1);
  ASSERT_NE(nullptr, ins);
  auto* widthParam = ins->getModel()->getParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  SNLInstParameter::create(ins, widthParam, "1");

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_mux2 mux0 ("));
  EXPECT_EQ(std::string::npos, dumped.find("naja_mux2 #("));
  EXPECT_EQ(std::string::npos, dumped.find(".WIDTH(1)"));
  EXPECT_EQ(std::string::npos, dumped.find("naja_mux2__w1"));
}

TEST_F(SNLVRLDumperTestParameters, testWideMuxAndMemoryPrimitiveFileDump) {
  ASSERT_NE(nullptr, createWideMux2Instance());
  ASSERT_NE(nullptr, createMemoryInstance());

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testWideMuxAndMemoryPrimitiveFileDump";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.dumpDesign(top_, outPath);

  const auto topDump = readTextFile(outPath / "top.v");
  EXPECT_NE(std::string::npos, topDump.find("naja_mux2 #("));
  EXPECT_NE(std::string::npos, topDump.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, topDump.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, topDump.find("naja_mux2__w8"));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_mux2 #("));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_mem #("));

  const auto primitiveDump = readTextFile(outPath / "naja_primitives.v");
  auto countSubstring = [](const std::string& text, const std::string& needle) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
      ++count;
      pos += needle.size();
    }
    return count;
  };
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_mux2 #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_mem #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_fa("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dff #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dlatch #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffn #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffrn #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffe #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffre #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffse #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffsr #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffsrn #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffss #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffssn #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffsre #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffsrne #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffsse #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_dffssne #("));
  EXPECT_EQ(1u, countSubstring(primitiveDump, "module naja_divmod #("));
}

TEST_F(SNLVRLDumperTestParameters, requiredParameterDeclarationRejected) {
  SNLParameter::create(top_, NLName("WIDTH"), SNLParameter::Type::Decimal);
  SNLVRLDumper dumper;
  std::ostringstream out;
  EXPECT_THROW(dumper.dumpDesign(top_, out), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestParameters, requiredInstanceParameter) {
  auto parameter = SNLParameter::create(model_, NLName("TEXT"), SNLParameter::Type::String);
  auto instance = SNLInstance::create(top_, model_, NLName("inst"));
  SNLVRLDumper dumper;
  dumper.setDumpHierarchy(false);
  std::ostringstream missing;
  EXPECT_THROW(dumper.dumpDesign(top_, missing), SNLVRLDumperException);
  SNLInstParameter::create(instance, parameter, "");
  std::ostringstream out;
  EXPECT_NO_THROW(dumper.dumpDesign(top_, out));
  EXPECT_NE(std::string::npos, out.str().find(".TEXT(\"\")"));
}

TEST_F(SNLVRLDumperTestParameters, testVerilatorSplitPackedSignalsScope) {
  auto* childInput = SNLBusTerm::create(model_, SNLTerm::Direction::Input, 3, 0, NLName("data_i"));
  SNLBusTerm::create(model_, SNLTerm::Direction::Output, 0, 3, NLName("data_o"));
  SNLScalarTerm::create(model_, SNLTerm::Direction::Input, NLName("enable"));
  SNLBusTerm::create(model_, SNLTerm::Direction::Input, 4, 4, NLName("single_i"));
  SNLBusTerm::create(model_, SNLTerm::Direction::Output, -2, -2, NLName("single_o"));
  SNLBusNet::create(model_, 0, 0, NLName("single_bus"));
  SNLBusNet::create(model_, 3, 0, NLName("internal_bus"));
  auto* input = SNLBusTerm::create(top_, SNLTerm::Direction::Input, 3, 0, NLName("data_i"));
  auto* bus = SNLBusNet::create(top_, 3, 0, NLName("top_bus"));
  input->setNet(bus);
  auto* instance = SNLInstance::create(top_, model_, NLName("child"));
  instance->setTermNet(childInput, bus);
  SNLVRLDumper dumper;
  std::ostringstream plain;
  dumper.dumpDesign(top_, plain);
  EXPECT_EQ(std::string::npos, plain.str().find("split_var"));
  SNLVRLDumper::Configuration config;
  config.setVerilatorSplitPackedSignals(true);
  dumper.setConfiguration(config);
  std::ostringstream annotated;
  dumper.dumpDesign(top_, annotated);
  const auto text = annotated.str();
  EXPECT_NE(std::string::npos, text.find("input [3:0] data_i /* verilator split_var */"));
  EXPECT_NE(std::string::npos, text.find("output [0:3] data_o /* verilator split_var */"));
  EXPECT_NE(std::string::npos, text.find("wire [3:0] internal_bus /* verilator split_var */;"));
  EXPECT_NE(std::string::npos, text.find("wire [3:0] top_bus /* verilator split_var */;"));
  const auto topStart = text.find("module top(");
  ASSERT_NE(std::string::npos, topStart);
  EXPECT_EQ(std::string::npos, text.substr(topStart, text.find(");", topStart) - topStart).find("split_var"));
  EXPECT_EQ(std::string::npos, text.find("enable /* verilator split_var */"));
  EXPECT_NE(std::string::npos, text.find("input [4:4] single_i"));
  EXPECT_NE(std::string::npos, text.find("output [-2:-2] single_o"));
  EXPECT_NE(std::string::npos, text.find("wire [0:0] single_bus;"));
  for (const auto* name : {"single_i", "single_o", "single_bus"}) {
    EXPECT_EQ(std::string::npos, text.find(std::string(name) + " /* verilator split_var */"));
  }
  const auto directory = std::filesystem::path(SNL_VRL_DUMPER_TEST_PATH) / "split_packed_multi_file";
  std::filesystem::create_directories(directory);
  dumper.setSingleFile(false);
  dumper.dumpDesign(top_, directory);
  EXPECT_NE(std::string::npos, readTextFile(directory / "model.v").find(
    "data_i /* verilator split_var */"));
  const auto multiTop = readTextFile(directory / "top.v");
  EXPECT_EQ(std::string::npos, multiTop.substr(0, multiTop.find(");")).find("split_var"));
  std::ostringstream libraryDump;
  dumper.dumpLibrary(top_->getLibrary(), libraryDump);
  EXPECT_EQ(std::string::npos, libraryDump.str().find("data_i /* verilator split_var */"));
  std::ostringstream reused;
  dumper.dumpDesign(model_, reused);
  EXPECT_EQ(std::string::npos, reused.str().substr(0, reused.str().find(");")).find("split_var"));
}
