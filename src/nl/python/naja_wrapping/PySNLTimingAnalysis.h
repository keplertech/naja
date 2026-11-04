// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <Python.h>
#include "NajaPythonExport.h"
namespace PYNAJA {
bool PySNLTimingAnalysis_checkOccurrence(PyObject* object);
PyObject* PySNLDesign_analyzeTiming(PyObject* object, PyObject* unused);
PyObject* PySNLDesign_getTimingData(PyObject* object, int query);
PyObject* PySNLOccurrence_getTimingClocks(PyObject* object, int query);
NAJA_PY_EXPORT int PySNLTimingAnalysis_addTypes(PyObject* module);
}
