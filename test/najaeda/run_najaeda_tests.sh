#!/bin/bash
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
#
# Bazel sh_test equivalent of setup_test_env.cmake + the najaeda-python-tests
# ctest: copies the najaeda package sources into a writable scratch dir,
# drops naja.so alongside them (so `from najaeda import naja` resolves,
# same reason CMake's setup_test_env.cmake symlinks it into place rather
# than leaving it top-level), then runs unittest discover against the
# real (uncopied) test/najaeda sources with the same benchmark/test-path
# env vars CMakeLists.txt's ENVIRONMENT string sets.
set -euo pipefail

najaeda_src_dir="$1"
naja_so="$2"
naja_runtime="$3"
python3_bin="$4"
test_src_dir="$5"
verilog_benchmarks="$6"
systemverilog_benchmarks="$7"
liberty_benchmarks="$8"
test_pattern="${9:-test*.py}"

runfiles_root="$PWD"
abspath() { echo "${runfiles_root}/$1"; }

scratch="$(mktemp -d)"
trap 'rm -rf "${scratch}"' EXIT

cp -R "$(abspath "${najaeda_src_dir}")" "${scratch}/najaeda"
cp "$(abspath "${naja_so}")" "${scratch}/najaeda/naja.so"

runtime_dir="$(dirname "$(abspath "${naja_runtime}")")"
export DYLD_LIBRARY_PATH="${runtime_dir}${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}"
export LD_LIBRARY_PATH="${runtime_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PYTHONPATH="${scratch}"
export VERILOG_BENCHMARKS_PATH="$(abspath "${verilog_benchmarks}")"
export SYSTEMVERILOG_BENCHMARKS_PATH="$(abspath "${systemverilog_benchmarks}")"
export LIBERTY_BENCHMARKS_PATH="$(abspath "${liberty_benchmarks}")"
export NAJAEDA_TEST_PATH="${scratch}"
export NAJAEDA_SOURCE_TEST_PATH="$(abspath "${test_src_dir}")"

if [[ -n "${NAJA_DB0_AUDIT_BINARY:-}" ]]; then
  export NAJA_DB0_AUDIT_BINARY="$(abspath "${NAJA_DB0_AUDIT_BINARY}")"
fi

if [[ -n "${NAJA_EDIT_BINARY:-}" ]]; then
  export NAJA_EDIT_BINARY="$(abspath "${NAJA_EDIT_BINARY}")"
fi

cd "${scratch}"
exec "$(abspath "${python3_bin}")" -m unittest discover -v "${NAJAEDA_SOURCE_TEST_PATH}" -p "${test_pattern}"
