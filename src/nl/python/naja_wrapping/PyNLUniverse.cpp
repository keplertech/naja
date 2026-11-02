// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLUniverse.h"

#include "PyInterface.h"
#include "PyNLID.h"
#include "PyNLDB.h"
#include "PyNLDBs.h"
#include "PyNLLibrary.h"
#include "PySNLDesign.h"
#include "PySNLInstance.h"
#include "PySNLInstTerm.h"
#include "PySNLBitNet.h"
#include "PySNLBitTerm.h"

#include "NajaLog.h"
#include "DNL.h"
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLObject.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLUtils.h"
#include "SNLSVConstructor.h"
#include "RemoveLoadlessLogic.h"
#include "ConstantPropagation.h"
#include "FanoutComputer.h"
#include "LogicLevelComputer.h"
#include "SNLOccurrence.h"
#include "PySNLOccurrence.h"

namespace PYNAJA {

using namespace naja::NL;
using namespace naja::NAJA_OPT;
using namespace naja::NAJA_METRICS;

namespace {

long long getCountDelta(size_t before, size_t after) {
  return static_cast<long long>(after) - static_cast<long long>(before);
}

SNLUtils::InstanceCount countTopDesignInstances() {
  auto universe = NLUniverse::get();
  return SNLUtils::countReachableInstances(
      universe == nullptr ? nullptr : universe->getTopDesign());
}

void logInstanceCountDelta(
    const char* label,
    const SNLUtils::InstanceCount& before,
    const SNLUtils::InstanceCount& after) {
  NAJA_LOG_INFO(
      "{} instance count:\n"
      "  unfolded: total {} -> {} (delta {}), leaf {} -> {} (delta {})\n"
      "  folded: total {} -> {} (delta {}), leaf {} -> {} (delta {})\n"
      "  reachable models: {} -> {}",
      label,
      before.totalInstances,
      after.totalInstances,
      getCountDelta(before.totalInstances, after.totalInstances),
      before.leafInstances,
      after.leafInstances,
      getCountDelta(before.leafInstances, after.leafInstances),
      before.foldedTotalInstances,
      after.foldedTotalInstances,
      getCountDelta(before.foldedTotalInstances, after.foldedTotalInstances),
      before.foldedLeafInstances,
      after.foldedLeafInstances,
      getCountDelta(before.foldedLeafInstances, after.foldedLeafInstances),
      before.reachableModels,
      after.reachableModels);
}

}  // namespace

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLUniverse, function)

static PyObject* PyNLUniverse_create() {
  NLUniverse* universe = nullptr;
  TRY
  universe = NLUniverse::create();
  NLCATCH
  return PyNLUniverse_Link(universe);
}

static PyObject* PyNLUniverse_get() {
  auto universe = NLUniverse::get();
  return PyNLUniverse_Link(universe);
}

static PyObject* PyNLUniverse_applyDLE() {
  const auto beforeInstanceCount = countTopDesignInstances();
  LoadlessLogicRemover remover;
  remover.setNormalizedUniquification(true);
  remover.process();
  const auto afterInstanceCount = countTopDesignInstances();
  logInstanceCountDelta("DLE optimization", beforeInstanceCount, afterInstanceCount);
  Py_RETURN_NONE;
}

// fanout calculation
static PyObject* PyNLUniverse_getMaxFanout() {
  FanoutComputer fanoutComputer;
  fanoutComputer.process();
  const auto& fanouts = fanoutComputer.getMaxFanoutTerms();
  std::vector<std::pair<SNLOccurrence, std::vector<SNLOccurrence>>> pyFanouts;
  for (const auto& [termID, readerIDs] : fanouts) {
    const DNLTerminalFull& term = naja::DNL::get()->getDNLTerminalFromID(termID);
    SNLOccurrence pyTerm = term.getOccurrence();
    std::vector<SNLOccurrence> pyReaders;
    for (const auto& readerID : readerIDs) {
      const DNLTerminalFull& readerTerm = naja::DNL::get()->getDNLTerminalFromID(readerID);
      pyReaders.push_back(readerTerm.getOccurrence());
    }
    pyFanouts.push_back({pyTerm, pyReaders});
  }
  // return a pair (maxFanout, terms(with PySNLOccurrence))
  PyObject* py_list = PyList_New(2);
  PyList_SetItem(py_list, 0, PyLong_FromSize_t(fanoutComputer.getMaxFanout()));
  PyObject* py_terms_list = PyList_New(pyFanouts.size());
  for (size_t i = 0; i < pyFanouts.size(); i++) {
    PyObject* readers_list = PyList_New(pyFanouts[i].second.size());
    for (size_t j = 0; j < pyFanouts[i].second.size(); j++) {
      PyList_SetItem(readers_list, j, PySNLOccurrence_Link(pyFanouts[i].second[j]));
    }
    PyObject* term_and_readers = PyTuple_New(2);
    PyTuple_SetItem(term_and_readers, 0, PySNLOccurrence_Link(pyFanouts[i].first));
    PyTuple_SetItem(term_and_readers, 1, readers_list);
    PyList_SetItem(py_terms_list, i, term_and_readers);
  }
  PyList_SetItem(py_list, 1, py_terms_list);
  return py_list;
}

static PyObject* PyNLUniverse_getMaxLogicLevel() {
  LogicLevelComputer logicLevelComputer;
  logicLevelComputer.process();
  const std::vector<std::vector<std::pair<DNLID, DNLID>>>& paths = logicLevelComputer.getMaxLogicLevelPaths();
  std::vector<std::vector<SNLOccurrence>> pyPaths;
  for (const auto& path : paths) {
    std::vector<SNLOccurrence> pyPath;
    for (const auto& [fromTermID, toTermID] : path) {
      if (fromTermID != naja::DNL::DNLID_MAX) {
        const DNLTerminalFull& fromTerm = naja::DNL::get()->getDNLTerminalFromID(fromTermID);
        // only one term in the path
        pyPath.push_back(fromTerm.getOccurrence());
      }
      if (toTermID != naja::DNL::DNLID_MAX) {
        const DNLTerminalFull& toTerm = naja::DNL::get()->getDNLTerminalFromID(toTermID);
        pyPath.push_back(toTerm.getOccurrence());
      }
    }
    pyPaths.push_back(pyPath);
  }
  // return a pair (maxLogicLevel, paths(with PySNLOccurrence))
  PyObject* py_list = PyList_New(2);
  PyList_SetItem(py_list, 0, PyLong_FromSize_t(logicLevelComputer.getMaxLogicLevel()));
  PyObject* py_paths_list = PyList_New(pyPaths.size());
  for (size_t i = 0; i < pyPaths.size(); i++) {
    PyObject* path_list = PyList_New(pyPaths[i].size());
    for (size_t j = 0; j < pyPaths[i].size(); j++) {
      PyList_SetItem(path_list, j, PySNLOccurrence_Link(pyPaths[i][j]));
    }
    PyList_SetItem(py_paths_list, i, path_list);
  }
  PyList_SetItem(py_list, 1, py_paths_list);
  return py_list;
}

static PyObject* PyNLUniverse_applyConstantPropagation() {
  const auto beforeInstanceCount = countTopDesignInstances();
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);
  cp.run();
  const auto afterInstanceCount = countTopDesignInstances();
  logInstanceCountDelta(
      "Constant propagation", beforeInstanceCount, afterInstanceCount);
  Py_RETURN_NONE;
}

static PyObject* PyNLUniverse_setTopDesign(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.setTopDesign()")
  if (IsPySNLDesign(arg)) {
    selfObject->setTopDesign(PYSNLDesign_O(arg));
  } else {
    setError("NLUniverse setTopDesign takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PyNLUniverse_setTopDB(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.setTopDB()")
  if (IsPyNLDB(arg)) {
    selfObject->setTopDB(PYNLDB_O(arg));
  } else {
    setError("NLUniverse setTopDB takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* pyNLUniverse_getSNLDesign(PyNLUniverse* self, PyObject* arg) {
  PyObject* arg0;
  SNLDesign* design = nullptr;
  METHOD_HEAD("NLUniverse.getSNLDesign()")

  if (!PyArg_ParseTuple(arg, "O:NLUniverse.getSNLDesign", &arg0)) {
    PyErr_SetString(PyExc_TypeError, "malformed NLUniverse getSNLDesign method");
    return nullptr;
  }

  if (PyUnicode_Check(arg0)) {
    const char* name = PyUnicode_AsUTF8(arg0);
    design = selfObject->getSNLDesign(NLName(name));
    return PySNLDesign_Link(design);
  } else if (PyTuple_Check(arg0) && PyTuple_Size(arg0) == 3) {
    PyObject* db_id_obj = PyTuple_GetItem(arg0, 0);
    PyObject* lib_id_obj = PyTuple_GetItem(arg0, 1);
    PyObject* design_id_obj = PyTuple_GetItem(arg0, 2);

    if (not PyLong_Check(db_id_obj)
      or not PyLong_Check(lib_id_obj)
      or not PyLong_Check(design_id_obj)) {
      PyErr_SetString(PyExc_TypeError, "Tuple must contain three integers");
      return nullptr;
    }

    NLID::DBID db_id = (NLID::DBID)PyLong_AsLong(db_id_obj);
    NLID::LibraryID lib_id = (NLID::LibraryID)PyLong_AsLong(lib_id_obj);
    NLID::DesignID design_id = (NLID::DesignID)PyLong_AsLong(design_id_obj);
    //PySys_WriteStderr("Looking for design with db_id=%d lib_id=%d design_id=%d\n", db_id, lib_id, design_id);
            
    design = selfObject->getSNLDesign(NLID::DesignReference(db_id, lib_id, design_id));
  } else {
    setError("NLUniverse getSNLDesign takes a string (design name) or a tuple of three integers (dbID, libID, designID)");
    return nullptr;
  }
  return PySNLDesign_Link(design);
}

static PyObject* PyNLUniverse_getObject(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.getObject()")
  if (not IsPyNLID(arg)) {
    setError("NLUniverse.getObject expects an NLID argument");
    return nullptr;
  }

  NLObject* object = selfObject->getObject(PYNLID_O(arg));
  if (not object) {
    Py_RETURN_NONE;
  }
  if (auto db = dynamic_cast<NLDB*>(object)) {
    return PyNLDB_Link(db);
  }
  if (auto library = dynamic_cast<NLLibrary*>(object)) {
    return PyNLLibrary_Link(library);
  }
  if (auto design = dynamic_cast<SNLDesign*>(object)) {
    return PySNLDesign_Link(design);
  }
  if (auto instance = dynamic_cast<SNLInstance*>(object)) {
    return PySNLInstance_Link(instance);
  }
  if (auto instTerm = dynamic_cast<SNLInstTerm*>(object)) {
    return PySNLInstTerm_Link(instTerm);
  }
  if (auto bitNet = dynamic_cast<SNLBitNet*>(object)) {
    return PySNLBitNet_Link(bitNet);
  }
  if (auto bitTerm = dynamic_cast<SNLBitTerm*>(object)) {
    return PySNLBitTerm_Link(bitTerm);
  }
  Py_RETURN_NONE;
}

GetObjectMethod(NLUniverse, SNLDesign, getTopDesign)
GetObjectMethod(NLUniverse, NLDB, getTopDB)
GetObjectByIndex(NLUniverse, NLDB, DB)
GetContainerMethod(NLUniverse, NLDB*, NLDBs, UserDBs)

static PyObject* PyNLUniverse_destroy(PyNLUniverse* self) {
  TRY
  if (self->object_ == nullptr) {
    setError("applying a destroy() to a Python object with no Hurricane object attached");
    return nullptr;
  }
  naja::NajaPythonProperty* proxy = static_cast<naja::NajaPythonProperty*>(
    self->object_->getProperty(naja::NajaPythonProperty::getPropertyName()));
  if (proxy == nullptr) { // Defensive: PyNLUniverse_Link always installs the proxy.
    setError("Trying to destroy() a Hurricane object of with no Proxy attached "); // LCOV_EXCL_LINE
    return nullptr; // LCOV_EXCL_LINE
  }
  SNLSVLiveASTLinkRegistry::clearAll();
  naja::DNL::destroy();
  self->object_->destroy();
  self->object_ = nullptr;
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PyNLUniverse_getContentHash(PyNLUniverse* self) {
  METHOD_HEAD("NLUniverse.getContentHash()")
  TRY
  return PyUnicode_FromString(toHex(selfObject->getContentHash()).c_str());
  NLCATCH
  return nullptr;
}

PyMethodDef PyNLUniverse_Methods[] = {
  { "getContentHash", (PyCFunction)PyNLUniverse_getContentHash, METH_NOARGS,
    "Return the versioned content SHA-256 as lowercase hex."},

  { "create", (PyCFunction)PyNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the NLUniverse (static object)"},
  { "destroy", (PyCFunction)PyNLUniverse_destroy, METH_NOARGS,
    "destroy the associated NLUniverse"},
  { "get", (PyCFunction)PyNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the NLUniverse (static object)"},
  { "getTopDesign", (PyCFunction)PyNLUniverse_getTopDesign, METH_NOARGS,
    "get the top SNLDesign"},
  { "setTopDesign", (PyCFunction)PyNLUniverse_setTopDesign, METH_O,
    "set the top SNLDesign"},
  { "getSNLDesign", (PyCFunction)pyNLUniverse_getSNLDesign, METH_VARARGS,
    "get the SNLDesign with the given name or NLID::DesignReference (dbID, libID, designID)"},
  { "getObject", (PyCFunction)PyNLUniverse_getObject, METH_O,
    "get the object with the given NLID"},
  { "setTopDB", (PyCFunction)PyNLUniverse_setTopDB, METH_O,
    "set the top NLDB"},
  { "getTopDB", (PyCFunction)PyNLUniverse_getTopDB, METH_NOARGS,
    "get the Top NLDB"},
  { "getDB", (PyCFunction)PyNLUniverse_getDB, METH_VARARGS,
    "get the NLDB with the given index"},
  { "getUserDBs", (PyCFunction)PyNLUniverse_getUserDBs, METH_NOARGS,
    "iterate on User NLDBs."},
  { "applyDLE", (PyCFunction)PyNLUniverse_applyDLE, METH_NOARGS|METH_STATIC,
   "apply Dead Logic Elimination on the top design of the NLUniverse."},
  { "applyConstantPropagation", (PyCFunction)PyNLUniverse_applyConstantPropagation, METH_NOARGS|METH_STATIC,
    "apply Constant Propagation on the top design of the NLUniverse."},
  { "getMaxFanout", (PyCFunction)PyNLUniverse_getMaxFanout, METH_NOARGS|METH_STATIC,
    "get the maximum fanout of the top design of the NLUniverse."},
  { "getMaxLogicLevel", (PyCFunction)PyNLUniverse_getMaxLogicLevel, METH_NOARGS|METH_STATIC,
    "get the maximum logic level of the top design of the NLUniverse."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(NLUniverse)

DBoLinkCreateMethod(NLUniverse)
PyTypeNLObjectWithoutNLIDLinkPyType(NLUniverse)
PyTypeObjectDefinitions(NLUniverse)

}
