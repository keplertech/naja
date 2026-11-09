# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

add_executable(snlVHDLConstructorTests VHDLConstructorTest.cpp)
target_compile_definitions(snlVHDLConstructorTests PRIVATE
  SNL_SCHEDULED_OVERWRITE_SV="${CMAKE_CURRENT_SOURCE_DIR}/scheduled_overwrite.sv"
  SNL_SCHEDULED_OVERWRITE_VHDL="${CMAKE_CURRENT_SOURCE_DIR}/scheduled_overwrite.vhd"
  SNL_VHDL_EQUIVALENT_ARITHMETIC_SV="${CMAKE_CURRENT_SOURCE_DIR}/arithmetic_equivalent.sv"
  SNL_VHDL_INDEXED_TABLES="${CMAKE_CURRENT_SOURCE_DIR}/indexed_tables.vhd"
  SNL_VHDL_ASYNCHRONOUS_RESET="${CMAKE_CURRENT_SOURCE_DIR}/asynchronous_reset.vhd"
  SNL_VHDL_MEMORY="${CMAKE_CURRENT_SOURCE_DIR}/memory.vhd"
  SNL_VHDL_GENERATED_FIR="${CMAKE_CURRENT_SOURCE_DIR}/fir_generated.vhd"
  SNL_VHDL_LOGICAL="${CMAKE_CURRENT_SOURCE_DIR}/logical.vhd"
  SNL_VHDL_HIERARCHY="${CMAKE_CURRENT_SOURCE_DIR}/hierarchy.vhd"
  SNL_VHDL_ENABLED="${CMAKE_CURRENT_SOURCE_DIR}/enabled.vhd"
  SNL_VHDL_RESET="${CMAKE_CURRENT_SOURCE_DIR}/reset.vhd"
  SNL_VHDL_RESET_ENABLE="${CMAKE_CURRENT_SOURCE_DIR}/reset_enable.vhd"
  SNL_VHDL_VECTORS="${CMAKE_CURRENT_SOURCE_DIR}/vectors.vhd"
  SNL_VHDL_EQUIVALENT_LOGICAL_SV="${CMAKE_CURRENT_SOURCE_DIR}/logical_equivalent.sv"
  SNL_VHDL_EQUIVALENT_VECTOR_MUX_SV="${CMAKE_CURRENT_SOURCE_DIR}/vector_mux_equivalent.sv"
  SNL_VHDL_RETAINED_VARIABLES="${CMAKE_CURRENT_SOURCE_DIR}/retained_variables.vhd"
  SNL_VHDL_VARIABLES="${CMAKE_CURRENT_SOURCE_DIR}/variables.vhd"
  SNL_VHDL_PIPELINE="${CMAKE_CURRENT_SOURCE_DIR}/pipeline.vhd"
  SNL_VHDL_EQUIVALENT_MUX_SV="${CMAKE_CURRENT_SOURCE_DIR}/mux_equivalent.sv"
  SNL_VHDL_EQUIVALENT_REGISTER_SV="${CMAKE_CURRENT_SOURCE_DIR}/register_equivalent.sv")
target_link_libraries(snlVHDLConstructorTests
  naja_snl_vhdl
  naja_snl_systemverilog
  naja_snl_verilog
  gtest_main)
gtest_discover_tests(snlVHDLConstructorTests)

# Downloaded inputs are exercised explicitly by vhdl-regress.yml, not CTest.
add_executable(snlVHDLExternalTests EXCLUDE_FROM_ALL VHDLExternalTest.cpp)
target_link_libraries(snlVHDLExternalTests naja_snl_vhdl gtest_main)

find_program(VHDL_NVC_EXECUTABLE nvc)
find_package(Python3 COMPONENTS Interpreter QUIET)
find_program(VHDL_IVERILOG_EXECUTABLE iverilog)
find_program(VHDL_VVP_EXECUTABLE vvp)
if(Python3_Interpreter_FOUND AND VHDL_IVERILOG_EXECUTABLE AND VHDL_VVP_EXECUTABLE)
  add_test(NAME VHDLIndexedTablesVerilog
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_indexed_tables.py"
      --adapter $<TARGET_FILE:snlVHDLConstructorTests>
      --iverilog "${VHDL_IVERILOG_EXECUTABLE}" --vvp "${VHDL_VVP_EXECUTABLE}")
  set_tests_properties(VHDLIndexedTablesVerilog PROPERTIES TIMEOUT 180)
endif()
if(VHDL_NVC_EXECUTABLE AND Python3_Interpreter_FOUND)
  add_test(NAME VHDLGeneratedFIRReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_fir.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>
      --source "${CMAKE_CURRENT_SOURCE_DIR}/fir_generated.vhd" --top fir_generated
      --lanes 4 --width 5 --test VHDLConstructorTest.GeneratedFIRHierarchyCycles)
  set_tests_properties(VHDLGeneratedFIRReference PROPERTIES TIMEOUT 180)
  set(VHDL_MEMORY_DUMP_ARGS)
  if(VHDL_IVERILOG_EXECUTABLE AND VHDL_VVP_EXECUTABLE)
    list(APPEND VHDL_MEMORY_DUMP_ARGS --iverilog "${VHDL_IVERILOG_EXECUTABLE}" --vvp "${VHDL_VVP_EXECUTABLE}")
  endif()
  add_test(NAME VHDLAsynchronousResetReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_asynchronous_reset.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>
      ${VHDL_MEMORY_DUMP_ARGS})
  set_tests_properties(VHDLAsynchronousResetReference PROPERTIES TIMEOUT 180)
  add_test(NAME VHDLMemoryReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_memory.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>
      ${VHDL_MEMORY_DUMP_ARGS})
  set_tests_properties(VHDLMemoryReference PROPERTIES TIMEOUT 180)
  add_test(NAME VHDLPipelineReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_pipeline.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLVariableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_variables.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLRetainedVariableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_retained_variables.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLLogicalReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_logical.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLVectorReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_vectors.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLHierarchyReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_hierarchy.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLEnableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_enable.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLResetReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_reset.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLResetEnableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_reset_enable.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  set_tests_properties(VHDLPipelineReference VHDLVariableReference
    VHDLRetainedVariableReference VHDLLogicalReference VHDLVectorReference
    VHDLHierarchyReference VHDLEnableReference VHDLResetReference
    VHDLResetEnableReference
    PROPERTIES TIMEOUT 180)
endif()

if(Python3_Interpreter_FOUND AND VHDL_NVC_EXECUTABLE AND VHDL_IVERILOG_EXECUTABLE AND VHDL_VVP_EXECUTABLE)
  add_test(NAME HDLScheduledOverwriteNative
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_scheduled_overwrite.py"
      --adapter $<TARGET_FILE:snlVHDLConstructorTests>
      --nvc "${VHDL_NVC_EXECUTABLE}" --iverilog "${VHDL_IVERILOG_EXECUTABLE}" --vvp "${VHDL_VVP_EXECUTABLE}")
  set_tests_properties(HDLScheduledOverwriteNative PROPERTIES TIMEOUT 180)
endif()
