// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLEquipotentialEdgeKind.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeObjectDefinitions(SNLEquipotentialEdgeKind)

void PySNLEquipotentialEdgeKind_postModuleInit() {
  PyObject* constant;
  LoadObjectConstant(
    PyTypeSNLEquipotentialEdgeKind.tp_dict,
    SNLEquipotential::Graph::EdgeKind::Port,
    "Port");
  LoadObjectConstant(
    PyTypeSNLEquipotentialEdgeKind.tp_dict,
    SNLEquipotential::Graph::EdgeKind::Assign,
    "Assign");
}

PyMethodDef PySNLEquipotentialEdgeKind_Methods[] = {
  {nullptr, nullptr, 0, nullptr}
};

void PySNLEquipotentialEdgeKind_LinkPyType() {
  PyTypeSNLEquipotentialEdgeKind.tp_methods = PySNLEquipotentialEdgeKind_Methods;
}

}  // namespace PYNAJA
