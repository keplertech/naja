// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>

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

using namespace naja::NL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

class SNLVRLDumperTestTermNets: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      NLDB* db = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
      top_ = SNLDesign::create(library, NLName("top"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  top_;
};

TEST_F(SNLVRLDumperTestTermNets, testFeedthru0) {
  ASSERT_TRUE(top_);
  auto inScalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("in"));
  auto outScalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("out"));

  auto feedthru = SNLScalarNet::create(top_, NLName("feedtru"));
  inScalar->setNet(feedthru);
  outScalar->setNet(feedthru);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testFeedthru0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / "top.v";

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testFeedthru0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestTermNets, testFeedthru1) {
  ASSERT_TRUE(top_);
  auto inScalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("in"));
  auto outScalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("out"));

  auto feedthru = SNLBusNet::create(top_, 5, 5, NLName("feedtru"));
  auto bit = feedthru->getBit(5);
  ASSERT_TRUE(bit);
  inScalar->setNet(bit);
  outScalar->setNet(bit);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testFeedthru1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / "top.v";

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testFeedthru1" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}
 
TEST_F(SNLVRLDumperTestTermNets, testFeedthru2) {
  ASSERT_TRUE(top_);
  auto inBus = SNLBusTerm::create(top_, SNLTerm::Direction::Input, -4, -4, NLName("in"));
  auto outBus = SNLBusTerm::create(top_, SNLTerm::Direction::Output, 6, 6, NLName("out"));

  auto feedthru = SNLScalarNet::create(top_, NLName("feedtru"));
  auto inBusBit = inBus->getBit(-4);
  ASSERT_TRUE(inBusBit);
  auto outBusBit = outBus->getBit(6);
  ASSERT_TRUE(outBusBit);
  inBusBit->setNet(feedthru);
  outBusBit->setNet(feedthru);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testFeedthru2";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / "top.v";

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testFeedthru2" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestTermNets, testFeedthru3) {
  ASSERT_TRUE(top_);
  auto inBus = SNLBusTerm::create(top_, SNLTerm::Direction::Input, -4, -4, NLName("in"));
  auto outBus = SNLBusTerm::create(top_, SNLTerm::Direction::Output, 6, 6, NLName("out"));

  auto feedthru = SNLBusNet::create(top_, 5, 5, NLName("feedtru"));
  auto bitNet = feedthru->getBit(5);
  ASSERT_TRUE(bitNet);

  auto inBusBit = inBus->getBit(-4);
  ASSERT_TRUE(inBusBit);
  auto outBusBit = outBus->getBit(6);
  ASSERT_TRUE(outBusBit);
  inBusBit->setNet(bitNet);
  outBusBit->setNet(bitNet);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testFeedthru3";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / "top.v";

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testFeedthru3" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestTermNets, testError1) {
  ASSERT_TRUE(top_);
  auto inScalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("in"));
  auto outScalar = SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("out"));

  //bus sharing same name than scalar input "in"
  auto feedthru = SNLBusNet::create(top_, 5, 5, NLName("in"));
  auto bit = feedthru->getBit(5);
  ASSERT_TRUE(bit);
  inScalar->setNet(bit);
  outScalar->setNet(bit);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testError1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestTermNets, testError2) {
  ASSERT_TRUE(top_);
  auto inBus = SNLBusTerm::create(top_, SNLTerm::Direction::Input, -4, -4, NLName("in"));
  auto outBus = SNLBusTerm::create(top_, SNLTerm::Direction::Output, 6, 6, NLName("out"));

  //bus term and scalar net sharing same name
  auto feedthru = SNLScalarNet::create(top_, NLName("in"));
  auto inBusBit = inBus->getBit(-4);
  ASSERT_TRUE(inBusBit);
  auto outBusBit = outBus->getBit(6);
  ASSERT_TRUE(outBusBit);
  inBusBit->setNet(feedthru);
  outBusBit->setNet(feedthru);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testError2";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
} 

TEST_F(SNLVRLDumperTestTermNets, testError3) {
  ASSERT_TRUE(top_);
  auto inBus = SNLBusTerm::create(top_, SNLTerm::Direction::Input, -4, -4, NLName("in"));
  auto outBus = SNLBusTerm::create(top_, SNLTerm::Direction::Output, 6, 6, NLName("out"));

  //same name but not same bit
  auto feedthru = SNLBusNet::create(top_, 5, 5, NLName("in"));
  auto bitNet = feedthru->getBit(5);
  ASSERT_TRUE(bitNet);

  auto inBusBit = inBus->getBit(-4);
  ASSERT_TRUE(inBusBit);
  auto outBusBit = outBus->getBit(6);
  ASSERT_TRUE(outBusBit);
  inBusBit->setNet(bitNet);
  outBusBit->setNet(bitNet);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testError3";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
} 

TEST_F(SNLVRLDumperTestTermNets, testError4) {
  ASSERT_TRUE(top_);
  auto term = SNLScalarTerm::create(top_, SNLTerm::Direction::InOut, NLName("term"));
  auto net = SNLScalarNet::create(top_, NLName("net"));
  term->setNet(net);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testError4";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
} 

TEST_F(SNLVRLDumperTestTermNets, anonymousPortConnectionsUseEmittedNames) {
  auto* bus = SNLBusNet::create(top_, 1, 0);
  auto* scalar = SNLScalarNet::create(top_);
  SNLBusTerm::create(top_, SNLTerm::Direction::Input, 1, 0, NLName("input_word"))->setNet(bus);
  SNLBusTerm::create(top_, SNLTerm::Direction::Output, 1, 0, NLName("output_word"))->setNet(bus);
  SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("bit_out"))->setNet(bus->getBit(0));
  SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("input.flag"))->setNet(scalar);
  SNLBusTerm::create(top_, SNLTerm::Direction::Output, 0, 0, NLName("scalar_out"))->getBit(0)->setNet(scalar);
  SNLScalarTerm::create(top_, SNLTerm::Direction::Output, NLName("flag_out"))->setNet(scalar);
  SNLVRLDumper dumper;
  std::ostringstream output;
  dumper.dumpDesign(top_, output);
  const auto text = output.str();
  const auto busName = "net_" + std::to_string(bus->getID());
  const auto scalarName = "net_" + std::to_string(scalar->getID());
  EXPECT_EQ(std::string::npos, text.find("<net:"));
  EXPECT_NE(std::string::npos, text.find("assign " + busName + "[1] = input_word[1];"));
  EXPECT_NE(std::string::npos, text.find("assign output_word[0] = " + busName + "[0];"));
  EXPECT_NE(std::string::npos, text.find("assign bit_out = " + busName + "[0];"));
  EXPECT_NE(std::string::npos, text.find("assign " + scalarName + " = \\input.flag ;"));
  EXPECT_NE(std::string::npos, text.find("assign scalar_out[0] = " + scalarName + ";"));
  EXPECT_NE(std::string::npos, text.find("assign flag_out = " + scalarName + ";"));
}

TEST_F(SNLVRLDumperTestTermNets, mixedBusConstantsHaveExactlyOneDriver) {
  // Exercise the representation independently of which HDL frontend made it.
  for (bool ascending: {false, true}) {
    for (bool outputPort: {false, true}) {
      const auto name = std::string(ascending ? "up" : "down") +
        (outputPort ? "_port" : "_internal");
      auto bus = SNLBusNet::create(top_, ascending ? -3 : 5,
        ascending ? 5 : -3, NLName(name));
      if (outputPort) {
        SNLBusTerm::create(top_, SNLTerm::Direction::Output,
          bus->getMSB(), bus->getLSB(), NLName(name))->setNet(bus);
      }
      // Adjacent same-value constants, adjacent differing values, isolated
      // bits and nonconstant gaps, including negative indices.
      for (auto index: {-2, -1, 2, 4}) {
        bus->getBit(index)->setType(SNLNet::Type::Assign0);
      }
      for (auto index: {0, 3}) {
        bus->getBit(index)->setType(SNLNet::Type::Assign1);
      }
      auto alias = SNLBusTerm::create(top_, SNLTerm::Direction::Output,
        bus->getMSB(), bus->getLSB(), NLName(name + "_alias"));
      alias->setNet(bus);
    }
  }
  SNLVRLDumper dumper;
  std::ostringstream output;
  dumper.dumpDesign(top_, output);
  const auto text = output.str();
  for (auto net: top_->getNets()) {
    auto bus = static_cast<SNLBusNet*>(net);
    auto name = bus->getName().getString();
    std::map<int, std::string> drivers;
    const std::regex assignment("assign " + name +
      R"(\[(-?\d+)(?::(-?\d+))?\] = (\d+)'b([01]+);)");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), assignment);
         it != std::sregex_iterator(); ++it) {
      const auto& match = *it;
      const int first = std::stoi(match[1]);
      const int last = match[2].matched ? std::stoi(match[2]) : first;
      const int step = first <= last ? 1 : -1;
      const auto value = match[4].str();
      ASSERT_EQ(std::abs(last - first) + 1, std::stoi(match[3]));
      ASSERT_EQ(std::stoi(match[3]), value.size());
      for (int bit = first, pos = 0; ; bit += step, ++pos) {
        drivers[bit] += value[pos];
        if (bit == last) break;
      }
    }
    for (auto bit: bus->getBits()) {
      const auto index = static_cast<SNLBusNetBit*>(bit)->getBit();
      EXPECT_EQ(bit->isAssign0() ? "0" : bit->isAssign1() ? "1" : "",
        drivers[index]) << name << "[" << index << "]";
    }
  }
  EXPECT_NE(std::string::npos, text.find("assign down_internal[-1:-2] = 2'b00;"));
  EXPECT_NE(std::string::npos, text.find("assign up_internal[-2:-1] = 2'b00;"));
}

TEST_F(SNLVRLDumperTestTermNets, uniformConstantsAreSubstitutedWithoutWires) {
  for (bool one: {false, true}) {
    auto name = one ? "one" : "zero";
    auto scalar = SNLScalarNet::create(top_, NLName(name));
    scalar->setType(one ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0);
    SNLScalarTerm::create(top_, SNLTerm::Direction::Output,
      NLName(std::string(name) + "_out"))->setNet(scalar);
    auto bus = SNLBusNet::create(top_, 3, 0, NLName(std::string(name) + "_bus"));
    bus->setType(scalar->getType());
    SNLBusTerm::create(top_, SNLTerm::Direction::Output, 3, 0,
      NLName(std::string(name) + "_word"))->setNet(bus);
  }
  SNLVRLDumper dumper;
  std::ostringstream output;
  dumper.dumpDesign(top_, output);
  const auto text = output.str();
  EXPECT_EQ(std::string::npos, text.find("wire "));
  EXPECT_NE(std::string::npos, text.find("assign zero_out = 1'b0;"));
  EXPECT_NE(std::string::npos, text.find("assign one_out = 1'b1;"));
  for (int bit = 0; bit < 4; ++bit) {
    EXPECT_NE(std::string::npos, text.find("assign zero_word[" + std::to_string(bit) + "] = 1'b0;"));
    EXPECT_NE(std::string::npos, text.find("assign one_word[" + std::to_string(bit) + "] = 1'b1;"));
  }
}
