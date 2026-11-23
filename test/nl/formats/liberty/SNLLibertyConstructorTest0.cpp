// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include <filesystem>

#include <chrono>
#include <fstream>
#include <system_error>

#include "NLUniverse.h"

#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesignModeling.h"
#include "SNLScalarTerm.h"

#include "SNLLibertyConstructor.h"

#include "SNLLibertyConstructorException.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest0: public ::testing::Test {
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

namespace {

void writeConflictingCellLiberty(
  const std::filesystem::path& path,
  const std::string& libraryName,
  const std::string& duplicateInput,
  const std::string& uniqueCellName) {
  std::ofstream output(path, std::ios::binary);
  output
    << "library (" << libraryName << ") {\n"
    << "  cell (duplicate) {\n"
    << "    pin (" << duplicateInput << ") { direction : input; }\n"
    << "    pin (Y) { direction : output; function : \"" << duplicateInput << "\"; }\n"
    << "  }\n"
    << "  cell (" << uniqueCellName << ") {\n"
    << "    pin (A) { direction : input; }\n"
    << "    pin (Y) { direction : output; function : \"A\"; }\n"
    << "  }\n"
    << "}\n";
}

std::filesystem::path writeTemporaryLiberty(
  const std::string& stem,
  const std::string& contents) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto path = std::filesystem::temp_directory_path()
    / std::filesystem::path(
      "naja_liberty_" + stem + "_" + std::to_string(stamp) + ".lib");
  std::ofstream output(path, std::ios::binary);
  output << contents;
  return path;
}

}  // namespace

TEST_F(SNLLibertyConstructorTest0, testConflictingCellNameFirstOneDefault) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto firstPath = std::filesystem::temp_directory_path()
    / std::filesystem::path(
      "naja_liberty_conflicting_cell_first_" + std::to_string(stamp) + ".lib");
  auto secondPath = std::filesystem::temp_directory_path()
    / std::filesystem::path(
      "naja_liberty_conflicting_cell_second_" + std::to_string(stamp) + ".lib");
  writeConflictingCellLiberty(firstPath, "FIRST", "A", "first_only");
  writeConflictingCellLiberty(secondPath, "SECOND", "B", "second_only");

  SNLLibertyConstructor constructor(library_);
  EXPECT_EQ(
    SNLLibertyConstructor::Config::ConflictingCellNamePolicy::FirstOne,
    constructor.config_.conflictingCellNamePolicy_);
  ASSERT_NO_THROW(
    constructor.construct(SNLLibertyConstructor::Paths{firstPath, secondPath}));

  EXPECT_EQ(3, library_->getSNLDesigns().size());
  auto duplicate = library_->getSNLDesign(NLName("duplicate"));
  ASSERT_NE(nullptr, duplicate);
  EXPECT_NE(nullptr, duplicate->getScalarTerm(NLName("A")));
  EXPECT_EQ(nullptr, duplicate->getScalarTerm(NLName("B")));
  EXPECT_NE(nullptr, library_->getSNLDesign(NLName("first_only")));
  EXPECT_NE(nullptr, library_->getSNLDesign(NLName("second_only")));

  std::error_code ec;
  std::filesystem::remove(firstPath, ec);
  std::filesystem::remove(secondPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testConflictingCellNameForbid) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto firstPath = std::filesystem::temp_directory_path()
    / std::filesystem::path(
      "naja_liberty_forbid_conflicting_cell_first_" + std::to_string(stamp) + ".lib");
  auto secondPath = std::filesystem::temp_directory_path()
    / std::filesystem::path(
      "naja_liberty_forbid_conflicting_cell_second_" + std::to_string(stamp) + ".lib");
  writeConflictingCellLiberty(firstPath, "FIRST", "A", "first_only");
  writeConflictingCellLiberty(secondPath, "SECOND", "B", "second_only");

  SNLLibertyConstructor constructor(library_);
  constructor.config_.conflictingCellNamePolicy_ =
    SNLLibertyConstructor::Config::ConflictingCellNamePolicy::Forbid;
  try {
    constructor.construct(SNLLibertyConstructor::Paths{firstPath, secondPath});
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find(secondPath.string()));
    EXPECT_NE(std::string::npos, e.getReason().find("duplicate"));
  }

  std::error_code ec;
  std::filesystem::remove(firstPath, ec);
  std::filesystem::remove(secondPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, test0) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("asap7_excerpt")
      / std::filesystem::path("test0.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("asap7sc7p5t_AO_LVT_FF_ccs_201020"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto design = library_->getSNLDesign(NLName("A2O1A1Ixp33_ASAP7_75t_L"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  auto a1 = design->getScalarTerm(NLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(NLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b = design->getScalarTerm(NLName("B"));
  ASSERT_NE(nullptr, b);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto c = design->getScalarTerm(NLName("C"));
  ASSERT_NE(nullptr, c);
  EXPECT_EQ(SNLTerm::Direction::Input, c->getDirection());
  auto y = design->getScalarTerm(NLName("Y"));
  ASSERT_NE(nullptr, y);
  EXPECT_EQ(SNLTerm::Direction::Output, y->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testPunctuatedCCBNames) {
  auto tempPath = writeTemporaryLiberty(
    "punctuated_ccb_names",
    R"liberty(
library(test) {
  cell(C) {
    pin(A) {
      direction:input;
      input_ccb(cell_cond__!A&!B&!C__1) {
        related_ccb_node : "net1:15";
      }
    }
    pin(Y) {
      direction : output;
      function : "A";
      timing() {
        related_pin : "A";
        timing_type : combinational;
        active_input_ccb(cell_cond__!A&!B&!C__1, vendor:block:b);
        active_output_ccb(cell_cond__!A&!B&!C__1);
      }
      output_ccb(cell_cond__!A&!B&!C__1) {
        related_ccb_node : "net2:16";
      }
    }
  }
}
)liberty");

  SNLLibertyConstructor constructor(library_);
  ASSERT_NO_THROW(constructor.construct(tempPath));

  auto design = library_->getSNLDesign(NLName("C"));
  ASSERT_NE(nullptr, design);
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("A")));
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("Y")));

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testInOut) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("inout_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("iocell"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  auto z = design->getScalarTerm(NLName("Z"));
  EXPECT_EQ(SNLTerm::Direction::InOut, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testInternalPin) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("internal_pin_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("internal_pin_test"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  auto i = design->getScalarTerm(NLName("I"));
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(design).isInitialized());
}

TEST_F(SNLLibertyConstructorTest0, testNonExistingFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("missing.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongFileType) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("not_a_file.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongSyntaxFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("syntax_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(testPath.string()));
    EXPECT_NE(std::string::npos, reason.find("line 1"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testUnquotedLatchEnableExpression) {
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("latch_unquoted_enable_test.lib"));
  SNLLibertyConstructor constructor(library_);
  ASSERT_NO_THROW(constructor.construct(testPath));
  auto design = library_->getSNLDesign(NLName("cell1"));
  ASSERT_NE(nullptr, design);
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("clk")));
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("g")));
}

TEST_F(SNLLibertyConstructorTest0, testDetailedParserErrorMessage) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_parser_detail_error_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << ":";
  }
  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(tempPath.string()));
    EXPECT_NE(std::string::npos, reason.find("line 1"));
    EXPECT_NE(std::string::npos, reason.find("Unexpected ':'"));
  }
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnnamedPinErrorHasGroupContext) {
  auto tempPath = writeTemporaryLiberty(
    "unnamed_pin",
    R"(library (UNNAMED_PIN) {
  cell (BAD) {
    pin () {
      direction : input;
    }
  }
})");

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("While processing `pin`"));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find("The `pin` group must specify a name."));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnnamedSequentialGroupIsIgnored) {
  auto tempPath = writeTemporaryLiberty(
    "unnamed_ff",
    R"(library (UNNAMED_FF) {
  cell (FF) {
    ff () {
      clocked_on : "CK";
      next_state : "D";
    }
    pin (CK) {
      direction : input;
    }
    pin (D) {
      direction : input;
    }
    pin (Q) {
      direction : output;
      function : "D";
    }
  }
})");

  SNLLibertyConstructor constructor(library_);
  ASSERT_NO_THROW(constructor.construct(tempPath));
  auto* design = library_->getSNLDesign(NLName("FF"));
  ASSERT_NE(nullptr, design);
  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(design));

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testMixedFFAndLatchCellRemainsLoadable) {
  auto tempPath = writeTemporaryLiberty(
    "mixed_ff_latch",
    R"LIB(library (MIXED_FF_LATCH) {
  cell (power_cell) {
    ff (Q1, QN1) {
      clocked_on : "clk";
      next_state : "(D * !scan_enable + scan_input * scan_enable)";
      clear : "(((!b_sig_b) * !Q2) + !read)";
      preset : "((!b_sig_b) * Q2)";
      clear_preset_var1 : "L";
      clear_preset_var2 : "H";
    }
    latch (Q2, QN2) {
      enable : "b_sig_one";
      data_in : "Q1";
    }
    pin (D) { direction : input; }
    pin (clk) { direction : input; }
    pin (scan_enable) { direction : input; }
    pin (scan_input) { direction : input; }
    pin (b_sig_b) { direction : input; }
    pin (read) { direction : input; }
    pin (b_sig_one) { direction : input; }
    pin (Q) { direction : output; function : "Q1"; }
  }
})LIB");

  SNLLibertyConstructor constructor(library_);
  ASSERT_NO_THROW(constructor.construct(tempPath));
  auto* design = library_->getSNLDesign(NLName("power_cell"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(8u, design->getScalarTerms().size());
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("Q")));
  EXPECT_FALSE(SNLDesignModeling::hasSequentialModel(design));

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testInvalidBusBoundsHaveGroupContext) {
  auto tempPath = writeTemporaryLiberty(
    "invalid_bus_bounds",
    R"(library (INVALID_BUS_BOUNDS) {
  type (BAD_BUS) {
    bit_from : invalid;
    bit_to : 0;
    downto : true;
  }
  cell (BAD) {
    bus (A) {
      bus_type : BAD_BUS;
      direction : input;
    }
  }
})");

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find("While processing `bus (A)`"));
    EXPECT_NE(std::string::npos, e.getReason().find("stoi"));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnnamedCellErrorHasCellContext) {
  auto tempPath = writeTemporaryLiberty(
    "unnamed_cell",
    R"(library (UNNAMED_CELL) {
  cell () {
  }
})");

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("cell `<unnamed>`"));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find("The `cell` group must specify a cell name."));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testPrimitiveCellInStandardLibraryError) {
  auto tempPath = writeTemporaryLiberty(
    "standard_library",
    R"(library (STANDARD_LIBRARY) {
  cell (PRIMITIVE) {
  }
})");
  auto* standardLibrary = NLLibrary::create(
    library_->getDB(),
    NLLibrary::Type::Standard,
    NLName("STANDARD_TARGET"));

  SNLLibertyConstructor constructor(standardLibrary);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("cell `PRIMITIVE`"));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find("Cannot create a primitive design"));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnexpectedTopLevelGroup) {
  auto tempPath = writeTemporaryLiberty(
    "unexpected_top_level",
    R"(cell (NOT_A_LIBRARY) {
})");

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "Expected a top-level `library` group, found `cell`."));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnnamedTopLevelLibrary) {
  auto tempPath = writeTemporaryLiberty(
    "unnamed_library",
    R"(library () {
})");

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(
        std::string::npos,
        e.getReason().find(
            "The top-level `library` group must specify a library name."));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testLibraryRenameCollisionHasFileContext) {
  auto tempPath = writeTemporaryLiberty(
    "library_name_collision",
    R"(library (EXISTING_LIBRARY) {
})");
  NLLibrary::create(
    library_->getDB(),
    NLLibrary::Type::Standard,
    NLName("EXISTING_LIBRARY"));

  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find(tempPath.string()));
    EXPECT_NE(
        std::string::npos,
        e.getReason().find("another library"));
  }

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testFunctionErrorHasLocationContext) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_bad_function_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output
      << "library (CTX) {\n"
      << "  cell (BAD) {\n"
      << "    pin (A) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (B) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (CI) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (Y) {\n"
      << "      direction : output;\n"
      << "      function : \"(((A ^ B) CI) | (A B\";\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  }
  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(tempPath.string()));
    EXPECT_NE(std::string::npos, reason.find("cell `BAD`"));
    EXPECT_NE(std::string::npos, reason.find("pin `Y`"));
    EXPECT_NE(std::string::npos, reason.find("at line 14"));
    EXPECT_NE(
        std::string::npos,
        reason.find("`(((A ^ B) CI) | (A B`"));
  }
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testMultiOutputFunctionErrorHasLocationContext) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_bad_multi_output_function_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output
      << "library (CTX_MULTI) {\n"
      << "  cell (BAD_MULTI) {\n"
      << "    pin (A) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (B) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (CI) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (Y0) {\n"
      << "      direction : output;\n"
      << "      function : \"A & B\";\n"
      << "    }\n"
      << "    pin (Y1) {\n"
      << "      direction : output;\n"
      << "      function : \"(((A ^ B) CI) | (A B\";\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  }
  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(tempPath.string()));
    EXPECT_NE(std::string::npos, reason.find("cell `BAD_MULTI`"));
    EXPECT_NE(std::string::npos, reason.find("pin `Y1`"));
    EXPECT_NE(std::string::npos, reason.find("at line 18"));
  }
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testMixedOutputKindsRemainOpaque) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_mixed_outputs_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output
      << "library (MIXED_OUTPUTS) {\n"
      << "  type (BUS2) {\n"
      << "    base_type : array;\n"
      << "    data_type : bit;\n"
      << "    bit_width : 2;\n"
      << "    bit_from : 1;\n"
      << "    bit_to : 0;\n"
      << "    downto : true;\n"
      << "  }\n"
      << "  cell (MIXED) {\n"
      << "    pin (A) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (Y) {\n"
      << "      direction : output;\n"
      << "      function : \"A\";\n"
      << "    }\n"
      << "    pin (IO) {\n"
      << "      direction : inout;\n"
      << "    }\n"
      << "    bus (BO) {\n"
      << "      bus_type : BUS2;\n"
      << "      direction : output;\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  }

  SNLLibertyConstructor constructor(library_);
  ASSERT_NO_THROW(constructor.construct(tempPath));
  EXPECT_EQ(NLName("MIXED_OUTPUTS"), library_->getName());

  auto design = library_->getSNLDesign(NLName("MIXED"));
  ASSERT_NE(nullptr, design);
  auto a = design->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto y = design->getScalarTerm(NLName("Y"));
  ASSERT_NE(nullptr, y);
  EXPECT_EQ(SNLTerm::Direction::Output, y->getDirection());
  auto io = design->getScalarTerm(NLName("IO"));
  ASSERT_NE(nullptr, io);
  EXPECT_EQ(SNLTerm::Direction::InOut, io->getDirection());
  auto bo = design->getBusTerm(NLName("BO"));
  ASSERT_NE(nullptr, bo);
  EXPECT_EQ(SNLTerm::Direction::Output, bo->getDirection());
  EXPECT_EQ(1, bo->getMSB());
  EXPECT_EQ(0, bo->getLSB());

  EXPECT_EQ(0u, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(design).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(design, y->getFlatID()).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(design, io->getFlatID()).isInitialized());
  ASSERT_NE(nullptr, bo->getBit(1));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(
      design, bo->getBit(1)->getFlatID()).isInitialized());
  ASSERT_NE(nullptr, bo->getBit(0));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(
      design, bo->getBit(0)->getFlatID()).isInitialized());

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testMalformedZip) {
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_test.lib.zip");
  {
    std::ofstream output(tempPath, std::ios::binary);
    const unsigned char badZipHeader[] = {'P', 'K', 0x03, 0x04};
    output.write(reinterpret_cast<const char*>(badZipHeader), sizeof(badZipHeader));
  }
  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnreadableFile) {
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_unreadable.lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << "library (x) { }";
  }
  std::error_code ec;
  std::filesystem::permissions(tempPath, std::filesystem::perms::none, ec);
  if (ec) {
    std::filesystem::remove(tempPath, ec);
    GTEST_SKIP() << "Permissions not supported on this platform";
  }

  {
    std::ifstream probe(tempPath, std::ios::binary);
    if (probe.is_open()) {
      std::filesystem::permissions(tempPath, std::filesystem::perms::owner_all, ec);
      std::filesystem::remove(tempPath, ec);
      GTEST_SKIP() << "Unreadable file still readable (likely running as root)";
    }
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
  std::filesystem::permissions(tempPath, std::filesystem::perms::owner_all, ec);
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testWrongDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("direction_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testMissingDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("missing_direction_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    const auto& reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(testPath.string()));
    EXPECT_NE(std::string::npos, reason.find("line 2, cell `cell0`"));
    EXPECT_NE(std::string::npos, reason.find("`pin (A)` at line 3"));
    EXPECT_NE(std::string::npos, reason.find("Direction not found"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testInconsistentBusChildDirections) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("inconsistent_bus_direction_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    const auto& reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(testPath.string()));
    EXPECT_NE(
        std::string::npos,
        reason.find("line 11, cell `PADCELL_DRV_MIXED`"));
    EXPECT_NE(std::string::npos, reason.find("`bus (DRV)` at line 16"));
    EXPECT_NE(
        std::string::npos,
        reason.find("Inconsistent child pin directions for bus DRV"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testMissingBusType) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("missing_bus_type_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

// Test error for truth table on bus with truth_table_on_bus_error.lib
TEST_F(SNLLibertyConstructorTest0, testTruthTableOnBusError) {
  NLLibrary*  library = NLLibrary::create(NLDB::create(NLUniverse::get()),
      NLLibrary::Type::Primitives, NLName("MYLIB"));
  SNLLibertyConstructor constructor(library);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("truth_table_on_bus_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testBundleMembersMismatch) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("bundle_members_mismatch_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Bundle D lists missing member D1"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testBundleDirectionMembersMismatch) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("bundle_direction_members_mismatch_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Bundle D lists missing member D1"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testBundleExtraMember) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("bundle_extra_member_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Bundle D defines extra member D1"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testBundleMissingMembers) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("bundle_missing_members_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Bundle D does not define members(...)"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testInternalBundleIgnored) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("internal_bundle_ignored.lib"));
  ASSERT_NO_THROW(constructor.construct(testPath));
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("synthetic_bundle_cell"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBundleTerms().empty());
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("clock")));
  EXPECT_NE(nullptr, design->getScalarTerm(NLName("enable")));
  EXPECT_EQ(nullptr, design->getBundleTerm(NLName("internal_data")));
}

TEST_F(SNLLibertyConstructorTest0, testNestedBundleError) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("nested_bundle_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Nested Liberty bundles are not supported in bundle D"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testMalformedBundleMemberDefinition) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("malformed_bundle_member_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Malformed Liberty bundle member definition in bundle D"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testBundleMemberMissingDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("bundle_member_missing_direction_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Direction not found for bundle member D0"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testDuplicateBundleMemberDefinition) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("duplicate_bundle_member_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Duplicate Liberty bundle member definition D0 in bundle D"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testInconsistentBundleDirectionWithChild) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("inconsistent_bundle_parent_direction_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Inconsistent child directions for bundle D"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testInconsistentBundleDirections) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("inconsistent_bundle_direction_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    EXPECT_NE(std::string::npos, e.getReason().find("Inconsistent child directions for bundle D"));
  }
}


TEST(LoaderProgressTests, SNLLibertyConstructor) {
  const auto path = std::filesystem::temp_directory_path() /
    ("naja-progress-SNLLibertyConstructor-" + std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count()));
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      if (NLUniverse::get()) NLUniverse::get()->destroy();
      std::error_code error;
      std::filesystem::remove(path, error);
    }
  } cleanup{path};
  { std::ofstream file(path); file << R"source(library(synthetic) {cell(buffer) {pin(A) {direction: input;} pin(Y) {direction: output; function: "A";} }})source"; }
  const auto destination = [] {
    auto* db = NLDB::create(NLUniverse::create());
    return NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("DESIGN"));
  };
  {
    SNLLibertyConstructor constructor(destination());
    constructor.construct(path);
  }
  const auto baseline = NLUniverse::get()->getContentHash();
  NLUniverse::get()->destroy();
  std::vector<std::string> stages;
  {
    SNLLibertyConstructor constructor(destination());
    constructor.setProgressCallback([&](const naja::NajaProgress& event) {
      if (stages.empty() || stages.back() != event.stage) stages.emplace_back(event.stage);
      EXPECT_TRUE(event.total == 0 || event.current <= event.total);
      return true;
    });
    constructor.construct(path);
  }
  EXPECT_EQ(baseline, NLUniverse::get()->getContentHash());
  EXPECT_EQ((std::vector<std::string>{"parse", "liberty", "done"}), stages);
  NLUniverse::get()->destroy();
  {
    SNLLibertyConstructor constructor(destination());
    constructor.setProgressCallback([](const naja::NajaProgress& event) {
      return !(event.stage == "liberty" && event.current > 0);
    });
    EXPECT_THROW(constructor.construct(path), naja::NajaCancelled);
  }
}
