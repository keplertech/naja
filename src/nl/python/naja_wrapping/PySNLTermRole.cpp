// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
#include "PySNLTermRole.h"

namespace PYNAJA {
using namespace naja::NL;

PyTypeObjectDefinitions(SNLTermRole)

void PySNLTermRole_postModuleInit() {
  PyObject* constant;
  using Role = SNLDesignModeling::SNLTermRole;
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::Clock, "Clock");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::DataInput, "DataInput");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::DataOutput, "DataOutput");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::AsyncReset, "AsyncReset");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::AsyncSet, "AsyncSet");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::SyncReset, "SyncReset");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::SyncSet, "SyncSet");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::Enable, "Enable");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::ScanInput, "ScanInput");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::ScanEnable, "ScanEnable");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::AsyncLoad, "AsyncLoad");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::AsyncLoadData, "AsyncLoadData");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::MemoryReadAddress, "MemoryReadAddress");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::MemoryReadData, "MemoryReadData");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::MemoryWriteAddress, "MemoryWriteAddress");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::MemoryWriteData, "MemoryWriteData");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::MemoryWriteEnable, "MemoryWriteEnable");
  LoadObjectConstant(PyTypeSNLTermRole.tp_dict, Role::Other, "Other");
}

PyMethodDef PySNLTermRole_Methods[] = {{nullptr, nullptr, 0, nullptr}};
void PySNLTermRole_LinkPyType() {
  PyTypeSNLTermRole.tp_methods = PySNLTermRole_Methods;
}
}
