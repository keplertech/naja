# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

option(NAJA_BENCHMARK_CODSPEED "Build benchmarks for CodSpeed CPU simulation" OFF)

if(NAJA_BENCHMARK_CODSPEED)
  include(FetchContent)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  set(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_INSTALL OFF)
  set(CODSPEED_MODE simulation CACHE STRING "Build mode for CodSpeed" FORCE)
  FetchContent_Declare(codspeed_benchmark
    # codspeed-cpp v2.4.0
    URL https://github.com/CodSpeedHQ/codspeed-cpp/archive/f5a917fdd14db7293bd37acb682873fec19f8b6c.tar.gz
    URL_HASH SHA256=c762ab075afbf52d50b588c5af1132e01e34ac332988bd54e9ef519fc2bf0036
    SOURCE_SUBDIR google_benchmark
  )
  FetchContent_MakeAvailable(codspeed_benchmark)
  # Upstream core also declares install rules; keep this CI dependency out of installs.
  set_property(DIRECTORY "${codspeed_benchmark_SOURCE_DIR}/google_benchmark"
    PROPERTY EXCLUDE_FROM_ALL TRUE)
else()
  find_package(benchmark REQUIRED)
endif()

add_executable(snl_bench
  SNLNetlistBench.cpp
  ${CMAKE_SOURCE_DIR}/test/nl/netlists/SNLNetlist0.cpp
)

target_include_directories(snl_bench PRIVATE
  ${CMAKE_SOURCE_DIR}/test/nl/netlists
)

target_link_libraries(snl_bench PRIVATE
  benchmark::benchmark
  naja_dnl
  naja_opt
  naja_snl_verilog
  naja_nl_dump
  naja_nl
)

target_compile_definitions(snl_bench PRIVATE
  SNL_VRL_BENCHMARKS_PATH="${CMAKE_SOURCE_DIR}/test/nl/formats/verilog/benchmarks"
)

add_executable(snl_format_bench SNLFormatBench.cpp)
target_link_libraries(snl_format_bench PRIVATE
  benchmark::benchmark naja_snl_systemverilog naja_snl_vhdl
  naja_snl_liberty naja_snl_verilog naja_nl_dump naja_nl
)
