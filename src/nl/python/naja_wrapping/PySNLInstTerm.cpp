// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstTerm.h"

#include "SNLInstTerm.h"
#include "SNLDesignModeling.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"
#include "PySNLInstance.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLInstTerm, function)

DBoLinkCreateMethod(SNLInstTerm)
PyTypeInheritedObjectDefinitions(SNLInstTerm, SNLNetComponent)

GetObjectMethod(SNLInstTerm, SNLInstance, getInstance)
GetObjectMethod(SNLInstTerm, SNLBitTerm, getBitTerm)

static PyObject* PySNLInstTerm_getRole(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.getRole()")
  TRY
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getTermRole(selfObject)));
  NLCATCH
}

static PyObject* PySNLInstTerm_getActiveLevel(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.getActiveLevel()")
  TRY
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getActiveLevel(selfObject)));
  NLCATCH
}

static PyObject* PySNLInstTerm_getResetActiveLevel(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.getResetActiveLevel()")
  TRY
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getResetActiveLevel(selfObject)));
  NLCATCH
}

#define INST_TERM_ROLE_PREDICATE(PYNAME, CPPNAME)                       \
  static PyObject* PySNLInstTerm_##PYNAME(PySNLInstTerm* self) {        \
    METHOD_HEAD("SNLInstTerm." #PYNAME "()")                            \
    TRY                                                               \
    if (SNLDesignModeling::CPPNAME(selfObject)) Py_RETURN_TRUE;          \
    Py_RETURN_FALSE;                                                     \
    NLCATCH                                                             \
  }

INST_TERM_ROLE_PREDICATE(isInvertedInput, isInvertedInput)
INST_TERM_ROLE_PREDICATE(isClock, isClock)
INST_TERM_ROLE_PREDICATE(isAsyncReset, isAsyncReset)
INST_TERM_ROLE_PREDICATE(isAsyncSet, isAsyncSet)
INST_TERM_ROLE_PREDICATE(isSyncReset, isSyncReset)
INST_TERM_ROLE_PREDICATE(isSyncSet, isSyncSet)
INST_TERM_ROLE_PREDICATE(isReset, isReset)
INST_TERM_ROLE_PREDICATE(isEnable, isEnable)
INST_TERM_ROLE_PREDICATE(isDataInput, isDataInput)
INST_TERM_ROLE_PREDICATE(isDataOutput, isDataOutput)

static PyObject* PySNLInstTerm_isData(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.isData()")
  TRY
  if (SNLDesignModeling::isDataInput(selfObject) ||
      SNLDesignModeling::isDataOutput(selfObject)) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
  NLCATCH
}

#undef INST_TERM_ROLE_PREDICATE

#define DEPRECATED_ROLE_ALIAS(ALIAS, REPLACEMENT) \
  static PyObject* PySNLInstTerm_##ALIAS(PySNLInstTerm* self) { \
    if (PyErr_WarnEx(PyExc_DeprecationWarning, \
        "SNLInstTerm." #ALIAS "() is deprecated and will be removed in a future release; " \
        "use " #REPLACEMENT "() instead.", 1) < 0) return nullptr; \
    return PySNLInstTerm_##REPLACEMENT(self); \
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

PyMethodDef PySNLInstTerm_Methods[] = {
  { "getBitTerm", (PyCFunction)PySNLInstTerm_getBitTerm, METH_NOARGS,
    "get the SNLBitTerm represented by this SNLInstTerm."},
  { "getInstance", (PyCFunction)PySNLInstTerm_getInstance, METH_NOARGS,
    "get the SNLInstance containing this SNLInstTerm."},
  {"getRole", (PyCFunction)PySNLInstTerm_getRole, METH_NOARGS, "get the primitive term role."},
  {"getActiveLevel", (PyCFunction)PySNLInstTerm_getActiveLevel, METH_NOARGS, "get active level for any role."},
  {"getResetActiveLevel", (PyCFunction)PySNLInstTerm_getResetActiveLevel, METH_NOARGS, "get reset/set active level."},
  {"isInvertedInput", (PyCFunction)PySNLInstTerm_isInvertedInput, METH_NOARGS, "whether this input has a gate bubble."},
  {"isClock", (PyCFunction)PySNLInstTerm_isClock, METH_NOARGS, "whether this term is a clock."},
  {"isAsyncReset", (PyCFunction)PySNLInstTerm_isAsyncReset, METH_NOARGS, "whether this term is an asynchronous reset."},
  {"isAsyncSet", (PyCFunction)PySNLInstTerm_isAsyncSet, METH_NOARGS, "whether this term is an asynchronous set."},
  {"isSyncReset", (PyCFunction)PySNLInstTerm_isSyncReset, METH_NOARGS, "whether this term is a synchronous reset."},
  {"isSyncSet", (PyCFunction)PySNLInstTerm_isSyncSet, METH_NOARGS, "whether this term is a synchronous set."},
  {"isReset", (PyCFunction)PySNLInstTerm_isReset, METH_NOARGS, "whether this term is a reset."},
  {"isEnable", (PyCFunction)PySNLInstTerm_isEnable, METH_NOARGS, "whether this term is an enable."},
  {"isData", (PyCFunction)PySNLInstTerm_isData, METH_NOARGS, "whether this term carries data."},
  {"isDataInput", (PyCFunction)PySNLInstTerm_isDataInput, METH_NOARGS, "whether this term is a data input."},
  {"isDataOutput", (PyCFunction)PySNLInstTerm_isDataOutput, METH_NOARGS, "whether this term is a data output."},
  // Deprecated compatibility aliases; use the camelCase predicates.
  {"is_clock", (PyCFunction)PySNLInstTerm_is_clock, METH_NOARGS, "Deprecated; use isClock()."},
  {"is_async_reset", (PyCFunction)PySNLInstTerm_is_async_reset, METH_NOARGS, "Deprecated; use isAsyncReset()."},
  {"is_async_set", (PyCFunction)PySNLInstTerm_is_async_set, METH_NOARGS, "Deprecated; use isAsyncSet()."},
  {"is_sync_reset", (PyCFunction)PySNLInstTerm_is_sync_reset, METH_NOARGS, "Deprecated; use isSyncReset()."},
  {"is_sync_set", (PyCFunction)PySNLInstTerm_is_sync_set, METH_NOARGS, "Deprecated; use isSyncSet()."},
  {"is_reset", (PyCFunction)PySNLInstTerm_is_reset, METH_NOARGS, "Deprecated; use isReset()."},
  {"is_enable", (PyCFunction)PySNLInstTerm_is_enable, METH_NOARGS, "Deprecated; use isEnable()."},
  {"is_data", (PyCFunction)PySNLInstTerm_is_data, METH_NOARGS, "Deprecated; use isData()."},
  {"is_data_input", (PyCFunction)PySNLInstTerm_is_data_input, METH_NOARGS, "Deprecated; use isDataInput()."},
  {"is_data_output", (PyCFunction)PySNLInstTerm_is_data_output, METH_NOARGS, "Deprecated; use isDataOutput()."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLInstTerm)

PyTypeNLFinalObjectWithNLIDLinkPyType(SNLInstTerm)

}
