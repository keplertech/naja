// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include <chrono>
#include <filesystem>
#include <fstream>

#include "SNLNet.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::NL;

TEST(SNLVRLConstructorTest0, test0) {
  EXPECT_EQ(SNLNet::Type::Standard,
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Wire));
  EXPECT_EQ(SNLNet::Type::Supply0,
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Supply0));
  EXPECT_EQ(SNLNet::Type::Supply1,
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Supply1));
  EXPECT_THROW(
    SNLVRLConstructor::VRLTypeToSNLType(naja::verilog::Net::Type::Unknown),
    SNLVRLConstructorException
  );
}

TEST(SNLVRLConstructorTest0, test1) {
  EXPECT_EQ(SNLTerm::Direction::Input,
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::Input));
  EXPECT_EQ(SNLTerm::Direction::Output,
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::Output));
  EXPECT_EQ(SNLTerm::Direction::InOut,
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::InOut));
  EXPECT_THROW(
    SNLVRLConstructor::VRLDirectionToSNLDirection(naja::verilog::Port::Direction::Unknown),
    SNLVRLConstructorException
  );
}


TEST(LoaderProgressTests, SNLVRLConstructor) {
  const auto path = std::filesystem::temp_directory_path() /
    ("naja-progress-SNLVRLConstructor-" + std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count()));
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      if (NLUniverse::get()) NLUniverse::get()->destroy();
      std::error_code error;
      std::filesystem::remove(path, error);
    }
  } cleanup{path};
  { std::ofstream file(path); file << R"source(module top(input a, output y); assign y=a; endmodule)source"; }
  const auto destination = [] {
    auto* db = NLDB::create(NLUniverse::create());
    return NLLibrary::create(db, NLLibrary::Type::Standard, NLName("DESIGN"));
  };
  {
    SNLVRLConstructor constructor(destination());
    constructor.construct(path);
  }
  const auto baseline = NLUniverse::get()->getContentHash();
  NLUniverse::get()->destroy();
  std::vector<std::string> stages;
  {
    SNLVRLConstructor constructor(destination());
    constructor.setProgressCallback([&](const naja::NajaProgress& event) {
      if (stages.empty() || stages.back() != event.stage) stages.emplace_back(event.stage);
      EXPECT_TRUE(event.total == 0 || event.current <= event.total);
      return true;
    });
    constructor.construct(path);
  }
  EXPECT_EQ(baseline, NLUniverse::get()->getContentHash());
  EXPECT_EQ((std::vector<std::string>{"parse", "first_pass", "second_pass", "done"}), stages);
  NLUniverse::get()->destroy();
  {
    SNLVRLConstructor constructor(destination());
    constructor.setProgressCallback([](const naja::NajaProgress& event) {
      return !(event.stage == "second_pass" && event.current > 0);
    });
    EXPECT_THROW(constructor.construct(path), naja::NajaCancelled);
  }
}
