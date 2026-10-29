#!/bin/bash
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
root="$PWD"
module="$root/$1"
runtime="$root/$2"
python="$root/$3"
tests="$root/$4"
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT
cp "$module" "$scratch/naja.so"
export PYTHONPATH="$scratch"
export DYLD_LIBRARY_PATH="$(dirname "$runtime")${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="$(dirname "$runtime")${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$python" "$tests" -v
