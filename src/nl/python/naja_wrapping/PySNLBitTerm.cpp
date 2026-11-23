// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitTerm.h"

#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesignModeling.h"

#include "PyInterface.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTermBit.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLBitTerm, function)

static PyObject* PySNLBitTerm_getRole(PySNLBitTerm* self) {
  METHOD_HEAD("SNLBitTerm.getRole()")
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getTermRole(selfObject)));
}

static PyObject* PySNLBitTerm_getResetActiveLevel(PySNLBitTerm* self) {
  METHOD_HEAD("SNLBitTerm.getResetActiveLevel()")
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getResetActiveLevel(selfObject)));
}

static bool parseTermRole(
    PyObject* object,
    SNLDesignModeling::SNLTermRole& role) {
  if (not PyLong_Check(object)) {
    return false;
  }
  using Role = SNLDesignModeling::SNLTermRole;
  switch (PyLong_AsLong(object)) {
    case static_cast<long>(Role::Clock): role = Role::Clock; return true;
    case static_cast<long>(Role::DataInput): role = Role::DataInput; return true;
    case static_cast<long>(Role::DataOutput): role = Role::DataOutput; return true;
    case static_cast<long>(Role::AsyncReset): role = Role::AsyncReset; return true;
    case static_cast<long>(Role::AsyncSet): role = Role::AsyncSet; return true;
    case static_cast<long>(Role::SyncReset): role = Role::SyncReset; return true;
    case static_cast<long>(Role::SyncSet): role = Role::SyncSet; return true;
    case static_cast<long>(Role::Enable): role = Role::Enable; return true;
    case static_cast<long>(Role::MemoryReadAddress): role = Role::MemoryReadAddress; return true;
    case static_cast<long>(Role::MemoryReadData): role = Role::MemoryReadData; return true;
    case static_cast<long>(Role::MemoryWriteAddress): role = Role::MemoryWriteAddress; return true;
    case static_cast<long>(Role::MemoryWriteData): role = Role::MemoryWriteData; return true;
    case static_cast<long>(Role::MemoryWriteEnable): role = Role::MemoryWriteEnable; return true;
    case static_cast<long>(Role::Other): role = Role::Other; return true;
    case static_cast<long>(Role::ScanInput): role = Role::ScanInput; return true;
    case static_cast<long>(Role::ScanEnable): role = Role::ScanEnable; return true;
    default: return false;
  }
}

static bool parseActiveLevel(
    PyObject* object,
    SNLDesignModeling::SNLActiveLevel& activeLevel) {
  if (not PyLong_Check(object)) {
    return false;
  }
  using Level = SNLDesignModeling::SNLActiveLevel;
  switch (PyLong_AsLong(object)) {
    case static_cast<long>(Level::High): activeLevel = Level::High; return true;
    case static_cast<long>(Level::Low): activeLevel = Level::Low; return true;
    case static_cast<long>(Level::NA): activeLevel = Level::NA; return true;
    default: return false;
  }
}

static PyObject* PySNLBitTerm_setRole(PySNLBitTerm* self, PyObject* args) {
  PyObject* roleObject = nullptr;
  PyObject* activeLevelObject = nullptr;
  if (not PyArg_ParseTuple(
      args, "O|O:SNLBitTerm.setRole", &roleObject, &activeLevelObject)) {
    setError("SNLBitTerm.setRole() expects a role and an optional active level");
    return nullptr;
  }
  auto role = SNLDesignModeling::SNLTermRole::Other;
  if (not parseTermRole(roleObject, role)) {
    setError("SNLBitTerm.setRole() expects a valid SNLTermRole");
    return nullptr;
  }
  auto activeLevel = SNLDesignModeling::SNLActiveLevel::NA;
  if (activeLevelObject and not parseActiveLevel(activeLevelObject, activeLevel)) {
    setError("SNLBitTerm.setRole() expects a valid SNLActiveLevel");
    return nullptr;
  }
  METHOD_HEAD("SNLBitTerm.setRole()")
  TRY
  SNLDesignModeling::setTermRole(selfObject, role, activeLevel);
  NLCATCH
  Py_RETURN_NONE;
}

#define TERM_ROLE_PREDICATE(PYNAME, CPPNAME)                        \
  static PyObject* PySNLBitTerm_##PYNAME(PySNLBitTerm* self) {      \
    METHOD_HEAD("SNLBitTerm." #PYNAME "()")                         \
    if (SNLDesignModeling::CPPNAME(selfObject)) Py_RETURN_TRUE;      \
    Py_RETURN_FALSE;                                                 \
  }

TERM_ROLE_PREDICATE(isClock, isClock)
TERM_ROLE_PREDICATE(isAsyncReset, isAsyncReset)
TERM_ROLE_PREDICATE(isAsyncSet, isAsyncSet)
TERM_ROLE_PREDICATE(isSyncReset, isSyncReset)
TERM_ROLE_PREDICATE(isSyncSet, isSyncSet)
TERM_ROLE_PREDICATE(isReset, isReset)
TERM_ROLE_PREDICATE(isEnable, isEnable)
TERM_ROLE_PREDICATE(isDataInput, isDataInput)
TERM_ROLE_PREDICATE(isDataOutput, isDataOutput)

static PyObject* PySNLBitTerm_isData(PySNLBitTerm* self) {
  METHOD_HEAD("SNLBitTerm.isData()")
  if (SNLDesignModeling::isDataInput(selfObject) ||
      SNLDesignModeling::isDataOutput(selfObject)) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

#undef TERM_ROLE_PREDICATE

#define DEPRECATED_ROLE_ALIAS(ALIAS, REPLACEMENT) \
  static PyObject* PySNLBitTerm_##ALIAS(PySNLBitTerm* self) { \
    if (PyErr_WarnEx(PyExc_DeprecationWarning, \
        "SNLBitTerm." #ALIAS "() is deprecated and will be removed in a future release; " \
        "use " #REPLACEMENT "() instead.", 1) < 0) return nullptr; \
    return PySNLBitTerm_##REPLACEMENT(self); \
  }

DEPRECATED_ROLE_ALIAS(is_clock, isClock)
DEPRECATED_ROLE_ALIAS(is_async_reset, isAsyncReset)
DEPRECATED_ROLE_ALIAS(is_async_set, isAsyncSet)
DEPRECATED_ROLE_ALIAS(is_sync_reset, isSyncReset)
DEPRECATED_ROLE_ALIAS(is_sync_set, isSyncSet)
DEPRECATED_ROLE_ALIAS(is_reset, isReset)
DEPRECATED_ROLE_ALIAS(is_enable, isEnable)
DEPRECATED_ROLE_ALIAS(is_data, isData)
DEPRECATED_ROLE_ALIAS(is_data_input, isDataInput)
DEPRECATED_ROLE_ALIAS(is_data_output, isDataOutput)

#undef DEPRECATED_ROLE_ALIAS

PyMethodDef PySNLBitTerm_Methods[] = {
  {"getRole", (PyCFunction)PySNLBitTerm_getRole, METH_NOARGS, "get the primitive term role."},
  {"setRole", (PyCFunction)PySNLBitTerm_setRole, METH_VARARGS,
    "set the primitive term role and optional active level."},
  {"getResetActiveLevel", (PyCFunction)PySNLBitTerm_getResetActiveLevel, METH_NOARGS, "get reset/set active level."},
  {"isClock", (PyCFunction)PySNLBitTerm_isClock, METH_NOARGS, "whether this term is a clock."},
  {"isAsyncReset", (PyCFunction)PySNLBitTerm_isAsyncReset, METH_NOARGS, "whether this term is an asynchronous reset."},
  {"isAsyncSet", (PyCFunction)PySNLBitTerm_isAsyncSet, METH_NOARGS, "whether this term is an asynchronous set."},
  {"isSyncReset", (PyCFunction)PySNLBitTerm_isSyncReset, METH_NOARGS, "whether this term is a synchronous reset."},
  {"isSyncSet", (PyCFunction)PySNLBitTerm_isSyncSet, METH_NOARGS, "whether this term is a synchronous set."},
  {"isReset", (PyCFunction)PySNLBitTerm_isReset, METH_NOARGS, "whether this term is a reset."},
  {"isEnable", (PyCFunction)PySNLBitTerm_isEnable, METH_NOARGS, "whether this term is an enable."},
  {"isData", (PyCFunction)PySNLBitTerm_isData, METH_NOARGS, "whether this term carries data."},
  {"isDataInput", (PyCFunction)PySNLBitTerm_isDataInput, METH_NOARGS, "whether this term is a data input."},
  {"isDataOutput", (PyCFunction)PySNLBitTerm_isDataOutput, METH_NOARGS, "whether this term is a data output."},
  // Deprecated compatibility aliases; use the camelCase predicates.
  {"is_clock", (PyCFunction)PySNLBitTerm_is_clock, METH_NOARGS, "Deprecated; use isClock()."},
  {"is_async_reset", (PyCFunction)PySNLBitTerm_is_async_reset, METH_NOARGS, "Deprecated; use isAsyncReset()."},
  {"is_async_set", (PyCFunction)PySNLBitTerm_is_async_set, METH_NOARGS, "Deprecated; use isAsyncSet()."},
  {"is_sync_reset", (PyCFunction)PySNLBitTerm_is_sync_reset, METH_NOARGS, "Deprecated; use isSyncReset()."},
  {"is_sync_set", (PyCFunction)PySNLBitTerm_is_sync_set, METH_NOARGS, "Deprecated; use isSyncSet()."},
  {"is_reset", (PyCFunction)PySNLBitTerm_is_reset, METH_NOARGS, "Deprecated; use isReset()."},
  {"is_enable", (PyCFunction)PySNLBitTerm_is_enable, METH_NOARGS, "Deprecated; use isEnable()."},
  {"is_data", (PyCFunction)PySNLBitTerm_is_data, METH_NOARGS, "Deprecated; use isData()."},
  {"is_data_input", (PyCFunction)PySNLBitTerm_is_data_input, METH_NOARGS, "Deprecated; use isDataInput()."},
  {"is_data_output", (PyCFunction)PySNLBitTerm_is_data_output, METH_NOARGS, "Deprecated; use isDataOutput()."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyObject* PySNLBitTerm_Link(SNLBitTerm* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busTermBit = dynamic_cast<SNLBusTermBit*>(object)) {
    return PySNLBusTermBit_Link(busTermBit);
  } else if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(object)) {
    return PySNLScalarTerm_Link(scalarTerm);
  }
  // LCOV_EXCL_START Defensive fallback: all concrete bit-term types are handled above.
  setError("Unsupported SNLBitTerm subtype");
  return nullptr;
  // LCOV_EXCL_STOP
}

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLBitTerm)
PyTypeInheritedObjectDefinitions(SNLBitTerm, SNLTerm)

}
