// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NajaPythonExport.h"
#include "PyInterface.h"
#include "SNLEquipotential.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLEquipotential::Graph::EdgeKind* object_;
} PySNLEquipotentialEdgeKind;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLEquipotentialEdgeKind;

extern void PySNLEquipotentialEdgeKind_LinkPyType();
extern void PySNLEquipotentialEdgeKind_postModuleInit();

}  // namespace PYNAJA
