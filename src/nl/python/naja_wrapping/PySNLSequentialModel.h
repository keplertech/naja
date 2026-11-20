// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PySNLDesign.h"
#include "PySNLInstance.h"

namespace PYNAJA {

PyObject* PySNLDesign_setSequentialModel(
    PySNLDesign* self, PyObject* args, PyObject* kwargs);
PyObject* PySNLDesign_hasSequentialModel(PySNLDesign* self);
PyObject* PySNLDesign_hasSequentialModelFromParameters(PySNLDesign* self);
PyObject* PySNLDesign_setSequentialModelFromParameters(
    PySNLDesign* self, PyObject* args, PyObject* kwargs);
PyObject* PySNLDesign_getSequentialModel(PySNLDesign* self);
PyObject* PySNLInstance_getSequentialInitialValues(PySNLInstance* self);
PyObject* PySNLInstance_getSequentialModel(PySNLInstance* self);

}  // namespace PYNAJA
