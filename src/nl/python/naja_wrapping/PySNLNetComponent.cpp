// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNetComponent.h"

#include "SNLNet.h"
#include "SNLTerm.h"
#include "SNLInstTerm.h"

#include "PyInterface.h"
#include "PySNLInstTerm.h"
#include "PySNLTerm.h"
#include "PySNLBitNet.h"

namespace PYNAJA {

using namespace naja::NL;

#undef ACCESS_OBJECT
#undef ACCESS_CLASS
#define ACCESS_OBJECT           parent_.object_
#define ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLNetComponent, function)

GetObjectMethod(SNLNetComponent, SNLBitNet, getNet)

static PyObject* PySNLNetComponent_setNet(PySNLNetComponent* self, PyObject* arg) {
  METHOD_HEAD("SNLNetComponent.setNet()")
  if (arg == Py_None) {
    selfObject->setNet(nullptr);
  } else if (IsPySNLNet(arg)) {
    TRY
    selfObject->setNet(PYSNLNet_O(arg));
    NLCATCH
  } else {
    setError("SNLNetComponent setNet takes SNLNet or None (for disconnection) argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PySNLNetComponent_getDirection(PySNLNetComponent* self) {
  METHOD_HEAD("NetComponent.getDirection()")
  return PyLong_FromLong((long)selfObject->getDirection());
}

PyMethodDef PySNLNetComponent_Methods[] = {
  { "getNet", (PyCFunction)PySNLNetComponent_getNet, METH_NOARGS,
    "get SNLNetComponent SNLBitNet"},
  { "setNet", (PyCFunction)PySNLNetComponent_setNet, METH_O,
    "set SNLNetComponent SNLNet"},
  { "getDirection", (PyCFunction)PySNLNetComponent_getDirection, METH_NOARGS,
    "get SNLNetComponent direction"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLNetComponent)

PyObject* PySNLNetComponent_Link(SNLNetComponent* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto instTerm = dynamic_cast<SNLInstTerm*>(object)) {
    return PySNLInstTerm_Link(instTerm);
  } else if (auto term = dynamic_cast<SNLTerm*>(object)) {
    return PySNLTerm_Link(term);
  }
  setError("Unsupported SNLNetComponent subtype");
  return nullptr;
}

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLNetComponent)
PyTypeInheritedObjectDefinitions(SNLNetComponent, SNLDesignObject)

}