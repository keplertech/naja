// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesign.h"
#include "PySNLSequentialModel.h"
#include "PySNLMemoryInterface.h"

#include "PyInterface.h"

#include "PyNLDB.h"
#include "PyNLLibrary.h"

#include "PySNLBitTerm.h"
#include "PySNLScalarTerm.h"
#include "PySNLBundleTerm.h"
#include "PySNLBusTerm.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNet.h"
#include "PySNLInstance.h"
#include "PySNLParameter.h"
#include "PyNLClock.h"
#include "PyNLClocks.h"
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
#include "PySNLInstTerms.h"
#include "PySNLScalarTerms.h"
#include "PySNLBusTerms.h"
#include "PySNLBundleTerms.h"
#include "PySNLNets.h"
#include "PySNLScalarNets.h"
#include "PySNLBusNets.h"
#include "PySNLBitNets.h"
#include "PySNLInstances.h"
#include "PySNLParameters.h"
#include "PySNLAttributes.h"
#include "PySNLAttribute.h"

#include "SNLDesign.h"
#include "NLDB0.h"
#include "SNLDesignModeling.h"
#include "SNLRTLInfos.h"
#include "PySNLSourceReference.h"
#include "NLBitDependencies.h"
#include "SNLTruthTable.h"
#include "SNLVRLDumper.h"

#include "NetlistGraph.h"

#include <optional>
#include <string>

namespace PYNAJA {

using namespace naja::NL;

namespace {

std::optional<SNLVRLDumper::RTLInfoDumpMode> parseRTLInfoDumpMode(const char* mode) {
  if (mode == nullptr) {
    return std::nullopt; // LCOV_EXCL_LINE
  }
  const std::string modeString(mode);
  if (modeString == "None" || modeString == "none") {
    return SNLVRLDumper::RTLInfoDumpMode::None;
  }
  if (modeString == "VerboseAttributes" || modeString == "verbose_attributes") {
    return SNLVRLDumper::RTLInfoDumpMode::VerboseAttributes;
  }
  if (modeString == "CompactAttribute" || modeString == "compact_attribute") {
    return SNLVRLDumper::RTLInfoDumpMode::CompactAttribute;
  }
  return std::nullopt;
}

}  // namespace

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesign, function)

static PyObject* PySNLDesign_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDesign.create", &arg0, &arg1)) {
    setError("malformed SNLDesign create method");
    return nullptr;
  }
  NLName name;
  if (arg1) {
    name = NLName(arg1);
  }

  SNLDesign* design = nullptr;
  TRY
  if (IsPyNLLibrary(arg0)) {
    design = SNLDesign::create(PYNLLibrary_O(arg0), name);
  } else {
    setError("SNLDesign create accepts NLLibrary as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_createPrimitive(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    setError("malformed SNLDesign createPrimitive method");
    return nullptr;
  }
  NLName name;
  if (arg1) {
    name = NLName(arg1);
  }

  SNLDesign* design = nullptr;
  TRY
  if (IsPyNLLibrary(arg0)) {
    design = SNLDesign::create(PYNLLibrary_O(arg0), SNLDesign::Type::Primitive, name);
  } else {
    setError("SNLDesign createPrimitive accepts NLLibrary as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_clone(PySNLDesign* self, PyObject* args) {
  const char* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "|s:SNLDesign.clone", &arg0)) {
    setError("malformed SNLDesign.clone method");
    return nullptr;
  }
  NLName name;
  if (arg0) {
    name = NLName(arg0);
  }

  SNLDesign* newDesign = nullptr;
  METHOD_HEAD("SNLDesign.clone()")
  TRY
  newDesign = selfObject->clone(name);
  NLCATCH
  return PySNLDesign_Link(newDesign);
}

static PyObject* PySNLDesign_dumpGateLevelVerilog(PySNLDesign* self, PyObject* args, PyObject* kwargs) {
  const char* path = nullptr;
  const char* topFileName = nullptr;
  int dumpRTLInfosAsAttributes = 0;
  int dumpAssignsAsInstances = 0;
  const char* rtlInfoDumpMode = nullptr;
  int verilatorSplitPackedSignals = 0;

  static const char* const kwords[] = {
    "path", "top_file_name", "dumpRTLInfosAsAttributes", "dumpAssignsAsInstances",
    "rtlInfoDumpMode", "verilatorSplitPackedSignals",
    nullptr
  };

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, "ss|ppsp:SNLDesign.dumpGateLevelVerilog",
    const_cast<char**>(kwords),
    &path, &topFileName, &dumpRTLInfosAsAttributes, &dumpAssignsAsInstances,
    &rtlInfoDumpMode, &verilatorSplitPackedSignals)) {
    setError("malformed SNLDesign.dumpGateLevelVerilog method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.dumpGateLevelVerilog()")
  TRY
  SNLVRLDumper dumper;
  dumper.setTopFileName(topFileName);
  dumper.setDumpRTLInfosAsAttributes(dumpRTLInfosAsAttributes);
  if (rtlInfoDumpMode) {
    auto parsedMode = parseRTLInfoDumpMode(rtlInfoDumpMode);
    if (not parsedMode) {
      setError("invalid rtlInfoDumpMode: expected None, VerboseAttributes, or CompactAttribute");
      return nullptr;
    }
    dumper.setRTLInfoDumpMode(*parsedMode);
  }
  dumper.setDumpAssignsAsInstances(dumpAssignsAsInstances);
  dumper.setVerilatorSplitPackedSignals(verilatorSplitPackedSignals);
  dumper.dumpDesign(selfObject, std::filesystem::path(path));
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_dumpVerilog(PySNLDesign* self, PyObject* args, PyObject* kwargs) {
  if (PyErr_WarnEx(PyExc_FutureWarning,
      "SNLDesign.dumpVerilog will be deprecated soon; use SNLDesign.dumpGateLevelVerilog instead.", 1) < 0) {
    return nullptr;
  }
  return PySNLDesign_dumpGateLevelVerilog(self, args, kwargs);
}

static PyObject* PySNLDesign_addCombinatorialArcs(PySNLDesign* self, PyObject* args) {
  const char* parameterValue = nullptr;
  PyObject* inputsObject = nullptr;
  PyObject* outputsObject = nullptr;
  if (PyTuple_Size(args) == 2) {
    if (not PyArg_ParseTuple(
        args, "OO:SNLDesign.addCombinatorialArcs",
        &inputsObject, &outputsObject)) {
      // LCOV_EXCL_START defensive: "O" accepts any Python object
      setError("malformed SNLDesign.addCombinatorialArcs method");
      return nullptr;
      // LCOV_EXCL_STOP
    }
  } else if (PyTuple_Size(args) == 3) {
    if (not PyArg_ParseTuple(
        args, "sOO:SNLDesign.addCombinatorialArcs",
        &parameterValue, &inputsObject, &outputsObject)) {
      setError("malformed SNLDesign.addCombinatorialArcs method");
      return nullptr;
    }
  } else {
    setError("malformed SNLDesign.addCombinatorialArcs method");
    return nullptr;
  }
  SNLDesignModeling::BitTerms terms0;
  SNLDesignModeling::BitTerms terms1;
  if (IsPySNLBitTerm(inputsObject)) {
    terms0.push_back(PYSNLBitTerm_O(inputsObject));
  } else if (IsPySNLBusTerm(inputsObject)) {
    auto bus = PYSNLBusTerm_O(inputsObject);
    terms0.insert(terms0.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(inputsObject)) {
    setError("malformed SNLDesign.addCombinatorialArcs method");
    return nullptr;
  }
  if (IsPySNLBitTerm(outputsObject)) {
    terms1.push_back(PYSNLBitTerm_O(outputsObject));
  } else if (IsPySNLBusTerm(outputsObject)) {
    auto bus = PYSNLBusTerm_O(outputsObject);
    terms1.insert(terms1.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(outputsObject)) {
    setError("malformed SNLDesign.addCombinatorialArcs method");
    return nullptr;
  }
  if (terms0.empty()) {
    for (int i=0; i<PyList_Size(inputsObject); ++i) {
      PyObject* object = PyList_GetItem(inputsObject, i);
      if (IsPySNLBitTerm(object)) {
        terms0.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms0.insert(terms0.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addCombinatorialArcs method");
        return nullptr;
      }
    }
  }
  if (terms1.empty()) {
    for (int j=0; j<PyList_Size(outputsObject); ++j) {
      PyObject* object = PyList_GetItem(outputsObject, j);
      if (IsPySNLBitTerm(object)) {
        terms1.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms1.insert(terms1.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addCombinatorialArcs method");
        return nullptr;
      }
    }
  }
  TRY
  if (parameterValue) {
    SNLDesignModeling::addCombinatorialArcs(parameterValue, terms0, terms1);
  } else {
    SNLDesignModeling::addCombinatorialArcs(terms0, terms1);
  }
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_setTimingModelParameter(
    PySNLDesign* self,
    PyObject* args) {
  const char* name = nullptr;
  const char* defaultValue = nullptr;
  if (not PyArg_ParseTuple(
      args, "ss:SNLDesign.setTimingModelParameter", &name, &defaultValue)) {
    setError(
      "SNLDesign.setTimingModelParameter() expects a parameter name "
      "and default value");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.setTimingModelParameter()")
  TRY
  SNLDesignModeling::setParameter(selfObject, name, defaultValue);
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_addInputsToClockArcs(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addInputsToClockArcs", &arg0, &arg1)) {
    setError(
      "malformed SNLDesign.addInputsToClockArcs method, accepted args are: inputs, clock."
    );
    return nullptr;
  }
  SNLDesignModeling::BitTerms terms;
  SNLBitTerm* clock = nullptr; 
  if (IsPySNLBitTerm(arg0)) {
    terms.push_back(PYSNLBitTerm_O(arg0));
  } else if (IsPySNLBusTerm(arg0)) {
    auto bus = PYSNLBusTerm_O(arg0);
    terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(arg0)) {
    setError(
      "malformed SNLDesign.addInputsToClockArcs method, "
      "accepted args for first argument are a list of SNLBitTerm, SNLBusTerm");
    return nullptr;
  }
  if (IsPySNLBitTerm(arg1)) {
    clock = PYSNLBitTerm_O(arg1);
  } else {
    setError("malformed SNLDesign.addInputsToClockArcs method");
    return nullptr;
  }
  if (terms.empty()) {
    for (int i=0; i<PyList_Size(arg0); ++i) {
      PyObject* object = PyList_GetItem(arg0, i);
      if (IsPySNLBitTerm(object)) {
        terms.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addInputsToClockArcs method");
        return nullptr;
      }
    }
  }
  SNLDesignModeling::addInputsToClockArcs(terms, clock);
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_addClockToOutputsArcs(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addClockToOutputsArcs", &arg0, &arg1)) {
    setError(
      "malformed SNLDesign.addClockToOutputsArcs method, accepted args are: clock, outputs."
    );
    return nullptr;
  }
  SNLBitTerm* clock = nullptr; 
  SNLDesignModeling::BitTerms terms;
  if (IsPySNLBitTerm(arg0)) {
    clock = PYSNLBitTerm_O(arg0);
  } else {
    setError("malformed SNLDesign.addClockToOutputsArcs: accepted first arg is: clock");
    return nullptr;
  }
  if (IsPySNLBitTerm(arg1)) {
    terms.push_back(PYSNLBitTerm_O(arg1));
  } else if (IsPySNLBusTerm(arg1)) {
    auto bus = PYSNLBusTerm_O(arg1);
    terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(arg1)) {
    setError(
      "malformed SNLDesign.addClockToOutputsArcs method, "
      "accepted args for second argument are a list of SNLBitTerm, SNLBusTerm");
    return nullptr;
  }
  if (terms.empty()) {
    for (int i=0; i<PyList_Size(arg1); ++i) {
      PyObject* object = PyList_GetItem(arg1, i);
      if (IsPySNLBitTerm(object)) {
        terms.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addInputsToClockArcs method");
        return nullptr;
      }
    }
  } 
  SNLDesignModeling::addClockToOutputsArcs(clock, terms);
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_setTruthTable(PySNLDesign* self, PyObject* args) {
  uint64_t tt = 0;
  if (not PyArg_ParseTuple(args, "K:SNLDesign.setTruthTable", &tt)) {
    setError("malformed SNLDesign.setTruthTable method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.setTruthTable()")
  std::vector<size_t> deps;
  for (auto term: selfObject->getBitTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Input) {
      deps.push_back(term->getOrderID());
    }
  }
  size_t size = deps.size();
  SNLTruthTable truthTable(size, tt, NLBitDependencies::encodeBits(deps));
  try {
    SNLDesignModeling::setTruthTable(selfObject, truthTable);
  } catch (const NLException& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_setTruthTableFromParameter(
    PySNLDesign* self, PyObject* args) {
  PyObject* outputObject = nullptr;
  PyObject* inputsObject = nullptr;
  PyObject* parameterObject = nullptr;
  uint64_t parameterBitOffset = 0;
  if (!PyArg_ParseTuple(
          args,
          "OOO|K:SNLDesign.setTruthTableFromParameter",
          &outputObject,
          &inputsObject,
          &parameterObject,
          &parameterBitOffset)) {
    setError("malformed SNLDesign.setTruthTableFromParameter method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.setTruthTableFromParameter()")
  if (!IsPySNLBitTerm(outputObject) || !PyList_Check(inputsObject) ||
      !IsPySNLParameter(parameterObject)) {
    setError(
        "SNLDesign.setTruthTableFromParameter expects output, inputs, parameter, and optional bit offset");
    return nullptr;
  }

  SNLDesignModeling::BitTerms inputs;
  for (Py_ssize_t index = 0; index < PyList_Size(inputsObject); ++index) {
    PyObject* inputObject = PyList_GetItem(inputsObject, index);
    if (!IsPySNLBitTerm(inputObject)) {
      setError(
          "SNLDesign.setTruthTableFromParameter inputs must be SNLBitTerm objects");
      return nullptr;
    }
    inputs.push_back(PYSNLBitTerm_O(inputObject));
  }
  try {
    SNLDesignModeling::setTruthTableFromParameter(
        selfObject,
        PYSNLBitTerm_O(outputObject),
        inputs,
        PYSNLParameter_O(parameterObject),
        static_cast<size_t>(parameterBitOffset));
  } catch (const NLException& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return nullptr;
  }
  Py_RETURN_NONE;
}

// Set truth tables for design
static PyObject* PySNLDesign_setTruthTables(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDesign.setTruthTables", &arg0)) {
    setError("malformed SNLDesign.setTruthTables method");
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.setTruthTables method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.setTruthTables()")
  std::vector<SNLTruthTable> truthTables;
  std::vector<size_t> deps;
  for (auto term: selfObject->getBitTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Input) {
      deps.push_back(term->getOrderID());
    }
  }
  auto encodedDeps = NLBitDependencies::encodeBits(deps);
  for (int i=0; i<PyList_Size(arg0); ++i) {
    PyObject* item = PyList_GetItem(arg0, i);
    if (not PyLong_Check(item)) {
      setError("malformed SNLDesign.setTruthTables method, expected list of integers");
      return nullptr;
    }
    // extract u_int64_t from item
    uint64_t size = PyLong_AsUnsignedLongLong(item);
    i++;
    // check if we are still under size
    if (i >= PyList_Size(arg0)) {
      setError("malformed SNLDesign.setTruthTables method, expected list of integers");
      return nullptr;
    }
    item = PyList_GetItem(arg0, i);
    if (not PyLong_Check(item)) {
      setError("malformed SNLDesign.setTruthTables method, expected list of integers");
      return nullptr;
    }
    uint64_t mask = PyLong_AsUnsignedLongLong(item);;
    auto tableDeps =
        deps.empty() ? SNLTruthTable::fullDependencies(size) : encodedDeps;
    truthTables.push_back(SNLTruthTable(size, mask, tableDeps));
  }
  try {
    SNLDesignModeling::setTruthTables(selfObject, truthTables);
  } catch (const NLException& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return nullptr;
  }
  Py_RETURN_NONE;
}

// Return the truth table for design
PyObject* PySNLDesign_getTruthTable(PySNLDesign* self) { 
  METHOD_HEAD("SNLDesign.getTruthTable()")
  TRY
  const SNLTruthTable& truthTable =
      SNLDesignModeling::getTruthTable(selfObject);
  if (!truthTable.isInitialized()) {
    Py_RETURN_NONE;
  }
  std::vector<uint64_t> chunks = truthTable.bits().getChunks();
  PyObject* py_list = PyList_New(1 + chunks.size()); 
  PyList_SetItem(py_list, 0, PyLong_FromLong(truthTable.size()));
  for (size_t i = 0; i < chunks.size(); i++) {
    PyList_SetItem(py_list, i + 1, PyLong_FromLong(chunks[i]));
  }
  return py_list;
  NLCATCH
}

// Return the truth table for design by output ID
static PyObject* PySNLDesign_getTruthTableByOutputID(PySNLDesign* self, PyObject* args) {
  long outputID = 0;
  if (not PyArg_ParseTuple(args, "l:SNLDesign.getTruthTableByOutputID", &outputID)) {
    setError("malformed SNLDesign.getTruthTableByOutputID method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.getTruthTableByOutputID()")
  TRY
  SNLTruthTable truthTable =
      SNLDesignModeling::getTruthTable(selfObject, outputID);
  if (!truthTable.isInitialized()) {
    Py_RETURN_NONE;
  }
  std::vector<uint64_t> chunks = truthTable.bits().getChunks();
  PyObject* py_list = PyList_New(1 + chunks.size()); 
  PyList_SetItem(py_list, 0, PyLong_FromLong(truthTable.size()));
  for (size_t i = 0; i < chunks.size(); i++) {
    PyList_SetItem(py_list, i + 1, PyLong_FromLong(chunks[i]));
  }
  return py_list;
  NLCATCH
}

static PyObject* PySNLDesign_dumpFullDotFile(PySNLDesign* self, PyObject* args) {
  char* path = NULL; 
  if (not PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &path)) {
    setError("dumpDotFile expact a string as argument");
    return nullptr;
  }
  std::filesystem::path outputPath;
  if (path) {
    outputPath = std::filesystem::path(path);
  }
  std::string dotFileName(outputPath.string());
  naja::NL::SNLDesign* design = self->object_;
  naja::SnlVisualiser snl(design);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_dumpContextDotFile(PySNLDesign* self, PyObject* args) {
  char* path = NULL; 
  if (not PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &path)) {
    setError("dumpDotFile expact a string as argument");
    return nullptr;
  }
  std::filesystem::path outputPath;
  if (path) {
    outputPath = std::filesystem::path(path);
  }
  std::string dotFileName(outputPath.string());
  naja::NL::SNLDesign* design = self->object_;
  naja::SnlVisualiser snl(design, false);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_getInstanceByIDList(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDesign.getInstanceByIDList", &arg0)) {
    setError("malformed SNLDesign.getInstanceByIDList method");
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.getInstanceByIDList method");
    return nullptr;
  }
  naja::NL::SNLDesign* design = self->object_;
  naja::NL::SNLInstance* instance = nullptr;
  for (int i=0; i<PyList_Size(arg0); ++i) {
    instance = design->getInstance(PyLong_AsLong(PyList_GetItem(arg0, i)));
    design = instance->getModel();
  }
  return PySNLInstance_Link(instance);
}

static PyObject* PySNLDesign_addAttribute(PySNLDesign* self, PyObject* args) {
  METHOD_HEAD("SNLDesign.addAttribute()")
  PySNLAttribute* pyAttribute = nullptr;
  if (PyArg_ParseTuple(args, "O!", &PyTypeSNLAttribute, &pyAttribute)) {
    auto attribute = PYSNLAttribute_O(pyAttribute);
    SNLAttributes::addAttribute(selfObject, *attribute);
  } else {
    setError("invalid number of parameters for addAttribute.");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_getSourceDeclaration(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.getSourceDeclaration()")
  return PYNAJA::getSourceDeclaration(selfObject->getRTLInfos());
}

static PyObject* PySNLDesign_getSourceOrigins(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.getSourceOrigins()")
  return PYNAJA::getSourceOrigins(selfObject->getRTLInfos());
}

static PyObject* PySNLDesign_hasSourceReferences(
  PySNLDesign* self, PyObject*) {
  METHOD_HEAD("SNLDesign.hasSourceReferences()")
  auto* infos = selfObject->getRTLInfos();
  if (infos && infos->hasSourceReferences()) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject* PySNLDesign_hasSourceLoc(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.hasSourceLoc()")
  auto* rtlInfos = selfObject->getRTLInfos();
  if (rtlInfos and rtlInfos->hasSourceLoc()) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject* PySNLDesign_getSourceLoc(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.getSourceLoc()")
  auto* rtlInfos = selfObject->getRTLInfos();
  if (not rtlInfos or not rtlInfos->hasSourceLoc()) {
    Py_RETURN_NONE;
  }
  const auto& loc = *rtlInfos->getSourceLoc();
  return Py_BuildValue(
    "(sIIII)",
    loc.file.getString().c_str(),
    static_cast<unsigned int>(loc.line),
    static_cast<unsigned int>(loc.column),
    static_cast<unsigned int>(loc.endLine),
    static_cast<unsigned int>(loc.endColumn));
}

static PyObject* PySNLDesign_getCombinatorialInputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getCombinatorialInputs, SNLDesign)
}

static PyObject* PySNLDesign_getCombinatorialOutputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getCombinatorialOutputs, SNLDesign)
}

static PyObject* PySNLDesign_getClockRelatedInputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getClockRelatedInputs, SNLDesign)
}

static PyObject* PySNLDesign_getClockRelatedOutputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getClockRelatedOutputs, SNLDesign)
}

static PyObject* PySNLDesign_getInputRelatedClocks(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getInputRelatedClocks, SNLDesign)
}

static PyObject* PySNLDesign_getOutputRelatedClocks(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getOutputRelatedClocks, SNLDesign)
}

#define DESIGN_ROLE_TERMS(NAME)                                           \
  static PyObject* PySNLDesign_##NAME(PySNLDesign* self) {                \
    METHOD_HEAD("SNLDesign." #NAME "()")                                  \
    TRY                                                                    \
    auto* objects = new naja::NajaCollection<SNLBitTerm*>(                \
        SNLDesignModeling::NAME(selfObject));                              \
    auto* pyObjects = PyObject_NEW(PySNLBitTerms, &PyTypeSNLBitTerms);     \
    if (!pyObjects) { delete objects; return nullptr; }                    \
    pyObjects->object_ = objects;                                          \
    return reinterpret_cast<PyObject*>(pyObjects);                         \
    NLCATCH                                                                \
    return nullptr;                                                        \
  }

DESIGN_ROLE_TERMS(getClockTerms)
DESIGN_ROLE_TERMS(getAsyncResetTerms)
DESIGN_ROLE_TERMS(getAsyncSetTerms)
DESIGN_ROLE_TERMS(getSyncResetTerms)
DESIGN_ROLE_TERMS(getSyncSetTerms)
DESIGN_ROLE_TERMS(getDataInputTerms)
DESIGN_ROLE_TERMS(getOutputTerms)
#undef DESIGN_ROLE_TERMS

GetObjectMethod(SNLDesign, NLDB, getDB)
GetObjectMethod(SNLDesign, NLLibrary, getLibrary)
GetObjectByName(SNLDesign, SNLInstance, getInstance)
GetObjectByName(SNLDesign, SNLTerm, getTerm)
GetObjectByName(SNLDesign, SNLScalarTerm, getScalarTerm)
GetObjectByName(SNLDesign, SNLBusTerm, getBusTerm)
GetObjectByName(SNLDesign, SNLBundleTerm, getBundleTerm)
GetObjectByName(SNLDesign, SNLNet, getNet)
GetObjectByName(SNLDesign, SNLScalarNet, getScalarNet)
GetObjectByName(SNLDesign, SNLBusNet, getBusNet)
GetObjectByName(SNLDesign, SNLParameter, getParameter)
GetObjectByIndex(SNLDesign, SNLInstance, InstanceByID)
GetObjectByIndex(SNLDesign, SNLTerm, TermByID)
GetObjectByIndex(SNLDesign, SNLNet, NetByID)
GetNameMethod(SNLDesign)
SetNameMethod(SNLDesign)
DirectGetNumericMethod(PySNLDesign_getID, getID, PySNLDesign, SNLDesign)
DirectGetNumericMethod(PySNLDesign_getRevisionCount, getRevisionCount, PySNLDesign, SNLDesign)
DirectGetNLIDMethod(PySNLDesign_getNLID, PySNLDesign, SNLDesign)
GetBoolAttribute(SNLDesign, isUnnamed)
GetBoolAttribute(SNLDesign, isBlackBox)
GetBoolAttribute(SNLDesign, isPrimitive)
GetBoolAttribute(SNLDesign, isLeaf)
GetBoolAttribute(SNLDesign, isAssign)
GetBoolAttribute(SNLDesign, isTopDesign)
GetBoolAttributeWithFunction(SNLDesign, hasModeling, SNLDesignModeling::hasModeling)
GetBoolAttributeWithFunction(SNLDesign, isConst0, SNLDesignModeling::isConst0)
GetBoolAttributeWithFunction(SNLDesign, isConst1, SNLDesignModeling::isConst1)
GetBoolAttributeWithFunction(SNLDesign, isConst, SNLDesignModeling::isConst)
GetBoolAttributeWithFunction(SNLDesign, isBuf, SNLDesignModeling::isBuf)
GetBoolAttributeWithFunction(SNLDesign, isInv, SNLDesignModeling::isInv)
GetBoolAttributeWithFunction(SNLDesign, isAnd, SNLDesignModeling::isAnd)
GetBoolAttributeWithFunction(SNLDesign, isNand, SNLDesignModeling::isNand)
GetBoolAttributeWithFunction(SNLDesign, isOr, SNLDesignModeling::isOr)
GetBoolAttributeWithFunction(SNLDesign, isNor, SNLDesignModeling::isNor)
GetBoolAttributeWithFunction(SNLDesign, isXor, SNLDesignModeling::isXor)
GetBoolAttributeWithFunction(SNLDesign, isXnor, SNLDesignModeling::isXnor)
GetBoolAttribute(SNLDesign, isMux)
GetBoolAttributeWithFunction(SNLDesign, isAddSub, NLDB0::isAddSub)
GetBoolAttributeWithFunction(SNLDesign, isSubtract, NLDB0::isSubtract)
GetBoolAttributeWithFunction(SNLDesign, isMultiply, NLDB0::isMultiply)
GetBoolAttributeWithFunction(SNLDesign, isShift, NLDB0::isShift)
GetBoolAttributeWithFunction(SNLDesign, isDivMod, NLDB0::isDivMod)

static PyObject* PySNLDesign_isShiftRight(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.isShiftRight()")
  try {
    if (NLDB0::isShift(selfObject) && NLDB0::getShiftSignature(selfObject).right) {
      Py_RETURN_TRUE;
    }
  } catch (...) {
    // Invalid shift metadata must not escape the predicate into Python.
  }
  Py_RETURN_FALSE;
}

static PyObject* PySNLDesign_isShiftArithmetic(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.isShiftArithmetic()")
  try {
    if (NLDB0::isShift(selfObject) && NLDB0::getShiftSignature(selfObject).arithmetic) {
      Py_RETURN_TRUE;
    }
  } catch (...) {
    // Invalid shift metadata must not escape the predicate into Python.
  }
  Py_RETURN_FALSE;
}
GetBoolAttributeWithFunction(SNLDesign, isSequential, SNLDesignModeling::isSequential)
HasElementsMethod(SNLDesign, hasTerms, getTerms)
GetContainerMethod(SNLDesign, SNLTerm*, SNLTerms, Terms)
GetContainerMethod(SNLDesign, NLClock*, NLClocks, Clocks)
GetContainerMethod(SNLDesign, SNLBitTerm*, SNLBitTerms, BitTerms)
GetContainerMethod(SNLDesign, SNLScalarTerm*, SNLScalarTerms, ScalarTerms)
GetContainerMethod(SNLDesign, SNLBusTerm*, SNLBusTerms, BusTerms)
GetContainerMethod(SNLDesign, SNLBundleTerm*, SNLBundleTerms, BundleTerms)
GetContainerMethod(SNLDesign, SNLNet*, SNLNets, Nets)
GetContainerMethod(SNLDesign, SNLScalarNet*, SNLScalarNets, ScalarNets)
GetContainerMethod(SNLDesign, SNLBusNet*, SNLBusNets, BusNets)
GetContainerMethod(SNLDesign, SNLBitNet*, SNLBitNets, BitNets)
HasElementsMethod(SNLDesign, hasInstances, getInstances)
GetContainerMethod(SNLDesign, SNLInstance*, SNLInstances, Instances)
GetContainerMethod(SNLDesign, SNLInstance*, SNLInstances, NonAssignInstances)
GetContainerMethod(SNLDesign, SNLInstance*, SNLInstances, AssignInstances)
HasElementsMethod(SNLDesign, hasPrimitiveInstances, getPrimitiveInstances)
GetContainerMethod(SNLDesign, SNLInstance*, SNLInstances, PrimitiveInstances)
HasElementsMethod(SNLDesign, hasNonPrimitiveInstances, getNonPrimitiveInstances)
GetContainerMethod(SNLDesign, SNLInstance*, SNLInstances, NonPrimitiveInstances)
GetContainerMethod(SNLDesign, SNLParameter*, SNLParameters, Parameters)
GetContainerMethod(SNLDesign, SNLAttribute, SNLAttributes, Attributes)

DBoDestroyAttribute(PySNLDesign_destroy, PySNLDesign)

static PyObject* PySNLDesign_hasRolesFromParameters(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.hasRolesFromParameters()")
  return PyBool_FromLong(SNLDesignModeling::hasRolesFromParameters(selfObject));
}

static PyObject* PySNLDesign_setRolesFromParameters(
    PySNLDesign* self, PyObject* args, PyObject* kwargs) {
  METHOD_HEAD("SNLDesign.setRolesFromParameters()")
  PyObject* parametersObject = nullptr;
  PyObject* rolesObject = nullptr;
  static const char* const keywords[] = {"parameters", "roles", nullptr};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", const_cast<char**>(keywords),
      &parametersObject, &rolesObject)) return nullptr;
  const std::string context = "SNLDesign.setRolesFromParameters: " + selfObject->getDescription();
  auto fail = [&context](const std::string& detail) {
    setError(context + ": " + detail);
    return static_cast<PyObject*>(nullptr);
  }; // LCOV_EXCL_LINE: compiler-generated lambda epilogue; body covered above
  if (!PyList_Check(parametersObject) || !PyList_Check(rolesObject)) {
    return fail("parameters and roles must be lists");
  }
  TRY
  std::vector<std::string> parameters;
  for (Py_ssize_t i = 0; i < PyList_Size(parametersObject); ++i) {
    auto* name = PyList_GetItem(parametersObject, i);
    if (!PyUnicode_Check(name)) return fail("parameter names must be strings");
    const char* text = PyUnicode_AsUTF8(name);
    if (!text) return nullptr;
    parameters.emplace_back(text);
  }
  SNLDesignModeling::TermRoleTable table;
  for (Py_ssize_t i = 0; i < PyList_Size(rolesObject); ++i) {
    const auto field = "roles[" + std::to_string(i) + "]: ";
    auto* entry = PyList_GetItem(rolesObject, i);
    if (!PyDict_Check(entry)) return fail(field + "expected a dictionary with values and roles");
    auto* valuesObject = PyDict_GetItemString(entry, "values");
    auto* mapping = PyDict_GetItemString(entry, "roles");
    if (!valuesObject || !PyList_Check(valuesObject) || !mapping || !PyDict_Check(mapping)) {
      return fail(field + "expected values list and roles dictionary");
    }
    std::vector<uint64_t> values;
    for (Py_ssize_t j = 0; j < PyList_Size(valuesObject); ++j) {
      auto* value = PyList_GetItem(valuesObject, j);
      if (!PyLong_Check(value)) return fail(field + "values must be unsigned 64-bit integers");
      values.push_back(PyLong_AsUnsignedLongLong(value));
      if (PyErr_Occurred()) return nullptr;
    }
    SNLDesignModeling::TermRoles roles;
    PyObject* termObject = nullptr;
    PyObject* roleObject = nullptr;
    Py_ssize_t position = 0;
    while (PyDict_Next(mapping, &position, &termObject, &roleObject)) {
      if (!IsPySNLBitTerm(termObject) || !PYSNLBitTerm_O(termObject)) {
        return fail(field + "roles keys must be live SNLBitTerm objects");
      }
      if (PYSNLBitTerm_O(termObject)->getDesign() != selfObject) {
        return fail(field + "role term must belong to the destination primitive");
      }
      SNLDesignModeling::TermRole role;
      if (roleObject != Py_None) {
        if (!PyTuple_Check(roleObject) || PyTuple_Size(roleObject) != 2) {
          return fail(field + "role must be None or (SNLTermRole, SNLActiveLevel)");
        }
        auto* roleValue = PyTuple_GetItem(roleObject, 0);
        auto* levelValue = PyTuple_GetItem(roleObject, 1);
        if (!PyLong_Check(roleValue) || !PyLong_Check(levelValue)) {
          return fail(field + "expected integer SNLTermRole and SNLActiveLevel values");
        }
        const auto roleNumber = PyLong_AsLong(roleValue);
        if (PyErr_Occurred()) return nullptr;
        const auto levelNumber = PyLong_AsLong(levelValue);
        if (PyErr_Occurred()) return nullptr;
        if (roleNumber < 0 || roleNumber > static_cast<long>(SNLDesignModeling::SNLTermRole::ScanEnable) ||
            levelNumber < 0 || levelNumber > static_cast<long>(SNLDesignModeling::SNLActiveLevel::NA)) {
          return fail(field + "invalid SNLTermRole or SNLActiveLevel");
        }
        role.role = static_cast<SNLDesignModeling::SNLTermRole>(roleNumber);
        role.activeLevel = static_cast<SNLDesignModeling::SNLActiveLevel>(levelNumber);
      }
      roles.emplace(PYSNLBitTerm_O(termObject), role);
    }
    if (!table.emplace(std::move(values), std::move(roles)).second) {
      return fail(field + "duplicate parameter values");
    }
  }
  SNLDesignModeling::setRolesFromParameters(selfObject, parameters, table);
  NLCATCH
  Py_RETURN_NONE;
}

PyMethodDef PySNLDesign_Methods[] = {
  {"hasSourceReferences", (PyCFunction)PySNLDesign_hasSourceReferences, METH_NOARGS,
    "Whether this object has a source declaration or any source origins."},
  {"getSourceDeclaration", (PyCFunction)PySNLDesign_getSourceDeclaration, METH_NOARGS,
    "getSourceDeclaration: language-neutral source metadata; independent of legacy source location."},
  {"getSourceOrigins", (PyCFunction)PySNLDesign_getSourceOrigins, METH_NOARGS,
    "getSourceOrigins: language-neutral source metadata; independent of legacy source location."},
  {"setRolesFromParameters", (PyCFunction)PySNLDesign_setRolesFromParameters,
    METH_VARARGS | METH_KEYWORDS, "set parameter-dependent primitive term roles."},
  {"hasRolesFromParameters", (PyCFunction)PySNLDesign_hasRolesFromParameters,
    METH_NOARGS, "whether parameter-dependent term roles are declared."},
  { "create", (PyCFunction)PySNLDesign_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
  { "getID", (PyCFunction)PySNLDesign_getID, METH_NOARGS,
    "get the ID."},
  { "getRevisionCount", (PyCFunction)PySNLDesign_getRevisionCount, METH_NOARGS,
    "get revision count."},
  { "createPrimitive", (PyCFunction)PySNLDesign_createPrimitive, METH_VARARGS|METH_STATIC,
    "SNLDesign Primitive creator"},
  { "addCombinatorialArcs", (PyCFunction)PySNLDesign_addCombinatorialArcs, METH_VARARGS|METH_STATIC,
    "add combinatorial arcs, optionally for a timing parameter value"},
  { "setTimingModelParameter", (PyCFunction)PySNLDesign_setTimingModelParameter, METH_VARARGS,
    "select the parameter and default value used by parameterized timing arcs"},
  { "addInputsToClockArcs", (PyCFunction)PySNLDesign_addInputsToClockArcs, METH_VARARGS|METH_STATIC,
    "add inputs to clock arcs"}, 
  { "addClockToOutputsArcs", (PyCFunction)PySNLDesign_addClockToOutputsArcs, METH_VARARGS|METH_STATIC,
    "add inputs to clock arcs"}, 
  { "getCombinatorialInputs", (PyCFunction)PySNLDesign_getCombinatorialInputs, METH_O|METH_STATIC,
    "get combinatorial inputs of a term"},
  { "getCombinatorialOutputs", (PyCFunction)PySNLDesign_getCombinatorialOutputs, METH_O|METH_STATIC,
    "get combinatorial outputs of a term"},
  { "getClockRelatedInputs", (PyCFunction)PySNLDesign_getClockRelatedInputs, METH_O|METH_STATIC,
    "get inputs related to a clock"},
  { "getClockRelatedOutputs", (PyCFunction)PySNLDesign_getClockRelatedOutputs, METH_O|METH_STATIC,
    "get outputs related to a clock"},
  { "getInputRelatedClocks", (PyCFunction)PySNLDesign_getInputRelatedClocks, METH_O|METH_STATIC,
    "get clocks related to an input"},
  { "getOutputRelatedClocks", (PyCFunction)PySNLDesign_getOutputRelatedClocks, METH_O|METH_STATIC,
    "get clocks related to an output"},
  { "getClockTerms", (PyCFunction)PySNLDesign_getClockTerms, METH_NOARGS,
    "get primitive clock terms"},
  { "getAsyncResetTerms", (PyCFunction)PySNLDesign_getAsyncResetTerms, METH_NOARGS,
    "get primitive asynchronous reset terms"},
  { "getAsyncSetTerms", (PyCFunction)PySNLDesign_getAsyncSetTerms, METH_NOARGS,
    "get primitive asynchronous set terms"},
  { "getSyncResetTerms", (PyCFunction)PySNLDesign_getSyncResetTerms, METH_NOARGS,
    "get primitive synchronous reset terms"},
  { "getSyncSetTerms", (PyCFunction)PySNLDesign_getSyncSetTerms, METH_NOARGS,
    "get primitive synchronous set terms"},
  { "getDataInputTerms", (PyCFunction)PySNLDesign_getDataInputTerms, METH_NOARGS,
    "get primitive data input terms"},
  { "getOutputTerms", (PyCFunction)PySNLDesign_getOutputTerms, METH_NOARGS,
    "get primitive data output terms"},
  { "setSequentialModelFromParameters", (PyCFunction)PySNLDesign_setSequentialModelFromParameters,
    METH_VARARGS|METH_KEYWORDS, "Set a table of parameter-selected sequential models."},
  { "hasMemoryInterface", (PyCFunction)PySNLDesign_hasMemoryInterface, METH_NOARGS,
    "Return whether this design has a memory interface."},
  { "getMemoryInterface", (PyCFunction)PySNLDesign_getMemoryInterface, METH_NOARGS,
    "Return the memory interface dictionary; raise RuntimeError if absent."},
  { "getSequentialModel", (PyCFunction)PySNLDesign_getSequentialModel,
    METH_NOARGS, "Get the static sequential model as expression trees."},
  { "hasSequentialModelFromParameters", (PyCFunction)PySNLDesign_hasSequentialModelFromParameters,
    METH_NOARGS, "Whether sequential behavior is selected by parameters."},
  { "setSequentialModel", (PyCFunction)PySNLDesign_setSequentialModel,
    METH_VARARGS|METH_KEYWORDS, "set the sequential model of a primitive"},
  { "hasSequentialModel", (PyCFunction)PySNLDesign_hasSequentialModel,
    METH_NOARGS, "Returns True if the primitive has a sequential model"},
  { "setTruthTable", (PyCFunction)PySNLDesign_setTruthTable, METH_VARARGS,
    "set truth table of a primitive"},
  { "setTruthTableFromParameter",
    (PyCFunction)PySNLDesign_setTruthTableFromParameter, METH_VARARGS,
    "set a primitive truth table derived from an instance parameter"},
  { "getTruthTable", (PyCFunction)PySNLDesign_getTruthTable, METH_NOARGS,
    "get truth table of a primitive"},
  { "isConst0", (PyCFunction)PySNLDesign_isConst0, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant 0"},
  { "isConst1", (PyCFunction)PySNLDesign_isConst1, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant 1"},
  { "isConst", (PyCFunction)PySNLDesign_isConst, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant (1 or 0)"},
  { "isBuf", (PyCFunction)PySNLDesign_isBuf, METH_NOARGS,
    "Returns True if this design is a buffer primitive"},
  { "isInv", (PyCFunction)PySNLDesign_isInv, METH_NOARGS,
    "Returns True if this design is an inverter primitive"},
  { "isAnd", (PyCFunction)PySNLDesign_isAnd, METH_NOARGS,
    "Returns True if this design is an AND primitive"},
  { "isNand", (PyCFunction)PySNLDesign_isNand, METH_NOARGS,
    "Returns True if this design is a NAND primitive"},
  { "isOr", (PyCFunction)PySNLDesign_isOr, METH_NOARGS,
    "Returns True if this design is an OR primitive"},
  { "isNor", (PyCFunction)PySNLDesign_isNor, METH_NOARGS,
    "Returns True if this design is a NOR primitive"},
  { "isXor", (PyCFunction)PySNLDesign_isXor, METH_NOARGS,
    "Returns True if this design is an XOR primitive"},
  { "isXnor", (PyCFunction)PySNLDesign_isXnor, METH_NOARGS,
    "Returns True if this design is an XNOR primitive"},
  { "isMux", (PyCFunction)PySNLDesign_isMux, METH_NOARGS,
    "Returns True if this design is a mux primitive"},
  { "isAddSub", (PyCFunction)PySNLDesign_isAddSub, METH_NOARGS,
    "Returns True if this design is an NLDB0 add or subtract primitive"},
  { "isSubtract", (PyCFunction)PySNLDesign_isSubtract, METH_NOARGS,
    "Returns True if this design is an NLDB0 subtract primitive"},
  { "isMultiply", (PyCFunction)PySNLDesign_isMultiply, METH_NOARGS,
    "Returns True if this design is an NLDB0 multiply primitive"},
  { "isShift", (PyCFunction)PySNLDesign_isShift, METH_NOARGS,
    "Returns True if this design is an NLDB0 shift primitive"},
  { "isDivMod", (PyCFunction)PySNLDesign_isDivMod, METH_NOARGS,
    "Returns True if this design is an NLDB0 divide/modulo primitive"},
  { "isShiftRight", (PyCFunction)PySNLDesign_isShiftRight, METH_NOARGS,
    "Returns True if this design is an NLDB0 right shift primitive"},
  { "isShiftArithmetic", (PyCFunction)PySNLDesign_isShiftArithmetic, METH_NOARGS,
    "Returns True if this design is an NLDB0 arithmetic shift primitive"},
  { "isTopDesign", (PyCFunction)PySNLDesign_isTopDesign, METH_NOARGS,
    "Returns True if this design is an inverter primitive"},  
  { "getName", (PyCFunction)PySNLDesign_getName, METH_NOARGS,
    "get SNLDesign name"},
  { "isUnnamed", (PyCFunction)PySNLDesign_isUnnamed, METH_NOARGS,
    "Returns True if the SNLDesign is unnamed"},
  { "setName", (PyCFunction)PySNLDesign_setName, METH_O,
    "Set the NLName of this SNLDesign."},
  { "isBlackBox", (PyCFunction)PySNLDesign_isBlackBox, METH_NOARGS,
    "Returns True if the SNLDesign is a Blackbox"},
  { "isPrimitive", (PyCFunction)PySNLDesign_isPrimitive, METH_NOARGS,
    "Returns True if the SNLDesign is a Primitive"},
  { "isLeaf", (PyCFunction)PySNLDesign_isLeaf, METH_NOARGS,
    "Returns True if the SNLDesign is a Leaf"},
  { "isAssign", (PyCFunction)PySNLDesign_isAssign, METH_NOARGS,
    "Returns True if the SNLDesign is an Assign"},
  { "isSequential", (PyCFunction)PySNLDesign_isSequential, METH_NOARGS,
    "Returns True if the SNLDesign is a Sequential"},
  { "getDB", (PyCFunction)PySNLDesign_getDB, METH_NOARGS,
    "Returns the SNLDesign owner SNLDB."},
  { "getLibrary", (PyCFunction)PySNLDesign_getLibrary, METH_NOARGS,
    "Returns the SNLDesign owner NLLibrary."},
  { "getTerm", (PyCFunction)PySNLDesign_getTerm, METH_VARARGS,
    "retrieve a SNLTerm."},
  { "getTermByID", (PyCFunction)PySNLDesign_getTermByID, METH_VARARGS,
    "retrieve a SNLTerm."},
  { "getScalarTerm", (PyCFunction)PySNLDesign_getScalarTerm, METH_VARARGS,
    "retrieve a SNLScalarTerm."},
  { "getBusTerm", (PyCFunction)PySNLDesign_getBusTerm, METH_VARARGS,
    "retrieve a SNLBusTerm."},
  { "getBundleTerm", (PyCFunction)PySNLDesign_getBundleTerm, METH_VARARGS,
    "retrieve a SNLBundleTerm."},
  { "getNet", (PyCFunction)PySNLDesign_getNet, METH_VARARGS,
    "retrieve a SNLNet."},
  { "getNetByID", (PyCFunction)PySNLDesign_getNetByID, METH_VARARGS,
    "retrieve a SNLNet."},
  { "getScalarNet", (PyCFunction)PySNLDesign_getScalarNet, METH_VARARGS,
    "retrieve a SNLScalarNet."},
  { "getBusNet", (PyCFunction)PySNLDesign_getBusNet, METH_VARARGS,
    "retrieve a SNLBusNet."},
  { "getInstance", (PyCFunction)PySNLDesign_getInstance, METH_VARARGS,
    "retrieve a SNLInstance."},
  { "getInstanceByID", (PyCFunction)PySNLDesign_getInstanceByID, METH_VARARGS,
    "retrieve a SNLInstance."},
  { "getParameter", (PyCFunction)PySNLDesign_getParameter, METH_VARARGS,
    "retrieve a SNLParameter."},
  { "hasTerms", (PyCFunction)PySNLDesign_hasTerms, METH_NOARGS,
    "Returns True if the SNLDesign has terms."},
  { "getTerms", (PyCFunction)PySNLDesign_getTerms, METH_NOARGS,
    "get a container of SNLTerms."},
  { "getBitTerms", (PyCFunction)PySNLDesign_getBitTerms, METH_NOARGS,
    "get a container of SNLBitTerms."},
  { "getScalarTerms", (PyCFunction)PySNLDesign_getScalarTerms, METH_NOARGS,
    "get a container of SNLScalarTerms."},
  { "getBusTerms", (PyCFunction)PySNLDesign_getBusTerms, METH_NOARGS,
    "get a container of SNLBusTerms."},
  { "getBundleTerms", (PyCFunction)PySNLDesign_getBundleTerms, METH_NOARGS,
    "get a container of SNLBundleTerms."},
  { "getNets", (PyCFunction)PySNLDesign_getNets, METH_NOARGS,
    "get a container of SNLNets."},
  { "getScalarNets", (PyCFunction)PySNLDesign_getScalarNets, METH_NOARGS,
    "get a container of SNLScalarNets."},
  { "getBusNets", (PyCFunction)PySNLDesign_getBusNets, METH_NOARGS,
    "get a container of SNLBusNets."},
  { "getBitNets", (PyCFunction)PySNLDesign_getBitNets, METH_NOARGS,
    "get a container of SNLBitNets."},
  { "hasInstances", (PyCFunction)PySNLDesign_hasInstances, METH_NOARGS,
    "Returns True if the SNLDesign has instances."},
  { "getInstances", (PyCFunction)PySNLDesign_getInstances, METH_NOARGS,
    "get a container of SNLInstances."},
  { "getNonAssignInstances", (PyCFunction)PySNLDesign_getNonAssignInstances, METH_NOARGS,
    "get instances excluding only assign glue."},
  { "getAssignInstances", (PyCFunction)PySNLDesign_getAssignInstances, METH_NOARGS,
    "get assign-glue instances."},
  { "hasPrimitiveInstances", (PyCFunction)PySNLDesign_hasPrimitiveInstances, METH_NOARGS,
    "Returns True if the SNLDesign has primitive instances."},
  { "getPrimitiveInstances", (PyCFunction)PySNLDesign_getPrimitiveInstances, METH_NOARGS,
    "get a container of Primitive SNLInstances."},
  { "hasNonPrimitiveInstances", (PyCFunction)PySNLDesign_hasNonPrimitiveInstances, METH_NOARGS,
    "Returns True if the SNLDesign has non-primitive instances."},
  { "getNonPrimitiveInstances", (PyCFunction)PySNLDesign_getNonPrimitiveInstances, METH_NOARGS,
    "get a container of NonPrimitive SNLInstances."},
  { "getClocks", (PyCFunction)PySNLDesign_getClocks, METH_NOARGS,
    "get a container of the NLClocks defined in this SNLDesign."},
  { "getParameters", (PyCFunction)PySNLDesign_getParameters, METH_NOARGS,
    "get a container of SNLParameters."},
  { "destroy", (PyCFunction)PySNLDesign_destroy, METH_NOARGS,
    "destroy this SNLDesign."},
  { "clone", (PyCFunction)PySNLDesign_clone, METH_VARARGS,
    "clone this SNLDesign."},
  { "dumpVerilog", (PyCFunction)PySNLDesign_dumpVerilog, METH_VARARGS|METH_KEYWORDS,
    "Compatibility alias for dumpGateLevelVerilog; will be deprecated soon. Emits FutureWarning."},
  { "dumpGateLevelVerilog", (PyCFunction)PySNLDesign_dumpGateLevelVerilog, METH_VARARGS|METH_KEYWORDS,
    "dump this SNLDesign as gate-level (structural) Verilog, not the original behavioral RTL."},
  { "dumpFullDotFile", (PyCFunction)PySNLDesign_dumpFullDotFile, METH_VARARGS,
    "dump full dot file for this SNLDesign."},
  { "dumpContextDotFile", (PyCFunction)PySNLDesign_dumpContextDotFile, METH_VARARGS,
    "dump context dot file for this SNLDesign."},
  { "getInstanceByIDList", (PyCFunction)PySNLDesign_getInstanceByIDList, METH_VARARGS,
    "get instance by ID list."},
  { "getNLID", (PyCFunction)PySNLDesign_getNLID, METH_NOARGS,
    "get NLID of the design."},
  { "getTruthTableByOutputID", (PyCFunction)PySNLDesign_getTruthTableByOutputID, METH_VARARGS,
    "get truth table by output ID."},
  { "setTruthTables", (PyCFunction)PySNLDesign_setTruthTables, METH_VARARGS,
    "set truth tables for design."},
  { "hasModeling", (PyCFunction)PySNLDesign_hasModeling, METH_NOARGS,
    "Returns True if the SNLDesign has modeling."},
  {"addAttribute", (PyCFunction)PySNLDesign_addAttribute, METH_VARARGS,
    "add an attribute to the design."},
  {"getAttributes", (PyCFunction)PySNLDesign_getAttributes, METH_NOARGS,
    "get a container of SNLAttributes."},
  {"hasSourceLoc", (PyCFunction)PySNLDesign_hasSourceLoc, METH_NOARGS,
    "Returns whether the SNLDesign has source location information."},
  {"getSourceLoc", (PyCFunction)PySNLDesign_getSourceLoc, METH_NOARGS,
    "Returns source location as (file, line, column, end_line, end_column), or None."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLDesign)

DBoLinkCreateMethod(SNLDesign)
PyTypeNLFinalObjectWithNLIDLinkPyType(SNLDesign)
PyTypeObjectDefinitions(SNLDesign)

}
