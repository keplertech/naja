// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstance.h"

#include "SNLInstance.h"
#include "SNLInstTerm.h"

#include "PyInterface.h"

#include "PySNLDesign.h"
#include "PySNLSequentialModel.h"
#include "PySNLMemoryInterface.h"
#include "PySNLInstParameter.h"
#include "PySNLInstTerm.h"
#include "PySNLBitTerm.h"
#include "PySNLInstTerms.h"
#include "PySNLInstParameters.h"

#include "SNLDesignModeling.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstance, function)

static PyObject* PySNLInstance_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "OO|s:SNLDB.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLInstance create method");
    return nullptr;
  }
  NLName name;
  if (arg2) {
    name = NLName(arg2);
  }

  SNLInstance* instance = nullptr;
  TRY
  if (not IsPySNLDesign(arg0)) {
    setError("SNLInstance create needs SNLDesign as first argument");
    return nullptr;
  }
  if (not IsPySNLDesign(arg1)) {
    setError("SNLInstance create needs SNLDesign as second argument");
    return nullptr;
  }
  instance = SNLInstance::create(PYSNLDesign_O(arg0), PYSNLDesign_O(arg1), name);
  NLCATCH
  return PySNLInstance_Link(instance);
}

GetObjectMethod(SNLInstance, SNLDesign, getModel)
GetObjectByName(SNLInstance, SNLInstParameter, getInstParameter)

static PyObject* PySNLInstance_getCombinatorialInputs(PySNLDesign*, PyObject* output) {
  if (IsPySNLInstTerm(output)) {
    auto outputITerm = PYSNLInstTerm_O(output);
    PySNLInstTerms* pyObjects = nullptr;
    TRY
    auto objects = new naja::NajaCollection<SNLInstTerm*>(SNLDesignModeling::getCombinatorialInputs(outputITerm));
    pyObjects = PyObject_NEW(PySNLInstTerms, &PyTypeSNLInstTerms);
    if (not pyObjects) return nullptr;
    pyObjects->object_ = objects;
    NLCATCH
    return (PyObject*)pyObjects;
  }
  setError("malformed SNLInstance.getCombinatorialInputs method");
  return nullptr;
}

static PyObject* PySNLInstance_getCombinatorialOutputs(PySNLDesign*, PyObject* input) {
  if (IsPySNLInstTerm(input)) {
    auto inputITerm = PYSNLInstTerm_O(input);
    PySNLInstTerms* pyObjects = nullptr;
    TRY
    auto objects = new naja::NajaCollection<SNLInstTerm*>(SNLDesignModeling::getCombinatorialOutputs(inputITerm));
    pyObjects = PyObject_NEW(PySNLInstTerms, &PyTypeSNLInstTerms);
    if (not pyObjects) return nullptr;
    pyObjects->object_ = objects;
    NLCATCH
    return (PyObject*)pyObjects;
  }
  setError("malformed SNLInstance.getCombinatorialOutputs method");
  return nullptr;
}

#define INSTANCE_MODELING_RELATED(GETTER)                              \
  static PyObject* PySNLInstance_##GETTER(                             \
      PySNLInstance*, PyObject* object) {                              \
    GetDesignModelingRelatedObjects(                                  \
      SNLInstTerm, GETTER, SNLInstance)                               \
  }

INSTANCE_MODELING_RELATED(getClockRelatedInputs)
INSTANCE_MODELING_RELATED(getClockRelatedOutputs)
INSTANCE_MODELING_RELATED(getInputRelatedClocks)
INSTANCE_MODELING_RELATED(getOutputRelatedClocks)
#undef INSTANCE_MODELING_RELATED

GetNameMethod(SNLInstance)

DBoLinkCreateMethod(SNLInstance)
DBoDeallocMethod(SNLInstance)

PyTypeInheritedObjectDefinitions(SNLInstance, SNLDesignObject)

static PyObject* PySNLInstance_getInstTerm(PySNLInstance* self, PyObject* args) {
  SNLInstTerm* obj = nullptr;
  METHOD_HEAD("SNLInstance.getInstTerm()")
  PySNLBitTerm* pyBitTerm = nullptr;
  if (PyArg_ParseTuple(args, "O!:SNLInstance.getInstTerm", &PyTypeSNLBitTerm, &pyBitTerm)) {
    TRY
    auto bitTerm = PYSNLBitTerm_O(pyBitTerm);
    if (bitTerm) {
      obj = selfObject->getInstTerm(bitTerm);
    }
    NLCATCH
  } else {
    setError("invalid number of parameters for getInstTerm.");
    return nullptr;
  }
  return PySNLInstTerm_Link(obj);
}

GetContainerMethod(SNLInstance, SNLInstTerm*, SNLInstTerms, InstTerms)
GetContainerMethod(SNLInstance, SNLInstParameter*, SNLInstParameters, InstParameters)

DirectGetNumericMethod(PySNLInstance_getID, getID, PySNLInstance, SNLInstance)

static PyObject* PySNLInstance_getContentHash(PySNLInstance* self) {
  METHOD_HEAD("SNLInstance.getContentHash()")
  TRY
  return PyUnicode_FromString(toHex(selfObject->getContentHash()).c_str());
  NLCATCH
  return nullptr;
}

PyMethodDef PySNLInstance_Methods[] = {
  { "getContentHash", (PyCFunction)PySNLInstance_getContentHash, METH_NOARGS,
    "Return the versioned content SHA-256 as lowercase hex."},

  { "getMemoryInterface", (PyCFunction)PySNLInstance_getMemoryInterface, METH_NOARGS,
    "Return the memory interface dictionary; raise RuntimeError if absent."},
  { "getSequentialModel", (PyCFunction)PySNLInstance_getSequentialModel, METH_NOARGS,
    "Resolve the instance sequential model as expression trees."},
  { "create", (PyCFunction)PySNLInstance_create, METH_VARARGS|METH_STATIC,
    "SNLInstance creator"},
  { "getName", (PyCFunction)PySNLInstance_getName, METH_NOARGS,
    "get SNLInstance name"},
  { "getID", (PyCFunction)PySNLInstance_getID, METH_NOARGS,
    "get the ID."},
  {"getModel", (PyCFunction)PySNLInstance_getModel, METH_NOARGS,
    "Returns the SNLInstance model SNLDesign."},
  {"getInstParameter", (PyCFunction)PySNLInstance_getInstParameter, METH_VARARGS,
    "Returns the SNLInstParameter by name."},
  {"getInstTerm", (PyCFunction)PySNLInstance_getInstTerm, METH_VARARGS,
    "Returns the SNLInstTerm corresponding to a model's SNLBitTerm."},
  {"getInstTerms", (PyCFunction)PySNLInstance_getInstTerms, METH_NOARGS,
    "get a container of SNLInstTerms."},
  {"getInstParameters", (PyCFunction)PySNLInstance_getInstParameters, METH_NOARGS,
    "get a container of SNLInstParameters."},
  { "getCombinatorialInputs", (PyCFunction)PySNLInstance_getCombinatorialInputs, METH_O|METH_STATIC,
    "get combinatorial inputs of an instance term"},
  { "getCombinatorialOutputs", (PyCFunction)PySNLInstance_getCombinatorialOutputs, METH_O|METH_STATIC,
    "get combinatorial outputs of an instance term"},
  { "getClockRelatedInputs", (PyCFunction)PySNLInstance_getClockRelatedInputs, METH_O|METH_STATIC,
    "get inputs related to a clock instance term"},
  { "getClockRelatedOutputs", (PyCFunction)PySNLInstance_getClockRelatedOutputs, METH_O|METH_STATIC,
    "get outputs related to a clock instance term"},
  { "getInputRelatedClocks", (PyCFunction)PySNLInstance_getInputRelatedClocks, METH_O|METH_STATIC,
    "get clocks related to an input instance term"},
  { "getOutputRelatedClocks", (PyCFunction)PySNLInstance_getOutputRelatedClocks, METH_O|METH_STATIC,
    "get clocks related to an output instance term"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLFinalObjectWithNLIDLinkPyType(SNLInstance)

}
