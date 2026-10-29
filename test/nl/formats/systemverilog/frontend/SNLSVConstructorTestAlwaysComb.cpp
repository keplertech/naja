// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <vector>

#include "NLDB0.h"
#include "NLDB.h"
#include "NLUniverse.h"
#include "NLLibrary.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLNet.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

#include "SNLSVConstructor.h"
#include "SNLSVConstructorException.h"

using namespace naja::NL;

#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

void expectUnsupportedConstruct(
  SNLSVConstructor& constructor,
  const std::filesystem::path& svPath,
  std::initializer_list<const char*> expectedSubstrings) {
  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    for (const auto* expected : expectedSubstrings) {
      EXPECT_NE(std::string::npos, reason.find(expected)) << reason;
    }
  }
}

SNLNet* getSingleAssignInputDriving(SNLBitNet* drivenNet) {
  if (!drivenNet) {
    ADD_FAILURE() << "Missing driven net";
    return nullptr;
  }

  std::vector<SNLInstance*> assignDrivers;
  for (auto* instTerm : drivenNet->getInstTerms()) {
    if (!instTerm || instTerm->getBitTerm() != NLDB0::getAssignOutput()) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    if (instance && NLDB0::isAssign(instance->getModel())) {
      assignDrivers.push_back(instance);
    }
  }

  EXPECT_EQ(1u, assignDrivers.size()) << drivenNet->getString();
  if (assignDrivers.size() != 1) {
    return nullptr;
  }

  auto* inputTerm = assignDrivers.front()->getInstTerm(NLDB0::getAssignInput());
  if (!inputTerm) {
    ADD_FAILURE() << "Assign driver has no input term";
    return nullptr;
  }
  return inputTerm->getNet();
}

size_t countOutputInstTermDrivers(SNLBitNet* drivenNet) {
  if (!drivenNet) {
    ADD_FAILURE() << "Missing driven net";
    return 0;
  }

  size_t drivers = 0;
  for (auto* instTerm : drivenNet->getInstTerms()) {
    if (instTerm &&
        instTerm->getDirection() == SNLTerm::Direction::Output) {
      ++drivers;
    }
  }
  return drivers;
}

size_t countMux2InputUses(SNLBitNet* net) {
  if (!net) {
    ADD_FAILURE() << "Missing net";
    return 0;
  }

  size_t uses = 0;
  for (auto* instTerm : net->getInstTerms()) {
    if (!instTerm || instTerm->getDirection() != SNLTerm::Direction::Input) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    if (instance && NLDB0::isMux2(instance->getModel())) {
      ++uses;
    }
  }
  return uses;
}

size_t countInputInstTermUses(SNLBitNet* net) {
  if (!net) {
    ADD_FAILURE() << "Missing net";
    return 0;
  }

  size_t uses = 0;
  for (auto* instTerm : net->getInstTerms()) {
    if (instTerm &&
        instTerm->getDirection() == SNLTerm::Direction::Input) {
      ++uses;
    }
  }
  return uses;
}

std::filesystem::path createTestDirectory(const char* name) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / name;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  return outPath;
}

}

class SNLSVConstructorTestAlwaysComb: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestAlwaysComb,
       parseAlwaysCombIndependentNonBlockingAssignmentsSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath =
      createTestDirectory("always_comb_independent_nonblocking_assignments_supported");

  const auto svPath =
      outPath / "always_comb_independent_nonblocking_assignments_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module always_comb_independent_nonblocking_assignments_supported(
  input  logic       sel0_i,
  input  logic       sel1_i,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  output logic [7:0] y0_o,
  output logic [7:0] y1_o,
  output logic [1:0] flags_o
);
  always_comb begin
    if (sel0_i)
      y0_o <= a_i;
    else
      y0_o <= b_i;
    case (sel1_i)
      1'b0: y1_o <= b_i;
      default: y1_o <= a_i;
    endcase
    for (int i = 0; i < 2; i++)
      flags_o[i] = a_i[i];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(
    NLName("always_comb_independent_nonblocking_assignments_supported"));
  ASSERT_NE(nullptr, top);
  EXPECT_NE(nullptr, top->getNet(NLName("y0_o")));
  EXPECT_NE(nullptr, top->getNet(NLName("y1_o")));
}

TEST_F(SNLSVConstructorTestAlwaysComb,
       parseAlwaysCombNonBlockingReadAfterWriteUnsupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_nonblocking_read_after_write_unsupported");
  const auto svPath = outPath / "always_comb_nonblocking_read_after_write_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module always_comb_nonblocking_read_after_write_unsupported(
  input logic a_i,
  output logic q_o,
  output logic r_o
);
  always_comb begin
    q_o <= a_i;
    r_o <= q_o;
  end
endmodule
)";
  svFile.close();
  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"scheduling-sensitive", "reads 'q_o' after an earlier non-blocking assignment"});
}

TEST_F(SNLSVConstructorTestAlwaysComb,
       parseAlwaysCombMixedBlockingNonBlockingSameTargetUnsupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_mixed_blocking_nonblocking_same_target_unsupported");
  const auto svPath =
    outPath / "always_comb_mixed_blocking_nonblocking_same_target_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module always_comb_mixed_blocking_nonblocking_same_target_unsupported(
  input logic a_i,
  input logic b_i,
  output logic q_o
);
  always_comb begin
    q_o = a_i;
    q_o <= b_i;
  end
endmodule
)";
  svFile.close();
  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"mixed blocking and non-blocking assignments", "same control-flow path"});
}

TEST_F(SNLSVConstructorTestAlwaysComb,
       parseAlwaysCombSchedulingSafetyBranchDiagnosticsCovered) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_scheduling_safety_branch_diagnostics_covered");
  const auto svPath =
    outPath / "always_comb_scheduling_safety_branch_diagnostics_covered.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module always_comb_scheduling_safety_branch_diagnostics_covered(
  input logic a_i, input logic sel_i,
  output logic q0_o, output logic r0_o,
  output logic q1_o, output logic r1_o,
  output logic q2_o, output logic r2_o,
  output logic q3_o, output logic r3_o,
  output logic q4_o, output logic r4_o,
  output logic q5_o, output logic r5_o,
  output logic q6_o, output logic r6_o,
  output logic idx_o, output logic indexed_o
);
  logic [1:0] indexed;
  always_comb begin
    q0_o <= a_i;
    if (sel_i) r0_o <= q0_o; else r0_o <= a_i;
  end
  always_comb begin
    q1_o <= a_i;
    if (sel_i) r1_o <= a_i; else r1_o <= q1_o;
  end
  always_comb begin
    q2_o <= a_i;
    case (sel_i)
      1'b0: r2_o <= q2_o;
      default: r2_o <= a_i;
    endcase
  end
  always_comb begin
    q3_o <= a_i;
    case (sel_i)
      1'b0: r3_o <= a_i;
      default: r3_o <= q3_o;
    endcase
  end
  always_comb begin
    q4_o = a_i;
    for (int i = 0; i < 2; i++)
      r4_o <= q4_o;
  end
  always_comb begin
    for (int i = 0; i < 1; i++) begin
      q5_o = a_i;
      q5_o <= a_i;
    end
    r5_o = q5_o;
  end
  always_comb begin
    {q6_o, r6_o} <= {a_i, sel_i};
  end
  always_comb begin
    idx_o <= sel_i;
    indexed = '0;
    indexed[idx_o] = a_i;
    indexed_o = indexed[0];
  end
endmodule
)";
  svFile.close();
  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"scheduling-sensitive"});
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConstantFalseShortCircuitSkipsUnsupportedBranch) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_constant_false_short_circuit_skips_unsupported_branch");

  const auto svPath =
    outPath / "always_comb_constant_false_short_circuit_skips_unsupported_branch.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_false_short_circuit_skips_unsupported_branch(
  input  logic a_i,
  output logic y_o
);
  typedef struct packed {
    bit RVH;
  } cfg_t;
  typedef struct packed {
    logic [1:0] ppn;
    logic       g;
  } pte_t;
  localparam cfg_t Cfg = '{RVH: 1'b0};
  pte_t pte;
  logic [2:0] content;

  always_comb begin
    y_o = a_i;
    content = 3'b0;
    if (a_i && Cfg.RVH) begin
      content = (pte | (a_i << 1));
      forever begin
        y_o = 1'b1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_constant_false_short_circuit_skips_unsupported_branch"));
  ASSERT_NE(top, nullptr);
  auto* a = dynamic_cast<SNLBitNet*>(top->getNet(NLName("a_i")));
  auto* y = dynamic_cast<SNLBitNet*>(top->getNet(NLName("y_o")));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(a, getSingleAssignInputDriving(y));
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConstantLogicalOperandsSelectExpectedBranches) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_constant_logical_operands_select_expected_branches");

  const auto svPath =
    outPath / "always_comb_constant_logical_operands_select_expected_branches.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_logical_operands_select_expected_branches(
  input  logic a_i,
  input  logic b_i,
  output logic and_o,
  output logic or_o
);
  always_comb begin
    and_o = b_i;
    or_o = b_i;
    for (int i = 0; i < 2; i++) begin
      if ((i == 1) && (a_i || 1'b1)) begin
        and_o = a_i;
      end
      if ((i == 0) || (a_i && 1'b0)) begin
        or_o = a_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_constant_logical_operands_select_expected_branches"));
  ASSERT_NE(top, nullptr);
  auto* a = dynamic_cast<SNLBitNet*>(top->getNet(NLName("a_i")));
  auto* andOutput = dynamic_cast<SNLBitNet*>(top->getNet(NLName("and_o")));
  auto* orOutput = dynamic_cast<SNLBitNet*>(top->getNet(NLName("or_o")));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(andOutput, nullptr);
  ASSERT_NE(orOutput, nullptr);
  EXPECT_EQ(a, getSingleAssignInputDriving(andOutput));
  EXPECT_EQ(a, getSingleAssignInputDriving(orOutput));
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombInvalidDeadBranchStatementUnwrappedAndSkipped) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_invalid_dead_branch_statement_unwrapped_and_skipped");

  const auto svPath =
    outPath / "always_comb_invalid_dead_branch_statement_unwrapped_and_skipped.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_invalid_dead_branch_statement_unwrapped_and_skipped(
  input  logic a_i,
  output logic y_o
);
  localparam bit ENABLE_BAD_BRANCH = 1'b0;
  logic [3:0] data;

  always_comb begin
    y_o = a_i;
    if (ENABLE_BAD_BRANCH) begin
      y_o = data[0:1];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_invalid_dead_branch_statement_unwrapped_and_skipped"));
  ASSERT_NE(top, nullptr);
  auto* a = dynamic_cast<SNLBitNet*>(top->getNet(NLName("a_i")));
  auto* y = dynamic_cast<SNLBitNet*>(top->getNet(NLName("y_o")));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(a, getSingleAssignInputDriving(y));
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConstantSelectedBranchCollectionFailureReportsContext) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_constant_selected_branch_collection_failure_reports_context");

  const auto svPath =
    outPath / "always_comb_constant_selected_branch_collection_failure_reports_context.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_selected_branch_collection_failure_reports_context(
  input  logic hold_i,
  input  logic d_i,
  output logic y_o
);
  localparam bit USE_UNSUPPORTED_BRANCH = 1'b1;

  always_comb begin
    y_o = 1'b0;
    if (USE_UNSUPPORTED_BRANCH) begin
      while (hold_i) begin
        y_o = d_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments",
     "constant-selected branch"});
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConstantBitThenWholeVectorOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_constant_bit_then_whole_vector_override");

  const auto svPath =
    outPath / "always_comb_constant_bit_then_whole_vector_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_bit_then_whole_vector_override(
  input  logic [3:0] a,
  output logic [3:0] y
);
  always_comb begin
    y[1] = 1'b1;
    y = a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_constant_bit_then_whole_vector_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(4, a->getWidth());
  ASSERT_EQ(4, y->getWidth());

  for (NLID::Bit bit = 0; bit < 4; ++bit) {
    ASSERT_NE(a->getBit(bit), nullptr);
    ASSERT_NE(y->getBit(bit), nullptr);
    EXPECT_EQ(a->getBit(bit), getSingleAssignInputDriving(y->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombWholeVectorThenConstantBitOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_whole_vector_then_constant_bit_override");

  const auto svPath =
    outPath / "always_comb_whole_vector_then_constant_bit_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_whole_vector_then_constant_bit_override(
  input  logic [3:0] a,
  output logic [3:0] y
);
  always_comb begin
    y = a;
    y[1] = 1'b1;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_whole_vector_then_constant_bit_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(4, a->getWidth());
  ASSERT_EQ(4, y->getWidth());

  for (NLID::Bit bit : {0, 2, 3}) {
    ASSERT_NE(a->getBit(bit), nullptr);
    ASSERT_NE(y->getBit(bit), nullptr);
    EXPECT_EQ(a->getBit(bit), getSingleAssignInputDriving(y->getBit(bit)))
      << "bit " << bit;
  }

  ASSERT_NE(a->getBit(1), nullptr);
  ASSERT_NE(y->getBit(1), nullptr);
  auto* y1Driver = getSingleAssignInputDriving(y->getBit(1));
  ASSERT_NE(y1Driver, nullptr);
  EXPECT_NE(a->getBit(1), y1Driver);
  EXPECT_TRUE(y1Driver->isAssign1());
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombOverlappingConstantRangesLastWriteWins) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_overlapping_constant_ranges_last_write_wins");

  const auto svPath =
    outPath / "always_comb_overlapping_constant_ranges_last_write_wins.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_overlapping_constant_ranges_last_write_wins(
  input  logic [2:0] a,
  input  logic [2:0] b,
  output logic [3:0] y
);
  always_comb begin
    y[3:1] = a;
    y[2:0] = b;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_overlapping_constant_ranges_last_write_wins"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* b = top->getBusNet(NLName("b"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(y, nullptr);

  EXPECT_EQ(b->getBit(0), getSingleAssignInputDriving(y->getBit(0)));
  EXPECT_EQ(b->getBit(1), getSingleAssignInputDriving(y->getBit(1)));
  EXPECT_EQ(b->getBit(2), getSingleAssignInputDriving(y->getBit(2)));
  EXPECT_EQ(a->getBit(2), getSingleAssignInputDriving(y->getBit(3)));
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombNestedPackedElementBasesThenWholeVectorOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_nested_packed_element_bases_then_whole_vector_override");

  const auto svPath =
    outPath / "always_comb_nested_packed_element_bases_then_whole_vector_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_nested_packed_element_bases_then_whole_vector_override(
  input  logic [1:0][3:0] a,
  output logic [1:0][3:0] y
);
  always_comb begin
    y[1][0] = 1'b1;
    y[0][2] = 1'b0;
    y = a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_nested_packed_element_bases_then_whole_vector_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(8, a->getWidth());
  ASSERT_EQ(8, y->getWidth());

  for (NLID::Bit bit = 0; bit < 8; ++bit) {
    ASSERT_NE(a->getBit(bit), nullptr);
    ASSERT_NE(y->getBit(bit), nullptr);
    EXPECT_EQ(a->getBit(bit), getSingleAssignInputDriving(y->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructFieldCompoundAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_field_compound_assignment_supported");

  const auto svPath =
    outPath / "always_comb_packed_struct_field_compound_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_packed_struct_field_compound_assignment_supported(
  input  logic [4:0] seed_i,
  input  logic [3:0] data_mask_i,
  input  logic       valid_mask_i,
  output logic [4:0] y_o
);
  typedef struct packed {
    logic [3:0] data;
    logic       valid;
  } pkt_s;

  pkt_s pkt_n;

  always_comb begin
    pkt_n = seed_i;
    pkt_n.data |= data_mask_i;
    pkt_n.valid &= valid_mask_i;
    y_o = pkt_n;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_field_compound_assignment_supported"));
  ASSERT_NE(top, nullptr);
  auto* pkt = top->getBusNet(NLName("pkt_n"));
  ASSERT_NE(pkt, nullptr);
  ASSERT_EQ(5, pkt->getWidth());
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructWholeThenFieldBranchOverridesSingleDriver) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_whole_then_field_branch_overrides_single_driver");

  const auto svPath =
    outPath / "always_comb_packed_struct_whole_then_field_branch_overrides_single_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_packed_struct_whole_then_field_branch_overrides_single_driver(
  input  logic       sel_i,
  input  logic [2:0] miss_i,
  input  logic [1:0] dirty_i,
  input  logic       lru_i,
  output logic [2:0] y_o
);
  typedef struct packed {
    logic [1:0] dirty;
    logic       lru;
  } stat_s;

  stat_s s;

  always_comb begin
    if (sel_i) begin
      s = miss_i;
    end else begin
      s.dirty = dirty_i;
      s.lru = lru_i;
    end
    y_o = s;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_whole_then_field_branch_overrides_single_driver"));
  ASSERT_NE(top, nullptr);
  auto* s = top->getBusNet(NLName("s"));
  ASSERT_NE(s, nullptr);
  ASSERT_EQ(3, s->getWidth());

  for (NLID::Bit bit = 0; bit < 3; ++bit) {
    ASSERT_NE(s->getBit(bit), nullptr);
    std::vector<SNLInstTerm*> drivers;
    for (auto* term : s->getBit(bit)->getInstTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Output) {
        drivers.push_back(term);
      }
    }
    ASSERT_EQ(1u, drivers.size()) << "bit " << bit;
    auto* mux = drivers.front()->getInstance();
    // Shared pure expressions may drive the named state through an alias.
    if (NLDB0::isAssign(mux->getModel())) {
      auto* input = mux->getInstTerm(NLDB0::getAssignInput())->getNet();
      for (auto* term : input->getInstTerms()) {
        if (term->getDirection() == SNLTerm::Direction::Output) mux = term->getInstance();
      }
    }
    ASSERT_TRUE(NLDB0::isMux2(mux->getModel()));
    EXPECT_EQ(top->getBusNet(NLName("miss_i"))->getBit(bit),
      mux->getInstTerm(NLDB0::getMux2InputB(mux->getModel())->getBit(bit))->getNet());
    EXPECT_EQ(bit == 0 ? static_cast<SNLBitNet*>(top->getScalarNet(NLName("lru_i"))) :
      static_cast<SNLBitNet*>(top->getBusNet(NLName("dirty_i"))->getBit(bit - 1)),
      mux->getInstTerm(NLDB0::getMux2InputA(mux->getModel())->getBit(bit))->getNet());
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombNestedPackedStructPayloadThenSubfieldsSingleDriver) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_nested_packed_struct_payload_then_subfields_single_driver");

  const auto svPath =
    outPath / "always_comb_nested_packed_struct_payload_then_subfields_single_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_nested_packed_struct_payload_then_subfields_single_driver(
  input  logic [3:0] msg_i,
  input  logic [1:0] src_i,
  input  logic       dst_i,
  input  logic [2:0] did_i,
  output logic [9:0] y_o
);
  typedef struct packed {
    logic [1:0] src_id;
    logic       dst_id;
    logic [2:0] src_did;
  } payload_s;

  typedef struct packed {
    logic [3:0] msg_type;
    payload_s   payload;
  } header_s;

  header_s h;

  always_comb begin
    h.msg_type = msg_i;
    h.payload = '0;
    h.payload.src_id = src_i;
    h.payload.dst_id = dst_i;
    h.payload.src_did = did_i;
    y_o = h;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_nested_packed_struct_payload_then_subfields_single_driver"));
  ASSERT_NE(top, nullptr);
  auto* h = top->getBusNet(NLName("h"));
  ASSERT_NE(h, nullptr);
  ASSERT_EQ(10, h->getWidth());

  for (NLID::Bit bit = 0; bit < 10; ++bit) {
    ASSERT_NE(h->getBit(bit), nullptr);
    EXPECT_NE(nullptr, getSingleAssignInputDriving(h->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructConditionLeavesExternalFieldDriverSingleDriver) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_condition_leaves_external_field_driver_single_driver");

  const auto svPath =
    outPath / "always_comb_packed_struct_condition_leaves_external_field_driver_single_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module ready_source(output logic ready_o);
  assign ready_o = 1'b1;
endmodule

module always_comb_packed_struct_condition_leaves_external_field_driver_single_driver(
  input  logic       sel_i,
  input  logic [1:0] data_i,
  output logic [3:0] y_o
);
  typedef struct packed {
    logic       v;
    logic       ready;
    logic [1:0] data;
  } link_s;

  link_s link;

  ready_source ready_i(.ready_o(link.ready));

  always_comb begin
    link.v = 1'b0;
    link.data = data_i;
    if (sel_i) begin
      link.v = 1'b1;
    end else begin
      link.data = 2'b10;
    end
    y_o = link;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_condition_leaves_external_field_driver_single_driver"));
  ASSERT_NE(top, nullptr);
  auto* link = top->getBusNet(NLName("link"));
  ASSERT_NE(link, nullptr);
  ASSERT_EQ(4, link->getWidth());
  ASSERT_NE(link->getBit(2), nullptr);
  EXPECT_EQ(1u, countOutputInstTermDrivers(link->getBit(2)));
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombCaseAssignmentFunctionCallSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_case_assignment_function_call_supported");

  const auto svPath = outPath / "always_comb_case_assignment_function_call_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_assignment_function_call_supported(
  input  logic [3:0] op_i,
  output logic [1:0] data_size_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: extract_transfer_size = 2'b00;
      4'h1: extract_transfer_size = 2'b01;
      default: extract_transfer_size = 2'b11;
    endcase
  endfunction

  always_comb begin
    data_size_o = extract_transfer_size(op_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_assignment_function_call_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_size_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseExternalCva6FunctionCallRhsPatternsSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "external_cva6_function_call_rhs_patterns_supported");

  const auto svPath =
    outPath / "external_cva6_function_call_rhs_patterns_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module external_cva6_function_call_rhs_patterns_supported(
  input  logic       clk_i,
  input  logic [3:0] bv_i,
  input  logic [3:0] lhs_i,
  input  logic [3:0] rhs_i,
  input  logic [7:0] data_i,
  input  logic [1:0] size_i,
  input  logic       sel_i,
  output logic [1:0] idx_o,
  output logic       eq_o,
  output logic [7:0] data_o,
  output logic [3:0] cs_o,
  output logic [5:0] meta_o
);
  typedef logic [1:0] idx_t;
  typedef struct packed {
    logic [2:0] addr;
    logic [2:0] size;
  } meta_t;

  meta_t meta;

  function automatic idx_t bv_to_index(input logic [3:0] bv);
    for (int i = 0; i < 4; i++) begin
      if (bv[i]) return idx_t'(i);
    end
    return idx_t'(0);
  endfunction

  function automatic logic set_equal(input logic [3:0] lhs, input logic [3:0] rhs);
    if (2 > 1) return lhs[1:0] == rhs[1:0];
    else return 1'b1;
  endfunction

  function automatic logic [7:0] prepare_data(
      input logic [7:0] data,
      input logic [1:0] size,
      input logic       sel);
    if (size == 2'd3) return data;
    else if (sel) return {{4{data[7]}}, data[7:4]};
    else return {4'b0, data[3:0]};
  endfunction

  function automatic logic [2:0] mem_size(input int unsigned bytes);
    if (bytes <= 1) return 3'd0;
    else if (bytes <= 2) return 3'd1;
    else if (bytes <= 4) return 3'd2;
    else begin
      assert (1'b1);
      return 3'd3;
    end
  endfunction

  function automatic logic [3:0] compute_cs(
      input logic [1:0] size,
      input logic [1:0] word);
    logic [3:0] ret;
    logic [1:0] off;
    case (size)
      2'd0: ret = 4'b0001;
      2'd1: ret = 4'b0011;
      default: ret = 4'b1111;
    endcase
    off = word[0 +: 2];
    return ret << off;
  endfunction

  assign idx_o = bv_to_index(bv_i);
  assign eq_o = set_equal(lhs_i, rhs_i);
  assign data_o = prepare_data(data_i, size_i, sel_i);
  assign meta = '{addr: lhs_i[2:0], size: mem_size(4)};
  assign meta_o = meta;

  always_comb begin
    cs_o = compute_cs(size_i, bv_i[1:0]);
  end

  property prop_core_req_size_max;
    @(posedge clk_i) eq_o |-> eq_o;
  endproperty
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("external_cva6_function_call_rhs_patterns_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("idx_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("meta_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseExternalCva6OutputFunctionCallDynamicMemberSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "external_cva6_output_function_call_dynamic_member_supported");

  const auto svPath =
    outPath / "external_cva6_output_function_call_dynamic_member_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module external_cva6_output_function_call_dynamic_member_supported(
  input  logic       ptr_i,
  input  logic       write_i,
  input  logic       init_i,
  input  logic [31:0] write_data_flat_i,
  input  logic [3:0]  write_be_flat_i,
  output logic [35:0] flat_o
);
  localparam int unsigned WBUF_DATA_NWORDS = 2;
  typedef logic [15:0] wbuf_data_t;
  typedef logic [1:0]  wbuf_be_t;
  typedef wbuf_data_t [WBUF_DATA_NWORDS-1:0] wbuf_data_buf_t;
  typedef wbuf_be_t   [WBUF_DATA_NWORDS-1:0] wbuf_be_buf_t;

  typedef struct packed {
    wbuf_data_buf_t data;
    wbuf_be_buf_t   be;
  } wbuf_data_entry_t;

  wbuf_data_entry_t [1:0] entries_q;
  wbuf_data_entry_t [1:0] entries_d;
  wbuf_data_buf_t write_data;
  wbuf_be_buf_t write_be;

  assign write_data[0] = write_data_flat_i[15:0];
  assign write_data[1] = write_data_flat_i[31:16];
  assign write_be[0] = write_be_flat_i[1:0];
  assign write_be[1] = write_be_flat_i[3:2];
  assign flat_o = entries_d[0];

  function automatic void wbuf_data_write(
      output wbuf_data_buf_t wbuf_ret_data,
      output wbuf_be_buf_t   wbuf_ret_be,
      input  wbuf_data_buf_t wbuf_old_data,
      input  wbuf_be_buf_t   wbuf_old_be,
      input  wbuf_data_buf_t wbuf_new_data,
      input  wbuf_be_buf_t   wbuf_new_be);
    for (int unsigned w = 0; w < WBUF_DATA_NWORDS; w++) begin
      wbuf_ret_data[w] = wbuf_new_be[w][0] ? wbuf_new_data[w] : wbuf_old_data[w];
      wbuf_ret_be[w] = wbuf_old_be[w] | wbuf_new_be[w];
    end
  endfunction

  always_comb begin
    automatic wbuf_be_buf_t buf_be;

    entries_d = entries_q;
    buf_be = init_i ? '0 : entries_q[ptr_i].be;

    if (write_i) begin
      wbuf_data_write(
          entries_d[ptr_i].data,
          entries_d[ptr_i].be,
          entries_q[ptr_i].data,
          buf_be,
          write_data,
          write_be);
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("external_cva6_output_function_call_dynamic_member_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flat_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombRHSCurrentLHSUsesReplayValueNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_rhs_current_lhs_uses_replay_value_no_feedback");

  const auto svPath =
    outPath / "always_comb_rhs_current_lhs_uses_replay_value_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_current_lhs_uses_replay_value_no_feedback(
  input  logic [3:0] a_i,
  input  logic       sel_i,
  input  logic       kill_i,
  output logic [3:0] y_o
);
  always_comb begin
    y_o = a_i;
    if (sel_i) begin
      y_o = kill_i ? 4'b0000 : y_o;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_rhs_current_lhs_uses_replay_value_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* y = top->getBusNet(NLName("y_o"));
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(4, y->getWidth());
  for (auto* bit : y->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(0u, countMux2InputUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConditionCurrentLHSUsesReplayValueNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_condition_current_lhs_uses_replay_value_no_feedback");

  const auto svPath =
    outPath / "always_comb_condition_current_lhs_uses_replay_value_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_current_lhs_uses_replay_value_no_feedback(
  input  logic [4:0] q_i,
  input  logic [4:0] init_i,
  input  logic       op_i,
  output logic [4:0] r_o,
  output logic       y_o
);
  always_comb begin
    r_o = q_i;
    y_o = 1'b0;
    if (op_i) begin
      r_o = init_i;
      y_o = 1'b1;
      if (r_o == 5'd4) begin
        r_o -= 5'd1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_condition_current_lhs_uses_replay_value_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* r = top->getBusNet(NLName("r_o"));
  ASSERT_NE(r, nullptr);
  ASSERT_EQ(5, r->getWidth());
  for (auto* bit : r->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(0u, countInputInstTermUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombTemporaryUsesCurrentLHSReplayValueNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_temporary_uses_current_lhs_replay_value_no_feedback");

  const auto svPath =
    outPath / "always_comb_temporary_uses_current_lhs_replay_value_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_temporary_uses_current_lhs_replay_value_no_feedback(
  input  logic [3:0] ext_i,
  input  logic       sel_i,
  output logic [3:0] result_o
);
  logic [3:0] rev;

  always_comb begin
    result_o = ext_i;
    for (int i = 0; i < 4; i++) begin
      rev[i] = result_o[3-i];
    end
    result_o = sel_i ? rev : result_o;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_temporary_uses_current_lhs_replay_value_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(0u, countInputInstTermUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombTemporarySelectionReplaySupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_temporary_selection_replay_supported");

  const auto svPath =
    outPath / "always_comb_temporary_selection_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_temporary_selection_replay_supported(
  input  logic [3:0] seed_i,
  input  logic [1:0] lo_i,
  input  logic       hi_i,
  output logic [3:0] result_o
);
  logic [3:0] tmp;

  always_comb begin
    tmp = seed_i;
    tmp[1:0] = lo_i;
    tmp[3] = hi_i;
    result_o = {tmp[0], tmp[3:1]};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_temporary_selection_replay_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombCaseTemporaryReplayDependenciesSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_case_temporary_replay_dependencies_supported");

  const auto svPath =
    outPath / "always_comb_case_temporary_replay_dependencies_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_temporary_replay_dependencies_supported(
  input  logic [3:0] seed_i,
  input  logic [1:0] sel_i,
  input  logic       a_i,
  input  logic       b_i,
  output logic [3:0] result_o
);
  logic [3:0] tmp;

  always_comb begin
    tmp = seed_i;
    unique case (sel_i)
      2'd0: tmp[0] = a_i;
      2'd1: tmp[1] = b_i;
      default: tmp[2] = a_i ^ b_i;
    endcase
    result_o = tmp;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_case_temporary_replay_dependencies_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructCaseSubfieldAssignmentsNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_case_subfield_assignments_no_feedback");

  const auto svPath =
    outPath / "always_comb_packed_struct_case_subfield_assignments_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_packed_struct_case_subfield_assignments_no_feedback(
  input  logic [1:0] sel_i,
  input  logic [3:0] imm_i,
  input  logic [3:0] rs3_i,
  output logic [4:0] instruction_o
);
  typedef struct packed {
    logic [3:0] result;
    logic       use_imm;
  } decoded_s;

  decoded_s decoded;

  always_comb begin
    case (sel_i)
      2'd0: begin
        decoded.result  = imm_i;
        decoded.use_imm = 1'b1;
      end
      2'd1: begin
        decoded.result  = rs3_i;
        decoded.use_imm = 1'b0;
      end
      default: begin
        decoded.result  = 4'b0000;
        decoded.use_imm = 1'b0;
      end
    endcase
    instruction_o = decoded;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_case_subfield_assignments_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* decoded = top->getBusNet(NLName("decoded"));
  ASSERT_NE(decoded, nullptr);
  ASSERT_EQ(5, decoded->getWidth());
  for (auto* bit : decoded->getBits()) {
    ASSERT_NE(bit, nullptr);
    // The duplicate replay of the internal temporary has no consumers.
    EXPECT_EQ(0u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(1u, countOutputInstTermDrivers(
      top->getBusTerm(NLName("instruction_o"))->getBit(static_cast<SNLBusNetBit*>(bit)->getBit())->getNet()));
    EXPECT_EQ(0u, countInputInstTermUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructCaseSubfieldClearUnderEmptyIfBranch) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_case_subfield_clear_under_empty_if_branch");

  const auto svPath =
    outPath / "always_comb_packed_struct_case_subfield_clear_under_empty_if_branch.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_packed_struct_case_subfield_clear_under_empty_if_branch(
  input  logic       save_i,
  input  logic       debug_i,
  input  logic [1:0] priv_i,
  input  logic [2:0] status_q_i,
  output logic [2:0] status_n_o
);
  typedef struct packed {
    logic mie;
    logic mpie;
    logic mpp;
  } status_s;

  status_s status_q;
  status_s status_n;

  always_comb begin
    status_q = status_q_i;
    status_n = status_q;
    unique case (1'b1)
      save_i: begin
        unique case (priv_i)
          2'b11: begin
            if (debug_i) begin
            end else begin
              status_n.mpie = status_q.mie;
              status_n.mie  = 1'b0;
              status_n.mpp  = 1'b1;
            end
          end
          default: ;
        endcase
      end
      default: ;
    endcase
    status_n_o = status_n;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(NLName(
    "always_comb_packed_struct_case_subfield_clear_under_empty_if_branch"));
  ASSERT_NE(top, nullptr);
  auto* statusN = top->getBusNet(NLName("status_n"));
  auto* debug = dynamic_cast<SNLBitNet*>(top->getNet(NLName("debug_i")));
  ASSERT_NE(statusN, nullptr);
  ASSERT_NE(debug, nullptr);
  ASSERT_EQ(3, statusN->getWidth());

  ASSERT_NE(statusN->getBit(2), nullptr);
  EXPECT_EQ(0u, countOutputInstTermDrivers(statusN->getBit(2)));
  EXPECT_EQ(0u, countInputInstTermUses(statusN->getBit(2)));
  EXPECT_EQ(1u, countOutputInstTermDrivers(
    top->getBusTerm(NLName("status_n_o"))->getBit(2)->getNet()));
  EXPECT_GT(countMux2InputUses(debug), 0u);
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructOutputFieldCaseNoOverlappingDrivers) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_output_field_case_no_overlapping_drivers");

  const auto svPath =
    outPath / "always_comb_packed_struct_output_field_case_no_overlapping_drivers.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic       other_op;
  logic       atomic_op;
  logic [3:0] amo_subop;
} decode_s;

module always_comb_packed_struct_output_field_case_no_overlapping_drivers(
  input logic [1:0] opcode_i,
  output decode_s decode_o
);
  always_comb begin
    case (opcode_i)
      2'd0: decode_o.other_op = 1'b1;
      default: decode_o.other_op = 1'b0;
    endcase
  end

  always_comb begin
    decode_o.atomic_op = 1'b1;

    unique case (opcode_i)
      2'd0: decode_o.amo_subop = 4'd1;
      2'd1: decode_o.amo_subop = 4'd2;
      default: begin
        decode_o.atomic_op = 1'b0;
        decode_o.amo_subop = 4'd0;
      end
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_output_field_case_no_overlapping_drivers"));
  ASSERT_NE(top, nullptr);
  auto* decode = top->getBusNet(NLName("decode_o"));
  ASSERT_NE(decode, nullptr);
  ASSERT_EQ(6, decode->getWidth());
  ASSERT_NE(nullptr, decode->getBit(5));
  EXPECT_EQ(1u, countOutputInstTermDrivers(decode->getBit(5)));
  for (auto* bit : decode->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_LE(countOutputInstTermDrivers(bit), 1u) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombUnpackedPackedStructOutputLoopNoDuplicateDefaultDrivers) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_unpacked_packed_struct_output_loop_no_duplicate_default_drivers");

  const auto svPath =
    outPath /
    "always_comb_unpacked_packed_struct_output_loop_no_duplicate_default_drivers.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic [2:0] ex;
  logic [3:0] address;
} fetch_entry_s;

module always_comb_unpacked_packed_struct_output_loop_no_duplicate_default_drivers(
  input  logic [3:0] pc_i,
  output fetch_entry_s [1:0] fetch_entry_o
);
  always_comb begin
    for (int unsigned i = 0; i < 2; i++) begin
      fetch_entry_o[i].ex = '0;
      fetch_entry_o[i].address = pc_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_unpacked_packed_struct_output_loop_no_duplicate_default_drivers"));
  ASSERT_NE(top, nullptr);
  auto* fetchEntry = top->getBusNet(NLName("fetch_entry_o"));
  ASSERT_NE(fetchEntry, nullptr);
  ASSERT_EQ(14, fetchEntry->getWidth());
  for (auto* bit : fetchEntry->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_LE(countOutputInstTermDrivers(bit), 1u) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombTemporarySelectionCompoundReplaySupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_temporary_selection_compound_replay_supported");

  const auto svPath =
    outPath / "always_comb_temporary_selection_compound_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_temporary_selection_compound_replay_supported(
  input  logic [3:0] seed_i,
  input  logic [2:0] inc_i,
  output logic [3:0] result_o
);
  logic [3:0] tmp;

  always_comb begin
    tmp = seed_i;
    tmp[2:0] += inc_i;
    result_o = tmp;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_temporary_selection_compound_replay_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombSameNameLocalsDifferentWidthsSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_same_name_locals_different_widths_supported");

  const auto svPath =
    outPath / "always_comb_same_name_locals_different_widths_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_same_name_locals_different_widths_supported(
  input  logic       sel_i,
  input  logic [3:0] narrow_i,
  input  logic [11:0] addr_i,
  input  logic [7:0] wide_i,
  output logic [7:0] data_o
);
  always_comb begin
    data_o = '0;
    unique case (sel_i)
      1'b0: begin
        automatic logic [3:0] index = addr_i - 12'h3A0;
        if (index[0]) begin
          index = index >> 1;
        end
        data_o[3:0] = index;
      end
      default: begin
        automatic logic [7:0] index = wide_i;
        data_o = index;
      end
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_same_name_locals_different_widths_supported"));
  ASSERT_NE(top, nullptr);
  auto* data = top->getBusNet(NLName("data_o"));
  ASSERT_NE(data, nullptr);
  ASSERT_EQ(8, data->getWidth());
  for (auto* bit : data->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_LE(countOutputInstTermDrivers(bit), 1u) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombUnpackedStructArrayTemporaryDynamicMemberReplaySupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_unpacked_struct_array_temporary_dynamic_member_replay_supported");

  const auto svPath =
    outPath /
    "always_comb_unpacked_struct_array_temporary_dynamic_member_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic       checked;
  logic [2:0] hit;
} entry_t;

module always_comb_unpacked_struct_array_temporary_dynamic_member_replay_supported(
  input  logic       en_i,
  input  logic       ptr_i,
  input  logic [2:0] hit_i,
  input  entry_t [1:0] entry_q_i,
  output entry_t [1:0] entry_d_o,
  output logic [2:0] selected_hit_o
);
  entry_t [1:0] entry_d;

  always_comb begin
    entry_d = entry_q_i;
    if (en_i) begin
      entry_d[ptr_i].checked = 1'b1;
      entry_d[ptr_i].hit = hit_i;
    end
    selected_hit_o = entry_d[ptr_i].hit;
  end

  assign entry_d_o = entry_d;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_unpacked_struct_array_temporary_dynamic_member_replay_supported"));
  ASSERT_NE(top, nullptr);
  auto* entryD = top->getBusNet(NLName("entry_d_o"));
  ASSERT_NE(entryD, nullptr);
  ASSERT_EQ(8, entryD->getWidth());
  auto* selectedHit = top->getBusNet(NLName("selected_hit_o"));
  ASSERT_NE(selectedHit, nullptr);
  ASSERT_EQ(3, selectedHit->getWidth());
  for (auto* bit : selectedHit->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_LE(countOutputInstTermDrivers(bit), 1u) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombUnpackedArrayLocalInitializerReplayWidthSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_unpacked_array_local_initializer_replay_width_supported");

  const auto svPath =
    outPath /
    "always_comb_unpacked_array_local_initializer_replay_width_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_array_local_initializer_replay_width_supported(
  input  logic       sel_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  typedef logic [3:0] pair_t [0:1];

  always_comb begin
    data_o = '0;
    unique case (sel_i)
      1'b0: begin
        automatic pair_t pair = '{lo_i, hi_i};
        data_o = {pair[1], pair[0]};
      end
      default: begin
        data_o = {hi_i, lo_i};
      end
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_unpacked_array_local_initializer_replay_width_supported"));
  ASSERT_NE(top, nullptr);
  auto* data = top->getBusNet(NLName("data_o"));
  ASSERT_NE(data, nullptr);
  ASSERT_EQ(8, data->getWidth());
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombUnpackedStructLocalInitializerReplaySupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_unpacked_struct_local_initializer_replay_supported");

  const auto svPath =
    outPath / "always_comb_unpacked_struct_local_initializer_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_struct_local_initializer_replay_supported(
  input  logic       sel_i,
  input  logic [3:0] data_i,
  output logic [3:0] data_o
);
  typedef struct {
    logic [3:0] data;
  } tmp_t;

  always_comb begin
    data_o = '0;
    unique case (sel_i)
      1'b0: begin
        automatic tmp_t tmp = '{data: data_i};
        data_o = tmp.data;
      end
      default: begin
        data_o = ~data_i;
      end
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(
    NLName("always_comb_unpacked_struct_local_initializer_replay_supported"));
  ASSERT_NE(nullptr, top);
  auto* output = top->getBusNet(NLName("data_o"));
  ASSERT_NE(nullptr, output);
  for (auto* bit : output->getBits()) {
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
  EXPECT_EQ(nullptr, top->getNet(NLName("tmp")));
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseAlwaysCombDpiOutputIsAbstracted) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory("always_comb_dpi_output_abstracted");

  const auto svPath = outPath / "always_comb_dpi_output_abstracted.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(import "DPI-C" context function void ext_lookup(
  input longint addr,
  output longint data
);

module always_comb_dpi_output_abstracted(
  input  logic [63:0] addr_i,
  input  logic        en_i,
  output logic [63:0] data_o
);
  logic [63:0] lookup;

  always_comb begin
    if (en_i)
      ext_lookup(addr_i, lookup);
    else
      lookup = 64'bx;
  end

  assign data_o = lookup;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(NLName("always_comb_dpi_output_abstracted"));
  ASSERT_NE(top, nullptr);
  auto* lookup = top->getBusNet(NLName("lookup"));
  ASSERT_NE(lookup, nullptr);
  ASSERT_EQ(64, lookup->getWidth());
  for (auto* bit : lookup->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
  auto* data = top->getBusNet(NLName("data_o"));
  ASSERT_NE(data, nullptr);
  ASSERT_EQ(64, data->getWidth());
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseAlwaysCombDpiMultipleOutputsAreAbstracted) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory("always_comb_dpi_multiple_outputs_abstracted");

  const auto svPath = outPath / "always_comb_dpi_multiple_outputs_abstracted.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(import "DPI-C" context function void ext_lookup(
  input longint addr,
  output longint data,
  output longint status
);

module always_comb_dpi_multiple_outputs_abstracted(
  input  logic [63:0] addr_i,
  output logic [63:0] data_o,
  output logic [63:0] status_o
);
  logic [63:0] lookup;
  logic [63:0] lookup_status;

  always_comb begin
    ext_lookup(addr_i, lookup, lookup_status);
  end

  assign data_o = lookup;
  assign status_o = lookup_status;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_dpi_multiple_outputs_abstracted"));
  ASSERT_NE(top, nullptr);
  for (const auto* name : {"lookup", "lookup_status"}) {
    auto* net = top->getBusNet(NLName(name));
    ASSERT_NE(net, nullptr);
    ASSERT_EQ(64, net->getWidth());
    for (auto* bit : net->getBits()) {
      ASSERT_NE(bit, nullptr);
      EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    }
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseFunctionDirectReturnLocalAssignmentsSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "function_direct_return_local_assignments_supported");

  const auto svPath =
    outPath / "function_direct_return_local_assignments_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module function_direct_return_local_assignments_supported(
  input  logic [3:0] seed_i,
  input  logic       hi_i,
  output logic [3:0] result_o
);
  function automatic logic [3:0] build(input logic [3:0] seed, input logic hi);
    logic [3:0] tmp;
    tmp = seed;
    tmp[3] = hi;
    return {tmp[0], tmp[3:1]};
  endfunction

  always_comb begin
    result_o = build(seed_i, hi_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("function_direct_return_local_assignments_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseAlwaysCombConcatenationLHSSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory("always_comb_concatenation_lhs_supported");

  const auto svPath = outPath / "always_comb_concatenation_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concatenation_lhs_supported(
  input  logic [1:0] d_i,
  output logic       a_o,
  output logic       b_o
);
  always_comb begin
    {a_o, b_o} = d_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_concatenation_lhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("a_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("b_o")), nullptr);
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseAlwaysCombWhileStatementUnsupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory("always_comb_while_statement_unsupported");

  const auto svPath = outPath / "always_comb_while_statement_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_while_statement_unsupported(
  input  logic en_i,
  input  logic d_i,
  output logic y_o
);
  always_comb begin
    while (en_i) begin
      y_o = d_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombUnpackedStructMemberLHSBitsSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_unpacked_struct_member_lhs_bits_supported");

  const auto svPath =
    outPath / "always_comb_unpacked_struct_member_lhs_bits_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_struct_member_lhs_bits_supported(
  input  logic [1:0] d_i,
  output logic [7:0] q_o
);
  typedef struct {
    logic [1:0] a;
  } unpacked_s_t;
  unpacked_s_t s;

  always_comb begin
    s.a = d_i;
    q_o = {6'b0, s.a};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(
    NLName("always_comb_unpacked_struct_member_lhs_bits_supported"));
  ASSERT_NE(nullptr, top);
  auto* input = top->getBusNet(NLName("d_i"));
  auto* output = top->getBusNet(NLName("q_o"));
  ASSERT_NE(nullptr, input);
  ASSERT_NE(nullptr, output);
  for (int bit = 0; bit < 8; ++bit) {
    auto* driver = getSingleAssignInputDriving(output->getBit(bit));
    ASSERT_NE(nullptr, driver);
    if (bit < 2) {
      EXPECT_EQ(input->getBit(bit), driver);
    } else {
      EXPECT_TRUE(driver->isAssign0());
    }
  }

}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombMemberAccessNonIntegralLHSWidthSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_member_access_non_integral_lhs_width_supported");

  const auto svPath =
    outPath / "always_comb_member_access_non_integral_lhs_width_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_member_access_non_integral_lhs_width_supported;
  typedef struct {
    logic [1:0] a [0:1];
  } unpacked_s_t;

  unpacked_s_t s_q;
  unpacked_s_t s_n;

  always_comb begin
    s_n.a = s_q.a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(
    NLName("always_comb_member_access_non_integral_lhs_width_supported"));
  ASSERT_NE(nullptr, top);
  auto* source = top->getBusNet(NLName("s_q"));
  auto* destination = top->getBusNet(NLName("s_n"));
  ASSERT_NE(nullptr, source);
  ASSERT_NE(nullptr, destination);
  EXPECT_EQ(4, source->getWidth());
  EXPECT_EQ(4, destination->getWidth());
  for (int bit = 0; bit < 4; ++bit) {
    EXPECT_EQ(source->getBit(bit), getSingleAssignInputDriving(destination->getBit(bit)));
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseCompoundShifts) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_SV_BENCHMARKS_PATH) /
                        "compound_shifts/compound_shifts.sv");
  auto* top = library_->getSNLDesign(NLName("compound_shifts_top"));
  ASSERT_NE(nullptr, top);
  for (const auto* name : {"left_o", "right_o", "arithmetic_left_o", "unsigned_arithmetic_o",
                          "signed_right_o", "replay_o", "field_o", "constant_o", "giant_o"}) {
    auto* output = top->getBusTerm(NLName(name));
    ASSERT_NE(nullptr, output);
    for (auto* bit : output->getBusBits()) {
      EXPECT_EQ(1u, countOutputInstTermDrivers(bit->getNet())) << name;
    }
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseStreamingAssignmentLHS) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_SV_BENCHMARKS_PATH) /
                        "streaming_lhs/streaming_lhs.sv");
  auto* top = library_->getSNLDesign(NLName("streaming_lhs_top"));
  ASSERT_NE(nullptr, top);
  for (const auto* name : {"forward_o", "reverse_o", "nested_o", "selected_o",
                          "swap_o", "array_o", "multi_o", "stream_rhs_o"}) {
    auto* output = top->getBusTerm(NLName(name));
    ASSERT_NE(nullptr, output);
    for (auto* bit : output->getBusBits()) {
      EXPECT_EQ(1u, countOutputInstTermDrivers(bit->getNet())) << name;
    }
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectDynamicStreamingAssignmentLHS) {
  const auto path = createTestDirectory("dynamic_streaming_lhs") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [3:0] d, input logic [1:0] i,
  output logic [7:0] q);
  always_comb begin
    q = '0;
    {>>{q[i+:4]}} = d;
  end
endmodule
)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"dynamic or with-clause streaming assignment LHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectOverlappingStreamingAssignmentLHS) {
  const auto path = createTestDirectory("overlapping_streaming_lhs") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [15:0] d, output logic [7:0] q);
  always_comb {>>{q,q}} = d;
endmodule
)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"overlapping streaming assignment targets"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectSequentialStreamingAssignmentLHS) {
  const auto path = createTestDirectory("sequential_streaming_lhs") / "top.sv";
  std::ofstream(path) << R"(module top(input logic clk, input logic [7:0] d,
  output logic [7:0] q);
  always_ff @(posedge clk) {>>{q}} <= d;
endmodule
)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectWithClauseStreamingAssignmentLHS) {
  const auto path = createTestDirectory("with_clause_streaming_lhs") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [15:0] d,
  output logic [7:0] q [0:3]);
  always_comb begin
    q = '{default:'0};
    {>>{q with [1:2]}} = d;
  end
endmodule
)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"with-clause streaming assignment LHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, streamingLHSGoldenValuesFromSlangConstantEvaluation) {
  const auto path = createTestDirectory("streaming_lhs_constant_reference") / "top.sv";
  std::ofstream(path) << R"(module top(output wire [15:0][37:0] y);
  function automatic logic [37:0] reference(input logic [19:0] value);
    logic [9:0] reverse_value, nested_value, forward_value;
    logic [3:0] a, b;
    {<<4{reverse_value}} = value[9:0];
    {<<4{{<<2{nested_value}}}} = value[9:0];
    {>>{forward_value}} = value;
    a = value[3:0];
    b = value[7:4];
    {>>{a,b}} = {b,a};
    return {forward_value,nested_value,reverse_value,a,b};
  endfunction
  for (genvar i = 0; i < 16; i++) begin : g
    // A localparam forces Slang evaluation, independently of Naja's procedural lowering.
    localparam logic [37:0] EXPECTED = reference(20'(i * 20'h8421));
    assign y[i] = EXPECTED;
  end
endmodule
)";
  SNLSVConstructor constructor(library_);
  constructor.construct(path);
  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  auto* y = top->getBusTerm(NLName("y"));
  ASSERT_NE(nullptr, y);
  for (unsigned i = 0; i < 16; ++i) {
    const uint64_t value = i * 0x8421;
    const uint64_t reverse = ((value & 3) << 8) | (((value >> 2) & 15) << 4) |
                             ((value >> 6) & 15);
    const uint64_t nested = ((reverse & 3) << 8) | (((reverse >> 2) & 3) << 6) |
      (((reverse >> 4) & 3) << 4) | (((reverse >> 6) & 3) << 2) | ((reverse >> 8) & 3);
    const uint64_t expected = ((value >> 10) << 28) | (nested << 18) |
                              (reverse << 8) | (value & 255);
    for (unsigned bit = 0; bit < 38; ++bit) {
      auto* net = getSingleAssignInputDriving(y->getBit(i * 38 + bit)->getNet());
      ASSERT_NE(nullptr, net);
      EXPECT_TRUE((expected >> bit) & 1 ? net->isAssign1() : net->isAssign0())
        << "value=" << value << " bit=" << bit;
    }
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseIndependentLatchLoops) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_SV_BENCHMARKS_PATH) /
                        "latch_loops/latch_loops.sv");
  auto* top = library_->getSNLDesign(NLName("latch_loops_top"));
  ASSERT_NE(nullptr, top);
  for (const auto* name : {"array_o", "nested_o", "priority_o", "partial_o", "nb_priority_o", "branch_o", "zero_o"}) {
    auto* output = top->getBusTerm(NLName(name));
    ASSERT_NE(nullptr, output);
    for (auto* bit : output->getBusBits()) {
      EXPECT_EQ(1u, countOutputInstTermDrivers(bit->getNet())) << name;
    }
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectDependentLatchLoopWrites) {
  const auto path = createTestDirectory("dependent_latch_loops") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic [7:0] d,
    output logic [7:0] q, r);
    always_latch begin
      if (en) q = d;
      if (en) r = q;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"read a variable assigned in the same block"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectMixedLatchLoopScheduling) {
  const auto path = createTestDirectory("mixed_latch_loops") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic [7:0] d,
    output logic [7:0] q);
    always_latch begin
      if (en) q = d;
      if (!en) q <= d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"mixed blocking and nonblocking"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectLatchLoopFunctionDependencies) {
  const auto path = createTestDirectory("latch_loop_function") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic [7:0] d,
    output logic [7:0] q, r);
    function automatic logic [7:0] read_q(); return q; endfunction
    always_latch begin
      if (en) q = d;
      if (en) r = read_q();
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"calls and expression side effects"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectDynamicLatchLoopTarget) {
  const auto path = createTestDirectory("dynamic_latch_loop") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, d, input logic [2:0] index,
    output logic [7:0] q, output logic r);
    always_latch begin
      if (en) q[index] = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"statically selected targets"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectLatchLoopConditionDependency) {
  const auto path = createTestDirectory("latch_loop_condition") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, d, output logic q, r);
    always_latch begin
      if (en) q = d;
      if (q) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"read a variable assigned in the same block"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchConditionCallCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_condition_call") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in,
    output logic [1:0] q, output logic r);
    logic d;
    assign d = d_in;
    function automatic logic f(input logic a); return a; endfunction
    always_latch begin
      if (f(en)) q = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"calls and expression side effects"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchConditionIncrementCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_condition_increment") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in,
    output logic [1:0] q, output logic r);
    logic d;
    assign d = d_in;
    always_latch begin
      if (++d) q = en;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"calls and expression side effects"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchConditionAssignmentCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_condition_assignment") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in,
    output logic [1:0] q, output logic r);
    logic d;
    assign d = d_in;
    always_latch begin
      if ((d = en)) q = en;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"calls and expression side effects"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchTimedWriteCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_timed_write") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in,
    output logic [1:0] q, output logic r);
    logic d;
    assign d = d_in;
    always_latch begin
      if (en) q <= #1 d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"timed independent latch writes"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchRealConditionCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_real_condition") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in, input real condition,
    output logic [1:0] q, output logic r);
    logic d;
    assign d = d_in;
    always_latch begin
      if (condition) q = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"Unsupported latch block"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchUnresolvedDataCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_unresolved_data") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in,
    output logic [1:0] q, output logic r);
    logic d;
    assign d = d_in;
    always_latch begin
      if (en) q = d ** en;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unable to resolve independent latch data"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchRealTargetCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_real_target") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, input logic d_in,
    output real q, output logic r);
    logic d;
    assign d = d_in;
    always_latch begin
      if (en) q = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"Unsupported latch block"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectReturnFunctionWideCaseWithoutDefaultCoverage) {
  const auto path = createTestDirectory("reject_return_function_wide_case_without_default") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [16:0] a, output logic q);
    function automatic logic f(input logic [16:0] a);
      case (a)
        17'd0: return 1;
      endcase
    endfunction
    always_comb q = f(a);
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unable to resolve always_comb RHS bits for Call"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectReturnFunctionUnsupportedDefaultCoverage) {
  const auto path = createTestDirectory("reject_return_function_unsupported_default") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [16:0] a, output logic q);
    function automatic logic f(input logic [16:0] a);
      case (a)
        0: return 1;
        default: while (a) return 0;
      endcase
    endfunction
    always_comb q = f(a);
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unable to resolve always_comb RHS bits for Call"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectStreamingUnresolvedRHSCoverage) {
  const auto path = createTestDirectory("reject_streaming_unresolved_rhs") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a, output logic [7:0] q);
    function automatic logic [7:0] f(input logic [7:0] a);
      while (a) return a;
      return 0;
    endfunction
    always_comb begin
      {>>{q}} = f(a);
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unable to resolve streaming assignment RHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectCompoundShiftUnresolvedCountCoverage) {
  const auto path = createTestDirectory("reject_compound_shift_unresolved_count") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a, output logic [7:0] q);
    function automatic logic [7:0] f(input logic [7:0] a);
      while (a) return a;
      return 0;
    endfunction
    always_comb begin
      q = a;
      q <<= f(a);
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"failed to resolve shift amount bits"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectStreamingNonblockingAssignmentCoverage) {
  const auto path = createTestDirectory("reject_streaming_nonblocking_assignment") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a, output logic [7:0] q);
    always_comb begin
      {>>{q}} <= a;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path,
    {"scheduling-sensitive combinational non-blocking assignment has an ambiguous target"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectStreamingMissingStorageCoverage) {
  const auto path = createTestDirectory("reject_streaming_missing_storage") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [15:0] d, output logic [7:0] q);
    byte missing[];
    always_comb {>>{{q, missing[0]}}} = d;
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unable to resolve fixed streaming assignment LHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectStreamingDynamicConcatenationLeafCoverage) {
  const auto path = createTestDirectory("reject_streaming_dynamic_concatenation_leaf") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a, output logic [7:0] q);
    always_comb begin
      {>>{{q[a[0]],q[1]}}} = a;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"dynamic or with-clause streaming assignment LHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseExhaustiveReturnFunctionCaseCoverage) {
  const auto path = createTestDirectory("parse_exhaustive_return_function_case") / "top.sv";
  std::ofstream(path) << R"(module top(input logic a, output logic q);
    function automatic logic f(input logic a);
      case (a)
        0: return 1'b0;
        1: return 1'b1;
      endcase
    endfunction
    always_comb q = f(a);
  endmodule)";
  SNLSVConstructor constructor(library_);
  constructor.construct(path);
  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(1u, countOutputInstTermDrivers(top->getScalarTerm(NLName("q"))->getNet()));
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseStreamingConcatenationBitConnectionsCoverage) {
  const auto path = createTestDirectory("parse_streaming_concatenation_bit_connections") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a, output logic [7:0] q);
    always_comb {>>{{q[7:4], q[3:0]}}} = a;
  endmodule)";
  SNLSVConstructor constructor(library_);
  constructor.construct(path);
  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  auto* q = top->getBusTerm(NLName("q"));
  auto* a = top->getBusTerm(NLName("a"));
  ASSERT_NE(nullptr, q);
  ASSERT_NE(nullptr, a);
  for (int bit = 0; bit < 8; ++bit) {
    EXPECT_EQ(a->getBit(bit)->getNet(), getSingleAssignInputDriving(q->getBit(bit)->getNet()));
  }
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectStreamingDynamicRHSCoverage) {
  const auto path = createTestDirectory("reject_streaming_dynamic_rhs") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a[0:3], input logic [1:0] n, output logic [7:0] q);
    always_comb {>>{q}} = {>>{a with [0+:n]}};
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unable to resolve streaming assignment RHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectStreamingDynamicStorageCoverage) {
  const auto path = createTestDirectory("reject_streaming_dynamic_storage") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [7:0] a);
    byte q[];
    always_comb {>>{q}} = a;
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"dynamic or with-clause streaming assignment LHS"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectForLoopUnsignedStepOverflowCoverage) {
  const auto path = createTestDirectory("reject_for_loop_unsigned_step_overflow") / "top.sv";
  std::ofstream(path) << R"(module top(input logic [3:0] d, output logic [3:0] q);
    localparam logic [63:0] STEP = 64'h8000000000000000;
    always_comb for (longint unsigned i = 0; i < 4; i = i + STEP) q[i] = d[i];
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unsupported for-loop step expression"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectIndependentLatchLocalDeclarationCoverage) {
  const auto path = createTestDirectory("reject_independent_latch_local_declaration") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, d, output logic q, r);
    always_latch begin
      automatic logic tmp;
      if (en) q = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unsupported statement in independent latch writes"});
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseIndependentLatchConstantGuardCoverage) {
  const auto path = createTestDirectory("parse_independent_latch_constant_guard") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, d, output logic q, r);
    always_latch begin
      if (1) q = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  constructor.construct(path);
  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(top->getScalarTerm(NLName("d"))->getNet(),
            getSingleAssignInputDriving(top->getScalarTerm(NLName("q"))->getNet()));
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseIndependentLatchEmptyAndFalseGuardCoverage) {
  const auto path = createTestDirectory("parse_independent_latch_empty_and_false_guard") / "top.sv";
  std::ofstream(path) << R"(module top(input logic en, d, output logic q, r);
    always_latch begin
      ;
      if (0) q = d;
      if (en) r = d;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  constructor.construct(path);
  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(0u, countOutputInstTermDrivers(top->getScalarTerm(NLName("q"))->getNet()));
  EXPECT_EQ(1u, countOutputInstTermDrivers(top->getScalarTerm(NLName("r"))->getNet()));
}

TEST_F(SNLSVConstructorTestAlwaysComb, rejectSequentialStreamingSchedulingReplayCoverage) {
  const auto path = createTestDirectory("reject_sequential_streaming_scheduling_replay") / "top.sv";
  std::ofstream(path) << R"(module top(input logic clk, input logic [7:0] d, output logic [7:0] q, r);
    always_ff @(posedge clk) begin
      q = d;
      {>>{q}} = d;
      // Reading q into another target forces scheduling replay.
      r = q;
    end
  endmodule)";
  SNLSVConstructor constructor(library_);
  expectUnsupportedConstruct(constructor, path, {"unsupported sequential streaming assignment"});
}


TEST_F(SNLSVConstructorTestAlwaysComb, parsePriorityReturnFunctionNestedStatementsCoverage) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "priority_return_shapes";
  std::filesystem::create_directories(directory);
  for (const auto& body : {
      "logic t; ;",
      "begin logic t; logic u; end for (int i = 0; i < 4; i++) if (a[i]) return 1'b1;",
      "begin logic t; t = a[0]; for (int i = 0; i < 4; i++) if (a[i]) return 1'b1; end",
      "begin for (int i = 0; i < 2; i++) if (a[i]) return 1'b1; for (int j = 2; j < 4; j++) if (a[j]) return 1'b1; end",
      "begin logic t; for (int i = 0; i < 2; i++) if (a[i]) return 1'b1; end begin logic u; for (int j = 2; j < 4; j++) if (a[j]) return 1'b1; end"}) {
    SCOPED_TRACE(body);
    const auto source = directory / "source.sv";
    std::ofstream(source)
      << "module top(input logic [3:0] a, output logic y);\n"
      << "function automatic logic f(input logic [3:0] a);\n"
      << body << "\nreturn " << (std::string(body) == "logic t; ;" ? "$clog2(a)" : "1'b0")
      << "; endfunction\n"
      << "assign y = f(a); endmodule\n";
    SNLSVConstructor constructor(library_);
    expectUnsupportedConstruct(constructor, source, {});
    auto* top = library_->getSNLDesign(NLName("top"));
    ASSERT_NE(nullptr, top);
    top->destroy();
  }
}
