#!/bin/bash
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

runfiles_root="$PWD"
module="${runfiles_root}/$1"
runtime="${runfiles_root}/$2"
python="${runfiles_root}/$3"
tests="${runfiles_root}/$4"
pattern="${5:-test*.py}"
scratch="$(mktemp -d "${TEST_TMPDIR:-/tmp}/naja-bindings.XXXXXX")"
trap 'rm -rf "${scratch}"' EXIT
cp "${module}" "${scratch}/naja.so"
runtime_dir="$(dirname "${runtime}")"
export DYLD_LIBRARY_PATH="${runtime_dir}${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}"
export LD_LIBRARY_PATH="${runtime_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PYTHONPATH="${scratch}"
export SNL_WRAPPING_TEST_PATH="${scratch}"
export FORMATS_PATH="${runfiles_root}/test/nl/formats"
cd "${scratch}"
"${python}" -m unittest discover -v -s "${tests}" -p "${pattern}"
