// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLEquipotential.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLRTLPrimitives.h"
#include "SNLRTLInfos.h"
#include "SNLVRLDumper.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLSVConstructor.h"
#include "VHDLConstructor.h"
#include "VHDLTestUtils.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace naja::NL;

class VHDLConstructorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto* db = NLDB::create(NLUniverse::create());
    library_ = NLLibrary::create(db, NLLibrary::Type::Standard);
  }
  void TearDown() override { NLUniverse::get()->destroy(); }

  NLLibrary* library_ {};
};

namespace {
using naja::NL::test::evaluateRTL;
using naja::NL::test::dffBits;

SNLBitNet* assignSource(SNLBitNet* net) {
  EXPECT_FALSE(net->isConstant());
  for (auto* term : net->getInstTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Output &&
        NLDB0::isAssign(term->getInstance()->getModel())) {
      return term->getInstance()->getInstTerm(NLDB0::getAssignInput())->getNet();
    }
  }
  ADD_FAILURE() << "expected an assign driver";
  return net;
}

std::string lfsrSource(unsigned width) {
  return "library ieee; use ieee.std_logic_1164.all; "
      "use ieee.numeric_std.all; use ieee.std_logic_unsigned.all; "
      "entity lfsr is generic(n : positive := " + std::to_string(width) + R"();
  port(clk, rst, ena : in std_logic; output : out std_logic_vector(n-1 downto 0)); end;
architecture rtl of lfsr is
  type tap_table is array (32 downto 2) of std_logic_vector(31 downto 0);
  signal taps : tap_table;
  signal temp : std_logic_vector(n-1 downto 0);
begin
  taps(n) <= "00000000001000000000000000000011";
  process(clk, rst, temp)
    variable feedback : std_logic;
  begin
    if rising_edge(clk) then
      if rst = '1' then
        temp <= (others => '1'); feedback := '1';
      elsif ena = '1' then
        feedback := temp(0);
        for i in n-2 downto 0 loop
          if taps(n)(i) = '1' then temp(i) <= temp(i+1) XOR feedback;
          else temp(i) <= temp(i+1); end if;
        end loop;
        temp(n-1) <= feedback;
      end if;
    end if;
    output <= temp;
  end process;
end;
)";
}
}

TEST_F(VHDLConstructorTest, RetainedIntentPropertyDescriptions) {
  library_->setName(NLName("DESIGN"));
  VHDLConstructor::ConstructOptions options;
  options.keepASTLink = true;
  options.diagnosticsReportPath = std::nullopt;
  auto* top = VHDLConstructor(library_, options).construct(R"(
    entity top is port(a : in bit; y : out bit); end;
    architecture rtl of top is begin y <= not a; end;
  )");
  ASSERT_NE(nullptr, top);
  auto* session = library_->getDB()->getProperty("VHDLIntentSession");
  ASSERT_NE(nullptr, session);
  EXPECT_EQ("VHDLIntentSession", session->getString());
  auto* record = top->getProperty("VHDLIntentRecord");
  ASSERT_NE(nullptr, record);
  EXPECT_EQ("VHDLIntentRecord", record->getString());
}

TEST_F(VHDLConstructorTest, RepeatedMuxFalseSelectionsUseAnd) {
  auto* top = VHDLConstructor(library_).construct(R"(
    entity top is port(a,b : in bit_vector(2 downto 0); s,t : in bit;
                       y : out bit_vector(2 downto 0)); end;
    architecture rtl of top is begin
      process(a,b,s,t) begin
        if s='1' then
          if t='1' then y<=a; else y<=b; end if;
        else y<=b; end if;
      end process;
    end;
  )");
  size_t muxes = 0;
  for (auto* instance : top->getInstances()) if (NLDB0::isMux2(instance->getModel())) ++muxes;
  EXPECT_EQ(1u, muxes);
  for (unsigned input = 0; input < 256; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    for (unsigned bit = 0; bit < 3; ++bit) {
      values[top->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (input >> bit) & 1;
      values[top->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (input >> (bit + 3)) & 1;
    }
    values[top->getScalarTerm(NLName("s"))->getNet()] = (input >> 6) & 1;
    values[top->getScalarTerm(NLName("t"))->getNet()] = (input >> 7) & 1;
    const unsigned expected = (input & 192) == 192 ? input & 7 : (input >> 3) & 7;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 3; ++bit) {
      EXPECT_EQ(bool((expected >> bit) & 1),
        evaluateRTL(top->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting));
    }
  }
}

TEST_F(VHDLConstructorTest, RepeatedMuxSelectionsAreCombined) {
  auto* top = VHDLConstructor(library_).construct(R"(
    entity top is port(a,b : in bit_vector(2 downto 0); s,t,u : in bit;
                       y : out bit_vector(2 downto 0)); end;
    architecture rtl of top is begin
      process(a,b,s,t,u) is variable temp : bit_vector(2 downto 0);
      begin
        temp:=a;
        if s='1' then temp:=b; end if;
        if t='1' then temp:=b; end if;
        if u='1' then temp:=b; end if;
        y<=temp;
      end process;
    end;
  )");
  size_t muxes = 0;
  for (auto* instance : top->getInstances()) if (NLDB0::isMux2(instance->getModel())) ++muxes;
  EXPECT_EQ(1u, muxes);
  for (unsigned input = 0; input < 512; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    for (unsigned bit = 0; bit < 3; ++bit) {
      values[top->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (input >> bit) & 1;
      values[top->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (input >> (bit + 3)) & 1;
    }
    for (const auto& pin : {std::pair{"s", 6}, std::pair{"t", 7}, std::pair{"u", 8}})
      values[top->getScalarTerm(NLName(pin.first))->getNet()] = (input >> pin.second) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    const unsigned expected = input & 448 ? (input >> 3) & 7 : input & 7;
    for (unsigned bit = 0; bit < 3; ++bit) {
      EXPECT_EQ(bool((expected >> bit) & 1),
        evaluateRTL(top->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting));
    }
  }
}

TEST_F(VHDLConstructorTest, OverwrittenMuxResultsPreserveValues) {
  auto* top = VHDLConstructor(library_).construct(R"(
    entity top is port(a,b : in bit_vector(3 downto 0); s : in bit;
                       y,z : out bit_vector(3 downto 0)); end;
    architecture rtl of top is begin
      process(a,b,s) is
        variable temp : bit_vector(3 downto 0);
      begin
        if s = '1' then temp := a; else temp := b; end if;
        temp := a;
        y <= temp;
        if s = '1' then z <= b; else z <= a; end if;
      end process;
    end;
  )");
  for (unsigned input = 0; input < 512; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    for (unsigned bit = 0; bit < 4; ++bit) {
      values[top->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (input >> bit) & 1;
      values[top->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (input >> (bit + 4)) & 1;
    }
    values[top->getScalarTerm(NLName("s"))->getNet()] = (input >> 8) & 1;
    for (const auto& value : {std::pair{"y", input & 15},
                             std::pair{"z", input & 256 ? (input >> 4) & 15 : input & 15}}) {
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned bit = 0; bit < 4; ++bit) {
        EXPECT_EQ(bool((value.second >> bit) & 1),
          evaluateRTL(top->getBusTerm(NLName(value.first))->getBit(bit)->getNet(), values, visiting));
      }
    }
  }
}

TEST_F(VHDLConstructorTest, WiringAssignmentsTraverseEquipotentials) {
  const std::vector<std::string> sources = {
    "entity wiring is port(a : in bit; y : out bit); end; "
    "architecture rtl of wiring is begin y <= a; end;",
    "entity wiring is port(a : in bit_vector(3 downto 0); "
    "y : out bit_vector(0 to 3)); end; "
    "architecture rtl of wiring is begin y <= a; end;",
    "library ieee; use ieee.std_logic_1164.all; "
    "entity wiring is port(a : in std_logic_vector(3 downto 0); "
    "y : out std_logic_vector(0 to 1)); end; "
    "architecture rtl of wiring is signal temp : std_logic_vector(1 downto 0); "
    "begin temp <= a(2 downto 1); y <= temp; end;",
    "library ieee; use ieee.std_logic_1164.all; "
    "entity wiring is port(a : in std_logic; y : out std_logic); end; "
    "architecture rtl of wiring is begin process(all) begin y <= a; end process; end;"
  };
  for (const auto& source : sources) {
    SCOPED_TRACE(source);
    VHDLConstructor constructor(library_);
    auto* design = constructor.construct(source);
    ASSERT_NE(design, nullptr);
    for (auto* instance : design->getInstances()) EXPECT_TRUE(instance->getModel()->isAssign());
    for (auto* output : design->getBitTerms()) {
      if (output->getDirection() != SNLTerm::Direction::Output) continue;
      SNLEquipotential standard(output);
      EXPECT_EQ(standard.getTermsSet().size(), 1);
      SNLEquipotential traversed(output, SNLEquipotential::Mode::TraverseAssigns);
      EXPECT_EQ(traversed.getTermsSet().size(), 2);
      EXPECT_TRUE(traversed.getInstTermOccurrencesSet().empty());
      auto* input = *traversed.getTermsSet().begin();
      if (input == output) input = *traversed.getTermsSet().rbegin();
      EXPECT_EQ(input->getDirection(), SNLTerm::Direction::Input);
      if (auto* bit = dynamic_cast<SNLBusTermBit*>(output)) {
        const auto expected = source.find("temp") != std::string::npos
            ? 2 - bit->getBit() : 3 - bit->getBit();
        ASSERT_NE(dynamic_cast<SNLBusTermBit*>(input), nullptr);
        EXPECT_EQ(static_cast<SNLBusTermBit*>(input)->getBit(), expected);
      }
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, FourStateLogicLiteralsPreserveValuesAndPositions) {
  const std::vector<std::pair<std::string, std::string>> literals{
      {"\"10XZ\"", "10XZ"}, {"B\"1_0_X_Z\"", "10XZ"},
      {"X\"XZ\"", "XXXXZZZZ"}, {"O\"Z1\"", "ZZZ001"},
      {"(others => 'X')", "XXXX"}, {"'Z' & \"01\" & 'X'", "Z01X"}};
  for (const auto& [literal, expected] : literals) for (const bool ascending : {false, true}) {
    SCOPED_TRACE(literal);
    const auto range = ascending ? "2 to " + std::to_string(expected.size()+1) : std::to_string(expected.size()+1) + " downto 2";
    const auto source = std::string("library ieee; use ieee.std_logic_1164.all; ") +
        "entity values is port(y : out std_logic_vector(" + range + ")); end; "
        "architecture rtl of values is begin y <= " + literal + "; end;";
    auto* design = VHDLConstructor(library_).construct(source);
    auto* output = design->getBusTerm(NLName("y"));
    for (size_t position = 0; position < expected.size(); ++position) {
      auto* bit = output->getBit(ascending ? 2+position : expected.size()+1-position);
      auto* source = assignSource(bit->getNet());
      EXPECT_EQ(source->isConstant0(), expected[position] == '0');
      EXPECT_EQ(source->isConstant1(), expected[position] == '1');
      EXPECT_EQ(source->isConstantX(), expected[position] == 'X');
      EXPECT_EQ(source->isConstantZ(), expected[position] == 'Z');
      SNLEquipotential net(bit, SNLEquipotential::Mode::TraverseAssigns);
      EXPECT_EQ(net.isConst0(), expected[position] == '0');
      EXPECT_EQ(net.isConst1(), expected[position] == '1');
      EXPECT_EQ(net.isConstX(), expected[position] == 'X');
      EXPECT_EQ(net.isConstZ(), expected[position] == 'Z');
    }
    design->destroy();
  }
  // Scalar assignments also select the RTL path without an aggregate or process.
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity scalar_values is port(x,z : out std_logic); end;
architecture rtl of scalar_values is begin x <= 'X'; z <= 'Z'; end;
)");
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("x"))->getNet())->isConstantX());
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("z"))->getNet())->isConstantZ());
}

TEST_F(VHDLConstructorTest, ConstantPortBitsHaveAssignDrivers) {
  auto* design = VHDLConstructor(library_).construct(
      std::string("library ieee; use ieee.std_logic_1164.all; ") +
      "entity values is port(a : in std_logic; y : out std_logic_vector(5 downto 0)); end; "
      "architecture rtl of values is begin y <= \"01XZ\" & a & '1'; end;");
  auto* output = design->getBusTerm(NLName("y"));
  EXPECT_EQ(design->getInstances().size(), 6);
  EXPECT_TRUE(assignSource(output->getBit(5)->getNet())->isConstant0());
  auto* one = assignSource(output->getBit(4)->getNet());
  EXPECT_TRUE(one->isConstant1());
  EXPECT_TRUE(assignSource(output->getBit(3)->getNet())->isConstantX());
  EXPECT_TRUE(assignSource(output->getBit(2)->getNet())->isConstantZ());
  EXPECT_EQ(assignSource(output->getBit(1)->getNet()),
      design->getScalarTerm(NLName("a"))->getNet());
  EXPECT_EQ(assignSource(output->getBit(0)->getNet()), one);
  design->destroy();
  // Scalar binary literals also exercise the simple constructor path.
  for (const auto* value : {"'0'", "'1'"}) {
    auto* design = VHDLConstructor(library_).construct(
        std::string("entity scalar_value is port(y : out bit); end; ") +
        "architecture rtl of scalar_value is begin y <= " + value + "; end;");
    auto* source = assignSource(design->getScalarTerm(NLName("y"))->getNet());
    EXPECT_EQ(source->isConstant1(), std::string(value) == "'1'");
    EXPECT_EQ(source->isConstant0(), std::string(value) == "'0'");
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, HierarchicalConstantOutputsHaveAssignDrivers) {
  auto* top = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity const_src is
  port(clk : in std_ulogic; data : out std_ulogic_vector(3 downto 0)); end;
architecture rtl of const_src is begin data <= (others => '1'); end;
library ieee; use ieee.std_logic_1164.all;
entity top is port(clk : in std_ulogic; a : in std_ulogic_vector(3 downto 0);
  q : out std_ulogic_vector(3 downto 0)); end;
architecture rtl of top is signal c : std_ulogic_vector(3 downto 0);
begin
  src : entity work.const_src port map(clk, c);
  q <= a xor c;
end;
)", "top");
  auto* src = top->getInstance(NLName("src"));
  ASSERT_NE(src, nullptr);
  EXPECT_EQ(src->getModel()->getInstances().size(), 4);
  for (auto* instance : src->getModel()->getInstances())
    EXPECT_TRUE(NLDB0::isAssign(instance->getModel()));
  for (auto* bit : src->getModel()->getBusTerm(NLName("data"))->getBits()) {
    EXPECT_TRUE(assignSource(bit->getNet())->isConstant1());
    SNLEquipotential traversed(src->getInstTerm(bit),
        SNLEquipotential::Mode::TraverseAssigns);
    EXPECT_TRUE(traversed.isConst1());
  }
}

TEST_F(VHDLConstructorTest, FourStateAssignmentsPreserveMuxAndRegisterData) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity choices is port(clk : in std_logic; sel : in bit; y,q : out std_logic_vector(1 downto 0)); end;
architecture rtl of choices is
  constant fallback : std_logic_vector(1 downto 0) := "XZ";
begin
  process(all) begin
    case sel is
      when '0' => y <= "10";
      when others => y <= fallback;
    end case;
  end process;
  process(clk) begin if rising_edge(clk) then q <= fallback; end if; end process;
end;
)");
  size_t muxes = 0;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (!NLDB0::isMux2(model)) continue;
    ++muxes;
    size_t x = 0, z = 0;
    for (auto* term : instance->getInstTerms()) if (term->getDirection() == SNLTerm::Direction::Input) {
      x += term->getNet()->isConstantX();
      z += term->getNet()->isConstantZ();
    }
    EXPECT_EQ(x, 1); EXPECT_EQ(z, 1);
  }
  EXPECT_EQ(muxes, 1);
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 2);
  size_t x = 0, z = 0;
  for (const auto& flop : flops) { x += flop.data->isConstantX(); z += flop.data->isConstantZ(); }
  EXPECT_EQ(x, 1); EXPECT_EQ(z, 1);
  std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("sel"))->getNet(), false}};
  std::unordered_set<SNLBitNet*> visiting;
  EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(1)->getNet(), values, visiting));
  EXPECT_FALSE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(0)->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, RejectsUnsupportedLogicLiteralValuesAndUses) {
  for (const auto* type : {"bit", "std_logic"}) for (const auto* value : {"'X'", "'Z'", "'U'", "'W'", "'L'", "'H'", "'-'", "'x'"}) {
    if (std::string(type) == "std_logic" && (std::string(value) == "'X'" || std::string(value) == "'Z'")) continue;
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("library ieee; use ieee.std_logic_1164.all; ") +
        "entity invalid is port(y : out " + type + "); end; architecture rtl of invalid is begin y <= " + value + "; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  for (const auto* value : {"X\"xZ\"", "B\"10zX\""}) {
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("library ieee; use ieee.std_logic_1164.all; ") +
        "entity invalid is port(y : out std_logic_vector(7 downto 0)); end; architecture rtl of invalid is begin y <= " + value + "; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  for (const auto* expression : {"a = 'X'", "a /= 'Z'", "unsigned(\"X0\") + 1 = 0"}) {
    try {
      VHDLConstructor(library_).construct(std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
        "entity invalid is port(a : in std_logic; y : out bit); end; architecture rtl of invalid is begin "
        "y <= '1' when " + expression + " else '0'; end;");
      FAIL() << "expected nonbinary operand rejection";
    } catch (const NLException& exception) {
      EXPECT_NE(std::string(exception.what()).find("explicit X/Z operands"), std::string::npos) << exception.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, FourStateUnresolvedLogicInputPortActuals) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity leaf is port(a : in std_ulogic; b : in std_ulogic_vector(2 to 3); y : out std_ulogic); end;
architecture rtl of leaf is begin y <= a; end;
library ieee; use ieee.std_logic_1164.all;
entity wrapper is port(y : out std_ulogic); end;
architecture rtl of wrapper is begin u : entity work.leaf port map('X', "Z1", y); end;
)", "wrapper");
  auto* instance = design->getInstance(NLName("u"));
  ASSERT_NE(instance, nullptr);
  auto* model = instance->getModel();
  EXPECT_TRUE(instance->getInstTerm(model->getScalarTerm(NLName("a")))->getNet()->isConstantX());
  EXPECT_TRUE(instance->getInstTerm(model->getBusTerm(NLName("b"))->getBit(2))->getNet()->isConstantZ());
  EXPECT_TRUE(instance->getInstTerm(model->getBusTerm(NLName("b"))->getBit(3))->getNet()->isConstant1());
}

TEST_F(VHDLConstructorTest, AssignTraversalStopsAtLogicAndStorage) {
  for (const auto* body : {"y <= not a;", "y <= a when sel = '1' else b;",
      "process(clk) begin if rising_edge(clk) then y <= a; end if; end process;"}) {
    SCOPED_TRACE(body);
    const auto source = std::string("library ieee; use ieee.std_logic_1164.all; "
        "entity boundary is port(a, b, sel, clk : in bit; y : out bit); end; "
        "architecture rtl of boundary is begin ") + body + " end;";
    VHDLConstructor constructor(library_);
    auto* design = constructor.construct(source);
    auto* output = design->getScalarTerm(NLName("y"));
    SNLEquipotential traversed(output, SNLEquipotential::Mode::TraverseAssigns);
    EXPECT_EQ(traversed.getTermsSet().size(), 1);
    ASSERT_EQ(traversed.getInstTermOccurrencesSet().size(), 1);
    auto* term = dynamic_cast<SNLInstTerm*>(
        traversed.getInstTermOccurrencesSet().begin()->getObject());
    ASSERT_NE(term, nullptr);
    EXPECT_FALSE(term->getInstance()->getModel()->isAssign());
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, IndexedLFSRResetEnableAndCycles) {
  for (const unsigned width : {2u, 4u, 9u, 32u}) {
    SCOPED_TRACE(width);
    auto* design = VHDLConstructor(library_).construct(lfsrSource(width));
    auto* state = dynamic_cast<SNLBusNet*>(design->getNet(NLName("temp")));
    ASSERT_NE(state, nullptr);
    const auto flops = dffBits(design);
    ASSERT_EQ(flops.size(), width);
    const uint64_t mask = (uint64_t(1) << width) - 1;
    uint64_t expected = 0;
    for (unsigned cycle = 0; cycle < 160; ++cycle) {
      const bool reset = cycle == 0 || cycle == 53 || cycle == 104;
      const bool enable = cycle % 5 != 0;
      std::unordered_map<SNLBitNet*, bool> values;
      values[design->getScalarTerm(NLName("rst"))->getNet()] = reset;
      values[design->getScalarTerm(NLName("ena"))->getNet()] = enable;
      for (unsigned bit = 0; bit < width; ++bit) values[state->getBit(bit)] = (expected >> bit) & 1;
      std::unordered_map<SNLBitNet*, bool> next;
      std::unordered_set<SNLBitNet*> visiting;
      for (const auto& flop : flops) {
        EXPECT_EQ(flop.clock,
                  design->getScalarTerm(NLName("clk"))->getNet());
        next[flop.output] =
            evaluateRTL(flop.data, values, visiting);
      }
      if (reset) expected = mask;
      else if (enable) expected = (expected >> 1) ^
          ((expected & 1) ? ((uint64_t(1) << (width - 1)) | (0x200003u & (mask >> 1))) : 0);
      for (unsigned bit = 0; bit < width; ++bit) {
        ASSERT_EQ(next.at(state->getBit(bit)), bool((expected >> bit) & 1)) << cycle << ":" << bit;
        ASSERT_EQ(evaluateRTL(design->getBusTerm(NLName("output"))->getBit(bit)->getNet(),
                              next, visiting), bool((expected >> bit) & 1));
      }
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, IndexedAscendingArraysAndScheduledPriority) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity indexed is port(clk, d : in bit; y : out bit_vector(7 downto 5)); end;
architecture rtl of indexed is
  type table_type is array (-2 to 1) of bit_vector(4 to 6);
  signal table_value : table_type;
  signal state : bit_vector(4 to 6);
begin
  table_value(-2) <= "101";
  process(clk, state)
    variable v : bit;
  begin
    if rising_edge(clk) then
      v := d;
      for i in 4 to 6 loop
        state(i) <= table_value(-2)(i) xor v;
      end loop;
      v := not v;
      state(4) <= v;
      state(5) <= state(4);
      for i in 6 to 4 loop state(i) <= '0'; end loop;
    end if;
    y <= state;
  end process;
end;
)");
  auto* state = dynamic_cast<SNLBusNet*>(design->getNet(NLName("state")));
  ASSERT_NE(state, nullptr);
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    values[design->getScalarTerm(NLName("d"))->getNet()] = pattern & 1;
    for (unsigned i = 0; i < 3; ++i) values[state->getBit(4 + i)] = (pattern >> (i + 1)) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    for (const auto& flop : dffBits(design)) {
      auto* output = flop.output;
      const bool expected = output == state->getBit(5) ? bool((pattern >> 1) & 1) : !(pattern & 1);
      EXPECT_EQ(evaluateRTL(flop.data, values, visiting), expected);
    }
    for (unsigned i = 0; i < 3; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(7-i)->getNet(), values, visiting),
                bool((pattern >> (i+1)) & 1));
  }
}

TEST_F(VHDLConstructorTest, IndexedExtendedIdentifiersRemainDistinct) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity indexed is port(\A\, \a\ : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;
architecture rtl of indexed is begin
  y(1) <= \A\(0);
  y(0) <= \a\(1);
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  values[design->getBusTerm(NLName("\\A\\"))->getBit(0)->getNet()] = true;
  values[design->getBusTerm(NLName("\\a\\"))->getBit(1)->getNet()] = false;
  std::unordered_set<SNLBitNet*> visiting;
  EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(1)->getNet(), values, visiting));
  EXPECT_FALSE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(0)->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, InvalidIndexedRTLPublishesNoDesign) {
  const auto source = lfsrSource(4);
  const std::vector<std::pair<std::string, std::string>> replacements{
      {"taps(n)(i)", "taps(33)(i)"},
      {"temp(i+1)", "temp(i+9)"},
      {"temp(i+1)", "temp(ena)"},
      {"feedback := temp(0);", "feedback := absent;"},
      {"process(clk, rst, temp)", "process(clk, rst)"},
      {"temp <= (others => '1')", "temp <= (others => 'U')"},
      {"signal taps : tap_table", "signal taps : missing_type"},
      {"taps(n) <=", "taps(n+1) <="},
      {"temp(n-1) <= feedback", "clk <= feedback"},
      {"temp(n-1) <= feedback", "feedback <= temp(0)"},
      {"if taps(n)(i) = '1'", "if i"},
      {"output <= temp;", "output <= temp; output <= temp;"},
      {"use ieee.std_logic_1164.all;", ""},
      {"n : positive := 4", "n : positive := 0"},
      {"std_logic_vector(n-1 downto 0)", "std_logic_vector(n downto 0)"},
      {"feedback := temp(0);", "feedback := temp;"},
  };
  for (const auto& [before, after] : replacements) {
    SCOPED_TRACE(after);
    auto invalid = source;
    const auto position = invalid.find(before);
    ASSERT_NE(position, std::string::npos);
    invalid.replace(position, before.size(), after);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, LowersConditionalAssignmentThroughSharedMux) {
  constexpr auto source = R"(
entity mux is
  port (a, b, sel : in bit; y : out bit);
end entity mux;
architecture rtl of mux is
begin
  y <= a when sel = '1' else b;
end architecture rtl;
)";

  VHDLConstructor constructor(library_);
  auto* design = constructor.construct(source);
  ASSERT_NE(design, nullptr);

  auto* muxTermA = NLDB0::getMux2InputA();
  auto* muxTermB = NLDB0::getMux2InputB();
  auto* muxSelect = NLDB0::getMux2Select();
  auto* muxOutput = NLDB0::getMux2Output();
  auto instances = design->getInstances();
  ASSERT_EQ(instances.size(), 1);
  auto* instance = *instances.begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getOrCreateMux2(1));
  EXPECT_EQ(instance->getInstTerm(muxTermA->getBit(0))->getNet(),
            design->getScalarTerm(NLName("a"))->getNet());
  EXPECT_EQ(instance->getInstTerm(muxTermB->getBit(0))->getNet(),
            design->getScalarTerm(NLName("b"))->getNet());
  EXPECT_EQ(instance->getInstTerm(muxSelect)->getNet(),
            design->getScalarTerm(NLName("sel"))->getNet());
  EXPECT_EQ(instance->getInstTerm(muxOutput->getBit(0))->getNet(),
            design->getScalarTerm(NLName("y"))->getNet());
}

TEST_F(VHDLConstructorTest, LowersTypedScalarLogicalOperators) {
  const std::vector<std::pair<std::string, NLDB0::GateType::GateTypeEnum>> operators{
      {"and", NLDB0::GateType::And}, {"nand", NLDB0::GateType::Nand},
      {"or", NLDB0::GateType::Or}, {"nor", NLDB0::GateType::Nor},
      {"xor", NLDB0::GateType::Xor}, {"xnor", NLDB0::GateType::Xnor}};
  for (const auto& [op, gateType] : operators) {
    SCOPED_TRACE(op);
    const std::string source =
        "entity logic is port(a, b : in bit; y : out bit); end; "
        "architecture rtl of logic is begin y <= a " + op + " b; end;";
    auto* design = VHDLConstructor(library_).construct(source);
    ASSERT_EQ(design->getInstances().size(), 1);
    EXPECT_EQ((*design->getInstances().begin())->getModel(),
              NLDB0::getOrCreateNInputGate(gateType, 2));
    design->destroy();
  }
  auto* design = VHDLConstructor(library_).construct(
      "entity logic is port(a : in bit; y : out bit); end; "
      "architecture rtl of logic is begin y <= not a; end;");
  ASSERT_EQ(design->getInstances().size(), 1);
  EXPECT_EQ((*design->getInstances().begin())->getModel(),
            NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1));
}

TEST_F(VHDLConstructorTest, PreservesVectorRangesAndPositionalMuxMapping) {
  std::ifstream fixture(SNL_VHDL_VECTORS);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  auto* a = design->getBusTerm(NLName("a"));
  auto* b = design->getBusTerm(NLName("b"));
  auto* y = design->getBusTerm(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(a->getMSB(), 0);
  EXPECT_EQ(a->getLSB(), 3);
  EXPECT_EQ(b->getMSB(), 7);
  EXPECT_EQ(b->getLSB(), 4);
  EXPECT_EQ(y->getMSB(), 3);
  EXPECT_EQ(y->getLSB(), 0);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* mux = *design->getInstances().begin();
  auto* model = NLDB0::getOrCreateMux2(4);
  ASSERT_EQ(mux->getModel(), model);
  for (size_t position = 0; position < 4; ++position) {
    EXPECT_EQ(mux->getInstTerm(
                  NLDB0::getMux2InputA(model)->getBitAtPosition(3 - position))->getNet(),
              a->getBitAtPosition(3 - position)->getNet());
    EXPECT_EQ(mux->getInstTerm(
                  NLDB0::getMux2InputB(model)->getBitAtPosition(3 - position))->getNet(),
              b->getBitAtPosition(3 - position)->getNet());
    EXPECT_EQ(mux->getInstTerm(
                  NLDB0::getMux2Output(model)->getBitAtPosition(3 - position))->getNet(),
              y->getBitAtPosition(3 - position)->getNet());
  }
  if (const auto* path = std::getenv("VHDL_VECTOR_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<std::string> values;
    for (std::string value; reference >> value;)
      values.push_back(value);
    EXPECT_EQ(values, (std::vector<std::string>{"0110", "1001", "0011"}));
  }
}

TEST_F(VHDLConstructorTest, AppliesDefaultGenericToPortRanges) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity generic_wire is
  generic(width : positive := 5);
  port(a : in bit_vector(width-1 downto 0);
       y : out bit_vector(width-1 downto 0));
end;
architecture rtl of generic_wire is begin y <= a; end;
)");
  ASSERT_NE(design, nullptr);
  auto* input = dynamic_cast<SNLBusTerm*>(design->getTerm(NLName("a")));
  auto* output = dynamic_cast<SNLBusTerm*>(design->getTerm(NLName("y")));
  ASSERT_NE(input, nullptr);
  ASSERT_NE(output, nullptr);
  EXPECT_EQ(input->getMSB(), 4);
  EXPECT_EQ(input->getLSB(), 0);
  EXPECT_EQ(output->getMSB(), 4);
  EXPECT_EQ(output->getLSB(), 0);
}

TEST_F(VHDLConstructorTest, ConstructsFromFile) {
  auto* design = VHDLConstructor(library_).constructFile(SNL_VHDL_VECTORS);
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(design->getName(), NLName("vector_mux"));
  EXPECT_THROW(
      VHDLConstructor(library_).constructFile("missing-vhdl-source.vhd"),
      NLException);
}

TEST_F(VHDLConstructorTest, DiagnosticsIncludeSourceLocation) {
  try {
    VHDLConstructor(library_).construct(R"(entity broken is
end entity broken;
architecture rtl of broken is
  signal value : bit;
  value <= '1';
end architecture rtl;
)");
    FAIL() << "expected malformed VHDL to be rejected";
  } catch (const NLException& exception) {
    EXPECT_EQ(exception.getReason(),
        "VHDL constructor: parse failed at line 5, column 3: "
        "expected keyword 'begin'");
  }
}

TEST_F(VHDLConstructorTest, UnusedArrayTypeDeclarationDoesNotBlockConstruction) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity lfsr is port(a : in bit; y : out bit); end;
architecture rtl of lfsr is
    type inner_taps is array (32 downto 2) of bit_vector(31 downto 0);
begin
    y <= a;
end;
)");
  ASSERT_NE(design, nullptr);
  EXPECT_NE(design->getTerm(NLName("a")), nullptr);
  EXPECT_NE(design->getTerm(NLName("y")), nullptr);
}

TEST_F(VHDLConstructorTest, LowersBitwiseVectorExpressionsByPosition) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity vector_logic is port (
  a : in bit_vector(0 to 3);
  b : in bit_vector(7 downto 4);
  y : out bit_vector(3 downto 0));
end;
architecture rtl of vector_logic is begin y <= a xor not b; end;
)");
  std::unordered_map<SNLDesign*, size_t> models;
  for (auto* instance : design->getInstances())
    ++models[instance->getModel()];
  EXPECT_EQ(models[NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1)], 4);
  EXPECT_EQ(models[NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2)], 4);
  EXPECT_EQ(design->getInstances().size(), 8);
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameVectorMuxModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_VECTOR_MUX_SV));
  auto* design = library_->getSNLDesign(NLName("vector_mux_sv"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getInstances().size(), 1);
  EXPECT_EQ((*design->getInstances().begin())->getModel(), NLDB0::getOrCreateMux2(4));
}

TEST_F(VHDLConstructorTest, UnsupportedVectorShapesPublishNoDesign) {
  for (const auto* source : {
      "entity bad_vector is port(a : in bit_vector(0 to 3); "
      "y : out bit_vector(2 downto 0)); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(a : in bit_vector; y : out bit_vector); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(a : in std_logic_vector(3 downto 0); "
      "y : out std_logic_vector(3 downto 0)); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(a : in bit_vector(3 to 0); "
      "y : out bit_vector(3 to 0)); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(clk : in bit; d : in bit_vector(3 downto 0); "
      "q : out bit_vector(3 downto 0)); end; architecture rtl of bad_vector is begin "
      "process(clk) begin if clk'event and clk = '1' then q <= d; end if; "
      "end process; end;"}) {
    SCOPED_TRACE(source);
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("bad_vector")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, NestedLogicalExpressionConnectivity) {
  std::ifstream fixture(SNL_VHDL_LOGICAL);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 3);
  auto* a = design->getScalarTerm(NLName("a"))->getNet();
  auto* b = design->getScalarTerm(NLName("b"))->getNet();
  auto* c = design->getScalarTerm(NLName("c"))->getNet();
  auto* y = design->getScalarTerm(NLName("y"))->getNet();
  SNLNet* andOutput = nullptr;
  SNLNet* notOutput = nullptr;
  SNLInstance* xorInstance = nullptr;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (model == NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2)) {
      auto* inputs = NLDB0::getGateNTerms(model);
      std::unordered_set<SNLNet*> nets{
          instance->getInstTerm(inputs->getBitAtPosition(0))->getNet(),
          instance->getInstTerm(inputs->getBitAtPosition(1))->getNet()};
      EXPECT_EQ(nets, (std::unordered_set<SNLNet*>{a, b}));
      andOutput = instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet();
    } else if (model == NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1)) {
      EXPECT_EQ(instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet(), c);
      notOutput = instance->getInstTerm(
          NLDB0::getGateNTerms(model)->getBitAtPosition(0))->getNet();
    } else if (model == NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2)) {
      xorInstance = instance;
      EXPECT_EQ(instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet(), y);
    }
  }
  ASSERT_NE(andOutput, nullptr);
  ASSERT_NE(notOutput, nullptr);
  ASSERT_NE(xorInstance, nullptr);
  auto* xorInputs = NLDB0::getGateNTerms(xorInstance->getModel());
  EXPECT_EQ((std::unordered_set<SNLNet*>{
                xorInstance->getInstTerm(xorInputs->getBitAtPosition(0))->getNet(),
                xorInstance->getInstTerm(xorInputs->getBitAtPosition(1))->getNet()}),
            (std::unordered_set<SNLNet*>{andOutput, notOutput}));

  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_LOGICAL_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  for (int stimulus = 0; stimulus < 8; ++stimulus) {
    std::unordered_map<SNLNet*, int> values{
        {a, (stimulus >> 2) & 1}, {b, (stimulus >> 1) & 1}, {c, stimulus & 1}};
    for (std::size_t pass = 0; pass < design->getInstances().size(); ++pass) {
      for (auto* instance : design->getInstances()) {
        auto* model = instance->getModel();
        const auto gate = NLDB0::getGateName(model);
        if (NLDB0::isNInputGate(model)) {
          auto* inputs = NLDB0::getGateNTerms(model);
          auto* lhs = instance->getInstTerm(inputs->getBitAtPosition(0))->getNet();
          auto* rhs = instance->getInstTerm(inputs->getBitAtPosition(1))->getNet();
          if (!values.contains(lhs) || !values.contains(rhs)) continue;
          const int value = gate == "and" ? values[lhs] & values[rhs]
                                           : values[lhs] ^ values[rhs];
          values[instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet()] = value;
        } else {
          auto* input = instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet();
          if (!values.contains(input)) continue;
          values[instance->getInstTerm(
              NLDB0::getGateNTerms(model)->getBitAtPosition(0))->getNet()] = 1 - values[input];
        }
      }
    }
    ASSERT_TRUE(values.contains(y));
    EXPECT_EQ(values[y], ((values[a] & values[b]) ^ (1 - values[c])));
    if (reference.is_open()) {
      int expected = -1;
      ASSERT_TRUE(reference >> expected);
      EXPECT_EQ(values[y], expected);
    }
  }
  if (reference.is_open()) {
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameLogicalModels) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_LOGICAL_SV));
  auto* design = library_->getSNLDesign(NLName("logic_nested_sv"));
  ASSERT_NE(design, nullptr);
  std::unordered_map<SNLDesign*, std::size_t> models;
  for (auto* instance : design->getInstances()) ++models[instance->getModel()];
  EXPECT_EQ(models[NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2)], 1);
  EXPECT_EQ(models[NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2)], 1);
  EXPECT_EQ(models[NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1)], 1);
}

TEST_F(VHDLConstructorTest, ScalarExpressionTypeErrorsPublishNoDesign) {
  for (const auto* expression : {
      "a = b", "a and flag", "'Z'", "'1' = '0'", "a + b"}) {
    SCOPED_TRACE(expression);
    const std::string source = std::string(
        "entity bad_logic is port(a, b : in bit; flag : in boolean; y : out bit); end; "
        "architecture rtl of bad_logic is begin y <= ") + expression + "; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("bad_logic")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, UnsupportedPortShapeDoesNotCreateDesign) {
  constexpr auto source = R"(
entity mux is port (a : in bit_vector(1 downto 0)); end entity mux;
architecture rtl of mux is begin end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("mux")), nullptr);
}

TEST_F(VHDLConstructorTest, NineValuedPortIsRejectedBeforeDesignCreation) {
  constexpr auto source = R"(
library ieee;
use ieee.std_logic_1164.all;
entity mux_logic is port (a : in std_logic; y : out std_logic); end entity mux_logic;
architecture rtl of mux_logic is begin y <= a; end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("mux_logic")), nullptr);
}

TEST_F(VHDLConstructorTest, NumericStdUnsignedAdditionIsLowered) {
  constexpr auto source = R"(
library ieee;
use ieee.numeric_std.all;
entity add_unsigned is port (
  a, b : in unsigned(3 downto 0); y : out unsigned(3 downto 0));
end entity add_unsigned;
library ieee;
use ieee.numeric_std.all;
architecture rtl of add_unsigned is begin y <= a + b; end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  ASSERT_NE(constructor.construct(source), nullptr);
  EXPECT_NE(library_->getSNLDesign(NLName("add_unsigned")), nullptr);
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameMuxModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_MUX_SV));
  auto* design = library_->getSNLDesign(NLName("mux_sv"));
  ASSERT_NE(design, nullptr);
  auto instances = design->getInstances();
  ASSERT_EQ(instances.size(), 1);
  EXPECT_EQ((*instances.begin())->getModel(), NLDB0::getOrCreateMux2(1));
}

TEST_F(VHDLConstructorTest, ClockedRegisterWiring) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity reg is port (clk, d : in bit; q : out bit); end;
architecture rtl of reg is begin
  process(CLK) is begin
    if clk'event and Clk = '1' then Q <= D; end if;
  end process;
end;
)");
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFF());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
}

TEST_F(VHDLConstructorTest, ParenthesizedClockGuardPipelineWiring) {
  for (const auto* type : {"bit", "std_logic"}) {
    for (const auto* guard : {"(CLK'EVENT AND clk = '1')",
                             "((clk'event) and (clk = '1'))", "(rising_edge(clk))"}) {
      SCOPED_TRACE(type);
      SCOPED_TRACE(guard);
      auto* design = VHDLConstructor(library_).construct(std::string(
          "library ieee; use ieee.std_logic_1164.all; "
          "entity pipeline is ") + (std::string(type) == "std_logic" ? "generic(n : integer range 1 to 1 := 1); " : "") +
          "port(clk, d : in " + type + "; q : out " + type +
          "); end; architecture rtl of pipeline is signal a, b : " + type +
          "; begin process(clk) begin if " + guard +
          " then a <= d; b <= a; q <= b; end if; end process; end;");
      ASSERT_EQ(design->getInstances().size(), 3u);
      const std::unordered_map<SNLBitNet*, SNLBitNet*> expected{
          {design->getScalarNet(NLName("a")), design->getScalarTerm(NLName("d"))->getNet()},
          {design->getScalarNet(NLName("b")), design->getScalarNet(NLName("a"))},
          {design->getScalarTerm(NLName("q"))->getNet(), design->getScalarNet(NLName("b"))}};
      for (auto* flop : design->getInstances()) {
        ASSERT_TRUE(NLDB0::isDFF(flop->getModel()));
        EXPECT_EQ(flop->getInstTerm(NLDB0::getDFFClock())->getNet(),
                  design->getScalarTerm(NLName("clk"))->getNet());
        EXPECT_EQ(flop->getInstTerm(NLDB0::getDFFData())->getNet(),
                  expected.at(flop->getInstTerm(NLDB0::getDFFOutput())->getNet()));
      }
      design->destroy();
    }
  }
}

TEST_F(VHDLConstructorTest, StructuredEventGuardPreservesVectorCycles) {
  for (const auto* guard : {"clk'event and clk = '1'", "((CLK'EVENT) and (clk = '1'))"}) {
    SCOPED_TRACE(guard);
    auto source = lfsrSource(4);
    source.replace(source.find("rising_edge(clk)"), std::string("rising_edge(clk)").size(), guard);
    auto* design = VHDLConstructor(library_).construct(source);
    auto* state = design->getBusNet(NLName("temp"));
    ASSERT_NE(state, nullptr);
    unsigned expected = 0;
    for (unsigned cycle = 0; cycle < 40; ++cycle) {
      const bool reset = cycle == 0 || cycle == 21;
      const bool enable = cycle % 3 != 0;
      std::unordered_map<SNLBitNet*, bool> values, next;
      values[design->getScalarTerm(NLName("rst"))->getNet()] = reset;
      values[design->getScalarTerm(NLName("ena"))->getNet()] = enable;
      for (unsigned bit = 0; bit < 4; ++bit) values[state->getBit(bit)] = (expected >> bit) & 1;
      std::unordered_set<SNLBitNet*> visiting;
      for (const auto& flop : dffBits(design)) {
        EXPECT_EQ(flop.clock,
                  design->getScalarTerm(NLName("clk"))->getNet());
        next[flop.output] =
            evaluateRTL(flop.data, values, visiting);
      }
      if (reset) expected = 15;
      else if (enable) expected = (expected >> 1) ^ ((expected & 1) ? 11 : 0);
      for (unsigned bit = 0; bit < 4; ++bit)
        EXPECT_EQ(next.at(state->getBit(bit)), bool((expected >> bit) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidParenthesizedClockSemantics) {
  for (const auto* guard : {"(clk'event and d = '1')"}) {
    for (const bool structured : {false, true}) {
      SCOPED_TRACE(guard);
      SCOPED_TRACE(structured);
      const auto source = std::string(
          "entity invalid is port(clk, d : in bit; q : out bit); end; "
          "architecture rtl of invalid is begin process(clk) begin if ") + guard +
          " then " + (structured ? "for i in 0 to 1 loop q <= d; end loop;" : "q <= d;") +
          " end if; end process; end;";
      EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
      EXPECT_TRUE(library_->getSNLDesigns().empty());
    }
  }
}

TEST_F(VHDLConstructorTest, RisingEdgeRegisterWiring) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity rising_reg is port (clk, d : in bit; q : out bit); end;
architecture rtl of rising_reg is begin
  process(clk) begin
    if rising_edge(clk) then q <= d; end if;
  end process;
end;
)");
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFF());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
}

TEST_F(VHDLConstructorTest, ClockEnableWiringAndCycles) {
  std::ifstream fixture(SNL_VHDL_ENABLED);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFFE());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEEnable())->getNet(),
            design->getScalarTerm(NLName("en"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
  if (const auto* path = std::getenv("VHDL_ENABLE_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<int> values;
    for (int value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<int>{1, 1, 0}));
  }
}

TEST_F(VHDLConstructorTest, SynchronousResetWiringAndCycles) {
  std::ifstream fixture(SNL_VHDL_RESET);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFFSR());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSRClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSRData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSRReset())->getNet(),
            design->getScalarTerm(NLName("rst"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSROutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
  if (const auto* path = std::getenv("VHDL_RESET_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<int> values;
    for (int value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<int>{1, 1, 0, 1}));
  }

  auto* pair = VHDLConstructor(library_).construct(R"(
entity reset_pair is
  port(clk, rst, d0, d1 : in bit; q0, q1 : out bit);
end;
architecture rtl of reset_pair is begin
  process(clk) begin
    if rising_edge(clk) then
      if rst = '1' then q0 <= '0'; q1 <= '0';
      else q0 <= d0; q1 <= d1; end if;
    end if;
  end process;
end;
)");
  ASSERT_EQ(pair->getInstances().size(), 2);
  for (auto* resetFlop : pair->getInstances()) {
    EXPECT_EQ(resetFlop->getModel(), NLDB0::getDFFSR());
    EXPECT_EQ(resetFlop->getInstTerm(NLDB0::getDFFSRReset())->getNet(),
              pair->getScalarTerm(NLName("rst"))->getNet());
  }
}

TEST_F(VHDLConstructorTest, SynchronousResetWithEnableWiringAndCycles) {
  std::ifstream fixture(SNL_VHDL_RESET_ENABLE);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  auto* model = NLDB0::getDFFSRE();
  EXPECT_EQ(instance->getModel(), model);
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("C")))->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("D")))->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("E")))->getNet(),
            design->getScalarTerm(NLName("en"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("R")))->getNet(),
            design->getScalarTerm(NLName("rst"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("Q")))->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
  if (const auto* path = std::getenv("VHDL_RESET_ENABLE_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<int> values;
    for (int value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<int>{1, 1, 1, 0, 0, 1}));
  }
}

TEST_F(VHDLConstructorTest, UnsupportedClockedProcessesPublishNoDesign) {
  for (const auto* body : {
      "process(clk) begin if clk'event and clk = '0' then q <= d; end if; end process;",
      "process(d) begin if clk'event and clk = '1' then q <= d; end if; end process;",
      "process(d) begin if rising_edge(clk) then q <= d; end if; end process;",
      "process(clk) begin if rising_edge(missing) then q <= d; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '0' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if missing = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if q = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '0' then q <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '1' then q <= '1'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if missing = '1' then q <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if q = '1' then q <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '1' then d <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) is variable v : bit; begin if rising_edge(clk) then if d = '1' then q <= '0'; else v := d; q <= v; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if rst = '0' then q <= '0'; elsif en = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if rst = '1' then q <= '0'; elsif en = '0' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if rst = '1' then q <= '1'; elsif en = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if clk'event and d = '1' then q <= d; end if; end process;",
      "process(clk) begin if missing'event and clk = '1' then q <= d; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= missing; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then d <= q; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d after 1 ns; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d; else q <= '0'; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d; q <= clk; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d when clk = '1' else clk; end if; end process;"}) {
    SCOPED_TRACE(body);
    const std::string source = std::string(
        "entity reg is port(clk, d : in bit; q : out bit); end; "
        "architecture rtl of reg is begin ") + body + " end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("reg")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameRegisterModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_REGISTER_SV));
  auto* design = library_->getSNLDesign(NLName("reg_sv"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFF());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
}

TEST_F(VHDLConstructorTest, ClockedPortTypesAndInitializationAreRejected) {
  for (const auto* ports : {
      "clk, d : in std_logic; q : out std_logic",
      "clk : in bit; d : in bit_vector(1 downto 0); q : out bit",
      "clk, d : in bit; q : out bit := '1'",
      "clk : out bit; d : in bit; q : out bit"}) {
    SCOPED_TRACE(ports);
    const std::string source = std::string("entity reg is port(") + ports +
        "); end; architecture rtl of reg is begin "
        "process(clk) begin if clk'event and clk = '1' then q <= d; "
        "end if; end process; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("reg")), nullptr);
  }
}

namespace {
std::string pipelineSource(const std::string& declarations, const std::string& writes) {
  return "entity pipeline is port(clk, d : in bit; q : out bit); end; "
      "architecture rtl of pipeline is " + declarations +
      " begin process(clk) is begin if clk'event and clk = '1' then " +
      writes + " end if; end process; end;";
}
}

TEST_F(VHDLConstructorTest, PipelineConnectivityAndCycles) {
  std::ifstream fixture(SNL_VHDL_PIPELINE);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  std::vector<std::string> sources{source,
      pipelineSource("signal stage : bit;", "q <= STAGE; Stage <= (d);"),
      pipelineSource("signal stage, extra : bit;", "extra <= stage; q <= stage; stage <= d;")};
  for (const auto& input : sources) {
    auto* design = VHDLConstructor(library_).construct(input);
    const bool extra = input.find("extra") != std::string::npos;
    ASSERT_EQ(design->getInstances().size(), extra ? 3 : 2);
    auto* clk = design->getScalarTerm(NLName("clk"))->getNet();
    auto* d = design->getScalarTerm(NLName("d"))->getNet();
    auto* q = design->getScalarTerm(NLName("q"))->getNet();
    auto* stage = design->getNet(NLName("stage"));
    ASSERT_NE(stage, nullptr);
    EXPECT_EQ(design->getScalarTerm(NLName("stage")), nullptr);
    std::unordered_map<SNLNet*, SNLNet*> drivers;
    for (auto* instance : design->getInstances()) {
      ASSERT_EQ(instance->getModel(), NLDB0::getDFF());
      EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(), clk);
      drivers.emplace(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
                      instance->getInstTerm(NLDB0::getDFFData())->getNet());
    }
    EXPECT_EQ(drivers.at(stage), d);
    EXPECT_EQ(drivers.at(q), stage);
    if (extra) EXPECT_EQ(drivers.at(design->getNet(NLName("extra"))), stage);
    // No DFF power-up state is promised. Start unknown, then compare only
    // after two rising edges have filled both stages with known input data.
    std::unordered_map<SNLNet*, int> values;
    for (const auto& [output, data] : drivers) values[output] = -1;
    const std::vector<int> stimulus{1, 0, 1, 1, 0, 0, 1, 0};
    std::vector<int> observed;
    for (std::size_t cycle = 0; cycle < stimulus.size(); ++cycle) {
      values[d] = stimulus[cycle];
      auto next = values;
      for (const auto& [output, data] : drivers) next[output] = values.at(data);
      values = next;
      if (cycle) {
        EXPECT_EQ(values.at(q), stimulus[cycle - 1]);
        observed.push_back(values.at(q));
      }
      // Changing data with no rising edge leaves the sampled outputs intact.
      values[d] = 1 - stimulus[cycle];
      EXPECT_EQ(values.at(q), cycle ? stimulus[cycle - 1] : -1);
    }
    if (const auto* path = std::getenv("VHDL_PIPELINE_REFERENCE")) {
      std::ifstream reference(path);
      ASSERT_TRUE(reference);
      for (int actual : observed) {
        int expected = -1;
        ASSERT_TRUE(reference >> expected);
        EXPECT_EQ(actual, expected);
      }
      std::string trailing;
      EXPECT_FALSE(reference >> trailing);
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, EventGuardStructuredEnablePreservesScheduling) {
  auto* design = VHDLConstructor(library_).construct(pipelineSource(
      "signal stage : bit;", "stage <= d; if d = '1' then q <= stage; end if;"));
  auto* stage = design->getScalarNet(NLName("stage"));
  auto* data = design->getScalarTerm(NLName("d"))->getNet();
  auto* output = design->getScalarTerm(NLName("q"))->getNet();
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    const bool d = pattern & 1, previous = pattern & 2, q = pattern & 4;
    std::unordered_map<SNLBitNet*, bool> values{{data, d}, {stage, previous}, {output, q}};
    std::unordered_set<SNLBitNet*> visiting;
    unsigned flops = 0;
    for (auto* flop : design->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
      ++flops;
      auto* target = flop->getInstTerm(NLDB0::getDFFOutput())->getNet();
      EXPECT_EQ(evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting),
                target == stage ? d : (d ? previous : q));
    }
    EXPECT_EQ(flops, 2u);
  }
}

TEST_F(VHDLConstructorTest, PipelineUnsupportedSemanticsPublishNoDesign) {
  for (const auto& [declarations, writes] : std::vector<std::pair<std::string, std::string>>{
      {"signal stage : std_logic;", "stage <= d; q <= stage;"},
      {"signal stage : bit_vector(1 downto 0);", "stage <= d; q <= stage;"},
      {"signal stage : bit bus;", "stage <= d; q <= stage;"},
      {"signal stage, STAGE : bit;", "stage <= d; q <= stage;"},
      {"signal d : bit;", "q <= d;"},
      {"signal stage : bit;", "q <= stage;"},
      {"signal stage : bit;", "stage <= d; q <= missing;"},
      {"signal stage : bit;", "stage <= d; q <= stage; stage <= clk;"},
      {"signal stage : bit;", "stage <= d; d <= stage;"},
      {"signal stage : bit;", "stage <= d; q <= stage after 1 ns;"},
      {"signal stage : bit;", "stage <= transport d; q <= stage;"},
      {"signal stage : bit;", "stage <= d; q <= stage; else q <= d;"},
      {"signal stage : bit;", "stage <= d; q <= stage; wait;"},
      {"signal stage : bit;", "stage <= d; q <= stage + d;"}}) {
    SCOPED_TRACE(declarations + writes);
    EXPECT_THROW(VHDLConstructor(library_).construct(pipelineSource(declarations, writes)), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  for (const auto* suffix : {
      "stage <= d;",
      "process(clk) begin if clk'event and clk = '1' then stage <= d; end if; end process;"}) {
    auto source = pipelineSource("signal stage : bit;", "stage <= d; q <= stage;");
    source.insert(source.rfind("end;"), suffix);
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, VariableSchedulingConnectivityAndCycles) {
  std::ifstream fixture(SNL_VHDL_VARIABLES);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 4);
  EXPECT_EQ(design->getNet(NLName("temp")), nullptr);
  EXPECT_EQ(design->getNet(NLName("copy")), nullptr);
  auto* clk = design->getScalarTerm(NLName("clk"))->getNet();
  auto* d = design->getScalarTerm(NLName("d"))->getNet();
  auto* stage = design->getNet(NLName("stage"));
  auto* delayed = design->getScalarTerm(NLName("delayed"))->getNet();
  auto* immediate = design->getScalarTerm(NLName("immediate"))->getNet();
  auto* captured = design->getScalarTerm(NLName("captured"))->getNet();
  std::unordered_map<SNLNet*, SNLNet*> drivers;
  for (auto* instance : design->getInstances()) {
    ASSERT_EQ(instance->getModel(), NLDB0::getDFF());
    EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(), clk);
    drivers.emplace(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
                    instance->getInstTerm(NLDB0::getDFFData())->getNet());
  }
  EXPECT_EQ(drivers.at(stage), d);
  EXPECT_EQ(drivers.at(delayed), stage);
  EXPECT_EQ(drivers.at(immediate), d);
  EXPECT_EQ(drivers.at(captured), stage);
  std::unordered_map<SNLNet*, int> values;
  for (const auto& [output, data] : drivers) values[output] = -1;
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_VARIABLE_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  const std::vector<int> stimulus{1, 0, 1, 1, 0, 0, 1, 0};
  for (std::size_t cycle = 0; cycle < stimulus.size(); ++cycle) {
    values[d] = stimulus[cycle];
    auto next = values;
    for (const auto& [output, data] : drivers) next[output] = values.at(data);
    values = next;
    EXPECT_EQ(values.at(immediate), stimulus[cycle]);
    EXPECT_EQ(values.at(delayed), cycle ? stimulus[cycle - 1] : -1);
    EXPECT_EQ(values.at(captured), values.at(delayed));
    if (cycle && reference.is_open()) {
      for (auto* output : {delayed, immediate, captured}) {
        int expected = -1;
        ASSERT_TRUE(reference >> expected);
        EXPECT_EQ(values.at(output), expected);
      }
    }
  }
  if (reference.is_open()) {
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, RetainedVariableConnectivityAndCycles) {
  std::ifstream fixture(SNL_VHDL_RETAINED_VARIABLES);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 2);
  auto* clk = design->getScalarTerm(NLName("clk"))->getNet();
  auto* d = design->getScalarTerm(NLName("d"))->getNet();
  auto* q = design->getScalarTerm(NLName("q"))->getNet();
  auto* retained = design->getNet(NLName("retained"));
  ASSERT_NE(retained, nullptr);
  EXPECT_EQ(design->getScalarTerm(NLName("retained")), nullptr);
  std::unordered_map<SNLNet*, SNLNet*> drivers;
  for (auto* instance : design->getInstances()) {
    ASSERT_EQ(instance->getModel(), NLDB0::getDFF());
    EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(), clk);
    drivers.emplace(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
                    instance->getInstTerm(NLDB0::getDFFData())->getNet());
  }
  EXPECT_EQ(drivers.at(retained), d);
  EXPECT_EQ(drivers.at(q), retained);
  std::unordered_map<SNLNet*, int> values{{retained, -1}, {q, -1}};
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_RETAINED_VARIABLE_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  const std::vector<int> stimulus{1, 0, 1, 1, 0, 0, 1, 0};
  for (std::size_t cycle = 0; cycle < stimulus.size(); ++cycle) {
    values[d] = stimulus[cycle];
    auto next = values;
    for (const auto& [output, data] : drivers) next[output] = values.at(data);
    values = next;
    EXPECT_EQ(values.at(retained), stimulus[cycle]);
    EXPECT_EQ(values.at(q), cycle ? stimulus[cycle - 1] : -1);
    if (cycle && reference.is_open()) {
      int expected = -1;
      ASSERT_TRUE(reference >> expected);
      EXPECT_EQ(values.at(q), expected);
    }
  }
  if (reference.is_open()) {
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, UnsupportedVariablesPublishNoDesign) {
  for (const auto& [declarations, body] : std::vector<std::pair<std::string, std::string>>{
      {"variable v : bit;", "q <= v;"},
      {"variable v : bit;", "v := v; q <= v;"},
      {"variable v : std_logic;", "v := d; q <= v;"},
      {"variable v : bit_vector(1 downto 0);", "v := d; q <= v;"},
      {"variable v : bit;", "v <= d; q <= v;"},
      {"variable v : bit;", "d := v; q <= d;"},
      {"variable v : bit;", "v := missing; q <= v;"},
      {"variable v, V : bit;", "v := d; q <= v;"},
      {"variable clk : bit;", "clk := d; q <= clk;"},
      {"variable v : bit;", "v := not d; q <= v;"},
      {"variable v : bit;", "v := d after 1 ns; q <= v;"},
      {"variable v : bit;", "v := d; q <= v; q <= d;"},
      {"variable v : bit;", "v := d;"}}) {
    SCOPED_TRACE(declarations + body);
    const std::string source = "entity p is port(clk, d : in bit; q : out bit); end; "
        "architecture rtl of p is begin process(clk) " + declarations +
        " begin if clk'event and clk = '1' then " + body + " end if; end process; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, LowersOneLevelDirectEntityHierarchy) {
  std::ifstream fixture(SNL_VHDL_HIERARCHY);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* top = VHDLConstructor(library_).construct(source, "HIERARCHY_TOP");
  ASSERT_NE(top, nullptr);
  auto* model = library_->getSNLDesign(NLName("inv4"));
  ASSERT_NE(model, nullptr);
  EXPECT_EQ(model->getInstances().size(), 4);
  ASSERT_EQ(top->getInstances().size(), 2);
  auto* u0 = top->getInstance(NLName("u0"));
  auto* u1 = top->getInstance(NLName("u1"));
  ASSERT_NE(u0, nullptr);
  ASSERT_NE(u1, nullptr);
  EXPECT_EQ(u0->getModel(), model);
  EXPECT_EQ(u1->getModel(), model);
  auto* a = top->getBusTerm(NLName("a"));
  auto* y = top->getBusTerm(NLName("y"));
  auto* mid = dynamic_cast<SNLBusNet*>(top->getNet(NLName("mid")));
  auto* modelA = model->getBusTerm(NLName("a"));
  auto* modelY = model->getBusTerm(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_NE(mid, nullptr);
  for (std::size_t position = 0; position < 4; ++position) {
    EXPECT_EQ(u0->getInstTerm(modelA->getBitAtPosition(position))->getNet(),
              a->getBitAtPosition(position)->getNet());
    EXPECT_EQ(u0->getInstTerm(modelY->getBitAtPosition(position))->getNet(),
              static_cast<SNLNet*>(mid->getBitAtPosition(position)));
    EXPECT_EQ(u1->getInstTerm(modelA->getBitAtPosition(position))->getNet(),
              static_cast<SNLNet*>(mid->getBitAtPosition(position)));
    EXPECT_EQ(u1->getInstTerm(modelY->getBitAtPosition(position))->getNet(),
              y->getBitAtPosition(position)->getNet());
  }
  if (const auto* path = std::getenv("VHDL_HIERARCHY_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<std::string> values;
    for (std::string value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<std::string>{"1001", "0110", "0011"}));
  }
}

TEST_F(VHDLConstructorTest, SpecializesGenericChildrenPerInstance) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity wire is
  generic(n : positive);
  port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0));
end;
architecture rtl of wire is begin y <= a; end;
entity generic_top is port(
  a3 : in bit_vector(2 downto 0); y3 : out bit_vector(2 downto 0);
  a5 : in bit_vector(4 downto 0); y5 : out bit_vector(4 downto 0));
end;
architecture structural of generic_top is begin
  w3: entity work.wire generic map(n => 3) port map(a3, y3);
  w5: entity work.wire generic map(n => 5) port map(a5, y5);
end;
)", "generic_top");
  ASSERT_NE(top, nullptr);
  auto* w3 = top->getInstance(NLName("w3"));
  auto* w5 = top->getInstance(NLName("w5"));
  ASSERT_NE(w3, nullptr);
  ASSERT_NE(w5, nullptr);
  EXPECT_NE(w3->getModel(), w5->getModel());
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(w3->getModel()->getTerm(NLName("a")))->getWidth(), 3);
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(w5->getModel()->getTerm(NLName("a")))->getWidth(), 5);
}

TEST_F(VHDLConstructorTest, ConstructsHierarchyFromFile) {
  auto* top = VHDLConstructor(library_).constructFile(
      SNL_VHDL_HIERARCHY, "hierarchy_top");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getName(), NLName("hierarchy_top"));
  EXPECT_NE(library_->getSNLDesign(NLName("inv4")), nullptr);
}

TEST_F(VHDLConstructorTest, InvalidHierarchyPublishesNoDesign) {
  constexpr auto valid = R"(
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture structural of top is begin
  u0: entity work.leaf port map(a, y);
end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(valid), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
  EXPECT_THROW(VHDLConstructor(library_).construct(valid, "missing"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());

  for (const auto* source : {
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u: entity work.leaf port map(y, a); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is signal spare : bit; begin
          u: entity work.leaf port map(a, y); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u0: entity work.leaf port map(a, y);
          u1: entity work.leaf port map(a, y); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          architecture other of leaf is begin y <= a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u: entity work.leaf port map(a, y); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity middle is port(a : in bit; y : out bit); end;
          architecture structural of middle is begin
          u: entity work.leaf port map(a, y); end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u: entity work.middle port map(a, y); end;)"}) {
    SCOPED_TRACE(source);
    EXPECT_THROW(VHDLConstructor(library_).construct(source, "top"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, ConstantFoldingAndCombinationalSharing) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity folded is port(a, b, s : in bit; v : in bit_vector(3 downto 0);
  y : out bit_vector(11 downto 0)); end;
architecture rtl of folded is begin
  y(0) <= a and '0'; y(1) <= '1' and a;
  y(2) <= '0' or a; y(3) <= a or '1';
  y(4) <= a xnor '0'; y(5) <= '1' xnor a;
  y(6) <= a when true else b;
  y(7) <= a when s = '1' else a;
  y(8) <= a and b; y(9) <= b and a;
  y(10) <= '1' when v = "0010" else '0';
  y(11) <= '1' when v = "0010" else '0';
end;
)");
  size_t inverters = 0, reductions = 0, ands = 0, muxes = 0;
  for (auto* instance : design->getInstances()) {
    const auto* model = instance->getModel();
    if (NLDB0::isMux2(model)) ++muxes;
    if (!NLDB0::isGate(model)) continue;
    if (NLDB0::getGateName(model) == "not") ++inverters;
    if (NLDB0::getGateName(model) == "and") {
      ++ands;
      if (NLDB0::getGateNTerms(model)->getWidth() == 4) ++reductions;
    }
    EXPECT_NE(NLDB0::getGateName(model), "xnor");
  }
  EXPECT_EQ(inverters, 4u);
  EXPECT_EQ(ands, 2u);
  EXPECT_EQ(reductions, 1u);
  EXPECT_EQ(muxes, 1u);
  for (unsigned pattern = 0; pattern < 128; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    for (const auto& [name, shift] : {std::pair{"a", 0u}, {"b", 1u}, {"s", 2u}})
      values[design->getScalarTerm(NLName(name))->getNet()] = (pattern >> shift) & 1;
    const auto v = pattern >> 3;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("v"))->getBit(bit)->getNet()] = (v >> bit) & 1;
    const bool a = pattern & 1, b = pattern & 2;
    const bool expected[]{false, a, a, true, !a, a, a, a, a && b, a && b, v == 2, v == 2};
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 12; ++bit)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting), expected[bit]);
  }
  if (const auto* directory = std::getenv("VHDL_TABLE_DUMP")) {
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.setTopFileName("folded.v");
    dumper.dumpDesign(design, directory);
  }
}

TEST_F(VHDLConstructorTest, IndexedTablePrimitivesAndSharing) {
  auto* design = VHDLConstructor(library_).constructFile(SNL_VHDL_INDEXED_TABLES);
  ASSERT_NE(design, nullptr);
  size_t tables = 0, constantTables = 0, roms = 0, muxes = 0, gates = 0, flops = 0;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (NLDB0::isTableSelect(model)) {
      ++tables;
      const auto signature = NLDB0::getTableSelectSignature(instance);
      EXPECT_EQ(signature.width, 8u);
      EXPECT_EQ(signature.depth, 3u);
      EXPECT_TRUE(signature.abits == 2 || signature.abits == 3);
      bool constants = true;
      for (auto* bit : NLDB0::getTableSelectData(model)->getBits()) {
        auto* net = instance->getInstTerm(bit)->getNet();
        constants &= net->isConstant0() || net->isConstant1();
      }
      constantTables += constants;
    } else if (NLDB0::isMemory(model)) {
      const auto signature = NLDB0::getMemorySignature(instance);
      EXPECT_EQ(signature.writePorts, 0u);
      EXPECT_EQ(signature.width, 8u);
      EXPECT_EQ(signature.depth, 3u);
      ++roms;
      ++constantTables;
    } else if (NLDB0::isMux2(model)) ++muxes;
    else if (NLDB0::isDFF(model)) ++flops;
    else if (NLDB0::isGate(model) || NLDB0::isFA(model)) ++gates;
    else if (NLDB0::isAssign(model)) continue;
    else FAIL() << "unexpected primitive: " << model->getName().getString();
  }
  // Three equivalent zero-based reads and both offset directions share.
  // A signal driven later still uses a table select with constant DATA.
  EXPECT_EQ(tables + roms, 5u);
  EXPECT_EQ(roms, 3u);
  EXPECT_EQ(constantTables, 4u);
  EXPECT_EQ(muxes, 0u);
  EXPECT_EQ(flops, 3u);
  EXPECT_LT(gates, 90u);
  const auto flopped = dffBits(design);
  ASSERT_EQ(flopped.size(), 24u);
  for (unsigned pattern = 0; pattern < 256; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    for (unsigned bit = 0; bit < 8; ++bit)
      values[design->getBusTerm(NLName("d"))->getBit(bit)->getNet()] = (pattern >> bit) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    for (const auto& flop : flopped) values[flop.output] = evaluateRTL(flop.data, values, visiting);
    for (unsigned address = 0; address < 3; ++address) {
      auto current = values;
      for (unsigned bit = 0; bit < 3; ++bit)
        current[design->getBusTerm(NLName("addr"))->getBit(bit)->getNet()] = (address >> bit) & 1;
      const unsigned expected = address == 0 ? pattern : address == 1 ? (pattern ^ 255) : (pattern ^ 0xa5);
      for (unsigned bit = 0; bit < 8; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("writable_q"))->getBit(bit)->getNet(), current, visiting),
                  bool((expected >> bit) & 1));
    }
  }
  if (const auto* directory = std::getenv("VHDL_TABLE_DUMP")) {
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.setTopFileName("indexed_tables.v");
    dumper.dumpDesign(design, directory);
  }
}

TEST_F(VHDLConstructorTest, PackageROMAndDynamicMemoryCycles) {
  VHDLConstructor constructor(library_);
  EXPECT_EQ(constructor.construct(R"(
library ieee; use ieee.std_logic_1164.all;
package tables is
  type table_type is array (0 to 3) of std_logic_vector(7 downto 0);
  constant rom : table_type := (x"63", x"7C", x"77", x"7B");
end package;
package body tables is end package body;
)"), nullptr);
  auto* design = constructor.construct(R"(
library ieee; use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all; use ieee.std_logic_unsigned.all;
use work.tables.all;
entity memory_test is
  port(clk, we : in std_logic; addr : in std_logic_vector(1 downto 0);
       data : in std_logic_vector(7 downto 0);
       q, lookup, rotated : out std_logic_vector(7 downto 0));
end;
architecture rtl of memory_test is
  signal ram : table_type;
  signal count : integer range 0 to 3;
begin
  lookup <= rom(conv_integer(addr));
  rotated <= data(3 downto 0) & data(7 downto 4);
  write_ram: process(clk) begin
    if rising_edge(clk) then
      if we = '1' then ram(conv_integer(addr)) <= data; end if;
      q <= ram(conv_integer(addr));
    end if;
  end process;
  counter: process(clk) begin
    if rising_edge(clk) then
      if count = 3 then count <= 0; else count <= count + 1; end if;
    end if;
  end process;
end;
)");
  ASSERT_NE(design, nullptr);
  const auto flops = dffBits(design);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  ASSERT_EQ(flops.size(), 10u);
  auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 1u);
  const auto signature = NLDB0::getMemorySignature(memories.begin()->first);
  EXPECT_EQ(signature.width, 8u);
  EXPECT_EQ(signature.depth, 4u);
  unsigned ram[4]{};
  const unsigned rom[]{0x63, 0x7c, 0x77, 0x7b};
  for (unsigned cycle = 0; cycle < 80; ++cycle) {
    const unsigned address = (cycle * 3) % 4;
    const unsigned data = (cycle * 73 + 5) & 255;
    const bool write = cycle % 3 != 0;
    auto values = state;
    values[design->getScalarTerm(NLName("we"))->getNet()] = write;
    for (unsigned bit = 0; bit < 2; ++bit)
      values[design->getBusTerm(NLName("addr"))->getBit(bit)->getNet()] = (address >> bit) & 1;
    for (unsigned bit = 0; bit < 8; ++bit)
      values[design->getBusTerm(NLName("data"))->getBit(bit)->getNet()] = (data >> bit) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 8; ++bit) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("lookup"))->getBit(bit)->getNet(), values, visiting),
                bool((rom[address] >> bit) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("rotated"))->getBit(bit)->getNet(), values, visiting),
                bool((data >> ((bit + 4) % 8)) & 1));
    }
    const auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
    for (const auto& flop : flops)
      state[flop.output] =
          evaluateRTL(flop.data, values, visiting, &memories);
    for (unsigned bit = 0; bit < 8; ++bit)
      EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBit(bit)->getNet()), bool((ram[address] >> bit) & 1));
    auto* count = design->getBusNet(NLName("count"));
    for (unsigned bit = 0; bit < 2; ++bit)
      EXPECT_EQ(state.at(count->getBit(bit)), bool((((cycle + 1) % 4) >> bit) & 1));
    if (write) ram[address] = data;
    memories = nextMemories;
  }
}

TEST_F(VHDLConstructorTest, PackageComponentSpecializationAndNestedGenerate) {
  auto* top = VHDLConstructor(library_).construct(R"(
package interfaces is
  component leaf is generic(n : integer range 1 to 8 := 2);
    port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0));
  end component;
end;
entity leaf is generic(n : integer range 1 to 8 := 2);
  port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0)); end;
architecture rtl of leaf is begin
  g: for i in 0 to n-1 generate
    h: for j in 0 to 0 generate y(i) <= not a(i+j); end generate;
  end generate;
end;
use work.interfaces.all;
entity wrapper is port(a : in bit_vector(3 downto 0); y : out bit_vector(3 downto 0)); end;
architecture rtl of wrapper is begin
  u: leaf generic map(n => 4) port map(y => y, a => a);
end;
)", "wrapper");
  ASSERT_NE(top, nullptr);
  auto* instance = top->getInstance(NLName("u"));
  ASSERT_NE(instance, nullptr);
  auto* leaf = instance->getModel();
  EXPECT_EQ(leaf->getBusTerm(NLName("a"))->getWidth(), 4);
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit) {
      auto* input = leaf->getBusTerm(NLName("a"))->getBit(bit);
      EXPECT_EQ(instance->getInstTerm(input)->getNet(), top->getBusTerm(NLName("a"))->getBit(bit)->getNet());
      values[input->getNet()] = (pattern >> bit) & 1;
    }
    for (unsigned bit = 0; bit < 4; ++bit)
      EXPECT_EQ(evaluateRTL(leaf->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
                !bool((pattern >> bit) & 1));
  }
}

TEST_F(VHDLConstructorTest, MultiplicationSkipsZeroPartialProducts) {
  for (const unsigned factor : {0u, 2u}) {
    SCOPED_TRACE(factor);
    const auto source = std::string(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity product is port(a : in unsigned(3 downto 0); y : out unsigned(3 downto 0)); end;
architecture rtl of product is begin y <= resize(to_unsigned()") +
        std::to_string(factor) + R"(, 4) * a, 4); end;
)";
    auto* design = VHDLConstructor(library_).construct(source);
    ASSERT_NE(design, nullptr);
    for (unsigned input = 0; input < 16; ++input) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (int bit = 0; bit < 4; ++bit)
        values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (input >> bit) & 1;
      for (int bit = 0; bit < 4; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
            bool(((factor * input) >> bit) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, StaticSelectorChoosesHardwareVector) {
  for (const bool condition : {false, true}) {
    SCOPED_TRACE(condition);
    const auto source = std::string(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity selector is port(a, b : in unsigned(1 downto 0); y : out unsigned(1 downto 0)); end;
architecture rtl of selector is begin y <= sel_suv_f()") +
        (condition ? "true" : "false") + R"(, a, b); end;
)";
    auto* design = VHDLConstructor(library_).construct(source);
    ASSERT_NE(design, nullptr);
    for (unsigned pattern = 0; pattern < 16; ++pattern) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (int bit = 0; bit < 2; ++bit) {
        values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (pattern >> bit) & 1;
        values[design->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (pattern >> (bit + 2)) & 1;
      }
      for (int bit = 0; bit < 2; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
            bool((pattern >> (bit + (condition ? 0 : 2))) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, InvalidRTLExpressionsAndPackagesDoNotPublishDesigns) {
  const std::string selector = R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity selector is port(a, b : in unsigned(1 downto 0); y : out unsigned(1 downto 0)); end;
architecture rtl of selector is begin y <= sel_suv_f(1, a, b); end;
)";
  const std::string top = "entity top is port(y : out bit); end; "
      "architecture rtl of top is begin y <= '0'; end;";
  std::vector<std::pair<std::string, std::string>> invalid = {
      {selector, "sel_suv_f condition must be boolean"},
      {"package p is end; package body p is end; package body p is end; " + top,
          "duplicate package body or body without declaration"},
      {"package body p is end; " + top, "package body without declaration"}};
  for (const auto& [declarations, operands] : std::vector<std::pair<std::string, std::string>>{
      {"signal a : integer range 0 to 3;", "a & a"},
      {"type t is (idle, busy); signal a : t;", "a & a"},
      {"type t is record field : bit; end record; signal a : t;", "a & a"},
      {"type row is array (0 to 1) of bit; type t is array (0 to 1) of row; signal a : t;", "a & a"},
      {"signal a : bit; signal b : std_logic;", "a & b"}}) {
    invalid.emplace_back("library ieee; use ieee.std_logic_1164.all; "
        "entity concat is port(y : out bit_vector(1 downto 0)); end; "
        "architecture rtl of concat is " + declarations + " begin process(all) begin y <= " +
        operands + "; end process; end;", "unsupported concatenation operands");
  }
  for (const auto& [source, diagnostic] : invalid) {
    SCOPED_TRACE(source);
    try {
      VHDLConstructor(library_).construct(source);
      FAIL() << "expected: " << diagnostic;
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, BooleanArrayConstantsDriveStaticAndRuntimeConditions) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity choices is generic(enabled : boolean := true);
  port(a : in unsigned(1 downto 0); y : out bit; z : out bit_vector(5 downto 2)); end;
architecture rtl of choices is
  type flags is array(5 downto 2) of boolean;
  constant data : flags := (5 => enabled, 3 => not enabled, others => true);
  constant first : boolean := data(5);
  function selected(index : natural) return boolean is begin return data(index); end;
begin
  y <= '1' when data(to_integer(a) + 2) else '0';
  g: for i in data'range generate
    on_path: if selected(i) and first generate z(i) <= '1'; end generate;
    off_path: if not selected(i) generate z(i) <= '0'; end generate;
  end generate;
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  for (int bit = 2; bit <= 5; ++bit)
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(bit)->getNet(), values, visiting), bit != 3);
  for (unsigned input = 0; input < 4; ++input) {
    values.clear();
    for (int bit = 0; bit < 2; ++bit) values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (input >> bit) & 1;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), input != 1);
  }
}

TEST_F(VHDLConstructorTest, NestedAndEnumerationIndexedBooleanArrayConstants) {
  auto* design = VHDLConstructor(library_).construct(R"(
package flags_pkg is
  type index_t is (first, middle, last);
  type row_t is array(index_t) of boolean;
  type matrix_t is array(2 to 3) of row_t;
  constant data : matrix_t := ((first | last => true, others => false), others => (others => false));
end;
use work.flags_pkg.all;
entity choices is port(y : out bit_vector(1 downto 0)); end;
architecture rtl of choices is begin
  yes: if data(2)(last) and not data(2)(middle) generate y(1) <= '1'; end generate;
  no: if not data(3)(first) generate y(0) <= '0'; end generate;
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(1)->getNet(), values, visiting));
  EXPECT_FALSE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(0)->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, BooleanArraySignalsAndPortsPreserveTyping) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity choices is port(clk : in bit; a,b : in boolean; y,q : out boolean); end;
architecture rtl of choices is
  type flags is array(2 to 3) of boolean;
  signal data : flags;
begin
  data <= (a, b);
  y <= data(2) xor data(3);
  process(clk) begin if clk'event and clk = '1' then q <= data(3); end if; end process;
end;
)");
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 1u);
  for (const bool a : {false,true}) for (const bool b : {false,true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(),a},
        {design->getScalarTerm(NLName("b"))->getNet(),b}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), a != b);
    EXPECT_EQ(evaluateRTL(flops.front().data, values, visiting), b);
  }
}

TEST_F(VHDLConstructorTest, BooleanInternalSignalSelectsRTLLoader) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity choices is port(y : out bit); end;
architecture rtl of choices is signal enabled : boolean;
begin enabled <= true; y <= '1' when enabled else '0'; end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, RejectsInvalidBooleanArrayValuesAndIndices) {
  for (const auto* initializer : {"('1', true)", "(1, false)", "(true, false, true)"}) {
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is port(y : out bit); end; architecture rtl of invalid is ") +
        "type flags is array(2 to 3) of boolean; constant data : flags := " + initializer +
        "; begin y <= '1' when data(2) else '0'; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  for (const auto* condition : {"data(1)", "false and data(true)", "data(2) + 1 = 1", "data(2) and 1"}) {
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is port(y : out bit); end; architecture rtl of invalid is ") +
        "type flags is array(2 to 3) of boolean; constant data : flags := (true, false); begin y <= '1' when " + condition + " else '0'; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, ArrayAggregateAssociationsRespectDeclaredIndexOrder) {
  const std::vector<std::pair<std::string, std::vector<unsigned>>> aggregates{
      {"(\"101\", others => \"011\")", {5,3,3,3}},
      {"(2 => \"101\", 4 => \"010\", others => \"011\")", {5,3,2,3}},
      {"(2 | 5 => \"101\", 3 to 4 => \"010\")", {5,2,2,5}},
      {"(5 downto 3 => \"101\", 2 => \"010\")", {2,5,5,5}}};
  for (const bool ascending : {false, true}) for (size_t form = 0; form < aggregates.size(); ++form) {
    SCOPED_TRACE(aggregates[form].first);
    const auto source = std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
        "entity lookup is port(a : in unsigned(2 downto 0); y : out bit_vector(2 downto 0)); end; "
        "architecture rtl of lookup is type rows is array(" + (ascending ? "2 to 5" : "5 downto 2") +
        ") of bit_vector(2 downto 0); constant table_data : rows := " + aggregates[form].first +
        "; begin y <= table_data(to_integer(a)); end;";
    auto* design = VHDLConstructor(library_).construct(source);
    for (unsigned input = 0; input < 4; ++input) {
      const auto expected = form == 0 ? ((ascending ? input == 0 : input == 3) ? 5u : 3u) : aggregates[form].second[input];
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (int bit = 0; bit < 3; ++bit) values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = ((input+2) >> bit) & 1;
      for (int bit = 0; bit < 3; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting), bool((expected >> bit) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, ArrayAggregateAssociationsLowerRuntimeValues) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity choices is port(a,b : in bit; y,z : out bit_vector(5 downto 2)); end;
architecture rtl of choices is begin
  y <= (a, b, others => not a);
  process(all) begin z <= (2 | 5 => b, 4 downto 3 => a); end process;
end;
)");
  for (const bool a : {false, true}) for (const bool b : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(),a},
        {design->getScalarTerm(NLName("b"))->getNet(),b}};
    std::unordered_set<SNLBitNet*> visiting;
    for (int bit = 2; bit <= 5; ++bit) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting), bit == 5 ? a : bit == 4 ? b : !a);
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(bit)->getNet(), values, visiting), bit == 2 || bit == 5 ? b : a);
    }
  }
}

TEST_F(VHDLConstructorTest, NestedAndIntegerArrayAggregateAssociations) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity tables is port(y : out bit_vector(5 downto 2)); end;
architecture rtl of tables is
  type flags is (first, middle, last);
  type row is array(flags) of bit;
  type matrix is array(2 to 3) of row;
  constant data : matrix := ((first | last => '1', others => '0'), others => (others => '0'));
  type numbers is array(5 downto 2) of natural;
  constant values : numbers := (5 => 3, 3 => 2, others => 1);
begin
  y(5) <= data(2)(last);
  y(4) <= data(3)(first);
  y(3) <= '1' when values(3) = 2 else '0';
  y(2) <= '1' when values(2) = 1 and values(5) = 3 else '0';
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  for (int bit = 2; bit <= 5; ++bit)
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting), bit != 4);
}

TEST_F(VHDLConstructorTest, RejectsInvalidArrayAggregateAssociations) {
  for (const auto* aggregate : {"('1', '0')", "('1', '0', '1', '0', '1')", "(2 => '1')",
      "(2 => '1', 2 => '0', others => '0')", "(2 to 4 => '1', 4 => '0', others => '0')",
      "(1 => '1', others => '0')", "(6 downto 4 => '1', others => '0')",
      "(others => '0', 2 => '1')", "(2 => '1', '0', others => '0')",
      "('1', 3 => '0', others => '0')",
      "(2 | others => '0')", "(a => '1', others => '0')"}) {
    SCOPED_TRACE(aggregate);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is port(a : in bit; y : out bit_vector(2 to 5)); end; ") +
        "architecture rtl of invalid is begin y <= " + aggregate + "; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is
 type rows is array(natural range <>) of bit;
 constant data : rows := ('1', others => '0');
begin y <= data(0); end;
)"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, InvalidPackageRTLDoesNotPublishDesigns) {
  const std::string source = R"(
library ieee; use ieee.std_logic_1164.all;
package constants is
  type table_type is array (0 to 1) of std_logic_vector(3 downto 0);
  constant rom : table_type := (x"A", x"5");
end;
library ieee; use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all; use ieee.std_logic_unsigned.all;
use work.constants.all;
entity lookup is port(a : in std_logic_vector(0 downto 0); y : out std_logic_vector(3 downto 0)); end;
architecture rtl of lookup is begin y <= rom(conv_integer(a)); end;
)";
  for (const auto& [before, after] : std::vector<std::pair<std::string, std::string>>{
      {"(x\"A\", x\"5\")", "(x\"A\", x\"5\", x\"0\")"},
      {"x\"A\"", "x\"U\""},
      {"rom(conv_integer(a))", "rom(a)"},
      {"rom(conv_integer(a))", "rom(2)"},
      {"use work.constants.all;", ""},
      {"use ieee.std_logic_arith.all;", ""},
      {"begin y <=", "begin a <= \"0\"; y <="},
  }) {
    SCOPED_TRACE(after);
    auto invalid = source;
    ASSERT_NE(invalid.find(before), std::string::npos);
    invalid.replace(invalid.find(before), before.size(), after);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, UnsupportedConversionOperandsDoNotPublishDesigns) {
  for (const auto& [conversion, operandType, resultType, diagnostic] :
      std::vector<std::tuple<std::string, std::string, std::string, std::string>>{
          {"unsigned", "std_logic", "unsigned(0 downto 0)", "unsupported vector conversion operand"},
          {"signed", "bit_vector(3 downto 0)", "signed(3 downto 0)", "unsupported vector conversion operand"},
          {"std_logic_vector", "bit_vector(3 downto 0)", "std_logic_vector(3 downto 0)",
              "unsupported vector conversion operand"},
          {"std_ulogic_vector", "bit_vector(3 downto 0)", "std_ulogic_vector(3 downto 0)",
              "unsupported vector conversion operand"},
          {"to_integer", "std_logic", "integer range 0 to 3", "unsupported to_integer argument"},
          {"to_integer", "std_logic_vector(1 downto 0)", "integer range 0 to 3", "unsupported to_integer argument"},
          {"to_integer", "unsigned(31 downto 0)", "integer range 0 to 3", "unsupported to_integer argument"},
          {"conv_integer", "std_logic", "integer range 0 to 3", "unsupported conv_integer argument"},
          {"conv_integer", "unsigned(1 downto 0)", "integer range 0 to 3", "unsupported conv_integer argument"},
          {"conv_integer", "std_logic_vector(31 downto 0)", "integer range 0 to 3",
              "unsupported conv_integer argument"}}) {
    SCOPED_TRACE(conversion + "(" + operandType + ")");
    const auto source =
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; "
        "use ieee.std_logic_arith.all; use ieee.std_logic_unsigned.all; "
        "entity conversion is port(a : in " + operandType + "); end; "
        "architecture rtl of conversion is signal y : " + resultType + "; "
        "begin y <= " + conversion + "(a); end;";
    try {
      VHDLConstructor(library_).construct(source);
      FAIL() << "unsupported conversion accepted";
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, ResolvedAndUnresolvedVectorConversionsPreserveBitOrder) {
  for (const auto* inputType : {"std_logic_vector", "std_ulogic_vector", "unsigned", "signed"}) {
    for (const auto* outputType : {"std_logic_vector", "std_ulogic_vector", "unsigned", "signed"}) {
      for (const bool ascending : {false, true}) {
        SCOPED_TRACE(std::string(inputType) + " -> " + outputType + (ascending ? " ascending" : " descending"));
        const auto source = std::string(
            "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; "
            "entity conversion is port(a : in ") + inputType +
            (ascending ? "(4 to 6); y : out " : "(6 downto 4); y : out ") + outputType +
            "(2 downto 0)); end; architecture rtl of conversion is begin y <= " +
            outputType + "(a); end;";
        auto* design = VHDLConstructor(library_).construct(source);
        for (unsigned pattern = 0; pattern < 8; ++pattern) {
          std::unordered_map<SNLBitNet*, bool> values;
          std::unordered_set<SNLBitNet*> visiting;
          for (int bit = 0; bit < 3; ++bit)
            values[design->getBusTerm(NLName("a"))->getBit(ascending ? 6 - bit : 4 + bit)->getNet()] =
                (pattern >> bit) & 1;
          for (int bit = 0; bit < 3; ++bit)
            EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
                bool((pattern >> bit) & 1));
        }
        design->destroy();
      }
    }
  }
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogPreservesArithmeticResults) {
  auto* svLibrary = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
  SNLSVConstructor(svLibrary).construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_ARITHMETIC_SV));
  for (const auto& [name, expression, width, count] :
       std::vector<std::tuple<std::string, std::string, unsigned, unsigned>>{
           {"add4", "a + b", 4, 4}, {"sub4", "a - b", 4, 4},
           {"mul4", "a * b", 8, 24}, {"mulconst4", "a * to_unsigned(5, 4)", 8, 8}}) {
    SCOPED_TRACE(name);
    auto* vhdl = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; "
        "entity " + name + " is port(a, b : in unsigned(3 downto 0); "
        "s : out unsigned(" + std::to_string(width - 1) + " downto 0)); end; "
        "architecture rtl of " + name + " is begin s <= " + expression + "; end;");
    auto* sv = svLibrary->getSNLDesign(NLName(name));
    ASSERT_NE(sv, nullptr);
    for (auto* design : {vhdl, sv}) {
      unsigned fullAdders = 0, wordArithmetic = 0;
      for (auto* instance : design->getInstances()) {
        auto* model = instance->getModel();
        if (design == sv && (NLDB0::isAddSub(model) || NLDB0::isMultiply(model))) {
          ++wordArithmetic;
          if (name == "add4" || name == "sub4") {
            ASSERT_TRUE(NLDB0::isAddSub(model));
            EXPECT_EQ(name == "sub4", NLDB0::isSubtract(model));
            EXPECT_EQ(width, NLDB0::getAddSubWidth(model));
          } else {
            ASSERT_TRUE(NLDB0::isMultiply(model));
            EXPECT_EQ(width, NLDB0::getMultiplyWidth(model));
          }
        } else if (NLDB0::isFA(model)) ++fullAdders;
        else if (!NLDB0::isAssign(model)) {
          ASSERT_TRUE(NLDB0::isGate(model));
          EXPECT_NE(model, NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2));
          EXPECT_NE(model, NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2));
        }
      }
      EXPECT_EQ(fullAdders, design == vhdl ? count : 0u);
      EXPECT_EQ(wordArithmetic, design == sv ? 1u : 0u);
      if (design == vhdl && name == "add4") {
        // Follow each sum through optional VHDL wiring Assigns and verify
        // the LSB-first carry chain and operand connections.
        SNLBitNet* carry = nullptr;
        for (unsigned bit = 0; bit < 4; ++bit) {
          SNLEquipotential output(design->getBusTerm(NLName("s"))->getBit(bit),
              SNLEquipotential::Mode::TraverseAssigns);
          ASSERT_EQ(output.getInstTermOccurrencesSet().size(), 1);
          auto* term = dynamic_cast<SNLInstTerm*>(output.getInstTermOccurrencesSet().begin()->getObject());
          ASSERT_NE(term, nullptr);
          EXPECT_EQ(term->getBitTerm(), NLDB0::getFAOutputS());
          auto* fa = term->getInstance();
          EXPECT_EQ(fa->getModel(), NLDB0::getFA());
          EXPECT_EQ(fa->getInstTerm(NLDB0::getFAInputA())->getNet(),
              design->getBusTerm(NLName("a"))->getBit(bit)->getNet());
          EXPECT_EQ(fa->getInstTerm(NLDB0::getFAInputB())->getNet(),
              design->getBusTerm(NLName("b"))->getBit(bit)->getNet());
          auto* ci = fa->getInstTerm(NLDB0::getFAInputCI())->getNet();
          if (bit) EXPECT_EQ(ci, carry);
          else EXPECT_TRUE(ci->isConstant0());
          carry = fa->getInstTerm(NLDB0::getFAOutputCO())->getNet();
        }
      }
      for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 16; ++b) {
        std::unordered_map<SNLBitNet*, bool> values;
        std::unordered_set<SNLBitNet*> visiting;
        for (unsigned bit = 0; bit < 4; ++bit) {
          values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
          if (auto* input = design->getBusTerm(NLName("b")))
            values[input->getBit(bit)->getNet()] = (b >> bit) & 1;
        }
        const auto expected = name == "add4" ? a + b : name == "sub4" ? a - b :
            name == "mul4" ? a * b : a * 5;
        for (unsigned bit = 0; bit < width; ++bit)
          EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("s"))->getBit(bit)->getNet(), values, visiting),
              bool((expected >> bit) & 1));
      }
    }
  }
}

TEST_F(VHDLConstructorTest, TruncatedArithmeticHasNoUnreadGateOutputs) {
  for (const auto& [name, operation, expectedAdders] :
       std::vector<std::tuple<std::string, std::string, unsigned>>{
           {"add4", "a + b", 4}, {"sub4", "a - b", 4},
           {"mul4", "resize(a * b, 4)", 12},
           {"slice4", "resize(a * b, 8)(3 downto 0)", 12},
           {"sum4", "resize(a + b, 4)", 4},
           {"diff4", "resize(a - b, 4)", 4},
           {"nested4", "resize((a * b) + (a * b), 4)", 16},
           {"convert4", "unsigned(std_logic_vector(resize(a * b, 4)))", 12}}) {
    SCOPED_TRACE(name);
    auto* design = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; "
        "entity " + name + " is port(a, b : in unsigned(3 downto 0); "
        "s : out unsigned(3 downto 0)); end; architecture rtl of " + name +
        " is begin s <= " + operation + "; end;");
    unsigned adders = 0;
    for (auto* instance : design->getInstances()) {
      if (NLDB0::isFA(instance->getModel())) {
        ++adders;
        continue; // Unused full-adder output pins are intentional.
      }
      for (auto* term : instance->getInstTerms()) {
        if (term->getDirection() != SNLTerm::Direction::Output) continue;
        auto* net = term->getNet();
        ASSERT_NE(net, nullptr);
        bool reader = false;
        for (auto* other : net->getInstTerms())
          reader |= other->getDirection() != SNLTerm::Direction::Output;
        for (auto* port : net->getBitTerms())
          reader |= port->getDirection() != SNLTerm::Direction::Input;
        EXPECT_TRUE(reader) << instance->getModel()->getName().getString();
      }
    }
    EXPECT_EQ(adders, expectedAdders);
    for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 16; ++b) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned bit = 0; bit < 4; ++bit) {
        values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
        values[design->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (b >> bit) & 1;
      }
      const auto expected = (name == "add4" || name == "sum4") ? a + b : (name == "sub4" || name == "diff4") ? a - b :
          name == "nested4" ? 2 * a * b : a * b;
      for (unsigned bit = 0; bit < 4; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("s"))->getBit(bit)->getNet(), values, visiting),
            bool((expected >> bit) & 1));
    }
  }
}

TEST_F(VHDLConstructorTest, ResizeRetainsSignedSignAndExtendsVectors) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity resize_math is port(a, b : in signed(3 downto 0);
  narrow : out signed(3 downto 0); wide : out signed(9 downto 0);
  unsigned_wide : out unsigned(9 downto 0)); end;
architecture rtl of resize_math is begin
  narrow <= resize(a * b, 4); wide <= resize(a * b, 10);
  unsigned_wide <= resize(unsigned(a) * unsigned(b), 10);
end;
)");
  for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 16; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit) {
      values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
      values[design->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (b >> bit) & 1;
    }
    const int product = (a < 8 ? int(a) : int(a)-16) * (b < 8 ? int(b) : int(b)-16);
    for (const auto& [name, expected] : std::vector<std::pair<std::string, unsigned>>{
        {"narrow", (unsigned(product) & 7) | (product < 0 ? 8 : 0)},
        {"wide", unsigned(product)}, {"unsigned_wide", a * b}}) {
      auto* term = design->getBusTerm(NLName(name));
      for (unsigned bit = 0; bit < term->getWidth(); ++bit)
        EXPECT_EQ(evaluateRTL(term->getBit(bit)->getNet(), values, visiting), bool((expected >> bit) & 1));
    }
  }
}

TEST_F(VHDLConstructorTest, InvalidResizePublishesNoDesign) {
  for (const auto* operation : {"resize(a, 0)", "resize(a, 65537)", "resize(a, b)",
       "resize(std_logic_vector(a), 4)", "resize(a * b, 3)", "a * b"}) {
    SCOPED_TRACE(operation);
    EXPECT_THROW(VHDLConstructor(library_).construct(
        std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
        "entity bad_resize is port(a, b : in unsigned(3 downto 0); s : out unsigned(3 downto 0)); end; "
        "architecture rtl of bad_resize is begin s <= " + operation + "; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, LegacyUnsignedArithmeticUsesFullAdders) {
  for (const auto* imports : {"use ieee.std_logic_unsigned.all;",
      "use ieee.std_logic_unsigned.all; use ieee.std_logic_arith.all;"}) {
    SCOPED_TRACE(imports);
    auto* design = VHDLConstructor(library_).construct(
        std::string("library ieee; use ieee.std_logic_1164.all; ") + imports +
        " entity legacy_math is port(a, b : in std_logic_vector(3 downto 0); "
        "s, d : out std_logic_vector(3 downto 0)); end; "
        "architecture rtl of legacy_math is begin s <= a + b; d <= a - b; end;");
    unsigned fullAdders = 0;
    for (auto* instance : design->getInstances()) {
      auto* model = instance->getModel();
      if (NLDB0::isFA(model)) ++fullAdders;
      else EXPECT_TRUE(model->isAssign() ||
          model == NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1));
    }
    EXPECT_EQ(fullAdders, 8);
    for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 16; ++b) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned bit = 0; bit < 4; ++bit) {
        values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
        values[design->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (b >> bit) & 1;
      }
      for (unsigned bit = 0; bit < 4; ++bit) {
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("s"))->getBit(bit)->getNet(), values, visiting),
            bool(((a + b) >> bit) & 1));
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("d"))->getBit(bit)->getNet(), values, visiting),
            bool(((a - b) >> bit) & 1));
      }
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, UnsignedArithmeticWidthsAndDirections) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity arithmetic is port(signal a : in unsigned(4 downto 2);
  signal b : in unsigned(7 to 8); product : out unsigned(4 downto 0);
  sum, difference : out unsigned(2 downto 0); joined : out unsigned(4 downto 0);
  eq : out bit); end;
architecture rtl of arithmetic is begin
  product <= a * b; sum <= a + b; difference <= a - b;
  joined <= a & b;
  eq <= '1' when a = b else '0';
end;
)");
  for (unsigned a = 0; a < 8; ++a) for (unsigned b = 0; b < 4; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(2+i)->getNet()] = (a >> i) & 1;
    for (unsigned i = 0; i < 2; ++i)
      values[design->getBusTerm(NLName("b"))->getBit(8-i)->getNet()] = (b >> i) & 1;
    for (const auto& [name, expected] : std::vector<std::pair<std::string, unsigned>>{
        {"product", a*b}, {"sum", (a+b)&7}, {"difference", (a-b)&7}, {"joined", (a<<2)|b}}) {
      auto* term = design->getBusTerm(NLName(name));
      for (unsigned i = 0; i < term->getWidth(); ++i)
        EXPECT_EQ(evaluateRTL(term->getBit(i)->getNet(), values, visiting), bool((expected >> i)&1))
            << name << ":" << a << ":" << b << ":" << i;
    }
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("eq"))->getNet(), values, visiting), a == b);
  }
}

TEST_F(VHDLConstructorTest, NumericVectorIntegerArithmeticOverloads) {
  for (const bool signedType : {false, true}) for (const unsigned width : {1u, 3u, 4u}) {
    const auto type = signedType ? "signed" : "unsigned";
    const std::vector<int> amounts = signedType ? std::vector<int>{-9, -1, 1, 9} : std::vector<int>{0, 1, 3, 10};
    for (const auto amount : amounts) for (const auto* operation : {"+", "-", "*"}) for (const bool reverse : {false, true}) {
      SCOPED_TRACE(std::string(type) + ':' + std::to_string(width) + ':' + std::to_string(amount) + operation + (reverse ? " left" : " right"));
      const auto resultWidth = std::string(operation) == "*" ? 2*width : width;
      const auto expr = reverse ? "(amount + 0) " + std::string(operation) + " a" : "a " + std::string(operation) + " (amount + 0)";
      const auto source = std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
          "entity arithmetic is generic(amount : integer := " + std::to_string(amount) + "); port(a : in " + type +
          "(7 to " + std::to_string(6+width) + "); y : out " + type + "(" + std::to_string(resultWidth-1) +
          " downto 0)); end; architecture rtl of arithmetic is begin y <= " + expr + "; end;";
      auto* design = VHDLConstructor(library_).construct(source);
      const unsigned mask = (1u << width)-1;
      const unsigned encoded = unsigned(amount) & mask;
      const int scalar = signedType && (encoded & (1u << (width-1))) ? int(encoded)-(1<<width) : int(encoded);
      for (unsigned input = 0; input <= mask; ++input) {
        const int vector = signedType && (input & (1u << (width-1))) ? int(input)-(1<<width) : int(input);
        const int left = reverse ? scalar : vector, right = reverse ? vector : scalar;
        const int expected = std::string(operation) == "+" ? left+right : std::string(operation) == "-" ? left-right : left*right;
        std::unordered_map<SNLBitNet*, bool> values;
        std::unordered_set<SNLBitNet*> visiting;
        for (unsigned bit = 0; bit < width; ++bit)
          values[design->getBusTerm(NLName("a"))->getBit(6+width-bit)->getNet()] = (input >> bit) & 1;
        for (unsigned bit = 0; bit < resultWidth; ++bit)
          EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
              bool((unsigned(expected) >> bit) & 1));
      }
      design->destroy();
    }
  }
}

TEST_F(VHDLConstructorTest, NarrowIntegerMultiplicationPreservesScalarConstants) {
  for (const unsigned limit : {1u, 3u}) for (const unsigned stride : {6u, 10u}) {
    SCOPED_TRACE(std::to_string(limit) + ':' + std::to_string(stride));
    const auto width = (limit + 1) * stride;
    const auto source = std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
        "entity selection is generic(stride : positive := " + std::to_string(stride) + "); "
        "port(a : in bit_vector(" + std::to_string(width-1) + " downto 0); "
        "index : in unsigned(1 downto 0); y : out bit_vector(stride-1 downto 0); "
        "z, r : out std_logic_vector(7 downto 0)); end; architecture rtl of selection is "
        "function extract(data : bit_vector; selector : unsigned) return bit_vector is "
        "variable lane : integer range 0 to " + std::to_string(limit) + "; begin "
        "lane := to_integer(selector); return data(lane*stride+stride-1 downto lane*stride); end; "
        "begin y <= extract(a,index); process(all) variable lane : integer range 0 to " +
        std::to_string(limit) + "; variable scaled, mirrored : integer range 0 to 255; begin lane := to_integer(index); "
        "scaled := lane*stride; mirrored := stride*lane; "
        "z <= std_logic_vector(to_unsigned(scaled,8)); "
        "r <= std_logic_vector(to_unsigned(mirrored,8)); end process; end;";
    auto* design = VHDLConstructor(library_).construct(source);
    uint64_t data = 0;
    for (unsigned lane = 0; lane <= limit; ++lane)
      data |= uint64_t(lane*7+3) << (lane*stride);
    for (unsigned index = 0; index <= limit; ++index) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned bit = 0; bit < width; ++bit)
        values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (data >> bit) & 1;
      for (unsigned bit = 0; bit < 2; ++bit)
        values[design->getBusTerm(NLName("index"))->getBit(bit)->getNet()] = (index >> bit) & 1;
      for (unsigned bit = 0; bit < stride; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
            bool((data >> (index*stride+bit)) & 1));
      for (const auto* name : {"z", "r"}) for (unsigned bit = 0; bit < 8; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(bit)->getNet(), values, visiting),
            bool(((index*stride) >> bit) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, NumericOrderingAndScalarComparisonsPreserveFullValues) {
  const std::vector<std::string> operations = {"=", "/=", "<", "<=", ">", ">="};
  const auto compare = [](int left, int right, size_t op) {
    switch (op) {
      case 0: return left == right;
      case 1: return left != right;
      case 2: return left < right;
      case 3: return left <= right;
      case 4: return left > right;
      default: return left >= right;
    }
  };
  for (const bool signedType : {false, true}) {
    const auto type = signedType ? "signed" : "unsigned";
    for (const int amount : (signedType ? std::vector<int>{-9, -1, 0, 9} : std::vector<int>{0, 1, 7, 8})) {
      for (const bool scalarLeft : {false, true}) {
        std::string body;
        for (size_t op = 0; op < operations.size(); ++op) {
          const auto left = scalarLeft ? std::to_string(amount) : "a";
          const auto right = scalarLeft ? "a" : std::to_string(amount);
          body += "y(" + std::to_string(op) + ") <= '1' when " + left + " " + operations[op] + " " + right + " else '0';";
          body += "z(" + std::to_string(op) + ") <= '1' when a " + operations[op] + " b else '0';";
        }
        const auto source = std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
            "entity comparisons is port(a : in " + type + "(2 to 4); b : in " + type +
            "(1 downto 0); y,z : out bit_vector(5 downto 0)); end; architecture rtl of comparisons is begin " + body + " end;";
        auto* design = VHDLConstructor(library_).construct(source);
        for (unsigned a = 0; a < 8; ++a) for (unsigned b = 0; b < 4; ++b) {
          const int av = signedType && a >= 4 ? int(a)-8 : int(a);
          const int bv = signedType && b >= 2 ? int(b)-4 : int(b);
          std::unordered_map<SNLBitNet*, bool> values;
          std::unordered_set<SNLBitNet*> visiting;
          for (int bit = 0; bit < 3; ++bit)
            values[design->getBusTerm(NLName("a"))->getBit(4-bit)->getNet()] = (a >> bit) & 1;
          for (int bit = 0; bit < 2; ++bit)
            values[design->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (b >> bit) & 1;
          for (size_t op = 0; op < operations.size(); ++op) {
            EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(op)->getNet(), values, visiting),
                compare(scalarLeft ? amount : av, scalarLeft ? av : amount, op));
            EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(op)->getNet(), values, visiting), compare(av,bv,op));
          }
        }
        design->destroy();
      }
    }
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidNumericScalarOverloads) {
  for (const auto* expression : {"a + (-1)", "(-1) * a", "a = (-1)", "a + true", "false < a", "a + 2147483648"}) {
    SCOPED_TRACE(expression);
    const bool comparison = std::string(expression).find('=') != std::string::npos || std::string(expression).find('<') != std::string::npos;
    const auto assignment = comparison ? std::string("'1' when ") + expression + " else '0'" : expression;
    const auto source = std::string("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; ") +
        "entity invalid is port(a : in unsigned(2 downto 0); y : out " +
        (comparison ? "bit" : "unsigned(2 downto 0)") + "); end; "
        "architecture rtl of invalid is begin y <= " + assignment + "; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, NumericDynamicIntegerOperands) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity arithmetic is port(a : in signed(2 downto 0); b : in unsigned(3 downto 0);
  p,d : out signed(2 downto 0); m : out signed(5 downto 0); c : out bit_vector(1 downto 0)); end;
architecture rtl of arithmetic is
  signal amount : integer range 0 to 15;
begin
  amount <= to_integer(b);
  p <= a + amount;
  d <= amount - a;
  m <= a * amount;
  c(0) <= '1' when a < amount else '0';
  c(1) <= '1' when amount <= a else '0';
end;
)");
  for (unsigned a = 0; a < 8; ++a) for (unsigned b = 0; b < 16; ++b) {
    const int av = a >= 4 ? int(a)-8 : int(a);
    const int bv = (b & 7) >= 4 ? int(b & 7)-8 : int(b & 7);
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (int bit = 0; bit < 3; ++bit) values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
    for (int bit = 0; bit < 4; ++bit) values[design->getBusTerm(NLName("b"))->getBit(bit)->getNet()] = (b >> bit) & 1;
    for (const auto& [name, expected] : std::vector<std::pair<std::string, int>>{{"p", av+bv}, {"d", bv-av}, {"m", av*bv}})
      for (int bit = 0; bit < (name == "m" ? 6 : 3); ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(bit)->getNet(), values, visiting), bool((unsigned(expected) >> bit) & 1));
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("c"))->getBit(0)->getNet(), values, visiting), av < int(b));
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("c"))->getBit(1)->getNet(), values, visiting), int(b) <= av);
  }
}

TEST_F(VHDLConstructorTest, SignalInitializersPreserveRegisterBitOrder) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity initialized is port(clk : in std_logic; d : in unsigned(2 downto 0);
  q : out unsigned(2 downto 0)); end;
architecture rtl of initialized is
  signal a, b : unsigned(4 to 6) := "101";
  signal c : bit := '1';
begin
  process(clk) begin if rising_edge(clk) then a <= d; b <= a; c <= '0'; end if; end process;
  q <= b;
end;
)");
  unsigned count = 0;
  for (auto* flop : design->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
    ++count;
    auto* initial = flop->getInstParameter(NLName("INIT"));
    ASSERT_NE(initial, nullptr);
    EXPECT_EQ(initial->getValue(), flop->getModel()->getBusTerm(NLName("Q")) ? "3'b101" : "1'b1");
  }
  EXPECT_EQ(count, 3u);
}

TEST_F(VHDLConstructorTest, RejectsUnrepresentableSignalInitializers) {
  for (const auto* body : {
      "signal s : bit := d; begin process(clk) begin if clk'event and clk = '1' then s <= d; end if; end process; q <= s;",
      "signal s : bit := '1'; begin s <= d; q <= s;",
      "signal s : bit := 'X'; begin process(clk) begin if clk'event and clk = '1' then s <= d; end if; end process; q <= s;"}) {
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(clk, d : in bit; q : out bit); end; architecture rtl of invalid is ") + body + " end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, ArchitectureLocalComponentBindings) {
  const std::string source = R"(
entity leaf is port(signal a : in bit; signal y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity wrapper is port(a : in bit; y : out bit); end;
architecture rtl of wrapper is
  component leaf is port(signal a : in bit; signal y : out bit); end component leaf;
begin u : leaf port map(y => y, a => a); end;
)";
  auto* top = VHDLConstructor(library_).construct(source, "wrapper");
  ASSERT_NE(top, nullptr);
  auto* child = top->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* model = child->getModel();
  EXPECT_EQ(child->getInstTerm(model->getScalarTerm(NLName("a")))->getNet(),
            top->getScalarTerm(NLName("a"))->getNet());
  EXPECT_EQ(child->getInstTerm(model->getScalarTerm(NLName("y")))->getNet(),
            top->getScalarTerm(NLName("y"))->getNet());
  top->destroy();
  model->destroy();
  for (const auto& replacement : {std::string("signal y : in bit"), std::string("signal y : out bit_vector(1 downto 0)")}) {
    auto invalid = source;
    const auto position = invalid.find("signal y : out bit", invalid.find("component leaf"));
    invalid.replace(position, std::string("signal y : out bit").size(), replacement);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid, "wrapper"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, InvalidUnsignedArithmeticPublishesNoDesign) {
  for (const auto* assignment : {"y <= a * b;", "y <= a + b;", "y <= a / b;"}) {
    const auto source = std::string(
        "library ieee; use ieee.numeric_std.all; "
        "entity invalid is port(a : in unsigned(2 downto 0); b : in unsigned(1 downto 0); "
        "y : out unsigned(3 downto 0)); end; architecture rtl of invalid is begin ") + assignment + " end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, RTLTopInferenceRejectsAmbiguousRoots) {
  const auto source = R"(
entity first is port(a : in bit; y : out bit); end;
architecture rtl of first is begin
  g: for i in 0 to 0 generate y <= a; end generate;
end;
entity second is port(a : in bit; y : out bit); end;
architecture rtl of second is begin y <= a; end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
  auto* selected = VHDLConstructor(library_).construct(source, "SECOND");
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->getName(), NLName("second"));
}

TEST_F(VHDLConstructorTest, RTLTopInferenceRejectsRootlessHierarchy) {
  const auto source = R"(
entity first is port(a : in bit; y : out bit); end;
architecture rtl of first is begin
  g: for i in 0 to 0 generate
    u: entity work.second port map(a, y);
  end generate;
end;
entity second is port(a : in bit; y : out bit); end;
architecture rtl of second is begin
  u: entity work.first port map(a, y);
end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, GeneratedFIRHierarchyCycles) {
  auto* top = VHDLConstructor(library_).constructFile(SNL_VHDL_GENERATED_FIR);
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getName(), NLName("fir_generated"));
  const auto inputs = naja::NL::test::firInputs(4);
  const auto outputs = naja::NL::test::simulateFIR(top, inputs);
  const unsigned coefficients[3][4]{{1, 3, 6, 10}, {10, 6, 3, 1}, {5, 7, 9, 11}};
  for (size_t cycle = 0; cycle < inputs.size(); ++cycle) {
    unsigned expected = 0;
    for (unsigned age = 0; age < 3; ++age) if (cycle >= age)
      for (unsigned lane = 0; lane < 4; ++lane)
        if ((inputs[cycle-age] >> lane) & 1) expected += coefficients[age][lane];
    EXPECT_EQ(outputs[cycle], expected & 31) << "cycle " << cycle;
  }
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_NE(top->getInstance(NLName("lanes[" + std::to_string(i) + "].singleton[0].f")), nullptr);
  if (const auto* trace = std::getenv("VHDL_FIR_REFERENCE")) {
    std::ifstream reference(trace);
    ASSERT_TRUE(reference);
    for (size_t cycle = 3; cycle < outputs.size(); ++cycle) {
      unsigned expected;
      ASSERT_TRUE(reference >> expected);
      EXPECT_EQ(outputs[cycle], expected) << "NVC cycle " << cycle;
    }
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, SignedVectorArithmeticExtendsTheSignBit) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_signed.all;
entity signed_math is port(a : in std_logic_vector(3 downto 0);
  b : in std_logic_vector(5 to 7); sum, diff : out std_logic_vector(8 downto 5); eq : out bit); end;
architecture rtl of signed_math is
begin sum <= a + b; diff <= a - b; eq <= '1' when a = b else '0'; end;
)");
  unsigned fullAdders = 0;
  for (auto* instance : design->getInstances())
    if (NLDB0::isFA(instance->getModel())) ++fullAdders;
  EXPECT_EQ(fullAdders, 8);
  for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 8; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
    for (unsigned bit = 0; bit < 3; ++bit)
      values[design->getBusTerm(NLName("b"))->getBit(7-bit)->getNet()] = (b >> bit) & 1;
    const int sa = a < 8 ? int(a) : int(a)-16, sb = b < 4 ? int(b) : int(b)-8;
    for (const auto& [name, result] : std::vector<std::pair<std::string, int>>{{"sum", sa+sb}, {"diff", sa-sb}})
      for (unsigned bit = 0; bit < 4; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(bit+5)->getNet(), values, visiting),
                  bool((unsigned(result) >> bit) & 1)) << name << ':' << sa << ':' << sb;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("eq"))->getNet(), values, visiting), sa == sb);
  }
}

TEST_F(VHDLConstructorTest, StaticConversionsAndIntegerArrayIndexSubtypes) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity conversions is port(a : in std_logic_vector(2 to 5);
  copy : out unsigned(3 downto 0); truncated, wide : out std_logic_vector(4 downto 0)); end;
architecture rtl of conversions is
  constant base : integer := -2147483648;
  type coefficients is array(integer range <>) of integer;
  constant table_value : coefficients := (-7, 47);
  type bounded is array(3 downto 1) of integer;
  constant table_bounded : bounded := (12, 9, 5);
  type masks is array(positive range <>) of std_logic_vector(4 downto 0);
  constant mask : masks := ("11111", "00000");
begin
  copy <= unsigned(a);
  truncated <= std_logic_vector(to_unsigned(table_value(base+1), 5)) and mask(1);
  wide <= std_logic_vector(to_unsigned(table_bounded(2) + table_value(base), 5));
end;
)");
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(5-bit)->getNet()] = (pattern >> bit) & 1;
    for (unsigned bit = 0; bit < 4; ++bit)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("copy"))->getBit(bit)->getNet(), values, visiting),
                bool((pattern >> bit) & 1));
    for (const auto& [name, expected] : std::vector<std::pair<std::string, unsigned>>{{"truncated", 15}, {"wide", 2}})
      for (unsigned bit = 0; bit < 5; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(bit)->getNet(), values, visiting),
                  bool((expected >> bit) & 1));
  }
}

TEST_F(VHDLConstructorTest, IndexedPortSlicesPreservePositionAndSharedSpecialization) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity leaf is generic(n : positive); port(a : in bit_vector(n-1 downto 0); y : out bit_vector(1 to n)); end;
architecture rtl of leaf is begin y <= a; end;
entity top is port(a : in bit_vector(3 downto 0); y : out bit_vector(4 to 7)); end;
architecture rtl of top is begin
  g: for i in 0 to 1 generate
    u: entity work.leaf generic map(2) port map(a(3-2*i downto 2-2*i), y(4+2*i to 5+2*i));
  end generate;
end;
)", "top");
  auto* first = top->getInstance(NLName("g[0].u"));
  auto* second = top->getInstance(NLName("g[1].u"));
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->getModel(), second->getModel());
  for (unsigned i = 0; i < 2; ++i) {
    auto* child = i ? second : first;
    auto* model = child->getModel();
    for (unsigned bit = 0; bit < 2; ++bit) {
      EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("a"))->getBit(1-bit))->getNet(),
                top->getBusTerm(NLName("a"))->getBit(3-2*i-bit)->getNet());
      EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("y"))->getBit(1+bit))->getNet(),
                top->getBusTerm(NLName("y"))->getBit(4+2*i+bit)->getNet());
    }
  }
}

TEST_F(VHDLConstructorTest, InvalidGeneratedFIRAndConversionsPublishNoDesigns) {
  std::ifstream fixture(SNL_VHDL_GENERATED_FIR);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  for (const auto& [before, after] : std::vector<std::pair<std::string, std::string>>{
      {"  y_out <= result(-1)", "  LANES: for k in 9 to 9 generate begin end generate; y_out <= result(-1)"},
      {"first(i)", "first(i+1)"}, {"result(i-1)", "result(i)"},
      {"regx_in(i+j)", "regx_in"}, {"result(i-1)", "regx_in(i)"},
      {"to_unsigned(c1, n)", "to_unsigned(-1, n)"},
      {"to_unsigned(c1, n)", "to_unsigned(c1, 0)"},
      {"to_unsigned(c1, n)", "to_unsigned(c1, -1)"},
      {"to_unsigned(c1, n)", "to_unsigned(c1, n, 1)"},
      {"to_unsigned(c1, n)", "to_unsigned(reg_in, n)"},
      {"to_unsigned(c1, n)", "unknown_function(c1, n)"},
      {"use ieee.numeric_std.all;", ""},
      {"constant first : coefficients := (1, 3, 6, 10);", "constant first : coefficients(0 to 4) := (1, 3, 6, 10);"},
      {"constant third : positive_indices", "constant third : positive_indices(0 to 3)"},
      {"(1, 3, 6, 10)", "(1, 3, 6, 2147483648)"},
      {"signal result : results;", "signal first : results; signal result : results;"},
      {"first(i), second(i), third(i+1), width", "first(i), second(i), third(i+1)"},
      {"use ieee.std_logic_signed.all;", "use ieee.std_logic_signed.all; use ieee.std_logic_unsigned.all;"}}) {
    SCOPED_TRACE(after);
    auto invalid = source;
    const auto position = invalid.find(before);
    ASSERT_NE(position, std::string::npos);
    invalid.replace(position, before.size(), after);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid, "fir_generated"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, MissingGenericDiagnosticIncludesSourceLocation) {
  const std::string source =
      "entity needs_width is\n"
      "generic(\n"
      "  Nin : integer);\n"
      "port(a : in bit_vector(Nin-1 downto 0); y : out bit_vector(Nin-1 downto 0)); end;\n"
      "architecture rtl of needs_width is begin y <= a; end;\n";
  try {
    VHDLConstructor(library_).construct(source, "needs_width");
    FAIL() << "An explicit top requires all generic values";
  } catch (const NLException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(message.find("missing generic value: nin"), std::string::npos);
    EXPECT_NE(message.find("line 3, column 3"), std::string::npos) << message;
  }
  EXPECT_TRUE(library_->getSNLDesigns().empty());
  EXPECT_EQ(VHDLConstructor(library_).construct(source), nullptr);
  auto* top = VHDLConstructor(library_).construct(
      "entity parent is port(a : in bit_vector(2 downto 0); y : out bit_vector(2 downto 0)); end;\n"
      "architecture rtl of parent is begin child : entity work.needs_width\n"
      "generic map(Nin => 3) port map(a, y); end;", "parent");
  ASSERT_NE(top, nullptr);
  auto* child = top->getInstance(NLName("child"));
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->getModel()->getBusTerm(NLName("a"))->getWidth(), 3);
}

TEST_F(VHDLConstructorTest, RetainedDependencyDiagnosticUsesOriginalFileAndLine) {
  // The dependency is only elaborated by its parent, after another source was
  // retained. Neither the parent file nor combined-source line is its location.
  const auto directory = std::filesystem::temp_directory_path() /
      ("naja-vhdl-diagnostic-" + std::to_string(reinterpret_cast<uintptr_t>(this)));
  std::filesystem::create_directories(directory);
  struct Cleanup {
    std::filesystem::path directory;
    ~Cleanup() { std::filesystem::remove_all(directory); }
  } cleanup{directory};
  const auto package = directory / "prefix.vhd";
  const auto child = directory / "child.vhd";
  const auto parent = directory / "parent.vhd";
  std::ofstream(package) << "package prefix is\nconstant unused : integer := 1;\nend;\n";
  std::ofstream(child) <<
      "entity child is generic(n : positive); port(a : in bit; y : out bit); end;\n"
      "architecture rtl of child is\n"
      "begin\n"
      "  y <= unknown;\n"
      "end;\n";
  std::ofstream(parent) <<
      "entity parent is port(a : in bit; y : out bit); end;\n"
      "architecture rtl of parent is begin\n"
      "u : entity work.child generic map(3) port map(a,y); end;\n";
  VHDLConstructor constructor(library_);
  EXPECT_EQ(constructor.constructFile(package), nullptr);
  EXPECT_EQ(constructor.constructFile(child), nullptr);
  try {
    constructor.constructFile(parent, "parent");
    FAIL() << "The invalid dependency must be rejected";
  } catch (const NLException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(message.find("no declaration for object: unknown"), std::string::npos);
    EXPECT_NE(message.find("in '" + child.string() + "' at line 4, column 8"), std::string::npos) << message;
  }
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, NumericSignedArithmeticAndConversions) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity numeric_signed_math is port(a : in signed(3 downto 0);
  b : in signed(5 to 7); sum, diff : out signed(8 downto 5);
  product : out signed(6 downto 0); joined : out signed(6 downto 0);
  roundtrip : out unsigned(3 downto 0); eq : out bit); end;
architecture rtl of numeric_signed_math is
begin
  sum <= a + b; diff <= a - b; product <= a * b; joined <= a & b;
  roundtrip <= unsigned(signed(std_logic_vector(a)));
  eq <= '1' when a = b else '0';
end;
)");
  unsigned fullAdders = 0;
  for (auto* instance : design->getInstances())
    if (NLDB0::isFA(instance->getModel())) ++fullAdders;
  EXPECT_EQ(fullAdders, 50); // Two 4-bit chains and six 7-bit product accumulations.
  for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 8; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
    for (unsigned bit = 0; bit < 3; ++bit)
      values[design->getBusTerm(NLName("b"))->getBit(7-bit)->getNet()] = (b >> bit) & 1;
    const int sa = a < 8 ? int(a) : int(a)-16, sb = b < 4 ? int(b) : int(b)-8;
    for (const auto& [name, result] : std::vector<std::pair<std::string, int>>{
        {"sum", sa+sb}, {"diff", sa-sb}, {"product", sa*sb},
        {"joined", int((a << 3) | b)}, {"roundtrip", int(a)}}) {
      auto* term = design->getBusTerm(NLName(name));
      for (unsigned bit = 0; bit < term->getWidth(); ++bit)
        EXPECT_EQ(evaluateRTL(term->getBit(term->getLSB()+bit)->getNet(), values, visiting),
                  bool((unsigned(result) >> bit) & 1)) << name << ':' << sa << ':' << sb;
    }
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("eq"))->getNet(), values, visiting), sa == sb);
  }
}

TEST_F(VHDLConstructorTest, InvalidSignedArithmeticPublishesNoDesign) {
  for (const auto* expression : {"a + b", "a * a", "a & b", "signed(missing)"}) {
    SCOPED_TRACE(expression);
    const auto source = std::string(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n"
        "entity invalid_signed is port(a : in signed(2 downto 0); b : in unsigned(2 downto 0);\n"
        "y : out signed(2 downto 0)); end; architecture rtl of invalid_signed is begin\n"
        "y <= ") + expression + "; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, GroupsArrayBusesAndPartialRegisters) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity grouped is port(clk, sel : in bit; a, b : in bit_vector(2 to 5);
  y : out bit_vector(9 downto 6)); end;
architecture rtl of grouped is
  type words is array (3 downto 2) of bit_vector(-1 to 2);
  signal storage : words;
  signal partial : bit_vector(7 downto 2) := "101001";
begin
  storage(3) <= a;
  storage(2) <= b;
  y <= storage(3) when sel = '1' else storage(2);
  process(clk) begin
    if rising_edge(clk) then
      partial(7 downto 6) <= a(2 to 3);
      partial(3 downto 2) <= b(4 to 5);
    end if;
  end process;
  process(clk) begin
    if rising_edge(clk) then partial(5 downto 4) <= "00"; end if;
  end process;
end;
)");
  for (const auto* name : {"storage(3)", "storage(2)"}) {
    auto* bus = design->getBusNet(NLName(name));
    ASSERT_NE(bus, nullptr);
    EXPECT_EQ(bus->getMSB(), -1);
    EXPECT_EQ(bus->getLSB(), 2);
  }
  unsigned muxes = 0, flops = 0;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (NLDB0::isMux2(model)) {
      ++muxes;
      EXPECT_EQ(NLDB0::getMux2Output(model)->getWidth(), 4);
    }
    if (NLDB0::isDFF(model)) {
      ++flops;
      ASSERT_NE(model->getBusTerm(NLName("Q")), nullptr);
      EXPECT_EQ(model->getBusTerm(NLName("Q"))->getWidth(), 2);
      ASSERT_NE(instance->getInstParameter(NLName("INIT")), nullptr);
      const auto first = instance->getInstTerm(model->getBusTerm(NLName("Q"))->getBit(1))->getNet();
      EXPECT_EQ(instance->getInstParameter(NLName("INIT"))->getValue(),
          first == design->getBusNet(NLName("partial"))->getBit(3) ? "2'b01" : "2'b10");
    }
  }
  EXPECT_EQ(muxes, 1u);
  EXPECT_EQ(flops, 3u);
  for (unsigned pattern = 0; pattern < 512; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    values[design->getScalarTerm(NLName("sel"))->getNet()] = pattern & 1;
    for (unsigned i = 0; i < 4; ++i) {
      values[design->getBusTerm(NLName("a"))->getBit(2+i)->getNet()] = (pattern >> (1+i)) & 1;
      values[design->getBusTerm(NLName("b"))->getBit(2+i)->getNet()] = (pattern >> (5+i)) & 1;
    }
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(9-i)->getNet(), values, visiting),
          bool((pattern >> ((pattern & 1 ? 1 : 5) + i)) & 1));
    for (const auto& flop : dffBits(design)) {
      const auto index = static_cast<SNLBusNetBit*>(flop.output)->getBit();
      const auto inputPosition = index >= 6 ? 1 + 7 - index : 7 + 3 - index;
      EXPECT_EQ(evaluateRTL(flop.data, values, visiting), index == 5 || index == 4 ? false : bool((pattern >> inputPosition) & 1));
    }
  }
}

TEST_F(VHDLConstructorTest, InferredMemoryReadBeforeWriteAndEnableCycles) {
  auto* design = VHDLConstructor(library_).constructFile(SNL_VHDL_MEMORY);
  auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 1u);
  auto* memory = memories.begin()->first;
  const auto signature = NLDB0::getMemorySignature(memory);
  EXPECT_EQ(signature.width, 8u);
  EXPECT_EQ(signature.depth, 4u);
  EXPECT_EQ(signature.readPorts, 1u);
  EXPECT_EQ(signature.writePorts, 1u);
  EXPECT_EQ(signature.resetMode, NLDB0::MemoryResetMode::None);
  EXPECT_EQ(memory->getInstTerm(NLDB0::getMemoryClock(memory->getModel()))->getNet(),
      design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_TRUE(memory->getInstTerm(NLDB0::getMemoryReset(memory->getModel()))->getNet()->isConstant0());
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 12u); // Addresses and read register, no storage DFFs.
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  unsigned words[4]{}, writeAddress = 0, readAddress = 0, registered = 0;
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_MEMORY_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  for (unsigned cycle = 0; cycle < 80; ++cycle) {
    const bool reset = cycle == 0 || cycle == 17 || cycle == 39;
    const bool enable = cycle > 0 && (cycle < 5 || cycle % 5 != 0);
    const bool write = cycle < 5 || cycle % 3 != 0;
    const unsigned data = (cycle * 37 + 11) & 255;
    auto values = state;
    for (const auto& [name, value] : std::vector<std::pair<std::string, bool>>{
        {"ce", enable}, {"we", write}, {"reset", reset}})
      values[design->getScalarTerm(NLName(name))->getNet()] = value;
    for (unsigned bit = 0; bit < 8; ++bit)
      values[design->getBusTerm(NLName("d"))->getBit(bit)->getNet()] = (data >> bit) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    const auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
    for (const auto& flop : flops)
      state[flop.output] = evaluateRTL(flop.data, values, visiting, &memories);
    memories = nextMemories;
    if (enable) {
      registered = words[readAddress];
      if (write) words[writeAddress] = data;
    }
    if (reset) writeAddress = readAddress = 0;
    else if (enable) {
      if (write) writeAddress = (writeAddress + 1) % 4;
      readAddress = (readAddress + 1) % 4;
    }
    values = state;
    unsigned actual[2]{};
    for (unsigned output = 0; output < 2; ++output)
      for (unsigned bit = 0; bit < 8; ++bit)
        if (evaluateRTL(design->getBusTerm(NLName(output ? "async_q" : "q"))->getBit(bit)->getNet(),
                        values, visiting, &memories)) actual[output] |= 1u << bit;
    EXPECT_EQ(actual[0], registered) << cycle;
    EXPECT_EQ(actual[1], words[readAddress]) << cycle;
    if (reference.is_open() && cycle >= 6) {
      unsigned q, asynchronous;
      ASSERT_TRUE(reference >> q >> asynchronous);
      EXPECT_EQ(actual[0], q) << cycle;
      EXPECT_EQ(actual[1], asynchronous) << cycle;
    }
  }
  if (reference.is_open()) { std::string trailing; EXPECT_FALSE(reference >> trailing); }
  if (const auto* directory = std::getenv("VHDL_MEMORY_DUMP")) {
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.setTopFileName("memory_test.v");
    dumper.dumpDesign(design, directory);
  }
}

TEST_F(VHDLConstructorTest, InferredMemoryBoundsWordOrderAndReadPorts) {
  for (const auto& [bounds, low, high, width] :
       std::vector<std::tuple<std::string, unsigned, unsigned, unsigned>>{
           {"0 to 3", 0, 3, 4}, {"3 downto 1", 1, 3, 4},
           {"2 to 4", 2, 4, 4}, {"7 downto 7", 7, 7, 1}}) {
    SCOPED_TRACE(bounds);
    const auto wordRange = "4 to " + std::to_string(3 + width);
    auto* design = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all; "
        "use ieee.std_logic_unsigned.all; "
        "entity bounded_ram is port(clk, ce, we : in std_logic; "
        "wa, ra : in std_logic_vector(3 downto 0); d : in std_logic_vector(" + wordRange + "); "
        "q, direct, first_word : out std_logic_vector(" + wordRange + ")); end; "
        "architecture rtl of bounded_ram is type words is array(" + bounds + ") of std_logic_vector(" +
        wordRange + "); signal ram, copied : words; begin "
        "copied <= ram; direct <= ram(conv_integer(ra)); first_word <= copied(" + std::to_string(low) + "); "
        "process(clk) variable address : integer range 0 to 15; begin if rising_edge(clk) then "
        "address := conv_integer(wa); if ce = '1' then if we = '1' then ram(address) <= d; end if; "
        "q <= ram(conv_integer(ra)); end if; end if; end process; end;");
    auto memories = naja::NL::test::zeroMemoryState(design);
    ASSERT_EQ(memories.size(), 1u);
    const auto signature = NLDB0::getMemorySignature(memories.begin()->first);
    EXPECT_EQ(signature.width, width);
    EXPECT_EQ(signature.depth, high - low + 1);
    EXPECT_GE(signature.readPorts, 2u);
    const auto flops = dffBits(design);
    ASSERT_EQ(flops.size(), width);
    std::unordered_map<SNLBitNet*, bool> state;
    for (const auto& flop : flops) state[flop.output] = false;
    std::vector<unsigned> words(high - low + 1);
    unsigned registered = 0;
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
      // Includes indices outside the array and with nonzero truncated high bits.
      const auto wa = cycle % 16, ra = cycle % 3 ? wa : (cycle + 5) % 16;
      const unsigned data = (cycle * 3 + 1) & ((1u << width) - 1);
      const bool ce = cycle % 5 != 0, we = cycle % 7 != 0;
      auto values = state;
      values[design->getScalarTerm(NLName("ce"))->getNet()] = ce;
      values[design->getScalarTerm(NLName("we"))->getNet()] = we;
      for (const auto& [name, value] : std::vector<std::pair<std::string, unsigned>>{
          {"wa", wa}, {"ra", ra}, {"d", data}}) {
        auto* term = design->getBusTerm(NLName(name));
        for (unsigned i = 0; i < term->getWidth(); ++i)
          values[term->getBitAtPosition(i)->getNet()] = (value >> (term->getWidth()-1-i)) & 1;
      }
      const unsigned read = ra >= low && ra <= high ? words[ra-low] : 0;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned i = 0; i < width; ++i) {
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("direct"))->getBitAtPosition(i)->getNet(),
                             values, visiting, &memories), bool((read >> (width-1-i)) & 1));
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("first_word"))->getBitAtPosition(i)->getNet(),
                             values, visiting, &memories), bool((words.front() >> (width-1-i)) & 1));
      }
      const auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
      for (const auto& flop : flops)
        state[flop.output] = evaluateRTL(flop.data, values, visiting, &memories);
      memories = nextMemories;
      if (ce) {
        registered = read;
        if (we && wa >= low && wa <= high) words[wa-low] = data;
      }
      for (unsigned i = 0; i < width; ++i)
        EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBitAtPosition(i)->getNet()),
                  bool((registered >> (width-1-i)) & 1)) << cycle;
      EXPECT_EQ(memories.begin()->second, std::vector<uint64_t>(words.begin(), words.end()));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, MemoryInferenceHandlesInitializersAndRejectsUnsupportedWritePatterns) {
  for (const auto& [writes, initializer] : std::vector<std::pair<std::string, std::string>>{
      {"ram(conv_integer(a)) <= d; ram(conv_integer(b)) <= d;", ""},
      {"ram(conv_integer(a))(3 downto 2) <= d(3 downto 2); ram(conv_integer(a))(1 downto 0) <= d(1 downto 0);", ""},
      {"for i in 0 to 1 loop ram(conv_integer(a)) <= d; end loop;", ""},
      {"ram(conv_integer(a)) <= d;", " := (others => (others => '0'))"}}) {
    SCOPED_TRACE(writes);
    auto* design = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all; "
        "use ieee.std_logic_unsigned.all; entity fallback is port(clk : in std_logic; "
        "a, b : in std_logic_vector(1 downto 0); d : in std_logic_vector(3 downto 0); "
        "q : out std_logic_vector(3 downto 0)); end; architecture rtl of fallback is "
        "type words is array(0 to 3) of std_logic_vector(3 downto 0); signal ram : words" + initializer + "; "
        "begin q <= ram(conv_integer(a)); process(clk) begin if rising_edge(clk) then " +
        std::string(writes) + " end if; end process; end;");
    if (initializer.empty()) {
      EXPECT_TRUE(naja::NL::test::zeroMemoryState(design).empty());
      EXPECT_EQ(dffBits(design).size(), 16u);
    } else {
      const auto memories = naja::NL::test::zeroMemoryState(design);
      ASSERT_EQ(memories.size(), 1u);
      EXPECT_EQ(memories.begin()->first->getInstParameter(NLName("INIT_ENABLE"))->getValue(), "1");
      EXPECT_TRUE(dffBits(design).empty());
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RecordsPreserveFieldOrderAndScheduledWrites) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
package bus_types is
  type request_t is record
    valid : std_ulogic;
    data : std_ulogic_vector(2 to 4);
  end record request_t;
  type envelope_t is record
    request : request_t;
    tag : bit;
  end record;
  constant idle : request_t := (data => (others => '0'), valid => '0');
end;
library ieee; use ieee.std_logic_1164.all; use work.bus_types.all;
entity record_test is port(clk, valid : in std_ulogic;
  data : in std_ulogic_vector(7 downto 5); y : out std_ulogic_vector(3 downto 0)); end;
architecture rtl of record_test is
  type entries_t is array (1 downto 0) of envelope_t;
  signal entries : entries_t;
  signal state : request_t;
begin
  entries(1) <= (request => (valid, data), tag => '1');
  entries(0) <= (request => idle, tag => '0');
  process(clk) begin
    if rising_edge(clk) then
      state <= entries(1).request;
      state.data(3) <= entries(0).request.valid;
    end if;
  end process;
  y <= state.valid & state.data;
end;
)");
  ASSERT_NE(design, nullptr);
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 4u);
  auto* state = design->getBusNet(NLName("state"));
  ASSERT_NE(state, nullptr);
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("valid"))->getNet()] = pattern & 8;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("data"))->getBitAtPosition(i)->getNet()] = pattern & (4 >> i);
    for (const auto& flop : flops) {
      unsigned position = 0;
      while (state->getBitAtPosition(position) != flop.output) ++position;
      const bool expected = position == 2 ? false : bool(pattern & (8 >> position));
      EXPECT_EQ(evaluateRTL(flop.data, values, visiting), expected);
      values[flop.output] = expected;
    }
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBitAtPosition(i)->getNet(), values, visiting),
          i == 2 ? false : bool(pattern & (8 >> i)));
  }
}

TEST_F(VHDLConstructorTest, RecordPortsBindAcrossHierarchy) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type pair_t is record a, b : bit; end record; end;
use work.types.all;
entity leaf is port(d : in pair_t; q : out pair_t); end;
architecture rtl of leaf is begin q <= (b => d.a, a => d.b); end;
use work.types.all;
entity parent is port(d : in pair_t; q : out pair_t); end;
architecture rtl of parent is begin u: entity work.leaf port map(d, q); end;
)", "parent");
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(design->getBusTerm(NLName("d"))->getWidth(), 2u);
  auto* child = design->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* model = child->getModel();
  for (unsigned i = 0; i < 2; ++i) {
    EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("d"))->getBitAtPosition(i))->getNet(),
        design->getBusTerm(NLName("d"))->getBitAtPosition(i)->getNet());
  }
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  values[model->getBusTerm(NLName("d"))->getBitAtPosition(0)->getNet()] = false;
  values[model->getBusTerm(NLName("d"))->getBitAtPosition(1)->getNet()] = true;
  EXPECT_TRUE(evaluateRTL(model->getBusTerm(NLName("q"))->getBitAtPosition(0)->getNet(), values, visiting));
  EXPECT_FALSE(evaluateRTL(model->getBusTerm(NLName("q"))->getBitAtPosition(1)->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, RejectsMalformedOrIncompatibleRecords) {
  for (const auto* body : {
      "r <= (a => '0', a => '1');",
      "r <= (a => '0');",
      "r <= (missing => '0', b => '1');",
      "r <= (a => '0', '1');",
      "r.missing <= '0';",
      "r <= not d;",
      "r <= d and d;",
      "r <= other;",
      "r.a <= d.b(0);",
      "r <= ('0', '1', '0');"}) {
    SCOPED_TRACE(body);
    const auto source = std::string(R"(
package types is
  type pair_t is record a, b : bit; end record;
  type other_t is record a, b : bit; end record;
end;
use work.types.all;
entity invalid is port(d : in pair_t; other : in other_t; r : out pair_t); end;
architecture rtl of invalid is begin
)") + body + " end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  }
  for (const auto* declaration : {
      "type pair_t is record a, a : bit; end record;",
      "type pair_t is record end record;",
      "type pair_t is record a : bit; end record wrong;",
      "type pair_t is record a : pair_t; end record;"}) {
    SCOPED_TRACE(declaration);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is end; architecture rtl of invalid is ") +
        declaration + " begin end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, RecordDefaultsAndFieldPortActuals) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type pair_t is record a, b : bit; end record; end;
entity leaf is port(d : in bit; q : out bit); end;
architecture rtl of leaf is begin q <= d; end;
use work.types.all;
entity parent is port(d : in pair_t; q : out pair_t); end;
architecture rtl of parent is
  constant zero : pair_t := (others => '0');
begin
  u: entity work.leaf port map(d.a, q.b);
  q.a <= zero.b;
end;
)", "parent");
  auto* child = design->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->getInstTerm(child->getModel()->getScalarTerm(NLName("d")))->getNet(),
      design->getBusTerm(NLName("d"))->getBitAtPosition(0)->getNet());
  EXPECT_EQ(child->getInstTerm(child->getModel()->getScalarTerm(NLName("q")))->getNet(),
      design->getBusTerm(NLName("q"))->getBitAtPosition(1)->getNet());
  EXPECT_TRUE(assignSource(design->getBusTerm(NLName("q"))->getBitAtPosition(0)->getNet())->isConstant0());
}

TEST_F(VHDLConstructorTest, UnresolvedLogicRequiresVisibilityAndSingleDriver) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity unresolved is port(a : in std_ulogic; y : out std_ulogic); end;
architecture rtl of unresolved is begin y <= not a; end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  values[design->getScalarTerm(NLName("a"))->getNet()] = false;
  EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting));
  EXPECT_THROW(VHDLConstructor(library_).construct(
      "entity invisible is port(a : in std_ulogic; y : out std_ulogic); end; "
      "architecture rtl of invisible is begin y <= a; end;"), NLException);
  EXPECT_THROW(VHDLConstructor(library_).construct(
      "library ieee; use ieee.std_logic_1164.all; "
      "entity multiple is port(a : in std_ulogic; y : out std_ulogic); end; "
      "architecture rtl of multiple is begin y <= a; y <= '0'; end;"), NLException);
}

TEST_F(VHDLConstructorTest, StaticIntegerArrayParametersPreserveBoundsAndDeclarationScope) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity calculated is port(y : out bit_vector(2 downto 0)); end;
architecture rtl of calculated is
  type numbers is array(integer range <>) of integer;
  constant original : numbers(-2 to 0) := (4, -1, 3);
  function fold(data : numbers) return integer is
    variable result : integer := 0;
  begin
    for i in data'range loop result := result * 2 + data(i); end loop;
    if data'ascending and data'left = -2 and data'right = 0 and data'length(1) = 3 then
      return result;
    end if;
    return 0;
  end;
  function forward(original : numbers := original) return integer is
    constant data : integer := 99;
  begin return fold(data => original); end;
  function captured return integer is begin return fold(original); end;
  constant score : integer := forward;
begin
  g: if score = 17 and captured = score generate y <= "101"; end generate;
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  for (int bit = 0; bit < 3; ++bit)
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting), bit != 1);
}

TEST_F(VHDLConstructorTest, StaticPackageIntegerArrayParametersUseFormalIndexOrder) {
  auto* design = VHDLConstructor(library_).construct(R"(
package math is
  type numbers is array(natural range <>) of natural;
  function fold(data : numbers(6 downto 4)) return natural;
end;
package body math is
  function fold(data : numbers(6 downto 4)) return natural is
    variable result : natural := 0;
  begin
    for i in data'range loop result := result * 2 + data(i); end loop;
    if not data'ascending and data'low = 4 and data'high = 6 then return result; end if;
    return 0;
  end;
end;
use work.math.all;
entity calculated is port(y : out bit); end;
architecture rtl of calculated is
  constant data : numbers(1 to 3) := (2, 1, 4);
  constant score : natural := fold(data);
begin y <= '1' when score = 14 else '0'; end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, RejectsInvalidStaticIntegerArrayArguments) {
  for (const auto* actual : {"wrong_type", "short_data", "1", "data(3)", "data => data, data => data"}) {
    SCOPED_TRACE(actual);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is
  type numbers is array(natural range <>) of natural;
  type other_numbers is array(natural range <>) of natural;
  constant data : numbers(2 to 4) := (1, 2, 3);
  constant short_data : numbers(2 to 3) := (1, 2);
  constant wrong_type : other_numbers(2 to 4) := (1, 2, 3);
  function first(data : numbers(6 downto 4)) return natural is begin return data(6); end;
  constant result : natural := first(
)") + actual + "); begin y <= '1' when result = 1 else '0'; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  for (const auto* body : {"return data(9);", "return data(true);", "return data'length(2);",
                          "data(2) := 9; return 1;"}) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is
  type numbers is array(2 to 4) of natural;
  constant data : numbers := (1, 2, 3);
  function first(data : numbers) return natural is begin
)") + body + " end; constant result : natural := first(data); begin y <= '1'; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is
  type numbers is array(2 to 3) of natural;
  constant data : numbers := (1, 2);
  constant bias : natural := 5;
  function first(bias : numbers) return natural is begin return bias + 1; end;
  constant result : natural := first(data);
begin y <= '1'; end;
)"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, StaticPackageFunctionsElaborateBoundsAndValues) {
  const std::string package = R"(
package math is
  constant bias : natural := 2;
  function width(n : natural) return natural;
  function choose(c : boolean; t, f : natural) return natural;
  function accumulate(n : natural; step : integer := bias) return integer;
  function valid(n : integer) return boolean;
end;
package body math is
  function width(n : natural) return natural is
  begin
    for i in 0 to 31 loop
      if 2**i >= n then return i; end if;
    end loop;
    return 32;
  end function;
  function choose(c : boolean; t, f : natural) return natural is
  begin
    if c then return t; else return f; end if;
  end function;
  function accumulate(n : natural; step : integer) return integer is
    constant offset : integer := bias;
    variable sum : integer := offset;
    variable i : integer := 9;
  begin
    for i in n downto 1 loop
      sum := sum + step;
    end loop;
    for j in 2 to 1 loop sum := 999; end loop;
    return sum + i - 9;
  end function;
  function valid(n : integer) return boolean is
  begin return (n >= 0) and (n < 100); end function;
end;
)";
  for (unsigned n : {0u, 1u, 2u, 3u, 4u, 7u, 8u, 9u, 31u, 32u, 33u, 1024u}) {
    unsigned expectedWidth = 0;
    while ((1u << expectedWidth) < n) ++expectedWidth;
    const auto source = package + R"(
use work.math.all;
entity calculated is port(y : out bit_vector(choose(width()" + std::to_string(n) + R"() > 0, width()" +
        std::to_string(n) + R"(), 1)-1 downto 0)); end;
architecture rtl of calculated is
  constant step : natural := 99;
  constant count : integer := accumulate(step => 3, n => 2);
  constant ok : boolean := valid(count) and (accumulate(3) = 8);
begin
  y <= (others => '1') when ok else (others => '0');
end;
)";
    auto* trial = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    auto* design = VHDLConstructor(trial).construct(source, "calculated");
    auto* term = design->getBusTerm(NLName("y"));
    ASSERT_NE(term, nullptr);
    EXPECT_EQ(term->getWidth(), std::max(1u, expectedWidth));
    for (auto* bit : term->getBits()) EXPECT_TRUE(assignSource(bit->getNet())->isConstant1());
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RuntimePackageReductionsPreserveArgumentBoundsAndDefaults) {
  for (const auto* operation : {"or", "and", "xor"}) for (const bool ascending : {false, true}) {
    SCOPED_TRACE(std::string(operation) + (ascending ? " ascending" : " descending"));
    auto* library = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    const auto source = std::string(R"(
library ieee; use ieee.std_logic_1164.all;
package helpers is
  function fold_bits(data : std_ulogic_vector; invert : boolean := false) return std_ulogic;
end;
package body helpers is
  function fold_bits(data : std_ulogic_vector; invert : boolean := false) return std_ulogic is
    variable result : std_ulogic := ')") + (std::string(operation) == "and" ? "1" : "0") + R"(';
  begin
    for idx in data'range loop result := result )" + operation + R"( data(idx); end loop;
    if invert then result := not result; end if;
    return result;
  end;
end;
library ieee; use ieee.std_logic_1164.all; use work.helpers.all;
entity reduction is port(a : in std_ulogic_vector()" + (ascending ? "8 to 10" : "10 downto 8") + R"();
  y, inverted : out std_ulogic); end;
architecture rtl of reduction is
  constant invert : boolean := true;
begin
  y <= fold_bits(a);
  inverted <= fold_bits(invert => true, data => a);
end;
)";
    auto* design = VHDLConstructor(library).construct(source);
    for (unsigned pattern = 0; pattern < 8; ++pattern) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (int bit = 0; bit < 3; ++bit)
        values[design->getBusTerm(NLName("a"))->getBit(8 + bit)->getNet()] = (pattern >> bit) & 1;
      const bool expected = std::string(operation) == "or" ? pattern != 0 :
          std::string(operation) == "and" ? pattern == 7 : bool((pattern ^ (pattern >> 1) ^ (pattern >> 2)) & 1);
      EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), expected);
      EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("inverted"))->getNet(), values, visiting), !expected);
    }
  }
}

TEST_F(VHDLConstructorTest, RuntimeLocalVectorFunctionsLowerLoopsNestedCallsAndBranches) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity functions is port(a : in bit_vector(6 downto 4); select_i : in bit;
  y, repeated : out bit_vector(2 downto 0); chosen : out bit); end;
architecture rtl of functions is
  constant copies : natural := 3;
  function replicate(value : bit; count : natural := copies) return bit_vector is
    variable result : bit_vector(count-1 downto 0);
  begin
    for idx in result'range loop result(idx) := value; end loop;
    return result;
  end;
  function reverse_bits(data : bit_vector) return bit_vector is
    variable result : bit_vector(data'range);
  begin
    for idx in data'range loop result(idx) := data(data'left + data'right - idx); end loop;
    return result;
  end;
  function transform(data : bit_vector; choice : bit) return bit_vector is
    variable result : bit_vector(data'range);
  begin
    if choice = '1' then result := reverse_bits(data); else result := not data; end if;
    return result;
  end;
  function pick(data : bit_vector; choice : bit) return bit is
  begin
    if choice = '1' then return data(data'left); else return data(data'right); end if;
  end;
begin
  y <= transform(a, select_i);
  repeated <= replicate(a(4));
  chosen <= pick(choice => select_i, data => a);
end;
)");
  for (unsigned pattern = 0; pattern < 8; ++pattern) for (const bool select : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (int bit = 0; bit < 3; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(4 + bit)->getNet()] = (pattern >> bit) & 1;
    values[design->getScalarTerm(NLName("select_i"))->getNet()] = select;
    for (int bit = 0; bit < 3; ++bit) {
      const bool expected = select ? bool((pattern >> (2-bit)) & 1) : !bool((pattern >> bit) & 1);
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting), expected);
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("repeated"))->getBit(bit)->getNet(), values, visiting), bool(pattern & 1));
    }
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("chosen"))->getNet(), values, visiting),
        bool((pattern >> (select ? 2 : 0)) & 1));
  }
}

TEST_F(VHDLConstructorTest, RejectsIncompleteOrUnsafeRuntimeFunctions) {
  for (const auto* body : {
      "variable result : bit; begin return result; end;",
      "variable result : bit; begin if data(0) = '1' then result := '1'; end if; return result; end;",
      "begin null; end;",
      "begin if data(0) = '1' then return '1'; end if; null; end;",
      "begin return missing; end;",
      "begin return f(data); end;",
      "begin return data; end;"}) {
    SCOPED_TRACE(body);
    const auto source = std::string("entity invalid is port(a : in bit_vector(2 downto 0); y : out bit); end; ") +
        "architecture rtl of invalid is function f(data : bit_vector) return bit is " + body +
        " begin y <= f(a); end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, RuntimeFunctionScopesAndConstrainedFormals) {
  auto* design = VHDLConstructor(library_).construct(R"(
package helpers is
  constant bias : bit := '1';
  function adjust(data : bit_vector(2 to 4); enabled : bit := bias) return bit_vector;
end;
package body helpers is
  function hidden(value : bit) return bit is begin return value xor bias; end;
  function adjust(data : bit_vector(2 to 4); enabled : bit := bias) return bit_vector is
    variable result : bit_vector(data'range);
  begin
    for idx in data'range loop result(idx) := hidden(data(idx)) and enabled; end loop;
    return result;
  end;
end;
use work.helpers.all;
entity scopes is port(a : in bit_vector(9 downto 5); y : out bit_vector(2 downto 0)); end;
architecture rtl of scopes is constant enabled : bit := '0'; begin
  y <= adjust(a(7 downto 5));
end;
)");
  for (unsigned pattern = 0; pattern < 32; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (int bit = 0; bit < 5; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(5 + bit)->getNet()] = (pattern >> bit) & 1;
    for (int bit = 0; bit < 3; ++bit)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
          !bool((pattern >> bit) & 1));
  }
}

TEST_F(VHDLConstructorTest, RejectsRuntimeDependentFunctionReturnBounds) {
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(a : in bit_vector(2 downto 0); b : in bit_vector(4 to 6);
  choice : in bit; y : out bit_vector(2 downto 0)); end;
architecture rtl of invalid is
  function pick(left, right : bit_vector; choice : bit) return bit_vector is
  begin
    if choice = '1' then return left; else return right; end if;
  end;
begin y <= pick(a, b, choice); end;
)"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, BinaryFunctionsInitializeArchitectureConstants) {
  auto* design = VHDLConstructor(library_).construct(R"(
package helpers is
  function invert(value : bit) return bit;
end;
package body helpers is
  function invert(value : bit) return bit is begin return not value; end;
end;
use work.helpers.all;
entity initialized is port(a : in bit; y : out bit); end;
architecture rtl of initialized is constant preset : bit := invert('0');
begin y <= preset and invert(a); end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
  }
}

TEST_F(VHDLConstructorTest, RejectsRuntimeFunctionDefaultProfileMismatch) {
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
package helpers is function f(value : bit := '0') return bit; end;
package body helpers is function f(value : bit := '1') return bit is begin return value; end; end;
use work.helpers.all;
entity invalid is port(a : in bit; y : out bit); end;
architecture rtl of invalid is begin y <= f(a); end;
)"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, StaticFunctionArithmeticAndEarlyReturns) {
  const auto source = R"(
package math is
  function calculate(n : integer) return integer;
  function unused return boolean;
  function safe return boolean;
end;
package body math is
  function calculate(n : integer) return integer is
    variable value : integer range -20 to 20 := 0;
  begin
    if n = 0 then return (-7) mod 3;
    elsif n = 1 then return ((-7) rem 3) + 3;
    elsif n = 2 then return (-2**2) + 6;
    else
      value := 9 / 4;
      return value;
    end if;
  end function;
  function unused return boolean is begin return (1 / 0) = 0; end function;
  function safe return boolean is begin return false and unused; end function;
end;
use work.math.all;
entity arithmetic is port(y : out bit_vector(1 downto 0)); end;
architecture rtl of arithmetic is begin
  y <= "11" when (calculate(0) = 2) and (calculate(1) = 2) and
    (calculate(2) = 2) and (calculate(3) = 2) and not safe else "00";
end;
)";
  auto* design = VHDLConstructor(library_).construct(source);
  for (auto* bit : design->getBusTerm(NLName("y"))->getBits()) EXPECT_TRUE(assignSource(bit->getNet())->isConstant1());
}

TEST_F(VHDLConstructorTest, StaticBooleanConversionsPreserveShortCircuitEvaluation) {
  for (const auto& [expression, expected] : std::vector<std::pair<std::string, bool>>{
      {"false and boolean((1 / 0) = 0)", false},
      {"true or boolean((1 / 0) = 0)", true},
      {"false nand boolean((1 / 0) = 0)", true},
      {"true nor boolean((1 / 0) = 0)", false},
      {"true and boolean(3 > 1)", true},
      {"false or boolean(3 < 1)", false},
      {"boolean(not boolean(false))", true}}) {
    SCOPED_TRACE(expression);
    auto* library = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    const auto source =
        "package flags is function selected return boolean; end; "
        "package body flags is function selected return boolean is begin return " + expression +
        "; end; end; use work.flags.all; "
        "entity conversion is generic(enabled : boolean := " + expression + "); port(y : out bit); end; "
        "architecture rtl of conversion is constant chosen : boolean := " + expression +
        "; begin y <= '1' when boolean(chosen) and boolean(enabled) and boolean(selected) else '0'; end;";
    auto* design = VHDLConstructor(library).construct(source);
    auto* net = design->getScalarTerm(NLName("y"))->getNet();
    EXPECT_EQ(assignSource(net)->isConstant1(), expected);
    EXPECT_EQ(assignSource(net)->isConstant0(), !expected);
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RejectsNonBooleanConversionOperandsEvenWhenShortCircuited) {
  for (const auto* expression : {"boolean(1)", "false and boolean(1)", "true or boolean(1)"}) {
    SCOPED_TRACE(expression);
    const auto source = std::string("entity conversion is port(y : out bit); end; ") +
        "architecture rtl of conversion is constant chosen : boolean := " + expression +
        "; begin y <= '0'; end;";
    try {
      VHDLConstructor(library_).construct(source);
      FAIL() << "integer-to-boolean conversion accepted";
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find("boolean conversion requires a boolean operand"), std::string::npos);
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidStaticPackageFunctionCalls) {
  const auto prefix = R"(
package math is function f(n : natural; enabled : boolean := true) return natural; end;
package body math is
function f(n : natural; enabled : boolean) return natural is
begin if enabled then return n; else return 1; end if; end function;
end;
use work.math.all;
entity invalid is port(y : out bit_vector(3 downto 0)); end;
architecture rtl of invalid is constant value : integer :=
)";
  for (const auto* call : {"f(-1)", "f(true)", "f(1, 0)", "f", "f(1, true, 2)",
       "f(n => 1, n => 2)", "f(missing => 1)", "f(n => 1, false)", "f(2**63)", "f(1, false and 7)", "f(1, true or 0)"}) {
    SCOPED_TRACE(call);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(prefix) + call +
        "; begin y <= (others => '0'); end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, RejectsUnsafeOrIncompleteStaticFunctionBodies) {
  for (const auto* body : {
      "return -1;",
      "return true;",
      "return caller_only;",
      "if false then n <= 2; end if; return 1;",
      "return 1 / 0;",
      "return f(n);",
      "if n = 0 then return 1; end if;",
      "n := 2; return n;",
      "for i in 1 to 2 loop i := 0; end loop; return 1;",
      "for i in 0 to 65535 loop for j in 0 to 65535 loop end loop; end loop; return 1;"}) {
    SCOPED_TRACE(body);
    const auto source = std::string(R"(
package math is function f(n : natural) return natural; end;
package body math is function f(n : natural) return natural is begin
)") + body + R"( end function; end;
use work.math.all;
entity invalid is port(y : out bit); end;
architecture rtl of invalid is
  constant caller_only : integer := 7;
  constant value : integer := f(1);
begin y <= '0'; end;
)";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  }
  for (const auto* definitions : {
      "",
      "package body math is function f(n : boolean) return natural is begin return 1; end; end;",
      "package body math is function f(n : natural) return integer is begin return 1; end; end;",
      "package body math is impure function f(n : natural) return natural is begin return 1; end; end;",
      "package body math is function f(n : natural) return natural is begin return 1; end; "
      "function f(n : integer) return natural is begin return 2; end; end;",
      "package body math is function f(n : natural) return natural is variable v : positive := 0; "
      "begin return v; end; end;"}) {
    SCOPED_TRACE(definitions);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "package math is function f(n : natural) return natural; end; ") + definitions +
        " use work.math.all; entity invalid is port(y : out bit); end; "
        "architecture rtl of invalid is constant v : natural := f(1); begin y <= '0'; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, StaticFunctionsUsePrivateHelpersAndRecursiveFrames) {
  auto* design = VHDLConstructor(library_).construct(R"(
package math is function factorial(n : natural) return positive; end;
package body math is
  function multiply(a, b : integer) return integer is begin return a * b; end;
  function factorial(n : natural) return positive is
    variable saved : natural := n;
  begin
    if n = 0 then return 1; end if;
    return multiply(saved, factorial(n - 1));
  end;
end;
use work.math.all;
entity recursive is port(y : out bit_vector(factorial(4)-1 downto 0)); end;
architecture rtl of recursive is begin y <= (others => '1'); end;
)");
  EXPECT_EQ(design->getBusTerm(NLName("y"))->getWidth(), 24u);
  for (auto* bit : design->getBusTerm(NLName("y"))->getBits()) EXPECT_TRUE(assignSource(bit->getNet())->isConstant1());
}

TEST_F(VHDLConstructorTest, InterfaceDefaultsAndInvalidPortAttributeScope) {
  auto* defaultPort = VHDLConstructor(library_).construct(R"(
entity default_port is port(a : in bit := '1'; y : out bit); end;
architecture rtl of default_port is begin y <= a; end;
)");
  ASSERT_NE(defaultPort, nullptr);
  EXPECT_EQ(defaultPort->getName(), NLName("default_port"));
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity attribute_bounds is port(a : in bit_vector(3 downto 0); y : out bit_vector(a'range)); end;
architecture rtl of attribute_bounds is begin y <= a; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, ArrayAttributesPreserveBoundsAndLoopDirectionsWithoutReadingValues) {
  for (const bool ascending : {false, true}) {
    SCOPED_TRACE(ascending);
    const std::string bounds = ascending ? "4 to 6" : "6 downto 4";
    const auto source = std::string("entity attributes is port(a : in bit_vector(") +
        bounds + "); y : out bit_vector(" + bounds + R"(); valid : out bit); end;
architecture rtl of attributes is begin
  process(a)
    variable temporary : bit_vector(a'reverse_range(1));
  begin
    for i in temporary'range loop
      temporary(i) := a(a'left + a'right - i);
    end loop;
    y <= temporary;
  end process;
  valid <= '1' when a'length(1) = 3 and y'length = 3 and
    a'low = 4 and a'high = 6 and a'left = )" + (ascending ? "4" : "6") +
        " and a'right = " + (ascending ? "6" : "4") +
        " and a'ascending = " + (ascending ? "true" : "false") + " else '0'; end;";
    auto* design = VHDLConstructor(library_).construct(source);
    EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("valid"))->getNet())->isConstant1());
    for (unsigned pattern = 0; pattern < 8; ++pattern) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (int bit = 0; bit < 3; ++bit)
        values[design->getBusTerm(NLName("a"))->getBit(4 + bit)->getNet()] = (pattern >> bit) & 1;
      for (int bit = 0; bit < 3; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(4 + bit)->getNet(), values, visiting),
            bool((pattern >> bit) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, ArrayAttributesSelectRecordFieldsAndConstantTableBounds) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity attributes is port(a : in bit_vector(2 downto 0); y : out bit_vector(2 downto 0); valid : out bit); end;
architecture rtl of attributes is
  type entry_t is record data : bit_vector(7 to 9); end record;
  type rows_t is array(3 downto 2) of entry_t;
  type table_t is array(5 to 7) of integer;
  constant counts : table_t := (2, 4, 6);
  signal rows : rows_t;
begin
  rows(2).data <= a;
  rows(3).data <= a;
  y <= rows(rows'low).data(rows(3).data'left to rows(3).data'right);
  valid <= '1' when rows_t'length = 2 and rows(2).data'length = 3 and
    rows(2).data'ascending and counts'left = 5 and counts'right = 7 and counts'length = 3 else '0';
end;
)");
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("valid"))->getNet())->isConstant1());
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (int bit = 0; bit < 3; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (pattern >> bit) & 1;
    for (int bit = 0; bit < 3; ++bit)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
          bool((pattern >> bit) & 1));
  }
}

TEST_F(VHDLConstructorTest, ArrayRangeAttributesInGenerateBodies) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity attributes is generic(template : bit_vector(2 to 4) := "101");
  port(a : in bit_vector(template'range); y : out bit_vector(template'range)); end;
architecture rtl of attributes is begin
  lanes: for i in a'reverse_range generate
    signal temporary : bit_vector(a'length-1 downto 0);
  begin
    temporary <= (others => a(i));
    y(i) <= temporary(temporary'low);
  end generate;
end;
)");
  for (int bit = 2; bit <= 4; ++bit) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (int input = 2; input <= 4; ++input)
      values[design->getBusTerm(NLName("a"))->getBit(input)->getNet()] = input == bit;
    EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting));
  }
}

TEST_F(VHDLConstructorTest, ArrayAttributesRetainIndexTypesAndNullRangeDirection) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity attributes is port(y : out bit); end;
architecture rtl of attributes is
  type empty_t is array(2 to 1) of integer;
  type index_t is (first, second, last);
  type entries_t is array(last downto first) of bit;
begin
  y <= '1' when empty_t'length = 0 and empty_t'low = 2 and empty_t'high = 1 and
    empty_t'ascending and entries_t'length = 3 and not entries_t'ascending else '0';
end;
)");
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("y"))->getNet())->isConstant1());
  design->destroy();
  for (const auto* value : {"entries'low", "entries'high", "entries'left", "entries'right"}) {
    const auto source = std::string(R"(
entity unsupported is port(y : out bit); end;
architecture rtl of unsupported is
  type index_t is (first, second);
  type entries_t is array(index_t) of bit;
  signal entries : entries_t;
  constant bound : integer := )") + value + "; begin entries <= (others => '0'); y <= '0'; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, HierarchicalInterfacesResolveVectorGenericAttributes) {
  for (const bool component : {false, true}) {
    auto* library = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    const auto source = std::string(R"(
entity leaf is generic(template : bit_vector(5 downto 3) := "101");
  port(a : in bit_vector(template'range); y : out bit_vector(template'range)); end;
architecture rtl of leaf is begin y <= a; end;
entity parent is port(a : in bit_vector(2 downto 0); y : out bit_vector(2 downto 0)); end;
architecture rtl of parent is
)") + (component ? "component leaf is generic(template : bit_vector(5 downto 3) := \"101\"); port(a : in bit_vector(template'range); y : out bit_vector(template'range)); end component;" : "") +
        " begin u: " + (component ? "leaf" : "entity work.leaf") + " port map(a,y); end;";
    auto* design = VHDLConstructor(library).construct(source, "parent");
    auto* child = design->getInstance(NLName("u"));
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->getModel()->getBusTerm(NLName("y"))->getWidth(), 3u);
    for (int bit = 0; bit < 3; ++bit) {
      auto* input = child->getModel()->getBusTerm(NLName("a"))->getBit(bit + 3);
      auto* output = child->getModel()->getBusTerm(NLName("y"))->getBit(bit + 3);
      EXPECT_EQ(child->getInstTerm(input)->getNet(), design->getBusTerm(NLName("a"))->getBit(bit)->getNet());
      EXPECT_EQ(child->getInstTerm(output)->getNet(), design->getBusTerm(NLName("y"))->getBit(bit)->getNet());
    }
  }
}

TEST_F(VHDLConstructorTest, RejectsUnsupportedArrayAttributesWithoutPublishingDesigns) {
  for (const auto* value : {"a'length(0)", "a'length(2)", "y'length", "a'unknown", "a'range"}) {
    SCOPED_TRACE(value);
    const auto source = std::string("entity attributes is port(a : in bit_vector(2 downto 0); y : out bit); end; ") +
        "architecture rtl of attributes is constant count : integer := " + value + "; begin y <= '0'; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity attributes is port(a : in bit_vector(2 downto 0); y : out bit); end;
architecture rtl of attributes is
  function count return natural is begin return a'length; end;
  constant size : natural := count;
begin y <= '0'; end;
)"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, ComponentArrayTypeAttributesUseDeclarationBounds) {
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity leaf is generic(count : positive := 5);
  port(a : in bit_vector(4 downto 0); y : out bit_vector(4 downto 0)); end;
architecture rtl of leaf is begin y <= a; end;
entity parent is generic(count : positive := 3);
  port(a : in bit_vector(4 downto 0); y : out bit_vector(4 downto 0)); end;
architecture rtl of parent is
  type indices_t is array(0 to count-1) of integer;
  component leaf is generic(count : positive);
    port(a : in bit_vector(indices_t'length-1 downto 0); y : out bit_vector(indices_t'length-1 downto 0));
  end component;
begin u: leaf generic map(count => 5) port map(a,y); end;
)", "parent"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, AsynchronousResetPolarityValuesAndHoldCycles) {
  auto* design = VHDLConstructor(library_).constructFile(SNL_VHDL_ASYNCHRONOUS_RESET);
  ASSERT_NE(design, nullptr);
  std::unordered_map<SNLBitNet*, bool> state;
  for (unsigned i = 0; i < 4; ++i) {
    state[design->getBusNet(NLName("low_state"))->getBitAtPosition(i)] = (12 >> (3-i)) & 1;
    state[design->getBusNet(NLName("high_state"))->getBitAtPosition(i)] = (5 >> (3-i)) & 1;
  }
  state[design->getScalarNet(NLName("retained"))] = false;
  std::vector<SNLInstance*> flops;
  size_t resetLow = 0, resetHigh = 0, setHigh = 0, plain = 0;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (NLDB0::isDFFRN(model)) ++resetLow;
    else if (NLDB0::isDFFR(model)) ++resetHigh;
    else if (NLDB0::isDFFS(model)) ++setHigh;
    else if (NLDB0::isDFF(model)) ++plain;
    else continue;
    flops.push_back(instance);
    std::string initial;
    for (auto* bit : model->getTerm(NLName("Q"))->getBits())
      initial += state.at(instance->getInstTerm(bit)->getNet()) ? '1' : '0';
    ASSERT_NE(instance->getInstParameter(NLName("INIT")), nullptr);
    EXPECT_EQ(instance->getInstParameter(NLName("INIT"))->getValue(),
        NLDB0::formatDFFInitValue(initial.size(), initial));
  }
  EXPECT_EQ(resetLow, 1u);
  EXPECT_EQ(resetHigh, 1u);
  EXPECT_EQ(setHigh, 2u);
  EXPECT_EQ(plain, 1u);
  // Each entry changes inputs without necessarily generating a clock edge.
  const std::vector<std::tuple<bool, bool, bool, bool, unsigned>> events{
    {0,1,0,0,5}, {0,0,0,0,5}, {0,1,0,0,5}, {1,1,0,1,10},
    {0,1,0,1,4}, {0,0,1,1,4}, {1,0,1,1,4}, {0,0,1,1,4},
    {0,1,0,0,9}, {1,1,0,0,9}, {0,1,0,1,6}, {1,1,0,1,6},
    {1,0,0,1,6}, {1,1,0,1,6}};
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_ASYNC_RESET_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  bool previousClock = false, expectedHeld = false;
  unsigned expectedLow = 12, expectedHigh = 5;
  for (const auto& [clk, rstn, rst, enable, data] : events) {
    auto values = state;
    const auto inputs = [&] {
      for (const auto& [name, value] : std::vector<std::pair<const char*, bool>>{
          {"clk", clk}, {"rstn", rstn}, {"rst", rst}, {"enable", enable}})
        values[design->getScalarTerm(NLName(name))->getNet()] = value;
      for (unsigned i = 0; i < 4; ++i)
        values[design->getBusTerm(NLName("d"))->getBitAtPosition(i)->getNet()] = (data >> (3-i)) & 1;
    };
    inputs();
    std::unordered_set<SNLBitNet*> visiting;
    auto next = state;
    for (auto* flop : flops) {
      auto* model = flop->getModel();
      const bool set = NLDB0::isDFFS(model);
      bool reset = false;
      if (!NLDB0::isDFF(model)) {
        const bool low = NLDB0::isDFFRN(model);
        auto* pin = model->getScalarTerm(NLName(set ? "S" : low ? "RN" : "R"));
        reset = evaluateRTL(flop->getInstTerm(pin)->getNet(), values, visiting) != low;
      }
      auto outputs = model->getTerm(NLName("Q"))->getBits();
      auto dataBits = model->getTerm(NLName("D"))->getBits();
      auto input = dataBits.begin();
      for (auto* output : outputs) {
        auto* q = flop->getInstTerm(output)->getNet();
        if (reset) next[q] = set;
        else if (clk && !previousClock)
          next[q] = evaluateRTL(flop->getInstTerm(*input)->getNet(), values, visiting);
        ++input;
      }
    }
    if (!rstn) expectedLow = 3;
    else if (clk && !previousClock && enable) expectedLow = data;
    if (rst) expectedHigh = 12;
    else if (clk && !previousClock) { expectedHigh = data; expectedHeld = !expectedHeld; }
    state = next;
    values = state;
    inputs();
    visiting.clear();
    const auto read = [&](const char* name) {
      unsigned value = 0;
      for (auto* bit : design->getTerm(NLName(name))->getBits())
        value = (value << 1) | evaluateRTL(bit->getNet(), values, visiting);
      return value;
    };
    const auto q = read("q"), p = read("p"), held = read("held");
    EXPECT_EQ(q, expectedLow);
    EXPECT_EQ(p, expectedHigh);
    EXPECT_EQ(held, unsigned(expectedHeld));
    if (reference.is_open()) {
      unsigned referenceQ, referenceP, referenceHeld;
      ASSERT_TRUE(reference >> referenceQ >> referenceP >> referenceHeld);
      EXPECT_EQ(q, referenceQ);
      EXPECT_EQ(p, referenceP);
      EXPECT_EQ(held, referenceHeld);
    }
    previousClock = clk;
  }
  if (reference.is_open()) { std::string trailing; EXPECT_FALSE(reference >> trailing); }
  if (const auto* directory = std::getenv("VHDL_ASYNC_RESET_DUMP")) {
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.setTopFileName("async_reset_test.v");
    dumper.dumpDesign(design, directory);
  }
}

TEST_F(VHDLConstructorTest, AsynchronousResetOnlyStateAndMemoryFallback) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity reset_storage is port(clk, rst, d : in bit; q : out bit_vector(1 downto 0); sticky : out bit); end;
architecture rtl of reset_storage is
  type memory_t is array(0 to 1) of bit;
  signal memory : memory_t;
  signal address : integer range 0 to 1;
begin
  address <= 1;
  process(clk, rst) begin
    if rst = '1' then
      for i in 0 to 1 loop memory(i) <= '0'; end loop;
      sticky <= '1';
    elsif rising_edge(clk) then memory(address) <= d;
    end if;
  end process;
  q(1) <= memory(0);
  q(0) <= memory(1);
end;
)");
  size_t flops = 0;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    EXPECT_FALSE(NLDB0::isMemory(model));
    if (NLDB0::isDFFR(model) || NLDB0::isDFFS(model)) {
      flops += model->getTerm(NLName("Q"))->getWidth();
      if (NLDB0::isDFFS(model))
        EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("D")))->getNet(),
            instance->getInstTerm(model->getScalarTerm(NLName("Q")))->getNet());
    }
  }
  EXPECT_EQ(flops, 3u);
}

TEST_F(VHDLConstructorTest, RejectsUnsupportedAsynchronousResetSemantics) {
  const auto check = [&](const std::string& process, const std::string& diagnostic) {
    auto* trial = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    try {
      VHDLConstructor(trial).construct(
          "entity invalid is port(clk, rst, d : in bit; q : out bit); end; "
          "architecture rtl of invalid is begin " + process + " end;");
      FAIL() << "expected asynchronous-reset rejection";
    } catch (const NLException& exception) {
      EXPECT_NE(std::string(exception.what()).find(diagnostic), std::string::npos) << exception.what();
    }
  };
  check("process(clk) begin if rst = '1' then q <= '0'; elsif rising_edge(clk) then q <= d; end if; end process;",
      "present in sensitivity");
  check("process(clk) begin if clk = '1' then q <= '0'; elsif rising_edge(clk) then q <= d; end if; end process;",
      "distinct from the clock");
  check("process(clk, rst) begin if rst = '1' then q <= d; elsif rising_edge(clk) then q <= d; end if; end process;",
      "constant binary values");
  check("process(clk, rst) begin if rst = '1' then if d = '1' then q <= '0'; end if; "
      "elsif rising_edge(clk) then q <= d; end if; end process;", "constant binary values");
  check("process(clk, rst) begin if rst = '1' then q <= '0'; elsif rising_edge(clk) then q <= d; end if; "
      "q <= d; end process;", "outside an asynchronous clock/reset guard");
  check("process(clk, rst) begin if rst = 'X' then q <= '0'; elsif rising_edge(clk) then q <= d; end if; end process;",
      "compare a signal to '0' or '1'");
  check("process(clk, rst) begin if rst = '1' then q <= '0'; elsif rising_edge(clk) then q <= d; "
      "else q <= '1'; end if; end process;", "parse failed");
}

TEST_F(VHDLConstructorTest, ConditionalGenerateBranchesNestedLoopsAndRegisters) {
  const std::string source = R"(
entity choices is generic(mode : natural := MODE_VALUE);
  port(clk : in bit; d : in bit_vector(3 downto 0);
       y, q : out bit_vector(3 downto 0)); end;
architecture rtl of choices is
  constant inverted : boolean := mode = 1;
begin
  lanes: for i in 0 to 3 generate
    selection: if mode = 0 generate
      y(i) <= d(i);
    elsif inverted generate begin
      y(i) <= not d(i);
    elsif mode = 2 generate
      even_lane: if i mod 2 = 0 generate y(i) <= d(i); else generate y(i) <= not d(i); end generate;
    else generate
      one: for j in 0 to 0 generate y(i+j) <= '1'; end generate;
    end generate selection;
    registered: if mode mod 2 = 0 generate
      p: process(clk) begin if rising_edge(clk) then q(i) <= d(i); end if; end process;
    else generate
      p: process(clk) begin if rising_edge(clk) then q(i) <= not d(i); end if; end process;
    end generate;
  end generate lanes;
  omitted: if false generate y(99) <= d(99); end generate;
end;
)";
  for (unsigned mode = 0; mode < 4; ++mode) {
    SCOPED_TRACE(mode);
    auto specialized = source;
    specialized.replace(specialized.find("MODE_VALUE"), 10, std::to_string(mode));
    auto* library = NLLibrary::create(library_->getDB(), NLName("choices_" + std::to_string(mode)));
    auto* design = VHDLConstructor(library).construct(specialized);
    ASSERT_NE(design, nullptr);
    unsigned flops = 0;
    for (auto* instance : design->getInstances()) if (NLDB0::isDFF(instance->getModel())) ++flops;
    EXPECT_EQ(flops, 4u);
    for (unsigned pattern = 0; pattern < 16; ++pattern) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned i = 0; i < 4; ++i)
        values[design->getBusTerm(NLName("d"))->getBit(i)->getNet()] = (pattern >> i) & 1;
      for (unsigned i = 0; i < 4; ++i) {
        const bool bit = (pattern >> i) & 1;
        const bool expected = mode == 0 ? bit : mode == 1 ? !bit : mode == 2 ? bit != bool(i % 2) : true;
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), expected);
      }
      for (auto* instance : design->getInstances()) {
        auto* model = instance->getModel();
        if (!NLDB0::isDFF(model)) continue;
        auto* q = instance->getInstTerm(model->getScalarTerm(NLName("Q")))->getNet();
        for (unsigned i = 0; i < 4; ++i) if (q == design->getBusTerm(NLName("q"))->getBit(i)->getNet()) {
          EXPECT_EQ(evaluateRTL(instance->getInstTerm(model->getScalarTerm(NLName("D")))->getNet(), values, visiting),
                    bool((pattern >> i) & 1) != bool(mode % 2));
        }
      }
    }
  }
}

TEST_F(VHDLConstructorTest, ConditionalGenerateHierarchyInfersTopAndSelectsOneBranch) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity left_leaf is port(a : in bit; y : out bit); end;
architecture rtl of left_leaf is begin y <= a; end;
entity right_leaf is port(a : in bit; y : out bit); end;
architecture rtl of right_leaf is begin y <= not a; end;
entity choices is generic(n : natural := 2); port(a : in bit; y : out bit); end;
architecture rtl of choices is begin
  g: if n = 0 generate
    u: entity work.left_leaf port map(a, y);
  elsif n > 0 generate
    h: for i in 0 to 0 generate u: entity work.right_leaf port map(a, y); end generate;
  else generate
    u: entity work.left_leaf port map(a, y);
  end generate;
end;
)");
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(design->getName(), NLName("choices"));
  auto* instance = design->getInstance(NLName("g.h[0].u"));
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(instance->getModel()->getName(), NLName("right_leaf"));
  EXPECT_EQ(design->getInstances().size(), 1u);
  EXPECT_EQ(library_->getSNLDesign(NLName("left_leaf")), nullptr);
}

TEST_F(VHDLConstructorTest, RejectsInvalidConditionalGenerateElaboration) {
  for (const auto* body : {
      "g: if d = '1' generate y <= d; end generate;",
      "g: if 1 generate y <= d; end generate;",
      "g: if false generate y <= d; end generate;",
      "g: if true generate y <= d; end generate; g: if false generate end generate;",
      "g: if true generate y <= d; end generate; h: if true generate y <= not d; end generate;"}) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(d : in bit; y : out bit); end; architecture rtl of invalid is begin ") +
        body + " end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, ConditionalGenerateMemoryWritesRetainRegisterLowering) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity storage is port(clk, d : in bit; y : out bit_vector(1 downto 0)); end;
architecture rtl of storage is
  type memory_t is array(0 to 1) of bit;
  signal memory : memory_t;
  signal address : integer range 0 to 1;
begin
  address <= 1;
  chosen: if true generate
    process(clk) begin if rising_edge(clk) then memory(address) <= d; end if; end process;
  else generate
    process(clk) begin if rising_edge(clk) then memory(address) <= not d; end if; end process;
  end generate;
  y(0) <= memory(0); y(1) <= memory(1);
end;
)");
  ASSERT_NE(design, nullptr);
  unsigned bits = 0;
  for (auto* instance : design->getInstances()) {
    EXPECT_FALSE(NLDB0::isMemory(instance->getModel()));
    if (NLDB0::isDFF(instance->getModel())) bits += instance->getModel()->getTerm(NLName("Q"))->getWidth();
  }
  EXPECT_EQ(bits, 2u);
}

TEST_F(VHDLConstructorTest, GenerateLocalSignalsTypesAndStateStayIndependent) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity scoped is port(clk : in bit; d : in bit_vector(3 downto 0); y : out bit_vector(3 downto 0)); end;
architecture rtl of scoped is begin
  lanes: for lane in 0 to 1 generate
    constant width : positive := 2;
    constant invert : boolean := lane = 1;
    type cell_t is record data : bit_vector(width-1 downto 0); end record;
    type memory_t is array(0 to 1) of bit_vector(width-1 downto 0);
    signal memory : memory_t := (others => (others => '0'));
    signal cell : cell_t;
  begin
    choice: if invert generate
      signal data : bit_vector(width-1 downto 0);
    begin
      data <= not d(lane*width+width-1 downto lane*width);
      cell.data <= data;
    else generate
      signal data : bit_vector(width-1 downto 0);
    begin
      data <= d(lane*width+width-1 downto lane*width);
      cell.data <= data;
    end generate;
    process(clk) begin
      if rising_edge(clk) then
        memory(0) <= cell.data;
        memory(1) <= memory(0);
      end if;
    end process;
    y(lane*width+width-1 downto lane*width) <= memory(1);
  end generate;
end;
)");
  ASSERT_NE(design, nullptr);
  for (unsigned lane = 0; lane < 2; ++lane) {
    auto prefix = "lanes[" + std::to_string(lane) + "].";
    EXPECT_NE(design->getBusNet(NLName(prefix + "memory(0)")), nullptr);
    EXPECT_NE(design->getBusNet(NLName(prefix + "choice.data")), nullptr);
  }
  const auto flops = test::dffBits(design);
  ASSERT_EQ(flops.size(), 8u);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  for (auto* instance : design->getInstances()) {
    EXPECT_FALSE(NLDB0::isMemory(instance->getModel()));
    if (NLDB0::isDFF(instance->getModel())) {
      ASSERT_NE(instance->getInstParameter(NLName("INIT")), nullptr);
      EXPECT_EQ(instance->getInstParameter(NLName("INIT"))->getValue(), NLDB0::formatDFFInitValue(2, "00"));
    }
  }
  unsigned previous = 0;
  for (unsigned input : {1u, 10u, 7u, 3u, 12u, 0u, 15u}) {
    auto values = state;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i)
      values[design->getBusTerm(NLName("d"))->getBit(i)->getNet()] = (input >> i) & 1;
    auto next = state;
    for (const auto& flop : flops) next[flop.output] = evaluateRTL(flop.data, values, visiting);
    state = next;
    values = state;
    visiting.clear();
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting),
                bool((previous >> i) & 1));
    previous = input ^ 12;
  }
}

TEST_F(VHDLConstructorTest, GenerateLocalConstantsAndWidthsRestoreBetweenScopes) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity scoped is port(y : out bit_vector(2 downto 0)); end;
architecture rtl of scoped is begin
  lanes: for lane in 0 to 2 generate
    constant width : positive := lane + 1;
    type table_t is array(0 to 1) of integer;
    constant table : table_t := (0, lane);
    signal data : bit_vector(width-1 downto 0);
  begin
    data <= (others => '1');
    y(lane) <= data(table(1));
  end generate;
  unused: if false generate
    constant width : integer := 1 / 0;
    signal data : bit_vector(width-1 downto 0);
  begin
    data <= (others => '0');
  end generate;
end;
)");
  ASSERT_NE(design, nullptr);
  for (unsigned lane = 0; lane < 3; ++lane) {
    auto* bus = design->getBusNet(NLName("lanes[" + std::to_string(lane) + "].data"));
    ASSERT_NE(bus, nullptr);
    EXPECT_EQ(bus->getWidth(), lane + 1);
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(lane)->getNet(), values, visiting));
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidGenerateLocalDeclarationsWithoutPublishing) {
  for (const auto* body : {
      "g: if true generate signal local : bit; begin y <= local; end generate;",
      "g: if true generate signal local : bit; begin local <= d; end generate; y <= local;",
      "g: if true generate constant n : integer := 1; begin end generate; h: if n = 1 generate y <= d; end generate;",
      "g: if true generate signal local : bit_vector(n-1 downto 0); constant n : integer := 2; begin y <= local(0); end generate;",
      "g: if true generate signal d : bit; begin y <= d; end generate;",
      "g: if true generate signal local, local : bit; begin y <= local; end generate;",
      "g: if true generate type local_t is array(0 to 1) of bit; begin end generate; h: if true generate signal local : local_t; begin y <= local(0); end generate;",
      "g: if true generate subtype local_t is missing; begin y <= d; end generate;"}) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(d : in bit; y : out bit); end; architecture rtl of invalid is begin ") +
        body + " end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, CombinationalProcessDefaultsOverridesAndVariables) {
  const std::string source = R"(
entity combinational is port(d : in bit_vector(3 downto 0); en, force_high : in bit;
  y : out bit_vector(3 downto 0); z : out bit_vector(0 to 3)); end;
architecture rtl of combinational is begin
  process(SENSITIVITY) variable temp : bit_vector(3 downto 0); begin
    temp := d;
    for i in 0 to 3 loop
      if en = '1' then temp(i) := not temp(i); end if;
    end loop;
    y <= (others => '0');
    if en = '1' then y <= temp;
    elsif force_high = '1' then y(1 downto 0) <= d(1 downto 0); end if;
    if force_high = '1' then y(3) <= '1'; end if;
  end process;
  g: if true generate
    process(all) begin
      if en = '1' then z <= d; else z <= not d; end if;
    end process;
  end generate;
end;
)";
  for (const auto* sensitivity : {"all", "d, en, force_high"}) {
    auto text = source;
    text.replace(text.find("SENSITIVITY"), 11, sensitivity);
    auto* library = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    auto* design = VHDLConstructor(library).construct(text);
    ASSERT_NE(design, nullptr);
    for (auto* instance : design->getInstances())
      EXPECT_TRUE(NLDB0::isAssign(instance->getModel()) ||
          NLDB0::isGate(instance->getModel()) || NLDB0::isFA(instance->getModel()) ||
          NLDB0::isMux2(instance->getModel()));
    for (unsigned pattern = 0; pattern < 64; ++pattern) {
      const auto d = pattern & 15;
      const bool en = pattern & 16, force = pattern & 32;
      unsigned expected = en ? d ^ 15 : force ? d & 3 : 0;
      if (force) expected |= 8;
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("d"))->getBit(i)->getNet()] = (d >> i) & 1;
      values[design->getScalarTerm(NLName("en"))->getNet()] = en;
      values[design->getScalarTerm(NLName("force_high"))->getNet()] = force;
      for (unsigned i = 0; i < 4; ++i) {
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((expected >> i) & 1));
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(3-i)->getNet(), values, visiting), bool(((en ? d : d ^ 15) >> i) & 1));
      }
    }
  }
}

TEST_F(VHDLConstructorTest, RejectsIncompleteOrEventDependentCombinationalProcesses) {
  for (const auto* body : {
      "process(all) begin if en = '1' then y <= d; end if; end process;",
      "process(en) begin y <= d; end process;",
      "process(all) variable temp : bit; begin if en = '1' then temp := d; end if; y <= temp; end process;",
      "process(all) variable temp : bit := '0'; begin y <= temp; end process;",
      "process(all) begin state <= not state; y <= state; end process;",
      "process(all) begin y <= d; wait; end process;",
      "process(all) begin y <= d; end process; y <= en;"}) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(d, en : in bit; y : out bit); end; architecture rtl of invalid is signal state : bit; begin ") + body + " end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, CaseChoicesNestedDecodeAndClockedHold) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity case_probe is port(clk, en : in bit; sel : in bit_vector(1 downto 0);
  d : in bit_vector(3 downto 0); y, q : out bit_vector(3 downto 0)); end;
architecture rtl of case_probe is
  signal reg : bit_vector(3 downto 0) := "0000";
begin
  process(all) variable temp : bit_vector(3 downto 0); begin
    temp := "1111";
    case sel is
      when "00" | "10" => temp := d;
      when "01" =>
        case en is when '0' => temp := not d; when '1' => temp := "0000"; end case;
      when others => null;
    end case;
    y <= temp;
  end process;
  process(clk) begin if rising_edge(clk) then
    case sel is
      when "00" => reg <= d;
      when "01" | "10" => reg <= not d;
      when others => null;
    end case;
  end if; end process;
  q <= reg;
end;
)");
  ASSERT_NE(design, nullptr);
  const auto flops = test::dffBits(design);
  ASSERT_EQ(flops.size(), 4u);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  unsigned stored = 0;
  for (unsigned pattern = 0; pattern < 128; ++pattern) {
    const unsigned d = pattern & 15, sel = (pattern >> 4) & 3;
    const bool en = pattern & 64;
    auto values = state;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("en"))->getNet()] = en;
    for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("d"))->getBit(i)->getNet()] = (d >> i) & 1;
    for (unsigned i = 0; i < 2; ++i) values[design->getBusTerm(NLName("sel"))->getBit(i)->getNet()] = (sel >> i) & 1;
    const unsigned expected = sel == 0 || sel == 2 ? d : sel == 1 ? (en ? 0 : d ^ 15) : 15;
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((expected >> i) & 1));
    auto next = state;
    for (const auto& flop : flops) next[flop.output] = evaluateRTL(flop.data, values, visiting);
    state = next;
    if (sel != 3) stored = sel == 0 ? d : d ^ 15;
    values = state; visiting.clear();
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("q"))->getBit(i)->getNet(), values, visiting), bool((stored >> i) & 1));
  }
}

TEST_F(VHDLConstructorTest, CaseIntegerRangesBooleanAndExhaustiveAssignments) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity ranged is port(a : in bit_vector(1 downto 0); y, z : out bit); end;
architecture rtl of ranged is begin
  process(all) variable index : integer range 0 to 3; begin
    index := 0;
    if a(0) = '1' then index := 1; end if;
    if a(1) = '1' then index := index + 2; end if;
    case index is when 0 to 1 => y <= '0'; when 3 downto 2 => y <= '1'; when others => y <= '0'; end case;
    case a(0) = '1' is when true => z <= '1'; when false => z <= '0'; end case;
  end process;
end;
)");
  ASSERT_NE(design, nullptr);
  for (unsigned pattern = 0; pattern < 4; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 2; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (pattern >> i) & 1;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), bool(pattern & 2));
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("z"))->getNet(), values, visiting), bool(pattern & 1));
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidCaseChoicesAndIncompleteAssignments) {
  for (const auto* body : {
      "case a is when \"00\" => y <= '0'; when \"00\" => y <= '1'; when others => y <= '0'; end case;",
      "case a is when \"00\" => y <= '0'; end case;",
      "case a is when a => y <= '0'; when others => y <= '1'; end case;",
      "case a is when \"0\" => y <= '0'; when others => y <= '1'; end case;",
      "case a is when \"00\" => y <= '0'; when others => null; end case;",
      "case 2 is when 0 to 2 => y <= '0'; when 2 to 3 => y <= '1'; when others => y <= '0'; end case;",
      "case 2 is when 0 to 5000 => y <= '0'; when others => y <= '0'; end case;"}) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(a : in bit_vector(1 downto 0); y : out bit); end; architecture rtl of invalid is begin process(all) begin ") +
        body + " end process; end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, EnumerationRecordStateAndArrayHistoryCycles) {
  auto* design = VHDLConstructor(library_).construct(R"(
package enum_types is
  type phase_t is (idle, running, done);
  type machine_t is record phase : phase_t; end record;
  type history_t is array(0 to 1) of phase_t;
end;
use work.enum_types.all;
entity enum_probe is port(clk, rst, en : in bit; y : out bit_vector(1 downto 0); previous_done : out bit); end;
architecture rtl of enum_probe is
  signal machine : machine_t := (phase => idle);
  signal history : history_t := (others => idle);
begin
  process(clk) begin if rising_edge(clk) then
    if rst = '1' then
      machine.phase <= idle; history <= (others => idle);
    else
      history(0) <= machine.phase; history(1) <= history(0);
      case machine.phase is
        when idle => if en = '1' then machine.phase <= running; end if;
        when running => machine.phase <= done;
        when done => if en = '1' then machine.phase <= idle; end if;
      end case;
    end if;
  end if; end process;
  process(all) begin
    case machine.phase is
      when idle => y <= "00";
      when running => y <= "01";
      when done => y <= "10";
    end case;
  end process;
  previous_done <= '1' when history(1) = done else '0';
end;
)");
  ASSERT_NE(design, nullptr);
  const auto flops = test::dffBits(design);
  ASSERT_EQ(flops.size(), 6u);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  for (auto* instance : design->getInstances()) if (NLDB0::isDFF(instance->getModel())) {
    auto width = instance->getModel()->getTerm(NLName("Q"))->getWidth();
    ASSERT_NE(instance->getInstParameter(NLName("INIT")), nullptr);
    EXPECT_EQ(instance->getInstParameter(NLName("INIT"))->getValue(), NLDB0::formatDFFInitValue(width, std::string(width, '0')));
  }
  unsigned phase = 0, history0 = 0, history1 = 0;
  for (unsigned cycle = 0; cycle < 24; ++cycle) {
    const bool reset = cycle == 0 || cycle == 11;
    const bool enable = cycle % 3 != 0;
    auto values = state;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("rst"))->getNet()] = reset;
    values[design->getScalarTerm(NLName("en"))->getNet()] = enable;
    auto next = state;
    for (const auto& flop : flops) next[flop.output] = evaluateRTL(flop.data, values, visiting);
    state = next;
    if (reset) phase = history0 = history1 = 0;
    else {
      history1 = history0; history0 = phase;
      if (phase == 1) phase = 2;
      else if (enable) phase = phase == 0 ? 1 : 0;
    }
    values = state; visiting.clear();
    for (unsigned i = 0; i < 2; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((phase >> i) & 1));
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("previous_done"))->getNet(), values, visiting), history1 == 2);
  }
}

TEST_F(VHDLConstructorTest, GenerateLocalEnumerationLiteralsDoNotEscape) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity enum_local is port(y : out bit_vector(1 downto 0)); end;
architecture rtl of enum_local is begin
  g: for i in 0 to 1 generate
    type local_t is (low, high);
    constant choice : local_t := high;
    signal state : local_t;
  begin
    state <= choice;
    y(i) <= '1' when state /= low else '0';
  end generate;
end;
)");
  ASSERT_NE(design, nullptr);
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  for (auto* bit : design->getTerm(NLName("y"))->getBits()) EXPECT_TRUE(evaluateRTL(bit->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, RejectsInvalidOrMixedEnumerationTypes) {
  for (const auto* body : {
      "x <= other_a; y <= '0';",
      "x <= first; y <= '1' when x = other_a else '0';",
      "x <= not first; y <= '0';",
      "x <= first and second; y <= '0';",
      "x <= 0; y <= '0';",
      "x <= first; process(all) begin case x is when first => y <= '0'; when second => y <= '1'; end case; end process;"}) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(y : out bit); end; architecture rtl of invalid is "
        "type state_t is (first, second, third); type other_t is (other_a, other_b, other_c); signal x : state_t; begin ") + body + " end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
  for (const auto* declarations : {"type state_t is (a, a);",
       "type state_t is (state_t);", "type state_t is (a, b); signal state_t : bit;"}) {
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(y : out bit); end; architecture rtl of invalid is ") + declarations + " begin y <= '0'; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, NestedRecordSensitivityCoversOnlySelectedBits) {
  const std::string source = R"(
entity field_probe is port(d : in bit_vector(3 downto 0); en, unused : in bit;
  y : out bit_vector(3 downto 0); pick : out bit); end;
architecture rtl of field_probe is
  type inner_t is record data : bit_vector(3 downto 0); flag : bit; end record;
  type packet_t is record inner : inner_t; other : bit; end record;
  signal packet : packet_t;
begin
  packet.inner.data <= d; packet.inner.flag <= en; packet.other <= unused;
  process(SENSITIVITY) variable index : integer range 0 to 3; begin
    y <= (others => '0'); index := 0;
    if packet.inner.flag = '1' then y <= packet.inner.data; index := 3; end if;
    pick <= packet.inner.data(index);
  end process;
end;
)";
  for (const auto* sensitivity : {"packet.inner.data, packet.inner.flag", "packet.inner", "packet", "all"}) {
    SCOPED_TRACE(sensitivity);
    auto text = source;
    text.replace(text.find("SENSITIVITY"), 11, sensitivity);
    auto* library = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
    auto* design = VHDLConstructor(library).construct(text);
    ASSERT_NE(design, nullptr);
    for (unsigned pattern = 0; pattern < 64; ++pattern) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("d"))->getBit(i)->getNet()] = (pattern >> i) & 1;
      const bool en = pattern & 16;
      values[design->getScalarTerm(NLName("en"))->getNet()] = en;
      values[design->getScalarTerm(NLName("unused"))->getNet()] = pattern & 32;
      for (unsigned i = 0; i < 4; ++i)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), en && bool((pattern >> i) & 1));
      EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("pick"))->getNet(), values, visiting), bool((pattern >> (en ? 3 : 0)) & 1));
    }
  }
  for (const auto* sensitivity : {"packet.inner.data", "packet.inner.flag", "packet.other", "packet.missing", "d.inner"}) {
    SCOPED_TRACE(sensitivity);
    auto text = source;
    text.replace(text.find("SENSITIVITY"), 11, sensitivity);
    EXPECT_THROW(VHDLConstructor(library_).construct(text), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("field_probe")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, RecordSensitivityAlsoChecksAssignmentsOutsideClockGuard) {
  const std::string source = R"(
entity mixed is port(clk, d : in bit; q, y : out bit); end;
architecture rtl of mixed is
  type packet_t is record a, b : bit; end record;
  signal packet : packet_t;
begin
  packet.a <= d; packet.b <= not d;
  process(clk, packet.a) begin
    if rising_edge(clk) then q <= d; end if;
    y <= packet.a;
  end process;
end;
)";
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(test::dffBits(design).size(), 1u);
  for (bool d : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("d"))->getNet(), d}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), d);
  }
  auto invalid = source;
  invalid.replace(invalid.find("y <= packet.a"), 13, "y <= packet.b");
  auto* library = NLLibrary::create(library_->getDB(), NLLibrary::Type::Standard);
  EXPECT_THROW(VHDLConstructor(library).construct(invalid), NLException);
  EXPECT_EQ(library->getSNLDesign(NLName("mixed")), nullptr);
}

TEST_F(VHDLConstructorTest, ArchitectureStaticFunctionsCaptureDeclarationScope) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity local_functions is generic(base : positive := 3); port(y : out bit_vector(5 downto 0)); end;
architecture rtl of local_functions is
  constant bias : integer := base;
  function add(n : integer := bias) return integer is
  begin return n + bias; end function;
  function check(bias : integer) return boolean is
    variable result : integer := add;
  begin return result = 6 and add(n => bias) = 4; end function;
  signal data : bit_vector(add-1 downto 0);
begin
  data <= (others => '1') when check(1) else (others => '0');
  y <= data;
end;
)");
  ASSERT_NE(design, nullptr);
  for (auto* bit : design->getBusTerm(NLName("y"))->getBits())
    EXPECT_TRUE(assignSource(bit->getNet())->isConstant1());
}

TEST_F(VHDLConstructorTest, RejectsUnsupportedArchitectureFunctionScopes) {
  for (const auto* declarations : {
      "function f return integer is begin return later; end; constant later : integer := 1;",
      "function f return integer;",
      "function f return integer is begin return 1; end; function f return integer is begin return 2; end;",
      "impure function f return integer is begin return 1; end;",
      "function f return integer is begin return caller_local; end; function g return integer is variable caller_local : integer := 1; begin return f; end; constant x : integer := g;"
  }) {
    SCOPED_TRACE(declarations);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
      "entity local_bad is port(y : out bit); end; architecture rtl of local_bad is ") +
      declarations + " begin y <= '1' when f = 1 else '0'; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, StaticFunctionExitsPreserveNestedLoopAndReturnFlow) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity loop_exit is port(y : out bit); end;
architecture rtl of loop_exit is
  function count(limit : natural) return natural is
    variable total : natural := 0;
    variable i : integer := 17;
  begin
    for i in 0 to 3 loop
      for j in 4 downto 0 loop
        exit when j < limit;
        total := total + 1;
        if j = 2 then exit; end if;
        total := total + 10;
      end loop;
      exit when i = 1;
    end loop;
    return total + i;
  end;
  function early return natural is
  begin
    for i in 1 to 2 loop
      for j in 1 to 2 loop
        exit when false;
        return 7;
      end loop;
    end loop;
    return 0;
  end;
begin y <= '1' when count(3) = 61 and count(0) = 63 and early = 7 else '0'; end;
)");
  ASSERT_NE(design, nullptr);
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("y"))->getNet())->isConstant1());
}

TEST_F(VHDLConstructorTest, RejectsInvalidFunctionExits) {
  for (const auto* body : {
      "if false then exit; end if; return 0;",
      "for i in 0 to 1 loop exit when 1; end loop; return 0;",
      "for i in 0 to 1 loop exit outer_loop; end loop; return 0;",
      "for i in 0 to 1 loop exit; end loop;"
  }) {
    SCOPED_TRACE(body);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
      "entity bad_exit is port(y : out bit); end; architecture rtl of bad_exit is "
      "function f return integer is begin ") + body +
      " end; begin y <= '1' when f = 0 else '0'; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, LiteralPortActualsPreserveFormalBitOrder) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity literal_leaf is port(a : in bit; b : in bit_vector(2 to 5); y : out bit); end;
architecture rtl of literal_leaf is begin y <= a xor b(2); end;
entity literal_top is port(y : out bit_vector(0 to 2)); end;
architecture rtl of literal_top is
  component literal_leaf is port(a : in bit; b : in bit_vector(2 to 5); y : out bit); end component;
begin
  p: entity work.literal_leaf port map('1', "1010", y(0));
  n: entity work.literal_leaf port map(y => y(1), b => x"a", a => '1');
  c: literal_leaf port map(a => '1', b => b"1010", y => y(2));
end;
)", "literal_top");
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getInstances().size(), 3u);
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    EXPECT_TRUE(instance->getInstTerm(model->getScalarTerm(NLName("a")))->getNet()->isConstant1());
    auto* bus = model->getBusTerm(NLName("b"));
    for (int i = 2; i <= 5; ++i) {
      auto* net = instance->getInstTerm(bus->getBit(i))->getNet();
      EXPECT_EQ(net->isConstant1(), i % 2 == 0);
      EXPECT_EQ(net->isConstant0(), i % 2 != 0);
    }
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidLiteralPortActuals) {
  for (const auto* ports : {
      "'1', \"10\", y", "'X', \"1010\", y", "'1', \"1010\", '0'",
      "'1', '0', y", "'1', \"10Z0\", y", "a => '1', b => \"1010\", a => '0'"
  }) {
    SCOPED_TRACE(ports);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity bad_literal_leaf is port(a : in bit; b : in bit_vector(3 downto 0); y : out bit); end;
architecture rtl of bad_literal_leaf is begin y <= a; end;
entity bad_literal_top is port(y : out bit); end;
architecture rtl of bad_literal_top is begin
  u: entity work.bad_literal_leaf port map(
)") + ports + "); end;", "bad_literal_top"), NLException);
  }
}

TEST_F(VHDLConstructorTest, SelectedAssignmentsDecodeExhaustiveAndOthersChoices) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity selected_top is port(s : in bit_vector(1 downto 0); d : in bit; y : out bit_vector(1 downto 0)); end;
architecture rtl of selected_top is begin
  with s select y(0) <= d when "00" | "10", not d when "01", '1' when "11";
  g: if true generate
    with s select y(1) <= '0' when "00", d when others;
  end generate;
end;
)");
  ASSERT_NE(design, nullptr);
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("d"))->getNet()] = pattern & 4;
    for (unsigned i = 0; i < 2; ++i)
      values[design->getBusTerm(NLName("s"))->getBit(i)->getNet()] = (pattern >> i) & 1;
    const auto select = pattern & 3;
    const bool data = pattern & 4;
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(0)->getNet(), values, visiting),
              select == 3 || (select == 1 ? !data : data));
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(1)->getNet(), values, visiting),
              select != 0 && data);
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidSelectedAssignmentChoices) {
  for (const auto* alternatives : {
      "'0' when \"00\"", "'0' when \"00\", '1' when \"00\", '0' when others",
      "'0' when s, '1' when others", "'0' when others, '1' when \"00\"",
      "\"00\" when \"00\", '0' when others"
  }) {
    SCOPED_TRACE(alternatives);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
      "entity bad_selected is port(s : in bit_vector(1 downto 0); y : out bit); end; "
      "architecture rtl of bad_selected is begin with s select y <= ") + alternatives + "; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, SynthesisExclusionsRemoveOnlyExplicitSimulationRegions) {
  const auto source = R"(
entity excluded_trace is port(y : out bit); end;
architecture rtl of excluded_trace is
-- pragma translate_off
  file simulation_log : text;
-- pragma translate_on
begin
-- synthesis translate_off
  process begin wait; end process;
-- synthesis translate_on
  y <= '1';
end;
)";
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_NE(design, nullptr);
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("y"))->getNet())->isConstant1());
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity active_file is end; architecture rtl of active_file is
file simulation_log : text;
begin end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, SynthesisIgnoresAssertionsAndReports) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity diagnostic_top is port(y : out bit); end;
architecture rtl of diagnostic_top is
  function value return integer is begin
    report "function; message" severity failure;
    return 1;
  end function;
  constant n : integer := value;
begin
  check_config: assert false report "ISA: " & unavailable_function("x") severity failure;
  postponed assert false;
  g: if n = 1 generate
    check_generated: assert false report "generated" severity error;
    process(all) begin
      report "process";
      case n is
        when 1 => assert false severity failure;
        when others => report "other";
      end case;
      y <= '1';
    end process;
  end generate;
end;
)");
  ASSERT_NE(design, nullptr);
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("y"))->getNet())->isConstant1());
}

TEST_F(VHDLConstructorTest, SynthesisWarningsAreDeduplicatedOnlyOnConsole) {
  const auto directory = std::filesystem::temp_directory_path() / "naja-vhdl-warning-test";
  const auto reportPath = directory / "reports" / "warnings.log";
  VHDLConstructor::ConstructOptions options;
  options.diagnosticsReportPath = reportPath;
  const auto source = R"(
entity diagnostic_top is port(y : out bit); end;
architecture rtl of diagnostic_top is begin
  assert false report "first" severity failure;
  assert false report "second";
  process(all) begin
    report "first";
    report "second";
    y <= '1';
  end process;
end;
)";
  testing::internal::CaptureStdout();
  auto* design = VHDLConstructor(library_, options).construct(source);
  const auto console = testing::internal::GetCapturedStdout();
  ASSERT_NE(design, nullptr);
  const auto count = [](const std::string& text, const std::string& needle) {
    std::size_t occurrences = 0;
    for (auto pos = text.find(needle); pos != std::string::npos;
         pos = text.find(needle, pos + needle.size())) ++occurrences;
    return occurrences;
  };
  std::ifstream input(reportPath);
  const std::string report((std::istreambuf_iterator<char>(input)), {});
  EXPECT_EQ(count(console, "[ignored-assertion]"), 1);
  EXPECT_EQ(count(console, "[ignored-report]"), 1);
  EXPECT_EQ(count(report, "[ignored-assertion]"), 2);
  EXPECT_EQ(count(report, "[ignored-report]"), 2);
  EXPECT_NE(report.find("<source>:4:3:"), std::string::npos);

  // Diagnostics survive subsequent parse failure and replace the previous report.
  EXPECT_THROW(VHDLConstructor(library_, options).construct(
    "entity broken is end; architecture rtl of broken is begin "
    "assert false; invalid; end;"), NLException);
  std::ifstream failedInput(reportPath);
  const std::string failedReport((std::istreambuf_iterator<char>(failedInput)), {});
  EXPECT_EQ(count(failedReport, "[ignored-assertion]"), 1);
  EXPECT_EQ(count(failedReport, "[ignored-report]"), 0);
  std::filesystem::remove_all(directory);
}

TEST_F(VHDLConstructorTest, SynthesisWarningConsoleOnlyAndInvalidDestinations) {
  VHDLConstructor::ConstructOptions options;
  options.diagnosticsReportPath.reset();
  testing::internal::CaptureStdout();
  auto* design = VHDLConstructor(library_, options).construct(
    "entity console_only is port(y : out bit); end; "
    "architecture rtl of console_only is begin assert false; y <= '1'; end;");
  const auto console = testing::internal::GetCapturedStdout();
  ASSERT_NE(design, nullptr);
  EXPECT_NE(console.find("console-only"), std::string::npos);
  EXPECT_NE(console.find("[ignored-assertion]"), std::string::npos);
  options.diagnosticsReportPath = "";
  EXPECT_THROW(VHDLConstructor(library_, options).construct(""), NLException);
  options.diagnosticsReportPath = std::filesystem::temp_directory_path();
  EXPECT_THROW(VHDLConstructor(library_, options).construct(""), NLException);
}

TEST_F(VHDLConstructorTest, IndividualPortAssociationsPreserveElementAndSliceOrder) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity parts_leaf is generic(n : positive); port(a : in bit_vector(n-1 downto 0); y : out bit_vector(4 to 7)); end;
architecture rtl of parts_leaf is begin y <= a; end;
entity parts_top is port(a : in bit_vector(8 to 11); y : out bit_vector(3 downto 0)); end;
architecture rtl of parts_top is
  constant high : integer := 1 + 2;
begin
  u: entity work.parts_leaf generic map(4) port map(
    A(high-1 downto 0) => a(9 to 11), a(high) => a(8),
    y(6 to 7) => y(1 downto 0), y(4) => y(3), y(5) => y(2));
end;
)", "parts_top");
  ASSERT_NE(top, nullptr);
  auto* child = top->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* model = child->getModel();
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("a"))->getBit(3-i))->getNet(),
              top->getBusTerm(NLName("a"))->getBit(8+i)->getNet());
    EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("y"))->getBit(4+i))->getNet(),
              top->getBusTerm(NLName("y"))->getBit(3-i)->getNet());
  }
}

TEST_F(VHDLConstructorTest, IndividualPortAssociationsSupportRecordsLiteralsAndComponents) {
  auto* top = VHDLConstructor(library_).construct(R"(
package fields_pkg is
  type pair_t is record flag : bit; data : bit_vector(1 downto 0); end record;
end;
use work.fields_pkg.all;
entity fields_leaf is port(a : in pair_t; y : out pair_t); end;
architecture rtl of fields_leaf is begin y <= a; end;
use work.fields_pkg.all;
entity fields_top is port(a : in pair_t; y : out pair_t); end;
architecture rtl of fields_top is
  component fields_leaf is port(a : in pair_t; y : out pair_t); end component;
begin
  u: fields_leaf port map(
    a.flag => a.flag, a.data(0) => '1', a.data(1) => '0',
    y.data => y.data, y.flag => y.flag);
end;
)", "fields_top");
  auto* child = top->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* model = child->getModel();
  auto* a = model->getBusTerm(NLName("a"));
  EXPECT_EQ(child->getInstTerm(a->getBitAtPosition(0))->getNet(),
            top->getBusTerm(NLName("a"))->getBitAtPosition(0)->getNet());
  EXPECT_TRUE(child->getInstTerm(a->getBitAtPosition(1))->getNet()->isConstant0());
  EXPECT_TRUE(child->getInstTerm(a->getBitAtPosition(2))->getNet()->isConstant1());
  for (int i = 0; i < 3; ++i)
    EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("y"))->getBitAtPosition(i))->getNet(),
              top->getBusTerm(NLName("y"))->getBitAtPosition(i)->getNet());
}

TEST_F(VHDLConstructorTest, IndividualSingleBitPortsInGenerate) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity bit_leaf is port(a : in bit_vector(0 downto 0); y : out bit_vector(0 downto 0)); end;
architecture rtl of bit_leaf is begin y <= a; end;
entity bit_top is port(a : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;
architecture rtl of bit_top is begin
  g: for i in 0 to 1 generate
    u: entity work.bit_leaf port map(a(0) => a(i), y(0) => y(i));
  end generate;
end;
)", "bit_top");
  for (int i = 0; i < 2; ++i) {
    auto* child = top->getInstance(NLName("g[" + std::to_string(i) + "].u"));
    ASSERT_NE(child, nullptr);
    for (const auto* name : {"a", "y"})
      EXPECT_EQ(child->getInstTerm(child->getModel()->getBusTerm(NLName(name))->getBit(0))->getNet(),
                top->getBusTerm(NLName(name))->getBit(i)->getNet());
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidIndividualPortAssociations) {
  for (const auto* mapping : {
      "a(0) => d(0), y => q", // incomplete
      "a(0) => d(0), a(0) => d(1), y => q", // duplicate
      "a => d, a(0) => d(0), y => q", // whole/element overlap
      "a(0) => d(0), y => q, a(1) => d(1)", // nonconsecutive
      "a(0) => d(0), a(1) => d(1), q", // positional after named
      "a(0) => d, a(1) => d(1), y => q", // width/type mismatch
      "a(0 to 1) => d, y => q", // wrong slice direction
      "a(2) => d(0), a(1) => d(1), y => q", // out of bounds
      "a(0) => open, a(1) => d(1), y => q",
      "a(missing) => d(0), a(1) => d(1), y => q",
      "a(n) => d(0), a(1) => d(1), y => q", // generic is not locally static
      "a(derived) => d(0), a(1) => d(1), y => q",
      "a => d, y(0) => '0', y(1) => q(1)", // output literal
      "a => d, y(0) => q(0), y(1) => q(0)" // overlapping output drivers
  }) {
    SCOPED_TRACE(mapping);
    const auto before = library_->getSNLDesigns().size();
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity invalid_parts_leaf is port(a : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;
architecture rtl of invalid_parts_leaf is begin y <= a; end;
entity invalid_parts_top is generic(n : natural := 0); port(d : in bit_vector(1 downto 0); q : out bit_vector(1 downto 0)); end;
architecture rtl of invalid_parts_top is
  constant derived : integer := n;
begin
  u: entity work.invalid_parts_leaf port map(
)") + mapping + "); end;", "invalid_parts_top"), NLException);
    EXPECT_EQ(library_->getSNLDesigns().size(), before);
  }
}

TEST_F(VHDLConstructorTest, IndividualComponentFormalsUseComponentBounds) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity remapped_leaf is port(a : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;
architecture rtl of remapped_leaf is begin y <= a; end;
entity remapped_top is port(d : in bit_vector(1 downto 0); q : out bit_vector(1 downto 0)); end;
architecture rtl of remapped_top is
  component remapped_leaf is port(a : in bit_vector(4 to 5); y : out bit_vector(4 to 5)); end component;
begin
  u: remapped_leaf port map(a(4) => d(1), a(5) => d(0), y(4) => q(1), y(5) => q(0));
end;
)", "remapped_top");
  auto* child = top->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  for (int i = 0; i < 2; ++i) {
    EXPECT_EQ(child->getInstTerm(child->getModel()->getBusTerm(NLName("a"))->getBit(i))->getNet(),
              top->getBusTerm(NLName("d"))->getBit(i)->getNet());
    EXPECT_EQ(child->getInstTerm(child->getModel()->getBusTerm(NLName("y"))->getBit(i))->getNet(),
              top->getBusTerm(NLName("q"))->getBit(i)->getNet());
  }
}

TEST_F(VHDLConstructorTest, OpenOutputsRemainUnconnectedAcrossInstanceForms) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity open_leaf is port(a : in bit; y : out bit; unused : out bit_vector(2 to 4)); end;
architecture rtl of open_leaf is begin y <= not a; unused <= (others => a); end;
entity open_top is port(a : in bit; y : out bit_vector(0 to 2)); end;
architecture rtl of open_top is
  component open_leaf is port(a : in bit; y : out bit; unused : out bit_vector(2 to 4)); end component;
begin
  p: entity work.open_leaf port map(a, y(0), open);
  c: open_leaf port map(unused => open, y => y(1), a => a);
  g: for i in 2 to 2 generate
    n: entity work.open_leaf port map(a => a, unused => open, y => y(i));
  end generate;
  all_open: entity work.open_leaf port map(a, open, open);
end;
)", "open_top");
  ASSERT_NE(top, nullptr);
  ASSERT_EQ(top->getInstances().size(), 4u);
  for (auto* child : top->getInstances()) {
    auto* model = child->getModel();
    EXPECT_EQ(child->getInstTerm(model->getScalarTerm(NLName("a")))->getNet(),
              top->getScalarTerm(NLName("a"))->getNet());
    for (auto* bit : model->getBusTerm(NLName("unused"))->getBusBits())
      EXPECT_EQ(child->getInstTerm(bit)->getNet(), nullptr);
    auto* output = child->getInstTerm(model->getScalarTerm(NLName("y")))->getNet();
    if (child->getName().getString() == "all_open") EXPECT_EQ(output, nullptr);
    else {
      const auto index = child->getName().getString() == "p" ? 0 : child->getName().getString() == "c" ? 1 : 2;
      EXPECT_EQ(output, top->getBusTerm(NLName("y"))->getBit(index)->getNet());
    }
  }
}

TEST_F(VHDLConstructorTest, OmittedOutputPortsRemainUnconnectedAcrossInstanceForms) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity leaf is port(a : in bit; y : out bit; unused : out bit_vector(2 to 4)); end;
architecture rtl of leaf is begin y <= not a; unused <= (others => a); end;
entity wrapper is port(a : in bit; y : out bit_vector(0 to 2)); end;
architecture rtl of wrapper is
  component leaf is port(a : in bit; y : out bit; unused : out bit_vector(2 to 4)); end component;
begin
  p: entity work.leaf port map(a, y(0));
  c: leaf port map(y => y(1), a => a);
  g: for i in 2 to 2 generate
    n: entity work.leaf port map(a => a, y => y(i));
  end generate;
  discarded: entity work.leaf port map(a => a);
end;
)", "wrapper");
  ASSERT_NE(top, nullptr);
  ASSERT_EQ(4u, top->getInstances().size());
  for (auto* instance : top->getInstances()) {
    auto* model = instance->getModel();
    EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("a")))->getNet(), top->getScalarTerm(NLName("a"))->getNet());
    for (auto* bit : model->getBusTerm(NLName("unused"))->getBusBits())
      EXPECT_EQ(instance->getInstTerm(bit)->getNet(), nullptr);
    auto* output = instance->getInstTerm(model->getScalarTerm(NLName("y")))->getNet();
    if (instance->getName().getString() == "discarded") EXPECT_EQ(output, nullptr);
    else {
      const auto index = instance->getName().getString() == "p" ? 0 : instance->getName().getString() == "c" ? 1 : 2;
      EXPECT_EQ(output, top->getBusTerm(NLName("y"))->getBit(index)->getNet());
    }
  }
}

TEST_F(VHDLConstructorTest, OmittedTrailingScalarOutputSelectsRTLLoader) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity leaf is port(a : in bit; y, unused : out bit); end;
architecture rtl of leaf is begin y <= a; unused <= a; end;
entity wrapper is port(a : in bit; y : out bit); end;
architecture rtl of wrapper is begin u: entity work.leaf port map(a,y); end;
)", "wrapper");
  auto* instance = top->getInstance(NLName("u"));
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(instance->getInstTerm(instance->getModel()->getScalarTerm(NLName("unused")))->getNet(), nullptr);
  EXPECT_EQ(instance->getInstTerm(instance->getModel()->getScalarTerm(NLName("y")))->getNet(), top->getScalarTerm(NLName("y"))->getNet());
}

TEST_F(VHDLConstructorTest, OmittedRecordOutputRemainsUnconnected) {
  auto* top = VHDLConstructor(library_).construct(R"(
package fields is type pair_t is record flag : bit; data : bit_vector(1 downto 0); end record; end;
use work.fields.all;
entity leaf is port(a : in bit; y : out pair_t); end;
architecture rtl of leaf is begin y <= (flag => a, data => "10"); end;
use work.fields.all;
entity wrapper is port(a : in bit); end;
architecture rtl of wrapper is begin u: entity work.leaf port map(a => a); end;
)", "wrapper");
  auto* instance = top->getInstance(NLName("u"));
  ASSERT_NE(instance, nullptr);
  for (auto* bit : instance->getModel()->getBusTerm(NLName("y"))->getBusBits())
    EXPECT_EQ(instance->getInstTerm(bit)->getNet(), nullptr);
}

TEST_F(VHDLConstructorTest, RejectsMissingInputsAndPartialOutputAssociations) {
  for (const auto* mapping : {"y => q", "a => d, y(0) => q(0)"}) {
    SCOPED_TRACE(mapping);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity leaf is port(a : in bit; y : out bit_vector(1 downto 0)); end;
architecture rtl of leaf is begin y <= (others => a); end;
entity wrapper is port(d : in bit; q : out bit_vector(1 downto 0)); end;
architecture rtl of wrapper is begin u: entity work.leaf port map(
)") + mapping + "); end;", "wrapper"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, WholeRecordOutputCanBeOpen) {
  auto* top = VHDLConstructor(library_).construct(R"(
package open_types is type pair_t is record flag : bit; data : bit_vector(1 downto 0); end record; end;
use work.open_types.all;
entity open_record_leaf is port(a : in bit; y : out pair_t); end;
architecture rtl of open_record_leaf is begin y <= (flag => a, data => "10"); end;
use work.open_types.all;
entity open_record_top is port(a : in bit); end;
architecture rtl of open_record_top is begin
  u: entity work.open_record_leaf port map(a => a, y => open);
end;
)", "open_record_top");
  auto* child = top->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* output = child->getModel()->getBusTerm(NLName("y"));
  ASSERT_NE(output, nullptr);
  ASSERT_EQ(output->getWidth(), 3u);
  for (auto* bit : output->getBusBits()) EXPECT_EQ(child->getInstTerm(bit)->getNet(), nullptr);
}

TEST_F(VHDLConstructorTest, InvalidOpenAssociationsStillRejectAndRollBack) {
  for (const auto* mapping : {
      "open, q", "a => open, y => q", // input without default
      "a => d, y(0) => open, y(1) => q(1)",
      "a => d, y(1 downto 0) => open", // individual slice, even the full range
      "a => d, y => open, y => q", // duplicate
      "a => d, absent => open", // unknown formal
      "a => d, open", // positional after named
      "d, open, open" // too many actuals
  }) {
    SCOPED_TRACE(mapping);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity invalid_open_leaf is port(a : in bit; y : out bit_vector(1 downto 0)); end;
architecture rtl of invalid_open_leaf is begin y <= (others => a); end;
entity invalid_open_top is port(d : in bit; q : out bit_vector(1 downto 0)); end;
architecture rtl of invalid_open_top is begin
  u: entity work.invalid_open_leaf port map(
)") + mapping + "); end;", "invalid_open_top"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, NamedProcessEndsPreserveCombinationalAndClockedLowering) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity labeled_top is port(clk, a : in bit; y, q : out bit); end;
architecture rtl of labeled_top is begin
  invert: process(all) begin y <= not a; end process INVERT;
  g: if true generate
    reg: process(clk) begin if rising_edge(clk) then q <= a; end if; end process reg;
  end generate;
end;
)");
  ASSERT_NE(top, nullptr);
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    values[top->getScalarTerm(NLName("a"))->getNet()] = input;
    EXPECT_EQ(evaluateRTL(top->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
  }
  size_t flopCount = 0;
  for (auto* instance : top->getInstances()) {
    if (!NLDB0::isDFF(instance->getModel())) continue;
    ++flopCount;
    EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
              top->getScalarTerm(NLName("clk"))->getNet());
    EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
              top->getScalarTerm(NLName("a"))->getNet());
    for (const bool value : {false, true}) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      values[instance->getInstTerm(NLDB0::getDFFOutput())->getNet()] = value;
      EXPECT_EQ(evaluateRTL(top->getScalarTerm(NLName("q"))->getNet(), values, visiting), value);
    }
  }
  EXPECT_EQ(flopCount, 1u);
}

TEST_F(VHDLConstructorTest, EnumerationIndexedRecordsPreserveLiteralOrder) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity enum_records is port(d : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;
architecture rtl of enum_records is
  type channel_t is (second, first);
  type entry_t is record valid : bit; data : bit_vector(1 downto 0); end record;
  type table_t is array(channel_t) of entry_t;
  signal entries : table_t;
begin
  entries(second) <= (valid => d(0), data => "10");
  entries(first) <= (valid => d(1), data => "01");
  y(0) <= entries(second).valid xor entries(second).data(1);
  y(1) <= entries(first).valid xor entries(first).data(0);
end;
)");
  for (unsigned pattern = 0; pattern < 4; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (int i = 0; i < 2; ++i) values[top->getBusTerm(NLName("d"))->getBit(i)->getNet()] = (pattern >> i) & 1;
    for (int i = 0; i < 2; ++i)
      EXPECT_EQ(evaluateRTL(top->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), !bool((pattern >> i) & 1));
  }
}

TEST_F(VHDLConstructorTest, EnumerationIndexedConstantsAndDescendingSlices) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity enum_constants is port(y : out bit_vector(3 downto 0)); end;
architecture rtl of enum_constants is
  type color_t is (red, green, blue);
  type fixed_t is array(blue downto red) of bit;
  type flexible_t is array(color_t range <>) of bit;
  constant fixed : fixed_t := ('0', '1', '1');
  constant flexible : flexible_t(blue downto red) := ('1', '0', '0');
  signal sliced : flexible_t(green downto red);
begin
  sliced <= flexible(green downto red);
  y(0) <= fixed(red);
  y(1) <= fixed(blue);
  y(2) <= sliced(green);
  y(3) <= flexible(blue);
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  for (int i = 0; i < 4; ++i)
    EXPECT_EQ(evaluateRTL(top->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), i == 0 || i == 3);
}

TEST_F(VHDLConstructorTest, EnumerationIndexedDynamicReadsAndClockedWrites) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity enum_dynamic is port(clk, rst, d : in bit; sel : in bit_vector(1 downto 0); y : out bit); end;
architecture rtl of enum_dynamic is
  type color_t is (red, green, blue);
  type storage_t is array(color_t) of bit;
  signal cells : storage_t;
  signal addr : color_t;
begin
  with sel select addr <= red when "00", green when "01", blue when others;
  process(clk) begin
    if rising_edge(clk) then
      if rst = '1' then cells <= (others => '0');
      else cells(addr) <= d; end if;
    end if;
  end process;
  y <= cells(addr);
end;
)");
  const auto flops = dffBits(top);
  ASSERT_EQ(flops.size(), 3u);
  for (auto* instance : top->getInstances()) EXPECT_FALSE(NLDB0::isMemory(instance->getModel()));
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  bool expected[3] = {false, false, false};
  for (int cycle = 0; cycle < 24; ++cycle) {
    const int sel = cycle % 4;
    const int addr = sel < 2 ? sel : 2;
    const bool reset = cycle == 0 || cycle == 13;
    const bool data = (cycle / 3) % 2;
    auto values = state;
    values[top->getScalarTerm(NLName("rst"))->getNet()] = reset;
    values[top->getScalarTerm(NLName("d"))->getNet()] = data;
    for (int bit = 0; bit < 2; ++bit) values[top->getBusTerm(NLName("sel"))->getBit(bit)->getNet()] = (sel >> bit) & 1;
    for (const auto& flop : flops) {
      std::unordered_set<SNLBitNet*> visiting;
      state[flop.output] = evaluateRTL(flop.data, values, visiting);
    }
    if (reset) for (auto& bit : expected) bit = false;
    else expected[addr] = data;
    for (int probe = 0; probe < 3; ++probe) {
      auto readValues = state;
      for (int bit = 0; bit < 2; ++bit)
        readValues[top->getBusTerm(NLName("sel"))->getBit(bit)->getNet()] = (probe >> bit) & 1;
      std::unordered_set<SNLBitNet*> visiting;
      EXPECT_EQ(evaluateRTL(top->getScalarTerm(NLName("y"))->getNet(), readValues, visiting), expected[probe]);
    }
  }
}

TEST_F(VHDLConstructorTest, RejectsEnumerationArrayIndexTypeAndRangeErrors) {
  for (const auto* selection : {"0", "other", "blue", "red to other"}) {
    SCOPED_TRACE(selection);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(R"(
entity bad_enum_index is port(y : out bit); end;
architecture rtl of bad_enum_index is
  type color_t is (red, green, blue);
  type other_t is (other);
  type table_t is array(red to green) of bit;
  constant lut : table_t := ('0', '1');
begin y <= lut(
)") + selection + "); end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity bad_enum_dynamic is port(d : in bit; y : out bit); end;
architecture rtl of bad_enum_dynamic is
  type color_t is (red, green);
  type other_t is (one, two);
  type table_t is array(color_t) of bit;
  constant lut : table_t := ('0', '1');
  signal addr : other_t;
begin addr <= one when d = '0' else two; y <= lut(addr); end;
)"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, EnumerationIndexedRecordPortActualsKeepElementOffsets) {
  auto* top = VHDLConstructor(library_).construct(R"(
package enum_ports is
  type channel_t is (second, first);
  type entry_t is record valid : bit; data : bit_vector(1 downto 0); end record;
  type table_t is array(channel_t) of entry_t;
end;
use work.enum_ports.all;
entity enum_port_leaf is port(req : in entry_t; y : out bit); end;
architecture rtl of enum_port_leaf is begin y <= req.valid xor req.data(0); end;
use work.enum_ports.all;
entity enum_port_top is port(d : in bit; y : out bit_vector(0 to 1)); end;
architecture rtl of enum_port_top is signal requests : table_t;
begin
  requests(second) <= (valid => d, data => "10");
  requests(first) <= (valid => d, data => "01");
  a: entity work.enum_port_leaf port map(req => requests(second), y => y(0));
  b: entity work.enum_port_leaf port map(req => requests(first), y => y(1));
end;
)", "enum_port_top");
  auto* array = top->getBusNet(NLName("requests"));
  ASSERT_NE(array, nullptr);
  for (int i = 0; i < 2; ++i) {
    auto* child = top->getInstance(NLName(i ? "b" : "a"));
    ASSERT_NE(child, nullptr);
    auto* input = child->getModel()->getBusTerm(NLName("req"));
    for (int bit = 0; bit < 3; ++bit)
      EXPECT_EQ(child->getInstTerm(input->getBitAtPosition(bit))->getNet(), array->getBitAtPosition(3*i+bit));
  }
}

TEST_F(VHDLConstructorTest, LogicalLibrariesOwnPackagesEntitiesAndWork) {
  auto* db = library_->getDB();
  auto* cells = NLLibrary::create(db, NLName("Cells"));
  library_->setName(NLName("DESIGN"));
  VHDLConstructor(cells).construct(
      "package sizes is constant width : positive := 3; end;");
  VHDLConstructor(library_).construct(
      "package sizes is constant width : positive := 7; end;");
  const std::string child = R"(
use work.sizes.all;
entity leaf is generic(n : positive); port(a : in bit_vector(width-1 downto 0);
y : out bit_vector(width-1 downto 0)); end;
architecture rtl of leaf is begin y <= a; end;
)";
  EXPECT_EQ(VHDLConstructor(cells).construct(child), nullptr);
  EXPECT_EQ(VHDLConstructor(library_).construct(child), nullptr);
  const std::string parent = R"(
library cells; use cells.sizes.all;
entity parent is port(a : in bit_vector(width-1 downto 0);
y : out bit_vector(width-1 downto 0)); end;
architecture rtl of parent is begin
u: entity cells.leaf generic map(n => 1) port map(a, y);
end;
)";
  auto* top = VHDLConstructor(library_).construct(parent, "parent");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getLibrary(), library_);
  auto* model = top->getInstance(NLName("u"))->getModel();
  EXPECT_EQ(model->getLibrary(), cells);
  EXPECT_EQ(model->getBusTerm(NLName("a"))->getWidth(), 3);
  EXPECT_EQ(VHDLConstructor(library_).construct(parent, "parent"), top);
  const std::string localParent = R"(
entity local_parent is port(a : in bit_vector(6 downto 0); y : out bit_vector(6 downto 0)); end;
architecture rtl of local_parent is begin
u: entity work.leaf generic map(n => 1) port map(a, y); end;
)";
  auto* local = VHDLConstructor(library_).construct(localParent, "local_parent");
  EXPECT_EQ(local->getInstance(NLName("u"))->getModel()->getLibrary(), library_);
  EXPECT_EQ(local->getInstance(NLName("u"))->getModel()->getBusTerm(NLName("a"))->getWidth(), 7);
}

TEST_F(VHDLConstructorTest, LogicalLibraryLookupRejectsMissingAndAmbiguousRoots) {
  const std::string source = R"(
library cells; use cells.sizes.all;
entity top is port(y : out bit); end;
architecture rtl of top is begin y <= '1'; end;
)";
  auto* nested = NLLibrary::create(library_, NLName("cells"));
  (void)nested;
  EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  auto* cells = NLLibrary::create(library_->getDB(), NLName("Cells"));
  VHDLConstructor(cells).construct("package sizes is constant n : integer := 1; end;");
  NLLibrary::create(library_->getDB(), NLName("CELLS"));
  try {
    VHDLConstructor(library_).construct(source);
    FAIL() << "ambiguous logical library accepted";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("ambiguous HDL library"), std::string::npos);
  }
}

TEST_F(VHDLConstructorTest, ImportedPackageWorkAndBodyUseDefiningLibrary) {
  library_->setName(NLName("DESIGN"));
  auto* cells = NLLibrary::create(library_->getDB(), NLName("cells"));
  for (auto* library : {library_, cells}) {
    const auto width = library == cells ? "3" : "7";
    VHDLConstructor(library).construct(std::string(
        "package dimensions is constant width : positive := ") + width + "; end;");
    VHDLConstructor(library).construct(std::string(R"(
use work.dimensions.all;
package sizes is
  constant size : positive := width;
  function size_f(n : natural) return natural;
end;
package body sizes is
  function size_f(n : natural) return natural is begin return n + )") + width + R"(; end;
end;
)");
  }
  auto* top = VHDLConstructor(library_).construct(R"(
library cells; use cells.sizes.all;
entity top is port(a : in bit_vector(size-1 downto 0);
y : out bit_vector(size_f(0)-1 downto 0)); end;
architecture rtl of top is begin y <= a; end;
)");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getBusTerm(NLName("a"))->getWidth(), 3);
}

TEST_F(VHDLConstructorTest, SameNamedLibraryTypesStayNominalAndFailureRollsBack) {
  library_->setName(NLName("DESIGN"));
  auto* cells = NLLibrary::create(library_->getDB(), NLName("cells"));
  for (auto* library : {library_, cells})
    VHDLConstructor(library).construct(
        "package types is type word is array(2 downto 0) of bit; end;");
  VHDLConstructor(cells).construct(R"(
use work.types.all;
entity leaf is generic(n : positive); port(a : in word; y : out word); end;
architecture rtl of leaf is begin y <= a; end;
)");
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
library cells; use work.types.all;
entity top is port(a : in word; y : out word); end;
architecture rtl of top is begin
u: entity cells.leaf generic map(n => 1) port map(a, y); end;
)"), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("top")), nullptr);
  EXPECT_EQ(cells->getSNLDesign(NLName("leaf__n_1")), nullptr);
}

TEST_F(VHDLConstructorTest, BooleanGenericDefaultsDriveGenerateAndDependentDefaults) {
  for (const bool enabled : {false, true}) {
    const auto source = std::string(R"(
entity top is generic(enabled : boolean := )") + (enabled ? "true" : "false") + R"(;
  inverted : boolean := not enabled);
  port(a : in bit; y : out bit); end;
architecture rtl of top is begin
  g: if inverted generate y <= not a; else generate y <= a; end generate;
end;
)";
    auto* top = VHDLConstructor(library_).construct(source);
    for (const bool input : {false, true}) {
      std::unordered_map<SNLBitNet*, bool> values{{top->getScalarTerm(NLName("a"))->getNet(), input}};
      std::unordered_set<SNLBitNet*> visiting;
      EXPECT_EQ(evaluateRTL(top->getScalarTerm(NLName("y"))->getNet(), values, visiting),
                enabled ? input : !input);
    }
    top->destroy();
  }
}

TEST_F(VHDLConstructorTest, BooleanGenericActualsAndSpecializationsKeepTypesAndParentScope) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity leaf is generic(enabled : boolean; n : positive := 1);
  port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= a when enabled else not a; end;
entity parent is generic(enabled : boolean := true);
  port(a : in bit; yes_y, no_y, copy_y, parent_y : out bit); end;
architecture rtl of parent is
  component leaf is generic(enabled : boolean; n : positive := 1);
    port(a : in bit; y : out bit); end component;
begin
  yes_i: entity work.leaf generic map(true, 2) port map(a, yes_y);
  no_i: leaf generic map(enabled => not enabled, n => 2) port map(a, no_y);
  copy_i: entity work.leaf generic map(enabled => 1 > 2, n => 2) port map(a, copy_y);
  parent_y <= a when enabled else not a;
end;
)", "parent");
  auto* yesModel = top->getInstance(NLName("yes_i"))->getModel();
  auto* noModel = top->getInstance(NLName("no_i"))->getModel();
  EXPECT_NE(yesModel, noModel);
  EXPECT_EQ(noModel, top->getInstance(NLName("copy_i"))->getModel());
  EXPECT_EQ(yesModel->getName(), NLName("leaf__enabled_true__n_2"));
  EXPECT_EQ(noModel->getName(), NLName("leaf__enabled_false__n_2"));
  for (auto* model : {yesModel, noModel, top}) {
    for (const bool input : {false, true}) {
      std::unordered_map<SNLBitNet*, bool> values{{model->getScalarTerm(NLName("a"))->getNet(), input}};
      std::unordered_set<SNLBitNet*> visiting;
      const auto output = model == top ? "parent_y" : "y";
      EXPECT_EQ(evaluateRTL(model->getScalarTerm(NLName(output))->getNet(), values, visiting),
                model == noModel ? !input : input);
    }
  }
}

TEST_F(VHDLConstructorTest, BooleanGenericTypeMismatchesRejectAndRollBack) {
  for (const auto* declaration : {"flag : boolean := 1", "n : integer := true",
                                  "flag : boolean", "flag : boolean := later; later : boolean := true"}) {
    SCOPED_TRACE(declaration);
    const auto source = std::string("entity bad is generic(") + declaration +
        "); port(y : out bit); end; architecture rtl of bad is begin y <= '1'; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source, "bad"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("bad")), nullptr);
  }
  for (const auto* actuals : {"1, 2", "true, false", "true, 0"}) {
    SCOPED_TRACE(actuals);
    const auto source = std::string(R"(
entity leaf is generic(flag : boolean; n : positive); port(y : out bit); end;
architecture rtl of leaf is begin y <= '1'; end;
entity parent is port(y : out bit); end;
architecture rtl of parent is begin
u: entity work.leaf generic map()") + actuals + R"() port map(y); end;
)";
    EXPECT_THROW(VHDLConstructor(library_).construct(source, "parent"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, SimplePathValidationDiagnostics) {
  const auto check = [&](const std::string& source, const std::string& diagnostic,
                         const std::string& top = "") {
    SCOPED_TRACE(source);
    try {
      VHDLConstructor(library_).construct(source, top);
      FAIL() << "expected: " << diagnostic;
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos)
          << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  };
  const std::string leaf = "entity leaf is port(a : in bit; y : out bit); end; "
      "architecture rtl of leaf is begin y <= a; end; ";
  const auto hierarchy = [&](const std::string& ports, const std::string& declarations,
                             const std::string& body) {
    return leaf + "entity top is port(" + ports + "); end; "
        "architecture rtl of top is " + declarations + " begin " + body + " end;";
  };
  check(leaf + "entity top is end;", "exactly one architecture", "top");
  check(leaf + "entity top is end; architecture a of top is begin end; "
      "architecture b of top is begin end;", "exactly one architecture", "top");
  check(hierarchy("a : in bit; y : out bit", "", "u: entity work.leaf port map(a,y); y <= a;"),
      "cannot mix behavior", "top");
  check(hierarchy("a : in bit; y : out bit", "", ""), "no entity instances", "top");
  check(hierarchy("a : in bit; y : out bit; spare : inout bit", "", "u: entity work.leaf port map(a,y);"),
      "hierarchy ports", "top");
  check(hierarchy("a : in bit; y : out bit", "signal spare : real;", "u: entity work.leaf port map(a,y);"),
      "hierarchy signals", "top");
  check(hierarchy("a : in bit; y : out bit", "", "u: entity work.top port map(a,y);"),
      "recursive hierarchy", "top");
  check("entity child is port(a : inout bit; y : out bit); end; "
      "architecture rtl of child is begin y <= a; end; "
      "entity top is port(a : in bit; y : out bit); end; "
      "architecture rtl of top is begin u: entity work.child port map(a,y); end;",
      "child ports", "top");
  check(hierarchy("a : in bit; y : out bit", "", "u: entity work.leaf port map(a,a);"),
      "cannot drive a top input", "top");
  check(leaf, "selected top does not match", "other");
  const auto scalar = [&](const std::string& extraPorts, const std::string& declarations,
                          const std::string& expression) {
    return "entity top is port(a, s : in bit; y : out bit" + extraPorts + "); end; "
        "architecture rtl of top is " + declarations + " begin y <= " + expression + "; end;";
  };
  check(scalar("", "", "a when s /= '1' else '0'"), "expected an equality condition");
  check(scalar("", "", "a when s = '0' else '0'"), "condition must compare");
  check(scalar("; spare : in integer", "", "a"), "only scalar bit and constrained bit_vector ports");
  check(scalar("; spare : in bit_vector(2147483648 downto 2147483648)", "", "a"), "bounds exceed");
  check(scalar("; spare : inout bit", "", "a"), "only in and out ports");
  check(scalar("", "signal spare : real;", "a"), "only scalar bit internal signals");
  check(scalar("", "signal spare : bit;", "a"), "internal signals currently require");
  check(scalar("", "", "a when s = '1' else a when s = '1' else '0'"), "expression shape");
  const auto clocked = [&](const std::string& declarations, const std::string& body) {
    return "entity top is port(clk, rst, d : in bit; q, r : out bit); end; "
        "architecture rtl of top is begin process(clk) " + declarations +
        " begin if clk'event and clk = '1' then " + body + " end if; end process; end;";
  };
  check(clocked("variable spare : boolean;", "q <= d;"), "only scalar bit process variables");
  check(clocked("", "if rst = '1' then q <= '0'; q <= '0'; else q <= d; end if;"),
      "multiple synchronous reset writes");
  check(clocked("", "if rst = '1' then q <= '0'; else q <= d; r <= d; end if;"),
      "reset and data branches must assign the same targets");
}

TEST_F(VHDLConstructorTest, SimpleScalarHierarchyAndSourceProperty) {
  const auto source = R"(
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture rtl of top is signal link : bit; begin
  first: entity work.leaf port map(a, link);
  second: entity work.leaf port map(link, y);
end;
)";
  auto* top = VHDLConstructor(library_).construct(source, "top");
  ASSERT_NE(top, nullptr);
  auto* first = top->getInstance(NLName("first"));
  auto* second = top->getInstance(NLName("second"));
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->getModel(), second->getModel());
  EXPECT_EQ(first->getInstTerm(first->getModel()->getScalarTerm(NLName("y")))->getNet(),
      top->getNet(NLName("link")));
  EXPECT_EQ(second->getInstTerm(second->getModel()->getScalarTerm(NLName("a")))->getNet(),
      top->getNet(NLName("link")));
  auto* property = library_->getProperty("VHDLSources");
  ASSERT_NE(property, nullptr);
  EXPECT_EQ(property->getString(), "VHDLSources");
  try {
    VHDLConstructor(library_).construct(source, "top");
    FAIL() << "expected duplicate top rejection";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("top entity name already exists"), std::string::npos);
  }
  EXPECT_EQ(library_->getSNLDesign(NLName("top")), top);
}

TEST_F(VHDLConstructorTest, SimpleDefaultedGenericSpecializations) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity leaf is generic(n, m : positive := 2; extra : natural := 0);
  port(a : in bit_vector(n-1 downto 0); y : out bit_vector(m-1 downto 0)); end;
architecture rtl of leaf is begin y <= a; end;
entity top is port(a : in bit_vector(2 downto 0); y, z : out bit_vector(2 downto 0)); end;
architecture rtl of top is begin
  first: entity work.leaf generic map(3, 3, 1) port map(a, y);
  second: entity work.leaf generic map(3, 3, 2) port map(a, z);
end;
)", "top");
  ASSERT_NE(top, nullptr);
  auto* first = top->getInstance(NLName("first"));
  auto* second = top->getInstance(NLName("second"));
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_NE(first->getModel(), second->getModel());
  EXPECT_EQ(first->getModel()->getName(), NLName("leaf_extra_1_m_3_n_3"));
  EXPECT_EQ(second->getModel()->getName(), NLName("leaf_extra_2_m_3_n_3"));
  EXPECT_EQ(first->getModel()->getBusTerm(NLName("a"))->getWidth(), 3);
  EXPECT_EQ(second->getModel()->getBusTerm(NLName("y"))->getWidth(), 3);
}

TEST_F(VHDLConstructorTest, SimpleHierarchyRollbackPreservesExistingDesigns) {
  auto* existing = SNLDesign::create(library_, NLName("blocked"));
  const std::string prefix = R"(
entity leaf is generic(n : positive := 2); port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= a; end;
entity blocked is port(a : in bit; y : out bit); end;
architecture rtl of blocked is begin y <= a; end;
entity top is port(a : in bit; y, z : out bit); end;
architecture rtl of top is begin
  first: entity work.leaf generic map(3) port map(a, y);
  second: entity work.blocked port map(a, z);
end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(prefix, "top"), NLException);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  EXPECT_EQ(library_->getSNLDesign(NLName("blocked")), existing);
  EXPECT_EQ(library_->getSNLDesign(NLName("leaf_n_3")), nullptr);
  EXPECT_EQ(library_->getSNLDesign(NLName("top")), nullptr);
  try {
    VHDLConstructor(library_).construct(R"(
entity grouped is generic(n, m : positive := 2); port(a : in bit; y : out bit); end;
architecture rtl of grouped is begin y <= a; end;
entity top is port(a : in bit; y : out bit); end;
architecture rtl of top is begin
  u: entity work.grouped generic map(2, 3) port map(a, y);
end;
)", "top");
    FAIL() << "expected grouped generic rejection";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("grouped generics with different actual values"),
        std::string::npos) << error.what();
  }
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  EXPECT_EQ(library_->getSNLDesign(NLName("blocked")), existing);
}

TEST_F(VHDLConstructorTest, SimpleExtendedIdentifierExpression) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity escaped is port(\A\ : in bit; y : out bit); end;
architecture rtl of escaped is begin y <= \A\; end;
)");
  ASSERT_NE(design, nullptr);
  ASSERT_NE(design->getScalarTerm(NLName("\\A\\")), nullptr);
  EXPECT_EQ(design->getInstances().size(), 1);
}

TEST_F(VHDLConstructorTest, FileParseDiagnosticIncludesPath) {
  const auto path = std::filesystem::temp_directory_path() / "naja-vhdl-invalid-coverage.vhd";
  { std::ofstream file(path); file << "entity broken is"; }
  try {
    VHDLConstructor(library_).constructFile(path);
    ADD_FAILURE() << "expected parse failure";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find(path.string()), std::string::npos);
    EXPECT_NE(std::string(error.what()).find("parse failed"), std::string::npos);
  }
  std::filesystem::remove(path);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, RTLFunctionAndTypeDiagnosticsRollBackDesigns) {
  const std::vector<std::pair<std::string, std::string>> cases{
    {"type items is array(real range <>) of bit;", "unsupported unconstrained array index subtype"},
    {"type items is array(integer range <>) of integer; constant n : integer := items'length;", "array attribute requires a constrained type or object"},
    {"function f(x : bit) return integer is begin return 1; end; constant n : bit := f(a);", "assignment or expression type/length mismatch"},
    {"function f(x : bit_vector) return bit is begin return x(0); end; constant n : bit := f(a);", "unconstrained function vector requires a vector actual"},
    {"function f(x : bit) return bit is begin x := '0'; return x; end; constant n : bit := f(a);", "initializer functions may assign only local variables"},
    {"function f(x : bit) return bit is begin for x in 0 to 1 loop null; end loop; return x; end; constant n : bit := f(a);", "shadowed runtime function loop parameters are unsupported"},
    {"function f(x : bit) return bit is variable x : bit; begin return x; end; constant n : bit := f(a);", "shadowed runtime function locals are unsupported"},
    {"function f(x, x : bit) return bit is begin return x; end; constant n : bit := f(a);", "duplicate function parameter"},
    {"type items is array(0 to 1) of integer; constant data : items := (1, 2); constant n : boolean := false and boolean(data(0));", "boolean conversion requires a boolean operand"},
    {"constant n : boolean := false and boolean(-1);", "boolean conversion requires a boolean operand"},
    {"constant n : boolean := false and boolean(not 1);", "static unary operand type mismatch"},
    {"constant n : boolean := false and boolean(1 + true);", "static operand type mismatch"},
    {"constant n : boolean := false and boolean(1 and 2);", "static logical operands must be boolean"},
    {"constant n : boolean := false and boolean(true + false);", "unsupported static scalar expression type"},
    {"constant n : boolean := false and boolean('1');", "unsupported static scalar expression type"},
    {"type indices is (first, last); type flags is array(indices) of boolean; constant data : flags := (others => true); constant n : boolean := false and data(0);", "array index enumeration type mismatch"}
  };
  for (const auto& [declarations, diagnostic] : cases) {
    SCOPED_TRACE(declarations);
    const auto source = "entity checked is port(a : in bit; y : out bit); end; "
        "architecture rtl of checked is constant marker : integer := 0; " + declarations + " begin y <= a; end;";
    try {
      VHDLConstructor(library_).construct(source);
      FAIL() << "invalid source accepted";
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, RuntimeFunctionLocalConstantsPreserveDeclarationOrder) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity local_values is port(a : in bit; y : out bit); end;
architecture rtl of local_values is
  function invert(x : bit) return bit is
    constant bias : bit := '1';
    variable result : bit := x xor bias;
    constant mask : bit := bias;
  begin return result and mask; end;
begin y <= invert(a); end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
  }
}

TEST_F(VHDLConstructorTest, StaticTypeInspectionPreservesArraysAndCapturedValues) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity inspected is port(y : out bit); end;
architecture rtl of inspected is
  type numbers is array(0 to 2) of integer;
  type flags is array(0 to 1) of boolean;
  constant data : numbers := (others => 3);
  constant enabled : flags := (others => true);
  constant bias : integer := 2;
  function positive_value(n : integer) return boolean is begin return n > 0; end;
  function selected(n : integer; b : boolean) return boolean is
  begin return boolean(bias + n > 0) and boolean(b) and boolean(positive_value(n)); end;
  function add(a, b : integer) return integer is begin return a + b; end;
  constant chosen : integer := sel_natural_f(false, 1 / 0, 3);
begin
  y <= '1' when boolean(enabled(0)) and boolean(data(1) = 3) and
    selected(1, true) and boolean(abs(-bias) = 2) and boolean(not false) and
    boolean((true xor false) and true) and boolean((3 * 2) = 6) and
    add(a => chosen, b => 2) = 5 else '0';
end;
)");
  EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("y"))->getNet())->isConstant1());
}

TEST_F(VHDLConstructorTest, RTLPortModesAreRejectedBeforePublishing) {
  for (const auto* mode : {"inout", "buffer", "linkage"}) {
    SCOPED_TRACE(mode);
    try {
      VHDLConstructor(library_).construct(std::string("entity modes is port(a : ") + mode +
          " bit_vector(1 downto 0)); end; architecture rtl of modes is constant marker : integer := 0; begin end;");
      FAIL() << "unsupported port mode accepted";
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find("RTL ports must be in or out"), std::string::npos);
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, RTLPackageAndExpressionDiagnosticsRollBackDesigns) {
  const std::vector<std::pair<std::string, std::string>> cases{
    {R"(
package helpers is
  function f(x : bit) return bit is
    constant n : integer := items'length;
  begin return x; end;
  type items is array(0 to 1) of integer;
end;
use work.helpers.all;
entity checked is port(a : in bit; y : out bit); end;
architecture rtl of checked is begin y <= f(a); end;
)", "array type is used before its declaration"},
    {R"(
package helpers is
  constant preset : bit := f('0');
  function f(x : bit) return bit is begin return x; end;
end;
use work.helpers.all;
entity checked is port(y : out bit); end;
architecture rtl of checked is begin y <= preset; end;
)", "function is used before its declaration"},
    {R"(
library ieee; use ieee.std_logic_unsigned.all;
entity checked is port(a : in bit_vector(1 downto 0); y : out bit); end;
architecture rtl of checked is constant marker : integer := 0;
begin y <= '1' when a < a else '0'; end;
)", "ordering requires numeric vectors or hardware integers"},
    {R"(
package helpers is
  type data_t is array(integer range <>) of bit;
  function f(data : data_t) return integer;
end;
package body helpers is function f(data : data_t) return integer is begin return 0; end; end;
use work.helpers.all;
entity checked is port(y : out bit); end;
architecture rtl of checked is constant data : data_t(0 to 1) := (others => '0');
  constant n : integer := f(data); begin y <= '0'; end;
)", "static array parameters require a one-dimensional integer array type"},
    {R"(
package helpers is
  function f(data : data_t) return integer;
  type data_t is array(integer range <>) of integer;
end;
package body helpers is function f(data : data_t) return integer is begin return 0; end; end;
use work.helpers.all;
entity checked is port(y : out bit); end;
architecture rtl of checked is constant data : data_t(0 to 1) := (others => 0);
  constant n : integer := f(data); begin y <= '0'; end;
)", "array type is used before its declaration"},
    {R"(
entity checked is port(y : out bit); end;
architecture rtl of checked is
  type data_t is array(natural range <>) of integer;
  constant data : data_t(0 to 1) := (others => 0);
  function f(data : data_t(-1 to 0)) return integer is begin return 0; end;
  constant n : integer := f(data); begin y <= '0'; end;
)", "array bounds violate the index subtype"},
    {R"(
package helpers is
  constant marker : bit := '1';
  function f return boolean;
end;
package body helpers is function f return boolean is
  begin return false and boolean(marker); end; end;
use work.helpers.all;
entity checked is port(y : out bit); end;
architecture rtl of checked is constant n : boolean := f; begin y <= '0'; end;
)", "unsupported static scalar type"},
    {R"(
package helpers is
  function f(x : bit) return bit;
  function f(x : bit_vector) return bit;
end;
package body helpers is function f(x : bit) return bit is begin return x; end; end;
use work.helpers.all;
entity checked is port(a : in bit; y : out bit); end;
architecture rtl of checked is begin y <= f(a); end;
)", "ambiguous or overloaded package function"}
  };
  for (const auto& [source, diagnostic] : cases) {
    SCOPED_TRACE(source);
    try {
      VHDLConstructor(library_).construct(source);
      FAIL() << "invalid source accepted";
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}


TEST_F(VHDLConstructorTest, SizedBitStringLiteralsPreserveHardwareBits) {
  for (const auto& [literal, expected] : std::vector<std::pair<std::string, std::string>>{
      {"5x\"0d\"", "01101"}, {"9x\"3e\"", "000111110"},
      {"5sx\"d\"", "11101"}, {"8d\"37\"", "00100101"},
      {"8ux\"Z\"", "0000ZZZZ"}, {"8sx\"X\"", "XXXXXXXX"},
      {"70d\"1180591620717411303423\"", std::string(70, '1')}}) {
    SCOPED_TRACE(literal);
    auto* design = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; entity sample is port(y : out std_logic_vector(1 to " +
        std::to_string(expected.size()) + ")); end; architecture rtl of sample is begin y <= " + literal + "; end;");
    auto* output = design->getBusTerm(NLName("y"));
    ASSERT_NE(output, nullptr);
    for (size_t i = 0; i < expected.size(); ++i) {
      SNLEquipotential bit(output->getBit(i + 1), SNLEquipotential::Mode::TraverseAssigns);
      EXPECT_EQ(bit.isConst0(), expected[i] == '0');
      EXPECT_EQ(bit.isConst1(), expected[i] == '1');
      EXPECT_EQ(bit.isConstX(), expected[i] == 'X');
      EXPECT_EQ(bit.isConstZ(), expected[i] == 'Z');
    }
    design->destroy();
  }
}


TEST_F(VHDLConstructorTest, UnknownChecksUseSynthesisSemantics) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity checked is port(a : in std_ulogic; v : in std_ulogic_vector(2 downto 0);
  s : in signed(2 downto 0); y : out std_ulogic; flags : out std_ulogic_vector(2 downto 0)); end;
architecture rtl of checked is
  function selected(a : std_ulogic) return std_ulogic is
  begin if is_x(a) then return 'X'; else return not a; end if; end;
begin
  process(all) begin
    if is_x(v) or is_x(s) then y <= 'X'; else y <= selected(a); end if;
  end process;
  flags(0) <= '1' when is_x('X') else '0';
  flags(1) <= '1' when is_x(s => 'Z') else '0';
  flags(2) <= '1' when is_x('1') else '0';
end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
  }
  for (int i = 0; i < 3; ++i)
    EXPECT_EQ(assignSource(design->getBusTerm(NLName("flags"))->getBit(i)->getNet())->isConstant1(), i != 2);
}

TEST_F(VHDLConstructorTest, InvalidUnknownCheckOperandsAreRejected) {
  for (const auto* expression : {"is_x(true)", "is_x(3)", "is_x(a, a)", "is_x(missing => a)"}) {
    SCOPED_TRACE(expression);
    EXPECT_THROW(VHDLConstructor(library_).construct(
        std::string("library ieee; use ieee.std_logic_1164.all; entity checked is port(a : in std_logic; y : out bit); end; ") +
        "architecture rtl of checked is begin y <= '1' when " + expression + " else '0'; end;"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}


TEST_F(VHDLConstructorTest, SubtypesKeepBaseTypesBoundsAndFunctionScope) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
package types is
  constant extent : integer := 3;
  subtype lane_t is std_ulogic_vector(extent-1 downto 0);
  subtype count_t is integer range 0 to 9;
  function adjusted(a : lane_t) return lane_t;
  function sum(a, b : count_t) return count_t;
end;
package body types is
  function adjusted(a : lane_t) return lane_t is variable result : lane_t;
  begin result := not a; return result; end;
  function sum(a, b : count_t) return count_t is begin return a + b; end;
end;
use work.types.all;
entity aliases is port(a : in lane_t; y : out lane_t; flags : out bit_vector(1 downto 0)); end;
architecture rtl of aliases is
  subtype local_t is lane_t;
  constant width : integer := 8;
  constant n : count_t := sum(2, 3);
  signal temp : local_t;
begin
  temp <= adjusted(a); y <= temp;
  g: for i in 0 to 1 generate subtype local_flag is bit; signal flag : local_flag;
  begin flag <= '1' when n = 5 else '0'; flags(i) <= flag; end generate;
end;
)");
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (pattern >> i) & 1;
    for (unsigned i = 0; i < 3; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), !bool((pattern >> i) & 1));
  }
}

TEST_F(VHDLConstructorTest, StaticWhileFunctionsElaborateAndHonorExits) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity calculated is port(y : out bit_vector(1 downto 0)); end;
architecture rtl of calculated is
  function steps(n : natural) return natural is variable remaining : natural := n; variable result : natural := 0;
  begin
    while remaining > 1 loop
      remaining := remaining / 3; result := result + 1; exit when result = 2;
    end loop;
    return result;
  end;
begin y <= "11" when steps(30) = 2 and steps(1) = 0 else "00"; end;
)");
  for (auto* bit : design->getBusTerm(NLName("y"))->getBits()) EXPECT_TRUE(assignSource(bit->getNet())->isConstant1());
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is function stalled return integer is
  begin while true loop null; end loop; return 0; end;
  constant n : integer := stalled;
begin y <= '0'; end;
)"), NLException);
}


TEST_F(VHDLConstructorTest, ReductionsPreserveLogicAcrossWidthsAndDirections) {
  for (const unsigned width : {1u, 4u}) for (const bool ascending : {false, true}) {
    const auto range = ascending ? "3 to " + std::to_string(width + 2) : std::to_string(width + 2) + " downto 3";
    auto* design = VHDLConstructor(library_).construct(
        "entity reduced is port(a : in bit_vector(" + range + "); y : out bit_vector(5 downto 0)); end; "
        "architecture rtl of reduced is begin y(0) <= and a; y(1) <= or a; y(2) <= xor a; "
        "y(3) <= nand a; y(4) <= nor a; y(5) <= xnor a; end;");
    for (unsigned pattern = 0; pattern < (1u << width); ++pattern) {
      std::unordered_map<SNLBitNet*, bool> values;
      std::unordered_set<SNLBitNet*> visiting;
      bool parity = false;
      for (unsigned i = 0; i < width; ++i) {
        const bool bit = (pattern >> i) & 1;
        values[design->getBusTerm(NLName("a"))->getBit(i + 3)->getNet()] = bit;
        parity ^= bit;
      }
      const std::vector<bool> expected{pattern == (1u << width)-1, pattern != 0, parity,
          pattern != (1u << width)-1, pattern == 0, !parity};
      for (unsigned i = 0; i < expected.size(); ++i)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), expected[i]);
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, ProcessConstantsAndTemporaryInitializersStayLocal) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity local_objects is port(a : in bit; y, z : out bit); end;
architecture rtl of local_objects is begin
  process(all)
    constant invert : bit := '1';
    variable temp : bit := '0';
  begin temp := a xor invert; y <= temp; end process;
  process(all)
    constant invert : bit := '0';
    variable temp : bit := '1';
  begin temp := a xor invert; z <= temp; end process;
end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("z"))->getNet(), values, visiting), input);
  }
}

TEST_F(VHDLConstructorTest, InputPortExpressionsBuildTheirCombinationalLogic) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= a; end;
entity parent is port(a, b : in bit; y : out bit); end;
architecture rtl of parent is begin u: entity work.leaf port map(a => a xor not b, y => y); end;
)", "parent");
  auto* child = design->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* actual = child->getInstTerm(child->getModel()->getScalarTerm(NLName("a")))->getNet();
  for (const bool a : {false, true}) for (const bool b : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), a},
        {design->getScalarTerm(NLName("b"))->getNet(), b}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(actual, values, visiting), a != !b);
  }
}

TEST_F(VHDLConstructorTest, ClockGuardEnableKeepsThePreviousValue) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity enabled is port(clk, d, ena : in bit; q : out bit); end;
architecture rtl of enabled is begin process(clk) begin
  if rising_edge(clk) and ena = '1' then q <= d; end if;
end process; end;
)");
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 1u);
  for (const bool previous : {false, true}) for (const bool data : {false, true}) for (const bool enable : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{flops.front().output, previous},
        {design->getScalarTerm(NLName("d"))->getNet(), data},
        {design->getScalarTerm(NLName("ena"))->getNet(), enable}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(flops.front().data, values, visiting), enable ? data : previous);
  }
}

TEST_F(VHDLConstructorTest, SubtypeRangesAndLabeledCasesPreserveAssignments) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity indexed is port(a : in bit_vector(2 downto 0); y : out bit_vector(2 downto 0)); end;
architecture rtl of indexed is subtype index_t is natural range 0 to 2; begin
  process(all) begin
    lanes: for i in index_t loop
      y(i) <= a(i);
      choice: case a(i) is when '0' => y(i) <= '1'; when others => end case choice;
    end loop lanes;
  end process;
end;
)");
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (pattern >> i) & 1;
    for (unsigned i = 0; i < 3; ++i) EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting));
  }
}


TEST_F(VHDLConstructorTest, ProceduresUpdateRecordFieldsAndPreserveOtherValues) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity updates is port(a, b, en : in bit; y : out bit_vector(2 downto 0)); end;
architecture rtl of updates is
  type inner_t is record first, second : bit; end record;
  type packet_t is record fields : inner_t; unchanged : bit; end record;
  procedure replace(variable value : inout inner_t; incoming : in bit) is
  begin value.first := incoming; end;
  procedure adjust(value : inout packet_t; enabled, incoming : in bit) is
  begin
    replace(incoming => incoming, value => value.fields);
    if enabled = '1' then value.fields.second := not incoming; end if;
  end;
begin
  process(all) variable temp : packet_t;
  begin
    temp := (fields => (first => '0', others => b), unchanged => a);
    adjust(temp, en, a);
    y <= temp.fields.first & temp.fields.second & temp.unchanged;
  end process;
end;
)");
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    const bool a = pattern & 1, b = pattern & 2, en = pattern & 4;
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), a},
        {design->getScalarTerm(NLName("b"))->getNet(), b}, {design->getScalarTerm(NLName("en"))->getNet(), en}};
    std::unordered_set<SNLBitNet*> visiting;
    const std::vector<bool> expected{a, en ? !a : b, a};
    for (unsigned i = 0; i < 3; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), expected[i]);
  }
}

TEST_F(VHDLConstructorTest, EnumerationLiteralsResolveFromTheirNominalContext) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity enums is port(a : in bit; y : out bit); end;
architecture rtl of enums is
  type first_t is (idle, active);
  type second_t is (active, idle);
  signal first : first_t; signal second : second_t;
begin
  first <= active when a = '1' else idle;
  second <= idle when a = '1' else active;
  y <= '1' when first = active and idle = second and first > idle else '0';
end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), input);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is type first_t is (idle, active); type second_t is (active, idle);
begin y <= '1' when idle = active else '0'; end;
)"), NLException);
}


TEST_F(VHDLConstructorTest, BasedIntegerLiteralsAndScalarExtremaElaborate) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity literals is port(y : out bit_vector(2 downto 0)); end;
architecture rtl of literals is
  subtype count_t is integer range 0 to 16#7f#;
  constant count : count_t := maximum(2#1_01#E+2, 1e1);
  constant lower : count_t := minimum(16#2a#, 8#17#);
begin y <= "101" when count = 20 and lower = 15 and 16#e# = 14 else "000"; end;
)");
  for (int i = 0; i < 3; ++i) EXPECT_EQ(assignSource(design->getBusTerm(NLName("y"))->getBit(i)->getNet())->isConstant1(), i != 1);
  for (const auto* value : {"1e-1", "2#2#", "17#1#", "16#ff#e20", "10#999999999999999999999#"}) {
    SCOPED_TRACE(value);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is port(y : out bit); end; ") +
        "architecture rtl of invalid is constant n : integer := " + value + "; begin y <= '0'; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, ProcessLocalsHideImportedLiteralsAndRestoreThemAfterwards) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type phase_t is (idle, active); end;
use work.types.all;
entity scopes is port(a : in bit; y, z : out bit); end;
architecture rtl of scopes is signal phase : phase_t;
begin
  process(all) variable active : bit;
  begin active := not a; y <= active; end process;
  phase <= active;
  z <= '1' when phase = active else '0';
end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
    EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("z"))->getNet(), values, visiting));
  }
}

TEST_F(VHDLConstructorTest, EnumPortsAndFunctionLiteralOverloadsKeepTheirTypes) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is
  type phase_t is (idle, active, done);
  function active(value : bit) return bit;
end;
package body types is function active(value : bit) return bit is begin return not value; end; end;
use work.types.all;
entity leaf is port(phase : in phase_t; data : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= active(data) when phase = active else '0'; end;
use work.types.all;
entity parent is port(a : in bit; y : out bit); end;
architecture rtl of parent is begin u: entity work.leaf port map(phase => active, data => a, y => y); end;
)", "parent");
  auto* child = design->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* phase = child->getModel()->getBusTerm(NLName("phase"));
  ASSERT_NE(phase, nullptr);
  ASSERT_EQ(phase->getWidth(), 2u);
  EXPECT_TRUE(child->getInstTerm(phase->getBit(0))->getNet()->isConstant1());
  EXPECT_TRUE(child->getInstTerm(phase->getBit(1))->getNet()->isConstant0());
}


TEST_F(VHDLConstructorTest, CombinationalInitializersMustMatchInitializedState) {
  for (const auto* initial : {"'0'", "'1'"}) {
    SCOPED_TRACE(initial);
    const auto source = std::string("entity startup is port(clk, d : in bit; y : out bit); end; ") +
        "architecture rtl of startup is signal stored : bit := '0'; signal next_value : bit := " + initial +
        "; begin process(clk) begin if rising_edge(clk) then stored <= d; end if; end process; "
        "next_value <= not stored; y <= next_value; end;";
    if (std::string(initial) == "'0'") EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    else {
      auto* design = VHDLConstructor(library_).construct(source);
      ASSERT_NE(design, nullptr);
      const auto flops = dffBits(design);
      ASSERT_EQ(flops.size(), 1u);
      std::unordered_map<SNLBitNet*, bool> values{{flops.front().output, false}};
      std::unordered_set<SNLBitNet*> visiting;
      EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting));
    }
  }
}

TEST_F(VHDLConstructorTest, IntegerTemporariesPreserveSignedComparisonsAndSliceBounds) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity temporaries is port(a : in unsigned(2 downto 0); y : out bit_vector(3 downto 0); negative : out bit); end;
architecture rtl of temporaries is begin
  process(all)
    variable selected, offset, delta : integer;
    variable result : bit_vector(3 downto 0);
  begin
    selected := to_integer(a);
    delta := selected - 3;
    if delta < 0 then negative <= '1'; else negative <= '0'; end if;
    result := "0000";
    for lane in 0 to 1 loop
      if selected = lane then
        offset := lane * 2;
        result(offset + 1 downto offset) := "11";
      end if;
    end loop;
    y <= result;
  end process;
end;
)");
  for (unsigned input = 0; input < 8; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("negative"))->getNet(), values, visiting), input < 3);
    const unsigned expected = input < 2 ? 3u << (input * 2) : 0;
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((expected >> i) & 1));
  }
}

TEST_F(VHDLConstructorTest, NumericConstantShiftsAndFunctionCasesPreserveBits) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity shifts is port(a : in signed(0 to 3); choice : in bit; y : out signed(3 downto 0);
  z : out unsigned(3 downto 0)); end;
architecture rtl of shifts is
  function transform(value : signed; mode : bit) return signed is
    variable result : signed(3 downto 0);
  begin
    case mode is
      when '0' => result := shift_right(value, 2);
      when '1' => result := shift_left(value, 1) + to_signed(-2, 4);
    end case;
    return result;
  end;
  function choose(value : signed; mode : bit) return signed is
  begin
    case mode is
      when '0' => return transform(value, '0');
      when '1' => return transform(value, '1');
    end case;
  end;
begin y <= choose(a, choice); z <= rotate_left(unsigned(a), 5); end;
)");
  for (unsigned input = 0; input < 16; ++input) for (bool choice : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(3-i)->getNet()] = (input >> i) & 1;
    values[design->getScalarTerm(NLName("choice"))->getNet()] = choice;
    const int signedInput = input & 8 ? int(input) - 16 : int(input);
    const unsigned expected = choice ? (input * 2 - 2) & 15 : unsigned(signedInput >> 2) & 15;
    const unsigned rotated = ((input << 1) | (input >> 3)) & 15;
    for (unsigned i = 0; i < 4; ++i) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((expected >> i) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(i)->getNet(), values, visiting), bool((rotated >> i) & 1));
    }
  }
}

TEST_F(VHDLConstructorTest, NumericScalarCarryAndSignedUnaryOperators) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity unary_ops is port(a : in unsigned(3 downto 0); carry : in std_ulogic;
  sum : out unsigned(3 downto 0); reverse_sum, negated, magnitude : out signed(3 downto 0)); end;
architecture rtl of unary_ops is begin
  sum <= a + carry;
  reverse_sum <= carry + signed(a);
  negated <= -signed(a);
  magnitude <= abs(signed(a));
end;
)");
  for (unsigned input = 0; input < 16; ++input) for (unsigned carry = 0; carry < 2; ++carry) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    values[design->getScalarTerm(NLName("carry"))->getNet()] = carry;
    const unsigned sum = (input + carry) & 15;
    const unsigned negated = (16 - input) & 15;
    for (unsigned i = 0; i < 4; ++i) {
      for (const auto* output : {"sum", "reverse_sum"})
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(output))->getBit(i)->getNet(), values, visiting), bool((sum >> i) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("negated"))->getBit(i)->getNet(), values, visiting), bool((negated >> i) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("magnitude"))->getBit(i)->getNet(), values, visiting),
          bool(((input & 8 ? negated : input) >> i) & 1));
    }
  }
}

TEST_F(VHDLConstructorTest, FunctionIntegerTemporariesSupplyNestedLoopBounds) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity chunks is port(a : in std_ulogic_vector(3 downto 0); y : out std_ulogic_vector(1 downto 0)); end;
architecture rtl of chunks is
  function inspect(value : std_ulogic_vector) return std_ulogic_vector is
    variable result : std_ulogic_vector(1 downto 0);
    variable stride, offset : natural;
    variable accumulated : std_ulogic;
  begin
    stride := 1;
    for stage in 0 to 1 loop
      accumulated := '0';
      for chunk in 0 to (4 / stride) - 1 loop
        offset := chunk * stride;
        accumulated := accumulated or (and value(offset + stride - 1 downto offset));
      end loop;
      result(stage) := accumulated;
      stride := stride * 2;
    end loop;
    return result;
  end;
begin y <= inspect(a); end;
)");
  for (unsigned input = 0; input < 16; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(0)->getNet(), values, visiting), input != 0);
    EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(1)->getNet(), values, visiting), (input & 3) == 3 || (input & 12) == 12);
  }
}

TEST_F(VHDLConstructorTest, RuntimeScalarFunctionArgumentsKeepTheirTypes) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity scalar_calls is port(a : in unsigned(2 downto 0); subtract : in bit; y : out bit); end;
architecture rtl of scalar_calls is
  function calculate(value : integer; lower : boolean) return integer is
  begin if lower then return value - 2; else return value + 1; end if; end;
begin y <= '1' when calculate(to_integer(a), subtract = '1') < 0 else '0'; end;
)");
  for (unsigned input = 0; input < 8; ++input) for (bool subtract : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    values[design->getScalarTerm(NLName("subtract"))->getNet()] = subtract;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), subtract && input < 2);
  }
}

TEST_F(VHDLConstructorTest, AcyclicScheduledSignalReadsSettleWithoutState) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity deltas is port(a : in bit; y : out bit); end;
architecture rtl of deltas is signal first, second : bit;
begin process(all) begin y <= second; second <= not first; first <= a; end process; end;
)");
  for (bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
  }
  for (const auto* assignments : {"first <= not first;", "first <= second; second <= not first;"}) {
    SCOPED_TRACE(assignments);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is port(y : out bit); end; ") +
        "architecture rtl of invalid is signal first, second : bit; begin process(all) begin " +
        assignments + " y <= first; end process; end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, CombinationalSelfReadProofTraversesHierarchyAndStopsAtRegisters) {
  for (bool registered : {false, true}) {
    const auto source = std::string("entity leaf is port(clk, d : in bit; q : out bit); end; ") +
        "architecture rtl of leaf is begin " + (registered ?
          "process(clk) begin if rising_edge(clk) then q <= d; end if; end process;" : "q <= d;") +
        " end; entity parent is port(clk : in bit; y : out bit); end; "
        "architecture rtl of parent is signal next_value, stored : bit; begin "
        "u: entity work.leaf port map(clk, next_value, stored); "
        "process(all) begin next_value <= not stored; y <= next_value; end process; end;";
    if (!registered) EXPECT_THROW(VHDLConstructor(library_).construct(source, "parent"), NLException);
    else {
      auto* design = VHDLConstructor(library_).construct(source, "parent");
      auto* child = design->getInstance(NLName("u"));
      ASSERT_EQ(dffBits(child->getModel()).size(), 1u);
      for (bool stored : {false, true}) {
        auto* output = child->getModel()->getScalarTerm(NLName("q"));
        std::unordered_map<SNLBitNet*, bool> values{{child->getInstTerm(output)->getNet(), stored}};
        std::unordered_set<SNLBitNet*> visiting;
        EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !stored);
      }
    }
  }
}

TEST_F(VHDLConstructorTest, UnusedMathRealImportsDoNotEnableRealOperations) {
  for (bool rtl : {false, true}) {
    const auto name = rtl ? "rtl_import" : "simple_import";
    const auto source = std::string("library ieee; use ieee.math_real.all; entity ") + name +
        " is port(a : in bit; y : out bit); end; architecture rtl of " + name + " is " +
        (rtl ? "constant marker : integer := 0;" : "") + " begin y <= a; end;";
    EXPECT_NE(VHDLConstructor(library_).construct(source), nullptr);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
library ieee; use ieee.math_real.all;
entity invalid is port(y : out bit); end;
architecture rtl of invalid is constant n : integer := integer(ceil(1.5)); begin y <= '0'; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, GeneratedMemoriesKeepIndependentStateAndReadBeforeWrite) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity scoped_ram is port(clk, we : in std_ulogic; address : in unsigned(1 downto 0);
  data : in std_ulogic_vector(7 downto 0); q : out std_ulogic_vector(7 downto 0)); end;
architecture rtl of scoped_ram is subtype index_t is natural range 0 to 3;
begin
  g: for lane in 0 to 1 generate
    type table_t is array(index_t) of std_ulogic_vector(3 downto 0);
    signal cells : table_t;
  begin
    process(clk) begin if rising_edge(clk) then
      q(lane*4+3 downto lane*4) <= cells(to_integer(address));
      if we='1' then cells(to_integer(address)) <= data(lane*4+3 downto lane*4); end if;
    end if; end process;
  end generate;
end;
)");
  auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 2u);
  EXPECT_NE(design->getInstance(NLName("g[0].cells_mem")), nullptr);
  EXPECT_NE(design->getInstance(NLName("g[1].cells_mem")), nullptr);
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 8u);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  unsigned expected[4]{};
  for (unsigned cycle = 0; cycle < 24; ++cycle) {
    const unsigned address = cycle % 4, data = (cycle * 19 + 7) & 255;
    const bool write = cycle % 3 != 0;
    auto values = state;
    values[design->getScalarTerm(NLName("we"))->getNet()] = write;
    for (unsigned i = 0; i < 2; ++i) values[design->getBusTerm(NLName("address"))->getBit(i)->getNet()] = (address >> i) & 1;
    for (unsigned i = 0; i < 8; ++i) values[design->getBusTerm(NLName("data"))->getBit(i)->getNet()] = (data >> i) & 1;
    auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
    std::unordered_set<SNLBitNet*> visiting;
    for (const auto& flop : flops) state[flop.output] = evaluateRTL(flop.data, values, visiting, &memories);
    for (unsigned i = 0; i < 8; ++i)
      EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBit(i)->getNet()), bool((expected[address] >> i) & 1));
    if (write) expected[address] = data;
    memories = std::move(nextMemories);
  }
}

TEST_F(VHDLConstructorTest, AggregateBitLimitIsIndependentOfIndividualDimensionLimit) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity wide_array is port(a : in bit; y : out bit); end;
architecture rtl of wide_array is
  type table_t is array(0 to 16) of bit_vector(4095 downto 0);
  signal unused_cells : table_t;
begin y <= a; end;
)");
  for (unsigned word = 0; word < 17; ++word) {
    auto* net = design->getBusNet(NLName("unused_cells(" + std::to_string(word) + ")"));
    ASSERT_NE(net, nullptr);
    EXPECT_EQ(net->getWidth(), 4096u);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is type table_t is array(0 to 256) of bit_vector(4095 downto 0);
begin y <= '0'; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, MaskedMemoryWritesPreserveDisabledLanesAndReadBeforeWrite) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity masked_ram is port(clk : in std_ulogic; enables, address : in unsigned(1 downto 0);
  data : in std_ulogic_vector(7 downto 0); q : out std_ulogic_vector(7 downto 0)); end;
architecture rtl of masked_ram is type table_t is array(0 to 3) of std_ulogic_vector(7 downto 0);
  signal cells : table_t;
begin
  process(clk) variable lower, upper : integer range 0 to 7; variable selected : integer range 0 to 3;
  begin if rising_edge(clk) then
    for lane in 0 to 1 loop
      lower := lane*4; upper := lower+3;
      if enables(lane)='1' then
        selected := to_integer(address);
        cells(selected)(upper downto lower) <= data(upper downto lower);
      end if;
    end loop;
    q <= cells(to_integer(address));
  end if; end process;
end;
)");
  auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 1u);
  auto* memory = memories.begin()->first;
  const auto signature = NLDB0::getMemorySignature(memory);
  EXPECT_TRUE(signature.independentWriteClocks);
  EXPECT_EQ(signature.writePorts, 1u);
  EXPECT_EQ(signature.width, 8u);
  EXPECT_EQ(memory->getInstTerm(memory->getModel()->getBusTerm(NLName("WCLK"))->getBit(0))->getNet(),
      design->getScalarTerm(NLName("clk"))->getNet());
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 8u);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  unsigned expected[4]{};
  for (unsigned cycle = 0; cycle < 64; ++cycle) {
    const unsigned address = cycle % 4, enables = (cycle / 4) % 4, data = (cycle * 23 + 9) & 255;
    auto values = state;
    for (unsigned i = 0; i < 2; ++i) {
      values[design->getBusTerm(NLName("address"))->getBit(i)->getNet()] = (address >> i) & 1;
      values[design->getBusTerm(NLName("enables"))->getBit(i)->getNet()] = (enables >> i) & 1;
    }
    for (unsigned i = 0; i < 8; ++i) values[design->getBusTerm(NLName("data"))->getBit(i)->getNet()] = (data >> i) & 1;
    auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
    std::unordered_set<SNLBitNet*> visiting;
    for (const auto& flop : flops) state[flop.output] = evaluateRTL(flop.data, values, visiting, &memories);
    for (unsigned i = 0; i < 8; ++i)
      EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBit(i)->getNet()), bool((expected[address] >> i) & 1));
    const unsigned mask = (enables & 1 ? 15 : 0) | (enables & 2 ? 240 : 0);
    expected[address] = (expected[address] & ~mask) | (data & mask);
    EXPECT_EQ(nextMemories.at(memory)[address], expected[address]);
    memories = std::move(nextMemories);
  }
}

TEST_F(VHDLConstructorTest, OutputPortsCanBeReadWithVHDL2008SignalSemantics) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity readable_outputs is port(a : in bit; first, second : out bit); end;
architecture rtl of readable_outputs is begin first <= a; second <= not first; end;
)");
  for (bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("first"))->getNet(), values, visiting), input);
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("second"))->getNet(), values, visiting), !input);
  }
}

TEST_F(VHDLConstructorTest, ArraySubtypeAttributesRetainBoundsInsideFunctions) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity alias_attributes is port(a : in bit_vector(8 downto 0); y : out bit_vector(2 to 6)); end;
architecture rtl of alias_attributes is
  subtype lane_t is bit_vector(2 to 6);
  constant width : integer := lane_t'length;
  function select_lane(value : bit_vector) return lane_t is
    variable result : lane_t;
  begin
    result(lane_t'range) := value(lane_t'reverse_range);
    return result;
  end;
begin y <= select_lane(a); end;
)");
  for (unsigned input = 0; input < 512; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 9; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 2; i <= 6; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((input >> (8-i)) & 1));
  }
}

TEST_F(VHDLConstructorTest, ClockedOutputReadsUsePreEdgeValues) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity readable_register is port(clk : in bit; q : out bit); end;
architecture rtl of readable_register is begin
  process(clk) begin if rising_edge(clk) then q <= not q; end if; end process;
end;
)");
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 1u);
  for (bool previous : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{flops.front().output, previous}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(flops.front().data, values, visiting), !previous);
  }
}

TEST_F(VHDLConstructorTest, EnumerationPositionAndValuePreserveNominalTypesAndRangeChecks) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity ordinals is port(a : in unsigned(2 downto 0); position : out unsigned(3 downto 0); active : out bit); end;
architecture rtl of ordinals is
  type phase_t is (idle, busy, done);
  type other_t is (done, idle);
  signal phase : phase_t;
  constant last : integer := phase_t'pos(done);
begin
  phase <= phase_t'val(to_integer(a));
  position <= to_unsigned(phase_t'pos(phase), 4);
  active <= '1' when phase=busy and last=2 and other_t'pos(done)=0 else '0';
end;
)");
  for (unsigned input = 0; input < 3; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("position"))->getBit(i)->getNet(), values, visiting), bool((input >> i) & 1));
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("active"))->getNet(), values, visiting), input == 1);
  }
  // Invalid runtime ordinals must select X rather than wrapping onto a valid literal.
  bool guarded = false;
  for (auto* instance : design->getInstances()) if (NLDB0::isMux2(instance->getModel()))
    for (auto* input : NLDB0::getMux2InputA(instance->getModel())->getBits())
      guarded |= instance->getInstTerm(input)->getNet()->isConstantX();
  EXPECT_TRUE(guarded);
  for (const auto* argument : {"-1", "3", "true"}) {
    SCOPED_TRACE(argument);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is port(y : out bit); end; ") +
        "architecture rtl of invalid is type phase_t is (idle, busy, done); signal phase : phase_t; begin "
        "phase <= phase_t'val(" + argument + "); y <= '0'; end;"), NLException);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(y : out bit); end;
architecture rtl of invalid is type first_t is (idle, busy); type second_t is (stopped, running);
constant ordinal : integer := first_t'pos(running); begin y <= '0'; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, StandardMatchHonorsLiteralWildcardsAndOperandTypes) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity patterns is port(a : in unsigned(0 to 3); bit_input : in std_ulogic;
  y : out bit_vector(3 downto 0)); end;
architecture rtl of patterns is
  constant left_x : std_logic_vector(1 downto 0) := "X0";
  constant both_x : std_logic_vector(1 downto 0) := "XX";
begin
  y(0) <= '1' when std_match(a, "1-0-") else '0';
  y(1) <= '1' when std_match(r => a, l => "-11-") else '0';
  y(2) <= '1' when std_match(bit_input, '-') else '0';
  y(3) <= '1' when std_match(left_x, "-0") and not std_match(both_x, "XX") and not std_match(a, "00") else '0';
end;
)");
  for (unsigned input = 0; input < 16; ++input) for (bool bitInput : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("a"))->getBit(3-i)->getNet()] = (input >> i) & 1;
    values[design->getScalarTerm(NLName("bit_input"))->getNet()] = bitInput;
    const bool expected[]{(input & 10) == 8, (input & 6) == 6, true, true};
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), expected[i]);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity invalid is port(a : in bit_vector(1 downto 0); y : out bit); end;
architecture rtl of invalid is begin y <= '1' when std_match(a, "--") else '0'; end;
)"), NLException);
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
library ieee; use ieee.numeric_std.all;
entity invalid is port(y : out bit); end;
architecture rtl of invalid is begin y <= '1' when std_match("00", "00") else '0'; end;
)"), NLException);
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity invalid is port(a : in unsigned(1 downto 0); y : out bit); end;
architecture rtl of invalid is begin y <= '1' when (others => '0') = a else '0'; end;
)"), NLException);

}

TEST_F(VHDLConstructorTest, ConcatenatedAggregatesInferBoundsAndComparisonContext) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity aggregate_context is port(a : in unsigned(3 downto 0); y, z : out unsigned(7 downto 0);
  zero, pattern : out bit); end;
architecture rtl of aggregate_context is begin
  y <= (1 downto 0 => '1') & a & (0 => '1', 1 => '0');
  z <= (1 => '1', 0 => '0') & a & ('1', '0');
  zero <= '1' when (3 downto 0 => '0') = a else '0';
  pattern <= '1' when "1010" = a else '0';
end;
)");
  for (unsigned input = 0; input < 16; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 0; i < 8; ++i) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool(((0xc2 | (input << 2)) >> i) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(i)->getNet(), values, visiting), bool(((0x42 | (input << 2)) >> i) & 1));
    }
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("zero"))->getNet(), values, visiting), input == 0);
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("pattern"))->getNet(), values, visiting), input == 10);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(a : in bit; y : out bit_vector(1 downto 0)); end;
architecture rtl of invalid is begin y <= (0 => '1', 0 => '0') & a; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, DynamicFixedWidthSlicesPreservePositionAndFunctionResults) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity dynamic_slices is port(a : in std_ulogic_vector(7 downto 0); address : in unsigned(1 downto 0);
  y, z : out std_ulogic_vector(1 downto 0)); end;
architecture rtl of dynamic_slices is
  signal ascending_data : std_ulogic_vector(4 to 11);
  function select_word(data : std_ulogic_vector; index : integer) return std_ulogic_vector is
  begin return data(index*2+1 downto index*2); end;
begin
  ascending_data <= a;
  y <= select_word(a, to_integer(address));
  z <= ascending_data(to_integer(address)*2+4 to to_integer(address)*2+5);
end;
)");
  for (unsigned input = 0; input < 256; ++input) for (unsigned address = 0; address < 4; ++address) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 8; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 0; i < 2; ++i) values[design->getBusTerm(NLName("address"))->getBit(i)->getNet()] = (address >> i) & 1;
    for (unsigned i = 0; i < 2; ++i) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((input >> (address*2+i)) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(i)->getNet(), values, visiting), bool((input >> ((3-address)*2+i)) & 1));
    }
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity invalid is port(a : in std_ulogic_vector(7 downto 0); address : in unsigned(1 downto 0); y : out std_ulogic); end;
architecture rtl of invalid is
  function bounds(data : std_ulogic_vector) return std_ulogic is begin return data(data'left); end;
begin y <= bounds(a(to_integer(address)+1 downto to_integer(address))); end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, DynamicSlicesRejectLargeRuntimeWidthCoefficients) {
  // The coefficient is 2^160: fixed-width arithmetic would wrap it to zero
  // and incorrectly accept this runtime-dependent width as a two-bit slice.
  try {
    VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity variable_slice is port(a : in std_ulogic_vector(7 downto 0);
  address : in unsigned(1 downto 0); y : out std_ulogic_vector(1 downto 0)); end;
architecture rtl of variable_slice is begin
  y <= a(to_integer(address)*4294967296*4294967296*4294967296*4294967296*4294967296+1
         downto to_integer(address)*0);
end;
)");
    FAIL() << "accepted a runtime-dependent slice width";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("runtime-dependent slice widths are unsupported"), std::string::npos);
  }
}

TEST_F(VHDLConstructorTest, DynamicWritesRequireWritableMatchingObjectKind) {
  unsigned variant = 0;
  for (const auto& [declaration, assignment] : std::vector<std::pair<std::string, std::string>>{
         {"", "a(to_integer(address)) <= data;"},
         {"constant fixed : bit_vector(3 downto 0) := (others => '0');",
          "fixed(to_integer(address)) <= data;"},
         {"signal stored : bit_vector(3 downto 0);", "stored(to_integer(address)) := data;"}}) {
    const auto name = "invalid_dynamic_write_" + std::to_string(variant++);
    try {
      VHDLConstructor(library_).construct(
        "library ieee; use ieee.numeric_std.all; entity " + name +
        " is port(a : in bit_vector(3 downto 0); address : in unsigned(1 downto 0); data : in bit); end; "
        "architecture rtl of " + name + " is " + declaration +
        " begin process(all) begin " + assignment + " end process; end;");
      FAIL() << "accepted an invalid dynamic write: " << assignment;
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(
        "dynamic assignment requires a writable object of the matching kind"), std::string::npos)
        << error.what();
    }
  }
}

TEST_F(VHDLConstructorTest, NestedDynamicWritesUpdateRecordFieldsAndVariables) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity nested_writes is port(clk, reset, data : in bit; address, lane : in unsigned(1 downto 0);
  q, y : out bit_vector(5 downto 0)); end;
architecture rtl of nested_writes is
  type packet_t is record bits : bit_vector(1 downto 0); end record;
  type bank_t is array(0 to 2) of packet_t;
  signal stored : bank_t;
begin
  process(clk) begin if rising_edge(clk) then
    if reset='1' then stored <= (others => (bits => (others => '0')));
    else stored(to_integer(address)).bits(to_integer(lane)) <= data; end if;
  end if; end process;
  process(all) variable temporary : bank_t;
  begin
    temporary := stored;
    temporary(to_integer(address)).bits(to_integer(lane)) := not data;
    y <= temporary(0).bits & temporary(1).bits & temporary(2).bits;
  end process;
  q <= stored(0).bits & stored(1).bits & stored(2).bits;
end;
)");
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 6u);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  unsigned expected = 0;
  for (unsigned cycle = 0; cycle < 48; ++cycle) {
    const unsigned address = cycle % 4, lane = (cycle / 4) % 4;
    const bool data = cycle % 3 != 0, reset = cycle % 17 == 0;
    auto values = state;
    values[design->getScalarTerm(NLName("data"))->getNet()] = data;
    values[design->getScalarTerm(NLName("reset"))->getNet()] = reset;
    for (unsigned i = 0; i < 2; ++i) {
      values[design->getBusTerm(NLName("address"))->getBit(i)->getNet()] = (address >> i) & 1;
      values[design->getBusTerm(NLName("lane"))->getBit(i)->getNet()] = (lane >> i) & 1;
    }
    unsigned temporary = expected;
    if (address < 3 && lane < 2) {
      const unsigned mask = 1u << ((2-address)*2+lane);
      temporary = (temporary & ~mask) | (data ? 0 : mask);
    }
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 6; ++i) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("q"))->getBit(i)->getNet(), values, visiting), bool((expected >> i) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((temporary >> i) & 1));
    }
    for (const auto& flop : flops) state[flop.output] = evaluateRTL(flop.data, values, visiting);
    if (reset) expected = 0;
    else if (address < 3 && lane < 2) {
      const unsigned mask = 1u << ((2-address)*2+lane);
      expected = (expected & ~mask) | (data ? mask : 0);
    }
  }
}

TEST_F(VHDLConstructorTest, PowerOfTwoModuloPreservesNegativeDividendSemantics) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity modulo_count is port(a : in unsigned(3 downto 0); y : out unsigned(1 downto 0); z : out unsigned(0 downto 0)); end;
architecture rtl of modulo_count is begin
  y <= to_unsigned((to_integer(a)-5) mod 4, 2);
  z <= to_unsigned(to_integer(a) mod 1, 1);
end;
)");
  for (unsigned input = 0; input < 16; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 0; i < 2; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool(((input+3) >> i) & 1));
    EXPECT_FALSE(evaluateRTL(design->getBusTerm(NLName("z"))->getBit(0)->getNet(), values, visiting));
  }
}

TEST_F(VHDLConstructorTest, RepeatedGuardsPreserveConditionalVariableAssignment) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity guarded_temps is port(enable, select_input, data : in bit; y, z, other : out bit); end;
architecture rtl of guarded_temps is begin
  process(all) variable temporary, case_temporary, unrelated : bit;
  begin
    unrelated := data;
    if enable='1' then temporary := data; end if;
    if select_input='1' then unrelated := not data; end if;
    if enable='1' then y <= temporary; else y <= '0'; end if;
    case select_input is when '0' => null; when '1' => case_temporary := data; end case;
    case select_input is when '0' => z <= '0'; when '1' => z <= case_temporary; end case;
    other <= unrelated;
  end process;
end;
)");
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    const bool enable = pattern & 1, selected = pattern & 2, data = pattern & 4;
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("enable"))->getNet()] = enable;
    values[design->getScalarTerm(NLName("select_input"))->getNet()] = selected;
    values[design->getScalarTerm(NLName("data"))->getNet()] = data;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), enable && data);
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("z"))->getNet(), values, visiting), selected && data);
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("other"))->getNet(), values, visiting), selected ? !data : data);
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(enable, unrelated, data : in bit; y : out bit); end;
architecture rtl of invalid is begin process(all) variable temporary : bit;
begin if enable='1' then temporary := data; end if;
if unrelated='1' then y <= temporary; else y <= '0'; end if; end process; end;
)"), NLException);
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(enable, data : in bit; y : out bit); end;
architecture rtl of invalid is
  procedure choose(en, d : in bit; variable value : out bit) is
  begin if en='1' then value := d; end if; end;
begin process(all) variable temporary : bit;
begin choose(enable, data, temporary); y <= temporary; end process; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, FunctionLocalsHideImportedNamesWithoutChangingCallerScope) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type phase_t is (idle, active); constant bias : integer := 9; end;
use work.types.all;
entity local_scope is port(a : in bit; y, z : out bit); end;
architecture rtl of local_scope is
  signal phase : phase_t;
  function transform(value : bit) return bit is
    variable active : bit := value;
    constant bias : integer := 1;
  begin active := not active; if bias=1 then return active; else return value; end if; end;
begin y <= transform(a); phase <= active; z <= '1' when phase=active and bias=9 else '0'; end;
)");
  for (bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
    EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("z"))->getNet(), values, visiting));
  }
}

TEST_F(VHDLConstructorTest, InterfacesAndArchitectureSignalsHideImportedNames) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type phase_t is (first, second, third); constant bias : integer := 9; end;
use work.types.all;
entity direct_scope is generic(bias : integer := 5); port(first : in bit; y, z : out bit); end;
architecture rtl of direct_scope is signal second : bit; signal phase : phase_t;
begin second <= not first; y <= second; phase <= third; z <= '1' when phase=third and bias=5 else '0'; end;
)");
  for (bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("first"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
    EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("z"))->getNet(), values, visiting));
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(a : in bit; y : out bit); end;
architecture rtl of invalid is signal a : bit; begin a <= '0'; y <= a; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, CombinationalStartupValuesMayBeHiddenBehindRegisteredData) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity settled_input is port(clk, data : in bit; y : out bit); end;
architecture rtl of settled_input is signal next_value : bit := '1'; signal stored : bit := '0';
begin
  next_value <= data;
  process(clk) begin if rising_edge(clk) then stored <= next_value; end if; end process;
  y <= stored;
end;
)");
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 1u);
  bool initialized = false;
  for (auto* instance : design->getInstances()) if (NLDB0::isDFF(instance->getModel())) {
    auto* init = instance->getInstParameter(NLName("INIT"));
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getValue(), "1'b0");
    initialized = true;
  }
  EXPECT_TRUE(initialized);
  for (bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("data"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(flops.front().data, values, visiting), input);
  }
}

TEST_F(VHDLConstructorTest, RuntimeEarlyReturnsRespectLoopOrderAndCaseContinuations) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity early_returns is port(data : in bit_vector(3 downto 0); reverse_order : in bit;
  position : out unsigned(2 downto 0); flags : out bit_vector(1 downto 0)); end;
architecture rtl of early_returns is
  subtype position_t is integer range 0 to 4;
  subtype pair_t is bit_vector(1 downto 0);
  function first_set(value : bit_vector; reverse : boolean) return position_t is
  begin
    if reverse then
      for i in 3 downto 0 loop if value(i)='1' then return i; end if; end loop;
    else
      for i in 0 to 3 loop if value(i)='1' then return i; end if; end loop;
    end if;
    return 4;
  end;
  function classify(value : bit_vector; mode : bit) return pair_t is
    variable result : pair_t := "00";
  begin
    for i in 0 to 1 loop
      case mode is
        when '0' => if value(i)='1' then return "01"; end if;
        when '1' => result := value(1 downto 0); return result;
      end case;
    end loop;
    return "10";
  end;
begin
  position <= to_unsigned(first_set(data, reverse_order='1'),3);
  flags <= classify(data, reverse_order);
end;
)");
  for (unsigned input = 0; input < 16; ++input) for (bool reverse : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i) values[design->getBusTerm(NLName("data"))->getBit(i)->getNet()] = (input >> i) & 1;
    values[design->getScalarTerm(NLName("reverse_order"))->getNet()] = reverse;
    unsigned expected = 4;
    for (unsigned i = 0; i < 4; ++i) {
      const unsigned bit = reverse ? 3-i : i;
      if ((input >> bit) & 1) { expected = bit; break; }
    }
    const unsigned flags = reverse ? input & 3 : (input & 3) ? 1 : 2;
    for (unsigned i = 0; i < 3; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("position"))->getBit(i)->getNet(), values, visiting), bool((expected >> i) & 1));
    for (unsigned i = 0; i < 2; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("flags"))->getBit(i)->getNet(), values, visiting), bool((flags >> i) & 1));
  }
  EXPECT_THROW(VHDLConstructor(library_).construct(R"(
entity invalid is port(data : in bit; y : out bit); end;
architecture rtl of invalid is subtype result_t is integer range 0 to 4;
  function f(value : bit) return result_t is begin if value='1' then return 7; else return 0; end if; end;
begin y <= '1' when f(data)=0 else '0'; end;
)"), NLException);
}

TEST_F(VHDLConstructorTest, RecordFieldArraysRetainAllDimensionsDuringSelections) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity field_arrays is port(a : in bit_vector(7 downto 0); y : out bit_vector(7 downto 0)); end;
architecture rtl of field_arrays is
  type lanes_t is array(0 to 1) of bit_vector(3 downto 0);
  type packet_t is record lanes : lanes_t; end record;
begin process(all) variable temporary : packet_t;
  begin
    for i in 0 to 1 loop temporary.lanes(i) := a(i*4+3 downto i*4); end loop;
    y <= temporary.lanes(0) & temporary.lanes(1);
  end process;
end;
)");
  for (unsigned input = 0; input < 256; ++input) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 8; ++i) values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 0; i < 8; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(i)->getNet(), values, visiting), bool((input >> ((i+4)%8)) & 1));
  }
}

TEST_F(VHDLConstructorTest, ExponentAndBasedGenericDefaultsUseTheRTLEvaluator) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity literal_generic is generic(width : positive := 2#10#E2; limit : natural := 1e1);
  port(a : in bit_vector(width-1 downto 0); y : out bit_vector(width-1 downto 0)); end;
architecture rtl of literal_generic is begin y <= a; end;
)");
  EXPECT_EQ(design->getBusTerm(NLName("a"))->getWidth(), 8u);
  EXPECT_EQ(design->getBusTerm(NLName("y"))->getWidth(), 8u);
}

TEST_F(VHDLConstructorTest, DirectBooleanObjectsHidePredefinedLiteralNames) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity boolean_names is port(true : in boolean; false : out boolean; y : out bit); end;
architecture rtl of boolean_names is begin false <= true; y <= '1' when false else '0'; end;
)");
  for (bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("true"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("false"))->getNet(), values, visiting), input);
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), input);
  }
}

TEST_F(VHDLConstructorTest, RuntimeNumericShiftsUseCanonicalModelsAndFullCounts) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity shift_ops is port(a : in unsigned(0 to 4); count : in unsigned(6 downto 0);
  l, r : out unsigned(4 downto 0); s : out signed(4 downto 0)); end;
architecture rtl of shift_ops is begin
  l <= shift_left(a, to_integer(count));
  r <= shift_right(a, to_integer(count));
  s <= shift_right(signed(a), to_integer(count));
end;
)");
  size_t shifts = 0;
  for (auto* instance : design->getInstances()) if (NLDB0::isShift(instance->getModel())) {
    ++shifts;
    EXPECT_EQ(NLDB0::getShiftSignature(instance).width, 5u);
    EXPECT_EQ(NLDB0::getShiftSignature(instance).countWidth, 7u);
  }
  EXPECT_EQ(shifts, 3u);
  for (unsigned input = 0; input < 32; ++input) for (unsigned count = 0; count < 128; ++count) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 5; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(4-i)->getNet()] = (input >> i) & 1;
    for (unsigned i = 0; i < 7; ++i)
      values[design->getBusTerm(NLName("count"))->getBit(i)->getNet()] = (count >> i) & 1;
    const unsigned left = count < 5 ? (input << count) & 31 : 0;
    const unsigned right = count < 5 ? input >> count : 0;
    const unsigned arithmetic = count >= 5 ? (input & 16 ? 31 : 0) :
        ((input >> count) | ((input & 16) && count ? (31u << (5-count)) : 0)) & 31;
    for (unsigned i = 0; i < 5; ++i) for (const auto& [name, expected] :
        std::vector<std::pair<std::string, unsigned>>{{"l", left}, {"r", right}, {"s", arithmetic}})
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(i)->getNet(), values, visiting),
          bool((expected >> i) & 1)) << name << " input=" << input << " count=" << count;
  }
}

TEST_F(VHDLConstructorTest, ClockedRecordVariablesRetainOnlyTheirIncomingValues) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity retained_fields is port(clk, enable : in bit; a : in unsigned(3 downto 0);
  previous_value, current_value : out unsigned(3 downto 0); toggled : out bit); end;
architecture rtl of retained_fields is
  type record_t is record data : unsigned(3 downto 0); flag : bit; end record;
begin
  process(clk)
    variable value : record_t := (data => "0011", flag => '0');
    variable immediate : unsigned(3 downto 0);
  begin
    if rising_edge(clk) then
      previous_value <= value.data;
      if enable = '1' then value.data := a; value.flag := not value.flag; end if;
      immediate := value.data;
      current_value <= immediate;
      toggled <= value.flag;
    end if;
  end process;
end;
)");
  const auto flops = dffBits(design);
  EXPECT_EQ(flops.size(), 14u); // Nine output bits and five retained variable bits.
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  for (auto* instance : design->getInstances()) if (NLDB0::isDFF(instance->getModel())) {
    const auto* initial = instance->getInstParameter(NLName("INIT"));
    if (!initial) continue;
    const auto text = initial->getValue();
    const auto digits = text.substr(text.find('b')+1);
    size_t i = 0;
    for (auto* bit : instance->getModel()->getTerm(NLName("Q"))->getBits())
      state[instance->getInstTerm(bit)->getNet()] = digits.at(i++) == '1';
  }
  unsigned retained = 3;
  bool flag = false;
  for (unsigned cycle = 0; cycle < 24; ++cycle) {
    const bool enable = cycle % 3 != 1;
    const unsigned input = (cycle * 7) & 15;
    auto values = state;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("enable"))->getNet()] = enable;
    for (unsigned i = 0; i < 4; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(i)->getNet()] = (input >> i) & 1;
    auto next = state;
    for (const auto& flop : flops) next[flop.output] = evaluateRTL(flop.data, values, visiting);
    const unsigned previous = retained;
    if (enable) { retained = input; flag = !flag; }
    for (unsigned i = 0; i < 4; ++i) {
      EXPECT_EQ(next.at(design->getBusTerm(NLName("previous_value"))->getBit(i)->getNet()), bool((previous >> i) & 1));
      EXPECT_EQ(next.at(design->getBusTerm(NLName("current_value"))->getBit(i)->getNet()), bool((retained >> i) & 1));
    }
    EXPECT_EQ(next.at(design->getScalarTerm(NLName("toggled"))->getNet()), flag);
    state = std::move(next);
  }
}

TEST_F(VHDLConstructorTest, DiagnosticOnlyClockedProcessesCreateNoStorage) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity checks is port(clk : in bit; a : in std_ulogic_vector(1 downto 0);
  y : out std_ulogic_vector(1 downto 0)); end;
architecture rtl of checks is begin
  process(clk) variable sampled : std_ulogic_vector(1 downto 0);
  begin if rising_edge(clk) then
    sampled := a;
    assert sampled = "00" report "check" severity failure;
  end if; end process;
  y <= a;
end;
)");
  EXPECT_TRUE(dffBits(design).empty());
  for (auto* instance : design->getInstances()) EXPECT_TRUE(NLDB0::isAssign(instance->getModel()));
}

TEST_F(VHDLConstructorTest, RetainedVariableInitialValuesMustHaveAFaithfulRepresentation) {
  for (const auto& [declaration, condition] : std::vector<std::pair<std::string, std::string>>{
      {"variable value : integer range 2 to 7;", "value = 3"},
      {"variable value : std_ulogic := 'Z';", "value = '1'"}}) {
    const auto source = std::string(R"(
library ieee; use ieee.std_logic_1164.all;
entity retained_invalid is port(clk : in bit; y : out bit); end;
architecture rtl of retained_invalid is begin process(clk)
)") + declaration + " begin if rising_edge(clk) then if " + condition + R"( then y <= '1'; else y <= '0'; end if;
end if; end process; end;
)";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, ScalarConditionsApplyVHDL2008ConditionConversion) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity conditions is port(a : in std_ulogic; b : in bit; y : out bit_vector(4 downto 0)); end;
architecture rtl of conditions is
  constant unknown : std_ulogic := 'X';
  function classify(value : std_ulogic) return bit is begin
    if value then return '1'; else return '0'; end if;
  end;
begin
  process(all) begin
    if a then y(0) <= '1'; else y(0) <= '0'; end if;
    if b then y(1) <= '1'; else y(1) <= '0'; end if;
  end process;
  y(2) <= classify(a);
  y(3) <= '1' when ??a else '0';
  y(4) <= '1' when unknown else '0';
end;
)");
  for (bool a : {false, true}) for (bool b : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("a"))->getNet()] = a;
    values[design->getScalarTerm(NLName("b"))->getNet()] = b;
    for (size_t bit = 0; bit < 5; ++bit)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
          bit == 4 ? false : bit == 1 ? b : a);
  }
}

TEST_F(VHDLConstructorTest, RejectsNonScalarOrInapplicableConditionConversions) {
  for (const auto& [type, condition] : std::vector<std::pair<std::string, std::string>>{
      {"bit_vector(0 downto 0)", "a"}, {"boolean", "??a"}}) {
    const auto source = "entity invalid_condition is port(a:in " + type + R"(; y:out bit);end;
architecture rtl of invalid_condition is begin process(all) begin if )" + condition + R"( then
  y<='1';else y<='0';end if;end process;end;)";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, VectorUpdateUsesSharedFrontendIndependentPrimitive) {
  for (const bool ascending : {false, true}) {
    const auto range = ascending ? "0 to 7" : "7 downto 0";
    const auto name = ascending ? "update_ascending" : "update_descending";
    const auto source = std::string(R"(library ieee;
use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity )") + name + " is port(index : in unsigned(2 downto 0); data : in bit; " +
      "base : in bit_vector(" + range + "); y : out bit_vector(" + range + ")); end; " +
      "architecture rtl of " + name + " is begin process(all) begin " +
      "y <= base; y(to_integer(index)) <= data; end process; end;";
    auto* top = VHDLConstructor(library_).construct(source);
    ASSERT_NE(top, nullptr);
    for (auto* instance : top->getInstances()) {
      ASSERT_NE(nullptr, instance->getRTLInfos());
      EXPECT_FALSE(instance->getRTLInfos()->getSourceOrigins().empty());
    }
    SNLDesign* model = nullptr;
    for (auto* instance : top->getInstances()) if (NLDB0::isVectorUpdate(instance->getModel())) {
      EXPECT_EQ(model, nullptr);
      model = instance->getModel();
    }
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(NLDB0::getOrCreateVectorUpdate({8, 1, 34, true}), model);
    // Compare against a separately elaborated SV write with the same layout.
    const auto svName = std::string(name) + "_sv";
    const auto path = std::filesystem::temp_directory_path() / (svName + ".sv");
    std::ofstream file(path);
    file << "// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>\n"
         << "// SPDX-" "License-Identifier: Apache-2.0\nmodule " << svName
         << "(input logic [2:0] index, input logic data, input logic ["
         << (ascending ? "0:7" : "7:0") << "] base, output logic ["
         << (ascending ? "0:7" : "7:0")
         << "] y); always_comb begin y=base; y[index]=data; end endmodule\n";
    file.close();
    SNLSVConstructor(library_).construct(path);
    auto* sv = library_->getSNLDesign(NLName(svName));
    ASSERT_NE(sv, nullptr);
    size_t updates = 0;
    for (auto* instance : sv->getInstances()) if (NLDB0::isVectorUpdate(instance->getModel())) {
      ++updates;
      EXPECT_EQ(model->getLibrary(), instance->getModel()->getLibrary());
    }
    EXPECT_EQ(1u, updates);
    for (unsigned pattern = 0; pattern < 4096; ++pattern) {
      const unsigned index = pattern & 7, data = (pattern >> 3) & 1, base = pattern >> 4;
      const unsigned position = ascending ? 7 - index : index;
      const unsigned expected = (base & ~(1u << position)) | (data << position);
      for (auto* design : {top, sv}) {
        std::unordered_map<SNLBitNet*, bool> values;
        auto* address = design->getBusTerm(NLName("index"));
        for (unsigned bit = 0; bit < 3; ++bit) values[address->getBit(bit)->getNet()] = (index >> bit) & 1;
        values[design->getScalarTerm(NLName("data"))->getNet()] = data;
        auto* input = design->getBusTerm(NLName("base"));
        auto* output = design->getBusTerm(NLName("y"));
        for (unsigned bit = 0; bit < 8; ++bit)
          values[input->getBitAtPosition(7-bit)->getNet()] = (base >> bit) & 1;
        std::unordered_set<SNLBitNet*> visiting;
        for (unsigned bit = 0; bit < 8; ++bit)
          EXPECT_EQ(bool((expected >> bit) & 1),
              evaluateRTL(output->getBitAtPosition(7-bit)->getNet(), values, visiting));
      }
    }
  }
}

TEST_F(VHDLConstructorTest, VectorSelectUsesSharedFrontendIndependentPrimitive) {
  for (const bool ascending : {false, true}) {
    const auto range = ascending ? "0 to 7" : "7 downto 0";
    const auto name = ascending ? "select_ascending" : "select_descending";
    const auto source = std::string(R"(library ieee;
use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity )") + name + " is port(index : in unsigned(2 downto 0); data : in bit_vector(" + range +
      "); y : out bit); end; architecture rtl of " + name +
      " is begin y <= data(to_integer(index)); end;";
    auto* top = VHDLConstructor(library_).construct(source);
    ASSERT_NE(nullptr, top);
    for (auto* instance : top->getInstances()) {
      ASSERT_NE(nullptr, instance->getRTLInfos());
      EXPECT_FALSE(instance->getRTLInfos()->getSourceOrigins().empty());
    }
    size_t selects = 0;
    for (auto* instance : top->getInstances()) selects += NLDB0::isVectorSelect(instance->getModel());
    EXPECT_EQ(1u, selects);
    for (unsigned pattern = 0; pattern < 2048; ++pattern) {
      const unsigned index = pattern & 7, data = pattern >> 3;
      const unsigned position = ascending ? 7-index : index;
      std::unordered_map<SNLBitNet*, bool> values;
      for (unsigned bit = 0; bit < 3; ++bit)
        values[top->getBusTerm(NLName("index"))->getBit(bit)->getNet()] = (index >> bit) & 1;
      for (unsigned bit = 0; bit < 8; ++bit)
        values[top->getBusTerm(NLName("data"))->getBitAtPosition(7-bit)->getNet()] = (data >> bit) & 1;
      std::unordered_set<SNLBitNet*> visiting;
      EXPECT_EQ(bool((data >> position) & 1),
          evaluateRTL(top->getScalarTerm(NLName("y"))->getNet(), values, visiting));
    }
  }
}

TEST_F(VHDLConstructorTest, AnonymousSourceReferences) {
  const std::string source = "-- source coordinates\r\n\r\n"
      "entity leaf is\r\n  port(a : in bit; y : out bit);\r\nend;\r\n"
      "architecture rtl of leaf is begin\r\n  y <= not a;\r\nend;\r\n";
  auto* design = VHDLConstructor(library_).construct(source);
  const auto& declaration = design->getRTLInfos()->getSourceDeclaration();
  ASSERT_TRUE(declaration);
  EXPECT_EQ(NLName("<source>"), declaration->range.file);
  EXPECT_EQ(3, declaration->range.line);
  EXPECT_EQ(5, declaration->range.endLine);
  EXPECT_EQ(4, declaration->range.endColumn);
  const auto& port = design->getTerm(NLName("a"))->getRTLInfos()->getSourceDeclaration();
  ASSERT_TRUE(port);
  EXPECT_EQ(4, port->range.line);
  EXPECT_EQ(8, port->range.column);
  EXPECT_EQ(8, port->range.endColumn);
  for (auto* instance : design->getInstances()) {
    EXPECT_FALSE(instance->getRTLInfos()->getSourceDeclaration());
    ASSERT_EQ(1, instance->getRTLInfos()->getSourceOrigins().size());
    const auto& origin = instance->getRTLInfos()->getSourceOrigins().front();
    EXPECT_EQ(NLName("naja-vhdl"), origin.provider);
    EXPECT_EQ(NLName("vhdl"), origin.language);
    EXPECT_EQ(NLName("rtl"), origin.representation);
    EXPECT_EQ(7, origin.range.line);
    EXPECT_EQ(8, origin.range.column);
    EXPECT_EQ(12, origin.range.endColumn);
  }
}

TEST_F(VHDLConstructorTest, StructuralSpecializationSourceReferences) {
  const std::string source =
      "-- original coordinates survive extraction and generic rewriting\n"
      "entity leaf is generic(n : positive := 123); port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0)); end;\n"
      "architecture rtl of leaf is begin\n  y <= not a;\nend;\n"
      "entity top is port(a : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;\n"
      "architecture rtl of top is begin\n  u: entity work.leaf generic map(2) port map(a, y);\nend;\n";
  auto* design = VHDLConstructor(library_).construct(source, "top");
  auto* instance = design->getInstance(NLName("u"));
  ASSERT_NE(nullptr, instance);
  ASSERT_TRUE(instance->getRTLInfos()->getSourceDeclaration());
  EXPECT_EQ(8, instance->getRTLInfos()->getSourceDeclaration()->range.line);
  auto* model = instance->getModel();
  ASSERT_TRUE(model->getRTLInfos()->getSourceDeclaration());
  EXPECT_EQ(2, model->getRTLInfos()->getSourceDeclaration()->range.line);
  auto* term = model->getBusTerm(NLName("a"));
  ASSERT_TRUE(term->getRTLInfos()->getSourceDeclaration());
  const auto column = source.substr(source.find("entity leaf")).find("a : in") + 1;
  EXPECT_EQ(column, term->getRTLInfos()->getSourceDeclaration()->range.column);
  for (auto* bit : term->getBusBits()) {
    ASSERT_TRUE(bit->getRTLInfos()->getSourceDeclaration());
    EXPECT_EQ(column, bit->getRTLInfos()->getSourceDeclaration()->range.column);
  }
  for (auto* gate : model->getInstances()) {
    ASSERT_FALSE(gate->getRTLInfos()->getSourceOrigins().empty());
    EXPECT_EQ(4, gate->getRTLInfos()->getSourceOrigins().front().range.line);
  }
}

TEST_F(VHDLConstructorTest, RTLMultilineSourceReferencesWithWideColumns) {
  const std::string source = "-- byte coordinates: \xc3\xa9\n"
      "entity top is generic(enabled : boolean := true); port(a, b : in bit; y : out bit); end;\n"
      "architecture rtl of top is begin\n  y <= " + std::string(70000, ' ') + "a and\n    b;\nend;\n";
  auto* design = VHDLConstructor(library_).construct(source);
  size_t gates = 0;
  for (auto* instance : design->getInstances()) {
    if (NLDB0::getGateName(instance->getModel()) != "and") continue;
    ++gates;
    const auto* infos = instance->getRTLInfos();
    ASSERT_NE(nullptr, infos);
    ASSERT_EQ(1, infos->getSourceOrigins().size());
    const auto& origin = infos->getSourceOrigins().front();
    EXPECT_EQ(NLName("<source>"), origin.range.file);
    EXPECT_EQ(4, origin.range.line);
    EXPECT_EQ(70008, origin.range.column);
    EXPECT_EQ(5, origin.range.endLine);
    EXPECT_EQ(5, origin.range.endColumn);
  }
  EXPECT_EQ(1, gates);
}


TEST_F(VHDLConstructorTest, StringGenericAttributesAndOrderingAreStatic) {
  for (const auto* condition : {
      "label'left = 1", "label'low = 1", "label'right = 3", "label'high = 3",
      "label'ascending", "label /= \"xyz\"", "label < \"xyz\"",
      "label <= \"abc\"", "label > \"aaa\"", "label >= \"abc\""}) {
    const std::string expression = condition;
    SCOPED_TRACE(expression);
    auto* design = VHDLConstructor(library_).construct(
        "entity strings is generic(label : string := \"abc\"); port(y : out bit); end; "
        "architecture rtl of strings is constant valid : boolean := " + expression +
        "; begin y <= '1' when valid else '0'; end;", "strings");
    ASSERT_NE(design, nullptr);
    EXPECT_EQ(design->getProperty("VHDLRTLSource")->getString(), "VHDLRTLSource");
    EXPECT_TRUE(assignSource(design->getScalarTerm(NLName("y"))->getNet())->isConstant1());
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RTLValidationDiagnosticsRollBackDesigns) {
  library_->setName(NLName("design"));
  const std::vector<std::pair<std::string, std::string>> cases{
    {"entity bad is generic(v : bit := '0'); end; architecture rtl of bad is begin end;",
      "RTL generics require"},
    {"entity bad is generic(v : string := 7); end; architecture rtl of bad is begin end;",
      "string generic actual must be a static string"},
    {"entity bad is generic(v : string := \"a\"); end; architecture rtl of bad is constant n : integer := v'foo; begin end;",
      "unsupported string generic attribute"},
    {"entity bad is end; architecture rtl of bad is constant n, n : integer := 1; begin end;",
      "duplicate constant"},
    {"use ieee.std_logic_1164.all; entity bad is port(y : out bit); end; architecture rtl of bad is begin y <= '0'; end;",
      "unsupported RTL package visibility"},
    {"entity bad is end; architecture rtl of bad is constant f : integer := 1; function f return integer is begin return 2; end; begin end;",
      "duplicate or overloaded architecture function"},
    {"entity bad is port(y : out bit); end; architecture rtl of bad is type words is array(0 to 1) of bit_vector(1 downto 0); signal y : words; begin end;",
      "duplicate local declaration"},
    {"entity bad is port(clk : out bit; d : in bit; q : out bit); end; architecture rtl of bad is begin process(clk) begin if falling_edge(clk) then q <= d; end if; end process; end;",
      "clock must name a scalar input"},
    {"entity bad is port(clk, rst, d : in bit; q : out bit); end; architecture rtl of bad is begin process(clk, rst) begin if rst = '1' then q <= '0'; elsif falling_edge(clk) then q <= d; end if; end process; end;",
      "falling-edge registers with asynchronous reset"},
    {"entity bad is port(y : out bit); end; architecture rtl of bad is function f(s : string) return bit is begin return '1'; end; begin y <= f(\"a\"); end;",
      "string function arguments require"}
  };
  for (const auto& [source, diagnostic] : cases) {
    SCOPED_TRACE(source);
    try {
      VHDLConstructor(library_).construct(source, "bad");
      FAIL() << "expected diagnostic: " << diagnostic;
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, ProcessLocalArraysAndRecordsPreserveValues) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity local_types is port(a : in bit; y : out bit); end;
architecture rtl of local_types is begin
  process(all)
    type pair_t is array(0 to 1) of bit;
    type packet_t is record data : pair_t; valid : bit; end record;
    variable packet : packet_t;
  begin packet := (data => (a, not a), valid => '1'); y <= packet.data(1) and packet.valid; end process;
end;
)");
  for (const bool input : {false, true}) {
    std::unordered_map<SNLBitNet*, bool> values{{design->getScalarTerm(NLName("a"))->getNet(), input}};
    std::unordered_set<SNLBitNet*> visiting;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting), !input);
  }
}

namespace {
// The source reads four-bit words so mixed logic states also exercise binary
// initialization formatting when a hexadecimal digit cannot represent a word.
std::string textInitializerSource(const std::filesystem::path& path,
    const std::string& declarations, const std::string& statements) {
  return "library ieee; use ieee.std_logic_1164.all; use std.textio.all; "
      "entity initialized is port(clk, en : in std_logic; address : in natural range 0 to 1; "
      "d : in std_logic_vector(3 downto 0); q : out std_logic_vector(3 downto 0)); end; "
      "architecture rtl of initialized is type words is array(0 to 1) of std_logic_vector(3 downto 0); "
      "impure function initialize return words is file source : text is in \"" + path.string() + "\"; "
      "variable row : line; variable word : std_logic_vector(3 downto 0); "
      "variable result : words := (others => (others => '0')); " + declarations +
      " begin " + statements + " return result; end; signal storage : words := initialize; "
      "begin process(clk) begin if rising_edge(clk) then if en = '1' then storage(address) <= d; "
      "end if; q <= storage(address); end if; end process; end;";
}

class VHDLInitializerFile {
 public:
  VHDLInitializerFile() {
    directory = std::filesystem::temp_directory_path() /
        ("naja-vhdl-initializer-" + std::to_string(reinterpret_cast<uintptr_t>(this)));
    std::filesystem::create_directories(directory);
    path = directory / "words.txt";
  }
  ~VHDLInitializerFile() { std::filesystem::remove_all(directory); }
  void write(const std::string& content) { std::ofstream(path) << content; }
  std::filesystem::path directory, path;
};
}

TEST_F(VHDLConstructorTest, TextInitializerDiagnosticsRollBackDesigns) {
  VHDLInitializerFile file;
  file.write("1010\n");
  for (const auto& [statements, diagnostic] : std::vector<std::pair<std::string, std::string>>{
      {"readline(source);", "initializer TextIO currently requires two positional arguments"},
      {"readline(row, row);", "readline requires a text file and a local line variable"},
      {"readline(source, word);", "readline requires a text file and a local line variable"},
      {"readline(source, row); read(row, count);", "initializer read requires a local logic vector variable"},
      {"readline(source, row); read(row, word(0));", "initializer read requires a local logic vector variable"},
      {"if endfile(row) then null; end if;", "endfile requires a visible text file"}}) {
    SCOPED_TRACE(statements);
    try {
      VHDLConstructor(library_).construct(textInitializerSource(file.path,
          "variable count : natural := 0;", statements), "initialized");
      FAIL() << "expected " << diagnostic;
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  file.write("10Q1\n");
  try {
    VHDLConstructor(library_).construct(textInitializerSource(file.path, "",
        "readline(source, row); read(row, word);"), "initialized");
    FAIL() << "expected invalid binary data diagnostic";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("invalid or unsupported binary initialization data"), std::string::npos)
        << error.what();
  }
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, TextInitializerControlFlowAndCacheTrackFileContents) {
  VHDLInitializerFile file;
  file.write("10XZ\r\n0101\n");
  const auto source = textInitializerSource(file.path, "", R"(
    for i in 0 to 1 loop
      if endfile(source) then exit; else readline(source, row); end if;
      case i is
        when 0 => read(row, word); result(i) := word;
        when others => read(row, word); result(i) := word;
      end case;
    end loop;
    if not endfile(source) then return result; end if;
    case 1 is when 0 => null; when others => return result; end case;
  )");
  auto* design = VHDLConstructor(library_).construct(source, "initialized");
  ASSERT_NE(design, nullptr);
  const auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories.begin()->first->getInstParameter(NLName("INIT"))->getValue(), "8'b010110xz");
  EXPECT_EQ(VHDLConstructor(library_).construct(source, "initialized"), design);
  file.write("0000\n1111\n");
  try {
    VHDLConstructor(library_).construct(source, "initialized");
    FAIL() << "expected changed initialization file diagnostic";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("initialization file changed for existing design"), std::string::npos)
        << error.what();
  }
  EXPECT_EQ(library_->getSNLDesign(NLName("initialized")), design);
}

TEST_F(VHDLConstructorTest, ImportedTypesStillEnforcePortAndTextIORestrictions) {
  VHDLInitializerFile file;
  file.write("1010\n");
  auto source = textInitializerSource(file.path, "", "readline(source, row); read(row, word);");
  const auto replaceAll = [](std::string& text, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
      text.replace(pos, from.size(), to);
      pos += to.size();
    }
  };
  replaceAll(source, "library ieee; use ieee.std_logic_1164.all;", "use work.types.all;");
  replaceAll(source, "std_logic_vector(3 downto 0)", "word_t");
  replaceAll(source, "std_logic", "bit");
  source = "library ieee; use ieee.std_logic_1164.all; package types is "
      "subtype word_t is std_logic_vector(3 downto 0); end; " + source;
  for (const auto& [input, top, diagnostic] : std::vector<std::tuple<std::string, std::string, std::string>>{
      {R"(
package types is type grid is array(0 to 1) of bit_vector(1 downto 0); end;
use work.types.all;
entity bad is port(a : in grid); end;
architecture rtl of bad is begin end;
)", "bad", "RTL ports require scalar values or one-dimensional logic vectors"},
      {source, "initialized", "logic-vector TextIO requires ieee.std_logic_1164 visibility"}}) {
    SCOPED_TRACE(input);
    try {
      VHDLConstructor(library_).construct(input, top);
      FAIL() << "expected " << diagnostic;
    } catch (const NLException& error) {
      EXPECT_NE(std::string(error.what()).find(diagnostic), std::string::npos) << error.what();
    }
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, InitializerCaseRejectsRuntimeSelectors) {
  VHDLInitializerFile file;
  file.write("0000\n");
  auto source = textInitializerSource(file.path, "",
      "case selector is when 0 => null; when others => null; end case;");
  source.replace(source.find("function initialize return"), std::string("function initialize return").size(),
      "function initialize(selector : natural) return");
  source.replace(source.find(":= initialize;"), std::string(":= initialize;").size(), ":= initialize(address);");
  try {
    VHDLConstructor(library_).construct(source, "initialized");
    FAIL() << "expected static case selector diagnostic";
  } catch (const NLException& error) {
    EXPECT_NE(std::string(error.what()).find("initializer case selector must be static"), std::string::npos)
        << error.what();
  }
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}
