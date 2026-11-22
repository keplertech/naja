// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitNet.h"

#include "SNLScalarNet.h"
#include "SNLBusNetBit.h"

#include "PyInterface.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNetBit.h"
#include "PySNLNetComponents.h"
#include "PySNLInstTerms.h"
#include "PySNLBitTerms.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLBitNet, function)

GetContainerMethod(SNLBitNet, SNLNetComponent*, SNLNetComponents, Components)
GetContainerMethod(SNLBitNet, SNLInstTerm*, SNLInstTerms, InstTerms)
GetContainerMethod(SNLBitNet, SNLBitTerm*, SNLBitTerms, BitTerms)

static PyObject* PySNLBitNet_getType(PySNLBitNet* self) {
  METHOD_HEAD("Net.getType()")
  return PyLong_FromLong((long)selfObject->getType());
}

//LCOV_EXCL_START
static PyObject* PySNLBitNet_getTypeAsString(PySNLBitNet* self) {
  METHOD_HEAD("Net.getTypeAsString()")
  switch (selfObject->getType()) {
    case SNLNet::Type::Standard: return PyUnicode_FromString("Standard");
    case SNLNet::Type::Assign0: return PyUnicode_FromString("Assign0");
    case SNLNet::Type::Assign1: return PyUnicode_FromString("Assign1");
    case SNLNet::Type::Supply0: return PyUnicode_FromString("Supply0");
    case SNLNet::Type::Supply1: return PyUnicode_FromString("Supply1");
    case SNLNet::Type::AssignX: return PyUnicode_FromString("AssignX");
    case SNLNet::Type::AssignZ: return PyUnicode_FromString("AssignZ");
    default: return PyUnicode_FromString("Unknown");
  }
}
//LCOV_EXCL_STOP

PyMethodDef PySNLBitNet_Methods[] = {
  { "getType", (PyCFunction)PySNLBitNet_getType, METH_NOARGS,
    "get the type of this Net."},
  { "getTypeAsString", (PyCFunction)PySNLBitNet_getTypeAsString, METH_NOARGS,
    "get the type of this Net as a string."},
  { "getComponents", (PyCFunction)PySNLBitNet_getComponents, METH_NOARGS,
    "get a container of Net Components."},
  { "getInstTerms", (PyCFunction)PySNLBitNet_getInstTerms, METH_NOARGS,
    "get a container of Net InstTerms."},
  { "getBitTerms", (PyCFunction)PySNLBitNet_getBitTerms, METH_NOARGS,
    "get a container of Net BitTerms."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyObject* PySNLBitNet_Link(SNLBitNet* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busNetBit = dynamic_cast<SNLBusNetBit*>(object)) {
    return PySNLBusNetBit_Link(busNetBit);
  } else if (auto scalarNet = dynamic_cast<SNLScalarNet*>(object)) {
    return PySNLScalarNet_Link(scalarNet);
  }
  setError("Unsupported SNLBitNet subtype");
  return nullptr;
}

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLBitNet)
PyTypeInheritedObjectDefinitions(SNLBitNet, SNLNet)

}
