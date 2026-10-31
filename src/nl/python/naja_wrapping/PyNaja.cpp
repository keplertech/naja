// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "NajaLog.h"
#include "NajaPerf.h"

#include "PyNLUniverse.h"
#include "PyNLID.h"
#include "PyNLDB.h"
#include "PyNLLibrary.h"
#include "PySNLAttribute.h"
#include "PySNLDesign.h"
#include "PySNLParameter.h"
#include "PyNLClock.h"
#include "PyNLClocks.h"
#include "PySNLNetType.h"
#include "PySNLBusNet.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNetBit.h"
#include "PySNLTermDirection.h"
#include "PySNLTermRole.h"
#include "PySNLActiveLevel.h"
#include "PySNLBundleTerm.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTermBit.h"
#include "PySNLBusTerm.h"
#include "PySNLInstance.h"
#include "PySNLInstParameter.h"
#include "PySNLInstTerm.h"
#include "PySNLAttributes.h"
#include "PyNLDBs.h"
#include "PyNLLibraries.h"
#include "PySNLDesigns.h"
#include "PySNLParameters.h"
#include "PySNLInstParameters.h"
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
#include "PySNLScalarTerms.h"
#include "PySNLBusTerms.h"
#include "PySNLBundleTerms.h"
#include "PySNLNets.h"
#include "PySNLBitNets.h"
#include "PySNLScalarNets.h"
#include "PySNLBusNets.h"
#include "PySNLNetComponents.h"
#include "PySNLInstances.h"
#include "PySNLInstTerms.h"
#include "PySNLPath.h"
#include "PySNLOccurrence.h"
#include "PySNLEquipotential.h"
#include "PySNLEquipotentialMode.h"
#include "PyLogicCone.h"
#include "PySNLOccurrences.h"
#include "PySNLUniquifier.h"

#include "NajaVersion.h"
#include "NLObject.h"
#include "SNLSVConstructor.h"
#include "SNLSVIntent.h"
#include "VHDLIntent.h"
#include "NLLibrary.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLNetComponent.h"
#include "SNLTerm.h"

namespace PYNAJA {

namespace {

constexpr const char* kFrontendSymbolCapsuleName = "naja.frontend.Symbol";
constexpr const char* kFrontendCompilationCapsuleName = "naja.frontend.Compilation";

PyObject* wrapNLObject(naja::NL::NLObject* object) {
  using namespace naja::NL;
  if (!object) {
    Py_RETURN_NONE;  // LCOV_EXCL_LINE defensive: live AST links never bind null objects
  }
  if (auto* design = dynamic_cast<SNLDesign*>(object)) {
    return PySNLDesign_Link(design);
  }
  if (auto* instance = dynamic_cast<SNLInstance*>(object)) {
    return PySNLInstance_Link(instance);
  }
  if (auto* net = dynamic_cast<SNLNet*>(object)) {
    return PySNLNet_Link(net);
  }
  if (auto* term = dynamic_cast<SNLTerm*>(object)) {
    return PySNLTerm_Link(term);
  }
  // LCOV_EXCL_START defensive: the live AST registry only binds designs,
  // instances, nets, and terms; keep the generic component/fallback handling.
  if (auto* component = dynamic_cast<SNLNetComponent*>(object)) {
    return PySNLNetComponent_Link(component);
  }
  Py_RETURN_NONE;
  // LCOV_EXCL_STOP
}

std::string formatIntentSourceLoc(const naja::NL::SNLSourceLoc& loc) {
  if (loc.line == 0 || loc.file.empty()) {
    return {};  // LCOV_EXCL_LINE defensive: frontend intent records have source locations
  }
  const auto filename = std::filesystem::path(loc.file.getString()).filename().string();
  return filename + ":" + std::to_string(loc.line);
}

int setDictItem(PyObject* dict, const char* key, PyObject* value) {
  if (!value) {
    return -1;  // LCOV_EXCL_LINE CPython allocation failure
  }
  const int status = PyDict_SetItemString(dict, key, value);
  Py_DECREF(value);
  return status;
}

PyObject* buildIntentParamDict(const naja::NL::SNLSVIntentParam& param) {
  if (param.name.empty()) {
    Py_RETURN_NONE;  // LCOV_EXCL_LINE frontend parameter records are always named
  }
  PyObject* dict = PyDict_New();
  if (!dict) {
    return nullptr;  // LCOV_EXCL_LINE CPython allocation failure
  }
  const auto src = formatIntentSourceLoc(param.loc);
  if (setDictItem(dict, "name", PyUnicode_FromString(param.name.c_str())) < 0 ||
      setDictItem(dict, "value", PyUnicode_FromString(param.value.c_str())) < 0 ||
      setDictItem(dict, "expr", PyUnicode_FromString(param.expr.c_str())) < 0 ||
      setDictItem(dict, "localparam", PyBool_FromLong(param.localParam)) < 0 ||
      setDictItem(dict, "src", PyUnicode_FromString(src.c_str())) < 0) {
    // LCOV_EXCL_START CPython allocation/dictionary insertion failure
    Py_DECREF(dict);
    return nullptr;
    // LCOV_EXCL_STOP
  }
  return dict;
}

PyObject* buildIntentTypeDict(const naja::NL::SNLSVIntentType& type) {
  if (!type.valid) {
    Py_RETURN_NONE;
  }
  PyObject* dict = PyDict_New();
  if (!dict) {
    return nullptr;  // LCOV_EXCL_LINE CPython allocation failure
  }
  const auto src = formatIntentSourceLoc(type.declLoc);
  if (setDictItem(dict, "type", PyUnicode_FromString(type.typeName.c_str())) < 0 ||
      setDictItem(
        dict,
        "canonical_kind",
        PyUnicode_FromString(type.canonicalKind.c_str())) < 0 ||
      setDictItem(dict, "src", PyUnicode_FromString(src.c_str())) < 0) {
    // LCOV_EXCL_START CPython allocation/dictionary insertion failure
    Py_DECREF(dict);
    return nullptr;
    // LCOV_EXCL_STOP
  }
  if (type.isEnum) {
    PyObject* enumDict = PyDict_New();
    if (!enumDict) {
      // LCOV_EXCL_START CPython allocation failure
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    PyObject* members = PyList_New(0);
    if (!members) {
      // LCOV_EXCL_START CPython allocation failure
      Py_DECREF(enumDict);
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP CPython allocation failure
    }
    for (const auto& member : type.members) {
      PyObject* memberDict = PyDict_New();
      if (!memberDict) {
        // LCOV_EXCL_START CPython allocation failure
        Py_DECREF(members);
        Py_DECREF(enumDict);
        Py_DECREF(dict);
        return nullptr;
        // LCOV_EXCL_STOP
      }
      if (setDictItem(memberDict, "name", PyUnicode_FromString(member.name.c_str())) < 0 ||
          setDictItem(
            memberDict,
            "encoding",
            PyUnicode_FromString(member.encoding.c_str())) < 0 ||
          PyList_Append(members, memberDict) < 0) {
        // LCOV_EXCL_START CPython allocation/list insertion failure
        Py_DECREF(memberDict);
        Py_DECREF(members);
        Py_DECREF(enumDict);
        Py_DECREF(dict);
        return nullptr;
        // LCOV_EXCL_STOP
      }
      Py_DECREF(memberDict);
    }
    const auto enumDecl = formatIntentSourceLoc(type.enumDeclLoc);
    if (setDictItem(enumDict, "width", PyLong_FromUnsignedLong(type.enumWidth)) < 0 ||
        setDictItem(enumDict, "decl", PyUnicode_FromString(enumDecl.c_str())) < 0 ||
        setDictItem(enumDict, "members", members) < 0 ||
        setDictItem(dict, "enum", enumDict) < 0) {
      // LCOV_EXCL_START CPython allocation/dictionary insertion failure
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
  } else if (type.isStruct) {
    PyObject* structDict = PyDict_New();
    if (!structDict) {
      // LCOV_EXCL_START CPython allocation failure
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    PyObject* fields = PyList_New(0);
    if (!fields) {
      // LCOV_EXCL_START CPython allocation failure
      Py_DECREF(structDict);
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    for (const auto& field : type.fields) {
      PyObject* fieldDict = PyDict_New();
      if (!fieldDict) {
        // LCOV_EXCL_START CPython allocation failure
        Py_DECREF(fields);
        Py_DECREF(structDict);
        Py_DECREF(dict);
        return nullptr;
        // LCOV_EXCL_STOP
      }
      if (setDictItem(fieldDict, "name", PyUnicode_FromString(field.name.c_str())) < 0 ||
          setDictItem(
            fieldDict,
            "type",
            PyUnicode_FromString(field.typeName.c_str())) < 0 ||
          setDictItem(fieldDict, "msb", PyLong_FromUnsignedLongLong(field.msb)) < 0 ||
          setDictItem(fieldDict, "lsb", PyLong_FromUnsignedLongLong(field.lsb)) < 0 ||
          PyList_Append(fields, fieldDict) < 0) {
        // LCOV_EXCL_START CPython allocation/list insertion failure
        Py_DECREF(fieldDict);
        Py_DECREF(fields);
        Py_DECREF(structDict);
        Py_DECREF(dict);
        return nullptr;
        // LCOV_EXCL_STOP
      }
      Py_DECREF(fieldDict);
    }
    const auto structDecl = formatIntentSourceLoc(type.structDeclLoc);
    if (setDictItem(
          structDict,
          "width",
          PyLong_FromUnsignedLongLong(type.structWidth)) < 0 ||
        setDictItem(
          structDict,
          "decl",
          PyUnicode_FromString(structDecl.c_str())) < 0 ||
        setDictItem(structDict, "fields", fields) < 0 ||
        setDictItem(dict, "struct", structDict) < 0) {
      // LCOV_EXCL_START CPython allocation/dictionary insertion failure
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
  }
  return dict;
}

PyObject* buildVHDLIntentValue(const naja::NL::VHDLIntentValue& record) {
  using Value = naja::NL::VHDLIntentValue;
  if (const auto* value = std::get_if<bool>(&record.value)) return PyBool_FromLong(*value);
  if (const auto* value = std::get_if<int64_t>(&record.value)) return PyLong_FromLongLong(*value);
  if (const auto* value = std::get_if<std::string>(&record.value))
    return PyUnicode_FromStringAndSize(value->data(), value->size());
  if (const auto* value = std::get_if<Value::List>(&record.value)) {
    auto* result = PyList_New(value->size());
    if (!result) return nullptr;
    for (size_t i = 0; i < value->size(); ++i) {
      auto* item = buildVHDLIntentValue((*value)[i]);
      if (!item) { Py_DECREF(result); return nullptr; }
      PyList_SET_ITEM(result, i, item);
    }
    return result;
  }
  if (const auto* value = std::get_if<Value::Dict>(&record.value)) {
    auto* result = PyDict_New();
    if (!result) return nullptr;
    for (const auto& [key, item] : *value)
      if (setDictItem(result, key.c_str(), buildVHDLIntentValue(item)) < 0) {
        // LCOV_EXCL_START CPython allocation/dictionary insertion failure
        Py_DECREF(result);
        return nullptr;
        // LCOV_EXCL_STOP
      }
    return result;
  }
  Py_RETURN_NONE;
}

const naja::NL::VHDLIntentValue* vhdlIntentField(const naja::NL::NLObject* object, const char* name) {
  const auto* record = naja::NL::VHDLIntent::get(object);
  if (!record) return nullptr;
  const auto* dict = std::get_if<naja::NL::VHDLIntentValue::Dict>(&record->value);
  if (!dict) return nullptr;
  const auto found = dict->find(name);
  return found == dict->end() ? nullptr : &found->second;
}

naja::NL::NLObject* parseIntentObject(PyObject* arg, const char* functionName) {
  if (IsPySNLDesign(arg)) {
    return PYSNLDesign_O(arg);
  }
  if (IsPySNLDesignObject(arg)) {
    return PYSNLDesignObject_O(arg);
  }
  setError(std::string(functionName) + " expects an SNLDesign or SNLDesignObject");
  return nullptr;
}

}  // namespace

static PyObject* getVersion(PyObject* self, PyObject* args) {
  return PyUnicode_FromString(naja::NAJA_VERSION.c_str());
}

static PyObject* getGitHash(PyObject* self, PyObject* args) {
  return PyUnicode_FromString(naja::NAJA_GIT_HASH.c_str());
}

static bool parseLogLevel_(const char* levelName,
                           spdlog::level::level_enum& level) {
  std::string levelStr(levelName ? levelName : "");
  level = naja::log::levelFromString(levelStr);
  std::string lowered = levelStr;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (level == spdlog::level::off && lowered != "off") {
    return false;
  }
  return true;
}

static PyObject* logInfo(PyObject* self, PyObject* args) {
  const char* message;

  if (!PyArg_ParseTuple(args, "s", &message)) {
    setError("Failed to parse arguments in logInfo");
    return nullptr;
  }
  NAJA_LOG_INFO("{}", message);
  Py_RETURN_NONE;
}

static PyObject* logMessage(PyObject* self, PyObject* args) {
  const char* levelName = nullptr;
  const char* message = nullptr;
  if (!PyArg_ParseTuple(args, "ss", &levelName, &message)) {
    setError("Failed to parse arguments in log");
    return nullptr;
  }
  spdlog::level::level_enum level;
  if (!parseLogLevel_(levelName, level)) {
    setError("Invalid log level");
    return nullptr;
  }
  naja::log::get()->log(level, "{}", message);
  Py_RETURN_NONE;
}

static PyObject* installLoggingHandler(PyObject* self, PyObject*) {
  PyObject* globals = PyDict_New();
  if (!globals) {
    return nullptr;  // LCOV_EXCL_LINE CPython allocation failure
  }
  PyObject* logFunction = PyObject_GetAttrString(self, "log");
  if (!logFunction) {
    // LCOV_EXCL_START CPython attribute lookup/allocation failure
    Py_DECREF(globals);
    return nullptr;
    // LCOV_EXCL_STOP
  }
  if (PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins()) < 0 ||
      PyDict_SetItemString(globals, "_naja_log", logFunction) < 0) {
    // LCOV_EXCL_START CPython dictionary insertion failure
    Py_DECREF(logFunction);
    Py_DECREF(globals);
    return nullptr;
    // LCOV_EXCL_STOP
  }
  Py_DECREF(logFunction);

  static constexpr const char* installScript = R"PY(
import logging as _logging

def _native_level(level):
    if level >= _logging.CRITICAL:
        return "critical"
    if level >= _logging.ERROR:
        return "error"
    if level >= _logging.WARNING:
        return "warning"
    if level >= _logging.INFO:
        return "info"
    return "debug"

class _NajaNativeLogHandler(_logging.Handler):
    _naja_native_handler = True

    def emit(self, record):
        try:
            _naja_log(_native_level(record.levelno), self.format(record))
        except Exception:
            self.handleError(record)

def _enable_native_delivery(root):
    python_level = root.level
    for handler in root.handlers:
        if (not getattr(handler, "_naja_native_handler", False)
                and handler.level == _logging.NOTSET):
            handler.setLevel(python_level)
    root.setLevel(_logging.DEBUG)

_root_logger = _logging.getLogger()
if not any(getattr(handler, "_naja_native_handler", False)
           for handler in _root_logger.handlers):
    _root_logger.addHandler(_NajaNativeLogHandler())
_enable_native_delivery(_root_logger)

if not getattr(_logging.basicConfig, "_naja_preserves_native_handler", False):
    _original_basic_config = _logging.basicConfig

    def _naja_basic_config(*args, **kwargs):
        root = _logging.getLogger()
        native_handlers = [
            handler for handler in root.handlers
            if getattr(handler, "_naja_native_handler", False)
        ]
        for handler in native_handlers:
            root.removeHandler(handler)
        try:
            return _original_basic_config(*args, **kwargs)
        finally:
            _enable_native_delivery(root)
            for handler in native_handlers:
                if handler not in root.handlers:
                    root.addHandler(handler)

    _naja_basic_config._naja_preserves_native_handler = True
    _logging.basicConfig = _naja_basic_config
)PY";

  PyObject* result = PyRun_String(
    installScript, Py_file_input, globals, globals);
  Py_DECREF(globals);
  if (!result) {
    return nullptr;
  }
  Py_DECREF(result);
  Py_RETURN_NONE;
}

static PyObject* logCritical(PyObject* self, PyObject* args) {
  const char* message;
  if (!PyArg_ParseTuple(args, "s", &message)) {
    setError("Failed to parse arguments in logCritical");
    return nullptr;
  }
  NAJA_LOG_CRITICAL("{}", message);
  Py_RETURN_NONE;
}

static PyObject* logWarn(PyObject* self, PyObject* args) {
  const char* message;
  if (!PyArg_ParseTuple(args, "s", &message)) {
    setError("Failed to parse arguments in logWarning");
    return nullptr;
  }
  NAJA_LOG_WARN("{}", message);
  Py_RETURN_NONE;
}

static PyObject* setLogLevel(PyObject* self, PyObject* args) {
  const char* levelName = nullptr;
  if (!PyArg_ParseTuple(args, "s", &levelName)) {
    setError("Failed to parse arguments in setLogLevel");
    return nullptr;
  }
  spdlog::level::level_enum level;
  if (!parseLogLevel_(levelName, level)) {
    setError("Invalid log level");
    return nullptr;
  }
  naja::log::setLevel(level);
  Py_RETURN_NONE;
}

static PyObject* addLogFile(PyObject* self, PyObject* args) {
  const char* path = nullptr;
  const char* levelName = nullptr;
  if (!PyArg_ParseTuple(args, "s|s", &path, &levelName)) {
    setError("Failed to parse arguments in addLogFile");
    return nullptr;
  }
  std::ofstream testStream(path, std::ios::app);
  if (!testStream.is_open()) {
    NAJA_LOG_WARN("Log file not writable: {}", path);
    Py_RETURN_NONE;
  }
  testStream.close();
  spdlog::level::level_enum level = spdlog::level::trace;
  if (levelName && !parseLogLevel_(levelName, level)) {
    setError("Invalid log level");
    return nullptr;
  }
  naja::log::addFileSink(path, level);
  Py_RETURN_NONE;
}

static PyObject* clearLogSinks(PyObject* self, PyObject* args) {
  naja::log::clearSinks();
  Py_RETURN_NONE;
}

static PyObject* liveCompilation(PyObject*, PyObject*) {
  auto* link = naja::NL::SNLSVLiveASTLinkRegistry::getLatest();
  if (!link || !link->getCompilation()) {
    Py_RETURN_NONE;
  }
  return PyCapsule_New(
    link->getCompilation(),
    kFrontendCompilationCapsuleName,
    nullptr);
}

static PyObject* astSymbolOf(PyObject*, PyObject* arg) {
  naja::NL::NLObject* object = nullptr;
  if (IsPySNLDesign(arg)) {
    object = PYSNLDesign_O(arg);
  } else if (IsPySNLDesignObject(arg)) {
    object = PYSNLDesignObject_O(arg);
  } else {
    setError("astSymbolOf expects an SNLDesign or SNLDesignObject");
    return nullptr;
  }
  auto* link = naja::NL::SNLSVLiveASTLinkRegistry::findForObject(object);
  if (!link) {
    Py_RETURN_NONE;
  }
  auto* symbol = link->getSymbol(object);
  if (!symbol) {
    Py_RETURN_NONE;
  }
  return PyCapsule_New(
    const_cast<slang::ast::Symbol*>(symbol),
    kFrontendSymbolCapsuleName,
    nullptr);
}

static PyObject* snlObjectsOf(PyObject*, PyObject* arg) {
  auto* symbol = static_cast<const slang::ast::Symbol*>(
    PyCapsule_GetPointer(arg, kFrontendSymbolCapsuleName));
  if (!symbol) {
    return nullptr;
  }
  auto* link = naja::NL::SNLSVLiveASTLinkRegistry::findForSymbol(symbol);
  auto* result = PyList_New(0);
  if (!result) {
    return nullptr;  // LCOV_EXCL_LINE CPython allocation failure
  }
  if (!link) {
    return result;
  }
  for (auto* object : link->getObjects(symbol)) {
    PyObject* pyObject = wrapNLObject(object);
    if (!pyObject) {
      // LCOV_EXCL_START CPython wrapper allocation failure
      Py_DECREF(result);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    if (pyObject == Py_None) {
      // LCOV_EXCL_START defensive: all registry-bound object kinds are wrapped
      Py_DECREF(pyObject);
      continue;
      // LCOV_EXCL_STOP
    }
    if (PyList_Append(result, pyObject) < 0) {
      // LCOV_EXCL_START CPython allocation/list insertion failure
      Py_DECREF(pyObject);
      Py_DECREF(result);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    Py_DECREF(pyObject);
  }
  return result;
}

static PyObject* intentDescribe(PyObject*, PyObject* arg) {
  naja::NL::NLObject* object = nullptr;
  if (IsPyNLLibrary(arg)) object = PYNLLibrary_O(arg);
  else object = parseIntentObject(arg, "intentDescribe");
  if (!object) {
    if (!PyErr_Occurred()) setError("intentDescribe received a destroyed object");
    return nullptr;
  }
  if (const auto* record = naja::NL::VHDLIntent::get(object)) return buildVHDLIntentValue(*record);
  return Py_BuildValue("{s:s,s:s}", "status", "unavailable", "reason",
      "No retained VHDL intent for this object in the current load session; re-elaborate in a fresh NLDB with keep_ast_link=True");
}

static PyObject* intentAvailable(PyObject*, PyObject*) {
  if (naja::NL::SNLSVIntent::available() || naja::NL::VHDLIntent::available()) {
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

static PyObject* intentTypeOf(PyObject*, PyObject* arg) {
  auto* object = parseIntentObject(arg, "intentTypeOf");
  if (!object) {
    return nullptr;
  }
  try {
    if (const auto* type = vhdlIntentField(object, "resolved_type")) return buildVHDLIntentValue(*type);
    return buildIntentTypeDict(naja::NL::SNLSVIntent::typeOf(object));
    // LCOV_EXCL_START defensive: SNLSVIntent::typeOf already catches failures
  } catch (...) {
    Py_RETURN_NONE;
  }
  // LCOV_EXCL_STOP
}

static PyObject* intentParametersOf(PyObject*, PyObject* arg) {
  auto* object = parseIntentObject(arg, "intentParametersOf");
  if (!object) {
    return nullptr;
  }
  try {
    if (const auto* declarations = vhdlIntentField(object, "declarations")) {
      naja::NL::VHDLIntentValue::List values;
      for (const auto& declaration : std::get<naja::NL::VHDLIntentValue::List>(declarations->value)) {
        const auto& dict = std::get<naja::NL::VHDLIntentValue::Dict>(declaration.value);
        const auto kind = dict.find("kind");
        if (kind != dict.end()) {
          const auto& name = std::get<std::string>(kind->second.value);
          if (name == "generic" || name == "constant") values.push_back(declaration);
        }
      }
      return buildVHDLIntentValue(naja::NL::VHDLIntentValue::Dict{
          {"status", "available"}, {"language", "vhdl"}, {"params", std::move(values)}});
    }
    const auto params = naja::NL::SNLSVIntent::parametersOf(object);
    if (!params.valid) {
      Py_RETURN_NONE;
    }
    PyObject* dict = PyDict_New();
    if (!dict) {
      return nullptr;  // LCOV_EXCL_LINE CPython allocation failure
    }
    PyObject* paramList = PyList_New(0);
    if (!paramList) {
      // LCOV_EXCL_START CPython allocation failure
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    for (const auto& param : params.params) {
      PyObject* paramDict = buildIntentParamDict(param);
      if (!paramDict) {
        // LCOV_EXCL_START CPython allocation/dictionary insertion failure
        Py_DECREF(paramList);
        Py_DECREF(dict);
        return nullptr;
        // LCOV_EXCL_STOP
      }
      if (paramDict == Py_None) {
        // LCOV_EXCL_START defensive: frontend parameter records are always named
        Py_DECREF(paramDict);
        continue;
        // LCOV_EXCL_STOP
      }
      if (PyList_Append(paramList, paramDict) < 0) {
        // LCOV_EXCL_START CPython allocation/list insertion failure
        Py_DECREF(paramDict);
        Py_DECREF(paramList);
        Py_DECREF(dict);
        return nullptr;
        // LCOV_EXCL_STOP
      }
      Py_DECREF(paramDict);
    }
    if (setDictItem(dict, "module", PyUnicode_FromString(params.module.c_str())) < 0 ||
        setDictItem(dict, "parameters", paramList) < 0 ||
        setDictItem(dict, "count", PyLong_FromSize_t(params.params.size())) < 0) {
      // LCOV_EXCL_START CPython allocation/dictionary insertion failure
      Py_DECREF(dict);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    return dict;
    // LCOV_EXCL_START defensive: SNLSVIntent::parametersOf already catches failures
  } catch (...) {
    Py_RETURN_NONE;
  }
  // LCOV_EXCL_STOP
}

static PyObject* intentPackageMember(PyObject*, PyObject* args) {
  const char* package = nullptr;
  const char* member = nullptr;
  if (!PyArg_ParseTuple(args, "ss", &package, &member)) {
    setError("intentPackageMember expects a package and member name");
    return nullptr;
  }
  try {
    auto param = naja::NL::SNLSVIntent::packageMember(package, member);
    if (!param.name.empty()) {
      return buildIntentParamDict(param);
    }
    return buildIntentTypeDict(
      naja::NL::SNLSVIntent::packageMemberType(package, member));
    // LCOV_EXCL_START defensive: SNLSVIntent package queries already catch failures
  } catch (...) {
    Py_RETURN_NONE;
  }
  // LCOV_EXCL_STOP
}

DEFINE_DEPRECATED_ALIAS(PyNLDB_snapshotManifest_deprecated, PyNLDB_snapshotManifest, "snapshot_manifest", "snapshotManifest")
DEFINE_DEPRECATED_ALIAS(liveCompilation_deprecated, liveCompilation, "live_compilation", "liveCompilation")
DEFINE_DEPRECATED_ALIAS(astSymbolOf_deprecated, astSymbolOf, "ast_symbol_of", "astSymbolOf")
DEFINE_DEPRECATED_ALIAS(snlObjectsOf_deprecated, snlObjectsOf, "snl_objects_of", "snlObjectsOf")
DEFINE_DEPRECATED_ALIAS(intentAvailable_deprecated, intentAvailable, "intent_available", "intentAvailable")
DEFINE_DEPRECATED_ALIAS(intentTypeOf_deprecated, intentTypeOf, "intent_type_of", "intentTypeOf")
DEFINE_DEPRECATED_ALIAS(intentParametersOf_deprecated, intentParametersOf, "intent_parameters_of", "intentParametersOf")
DEFINE_DEPRECATED_ALIAS(intentPackageMember_deprecated, intentPackageMember, "intent_package_member", "intentPackageMember")

static PyMethodDef NajaMethods[] = {
  { "getVersion", getVersion, METH_NOARGS, "get the version of Naja" },
  { "getGitHash", getGitHash, METH_NOARGS, "get the Naja git hash" },
  { "snapshotManifest", PyNLDB_snapshotManifest, METH_VARARGS,
    "read a NajaIF snapshot manifest without loading the snapshot" },
  { "log", logMessage, METH_VARARGS, "log a message at the requested level" },
  { "installLoggingHandler", installLoggingHandler, METH_NOARGS,
    "route standard Python logging through the native Naja logger" },
  { "logInfo", logInfo, METH_VARARGS, "log an info message" },
  { "logWarn", logWarn, METH_VARARGS, "log a warning message" },
  { "logCritical", logCritical, METH_VARARGS, "log a critical message" },
  { "setLogLevel", setLogLevel, METH_VARARGS, "set the global log level" },
  { "addLogFile", addLogFile, METH_VARARGS, "add a file sink to the logger" },
  { "clearLogSinks", clearLogSinks, METH_NOARGS, "clear all log sinks" },
  { "liveCompilation", liveCompilation, METH_NOARGS,
    "Return the live frontend compilation capsule for the latest retained SystemVerilog load." },
  { "astSymbolOf", astSymbolOf, METH_O,
    "Return the live frontend AST symbol capsule associated with an SNL object, or None." },
  { "snlObjectsOf", snlObjectsOf, METH_O,
    "Return SNL objects associated with a live frontend AST symbol capsule." },
  { "intentDescribe", intentDescribe, METH_O,
    "Return a VHDL semantic catalogue for an SNL object or library, with explicit availability status." },
  { "intentAvailable", intentAvailable, METH_NOARGS,
    "Return whether live source intent data is available for SystemVerilog or VHDL." },
  { "intentTypeOf", intentTypeOf, METH_O,
    "Return a plain Python dict describing live source type intent for an SNL object, or None." },
  { "intentParametersOf", intentParametersOf, METH_O,
    "Return a plain Python dict with live source parameter intent for an SNL object, or None." },
  { "intentPackageMember", intentPackageMember, METH_VARARGS,
    "Return a plain Python dict for a live source package member, or None." },
  { "snapshot_manifest", PyNLDB_snapshotManifest_deprecated, METH_VARARGS,
    "deprecated alias of snapshotManifest; emits a DeprecationWarning."},
  { "live_compilation", liveCompilation_deprecated, METH_NOARGS,
    "deprecated alias of liveCompilation; emits a DeprecationWarning."},
  { "ast_symbol_of", astSymbolOf_deprecated, METH_O,
    "deprecated alias of astSymbolOf; emits a DeprecationWarning."},
  { "snl_objects_of", snlObjectsOf_deprecated, METH_O,
    "deprecated alias of snlObjectsOf; emits a DeprecationWarning."},
  { "intent_available", intentAvailable_deprecated, METH_NOARGS,
    "deprecated alias of intentAvailable; emits a DeprecationWarning."},
  { "intent_type_of", intentTypeOf_deprecated, METH_O,
    "deprecated alias of intentTypeOf; emits a DeprecationWarning."},
  { "intent_parameters_of", intentParametersOf_deprecated, METH_O,
    "deprecated alias of intentParametersOf; emits a DeprecationWarning."},
  { "intent_package_member", intentPackageMember_deprecated, METH_VARARGS,
    "deprecated alias of intentPackageMember; emits a DeprecationWarning."},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef najaModule = {
  PyModuleDef_HEAD_INIT,
  "naja",
  "Python interface for Naja API",
  -1,
  NajaMethods
};

PyMODINIT_FUNC PyInit_naja(void) {
  naja::log::initFromEnv();
  // Embedders opt in with NAJA_PERF=1 or an explicit path; absent and empty
  // values leave the performance singleton disabled.
  auto perfLogPath =
    naja::NajaPerf::getLogPathFromEnv("NAJA_PERF", "naja_perf.log");
  naja::NajaPerf::create(perfLogPath, "naja_python");

  PyNLUniverse_LinkPyType();
  PyNLID_LinkPyType();
  PyNLDB_LinkPyType();
  PyNLLibrary_LinkPyType();
  PyNLClock_LinkPyType();
  PySNLAttribute_LinkPyType();
  PySNLDesign_LinkPyType();
  PySNLParameter_LinkPyType();
  PySNLDesignObject_LinkPyType ();
  PySNLNet_LinkPyType();
  PySNLNetType_LinkPyType();
  PySNLBusNet_LinkPyType();
  PySNLBitNet_LinkPyType();
  PySNLScalarNet_LinkPyType();
  PySNLBusNetBit_LinkPyType();
  PySNLNetComponent_LinkPyType ();
  PySNLTerm_LinkPyType();
  PySNLTermDirection_LinkPyType();
  PySNLTermRole_LinkPyType();
  PySNLActiveLevel_LinkPyType();
  PySNLBundleTerm_LinkPyType();
  PySNLBusTerm_LinkPyType();
  PySNLBitTerm_LinkPyType();
  PySNLScalarTerm_LinkPyType();
  PySNLBusTermBit_LinkPyType();
  PySNLInstance_LinkPyType();
  PySNLInstParameter_LinkPyType();
  PySNLInstTerm_LinkPyType();
  PySNLPath_LinkPyType();
  PySNLUniquifier_LinkPyType();
  PySNLEquipotential_LinkPyType();
  PySNLEquipotentialMode_LinkPyType();
  PyLogicCone_LinkPyType();
  PySNLOccurrence_LinkPyType();

  PyNLDBs_LinkPyType();
  PyNLLibraries_LinkPyType();
  PyNLClocks_LinkPyType();
  PySNLDesigns_LinkPyType();
  PySNLAttributes_LinkPyType();
  PySNLParameters_LinkPyType();
  PySNLTerms_LinkPyType();
  PySNLBitTerms_LinkPyType();
  PySNLScalarTerms_LinkPyType();
  PySNLBusTerms_LinkPyType();
  PySNLBundleTerms_LinkPyType();
  PySNLNets_LinkPyType();
  PySNLBitNets_LinkPyType();
  PySNLScalarNets_LinkPyType();
  PySNLBusNets_LinkPyType();
  PySNLNetComponents_LinkPyType();
  PySNLInstances_LinkPyType();
  PySNLInstParameters_LinkPyType();
  PySNLInstTerms_LinkPyType();
  PySNLOccurrences_LinkPyType();

  PYTYPE_READY(SNLAttribute);
  PYTYPE_READY(NLID);
  PYTYPE_READY(NLUniverse);
  PYTYPE_READY(NLDB);
  PYTYPE_READY(NLLibrary);
  PYTYPE_READY(SNLDesign);
  PYTYPE_READY(SNLParameter);
  PYTYPE_READY(SNLInstParameter);
  PYTYPE_READY(SNLDesignObject);
  PYTYPE_READY(SNLTermDirection);
  PYTYPE_READY_SUB(SNLNet, SNLDesignObject);
  PYTYPE_READY(SNLNetType);
  PYTYPE_READY_SUB(SNLBusNet, SNLNet);
  PYTYPE_READY_SUB(SNLBitNet, SNLNet);
  PYTYPE_READY_SUB(SNLScalarNet, SNLBitNet);
  PYTYPE_READY_SUB(SNLBusNetBit, SNLBitNet);
  PYTYPE_READY_SUB(SNLNetComponent, SNLDesignObject);
  PYTYPE_READY_SUB(SNLTerm, SNLNetComponent);
  PYTYPE_READY_SUB(SNLBundleTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLBusTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLBitTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLScalarTerm, SNLBitTerm);
  PYTYPE_READY_SUB(SNLBusTermBit, SNLBitTerm);
  PYTYPE_READY_SUB(SNLInstance, SNLDesignObject);
  PYTYPE_READY_SUB(SNLInstTerm, SNLNetComponent);

  PYTYPE_READY(SNLPath);
  PYTYPE_READY(SNLUniquifier);
  PYTYPE_READY(SNLEquipotentialMode);
  PYTYPE_READY(SNLEquipotential);
  PYTYPE_READY(LogicCone);
  PYTYPE_READY(SNLOccurrence);
  PYTYPE_READY(SNLAttributes);
  PYTYPE_READY(NLDBs);
  PYTYPE_READY(NLDBsIterator);
  PYTYPE_READY(NLLibraries);
  PYTYPE_READY(NLLibrariesIterator);
  PYTYPE_READY(NLClocks);
  PYTYPE_READY(NLClocksIterator);
  PYTYPE_READY(SNLDesigns);
  PYTYPE_READY(SNLDesignsIterator);
  PYTYPE_READY(SNLParameters);
  PYTYPE_READY(SNLParametersIterator);
  PYTYPE_READY(SNLTerms);
  PYTYPE_READY(SNLTermsIterator);
  PYTYPE_READY(SNLBitTerms);
  PYTYPE_READY(SNLBitTermsIterator);
  PYTYPE_READY(SNLScalarTerms);
  PYTYPE_READY(SNLScalarTermsIterator);
  PYTYPE_READY(SNLBusTerms);
  PYTYPE_READY(SNLBusTermsIterator);
  PYTYPE_READY(SNLBundleTerms);
  PYTYPE_READY(SNLBundleTermsIterator);
  PYTYPE_READY(SNLNets);
  PYTYPE_READY(SNLNetsIterator);
  PYTYPE_READY(SNLBitNets);
  PYTYPE_READY(SNLBitNetsIterator);
  PYTYPE_READY(SNLScalarNets);
  PYTYPE_READY(SNLScalarNetsIterator);
  PYTYPE_READY(SNLBusNets);
  PYTYPE_READY(SNLBusNetsIterator);
  PYTYPE_READY(SNLNetComponents);
  PYTYPE_READY(SNLNetComponentsIterator);
  PYTYPE_READY(SNLInstances);
  PYTYPE_READY(SNLInstancesIterator);
  PYTYPE_READY(SNLInstParameters);
  PYTYPE_READY(SNLInstParametersIterator);
  PYTYPE_READY(SNLInstTerms);
  PYTYPE_READY(SNLInstTermsIterator);
  PYTYPE_READY(SNLTermRole);
  PYTYPE_READY(SNLActiveLevel);
  PYTYPE_READY(SNLOccurrences);
  PYTYPE_READY(SNLOccurrencesIterator);

  PyObject* mod = PyModule_Create(&najaModule);

  if (not mod) {
    //LCOV_EXCL_START
    NAJA_LOG_ERROR("Failed to initialize Naja python module.");
    return nullptr;
    //LCOV_EXCL_STOP
  }

  if (PyNLDB_addSystemVerilogExceptions(mod) < 0) {
    // LCOV_EXCL_START CPython exception registration failure cleanup
    Py_DECREF(mod);
    return nullptr;
    // LCOV_EXCL_STOP
  }

  PyModule_AddType(mod, &PyTypeSNLAttribute);
  PyModule_AddType(mod, &PyTypeNLID);
  PyModule_AddType(mod, &PyTypeNLUniverse);
  PyModule_AddType(mod, &PyTypeNLDB);
  PyModule_AddType(mod, &PyTypeNLLibrary);
  PyModule_AddType(mod, &PyTypeSNLDesign);
  PyModule_AddType(mod, &PyTypeSNLParameter);
  PyModule_AddType(mod, &PyTypeNLClock);
  PyModule_AddType(mod, &PyTypeSNLDesignObject);
  PyModule_AddType(mod, &PyTypeSNLNet);
  PyModule_AddType(mod, &PyTypeSNLBusNet);
  PyModule_AddType(mod, &PyTypeSNLBitNet);
  PyModule_AddType(mod, &PyTypeSNLScalarNet);
  PyModule_AddType(mod, &PyTypeSNLBusNetBit);
  PyModule_AddType(mod, &PyTypeSNLNetComponent);
  PyModule_AddType(mod, &PyTypeSNLTerm);
  PyModule_AddType(mod, &PyTypeSNLBundleTerm);
  PyModule_AddType(mod, &PyTypeSNLBusTerm);
  PyModule_AddType(mod, &PyTypeSNLBitTerm);
  PyModule_AddType(mod, &PyTypeSNLScalarTerm);
  PyModule_AddType(mod, &PyTypeSNLBusTermBit);
  PyModule_AddType(mod, &PyTypeSNLInstance);
  PyModule_AddType(mod, &PyTypeSNLInstParameter);
  PyModule_AddType(mod, &PyTypeSNLInstTerm);
  PyModule_AddType(mod, &PyTypeSNLTermRole);
  PyModule_AddType(mod, &PyTypeSNLActiveLevel);
  PyModule_AddType(mod, &PyTypeSNLPath);
  PyModule_AddType(mod, &PyTypeSNLUniquifier);
  PyModule_AddType(mod, &PyTypeSNLOccurrence);
  PyModule_AddType(mod, &PyTypeSNLEquipotential);
  PyModule_AddType(mod, &PyTypeLogicCone);

  PyNLID_postModuleInit();
  PySNLTerm_postModuleInit();
  PySNLNet_postModuleInit();
  PySNLEquipotential_postModuleInit();
  PyLogicCone_postModuleInit();
  PySNLTermRole_postModuleInit();
  PySNLActiveLevel_postModuleInit();

  return mod;
}

}
