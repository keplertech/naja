// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLOccurrence.h"
#include "PySNLTimingAnalysis.h"
#include "PyInterface.h"
#include "PySNLDesign.h"
#include "SNLDesignObject.h"
#include "PySNLDesignObject.h"
#include "PySNLNet.h"
#include "PySNLNetComponent.h"
#include "SNLNet.h"
#include "SNLNetComponent.h"
#include "PySNLPath.h"
#include "PySNLInstance.h"
#include "PySNLInstTerm.h"
#include "SNLDesignObject.h"
#include "SNLInstance.h"
#include "SNLPath.h"
#include "SNLOccurrence.h"
#include "SNLDesignModeling.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) \
  if (!PySNLTimingAnalysis_checkOccurrence(reinterpret_cast<PyObject*>(self))) return nullptr; \
  GENERIC_METHOD_HEAD(SNLOccurrence, function)

namespace {
PyObject* occurrenceOwners(const SNLOccurrence& occurrence) {
  auto path = occurrence.getPath();
  auto* tuple = PyTuple_New(path.size() + (occurrence.isValid() ? 1 : 0));
  if (!tuple) return nullptr;
  Py_ssize_t index = 0;
  if (auto* object = occurrence.getObject()) {
    PyObject* linked = nullptr;
    if (auto* component = dynamic_cast<SNLNetComponent*>(object)) linked = PySNLNetComponent_Link(component);
    else if (auto* net = dynamic_cast<SNLNet*>(object)) linked = PySNLNet_Link(net);
    else if (auto* instance = dynamic_cast<SNLInstance*>(object)) linked = PySNLInstance_Link(instance);
    if (!linked) {
      // Known object families are dispatched above; this requires allocation
      // failure or an unsupported native SNLDesignObject subclass.
      // LCOV_EXCL_START
      if (!PyErr_Occurred()) setError("Unsupported occurrence object");
      Py_DECREF(tuple); return nullptr;
      // LCOV_EXCL_STOP
    }
    PyTuple_SET_ITEM(tuple, index++, linked);
  }
  for (auto* instance: path.getInstances()) {
    auto* linked = PySNLInstance_Link(instance);
    if (!linked) { Py_DECREF(tuple); return nullptr; }
    PyTuple_SET_ITEM(tuple, index++, linked);
  }
  return tuple;
}
}
bool PySNLOccurrence_isAlive(PyObject* object) {
  if (!IsPySNLOccurrence(object)) return true;
  auto* self = PYSNLOccurrence(object);
  if (!self->owners_) return true;
  for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(self->owners_); ++i) {
    if (!PYSNLDesignObject_O(PyTuple_GET_ITEM(self->owners_, i))) {
      setError("Occurrence references a destroyed netlist object"); return false;
    }
  }
  return true;
}
int PySNLOccurrence_set(PySNLOccurrence* self, const SNLOccurrence& occurrence) {
  auto* owners = occurrenceOwners(occurrence);
  if (!owners) return -1;
  SNLOccurrence* copy = nullptr;
  try { copy = new SNLOccurrence(occurrence); }
  catch (...) { Py_DECREF(owners); PyErr_NoMemory(); return -1; } // LCOV_EXCL_LINE C++ allocation failure while copying an occurrence.
  delete self->object_;
  Py_XDECREF(self->owners_);
  self->object_ = copy;
  self->owners_ = owners;
  self->generation_ = pythonNetlistGeneration();
  self->hasPath_ = !occurrence.getPath().empty();
  return 0;
}

static int PySNLOccurrence_Init(PySNLOccurrence* self, PyObject* args, PyObject* kwargs) {
  SNLOccurrence* snlOccurrence = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLOccurrence has three types of constructors:
  if (not PyArg_ParseTuple(args, "|OO:SNLOccurrence", &arg0, &arg1)) {
    setError("malformed SNLOccurrence create method");
    return -1;
  }
  if (arg0 == nullptr) {
    snlOccurrence = new SNLOccurrence();
  } else if (arg1 == nullptr) {
    if (IsPySNLDesignObject(arg0)) {
      snlOccurrence = new SNLOccurrence(PYSNLDesignObject_O(arg0));
    } else {
      setError("SNLOccurrence create accepts SNLDesignObject as only argument");
      return -1;
    }
  } else if (IsPySNLPath(arg0) and IsPySNLDesignObject(arg1)) {
    snlOccurrence = new SNLOccurrence(*PYSNLPath_O(arg0), PYSNLDesignObject_O(arg1));
  }  else {
    setError("invalid number of parameters for Occurrence constructor.");
    return -1;
  }
  int status = PySNLOccurrence_set(self, *snlOccurrence);
  delete snlOccurrence;
  return status;
}

PyObject* PySNLOccurrence_Link(const SNLOccurrence& occurrence) {
  auto* object = reinterpret_cast<PySNLOccurrence*>(PyTypeSNLOccurrence.tp_alloc(&PyTypeSNLOccurrence, 0));
  if (!object) return nullptr;
  if (PySNLOccurrence_set(object, occurrence) < 0) { Py_DECREF(object); return nullptr; }
  return reinterpret_cast<PyObject*>(object);
}
static void PySNLOccurrence_DeAlloc(PySNLOccurrence* self) {
  delete self->object_;
  Py_XDECREF(self->owners_);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static Py_uhash_t hashSNLPathForOccurrence(const SNLPath& path) {
  Py_uhash_t seed = 0;
  for (auto instance: path.getInstances()) {
    PYNAJA::combinePyHash(seed, PYNAJA::hashNLID(instance->getNLID()));
  }
  return seed;
}

static Py_uhash_t hashSNLOccurrence(const SNLOccurrence& occurrence) {
  Py_uhash_t seed = 0;
  auto object = occurrence.getObject();
  if (object) {
    PYNAJA::combinePyHash(seed, PYNAJA::hashNLID(object->getNLID()));
  }
  PYNAJA::combinePyHash(seed, hashSNLPathForOccurrence(occurrence.getPath()));
  return seed;
}

static Py_hash_t PySNLOccurrence_Hash(PySNLOccurrence* self) {
  if (!PySNLTimingAnalysis_checkOccurrence(reinterpret_cast<PyObject*>(self))) return -1;
  if (not self->ACCESS_OBJECT) {
    setError("Attempt to call SNLOccurrence.__hash__() on an unbound object");
    return -1;
  }
  return PYNAJA::finishPyHash(hashSNLOccurrence(*self->ACCESS_OBJECT));
}

GetObjectMethod(SNLOccurrence, SNLNetComponent, getNetComponent)
GetObjectMethod(SNLOccurrence, SNLInstTerm, getInstTerm)
GetObjectMethod(SNLOccurrence, SNLInstance, getInstance)
GetObjectMethod(SNLOccurrence, SNLDesign, getDesign) // LCOV_EXCL_LINE
GetObjectMethod(SNLOccurrence, SNLPath, getPath)
GetBoolAttribute(SNLOccurrence, isInstanceOccurrence)

static PyObject* PySNLOccurrence_getRole(PySNLOccurrence* self) {
  METHOD_HEAD("SNLOccurrence.getRole()")
  TRY
  const auto role = selfObject->getInstTerm()
      ? SNLDesignModeling::getTermRole(selfObject->getInstTerm())
      : SNLDesignModeling::getTermRole(selfObject->getBitTerm());
  return PyLong_FromLong(static_cast<long>(role));
  NLCATCH
}

static PyObject* PySNLOccurrence_getResetActiveLevel(PySNLOccurrence* self) {
  METHOD_HEAD("SNLOccurrence.getResetActiveLevel()")
  TRY
  const auto level = selfObject->getInstTerm()
      ? SNLDesignModeling::getResetActiveLevel(selfObject->getInstTerm())
      : SNLDesignModeling::getResetActiveLevel(selfObject->getBitTerm());
  return PyLong_FromLong(static_cast<long>(level));
  NLCATCH
}

static PyObject* PySNLOccurrence_getLaunchClocks(PyObject* self, PyObject*) { return PySNLOccurrence_getTimingClocks(self, 0); }
static PyObject* PySNLOccurrence_getCaptureClocks(PyObject* self, PyObject*) { return PySNLOccurrence_getTimingClocks(self, 1); }
static PyObject* PySNLOccurrence_getClocks(PyObject* self, PyObject*) { return PySNLOccurrence_getTimingClocks(self, 2); }
static PyObject* PySNLOccurrence_getFaninStartpoints(PyObject* self, PyObject* args, PyObject* kwargs) { return PySNLOccurrence_getTimingCone(self, args, kwargs, false); }
static PyObject* PySNLOccurrence_getFanoutEndpoints(PyObject* self, PyObject* args, PyObject* kwargs) { return PySNLOccurrence_getTimingCone(self, args, kwargs, true); }

PyMethodDef PySNLOccurrence_Methods[] = {
  {"getFaninStartpoints", (PyCFunction)PySNLOccurrence_getFaninStartpoints, METH_VARARGS|METH_KEYWORDS, "getFaninStartpoints(withCells=True): launch-view startpoints, cell unions and directness."},
  {"getFanoutEndpoints", (PyCFunction)PySNLOccurrence_getFanoutEndpoints, METH_VARARGS|METH_KEYWORDS, "getFanoutEndpoints(withCells=True): launch-view stop pins and associated register outputs."},
  {"getLaunchClocks", (PyCFunction)PySNLOccurrence_getLaunchClocks, METH_NOARGS, "Return launch domains reaching this terminal or bit net."},
  {"getCaptureClocks", (PyCFunction)PySNLOccurrence_getCaptureClocks, METH_NOARGS, "Return downstream data-capture domains, stopping at storage."},
  {"getClocks", (PyCFunction)PySNLOccurrence_getClocks, METH_NOARGS, "Return propagated clock-waveform origins at this occurrence."},
  {"getRole", (PyCFunction)PySNLOccurrence_getRole, METH_NOARGS,
    "get the role of the referenced term in its instance context (Other for non-terms)."},
  {"getResetActiveLevel", (PyCFunction)PySNLOccurrence_getResetActiveLevel, METH_NOARGS,
    "get the reset/set active level in its instance context."},
  { "getNetComponent", (PyCFunction)PySNLOccurrence_getNetComponent, METH_NOARGS,
    "get the SNLNetComponent of the SNLOccurrence."},
  { "getInstTerm", (PyCFunction)PySNLOccurrence_getInstTerm, METH_NOARGS,
    "get the SNLInstTerm of the SNLOccurrence."},
  { "getInstance", (PyCFunction)PySNLOccurrence_getInstance, METH_NOARGS,
    "get the SNLInstance of the SNLOccurrence (None if the object is not an instance)."},
  { "getDesign", (PyCFunction)PySNLOccurrence_getDesign, METH_NOARGS,
    "get the design context of the SNLOccurrence."},
  { "isInstanceOccurrence", (PyCFunction)PySNLOccurrence_isInstanceOccurrence, METH_NOARGS,
    "return whether the SNLOccurrence references an SNLInstance."},
  { "getPath", (PyCFunction)PySNLOccurrence_getPath, METH_NOARGS,
    "get the SNLPath of the SNLInstTermOccurrence."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedNLObjectWithoutNLIDLinkPyTypeWithHash(SNLOccurrence, PySNLOccurrence_Hash)
PyTypeObjectDefinitions(SNLOccurrence)

}  // namespace PYNAJA
