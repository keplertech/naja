// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <cstdlib>
#include <thread>

#include "NajaPerf.h"
#include "NajaProgress.h"
using namespace naja;

#ifndef NAJA_CORE_TESTS_PATH
#define NAJA_CORE_TESTS_PATH "Undefined"
#endif

namespace {
void setEnvVar(const char* name, const std::string& value) {
#ifdef _WIN32
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void unsetEnvVar(const char* name) {
#ifdef _WIN32
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}
}  // namespace

TEST(NajaPerfTests, getLogPathFromEnvDisablesWhenUnset) {
  unsetEnvVar("NAJA_PERF_TEST_PATH");
  EXPECT_EQ(
    std::nullopt,
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log"));
}

TEST(NajaPerfTests, getLogPathFromEnvUsesEnvValue) {
  setEnvVar("NAJA_PERF_TEST_PATH", "custom_perf.log");
  auto logPath =
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log");
  ASSERT_TRUE(logPath);
  EXPECT_EQ(*logPath, std::filesystem::path("custom_perf.log"));
  unsetEnvVar("NAJA_PERF_TEST_PATH");
}

TEST(NajaPerfTests, getLogPathFromEnvDisablesForEmptyValue) {
  setEnvVar("NAJA_PERF_TEST_PATH", "");
  EXPECT_EQ(
    std::nullopt,
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log"));
  unsetEnvVar("NAJA_PERF_TEST_PATH");
}

TEST(NajaPerfTests, getLogPathFromEnvUsesDefaultForLegacyEnable) {
  setEnvVar("NAJA_PERF_TEST_PATH", "1");
  auto logPath =
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log");
  ASSERT_TRUE(logPath);
  EXPECT_EQ(*logPath, std::filesystem::path("naja_perf.log"));
  unsetEnvVar("NAJA_PERF_TEST_PATH");
}

TEST(NajaPerfTests, getLogPathFromEnvUsesDefaultForNullOrEmptyEnvVarName) {
  EXPECT_EQ(
    NajaPerf::getLogPathFromEnv(nullptr, "naja_perf.log"),
    std::filesystem::path("naja_perf.log"));
  EXPECT_EQ(
    NajaPerf::getLogPathFromEnv("", "naja_perf.log"),
    std::filesystem::path("naja_perf.log"));
}

TEST(NajaPerfTests, test0) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  EXPECT_EQ(nullptr, NajaPerf::create(std::nullopt, "disabled"));
  {
    NajaPerf::Scope disabledScope("disabled_scope");
    EXPECT_EQ(nullptr, NajaPerf::get());
  }

  NajaPerf::create(logPath/"naja_perf_test0.log", "top");
  auto perf = NajaPerf::get();
  EXPECT_EQ(1, perf->getStack().size());
  ASSERT_NE(perf, nullptr);
  {
    NajaPerf::Scope scope("test0");
    EXPECT_EQ(2, perf->getStack().size());
    {
      NajaPerf::Scope scope("test1");
      EXPECT_EQ(3, perf->getStack().size());
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      {
        NajaPerf::Scope scope("test2");
        EXPECT_EQ(4, perf->getStack().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      EXPECT_EQ(3, perf->getStack().size());
      {
        NajaPerf::Scope scope("test3");
        EXPECT_EQ(4, perf->getStack().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      EXPECT_EQ(3, perf->getStack().size());
    }
    EXPECT_EQ(2, perf->getStack().size());
  }
  EXPECT_EQ(1, perf->getStack().size());
}


TEST(NajaProgressTests, throttlesAndChecksCancellationOnLoadingThread) {
  NajaProgressReporter reporter;
  unsigned calls = 0;
  const auto thread = std::this_thread::get_id();
  reporter.setCallback([&](const NajaProgress& event) {
    EXPECT_EQ(thread, std::this_thread::get_id());
    EXPECT_EQ("construct", event.stage);
    EXPECT_LE(event.current, event.total);
    ++calls;
    return calls < 3;
  });
  reporter.begin("construct", "synthetic", 100);
  EXPECT_EQ(1, calls);
  std::this_thread::sleep_for(std::chrono::milliseconds(55));
  for (unsigned i = 0; i < 63; ++i) reporter.advance();
  EXPECT_EQ(1, calls);
  reporter.advance();
  EXPECT_EQ(2, calls);
  EXPECT_THROW(reporter.finish(), NajaCancelled);
  reporter.setCallback({});
  EXPECT_NO_THROW(reporter.begin("parse"));
}
