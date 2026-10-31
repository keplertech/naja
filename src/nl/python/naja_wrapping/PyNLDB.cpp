// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLDB.h"
#include "PyNLClock.h"
#include "PyNLClocks.h"

#include <Python.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLCapnP.h"
#include "SNLDumpManifest.h"
#include "SNLLibertyConstructor.h"
#include "SNLSVConstructor.h"
#include "SNLSVConstructorException.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"
#include "VHDLConstructor.h"
#include "VHDLIntent.h"
#include "SNLHDLLibrary.h"

#include "PyInterface.h"
#include "PyNLUniverse.h"
#include "PyNLLibraries.h"
#include "PyNLLibrary.h"
#include "PySNLDesign.h"

namespace PYNAJA {

using namespace naja::NL;

namespace {

PyObject* PySystemVerilogError = nullptr;
PyObject* PySystemVerilogSyntaxError = nullptr;
PyObject* PySystemVerilogUnsupportedError = nullptr;
PyObject* PySystemVerilogInternalError = nullptr;

PyObject* buildDiagnostics(const std::vector<SNLSVDiagnostic>& diagnostics) {
  PyObject* result = PyList_New(static_cast<Py_ssize_t>(diagnostics.size()));
  if (!result) {
    return nullptr; // LCOV_EXCL_LINE CPython allocation failure
  }
  for (size_t index = 0; index < diagnostics.size(); ++index) {
    const auto& diagnostic = diagnostics[index];
    PyObject* item = Py_BuildValue(
      "{s:s,s:s,s:K,s:K,s:s}",
      "severity", diagnostic.severity.c_str(),
      "file", diagnostic.file.c_str(),
      "line", static_cast<unsigned long long>(diagnostic.line),
      "column", static_cast<unsigned long long>(diagnostic.column),
      "message", diagnostic.message.c_str());
    if (!item) {
      // LCOV_EXCL_START CPython allocation failure cleanup
      Py_DECREF(result);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    PyList_SET_ITEM(result, static_cast<Py_ssize_t>(index), item);
  }
  return result;
}

PyObject* buildUnsupportedElements(
  const std::vector<SNLSVUnsupportedElement>& unsupportedElements) {
  PyObject* result = PyList_New(static_cast<Py_ssize_t>(unsupportedElements.size()));
  if (!result) {
    return nullptr; // LCOV_EXCL_LINE CPython allocation failure
  }
  for (size_t index = 0; index < unsupportedElements.size(); ++index) {
    const auto& unsupported = unsupportedElements[index];
    PyObject* item = Py_BuildValue(
      "{s:s,s:s}",
      "message", unsupported.message.c_str(),
      "code", unsupported.code.c_str());
    if (!item) {
      // LCOV_EXCL_START CPython allocation failure cleanup
      Py_DECREF(result);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    PyList_SET_ITEM(result, static_cast<Py_ssize_t>(index), item);
  }
  return result;
}

void raiseSystemVerilogError(
  PyObject* exceptionType,
  const std::string& message,
  const char* attributeName = nullptr,
  PyObject* attributeValue = nullptr) {
  if (attributeName && !attributeValue) {
    return; // LCOV_EXCL_LINE CPython allocation failure already set an exception
  }
  PyObject* exception = PyObject_CallFunction(exceptionType, "s", message.c_str());
  if (!exception) {
    // LCOV_EXCL_START CPython allocation failure cleanup
    Py_XDECREF(attributeValue);
    return;
    // LCOV_EXCL_STOP
  }
  if (attributeName && attributeValue) {
    if (PyObject_SetAttrString(exception, attributeName, attributeValue) < 0) {
      // LCOV_EXCL_START CPython attribute failure cleanup
      Py_DECREF(attributeValue);
      Py_DECREF(exception);
      return;
      // LCOV_EXCL_STOP
    }
    Py_DECREF(attributeValue);
  }
  PyErr_SetObject(exceptionType, exception);
  Py_DECREF(exception);
}

}  // namespace

int PyNLDB_addSystemVerilogExceptions(PyObject* module) {
  auto addException = [module](
    const char* qualifiedName, const char* moduleName, PyObject* base, PyObject*& result) {
    result = PyErr_NewException(qualifiedName, base, nullptr);
    if (!result) {
      return -1; // LCOV_EXCL_LINE CPython allocation failure
    }
    Py_INCREF(result);
    if (PyModule_AddObject(module, moduleName, result) < 0) {
      // LCOV_EXCL_START CPython module failure cleanup
      Py_DECREF(result);
      Py_DECREF(result);
      result = nullptr;
      return -1;
      // LCOV_EXCL_STOP
    }
    return 0;
  };

  if (addException(
        "naja.SystemVerilogError", "SystemVerilogError",
        PyExc_RuntimeError, PySystemVerilogError) < 0 ||
      addException(
        "naja.SystemVerilogSyntaxError", "SystemVerilogSyntaxError",
        PySystemVerilogError, PySystemVerilogSyntaxError) < 0 ||
      addException(
        "naja.SystemVerilogUnsupportedError", "SystemVerilogUnsupportedError",
        PySystemVerilogError, PySystemVerilogUnsupportedError) < 0 ||
      addException(
        "naja.SystemVerilogInternalError", "SystemVerilogInternalError",
        PySystemVerilogError, PySystemVerilogInternalError) < 0) {
    return -1; // LCOV_EXCL_LINE CPython exception registration failure
  }
  return 0;
}

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLDB, function)

static PyObject* PyNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.create", &arg)) {
    setError("malformed NLDB create");
    return nullptr;
  }
  if (not IsPyNLUniverse(arg)) {
    setError("NLDB create argument should be a NLUniverse");
    return nullptr;
  }
  auto universe = PYNLUNIVERSE_O(arg);
  NLDB* db = nullptr;
  if (universe == nullptr) {
    setError("NLDB create NLUniverse is null");
    return nullptr;
  }
  TRY
  db = NLDB::create(universe);
  NLCATCH // LCOV_EXCL_LINE should throw if universe is null, already checked
  return PyNLDB_Link(db);
}

static PyObject* PyNLDB_loadNajaIF(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.loadNajaIF", &arg)) {
    setError("malformed NLDB loadNajaIF");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "NLDB loadNajaIF argument should be a file path, got: "
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  NLDB* db = nullptr;
  const std::filesystem::path path(PyUnicode_AsUTF8(arg));
  if (NLUniverse::get() == nullptr) {
    NLUniverse::create();
  }
  TRY 
  db = SNLCapnP::load(path);
  NLUniverse::get()->setTopDesign(db->getTopDesign());
  NLCATCH
  return PyNLDB_Link(db);
}

PyObject* PyNLDB_snapshotManifest(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:naja.snapshotManifest", &arg)) {
    setError("malformed naja snapshotManifest");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "naja snapshotManifest argument should be a file path, got: "
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  const std::filesystem::path path(PyUnicode_AsUTF8(arg));
  PyObject* result = nullptr;
  TRY
  auto manifest = SNLDumpManifest::load(path);
  auto schemaVersion = manifest.getSchemaVersion();
  result = Py_BuildValue(
    "{s:(III),s:s,s:s}",
    "schema_version",
    schemaVersion.getMajor(),
    schemaVersion.getMinor(),
    schemaVersion.getRevision(),
    "producer_version",
    manifest.getProducerVersion().c_str(),
    "producer_git_hash",
    manifest.getProducerGitHash().c_str());
  NLCATCH
  return result;
}

PyObject* PyNLDB_dumpNajaIF(PyNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.dumpNajaIF", &arg)) {
    setError("malformed NLDB dumpNajaIF");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "NLDB dumpNajaIF argument should be a file path, got:"
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  METHOD_HEAD("NLDB.dumpNajaIF()")
  TRY
  SNLCapnP::dump(selfObject, PyUnicode_AsUTF8(arg));
  NLCATCH
  Py_RETURN_NONE;
}

PyObject* PyNLDB_loadLibertyPrimitives(PyNLDB* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.loadLibertyPrimitives", &arg0)) {
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    PyErr_Format(
      PyExc_TypeError,
      "NLDB.loadLibertyPrimitives: files must be a list[str], got %s",
      Py_TYPE(arg0)->tp_name);
    return nullptr;
  }
  if (PyList_Size(arg0) == 0) {
    PyErr_SetString(
      PyExc_ValueError,
      "NLDB.loadLibertyPrimitives: files must contain at least one Liberty path");
    return nullptr;
  }
  SNLLibertyConstructor::Paths paths;
  paths.reserve(static_cast<size_t>(PyList_Size(arg0)));
  for (int i = 0; i < PyList_Size(arg0); ++i) {
    PyObject* object = PyList_GetItem(arg0, i);
    if (not PyUnicode_Check(object)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadLibertyPrimitives: files[%d] must be a str path, got %s",
        i,
        Py_TYPE(object)->tp_name);
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    if (pathStr.empty()) {
      PyErr_Format(
        PyExc_ValueError,
        "NLDB.loadLibertyPrimitives: files[%d] must not be empty",
        i);
      return nullptr;
    }
    paths.emplace_back(pathStr);
  }
  METHOD_HEAD("NLDB.loadLibertyPrimitives()")
  NLLibrary* primitivesLibrary = nullptr;
  primitivesLibrary = selfObject->getLibrary(NLName("PRIMS"));
  if (primitivesLibrary == nullptr) {
    primitivesLibrary =
      NLLibrary::create(selfObject, NLLibrary::Type::Primitives, NLName("PRIMS"));
  }
  // LCOV_EXCL_START
  TRY
  SNLLibertyConstructor constructor(primitivesLibrary);
  constructor.construct(paths);
  NLCATCH
  // LCOV_EXCL_STOP
  Py_RETURN_NONE;
}

static PyObject* loadGateLevelVerilog(
    PyNLDB* self, PyObject* args, PyObject* kwargs, const char* methodName) {
  const std::string parseFormat = std::string("O|pppOp:") + methodName;
  PyObject* files = nullptr;
  int keep_assigns = 1;  // Default: true
  int blackbox_unknown_modules = 0; // Default: false
  int preprocess_enabled = 0; // Default: false
  int yosys_source_attributes = 0;
  PyObject* conflicting_design_name_policy = nullptr; // Optional: string

  static const char* const kwords[] = {
    "files", "keep_assigns", "blackbox_unknown_modules", "preprocess_enabled",
    "conflicting_design_name_policy", "yosys_source_attributes",
    nullptr
  };

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, parseFormat.c_str(),
    const_cast<char**>(kwords),
    &files, &keep_assigns, &blackbox_unknown_modules, &preprocess_enabled,
    &conflicting_design_name_policy, &yosys_source_attributes)) {
    return nullptr;
  }

  if (not PyList_Check(files)) {
    PyErr_Format(
      PyExc_TypeError,
      "%s: files must be a list[str], got %s",
      methodName, Py_TYPE(files)->tp_name);
    return nullptr;
  }
  if (PyList_Size(files) == 0) {
    PyErr_Format(
      PyExc_ValueError,
      "%s: files must contain at least one Verilog path", methodName);
    return nullptr;
  }
  using Policy = SNLVRLConstructor::Config::ConflictingDesignNamePolicy;
  Policy policy = Policy::Forbid;
  // Conflicting design name policy (optional)
  if (conflicting_design_name_policy != nullptr && conflicting_design_name_policy != Py_None) {
    if (!PyUnicode_Check(conflicting_design_name_policy)) {
      PyErr_Format(
        PyExc_TypeError,
        "%s: conflicting_design_name_policy must be a str or None, "
        "got %s",
        methodName, Py_TYPE(conflicting_design_name_policy)->tp_name);
      return nullptr;
    }

    const std::string s = PyUnicode_AsUTF8(conflicting_design_name_policy);
    if (s == "forbid") {
      policy = Policy::Forbid;
    } else if (s == "first") {
      policy = Policy::FirstOne;
    } else if (s == "last") {
      policy = Policy::LastOne;
    } else if (s == "verify") {
      policy = Policy::VerifyEquality;
    } else {
      PyErr_Format(
        PyExc_ValueError,
        "%s: invalid conflicting_design_name_policy '%s'; "
        "expected one of: forbid, first, last, verify",
        methodName, s.c_str());
      return nullptr;
    }
  }
  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  for (int i = 0; i < PyList_Size(files); ++i) {
    PyObject* object = PyList_GetItem(files, i);
    if (not PyUnicode_Check(object)) {
      PyErr_Format(
        PyExc_TypeError,
        "%s: files[%d] must be a str path, got %s",
        methodName, i,
        Py_TYPE(object)->tp_name);
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    if (pathStr.empty()) {
      PyErr_Format(
        PyExc_ValueError,
        "%s: files[%d] must not be empty",
        methodName, i);
      return nullptr;
    }
    const std::filesystem::path path(pathStr);
    inputPaths.push_back(path);
  }
  METHOD_HEAD("NLDB.loadGateLevelVerilog()")
  NLDB* db = selfObject;
  SNLDesign* top = nullptr;
  TRY
  // The loader can be called multiple times; the last load determines the top.
  NLLibrary* designLibrary = db->getLibrary(NLName("DESIGN"));
  if (designLibrary == nullptr) {
    designLibrary = NLLibrary::create(db, NLName("DESIGN"));
  }
  SNLVRLConstructor constructor(designLibrary);
  constructor.config_.blackboxUnknownModules_ = blackbox_unknown_modules;
  constructor.config_.preprocessEnabled_ = preprocess_enabled;
  constructor.config_.yosysSourceAttributes_ = yosys_source_attributes;
  constructor.config_.conflictingDesignNamePolicy_ = policy;
  try {
    constructor.construct(inputPaths);
  } catch (const std::exception& e) {
    const auto location = constructor.getCurrentLocation();
    std::ostringstream error;
    error << "Error while parsing Verilog";
    if (not location.currentPath_.empty()) {
      error << " at " << location.currentPath_.string();
      if (location.line_ != 0) {
        error << ":" << location.line_;
        if (location.column_ != 0) {
          error << ":" << location.column_;
        }
      }
    }
    error << ": ";
    setError(error.str() + e.what());
    return nullptr;
  }
  if (not keep_assigns) {
    db->mergeAssigns();
  }
  top = SNLUtils::findTop(designLibrary);
  if (top) {
    NLUniverse::get()->setTopDesign(top);
    NLUniverse::get()->setTopDB(top->getDB());
  } else {
    // LCOV_EXCL_START
    setError(
      "No top design was found after parsing verilog. Ensure the input has "
      "exactly one uninstantiated root module.");
    return nullptr;
    //LCOV_EXCL_STOP
  }
  NLCATCH
  return PySNLDesign_Link(top);
}

// HDL loaders share the same destination-library convention.
static bool parseHDLLibrary(PyObject* object, std::string& name) {
  name = "DESIGN";
  if (!object) return true;
  if (!PyUnicode_Check(object)) {
    PyErr_SetString(PyExc_TypeError, "library must be a str");
    return false;
  }
  const auto* text = PyUnicode_AsUTF8(object);
  if (!text) return false;
  name = text;
  if (name.empty() || name.find_first_not_of(" \t\r\n") == std::string::npos) {
    PyErr_SetString(PyExc_ValueError, "library must not be empty");
    return false;
  }
  return true;
}

static NLLibrary* hdlDestination(NLDB* db, const std::string& name) {
  auto* result = findHDLLibrary(db, name);
  return result ? result : NLLibrary::create(db, NLName(name));
}


PyObject* PyNLDB_loadVerilog(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  if (PyErr_WarnEx(PyExc_FutureWarning,
      "NLDB.loadVerilog will be deprecated soon; use NLDB.loadGateLevelVerilog instead.", 1) < 0) {
    return nullptr;
  }
  return loadGateLevelVerilog(self, args, kwargs, "NLDB.loadVerilog");
}

static PyObject* PyNLDB_loadGateLevelVerilog(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  return loadGateLevelVerilog(self, args, kwargs, "NLDB.loadGateLevelVerilog");
}

PyObject* PyNLDB_loadVHDL(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  VHDLIntent::invalidate(self->object_);
  PyObject* file = nullptr;
  PyObject* topObject = nullptr;
  PyObject* libraryObject = nullptr;
  PyObject* diagnosticsReportPath = nullptr;
  int keepASTLink = 0;
  static const char* const kwords[] = {"file", "top", "diagnostics_report_path", "library", "keep_ast_link", nullptr};
  // Keep the legacy file keyword and positional argument order.
  if (kwargs && PyDict_GetItemString(kwargs, "files")) {
    if (PyDict_GetItemString(kwargs, "file")) {
      PyErr_SetString(PyExc_TypeError, "NLDB.loadVHDL: use either file or files");
      return nullptr;
    }
    PyObject* normalized = PyDict_Copy(kwargs);
    if (!normalized) return nullptr;
    PyDict_SetItemString(normalized, "file", PyDict_GetItemString(kwargs, "files"));
    PyDict_DelItemString(normalized, "files");
    PyObject* result = PyNLDB_loadVHDL(self, args, normalized);
    Py_DECREF(normalized);
    return result;
  }
  if (not PyArg_ParseTupleAndKeywords(
      args, kwargs, "O|OOOp:NLDB.loadVHDL", const_cast<char**>(kwords),
      &file, &topObject, &diagnosticsReportPath, &libraryObject, &keepASTLink)) {
    return nullptr;
  }
  std::string libraryName;
  if (!parseHDLLibrary(libraryObject, libraryName)) return nullptr;
  std::vector<std::filesystem::path> paths;
  const bool batch = PyList_Check(file);
  const auto appendPath = [&](PyObject* item) {
    if (!PyUnicode_Check(item)) {
      PyErr_Format(PyExc_TypeError,
          "NLDB.loadVHDL: file must be a str path or list[str], got %s",
          Py_TYPE(item)->tp_name);
      return false;
    }
    const auto* value = PyUnicode_AsUTF8(item);
    if (!value) return false;
    if (!*value) {
      PyErr_SetString(PyExc_ValueError, "NLDB.loadVHDL: file must not be empty");
      return false;
    }
    paths.emplace_back(value);
    return true;
  };
  if (batch) {
    if (!PyList_Size(file)) {
      PyErr_SetString(PyExc_ValueError, "NLDB.loadVHDL: files must not be empty");
      return nullptr;
    }
    for (Py_ssize_t i = 0; i < PyList_Size(file); ++i)
      if (!appendPath(PyList_GetItem(file, i))) return nullptr;
  } else if (!appendPath(file)) return nullptr;
  std::string top;
  if (topObject != nullptr && topObject != Py_None) {
    if (not PyUnicode_Check(topObject)) {
      PyErr_Format(
        PyExc_TypeError, "NLDB.loadVHDL: top must be a str or None, got %s",
        Py_TYPE(topObject)->tp_name);
      return nullptr;
    }
    top = PyUnicode_AsUTF8(topObject);
    if (top.empty()) {
      PyErr_SetString(PyExc_ValueError, "NLDB.loadVHDL: top must not be empty");
      return nullptr;
    }
  }

  VHDLConstructor::ConstructOptions options;
  options.keepASTLink = keepASTLink;
  if (diagnosticsReportPath == Py_None) {
    options.diagnosticsReportPath.reset();
  } else if (diagnosticsReportPath != nullptr) {
    if (!PyUnicode_Check(diagnosticsReportPath)) {
      PyErr_SetString(PyExc_TypeError,
        "NLDB.loadVHDL: diagnostics_report_path must be a str or None");
      return nullptr;
    }
    const std::string destination = PyUnicode_AsUTF8(diagnosticsReportPath);
    if (destination.empty()) {
      PyErr_SetString(PyExc_ValueError,
        "NLDB.loadVHDL: diagnostics_report_path must not be empty; use None for console-only");
      return nullptr;
    }
    options.diagnosticsReportPath = destination;
  }

  if (PyErr_WarnEx(
        PyExc_RuntimeWarning,
        "The VHDL parser is in Beta mode; its supported language subset and behavior may change.",
        1) < 0) {
    return nullptr;
  }

  METHOD_HEAD("NLDB.loadVHDL()")
  NLDB* db = selfObject;
  SNLDesign* design = nullptr;
  TRY
  auto* designLibrary = hdlDestination(db, libraryName);
  const VHDLConstructor constructor(designLibrary, options);
  design = batch ? constructor.constructFiles(paths, top) :
      constructor.constructFile(paths.front(), top);
  if (design) NLUniverse::get()->setTopDesign(design);
  NLUniverse::get()->setTopDB(db);
  NLCATCH
  if (!design) Py_RETURN_NONE;
  return PySNLDesign_Link(design);
}

PyObject* PyNLDB_loadSystemVerilog(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  PyObject* files = nullptr;
  PyObject* libraryObject = nullptr;
  int keep_assigns = 1;  // Default: true
  PyObject* elaborated_ast_json_path = nullptr;  // Optional: string
  int pretty_print_elaborated_ast_json = 1;  // Default: true
  int include_source_info_in_elaborated_ast_json = 1;  // Default: true
  PyObject* flist = nullptr;  // Optional: string path passed as a frontend file list
  PyObject* diagnostics_report_path = nullptr;  // Optional: string
  PyObject* defines = nullptr;  // Optional: list of preprocessor defines
  PyObject* suppress_warnings = nullptr;  // Optional: list of warning names
  int keep_ast_link = 0;  // Default: false
  int blackbox_unknown_modules = 0;  // Default: false
  int blackbox_multi_writer_memories = 0;  // Default: false
  PyObject* frontend_options = nullptr;  // Optional: backend-specific option tokens

  static const char* const kwords[] = {
    "files", "keep_assigns", "elaborated_ast_json_path",
    "pretty_print_elaborated_ast_json", "include_source_info_in_elaborated_ast_json", "flist",
    "diagnostics_report_path", "defines", "suppress_warnings", "keep_ast_link",
    "blackbox_unknown_modules", "library", "blackbox_multi_writer_memories", "frontend_options",
    nullptr
  };

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, "O|pOppOOOOppOpO:NLDB.loadSystemVerilog",
    const_cast<char**>(kwords),
    &files, &keep_assigns, &elaborated_ast_json_path,
    &pretty_print_elaborated_ast_json,
    &include_source_info_in_elaborated_ast_json, &flist, &diagnostics_report_path,
    &defines, &suppress_warnings, &keep_ast_link, &blackbox_unknown_modules, &libraryObject,
    &blackbox_multi_writer_memories, &frontend_options)) {
    return nullptr;
  }

  std::string libraryName;
  if (!parseHDLLibrary(libraryObject, libraryName)) return nullptr;
  if (not PyList_Check(files)) {
    PyErr_Format(
      PyExc_TypeError,
      "NLDB.loadSystemVerilog: files must be a list[str], got %s",
      Py_TYPE(files)->tp_name);
    return nullptr;
  }

  SNLSVConstructor::ConstructOptions options;
  options.prettyPrintElaboratedASTJson = pretty_print_elaborated_ast_json;
  options.includeSourceInfoInElaboratedASTJson =
    include_source_info_in_elaborated_ast_json;
  options.keepASTLink = keep_ast_link;
  options.blackboxUnknownModules = blackbox_unknown_modules;
  options.blackboxMultiWriterMemories = blackbox_multi_writer_memories;
  if (frontend_options != nullptr && frontend_options != Py_None) {
    if (!PyDict_Check(frontend_options)) {
      PyErr_SetString(PyExc_TypeError,
        "NLDB.loadSystemVerilog: frontend_options must be a dict[str, list[str]] or None");
      return nullptr;
    }
    PyObject* key = nullptr;
    PyObject* arguments = nullptr;
    Py_ssize_t position = 0;
    while (PyDict_Next(frontend_options, &position, &key, &arguments)) {
      if (!PyUnicode_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "NLDB.loadSystemVerilog: frontend_options keys must be strings");
        return nullptr;
      }
      if (PyUnicode_CompareWithASCIIString(key, "slang") != 0) {
        PyErr_SetString(PyExc_ValueError,
          "NLDB.loadSystemVerilog: frontend_options supports only the active frontend 'slang'");
        return nullptr;
      }
      if (!PyList_Check(arguments)) {
        PyErr_SetString(PyExc_TypeError, "NLDB.loadSystemVerilog: frontend_options['slang'] must be a list[str]");
        return nullptr;
      }
      auto& tokens = options.frontendOptions["slang"];
      for (Py_ssize_t i = 0; i < PyList_Size(arguments); ++i) {
        PyObject* item = PyList_GetItem(arguments, i);
        if (!PyUnicode_Check(item)) {
          PyErr_Format(PyExc_TypeError, "NLDB.loadSystemVerilog: frontend_options['slang'][%zd] must be a str", i);
          return nullptr;
        }
        Py_ssize_t length = 0;
        const char* value = PyUnicode_AsUTF8AndSize(item, &length);
        if (!value) return nullptr;
        std::string argument(value, length);
        if (argument.empty() || argument.find('\0') != std::string::npos) {
          PyErr_Format(PyExc_ValueError,
            "NLDB.loadSystemVerilog: frontend_options['slang'][%zd] must be nonempty and contain no NUL bytes", i);
          return nullptr;
        }
        tokens.push_back(std::move(argument));
      }
    }
  }

  if (elaborated_ast_json_path != nullptr &&
      elaborated_ast_json_path != Py_None) {
    if (not PyUnicode_Check(elaborated_ast_json_path)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadSystemVerilog: elaborated_ast_json_path must be a str or None, "
        "got %s",
        Py_TYPE(elaborated_ast_json_path)->tp_name);
      return nullptr;
    }
    const std::string path = PyUnicode_AsUTF8(elaborated_ast_json_path);
    if (path.empty()) {
      PyErr_SetString(
        PyExc_ValueError,
        "NLDB.loadSystemVerilog: elaborated_ast_json_path must not be empty");
      return nullptr;
    }
    options.elaboratedASTJsonPath = std::filesystem::path(path);
  }
  if (diagnostics_report_path == Py_None) {
    options.diagnosticsReportPath.reset();
  } else if (diagnostics_report_path != nullptr) {
    if (not PyUnicode_Check(diagnostics_report_path)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadSystemVerilog: diagnostics_report_path must be a str or None, "
        "got %s",
        Py_TYPE(diagnostics_report_path)->tp_name);
      return nullptr;
    }
    const std::string path = PyUnicode_AsUTF8(diagnostics_report_path);
    if (path.empty()) {
      PyErr_SetString(
        PyExc_ValueError,
        "NLDB.loadSystemVerilog: diagnostics_report_path must not be empty; "
        "pass None to disable the report");
      return nullptr;
    }
    options.diagnosticsReportPath = std::filesystem::path(path);
  }

  if (suppress_warnings != nullptr && suppress_warnings != Py_None) {
    if (not PyList_Check(suppress_warnings)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadSystemVerilog: suppress_warnings must be a list[str] or None, "
        "got %s",
        Py_TYPE(suppress_warnings)->tp_name);
      return nullptr;
    }
    for (Py_ssize_t i = 0; i < PyList_Size(suppress_warnings); ++i) {
      PyObject* item = PyList_GetItem(suppress_warnings, i);
      if (not PyUnicode_Check(item)) {
        PyErr_Format(
          PyExc_TypeError,
          "NLDB.loadSystemVerilog: suppress_warnings[%zd] must be a str, got %s",
          i,
          Py_TYPE(item)->tp_name);
        return nullptr;
      }
      const std::string warning = PyUnicode_AsUTF8(item);
      if (warning.empty()) {
        PyErr_Format(
          PyExc_ValueError,
          "NLDB.loadSystemVerilog: suppress_warnings[%zd] must not be empty",
          i);
        return nullptr;
      }
      if (warning.find_first_of(" \t\r\n") != std::string::npos) {
        PyErr_Format(
          PyExc_ValueError,
          "NLDB.loadSystemVerilog: suppress_warnings[%zd] must not contain "
          "whitespace; got '%s'",
          i,
          warning.c_str());
        return nullptr;
      }
      if (warning.starts_with("-W")) {
        PyErr_Format(
          PyExc_ValueError,
          "NLDB.loadSystemVerilog: suppress_warnings[%zd] must be the warning "
          "name without a -W/-Wno- prefix; got '%s'",
          i,
          warning.c_str());
        return nullptr;
      }
      options.suppressWarnings.push_back(warning);
    }
  }

  if (defines != nullptr && defines != Py_None) {
    if (not PyList_Check(defines)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadSystemVerilog: defines must be a list[str] or None, got %s",
        Py_TYPE(defines)->tp_name);
      return nullptr;
    }
    for (Py_ssize_t i = 0; i < PyList_Size(defines); ++i) {
      PyObject* item = PyList_GetItem(defines, i);
      if (not PyUnicode_Check(item)) {
        PyErr_Format(
          PyExc_TypeError,
          "NLDB.loadSystemVerilog: defines[%zd] must be a str, got %s",
          i,
          Py_TYPE(item)->tp_name);
        return nullptr;
      }
      const std::string define = PyUnicode_AsUTF8(item);
      if (define.empty()) {
        PyErr_Format(
          PyExc_ValueError,
          "NLDB.loadSystemVerilog: defines[%zd] must not be empty",
          i);
        return nullptr;
      }
      if (define.find_first_of(" \t\r\n") != std::string::npos) {
        PyErr_Format(
          PyExc_ValueError,
          "NLDB.loadSystemVerilog: defines[%zd] must not contain whitespace; "
          "got '%s'",
          i,
          define.c_str());
        return nullptr;
      }
      options.preprocessorDefines.push_back(define);
    }
  }

  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  if (flist != nullptr && flist != Py_None) {
    if (not PyUnicode_Check(flist)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadSystemVerilog: flist must be a str or None, got %s",
        Py_TYPE(flist)->tp_name);
      return nullptr;
    }
    const std::string flistPath = PyUnicode_AsUTF8(flist);
    if (flistPath.empty()) {
      PyErr_SetString(
        PyExc_ValueError,
        "NLDB.loadSystemVerilog: flist must not be empty");
      return nullptr;
    }
    inputPaths.emplace_back(std::filesystem::path("-f"));
    inputPaths.emplace_back(std::filesystem::path(flistPath));
  }
  for (int i = 0; i < PyList_Size(files); ++i) {
    PyObject* object = PyList_GetItem(files, i);
    if (not PyUnicode_Check(object)) {
      PyErr_Format(
        PyExc_TypeError,
        "NLDB.loadSystemVerilog: files[%d] must be a str path, got %s",
        i,
        Py_TYPE(object)->tp_name);
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    if (pathStr.empty()) {
      PyErr_Format(
        PyExc_ValueError,
        "NLDB.loadSystemVerilog: files[%d] must not be empty",
        i);
      return nullptr;
    }
    const std::filesystem::path path(pathStr);
    inputPaths.push_back(path);
  }
  if (inputPaths.empty()) {
    PyErr_SetString(
      PyExc_ValueError,
      "NLDB.loadSystemVerilog: provide at least one files path or flist");
    return nullptr;
  }

  METHOD_HEAD("NLDB.loadSystemVerilog()")
  NLDB* db = selfObject;
  SNLDesign* top = nullptr;
  TRY
  auto* designLibrary = hdlDestination(db, libraryName);
  VHDLIntent::invalidate(db);
  SNLSVConstructor constructor(designLibrary);
  try {
    constructor.construct(inputPaths, options);
  } catch (const std::exception& e) {
    std::ostringstream error;
    error << "Error while parsing SystemVerilog: ";
    const auto message = error.str() + e.what();
    if (const auto* syntaxError = dynamic_cast<const SNLSVSyntaxError*>(&e)) {
      raiseSystemVerilogError(
        PySystemVerilogSyntaxError,
        message,
        "diagnostics",
        buildDiagnostics(syntaxError->getDiagnostics()));
    } else if (const auto* unsupportedError =
                 dynamic_cast<const SNLSVUnsupportedConstructError*>(&e)) {
      raiseSystemVerilogError(
        PySystemVerilogUnsupportedError,
        message,
        "unsupported_elements",
        buildUnsupportedElements(unsupportedError->getUnsupportedElements()));
    } else {
      raiseSystemVerilogError(PySystemVerilogInternalError, message);
    }
    return nullptr;
  }

  if (not keep_assigns) {
    db->mergeAssigns();
  }
  top = SNLUtils::findTop(designLibrary);
  if (top) {
    NLUniverse::get()->setTopDesign(top);
    NLUniverse::get()->setTopDB(top->getDB());
  } else {
    setError(
      "No top design was found after parsing systemverilog. Ensure the input "
      "has exactly one uninstantiated root module, or select one with a "
      "--top <module> entry in flist.");
    return nullptr;
  }
  NLCATCH
  return PySNLDesign_Link(top);
}

static PyObject* PyNLDB_dumpGateLevelVerilog(PyNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.dumpGateLevelVerilog", &arg)) {
    setError("malformed NLDB dumpGateLevelVerilog");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    setError("NLDB dumpGateLevelVerilog argument should be a file path");
    return nullptr;
  }
  std::ofstream output(PyUnicode_AsUTF8(arg));
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(self->object_->getTopDesign(), output);
  Py_RETURN_NONE;
}

PyObject* PyNLDB_dumpVerilog(PyNLDB* self, PyObject* args) {
  if (PyErr_WarnEx(PyExc_FutureWarning,
      "NLDB.dumpVerilog will be deprecated soon; use NLDB.dumpGateLevelVerilog instead.", 1) < 0) {
    return nullptr;
  }
  return PyNLDB_dumpGateLevelVerilog(self, args);
}

PyObject* PyNLDB_getLibrary(PyNLDB* self, PyObject* arg) {
  NLLibrary* lib = nullptr;
  METHOD_HEAD("NLDB.getLibrary()")
  if (PyUnicode_Check(arg)) {
    const char* name = PyUnicode_AsUTF8(arg);
    lib = selfObject->getLibrary(NLName(name));
  } else if (PyLong_Check(arg)) {
    int index = PyLong_AsLong(arg);
    lib = selfObject->getLibrary(index);
  } else {
      setError("invalid number of parameters for getLibrary.");
      return nullptr;
  }
  return PyNLLibrary_Link(lib);
}

DirectGetNumericMethod(PyNLDB_getID, getID, PyNLDB, NLDB)
DirectGetNLIDMethod(PyNLDB_getNLID, PyNLDB, NLDB)

GetBoolAttribute(NLDB, isTopDB)
GetObjectMethod(NLDB, SNLDesign, getTopDesign)

GetContainerMethod(NLDB, NLLibrary*, NLLibraries, Libraries)
GetContainerMethod(NLDB, NLLibrary*, NLLibraries, GlobalLibraries)
GetContainerMethod(NLDB, NLLibrary*, NLLibraries, PrimitiveLibraries)
GetContainerMethod(NLDB, NLClock*, NLClocks, Clocks)

static PyObject* PyNLDB_destroy(PyNLDB* self) {
  TRY
  if (self->object_ == nullptr) {
    setError("applying a destroy() to a Python object with no Hurricane object attached");
    return nullptr;
  }
  naja::NajaPythonProperty* proxy = static_cast<naja::NajaPythonProperty*>(
    self->object_->getProperty(naja::NajaPythonProperty::getPropertyName()));
  if (proxy == nullptr) { // Defensive: PyNLDB_Link always installs the proxy.
    setError("Trying to destroy() a Hurricane object of with no Proxy attached "); // LCOV_EXCL_LINE
    return nullptr; // LCOV_EXCL_LINE
  }
  SNLSVLiveASTLinkRegistry::clear(self->object_);
  self->object_->destroy();
  self->object_ = nullptr;
  NLCATCH
  Py_RETURN_NONE;
}

PyMethodDef PyNLDB_Methods[] = {
  { "create", (PyCFunction)PyNLDB_create, METH_VARARGS | METH_STATIC,
    "create a NLDB."},
  { "getID", (PyCFunction)PyNLDB_getID, METH_NOARGS,
    "get the NLDB ID."},
  { "getNLID", (PyCFunction)PyNLDB_getNLID, METH_NOARGS,
    "get the NLID."},
  { "isTopDB", (PyCFunction)PyNLDB_isTopDB, METH_NOARGS,
    "Returns True if the NLDB is the top DB."},
  { "getClocks", (PyCFunction)PyNLDB_getClocks, METH_NOARGS,
    "get a container of every NLClock defined in the designs of this NLDB."},
  { "getTopDesign", (PyCFunction)PyNLDB_getTopDesign, METH_NOARGS,
    "get the top design."},
  { "loadNajaIF", (PyCFunction)PyNLDB_loadNajaIF, METH_VARARGS | METH_STATIC,
    "create a NLDB from NajaIF format."},
  { "dumpNajaIF", (PyCFunction)PyNLDB_dumpNajaIF, METH_VARARGS,
    "dump this NLDB to NajaIF format."},
  { "loadLibertyPrimitives", (PyCFunction)PyNLDB_loadLibertyPrimitives, METH_VARARGS,
    "import primitives from Liberty format."},
  { "loadVerilog", (PyCFunction)PyNLDB_loadVerilog, METH_VARARGS|METH_KEYWORDS,
    "Compatibility alias for loadGateLevelVerilog; will be deprecated soon. Emits FutureWarning."},
  { "loadGateLevelVerilog", (PyCFunction)PyNLDB_loadGateLevelVerilog, METH_VARARGS|METH_KEYWORDS,
    "create a design from gate-level (structural) Verilog netlists.\n"
    "This loader does not support general behavioral RTL. Use loadSystemVerilog\n"
    "for Verilog or SystemVerilog RTL, including .v files, within its supported subset.\n\n"
    "Args:\n"
    "  files (list[str]): input gate-level Verilog files\n"
    "  keep_assigns (bool, optional): keep continuous assigns (default True)\n"
    "  blackbox_unknown_modules (bool, optional): create unknown modules as blackboxes (default False)\n"
    "  preprocess_enabled (bool, optional): enable Verilog preprocessing (default False)\n"
    "  yosys_source_attributes (bool, optional): explicitly decode Yosys src origins (default False)\n"
    "  conflicting_design_name_policy (str, optional): how to handle duplicate module names in the same library. "
    "Accepted values: 'forbid' (default), 'first', 'last', 'verify'."},
  { "loadVHDL", (PyCFunction)PyNLDB_loadVHDL, METH_VARARGS|METH_KEYWORDS,
    "load VHDL source files; package-only files return None.\n"
    "A single-file entity with required generics is retained and returns None when top is omitted.\n"
    "An explicit top selects the root; batches otherwise infer a unique uninstantiated entity.\n\n"
    "Warning:\n"
    "  VHDL support is experimental and uses a restricted two-state RTL subset.\n\n"
    "Args:\n"
    "  file (str | list[str]): input files in any order; files is a keyword alias\n"
    "  library (str): destination root library (default DESIGN)\n"
    "  diagnostics_report_path (str | None): all warning occurrences; defaults to naja_vhdl_diagnostics.log.\n"
    "  top (str | None, optional): entity selected as the structural top\n"
    "  keep_ast_link (bool): retain copied VHDL source intent for this load (default False)"},
  { "loadSystemVerilog", (PyCFunction)PyNLDB_loadSystemVerilog, METH_VARARGS|METH_KEYWORDS,
    "elaborate Verilog or SystemVerilog RTL using slang, including .v and .sv files.\n"
    "Supports an RTL subset; this is not a full language implementation or a simulator.\n\n"
    "Warning:\n"
    "  SystemVerilog support is under active development and in early beta mode.\n\n"
    "Args:\n"
    "  files (list[str]): input SystemVerilog files\n"
    "  library (str): destination root library (default DESIGN)\n"
    "  keep_assigns (bool, optional): keep continuous assigns (default True)\n"
    "  elaborated_ast_json_path (str, optional): dump elaborated frontend AST JSON to this path\n"
    "  pretty_print_elaborated_ast_json (bool, optional): pretty-print AST JSON (default True)\n"
    "  include_source_info_in_elaborated_ast_json (bool, optional): include source info in AST JSON (default True)\n"
    "  flist (str, optional): frontend file-list command path\n"
    "  diagnostics_report_path (str | None, optional): incremental frontend diagnostics report path "
    "(default naja_sv_diagnostics.log; None disables the report file)\n"
    "  defines (list[str], optional): SystemVerilog preprocessor defines passed as -D<name>[=<value>]\n"
    "  suppress_warnings (list[str], optional): frontend warning names to suppress\n"
    "  frontend_options (dict[str, list[str]] | None, optional): backend-specific option tokens (currently slang; default None)\n"
    "  blackbox_multi_writer_memories (bool, optional): blackbox entire modules with unsupported multi-writer memories (default False)\n"
    "  keep_ast_link (bool, optional): retain live frontend AST to SNL object links when supported "
    "(default False)."},
  { "dumpVerilog", (PyCFunction)PyNLDB_dumpVerilog, METH_VARARGS,
    "Compatibility alias for dumpGateLevelVerilog; will be deprecated soon. Emits FutureWarning."},
  { "dumpGateLevelVerilog", (PyCFunction)PyNLDB_dumpGateLevelVerilog, METH_VARARGS,
    "dump this NLDB as gate-level (structural) Verilog, not the original behavioral RTL."},
  { "getLibrary", (PyCFunction)PyNLDB_getLibrary, METH_O,
    "retrieve a NLLibrary."},
  { "getLibraries", (PyCFunction)PyNLDB_getLibraries, METH_NOARGS,
    "iterate on this NLDB SNLLibraries."},
  { "getGlobalLibraries", (PyCFunction)PyNLDB_getGlobalLibraries, METH_NOARGS,
    "iterate on all the Libraries owned (directly or indirectly) by this NLDB."},
  { "getPrimitiveLibraries", (PyCFunction)PyNLDB_getPrimitiveLibraries, METH_NOARGS,
    "iterate on all the primitive Libraries owned (directly or indirectly) by this NLDB."},
  { "destroy", (PyCFunction)PyNLDB_destroy, METH_NOARGS,
    "destroy this NLDB."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

DBoDeallocMethod(NLDB)

DBoLinkCreateMethod(NLDB)
PyTypeNLFinalObjectWithNLIDLinkPyType(NLDB)
PyTypeObjectDefinitions(NLDB)

}  // namespace PYNAJA
