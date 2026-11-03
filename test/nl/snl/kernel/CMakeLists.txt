# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(snl_tests
  NLContentHashTest.cpp
  NLIDTest.cpp
  NLNameTest.cpp
  NLUniverseTest.cpp NLDBTest.cpp NLLibraryTest.cpp
  NLDB0Test.cpp
  SNLDesignTest.cpp SNLDesignCloneTest.cpp
  SNLBundleTermTest.cpp
  SNLTermTest.cpp
  SNLInstanceTest0.cpp SNLInstanceTest1.cpp
  SNLInstanceTest2.cpp SNLInstanceTest3.cpp
  SNLInstanceSetModelTest.cpp
  SNLNetTest.cpp
  SNLPathTest0.cpp SNLPathTest1.cpp SNLPathTest2.cpp
  SNLOccurrenceTest.cpp SNLParameterTest.cpp
  SNLAttributesTest.cpp
  SNLRTLInfosTest.cpp
  SNLMergeAssignsTest.cpp
  NLClockTest.cpp
  SNLUtilsTest.cpp
)

SET(snl_tests_with_pyloader
  SNLEquipotentialTest0.cpp
  SNLEquipotentialTest1.cpp
)

ADD_EXECUTABLE(snlTests ${snl_tests})
target_compile_definitions(snlTests PRIVATE
  SNL_BENCHS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchs")
TARGET_LINK_LIBRARIES(snlTests naja_nl gmock gtest_main)
GTEST_DISCOVER_TESTS(snlTests)

ADD_EXECUTABLE(snlDB0InternalTests NLDB0InternalTest.cpp)
TARGET_LINK_LIBRARIES(snlDB0InternalTests naja_nl gmock gtest_main)
GTEST_DISCOVER_TESTS(snlDB0InternalTests)

if(NOT BUILD_NAJA_PYTHON AND NOT BUILD_EMSCRIPTEN)
  ADD_EXECUTABLE(snlTestsWithPyLoader ${snl_tests_with_pyloader})
  target_compile_definitions(snlTestsWithPyLoader PRIVATE
    SNL_BENCHS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchs")
  TARGET_LINK_LIBRARIES(snlTestsWithPyLoader naja_snl_pyloader gmock gtest_main)
  GTEST_DISCOVER_TESTS(snlTestsWithPyLoader
    DISCOVERY_TIMEOUT 30
    PROPERTIES ENVIRONMENT
    PYTHONPATH=${PROJECT_BINARY_DIR}/src/nl/python/naja_wrapping
  )
endif()
