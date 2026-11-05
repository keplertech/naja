// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <Python.h>
#include <cstdint>
#include "NajaPythonExport.h"
namespace PYNAJA {
// GIL-protected generation of raw Python netlist edits. Failed edits invalidate
// conservatively too: native methods need not offer transactional failure.
uint64_t pythonNetlistGeneration();
NAJA_PY_EXPORT int trackNetlistMethods(PyTypeObject* type);
}
