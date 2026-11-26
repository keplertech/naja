# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(TEST_CASE STREQUAL "gate-level-verilog-formats")
  set(TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/naja-edit-format-test")
  file(MAKE_DIRECTORY "${TEST_DIR}")
  file(WRITE "${TEST_DIR}/input.v" [=[
module top(input a, output y); assign y = a; endmodule
]=])
  foreach(INPUT_FORMAT gate-level-verilog verilog)
    foreach(OUTPUT_FORMAT gate-level-verilog verilog inferred)
      set(OUTPUT_FILE "${TEST_DIR}/${INPUT_FORMAT}-${OUTPUT_FORMAT}.v")
      set(TEST_ARGS -f ${INPUT_FORMAT} -i "${TEST_DIR}/input.v" -o "${OUTPUT_FILE}")
      set(EXPECTED_WARNINGS 0)
      if(INPUT_FORMAT STREQUAL "verilog")
        math(EXPR EXPECTED_WARNINGS "${EXPECTED_WARNINGS} + 1")
      endif()
      if(NOT OUTPUT_FORMAT STREQUAL "inferred")
        list(APPEND TEST_ARGS -t ${OUTPUT_FORMAT})
        if(OUTPUT_FORMAT STREQUAL "verilog")
          math(EXPR EXPECTED_WARNINGS "${EXPECTED_WARNINGS} + 1")
        endif()
      endif()
      execute_process(
        COMMAND "${NAJA_EDIT}" ${TEST_ARGS}
        WORKING_DIRECTORY "${TEST_DIR}"
        RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE STDOUT
        ERROR_VARIABLE STDERR)
      set(OUTPUT "${STDOUT}\n${STDERR}")
      if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "naja_edit failed for ${TEST_ARGS}:\n${OUTPUT}")
      endif()
      string(REGEX MATCHALL "will be deprecated soon" WARNINGS "${OUTPUT}")
      list(LENGTH WARNINGS WARNING_COUNT)
      if(NOT WARNING_COUNT EQUAL EXPECTED_WARNINGS)
        message(FATAL_ERROR "Expected ${EXPECTED_WARNINGS} compatibility warnings for ${TEST_ARGS}:\n${OUTPUT}")
      endif()
      if(EXPECTED_WARNINGS GREATER 0 AND NOT OUTPUT MATCHES "will be deprecated soon; use 'gate-level-verilog' instead")
        message(FATAL_ERROR "Missing migration guidance for ${TEST_ARGS}:\n${OUTPUT}")
      endif()
      file(READ "${OUTPUT_FILE}" NETLIST)
      # Only the timestamp banner may differ between invocations.
      string(FIND "${NETLIST}" "module " MODULE_OFFSET)
      if(MODULE_OFFSET LESS 0)
        message(FATAL_ERROR "Missing module in ${OUTPUT_FILE}")
      endif()
      string(SUBSTRING "${NETLIST}" ${MODULE_OFFSET} -1 NETLIST_BODY)
      if(DEFINED EXPECTED_BODY AND NOT NETLIST_BODY STREQUAL EXPECTED_BODY)
        message(FATAL_ERROR "Netlist differs for ${TEST_ARGS}")
      endif()
      set(EXPECTED_BODY "${NETLIST_BODY}")
    endforeach()
  endforeach()
  return()
endif()

if(TEST_CASE STREQUAL "invalid-input-format")
  set(TEST_ARGS --from_format json)
  set(EXPECTED
    "Invalid value 'json' for --from_format \\(-f\\).*Expected one of: gate-level-verilog \\(or verilog\\), systemverilog \\(or sv\\), vhdl, snl")
elseif(TEST_CASE STREQUAL "missing-required-input")
  set(TEST_ARGS --from_format snl)
  set(EXPECTED "--input \\(-i\\) is required with --from_format 'snl'")
elseif(TEST_CASE STREQUAL "missing-input-path")
  set(TEST_ARGS
    --from_format gate-level-verilog
    --input definitely-missing-naja-edit-input.v)
  set(EXPECTED
    "input path 'definitely-missing-naja-edit-input.v' does not exist.*resolved to")
elseif(TEST_CASE STREQUAL "invalid-output-format")
  set(TEST_ARGS --to_format systemverilog)
  set(EXPECTED
    "Invalid value 'systemverilog' for --to_format \\(-t\\).*Expected one of: gate-level-verilog \\(or verilog\\), snl, dot")
else()
  message(FATAL_ERROR "Unknown naja_edit error test case: ${TEST_CASE}")
endif()

execute_process(
  COMMAND "${NAJA_EDIT}" ${TEST_ARGS}
  RESULT_VARIABLE RESULT
  OUTPUT_VARIABLE STDOUT
  ERROR_VARIABLE STDERR)
set(OUTPUT "${STDOUT}\n${STDERR}")

if(RESULT EQUAL 0)
  message(FATAL_ERROR
    "naja_edit unexpectedly accepted invalid arguments for ${TEST_CASE}:\n${OUTPUT}")
endif()
if(NOT OUTPUT MATCHES "${EXPECTED}")
  message(FATAL_ERROR
    "naja_edit did not emit the expected diagnostic for ${TEST_CASE}.\n"
    "Expected regex: ${EXPECTED}\n"
    "Output:\n${OUTPUT}")
endif()
