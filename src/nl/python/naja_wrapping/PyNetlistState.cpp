// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "PyNetlistState.h"
#include <array>
#include <string_view>
#include <unordered_set>
#include <utility>
namespace PYNAJA {
namespace {
uint64_t generation = 0;
constexpr size_t capacity = 512;
std::array<PyCFunction, capacity> originals {};
size_t count = 0;
std::unordered_set<PyTypeObject*> tracked;
// Unknown methods are conservatively writes. Keep this allowlist restricted to
// read operations; future editing APIs must not silently bypass invalidation.
bool readOnly(std::string_view name) {
  for (auto prefix: {"get", "is", "has", "dump"}) {
    if (name.substr(0, std::char_traits<char>::length(prefix)) == prefix) return true;
  }
  return name == "analyzeTiming" || name == "size" || name == "empty" || name == "toTuple";
}
template<size_t I> PyObject* call(PyObject* self, PyObject* args) {
  ++generation;
  return originals[I](self, args);
}
template<size_t I> PyObject* callKeywords(PyObject* self, PyObject* args, PyObject* kwargs) {
  ++generation;
  return reinterpret_cast<PyCFunctionWithKeywords>(originals[I])(self, args, kwargs);
}
template<size_t... I> constexpr auto calls(std::index_sequence<I...>) {
  return std::array<PyCFunction, sizeof...(I)>{call<I>...};
}
template<size_t... I> auto keywordCalls(std::index_sequence<I...>) {
  return std::array<PyCFunction, sizeof...(I)>{reinterpret_cast<PyCFunction>(callKeywords<I>)...};
}
constexpr auto dispatch = calls(std::make_index_sequence<capacity>{});
const auto keywordDispatch = keywordCalls(std::make_index_sequence<capacity>{});
}
uint64_t pythonNetlistGeneration() { return generation; }
int trackNetlistMethods(PyTypeObject* type) {
  if (!tracked.insert(type).second) return 0;
  for (auto* method = type->tp_methods; method && method->ml_name; ++method) {
    if (readOnly(method->ml_name)) continue;
    if (count == capacity || (method->ml_flags & METH_FASTCALL)) {
      // Fixed module method tables fit capacity and do not use FASTCALL.
      // This protects against unsupported native build configurations.
      // LCOV_EXCL_START
      PyErr_SetString(PyExc_RuntimeError, "Cannot track raw netlist editing method");
      return -1;
      // LCOV_EXCL_STOP
    }
    originals[count] = method->ml_meth;
    method->ml_meth = (method->ml_flags & METH_KEYWORDS) ? keywordDispatch[count] : dispatch[count];
    ++count;
  }
  return 0;
}
}
