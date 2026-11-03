// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLLibrary.h"

#include "PyInterface.h"
#include "PyNLDB.h"
#include "PyNLLibraries.h"
#include "PySNLDesign.h"
#include "PySNLDesigns.h"

#include "NLLibrary.h"

using namespace naja::NL;

namespace PYNAJA {

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLLibrary, function)

namespace {

static PyObject* createLibrary(PyObject* args, NLLibrary::Type type) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:NLLibrary.createLibrary", &arg0, &arg1)) {
    setError("malformed NLLibrary create");
    return nullptr;
  }
  NLName name;
  if (arg1) {
    name = NLName(arg1);
  }

  NLLibrary* lib = nullptr;
  TRY
  if (IsPyNLDB(arg0)) {
    lib = NLLibrary::create(PYNLDB_O(arg0), type, name);
  } else if (IsPyNLLibrary(arg0)) {
    lib = NLLibrary::create(PYNLLibrary_O(arg0), type, name);
  } else {
    setError("NLLibrary creator accepts as first argument either a NLDB or a NLLibrary");
    return nullptr;
  }
  NLCATCH
  return PyNLLibrary_Link(lib);
}

}

static PyObject* PyNLLibrary_create(PyObject*, PyObject* args) {
  return createLibrary(args, NLLibrary::Type::Standard);
}

static PyObject* PyNLLibrary_createPrimitives(PyObject*, PyObject* args) {
  return createLibrary(args, NLLibrary::Type::Primitives);
}

static PyObject* PyNLLibrary_getSNLDesign(PyNLLibrary* self, PyObject* arg) {
  SNLDesign* design = nullptr;
  METHOD_HEAD("NLLibrary.getSNLDesign()")
  if (PyUnicode_Check(arg)) {
    const char* name = PyUnicode_AsUTF8(arg);
    design = selfObject->getSNLDesign(NLName(name));
  } else if (PyLong_Check(arg)) {
    int index = PyLong_AsLong(arg);
    design = selfObject->getSNLDesign(index);
  } else {
      setError("invalid number of parameters for getSNLDesign.");
      return nullptr;
  }
  return PySNLDesign_Link(design);
}

GetObjectMethod(NLLibrary, NLDB, getDB)
GetObjectByName(NLLibrary, NLLibrary, getLibrary)

DirectGetNumericMethod(PyNLLibrary_getID, getID, PyNLLibrary, NLLibrary)
DirectGetNLIDMethod(PyNLLibrary_getNLID, PyNLLibrary, NLLibrary)

GetBoolAttribute(NLLibrary, isStandard)
GetBoolAttribute(NLLibrary, isPrimitives)

SetNameMethod(NLLibrary)
GetNameMethod(NLLibrary)

GetContainerMethod(NLLibrary, NLLibrary*, NLLibraries, Libraries)
GetContainerMethod(NLLibrary, SNLDesign*, SNLDesigns, SNLDesigns)

DBoDeallocMethod(NLLibrary)

DBoLinkCreateMethod(NLLibrary)
PyTypeObjectDefinitions(NLLibrary)

static PyObject* PyNLLibrary_getContentHash(PyNLLibrary* self) {
  METHOD_HEAD("NLLibrary.getContentHash()")
  TRY
  return PyUnicode_FromString(toHex(selfObject->getContentHash()).c_str());
  NLCATCH
  return nullptr; // LCOV_EXCL_LINE Unreachable: the success path and every NLCATCH handler return.
}

PyMethodDef PyNLLibrary_Methods[] = {
  { "getContentHash", (PyCFunction)PyNLLibrary_getContentHash, METH_NOARGS,
    "Return the versioned content SHA-256 as lowercase hex."},

  { "create", (PyCFunction)PyNLLibrary_create, METH_VARARGS|METH_STATIC,
    "NLLibrary creator"},
  {"setName", (PyCFunction)PyNLLibrary_setName, METH_O,
    "Set the NLName of this NLLibrary."},
  { "createPrimitives", (PyCFunction)PyNLLibrary_createPrimitives, METH_VARARGS|METH_STATIC,
    "Primitives NLLibrary creator"},
  { "getName", (PyCFunction)PyNLLibrary_getName, METH_NOARGS,
    "get NLLibrary name"},
  { "isStandard", (PyCFunction)PyNLLibrary_isStandard, METH_NOARGS,
    "is this a standard NLLibrary?"},
  { "isPrimitives", (PyCFunction)PyNLLibrary_isPrimitives, METH_NOARGS,
    "is this a primitives NLLibrary?"},
  { "getID", (PyCFunction)PyNLLibrary_getID, METH_NOARGS,
    "get the ID."},
  { "getNLID", (PyCFunction)PyNLLibrary_getNLID, METH_NOARGS,
    "get the NLID."},
  { "getDB", (PyCFunction)PyNLLibrary_getDB, METH_VARARGS,
    "get Parent DB."},
  { "getLibrary", (PyCFunction)PyNLLibrary_getLibrary, METH_VARARGS,
    "retrieve a NLLibrary."},
  { "getLibraries", (PyCFunction)PyNLLibrary_getLibraries, METH_NOARGS,
    "iterate over the child NLLibraries."},
  { "getSNLDesign", (PyCFunction)PyNLLibrary_getSNLDesign, METH_O,
    "retrieve a SNLDesign."},
  { "getSNLDesigns", (PyCFunction)PyNLLibrary_getSNLDesigns, METH_NOARGS,
    "get a container of SNLDesigns."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLFinalObjectWithNLIDLinkPyType(NLLibrary)

}
