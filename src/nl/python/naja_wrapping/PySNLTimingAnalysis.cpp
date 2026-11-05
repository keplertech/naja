// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "PySNLTimingAnalysis.h"

#include <memory>
#include <optional>
#include <unordered_map>

#include "PyInterface.h"
#include "PyNLClock.h"
#include "PySNLDesign.h"
#include "SNLDesign.h"
#include "SNLBitNet.h"
#include "SNLPath.h"
#include "NajaPrivateProperty.h"
#include "PySNLOccurrence.h"
#include "DNL.h"
#include "NLUniverse.h"
#include "TSNLGraphConstructor.h"
#include "TClockPropagator.h"

namespace PYNAJA {
namespace {
using namespace naja::TG;
using namespace naja::NL;

// Private C++ context owned by a property on the analyzed design. The
// constructor owns its TGraph through unique_ptr; Python never sees that graph.
struct SNLTimingAnalysis {
  TSNLGraphConstructor constructor;
  uint64_t generation {pythonNetlistGeneration()};
  bool valid {true};
  std::optional<TClockPropagator::Result> result;
  std::vector<TEdge::ClockMask> driving;
  std::vector<TEdge::ClockMask> receiving;
  std::vector<size_t> endpoints;
};
class TimingProperty final: public naja::NajaPrivateProperty {
 public:
  static constexpr const char* Name = "Naja::PythonTimingAnalysis";
  std::string getName() const override { return Name; }
  std::string getString() const override { return Name; } // LCOV_EXCL_LINE Private-property debug hook, not exposed to Python.
  static TimingProperty* create(SNLDesign* design, std::shared_ptr<SNLTimingAnalysis> state) {
    auto* property = new TimingProperty;
    property->state = std::move(state);
    property->postCreate(design);
    return property;
  }
  void replace(std::shared_ptr<SNLTimingAnalysis> next) {
    state->valid = false;
    state = std::move(next);
  }
  std::shared_ptr<SNLTimingAnalysis> state;
 private:
  ~TimingProperty() override { state->valid = false; }
};
struct View {
  PyObject_HEAD
  std::shared_ptr<SNLTimingAnalysis>* state;
  size_t index;
};
PyTypeObject clockType = {PyVarObject_HEAD_INIT(nullptr, 0)};
PyTypeObject occurrenceType = {PyVarObject_HEAD_INIT(nullptr, 0)};
struct OccurrenceView {
  PySNLOccurrence base;
  std::shared_ptr<SNLTimingAnalysis>* state;
};
PyObject* cycleError = nullptr;

SNLTimingAnalysis* checked(View* self) {
  if (!self->state || !*self->state) {
    // Views are private factory products; Python cannot construct an unbound view.
    // LCOV_EXCL_START
    setError("Unbound timing view");
    return nullptr;
    // LCOV_EXCL_STOP
  }
  auto& state = **self->state;
  // Validate the native owner/cache before touching borrowed SNL objects.
  if (!state.valid || state.generation != pythonNetlistGeneration()) {
    setError("Stale timing analysis: netlist edited, design destroyed, or analysis rebuilt; call analyzeTiming() again");
    return nullptr;
  }
  return &state;
}
#define VIEW_HEAD auto* state = checked(self); if (!state) return nullptr;

void dealloc(View* self) {
  delete self->state;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}
PyObject* view(PyTypeObject& type, const std::shared_ptr<SNLTimingAnalysis>& state, size_t index = 0) {
  auto* object = reinterpret_cast<View*>(type.tp_alloc(&type, 0));
  if (!object) return nullptr;
  try {
    object->state = new std::shared_ptr<SNLTimingAnalysis>(state);
    object->index = index;
  } catch (...) { // LCOV_EXCL_START C++ allocation failure.
    Py_DECREF(object);
    return PyErr_NoMemory();
  } // LCOV_EXCL_STOP
  return reinterpret_cast<PyObject*>(object);
}
const TSNLNodeProxy* proxy(SNLTimingAnalysis& state, size_t id) {
  return static_cast<const TSNLNodeProxy*>(state.constructor.getGraph().getNode(id)->getProxy());
}
const char* roleName(TNodeProxy::Role role) {
  switch (role) {
    case TNodeProxy::Role::Single: return "single";
    case TNodeProxy::Role::Drive: return "drive";
    case TNodeProxy::Role::Load: return "load";
  }
  return "unknown"; // LCOV_EXCL_LINE Defensive fallback; all enum values handled above.
}
bool validOccurrence(OccurrenceView* self) {
  View temporary {};
  temporary.state = self->state;
  return checked(&temporary) != nullptr;
}
void deallocOccurrence(OccurrenceView* self) {
  delete self->base.object_;
  Py_XDECREF(self->base.owners_);
  delete self->state;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}
PyObject* occurrenceGetAttr(PyObject* self, PyObject* name) {
  if (!validOccurrence(reinterpret_cast<OccurrenceView*>(self))) return nullptr;
  return PyObject_GenericGetAttr(self, name);
}
PyObject* occurrenceRepr(PyObject* self) {
  if (!validOccurrence(reinterpret_cast<OccurrenceView*>(self))) return nullptr;
  return PyTypeSNLOccurrence.tp_repr(self);
}
Py_hash_t occurrenceHash(PyObject* self) {
  if (!validOccurrence(reinterpret_cast<OccurrenceView*>(self))) return -1;
  return PyTypeSNLOccurrence.tp_hash(self);
}
PyObject* occurrenceCompare(PyObject* self, PyObject* other, int op) {
  if (!validOccurrence(reinterpret_cast<OccurrenceView*>(self)) ||
      !PySNLTimingAnalysis_checkOccurrence(other)) return nullptr;
  return PyTypeSNLOccurrence.tp_richcompare(self, other, op);
}
PyObject* linkedOccurrence(View* self, const SNLOccurrence& occurrence) {
  auto* object = reinterpret_cast<OccurrenceView*>(occurrenceType.tp_alloc(&occurrenceType, 0));
  if (!object) return nullptr;
  try {
    if (PySNLOccurrence_set(&object->base, occurrence) < 0) { Py_DECREF(object); return nullptr; }
    object->state = new std::shared_ptr<SNLTimingAnalysis>(*self->state);
  } catch (...) { // LCOV_EXCL_START C++ allocation failure.
    Py_DECREF(object);
    return PyErr_NoMemory();
  } // LCOV_EXCL_STOP
  return reinterpret_cast<PyObject*>(object);
}
PyObject* occurrence(View* self, size_t id) {
  return linkedOccurrence(self, proxy(**self->state, id)->getOccurrence());
}
// Helpers consume owned references, including on allocation failure.
bool append(PyObject* list, PyObject* value) {
  if (!value) return false;
  int status = PyList_Append(list, value);
  Py_DECREF(value);
  return status == 0;
}
bool put(PyObject* dict, const char* key, PyObject* value) {
  if (!value) return false;
  int status = PyDict_SetItemString(dict, key, value);
  Py_DECREF(value);
  return status == 0;
}
PyObject* clocks(View* self, const TEdge::ClockMask& mask) {
  auto* set = PyFrozenSet_New(nullptr);
  if (!set) return nullptr;
  for (auto i = mask.find_first(); i != TEdge::ClockMask::npos; i = mask.find_next(i)) {
    auto* clock = view(clockType, *self->state, i);
    if (!clock || PySet_Add(set, clock) < 0) {
      Py_XDECREF(clock); Py_DECREF(set); return nullptr; // LCOV_EXCL_LINE CPython allocation failure.
    }
    Py_DECREF(clock);
  }
  return set;
}
const char* reasonName(TSNLGraphConstructor::CoverageReason reason) {
  using R = TSNLGraphConstructor::CoverageReason;
  switch (reason) {
    case R::Memory: return "memory";
    case R::InOut: return "inout";
    case R::Blackbox: return "blackbox";
    case R::UnsupportedLatch: return "unsupported_latch";
    case R::MultipleDrivers: return "multiple_drivers";
    case R::IncompleteModeling: return "incomplete_modeling";
    case R::AmbiguousClockGate: return "ambiguous_clock_gate";
  }
  return "unknown"; // LCOV_EXCL_LINE Defensive fallback; all enum values handled above.
}
const char* treatmentName(TSNLGraphConstructor::Treatment treatment) {
  using T = TSNLGraphConstructor::Treatment;
  switch (treatment) {
    case T::Opaque: return "opaque";
    case T::Approximated: return "approximated";
    case T::Bidirectional: return "bidirectional";
    case T::AllDrivers: return "all_drivers";
  }
  return "unknown"; // LCOV_EXCL_LINE Defensive fallback; all enum values handled above.
}
PyObject* coverage(View* self, PyObject*) {
  VIEW_HEAD
  TRY
  auto* list = PyList_New(0);
  if (!list) return nullptr;
  for (const auto& entry: state->constructor.getCoverageReport()) {
    auto* dict = Py_BuildValue("{s:N,s:s,s:s,s:s,s:s}",
      "occurrence", linkedOccurrence(self, entry.occurrence), "model", entry.modelName.c_str(),
      "reason", reasonName(entry.reason), "treatment", treatmentName(entry.treatment),
      "detail", entry.detail.c_str());
    if (!append(list, dict)) { Py_DECREF(list); return nullptr; }
  }
  return list;
  NLCATCH // LCOV_EXCL_LINE Allocation/library failures while formatting checked analysis state.
}
PyObject* coverageCounts(View* self, PyObject*) {
  VIEW_HEAD
  TRY
  auto* dict = PyDict_New();
  if (!dict) return nullptr;
  using R = TSNLGraphConstructor::CoverageReason;
  const auto& counts = state->constructor.getCoverageCounts();
  for (auto reason: {R::Memory, R::InOut, R::Blackbox, R::UnsupportedLatch,
       R::MultipleDrivers, R::IncompleteModeling, R::AmbiguousClockGate}) {
    auto it = counts.find(reason);
    if (!put(dict, reasonName(reason), PyLong_FromSize_t(it == counts.end() ? 0 : it->second))) {
      Py_DECREF(dict); return nullptr; // LCOV_EXCL_LINE CPython allocation failure.
    }
  }
  return dict;
  NLCATCH // LCOV_EXCL_LINE Allocation/library failures while formatting checked analysis state.
}
PyObject* nlClock(View* self, PyObject*) {
  VIEW_HEAD
  return PyNLClock_Link(const_cast<NLClock*>(state->constructor.getGraph().getClock(self->index)->getNLClock()));
}
Py_hash_t clockHash(View* self) {
  if (!checked(self)) return -1;
  auto hash = static_cast<Py_hash_t>(reinterpret_cast<uintptr_t>(self->state->get()) ^ self->index);
  return hash == -1 ? -2 : hash;
}
PyObject* clockCompare(View* self, PyObject* other, int op) {
  VIEW_HEAD
  if (!PyObject_TypeCheck(other, &clockType) || (op != Py_EQ && op != Py_NE)) Py_RETURN_NOTIMPLEMENTED;
  auto* right = reinterpret_cast<View*>(other);
  if (!checked(right)) return nullptr;
  bool equal = *self->state == *right->state && self->index == right->index;
  return PyBool_FromLong(op == Py_EQ ? equal : !equal);
}


std::shared_ptr<SNLTimingAnalysis> designState(SNLDesign* design) {
  auto* property = dynamic_cast<TimingProperty*>(design->getProperty(TimingProperty::Name));
  if (!property) {
    setError("No timing analysis for this occurrence's root design; call top.analyzeTiming()");
    return {};
  }
  View temporary {};
  temporary.state = &property->state;
  return checked(&temporary) ? property->state : std::shared_ptr<SNLTimingAnalysis>{};
}
PyObject* sources(View* self, PyObject*) {
  VIEW_HEAD
  auto* list = PyList_New(0);
  if (!list) return nullptr;
  for (const auto& seed: state->constructor.getGraph().getClock(self->index)->getSeeds()) {
    if (!append(list, occurrence(self, seed.node))) { Py_DECREF(list); return nullptr; }
  }
  return list;
}
// Read the already propagated masks. No new propagation policy is introduced.
void collectMasks(SNLTimingAnalysis& state) {
  const auto& graph = state.constructor.getGraph();
  auto size = graph.getNodesStorage().size();
  const TEdge::ClockMask empty(graph.getClocksCount());
  state.driving.assign(size, empty);
  state.receiving.assign(size, empty);
  for (const auto& clock: graph.getClocksStorage()) {
    for (const auto& seed: clock.getSeeds()) state.driving[seed.node].set(clock.getIndex());
  }
  for (const auto& edge: graph.getEdgesStorage()) {
    if (!edge.isAlive() || edge.isDisabled()) continue;
    state.driving[edge.getSource()] |= edge.getFromClocks();
    const bool stateArc = edge.getKind() == TEdge::Kind::State ||
      graph.getNode(edge.getTarget())->isFlopDataOutput();
    state.driving[edge.getTarget()] |= stateArc
      ? state.result->clockOrigins[edge.getSource()] : edge.getFromClocks();
    state.receiving[edge.getSource()] |= edge.getToClocks();
  }
  std::vector<bool> endpoint(size, false);
  for (const auto& capture: graph.getCaptureEdgesStorage()) {
    if (!capture.isAlive() || capture.getKind() != TCaptureEdge::Kind::Data) continue;
    state.receiving[capture.getTarget()] |= state.result->clockOrigins[capture.getSource()];
    if (!endpoint[capture.getTarget()]) {
      state.endpoints.push_back(capture.getTarget());
      endpoint[capture.getTarget()] = true;
    }
  }
}
PyObject* unclocked(View* self) {
  auto* list = PyList_New(0);
  if (!list) return nullptr;
  for (auto id: (**self->state).result->unclockedFlopClockPins) {
    if (!append(list, occurrence(self, id))) { Py_DECREF(list); return nullptr; }
  }
  return list;
}
PyObject* associations(View* self, bool generated) {
  auto& state = **self->state;
  auto* list = PyList_New(0);
  if (!list) return nullptr;
  const auto& ids = generated ? state.result->generatedClockStateArcs : state.result->controlStateArcs;
  for (auto id: ids) {
    const auto* edge = state.constructor.getGraph().getEdge(id);
    auto* dict = PyDict_New();
    if (!dict || !put(dict, "clockPin", occurrence(self, edge->getSource())) ||
        !put(dict, "output", occurrence(self, edge->getTarget())) ||
        !put(dict, "clocks", clocks(self, state.result->clockOrigins[edge->getSource()]))) {
      Py_XDECREF(dict); Py_DECREF(list); return nullptr; // LCOV_EXCL_LINE CPython allocation failure.
    }
    if (!append(list, dict)) { Py_DECREF(list); return nullptr; }
  }
  return list;
}
#define METHOD(name, function, flags, doc) {name, reinterpret_cast<PyCFunction>(function), flags, doc}
PyMethodDef clockMethods[] = {
  METHOD("getNLClock", nlClock, METH_NOARGS, "Return the NLClock declaration of this occurrence-specific domain."),
  METHOD("getSources", sources, METH_NOARGS, "Return the clock source occurrences in this domain."),
  {nullptr, nullptr, 0, nullptr}
};
int ready(PyTypeObject& type, const char* name, PyMethodDef* methods, PyGetSetDef* properties = nullptr) {
  type.tp_name = name;
  type.tp_basicsize = sizeof(View);
  type.tp_flags = Py_TPFLAGS_DEFAULT;
  type.tp_dealloc = reinterpret_cast<destructor>(dealloc);
  type.tp_methods = methods;
  type.tp_getset = properties;
  return PyType_Ready(&type);
}
}
bool PySNLTimingAnalysis_checkOccurrence(PyObject* object) {
  if (PyObject_TypeCheck(object, &occurrenceType) &&
      !validOccurrence(reinterpret_cast<OccurrenceView*>(object))) return false;
  return PySNLOccurrence_isAlive(object);
}

PyObject* PySNLDesign_analyzeTiming(PyObject* object, PyObject*) {
  auto* design = PYSNLDesign_O(object);
  if (!design) { setError("Cannot analyze a destroyed design"); return nullptr; }
  if (design != NLUniverse::getTopDesign()) {
    setError("analyzeTiming() requires the active top design"); return nullptr;
  }
  std::shared_ptr<SNLTimingAnalysis> next;
  try {
    next = std::make_shared<SNLTimingAnalysis>();
    // Raw writers have no shared native lock. Retain the GIL throughout.
    naja::DNL::destroy();
    next->constructor.construct();
    next->result = TClockPropagator::run(next->constructor.getGraph());
    collectMasks(*next);
    next->generation = pythonNetlistGeneration();
    auto* property = dynamic_cast<TimingProperty*>(design->getProperty(TimingProperty::Name));
    if (property) property->replace(next);
    else TimingProperty::create(design, next);
    Py_RETURN_NONE;
  } catch (const TClockPropagator::CycleError& error) {
    View temporary {};
    temporary.state = &next;
    auto* exception = PyObject_CallFunction(cycleError, "s", error.what());
    if (!exception) return nullptr;
    auto* occ = occurrence(&temporary, error.getNode());
    auto* description = PyUnicode_FromString(error.getDescription().c_str());
    auto* role = PyUnicode_FromString(roleName(error.getRole()));
    int status = occ ? PyObject_SetAttrString(exception, "occurrence", occ) : -1;
    if (status == 0 && description) status = PyObject_SetAttrString(exception, "description", description);
    if (status == 0 && role) status = PyObject_SetAttrString(exception, "role", role);
    Py_XDECREF(occ); Py_XDECREF(description); Py_XDECREF(role);
    if (status == 0 && !PyErr_Occurred()) PyErr_SetObject(cycleError, exception);
    Py_DECREF(exception);
    return nullptr;
  } catch (const NLException& error) {
    setError(error.getReason()); return nullptr;
  } catch (const std::exception& error) { // LCOV_EXCL_START Allocation/library failures; public validation uses NLException.
    setError(error.what()); return nullptr;
  } // LCOV_EXCL_STOP
}
PyObject* PySNLDesign_getTimingData(PyObject* object, int query) {
  auto* design = PYSNLDesign_O(object);
  if (!design) { setError("Cannot query a destroyed design"); return nullptr; }
  TRY
  auto state = designState(design);
  if (!state) return nullptr;
  View temporary {};
  temporary.state = &state;
  if (query == 0) return coverage(&temporary, nullptr);
  if (query == 1) return coverageCounts(&temporary, nullptr);
  if (query == 2) {
    auto* list = PyList_New(0);
    if (!list) return nullptr;
    for (auto id: state->endpoints) {
      if (!append(list, occurrence(&temporary, id))) { Py_DECREF(list); return nullptr; }
    }
    return list;
  }
  if (query == 3) {
    auto* list = PyList_New(0);
    if (!list) return nullptr;
    for (const auto& clock: state->constructor.getGraph().getClocksStorage()) {
      if (!append(list, view(clockType, state, clock.getIndex()))) { Py_DECREF(list); return nullptr; }
    }
    return list;
  }
  auto* dict = PyDict_New();
  if (!dict) return nullptr;
  if (!put(dict, "unclockedClockPins", unclocked(&temporary)) ||
      !put(dict, "generatedClocks", associations(&temporary, true)) ||
      !put(dict, "clockControls", associations(&temporary, false))) {
    Py_DECREF(dict); return nullptr; // LCOV_EXCL_LINE CPython allocation failure.
  }
  return dict;
  NLCATCH
}
PyObject* PySNLOccurrence_getTimingClocks(PyObject* object, int query) {
  if (!PySNLTimingAnalysis_checkOccurrence(object)) return nullptr;
  auto* wrapper = PYSNLOccurrence(object);
  if (wrapper->hasPath_ && wrapper->generation_ != pythonNetlistGeneration()) {
    setError("Stale hierarchical occurrence: recreate it after netlist edits"); return nullptr;
  }
  auto* occ = PYSNLOccurrence_O(object);
  if (!occ || !occ->isValid()) { setError("Expected a valid single-bit occurrence"); return nullptr; }
  TRY
  auto state = designState(occ->getDesign());
  if (!state) return nullptr;
  TEdge::ClockMask mask(state->constructor.getGraph().getClocksCount());
  const auto& masks = query == 0 ? state->driving : query == 1 ? state->receiving : state->result->clockOrigins;
  bool found = false;
  auto add = [&](const SNLOccurrence& pin) {
    for (auto id: {state->constructor.getNode(pin), state->constructor.getDriverNode(pin)}) {
      if (id != TGraph::kInvalidNode) { mask |= masks[id]; found = true; }
    }
  };
  if (auto* net = dynamic_cast<SNLBitNet*>(occ->getObject())) {
    for (auto* component: net->getComponents()) add(SNLOccurrence(occ->getPath(), component));
    // An isolated net has no known domains, but is still a valid net query.
    found = true;
  } else if (occ->getNetComponent()) {
    add(*occ);
  } else {
    setError("Timing queries require a single-bit terminal or net occurrence"); return nullptr;
  }
  if (!found) { setError("Occurrence is absent from the analyzed top"); return nullptr; }
  View temporary {};
  temporary.state = &state;
  return clocks(&temporary, mask);
  NLCATCH
}
int PySNLTimingAnalysis_addTypes(PyObject* module) {
  occurrenceType.tp_name = "naja._TimingOccurrence";
  occurrenceType.tp_basicsize = sizeof(OccurrenceView);
  occurrenceType.tp_flags = Py_TPFLAGS_DEFAULT;
  occurrenceType.tp_base = &PyTypeSNLOccurrence;
  occurrenceType.tp_dealloc = reinterpret_cast<destructor>(deallocOccurrence);
  occurrenceType.tp_getattro = occurrenceGetAttr;
  occurrenceType.tp_repr = occurrenceRepr;
  occurrenceType.tp_str = occurrenceRepr;
  occurrenceType.tp_hash = occurrenceHash;
  occurrenceType.tp_richcompare = occurrenceCompare;
  if (PyType_Ready(&occurrenceType) < 0) return -1;
  occurrenceType.tp_new = nullptr;
  clockType.tp_hash = reinterpret_cast<hashfunc>(clockHash);
  clockType.tp_richcompare = reinterpret_cast<richcmpfunc>(clockCompare);
  if (ready(clockType, "naja.SNLClockDomain", clockMethods) < 0) return -1;
  if (PyModule_AddType(module, &clockType) < 0) return -1;
  cycleError = PyErr_NewException("naja.SNLTimingCycleError", PyExc_RuntimeError, nullptr);
  if (!cycleError) return -1;
  // Python 3.9 has no PyModule_AddObjectRef. Keep our reference while giving
  // the module its own; PyModule_AddObject steals it only on success.
  Py_INCREF(cycleError);
  int status = PyModule_AddObject(module, "SNLTimingCycleError", cycleError);
  if (status < 0) {
    Py_DECREF(cycleError);
  }
  return status;
}
}
