# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(NOT NAJA_PYTHON_EXECUTABLE)
  message(FATAL_ERROR "NAJA_PYTHON_EXECUTABLE is not set")
endif()

# Setup test: copy najaeda sources into the binary dir and symlink naja.so.
# Runs as a ctest FIXTURES_SETUP so the environment is ready before the real test.
add_test(NAME najaeda-python-tests-setup
  COMMAND ${CMAKE_COMMAND}
    -DSRC=${CMAKE_SOURCE_DIR}/src/najaeda/najaeda
    -DDST=${CMAKE_CURRENT_BINARY_DIR}/najaeda
    -DNAJA_SO=${CMAKE_BINARY_DIR}/src/nl/python/naja_wrapping/naja.so
    -P ${CMAKE_CURRENT_SOURCE_DIR}/setup_test_env.cmake
)
set_tests_properties(najaeda-python-tests-setup PROPERTIES
  FIXTURES_SETUP najaeda-test-env
)

# Actual test: run Python unittest directly so ENVIRONMENT applies correctly.
add_test(NAME najaeda-python-tests
  COMMAND "${NAJA_PYTHON_EXECUTABLE}" -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(najaeda-python-tests PROPERTIES
  FIXTURES_REQUIRED najaeda-test-env
  ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR};VERILOG_BENCHMARKS_PATH=${CMAKE_SOURCE_DIR}/test/nl/formats/verilog/benchmarks;SYSTEMVERILOG_BENCHMARKS_PATH=${CMAKE_SOURCE_DIR}/test/nl/formats/systemverilog/benchmarks;LIBERTY_BENCHMARKS_PATH=${CMAKE_SOURCE_DIR}/test/nl/formats/liberty/benchmarks;NAJA_DB0_AUDIT_BINARY=$<TARGET_FILE:snlCapnPTests>;NAJAEDA_TEST_PATH=${CMAKE_CURRENT_BINARY_DIR};NAJAEDA_SOURCE_TEST_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
)

# Keep reproducibility visible as a separately selectable CI contract. The same
# module is also discovered by the ordinary Python suite above. Keep "python"
# in the name: sanitizer jobs exclude Python tests and their setup fixture.
add_test(NAME najaeda-python-determinism-tests
  COMMAND "${NAJA_PYTHON_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/test_najaeda_determinism.py -v
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(najaeda-python-determinism-tests PROPERTIES
  FIXTURES_REQUIRED najaeda-test-env
  LABELS determinism
  TIMEOUT 300
  ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR};NAJA_DB0_AUDIT_BINARY=$<TARGET_FILE:snlCapnPTests>;NAJAEDA_TEST_PATH=${CMAKE_CURRENT_BINARY_DIR}"
)

add_test(NAME najaeda-python-content-hash-tests
  COMMAND "${NAJA_PYTHON_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/test_najaeda_content_hash.py -v
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(najaeda-python-content-hash-tests PROPERTIES
  FIXTURES_REQUIRED najaeda-test-env
  LABELS content_hash
  ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}"
)
