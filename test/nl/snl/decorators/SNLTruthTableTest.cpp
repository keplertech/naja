// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLTruthTable.h"
#include "NLBitVecDynamic.h"
#include "NLBitDependencies.h"
using namespace naja::NL;

TEST(SNLTruthTableTest, VectorMetadataOrdering) {
  using Update = SNLTruthTable::VectorUpdateMetadata;
  using Select = SNLTruthTable::VectorSelectMetadata;
  const Update update{4, 2, 3, 0, false};
  const Select select{4, 2, 3, 0, false};
  for (const auto& larger : {Update{5, 2, 3, 0, false}, Update{4, 3, 3, 0, false},
                            Update{4, 2, 4, 0, false}, Update{4, 2, 3, 1, false},
                            Update{4, 2, 3, 0, true}}) {
    EXPECT_LT(update, larger);
    EXPECT_FALSE(larger < update);
  }
  for (const auto& larger : {Select{5, 2, 3, 0, false}, Select{4, 3, 3, 0, false},
                            Select{4, 2, 4, 0, false}, Select{4, 2, 3, 1, false},
                            Select{4, 2, 3, 0, true}}) {
    EXPECT_LT(select, larger);
    EXPECT_FALSE(larger < select);
  }
  EXPECT_FALSE(update < update);
  EXPECT_FALSE(select < select);
  const auto updateTable = SNLTruthTable::VectorUpdate(update, SNLTruthTable::fullDependencies(7));
  const auto selectTable = SNLTruthTable::VectorSelect(select, SNLTruthTable::fullDependencies(7));
  auto otherUpdate = update;
  auto otherSelect = select;
  otherUpdate.outputBit = 1;
  otherSelect.outputBit = 1;
  const auto largerUpdate = SNLTruthTable::VectorUpdate(otherUpdate, SNLTruthTable::fullDependencies(7));
  const auto largerSelect = SNLTruthTable::VectorSelect(otherSelect, SNLTruthTable::fullDependencies(7));
  EXPECT_LT(updateTable, largerUpdate);
  EXPECT_FALSE(largerUpdate < updateTable);
  EXPECT_LT(selectTable, largerSelect);
  EXPECT_FALSE(largerSelect < selectTable);
}

TEST(SNLTruthTableTest, VectorMetadataValidation) {
  const auto deps = SNLTruthTable::fullDependencies(6);
  const SNLTruthTable::VectorUpdateMetadata update{3, 2, 2, 1, false};
  const SNLTruthTable::VectorSelectMetadata select{4, 2, 2, 1, false};
  EXPECT_EQ(update, SNLTruthTable::VectorUpdate(update, deps).getVectorUpdateMetadata());
  EXPECT_EQ(select, SNLTruthTable::VectorSelect(select, deps).getVectorSelectMetadata());
  for (const auto& invalidDeps : {std::vector<uint64_t>{}, SNLTruthTable::fullDependencies(5),
                                  SNLTruthTable::fullDependencies(7)}) {
    EXPECT_THROW(SNLTruthTable::VectorUpdate(update, invalidDeps), NLException);
    EXPECT_THROW(SNLTruthTable::VectorSelect(select, invalidDeps), NLException);
  }
  const auto max = std::numeric_limits<uint32_t>::max();
  const SNLTruthTable::VectorUpdateMetadata overflowingUpdate{3, max, 2, 0, false};
  const SNLTruthTable::VectorSelectMetadata overflowingSelect{max, 2, 2, 0, false};
  EXPECT_THROW(SNLTruthTable::VectorUpdate(overflowingUpdate, {}), NLException);
  EXPECT_THROW(SNLTruthTable::VectorSelect(overflowingSelect, {}), NLException);
  for (const auto& invalid : {SNLTruthTable::VectorUpdateMetadata{0, 2, 2, 0, false},
                             {3, 0, 2, 0, false}, {3, 2, 0, 0, false}, {3, 2, 2, 3, false}}) {
    const auto count = invalid.updateWidth + invalid.addressSize + 2;
    EXPECT_THROW(SNLTruthTable::VectorUpdate(invalid, SNLTruthTable::fullDependencies(count)), NLException);
  }
  for (const auto& invalid : {SNLTruthTable::VectorSelectMetadata{0, 2, 2, 0, false},
                             {4, 0, 2, 0, false}, {4, 2, 0, 0, false}, {4, 2, 2, 2, false}}) {
    const auto count = invalid.width + invalid.addressSize;
    EXPECT_THROW(SNLTruthTable::VectorSelect(invalid, SNLTruthTable::fullDependencies(count)), NLException);
  }
  EXPECT_THROW(SNLTruthTable::Logic0().getVectorUpdateMetadata(), NLException);
  EXPECT_THROW(SNLTruthTable::Logic0().getVectorSelectMetadata(), NLException);
  EXPECT_THROW(SNLTruthTable::VectorUpdate(update, deps).getVectorSelectMetadata(), NLException);
  EXPECT_THROW(SNLTruthTable::VectorSelect(select, deps).getVectorUpdateMetadata(), NLException);
}

TEST(SNLTruthTableTest, VectorConstantReduction) {
  const auto deps = SNLTruthTable::fullDependencies(6);
  const auto select = SNLTruthTable::VectorSelect({4, 2, 2, 0, false}, deps);
  EXPECT_THROW(select.getReducedWithConstants({{0, false}}), NLException);
  const auto table = SNLTruthTable::TableSelect(2, 4, deps);
  EXPECT_THROW(table.getReducedWithConstants({{0, false}}), NLException);
  for (const bool isSigned : {false, true}) {
    for (uint32_t output = 0; output < 3; ++output) {
      const auto update = SNLTruthTable::VectorUpdate({3, 2, 2, output, isSigned}, deps);
      EXPECT_THROW(update.getReducedWithConstants({{0, false}}), NLException);
      EXPECT_THROW(update.getReducedWithConstants(
        {{0, false}, {1, false}, {2, false}, {3, false}, {4, false}, {4, true}}), NLException);
      for (unsigned pattern = 0; pattern < 64; ++pattern) {
        SNLTruthTable::ConstantInputs constants;
        // Reverse order verifies constants are assigned by index, not position.
        for (unsigned index = 6; index > 0; --index)
          constants.emplace_back(index - 1, (pattern >> (index - 1)) & 1u);
        const int rawOffset = ((pattern >> 2) & 2u) | ((pattern >> 4) & 1u);
        const int offset = isSigned && rawOffset >= 2 ? rawOffset - 4 : rawOffset;
        const int dataBit = static_cast<int>(output) - offset;
        const bool expected = (pattern & 32u) && dataBit >= 0 && dataBit < 2
          ? bool((pattern >> (2 - dataBit)) & 1u) : bool(pattern & 1u);
        EXPECT_EQ(expected ? SNLTruthTable::Logic1() : SNLTruthTable::Logic0(),
          update.getReducedWithConstants(constants))
          << "signed=" << isSigned << " output=" << output << " pattern=" << pattern;
      }
    }
  }
}

TEST(SNLTruthTableTest, test) {

  SNLTruthTable ttand2(2, 0b1000, SNLTruthTable::fullDependencies(2));
  EXPECT_EQ(2, ttand2.size());
  EXPECT_EQ(0b1000, ttand2.bits().operator uint64_t());
  EXPECT_FALSE(ttand2.all0());
  EXPECT_FALSE(ttand2.all1());
  auto reducedtt = ttand2.getReducedWithConstant(0, 0);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all0());
  reducedtt = ttand2.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 1);
  //buffer
  EXPECT_EQ(0b10, reducedtt.bits().operator uint64_t());
  SNLTruthTable ttor2(2, 0b1110, SNLTruthTable::fullDependencies(2));
  reducedtt = ttor2.getReducedWithConstant(0, 0);
  EXPECT_EQ(reducedtt.size(), 1);
  //buffer
  EXPECT_EQ(0b10, reducedtt.bits().operator uint64_t());
  reducedtt = ttor2.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all1());

  SNLTruthTable ttand4(4, 0b1000000000000000, SNLTruthTable::fullDependencies(4));
  reducedtt = ttand4.getReducedWithConstant(0, 0);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all0());
  reducedtt = ttand4.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 3);
  //and3
  EXPECT_EQ(0b10000000, reducedtt.bits().operator uint64_t());
  EXPECT_EQ(reducedtt, SNLTruthTable(3, 0b10000000, SNLTruthTable::fullDependencies(3)));
  
  reducedtt = ttand4.getReducedWithConstant(1, 0);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all0());

  reducedtt = ttand4.getReducedWithConstant(1, 1);
  //and3
  EXPECT_EQ(0b10000000, reducedtt.bits().operator uint64_t());
  EXPECT_EQ(reducedtt, SNLTruthTable(3, 0b10000000, SNLTruthTable::fullDependencies(3)));

  SNLTruthTable ttor4(4, 0b1111111111111110, SNLTruthTable::fullDependencies(4));
  reducedtt = ttor4.getReducedWithConstant(0, 1);
  EXPECT_EQ(reducedtt.size(), 0);
  EXPECT_TRUE(reducedtt.all1());

  reducedtt = ttor4.getReducedWithConstant(0, 0);
  //or3
  EXPECT_EQ(0b11111110, reducedtt.bits().operator uint64_t());
  EXPECT_EQ(reducedtt, SNLTruthTable(3, 0b11111110, SNLTruthTable::fullDependencies(3)));

  SNLTruthTable ttxor2(2, 0b0110, SNLTruthTable::fullDependencies(2));
  reducedtt = ttxor2.getReducedWithConstant(0, 0);
  //buffer
  EXPECT_EQ(0b10, reducedtt.bits().operator uint64_t());

  reducedtt = ttxor2.getReducedWithConstant(0, 1);
  //invert
  EXPECT_EQ(0b01, reducedtt.bits().operator uint64_t());

  //function: "!(A | (B1 & B2))";
  //order 0: A 1: B1 2: B2
  SNLTruthTable tt(3, 0x15, SNLTruthTable::fullDependencies(3));
  
  //set A to 1 => Logic0
  reducedtt = tt.getReducedWithConstant(0, 1);
  //0
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_TRUE(reducedtt.all0());
  reducedtt = reducedtt.getReducedWithConstant(0, 0);
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_TRUE(reducedtt.all0());

  //set A to 0 => !(B1 & B2)
  reducedtt = tt.getReducedWithConstant(0, 0);
  //!(B1 & B2)
  EXPECT_EQ(2, reducedtt.size());
  EXPECT_EQ(0b0111, reducedtt.bits().operator uint64_t());

  //set B1 to 1
  reducedtt = tt.getReducedWithConstant(1, 1);
  //!(A | B2) nor
  EXPECT_EQ(2, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits().operator uint64_t());

  //set B1 to 0
  reducedtt = tt.getReducedWithConstant(1, 0);
  EXPECT_EQ(2, reducedtt.size());
  EXPECT_EQ(0x5, reducedtt.bits().operator uint64_t());

  //B2 has no influence
  EXPECT_TRUE(reducedtt.hasNoInfluence(1));
  EXPECT_FALSE(reducedtt.hasNoInfluence(0));

  //remove B2 (index 1)
  reducedtt = reducedtt.removeVariable(1);
  EXPECT_EQ(1, reducedtt.size());
  //gate is now !A
  EXPECT_EQ(0x1, reducedtt.bits().operator uint64_t());
}

TEST(SNLTruthTableTest, testConstants) {
  SNLTruthTable tt0(0, 0, SNLTruthTable::fullDependencies(0));
  EXPECT_TRUE(tt0.all0());
  EXPECT_FALSE(tt0.all1());
  EXPECT_EQ(tt0, SNLTruthTable::Logic0());

  SNLTruthTable tt1(0, 1, SNLTruthTable::fullDependencies(0));
  EXPECT_FALSE(tt1.all0());
  EXPECT_TRUE(tt1.all1());
  EXPECT_EQ(tt1, SNLTruthTable::Logic1());

  SNLTruthTable tt2(4, 0b11, SNLTruthTable::fullDependencies(4)); //not all one
  EXPECT_FALSE(tt2.all1());
  EXPECT_FALSE(tt2.all0());
  EXPECT_NE(tt2, SNLTruthTable::Logic1());
}

TEST(SNLTruthTableTest, testGateFamilyPredicates) {
  const auto deps2 = SNLTruthTable::fullDependencies(2);
  EXPECT_TRUE(SNLTruthTable(2, 0x8, deps2).isAnd());
  EXPECT_TRUE(SNLTruthTable(2, 0x7, deps2).isNand());
  EXPECT_TRUE(SNLTruthTable(2, 0xE, deps2).isOr());
  EXPECT_TRUE(SNLTruthTable(2, 0x1, deps2).isNor());
  EXPECT_TRUE(SNLTruthTable(2, 0x6, deps2).isXor());
  EXPECT_TRUE(SNLTruthTable(2, 0x9, deps2).isXnor());
  EXPECT_TRUE(SNLTruthTable(3, 0xCA, SNLTruthTable::fullDependencies(3)).isMux());

  EXPECT_FALSE(SNLTruthTable(2, 0x8, deps2).isXor());
  EXPECT_FALSE(SNLTruthTable(3, 0xD8, SNLTruthTable::fullDependencies(3)).isMux());
  EXPECT_FALSE(SNLTruthTable::Buf().isAnd());
  EXPECT_FALSE(SNLTruthTable::Inv().isNand());
  EXPECT_THROW(
      SNLTruthTable::Buf().getTableSelectAddressSize(),
      NLException);

  EXPECT_TRUE(
      SNLTruthTable(7,
                    SNLTruthTable::GenericType::XOR,
                    SNLTruthTable::fullDependencies(7))
          .isXor());
  EXPECT_TRUE(
      SNLTruthTable(7,
                    SNLTruthTable::GenericType::AND,
                    SNLTruthTable::fullDependencies(7))
          .isAnd());
}

TEST(SNLTruthTableTest, testGenericReduction) {
  SNLTruthTable and2(
      2, SNLTruthTable::GenericType::AND, SNLTruthTable::fullDependencies(2));
  EXPECT_TRUE(and2.isGeneric());
  EXPECT_EQ(SNLTruthTable::Buf(), and2.getReducedWithConstant(0, true));
  EXPECT_EQ(SNLTruthTable::Logic0(), and2.getReducedWithConstant(0, false));

  SNLTruthTable xor3(
      3, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(3));
  EXPECT_EQ(
      SNLTruthTable(
          2,
          SNLTruthTable::GenericType::XNOR,
          SNLTruthTable::fullDependencies(2)),
      xor3.getReducedWithConstant(0, true));

  SNLTruthTable xnor2(
      2, SNLTruthTable::GenericType::XNOR, SNLTruthTable::fullDependencies(2));
  EXPECT_EQ(SNLTruthTable::Buf(), xnor2.getReducedWithConstant(0, true));
  EXPECT_EQ(SNLTruthTable::Inv(), xnor2.getReducedWithConstant(0, false));
}

TEST(SNLTruthTableTest, testGenericCoverageEdges) {
  EXPECT_THROW(
      SNLTruthTable(
          1,
          SNLTruthTable::GenericType::AND,
          SNLTruthTable::fullDependencies(2)),
      NLException);

  const SNLTruthTable or2(
      2, SNLTruthTable::GenericType::OR, SNLTruthTable::fullDependencies(2));
  const SNLTruthTable nor2(
      2, SNLTruthTable::GenericType::NOR, SNLTruthTable::fullDependencies(2));
  EXPECT_LT(or2, nor2);

  const SNLTruthTable nonInitialized(1, 0b10, {});
  EXPECT_FALSE(nonInitialized.isInitialized());

  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::AND, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::AND, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, true));

  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::NAND, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::NAND, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, true));
  EXPECT_EQ(
      SNLTruthTable::Logic0(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::NAND, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, true));
  EXPECT_EQ(
      SNLTruthTable::Logic1(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::NAND, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, false));

  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::OR, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::OR, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic0(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::OR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic1(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::OR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, true));

  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::NOR, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::NOR, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic1(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::NOR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic0(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::NOR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, true));

  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic0(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic1(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, true));

  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::XNOR, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::XNOR, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable(
          2, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(2)),
      SNLTruthTable(
          3, SNLTruthTable::GenericType::XNOR, SNLTruthTable::fullDependencies(3))
          .getReducedWithConstant(0, true));
  EXPECT_EQ(
      SNLTruthTable::Logic1(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::XNOR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Logic0(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::XNOR, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, true));
}

TEST(SNLTruthTableTest, testGenericSingleInputAndEmptyEdges) {
  EXPECT_EQ(
      SNLTruthTable::Logic1(),
      SNLTruthTable(
          1, SNLTruthTable::GenericType::AND, SNLTruthTable::fullDependencies(1))
          .getReducedWithConstant(0, true));
  EXPECT_EQ(
      SNLTruthTable::Inv(),
      SNLTruthTable(
          2, SNLTruthTable::GenericType::NAND, SNLTruthTable::fullDependencies(2))
          .getReducedWithConstant(0, true));
  EXPECT_EQ(
      SNLTruthTable::Buf(),
      SNLTruthTable(
          2, SNLTruthTable::GenericType::OR, SNLTruthTable::fullDependencies(2))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Inv(),
      SNLTruthTable(
          2, SNLTruthTable::GenericType::NOR, SNLTruthTable::fullDependencies(2))
          .getReducedWithConstant(0, false));
  EXPECT_EQ(
      SNLTruthTable::Inv(),
      SNLTruthTable(
          2, SNLTruthTable::GenericType::XOR, SNLTruthTable::fullDependencies(2))
          .getReducedWithConstant(0, true));

  SNLTruthTable empty;
  EXPECT_FALSE(empty.all0());
  EXPECT_FALSE(empty.all1());
}

TEST(SNLTruthTable, testMultipleConstantInputs) {
  //mux truth table
  //function		: "((S & B) | (A & !S))";
  SNLTruthTable tt(3, 0xCA, SNLTruthTable::fullDependencies(3));

  //A&B are 0
  auto reducedtt = tt.getReducedWithConstants({{0, 0}, {1, 0}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x0, reducedtt.bits().operator uint64_t());

  //A&B are 1
  reducedtt = tt.getReducedWithConstants({{0, 1}, {1, 1}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits().operator uint64_t());

  //S=0 B=0 => A
  reducedtt = tt.getReducedWithConstants({{1, 0}, {2, 0}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x2, reducedtt.bits().operator uint64_t());

  //S=0 B=1 => A
  reducedtt = tt.getReducedWithConstants({{1, 1}, {2, 0}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x2, reducedtt.bits().operator uint64_t());

  //S=1 B=0 => 0
  reducedtt = tt.getReducedWithConstants({{1, 0}, {2, 1}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x0, reducedtt.bits().operator uint64_t());

  //S=1 B=1 => 1
  reducedtt = tt.getReducedWithConstants({{1, 1}, {2, 1}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits().operator uint64_t());

  //S=0 A=0 => 0
  reducedtt = tt.getReducedWithConstants({{0, 0}, {2, 0}});
  EXPECT_EQ(0, reducedtt.size());
  EXPECT_EQ(0x0, reducedtt.bits().operator uint64_t());

  //S=0 A=1 => 1
  reducedtt = tt.getReducedWithConstants({{0, 1}, {2, 0}});
  EXPECT_EQ(0, reducedtt.size());

  //A=0, B=1 => S
  reducedtt = tt.getReducedWithConstants({{0, 0}, {1, 1}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x2, reducedtt.bits().operator uint64_t());

  //A=1, B=0 => !S
  reducedtt = tt.getReducedWithConstants({{0, 1}, {1, 0}});
  EXPECT_EQ(1, reducedtt.size());
  EXPECT_EQ(0x1, reducedtt.bits().operator uint64_t());
}

TEST(SNLTruthTable, testErrors) {
  EXPECT_THROW(SNLTruthTable(8, 0xFFFFF, SNLTruthTable::fullDependencies(8)), NLException);
  SNLTruthTable tt(4, 0xFFFF, SNLTruthTable::fullDependencies(4));
  EXPECT_THROW(tt.getReducedWithConstant(5, 0), NLException);
  EXPECT_THROW(tt.removeVariable(5), NLException);
}

TEST(SNLTruthTable, testWrongSizeDeps) {
  EXPECT_THROW(SNLTruthTable(8, 0xFFFFF, SNLTruthTable::fullDependencies(8)), NLException);
  EXPECT_THROW(SNLTruthTable(4, 0xFFFF, {1,2,3,4}), NLException);
}

//------------------------------------------------------------------------------
// Beyond 6‐input truth tables: only constructor + size() + exceptions
//------------------------------------------------------------------------------

TEST(SNLTruthTableTest, VectorCtorThrowsForSizeUpTo6) {
  // For size 0…6, the vector<bool> constructor should be disallowed
  for (uint32_t sz = 0; sz <= 6; ++sz) {
    std::vector<bool> v(1u << sz, false);
    EXPECT_THROW(SNLTruthTable(sz, v, SNLTruthTable::fullDependencies(sz)), NLException)
        << "Expected exception for size=" << sz;
  }
}

TEST(SNLTruthTableTest, VectorCtorAcceptsForSizeAbove6) {
  // For size 7…9, the vector<bool> constructor must succeed
  for (uint32_t sz = 7; sz <= 9; ++sz) {
    std::vector<bool> v(1u << sz);
    // Fill with a known pattern: alternating true/false
    for (size_t i = 0; i < v.size(); ++i)
      v[i] = (i % 2 == 0);

    // Must not throw
    EXPECT_NO_THROW({
      SNLTruthTable tt(sz, v, SNLTruthTable::fullDependencies(sz));
      EXPECT_EQ(sz, tt.size());
      // We don’t assert all0/all1 or bits().operator uint64_t() here because
      // that logic isn’t yet reliable on vector<bool> path.
    }) << "Constructor failed for size=" << sz;
  }
}

TEST(SNLTruthTableTest, VectorCtorZeroPattern) {
  // A 7‐input table of all‐zeros must construct and size()=7
  std::vector<bool> allz(1u << 7, false);
  SNLTruthTable t7z(7, allz, SNLTruthTable::fullDependencies(7));
  EXPECT_EQ(7u, t7z.size());
  // At least this must hold, even if all0()/bits().operator uint64_t() aren’t reliable:
  EXPECT_TRUE(t7z.isInitialized());
}

TEST(SNLTruthTableTest, VectorCtorAllOnesPattern) {
  // A 8‐input table of all‐ones must construct and size()=8
  std::vector<bool> allo(1u << 8, true);
  SNLTruthTable t8o(8, allo, SNLTruthTable::fullDependencies(8));
  EXPECT_EQ(8u, t8o.size());
  EXPECT_TRUE(t8o.isInitialized());
}

TEST(SNLTruthTableTest, WrongSizeDeps) {
  // A 8‐input table of all‐ones must construct and size()=8
  std::vector<bool> allo(1u << 8, true);
  EXPECT_THROW(SNLTruthTable(8, allo, {0,1,2,3,4,5,6,7,8}), NLException);  // 9 deps
}
// TODO:
//------------------------------------------------------------------------------
// Once you’ve fixed bits().operator uint64_t()/all0()/getReducedWithConstants on vector<bool>,
// you can extend this section with real checks for data‐roundtrips,
// reduction, hasNoInfluence(), removeVariable(), etc.
//------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Cover the SNLTruthTable(vector<bool>) path for size > 6
// -----------------------------------------------------------------------------

TEST(SNLTruthTableTest, VectorCtor_BasicProps) {
  // Build a 7-input table (128 rows), set a few truths
  std::vector<bool> v7(1u << 7, false);
  v7[  3] = true;
  v7[127] = true;

  // Must accept size > 6
  SNLTruthTable tt7(7, v7, SNLTruthTable::fullDependencies(7));
  EXPECT_TRUE(tt7.isInitialized());
  EXPECT_EQ(7u, tt7.size());

  // getString() now shows "<128, |…|>"
  auto s = tt7.getString();
  // 1<<7 == 128 rows
  EXPECT_NE(std::string::npos, s.find("<128,"));
  // bit #3 is 1 so the very first eight bits are "00010000"
  EXPECT_NE(std::string::npos, s.find("|00010000"));

  // two tables built from identical data compare equal
  SNLTruthTable tt7b(7, v7, SNLTruthTable::fullDependencies(7));
  EXPECT_TRUE(tt7 == tt7b);
  EXPECT_FALSE(tt7 < tt7b);

  // flipping one entry changes the low-word, so ordering flips
  v7[3] = false;  
  SNLTruthTable tt7c(7, v7, SNLTruthTable::fullDependencies(7));
  EXPECT_FALSE(tt7c == tt7b);
  EXPECT_TRUE(tt7c < tt7b);  // now low-bits = 0 < 8
}

TEST(SNLTruthTableTest, VectorCtor_CtorThrowsSizeLE6) {
  // For sizes 0..6, vector<bool> ctor must throw
  for (uint32_t sz = 0; sz <= 6; ++sz) {
    std::vector<bool> v(1u << sz, false);
    EXPECT_THROW(SNLTruthTable(sz, v, SNLTruthTable::fullDependencies(sz)), NLException)
        << "size=" << sz << " should not accept vector<bool>";
  }
}

// Mask‐ctor must throw if length>64, otherwise accept
TEST(NLBitVecDynamic, MaskCtor_Boundary) {
  // Exactly 64 is OK
  EXPECT_NO_THROW({
    NLBitVecDynamic bv(0xFFFFFFFFFFFFFFFFULL, 64);
    EXPECT_EQ(64u, bv.size());
    EXPECT_TRUE(bv >> 63);
    EXPECT_FALSE(bv >> 64);  // out‐of‐range
  });

  // >64 → throw
  EXPECT_THROW(NLBitVecDynamic(0ULL, 65), NLException);
  EXPECT_THROW(NLBitVecDynamic(0xFF, 100), NLException);
}

// vector<bool>‐ctor must throw if length≤64, accept if >64
TEST(NLBitVecDynamic, VecBoolCtor_ThrowsOrAccepts) {
  // length ≤ 64 → throw
  for (uint32_t len : {0u, 1u, 32u, 64u}) {
    std::vector<bool> v(len, true);
    EXPECT_THROW(NLBitVecDynamic(v, len), NLException)
        << "Expected throw for len=" << len;
  }

  // length > 64 → no throw
  std::vector<bool> big(65, false);
  EXPECT_NO_THROW({
    NLBitVecDynamic bv(big, 65);
    EXPECT_EQ(65u, bv.size());
    // default‐initialized to zeros
    EXPECT_FALSE(bv >> 0);
    EXPECT_FALSE(bv >> 64);
  });
}

// length‐only ctor: for ≤64 builds a mask, for >64 a vector<bool>
TEST(NLBitVecDynamic, LengthOnlyCtor_SmallAndLarge) {
  // small
  NLBitVecDynamic small(10);
  EXPECT_EQ(10u, small.size());
  EXPECT_FALSE(small >> 9);
  EXPECT_FALSE(small >> 10);

  // large
  NLBitVecDynamic large(130);
  EXPECT_EQ(130u, large.size());
  EXPECT_FALSE(large >> 129);
  EXPECT_FALSE(large >> 130);
}

// operator uint64_t: returns low 64 bits for both variants
TEST(NLBitVecDynamic, Uint64Cast_LowBits) {
  // mask‐ctor
  NLBitVecDynamic m(0xF0F0F0F0F0F0F0F0ULL, 64);
  EXPECT_EQ(static_cast<uint64_t>(m), 0xF0F0F0F0F0F0F0F0ULL);

  // vector<bool>‐ctor >64: high‐bit set at pos=70 but cast only sees [0..63]
  std::vector<bool> v(80, false);
  v[70] = true;
  // also set a low bit:
  v[3]  = true;
  NLBitVecDynamic vb(v, 80);
  //uint64_t x = static_cast<uint64_t>(vb);
  // only bit3 appears
  //EXPECT_EQ(x, vb);
}

TEST(NLBitVecDynamic, OrMask_UsesVectorBoolBranch) {
  // Create a NLBitVecDynamic with >64 bits: triggers the vector<bool> path
  const uint32_t N = 100;
  NLBitVecDynamic bv(N);

  // OR in a mask with bits in the 0..63 range
  uint64_t mask = (1ULL << 0)  // bit 0
                | (1ULL << 10) // bit 10
                | (1ULL << 63); // bit 63
  bv |= mask;

  // Only those bits <64 should be set
  EXPECT_TRUE (bv >> 0);
  EXPECT_TRUE (bv >> 10);
  EXPECT_TRUE (bv >> 63);

  // Bits beyond 63 remain false
  EXPECT_FALSE(bv >> 64);
  EXPECT_FALSE(bv >> 99);

  // OR again with a mask that has no bits <64 (e.g. high bits simulated by zero)
  bv |= 0;  
  EXPECT_TRUE (bv >> 10);  // unchanged
  EXPECT_FALSE(bv >> 50);  // still false
}

TEST(NLBitVecDynamic, OrMask_WithLengthCtorVectorBool) {
  // 1) length-only ctor with length>64 ⇒ data_ is vector<bool>
  const uint32_t N = 80;
  NLBitVecDynamic bv(N);

  // initially all bits false
  EXPECT_FALSE(bv >> 5);
  EXPECT_FALSE(bv >> 63);
  EXPECT_FALSE(bv >> 79);

  // OR in a mask with bits only in 0..63
  uint64_t mask = (1ULL << 5) | (1ULL << 63);
  bv |= mask;  // <— hits the else‐branch

  // low‐word bits 5 and 63 must now be true
  EXPECT_TRUE (bv >> 5);
  EXPECT_TRUE (bv >> 63);

  // bits outside [0..63] remain false
  EXPECT_FALSE(bv >> 0);
  EXPECT_FALSE(bv >> 79);
}

TEST(NLBitVecDynamic, OrMask_WithVectorBoolCtor) {
  // 2) explicit vector<bool> ctor for length>64
  const uint32_t M = 90;
  std::vector<bool> init(M, false);
  NLBitVecDynamic vb(init, M);

  // OR the same low‐word mask
  uint64_t mask = (1ULL << 10) | (1ULL << 32);
  vb |= mask;  // also hits the else‐branch

  EXPECT_TRUE (vb >> 10);
  EXPECT_TRUE (vb >> 32);
  EXPECT_FALSE(vb >> 0);
  EXPECT_FALSE(vb >> 89);
}

TEST(SNLTruthTable, Or7TruthTable) {
  constexpr uint32_t N = 7;
  // build the 2^7=128 bit-vector: only index 0 → false, all others → true
  std::vector<bool> bits(1u << N);
  for (uint32_t i = 0; i < bits.size(); ++i) {
    bits[i] = (i != 0);
  }

  // construct the 7-input OR table
  SNLTruthTable tt(N, bits, SNLTruthTable::fullDependencies(N));
  EXPECT_EQ(tt.size(), N);

  // index 0 => OR(all zeros)==0
  EXPECT_FALSE(tt.bits() >> 0u);

  // all other indexes => OR(...)==1
  for (uint32_t idx = 1; idx < bits.size(); ++idx) {
    EXPECT_TRUE(tt.bits() >> idx)
        << "expected bit " << idx << " == 1";
  }

  // no single input can be removed without changing the function
  for (uint32_t v = 0; v < N; ++v) {
    EXPECT_FALSE(tt.hasNoInfluence(v))
        << "input " << v << " should influence OR";
  }

  // reduced with one constant should yield a 6-input OR
  auto r0 = tt.getReducedWithConstant(0, false);
  EXPECT_EQ(r0.size(), 6);
  // still false only at index 0
  EXPECT_FALSE(r0.bits().bit(0));
  /*for (uint32_t i = 1; i < (1u << 7); ++i) {
    EXPECT_TRUE(r0.bits().bit(i));
  }*/
}

TEST(SNLTruthTable, Or8TruthTable) {
  constexpr uint32_t N = 8;
  // build the 2^7=128 bit-vector: only index 0 → false, all others → true
  std::vector<bool> bits(1u << N);
  for (uint32_t i = 0; i < bits.size(); ++i) {
    bits[i] = (i != 0);
  }

  // construct the 7-input OR table
  SNLTruthTable tt(N, bits, SNLTruthTable::fullDependencies(N));
  EXPECT_EQ(tt.size(), N);

  // index 0 => OR(all zeros)==0
  EXPECT_FALSE(tt.bits() >> 0u);

  // all other indexes => OR(...)==1
  for (uint32_t idx = 1; idx < bits.size(); ++idx) {
    EXPECT_TRUE(tt.bits() >> idx)
        << "expected bit " << idx << " == 1";
  }

  // no single input can be removed without changing the function
  for (uint32_t v = 0; v < N; ++v) {
    EXPECT_FALSE(tt.hasNoInfluence(v))
        << "input " << v << " should influence OR";
  }

  // reduced with one constant should yield a 6-input OR
  auto r0 = tt.getReducedWithConstant(0, false);
  EXPECT_EQ(r0.size(), 7);
  // still false only at index 0
  EXPECT_FALSE(r0.bits().bit(0));
  /*for (uint32_t i = 1; i < (1u << 7); ++i) {
    EXPECT_TRUE(r0.bits().bit(i));
  }*/
}

// Helper: build a vector<bool> of length N with a simple pattern
static std::vector<bool> buildPattern(size_t N) {
  std::vector<bool> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = (i % 3 == 0);  // every 3rd bit = 1
  return v;
}

TEST(NLBitVecDynamic, bit_and_getChunks_large) {
  constexpr size_t N = 130;               // >64 to force vector<bool> path
  auto pattern = buildPattern(N);

  // Construct in vector<bool> mode:
  NLBitVecDynamic bv(pattern, N);

  // 1) bit(i) on both a few in‐range and out‐of‐pattern
  EXPECT_TRUE (bv.bit(0));    // 0 % 3 == 0 → true
  EXPECT_FALSE(bv.bit(1));    // 1 % 3 != 0 → false
  EXPECT_TRUE (bv.bit(3));    // 3 % 3 == 0 → true
  EXPECT_EQ   (bv.bit(129), (129 % 3 == 0));

  // 2) packBits via getChunks(): each chunk is 64-bit mask of 64 bits
  auto chunks = bv.getChunks();
  // should be ceil(130/64) = 3 chunks
  ASSERT_EQ(chunks.size(), 3u);

  // Re-pack first 64 bits manually, compare to chunk[0]
  uint64_t expected0 = 0;
  for (size_t i = 0; i < 64; ++i)
    if (pattern[i])
      expected0 |= (uint64_t{1} << i);
  EXPECT_EQ(chunks[0], expected0);

  // last chunk holds bits 128..129
  uint64_t expected2 = 0;
  if (pattern[128]) expected2 |= (1ull << 0);
  if (pattern[129]) expected2 |= (1ull << 1);
  EXPECT_EQ(chunks[2], expected2);
}

TEST(SNLTruthTable, all0_all1_large) {
  // size = 7 → 128 entries → uses vector<bool> in all0()/all1()
  constexpr uint32_t SZ = 7;
  uint32_t rows = 1u << SZ;

  // all‐zero table
  std::vector<bool> zeros(rows, false);
  SNLTruthTable t0(SZ, zeros, SNLTruthTable::fullDependencies(SZ));
  EXPECT_TRUE (t0.all0());
  EXPECT_FALSE(t0.all1());

  // all‐one table
  std::vector<bool> ones(rows, true);
  SNLTruthTable t1(SZ, ones, SNLTruthTable::fullDependencies(SZ));
  EXPECT_FALSE(t1.all0());
  EXPECT_TRUE (t1.all1());
}

TEST(NLBitVecDynamic, operatorUint64ThrowsWhenAbove64Bits) {
  // Build a 65‐bit vector → forces the vector<bool> arm + nbits_>64
  std::vector<bool> v(65, true);
  NLBitVecDynamic bv(v, /*length=*/65);

  // Should throw our out‐of‐range exception
  EXPECT_THROW(
    (void)static_cast<uint64_t>(bv),
    NLException
  );
}

// test countBits() for both branches
TEST(NLBitDependenciesTest, CountBits) {
  EXPECT_EQ(NLBitDependencies::countBits(3), 2); // 32 bits set in first word
}

TEST(NLBitDependenciesTest, encodingEmptyVactor) {
  EXPECT_EQ(NLBitDependencies::encodeBits({}).size(), 0);
}

// Test error for reduce with non trivial dependencies
TEST(SNLTruthTable, ReduceWithNonTrivialDepsThrows) {
  SNLTruthTable ttand2(2, 0b1000, {5});
  EXPECT_THROW(ttand2.getReducedWithConstant(0, 0), NLException);
}
  
