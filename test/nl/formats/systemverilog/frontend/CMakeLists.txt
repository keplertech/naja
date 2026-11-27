# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(snl_sv_constructor_tests
    SNLSVConstructorTestSimple.cpp
    SNLSVConstructorTestMemoryInference.cpp
    SNLSVConstructorTestGenerateMemory.cpp
    SNLSVConstructorTestAlwaysComb.cpp
    SNLSVConstructorTestSequentialOverlap.cpp
    SNLSVConstructorTestElab.cpp
    SNLSVIntentTest.cpp
)
add_executable(snlSVConstructorTests ${snl_sv_constructor_tests})
target_compile_definitions(snlSVConstructorTests PRIVATE
  SNL_SV_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks"
  SNL_SV_DUMPER_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}")
if(ENABLE_SV_CONSTRUCTOR_PERF_REPORT)
  target_compile_definitions(
    snlSVConstructorTests
    PRIVATE NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT)
endif()
target_link_libraries(snlSVConstructorTests
  naja_snl_systemverilog_test
  naja_snl_verilog
  naja_dnl
  naja_metrics
  gmock
  gtest_main)
gtest_discover_tests(snlSVConstructorTests
  DISCOVERY_TIMEOUT 30
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/nl/python/naja_wrapping)
