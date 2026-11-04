# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(sources
  TGraphTestMain.cpp
  TGraphTests.cpp
  TSNLGraphConstructorTests.cpp
)

ADD_EXECUTABLE(tgraphTests ${sources})

TARGET_LINK_LIBRARIES(tgraphTests naja_nl naja_dnl naja_tgraph naja_snl_systemverilog TBB::tbb gmock gtest)

GTEST_DISCOVER_TESTS(tgraphTests)
