// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNet.h"

#include "SNLScalarNet.h"
#include "SNLBusNet.h"

#include "PyInterface.h"
#include "PySNLNetType.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNet.h"
#include "PySNLDesign.h"
#include "PySNLBitNets.h"

namespace PYNAJA {

using namespace naja::NL;

#undef ACCESS_OBJECT
#undef ACCESS_CLASS
#define ACCESS_OBJECT            parent_.object_
#define ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLNet, function)

GetNameMethod(SNLNet)
GetContainerMethodWithMethodName(Net, BitNet, getBits)

PyObject* PySNLNet_Link(SNLNet* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busNet = dynamic_cast<SNLBusNet*>(object)) {
    return PySNLBusNet_Link(busNet);
  } else {
    auto scalarNet = static_cast<SNLScalarNet*>(object);
    return PySNLScalarNet_Link(scalarNet);
  }
}

static PyObject* setType(PySNLNet* self, PyObject* arg) {
  METHOD_HEAD("SNLNet.setType()")

  if (PyLong_Check(arg)) {
    int intType = PyLong_AsUnsignedLong(arg);
    SNLNet::Type type = SNLNet::Type::TypeEnum(intType);
    selfObject->setType(type);
  } else {
    setError("SNLNet setType takes SNLNet.Type argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

PyTypeInheritedObjectDefinitions(SNLNet, SNLDesignObject)

DirectGetNumericMethod(PySNLNet_getWidth, getWidth, PySNLNet, SNLNet)
DirectGetNumericMethod(PySNLNet_getID, getID, PySNLNet, SNLNet)
GetBoolAttribute(SNLNet, isConstant0)
GetBoolAttribute(SNLNet, isConstant1)
GetBoolAttribute(SNLNet, isConstantX)
GetBoolAttribute(SNLNet, isConstantZ)
GetBoolAttribute(SNLNet, isConstant)

PyMethodDef PySNLNet_Methods[] = {
  { "getID", (PyCFunction)PySNLNet_getID, METH_NOARGS,
    "get SNLNet design-object ID"},
  { "getName", (PyCFunction)PySNLNet_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getWidth", (PyCFunction)PySNLNet_getWidth, METH_NOARGS,
    "get SNLNet width"},
  { "getBits", (PyCFunction)PySNLNet_getBits, METH_NOARGS,
    "get a container of SNLBitNets."},
  { "setType", (PyCFunction)setType, METH_O,
    "set the type of this Net."},
  { "isConstant0", (PyCFunction)PySNLNet_isConstant0, METH_NOARGS,
    "Returns True if this SNLNet is a Constant 0."},
  { "isConstant1", (PyCFunction)PySNLNet_isConstant1, METH_NOARGS,
    "Returns True if this SNLNet is a Constant 1."},
  { "isConstantX", (PyCFunction)PySNLNet_isConstantX, METH_NOARGS,
    "Returns True if this SNLNet is a Constant X."},
  { "isConstantZ", (PyCFunction)PySNLNet_isConstantZ, METH_NOARGS,
    "Returns True if this SNLNet is a Constant Z."},
  { "isConstant", (PyCFunction)PySNLNet_isConstant, METH_NOARGS,
    "Returns True if this SNLNet is a Constant."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLNet)

void PySNLNet_postModuleInit() {
  PySNLNetType_postModuleInit();
  PyDict_SetItemString(PyTypeSNLNet.tp_dict, "Type", (PyObject*)&PyTypeSNLNetType);
}

}
