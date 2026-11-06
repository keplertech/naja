// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "PySNLTimingAnalysis.h"

#include <memory>
#include <algorithm>
#include <map>
#include <set>
#include <deque>
#include <tuple>
#include <array>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "PyInterface.h"
#include "PyNLClock.h"
#include "PyNLReset.h"
#include "TConstantPropagator.h"
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
#include "NLDB0.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLBitTerm.h"
#include "SNLDesignModeling.h"
#include "NLClock.h"

namespace PYNAJA {
namespace {
using namespace naja::TG;
using namespace naja::NL;
constexpr size_t invalid = TGraph::kInvalidNode;
struct ConeArc {
  size_t node;
  size_t cell {invalid};
  bool logic {false};
};
struct ConeNode {
  std::vector<ConeArc> incoming, outgoing;
  std::vector<size_t> clocks, outputs;
  const char* launch {nullptr};
  const char* stop {nullptr};
};
struct ConeResult {
  size_t pin;
  std::set<size_t> cells;
  bool direct {true};
  size_t startpoint {invalid};
};
struct ConeSummary {
  size_t boundary {invalid};
  std::vector<ConeArc> children;
};
struct ConeSummaries {
  // Each physical node is summarized once in this direction/mode. Index zero
  // is the empty cone; connectivity-only nodes alias their child's summary.
  std::unordered_map<size_t, size_t> nodes;
  std::vector<ConeSummary> storage {ConeSummary{}};
};

// Private C++ context owned by a property on the analyzed design. The
// constructor owns its TGraph through unique_ptr; Python never sees that graph.
struct SNLTimingAnalysis {
  TSNLGraphConstructor constructor;
  uint64_t generation {pythonNetlistGeneration()};
  bool valid {true};
  std::optional<TClockPropagator::Result> result;
  std::vector<TConstantPropagator::Value> values;
  std::vector<TEdge::ClockMask> resets, resetSeeds, asyncResets, syncResets;
  std::vector<std::vector<size_t>> rawInputs, dataInputs, asyncInputs;
  struct Synchronizer { size_t output, domain, stage; };
  std::vector<Synchronizer> synchronizers;
  std::vector<TEdge::ClockMask> driving;
  std::vector<TEdge::ClockMask> receiving;
  std::vector<TEdge::ClockMask> seeds;
  std::vector<size_t> endpoints;
  std::vector<size_t> recovery;
  // Shared, occurrence-aware data adjacency and boundary metadata. No expanded
  // transitive cell sets are retained per node (quadratic on a long chain).
  std::vector<ConeNode> cone;
  std::vector<std::string> names;
  std::vector<SNLOccurrence> cells;
  std::vector<SNLOccurrence> constants;
  std::map<SNLOccurrence, size_t> constantNodes;
  using ConeKey = std::tuple<size_t, bool, bool>;
  std::map<ConeKey, std::vector<ConeResult>> coneCache;
  std::deque<ConeKey> coneCacheOrder;
  size_t coneCacheWeight {0};
  std::array<ConeSummaries, 4> coneSummaries;
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
PyTypeObject resetType = {PyVarObject_HEAD_INIT(nullptr, 0)};
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
  auto& state = **self->state;
  auto count = state.constructor.getGraph().getNodesStorage().size();
  return linkedOccurrence(self, id < count ? proxy(state, id)->getOccurrence() : state.constants.at(id - count));
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
PyObject* domains(View* self, const TEdge::ClockMask& mask, PyTypeObject& type) {
  auto* set = PyFrozenSet_New(nullptr);
  if (!set) return nullptr;
  for (auto i = mask.find_first(); i != TEdge::ClockMask::npos; i = mask.find_next(i)) {
    auto* clock = view(type, *self->state, i);
    if (!clock || PySet_Add(set, clock) < 0) {
      Py_XDECREF(clock); Py_DECREF(set); return nullptr; // LCOV_EXCL_LINE CPython allocation failure.
    }
    Py_DECREF(clock);
  }
  return set;
}
PyObject* clocks(View* self, const TEdge::ClockMask& mask) { return domains(self, mask, clockType); }
PyObject* resets(View* self, const TEdge::ClockMask& mask) { return domains(self, mask, resetType); }
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
  if (Py_TYPE(reinterpret_cast<PyObject*>(self)) != Py_TYPE(other) || (op != Py_EQ && op != Py_NE)) {
    Py_RETURN_NOTIMPLEMENTED;
  }
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
PyObject* nlReset(View* self, PyObject*) {
  VIEW_HEAD
  return PyNLReset_Link(const_cast<NLReset*>(state->constructor.getResetDomains().at(self->index).declaration));
}
PyObject* resetSources(View* self, PyObject*) {
  VIEW_HEAD
  auto* list = PyList_New(0);
  if (!list) {
    return nullptr;
  }
  for (auto id : state->constructor.getResetDomains().at(self->index).sources) {
    if (!append(list, occurrence(self, id))) { Py_DECREF(list); return nullptr; }
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
  state.seeds.assign(size, empty);
  for (const auto& clock: graph.getClocksStorage()) {
    for (const auto& seed: clock.getSeeds()) {
      state.driving[seed.node].set(clock.getIndex());
      state.seeds[seed.node].set(clock.getIndex());
    }
  }
  for (const auto& edge: graph.getEdgesStorage()) {
    if (!edge.isAlive() || edge.isDisabled() || edge.isDataDisabled()) {
      continue;
    }
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
// Build once per analysis. Both directions use exactly the same physical data
// arcs, so their cell unions agree, including at hierarchy and split inouts.
void collectCone(SNLTimingAnalysis& state) {
  const auto& graph = state.constructor.getGraph();
  const auto size = graph.getNodesStorage().size();
  const bool declaredResets = !state.constructor.getResetDomains().empty();
  state.cone.resize(size);
  if (declaredResets) {
    state.rawInputs.resize(size);
    state.dataInputs.resize(size);
    state.asyncInputs.resize(size);
  }
  std::unordered_map<const SNLDesign*, bool> transparentModels;
  state.names.reserve(size);
  for (size_t id = 0; id < size; ++id) state.names.push_back(proxy(state, id)->getOccurrence().getString());
  std::map<SNLOccurrence, size_t> cellIds;
  auto& constantIds = state.constantNodes;
  std::set<SNLOccurrence> opaque;
  for (const auto& entry: state.constructor.getCoverageReport())
    if (entry.treatment == TSNLGraphConstructor::Treatment::Opaque) opaque.insert(entry.occurrence);
  auto connect = [&](size_t from, size_t to, size_t cell = invalid, bool logic = false) {
    state.cone[from].outgoing.push_back({to, cell, logic});
    state.cone[to].incoming.push_back({from, cell, logic});
  };
  for (const auto& edge: graph.getEdgesStorage()) {
    if (!edge.isAlive() || edge.isDisabled()) continue;
    if (edge.getKind() == TEdge::Kind::State) {
      state.cone[edge.getSource()].outputs.push_back(edge.getTarget());
      state.cone[edge.getTarget()].clocks.push_back(edge.getSource());
      state.cone[edge.getTarget()].launch = "register";
      continue;
    }
    if (declaredResets) {
      state.rawInputs[edge.getTarget()].push_back(edge.getSource());
    }
    if (edge.isDataDisabled()) {
      continue;
    }
    auto* arc = static_cast<const TSNLEdgeProxy*>(edge.getProxy());
    size_t cell = invalid;
    bool logic = false;
    if (arc->getKind() == TSNLEdgeProxy::Kind::Combinational || arc->getKind() == TSNLEdgeProxy::Kind::ClockPass) {
      auto* instance = arc->getOccurrence().getInstance();
      auto* model = instance->getModel();
      if (!NLDB0::isAssign(model)) {
        auto [it, added] = cellIds.emplace(arc->getOccurrence(), state.cells.size());
        if (added) state.cells.push_back(arc->getOccurrence());
        cell = it->second;
        auto [classification, inserted] = transparentModels.try_emplace(model, false);
        if (inserted) {
          classification->second = NLDB0::isDB0Primitive(model) &&
                                   (SNLDesignModeling::isBuf(model) || SNLDesignModeling::isInv(model));
        }
        logic = !classification->second && !edge.isDataTransparent();
      }
    }
    connect(edge.getSource(), edge.getTarget(), cell, logic);
  }
  for (const auto& capture: graph.getCaptureEdgesStorage()) {
    if (!capture.isAlive()) continue;
    auto& node = state.cone[capture.getTarget()];
    node.stop = capture.getKind() == TCaptureEdge::Kind::Data ? "register" : "clock_gating";
    node.clocks.push_back(capture.getSource());
  }
  // Build input-to-state-output associations from the existing explicit
  // sequential expressions. In DB0 each D bit names just its own state, while
  // enables and resets can name many states. No D -> Q traversal arc is added.
  std::map<SNLOccurrence, std::vector<size_t>> leaves;
  for (size_t id = 0; id < size; ++id) {
    const auto& occ = proxy(state, id)->getOccurrence();
    auto* pin = occ.getInstTerm();
    if (pin && pin->getInstance()->getModel()->isLeaf() && proxy(state, id)->getRole() != TNodeProxy::Role::Drive)
      leaves[SNLOccurrence(occ.getPath(), pin->getInstance())].push_back(id);
  }
  for (const auto& [occ, pins]: leaves) {
    auto* instance = occ.getInstance();
    if (SNLDesignModeling::hasSequentialModel(instance->getModel())) {
      const auto& seq = SNLDesignModeling::getSequentialModel(instance);
      std::map<SNLBitTerm*, size_t, SNLBitTerm::InDesignLess> ids;
      for (auto id: pins) ids[proxy(state, id)->getOccurrence().getInstTerm()->getBitTerm()] = id;
      auto terms = [](const SNLDesignModeling::BooleanExpression& expression) {
        std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> result;
        std::vector<size_t> pending {expression.root};
        std::set<size_t> seen;
        while (!pending.empty()) {
          auto id = pending.back(); pending.pop_back();
          if (id >= expression.nodes.size() || !seen.insert(id).second) continue;
          const auto& node = expression.nodes[id];
          if (node.operation == SNLDesignModeling::BooleanExpression::Operator::Term) result.insert(node.term);
          pending.insert(pending.end(), node.operands.begin(), node.operands.end());
        }
        return result;
      };
      std::vector<std::set<SNLBitTerm*, SNLBitTerm::InDesignLess>> inputs(seq.states.size());
      for (size_t i = 0; i < seq.states.size(); ++i) {
        inputs[i] = terms(seq.states[i].nextState);
        for (const auto* expression: {&seq.states[i].clear, &seq.states[i].preset})
          if (*expression) {
            auto more = terms(**expression);
            inputs[i].insert(more.begin(), more.end());
          }
      }
      for (const auto& output: seq.outputs) {
        auto out = state.constructor.getDriverNode(SNLOccurrence(occ.getPath(), instance->getInstTerm(output.term)));
        if (out == invalid || !state.cone[out].launch) continue; // opaque model
        for (const auto& node: output.function.nodes) {
          if (node.operation != SNLDesignModeling::BooleanExpression::Operator::State || node.state >= inputs.size()) continue;
          if (declaredResets) {
            for (auto* input: terms(seq.states[node.state].nextState)) {
              auto found = ids.find(input);
              if (found != ids.end()) {
                state.dataInputs[out].push_back(found->second);
              }
            }
            for (const auto* expression : {&seq.states[node.state].clear, &seq.states[node.state].preset}) {
              if (*expression) {
                for (auto* input : terms(**expression)) {
                  auto found = ids.find(input);
                  if (found != ids.end()) {
                    state.asyncInputs[out].push_back(found->second);
                  }
                }
              }
            }
          }
          for (auto* input: inputs[node.state]) {
            auto found = ids.find(input);
            if (found != ids.end()) state.cone[found->second].outputs.push_back(out);
          }
        }
      }
    } else if (SNLDesignModeling::hasMemoryInterface(instance->getModel())) {
      for (auto id: pins) {
        auto& info = state.cone[id];
        if (!info.stop || std::string(info.stop) != "register") continue;
        for (auto clock: info.clocks) {
          const auto& outputs = state.cone[clock].outputs;
          info.outputs.insert(info.outputs.end(), outputs.begin(), outputs.end());
        }
      }
    }
  }
  for (size_t id = 0; id < size; ++id) {
    const auto* node = graph.getNode(id);
    const auto& occ = proxy(state, id)->getOccurrence();
    const auto role = proxy(state, id)->getRole();
    auto* pin = occ.getInstTerm();
    auto& info = state.cone[id];
    if (!pin) {
      if (role != TNodeProxy::Role::Load && occ.getNetComponent()->getDirection() != SNLTerm::Direction::Output)
        info.launch = "input";
      if (role != TNodeProxy::Role::Drive && occ.getNetComponent()->getDirection() != SNLTerm::Direction::Input)
        info.stop = "output";
    } else if (pin->getInstance()->getModel()->isLeaf()) {
      if (node->getPinRole() == TNode::PinRole::AsyncReset || node->getPinRole() == TNode::PinRole::AsyncSet) {
        info.stop = "recovery";
        state.recovery.push_back(id);
        // Async controls do not have capture arcs. Find their owning clock(s)
        // from the already modeled output state associations.
        for (auto* output: pin->getInstance()->getInstTerms()) {
          auto out = state.constructor.getDriverNode(SNLOccurrence(occ.getPath(), output));
          if (out != invalid && state.cone[out].launch && std::string(state.cone[out].launch) == "register") {
            const auto& clocks = state.cone[out].clocks;
            info.clocks.insert(info.clocks.end(), clocks.begin(), clocks.end());
            if (SNLDesignModeling::hasMemoryInterface(pin->getInstance()->getModel())) info.outputs.push_back(out);
          }
        }
      } else if (node->getPinRole() == TNode::PinRole::Clock || node->getPinRole() == TNode::PinRole::Gate) {
        info.stop = "clock";
      }
      if (node->getCellKind() == TNode::CellKind::Unknown &&
          (!SNLDesignModeling::hasMemoryInterface(pin->getInstance()->getModel()) ||
           opaque.count(SNLOccurrence(occ.getPath(), pin->getInstance())))) {
        if (role != TNodeProxy::Role::Load && pin->getDirection() != SNLTerm::Direction::Input)
          info.launch = "blackbox";
        if (role != TNodeProxy::Role::Drive && pin->getDirection() != SNLTerm::Direction::Output)
          info.stop = "blackbox";
      }
      if (node->isCombinationalDriver() && (declaredResets ? state.rawInputs[id].empty() : info.incoming.empty())) {
        info.launch = "constant";
      }
    }
    // Constants are represented by typed bit nets rather than graph terminals.
    // Add private query vertices; the timing graph and propagation stay intact.
    auto* net = occ.getComponentBitNet();
    if (net && net->isConstant() && role != TNodeProxy::Role::Drive) {
      SNLOccurrence constant(occ.getPath(), net);
      auto [it, added] = constantIds.emplace(constant, state.cone.size());
      if (added) {
        state.constants.push_back(constant);
        state.names.push_back(constant.getString());
        state.cone.emplace_back();
        state.cone.back().launch = "constant";
      }
      connect(it->second, id);
    }
  }
  for (auto& info: state.cone) {
    std::sort(info.outputs.begin(), info.outputs.end(), [&](auto a, auto b) {
      return state.names[a] < state.names[b];
    });
    info.outputs.erase(std::unique(info.outputs.begin(), info.outputs.end()), info.outputs.end());
    std::sort(info.clocks.begin(), info.clocks.end());
    info.clocks.erase(std::unique(info.clocks.begin(), info.clocks.end()), info.clocks.end());
  }
  for (size_t id = 0; id < size; ++id) {
    if (state.seeds[id].any() && (!state.cone[id].launch || std::string(state.cone[id].launch) != "register"))
      state.cone[id].launch = "clock";
  }
}

// Propagate the physical reset view independently of data pruning. A fixed
// point handles reset-derived Qs without adding any data-through state arc.
void collectResets(SNLTimingAnalysis& state) {
  const auto& graph = state.constructor.getGraph();
  const auto size = graph.getNodesStorage().size();
  const TEdge::ClockMask empty(state.constructor.getResetDomains().size());
  state.resets.assign(size, empty);
  if (state.constructor.getResetDomains().empty()) {
    return;
  }
  state.resetSeeds.assign(size, empty);
  state.asyncResets.assign(size, empty);
  state.syncResets.assign(size, empty);
  std::deque<size_t> pending;
  std::vector<bool> queued(size, false);
  auto schedule = [&](size_t id) { if (!queued[id]) { pending.push_back(id); queued[id] = true; } };
  auto propagate = [&]() {
    while (!pending.empty()) {
      auto id = pending.front(); pending.pop_front(); queued[id] = false;
      for (auto edgeId: graph.getNode(id)->getOutEdges()) {
        const auto& edge = *graph.getEdge(edgeId);
        if (!edge.isAlive() || edge.isDisabled() || edge.getKind() == TEdge::Kind::State) {
          continue;
        }
        auto target = edge.getTarget();
        if (state.resets[id].is_subset_of(state.resets[target])) {
          continue;
        }
        state.resets[target] |= state.resets[id]; schedule(target);
      }
    }
  };
  const auto& declarations = state.constructor.getResetDomains();
  for (size_t domain = 0; domain < declarations.size(); ++domain) {
    for (auto id: declarations[domain].sources) {
      state.resets[id].set(domain); state.resetSeeds[id].set(domain); schedule(id);
      if (!state.cone[id].launch || std::string(state.cone[id].launch) != "register") {
        state.cone[id].launch = "reset";
      }
    }
  }
  propagate();
  // Physical D-cone boundaries, cached once per state output. Literal constants
  // are checked on the raw graph, not on the reset-inactive interpretation.
  std::map<size_t, std::vector<size_t>> startpoints;
  for (size_t out = 0; out < size; ++out) {
    if (state.asyncInputs[out].empty() || state.dataInputs[out].empty()) {
      continue;
    }
    std::vector<size_t> work = state.dataInputs[out];
    std::set<size_t> seen, starts;
    while (!work.empty()) {
      auto id = work.back(); work.pop_back();
      if (!seen.insert(id).second) {
        continue;
      }
      const auto& occ = proxy(state, id)->getOccurrence();
      auto* net = occ.getComponentBitNet();
      if (net && net->isConstant()) {
        continue;
      }
      if (state.cone[id].launch) {
        if (std::string(state.cone[id].launch) != "constant") {
          starts.insert(id);
        }
      } else if (state.rawInputs[id].empty()) {
        starts.insert(id);
      } else {
        work.insert(work.end(), state.rawInputs[id].begin(), state.rawInputs[id].end());
      }
    }
    startpoints.emplace(out, std::vector<size_t>(starts.begin(), starts.end()));
  }
  std::vector<std::map<size_t, size_t>> stages(size);
  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto& [out, starts]: startpoints) {
      auto candidates = empty;
      for (auto pin : state.asyncInputs[out]) {
        candidates |= state.resets[pin];
      }
      for (auto domain = candidates.find_first(); domain != TEdge::ClockMask::npos; domain = candidates.find_next(domain)) {
        if (stages[out].count(domain)) {
          continue;
        }
        bool derived = true;
        size_t stage = 1;
        for (auto start: starts) {
          auto found = stages[start].find(domain);
          if (found == stages[start].end()) { derived = false; break; }
          stage = std::max(stage, found->second + 1);
        }
        if (!derived) {
          continue;
        }
        stages[out][domain] = stage;
        state.synchronizers.push_back({out, domain, stage});
        state.resets[out].set(domain); schedule(out); changed = true;
      }
    }
    propagate();
  }
  // Reset provenance of disabled inputs propagates over the physical D cone
  // up to state outputs. Shared fanout is summarized once, not walked per row.
  for (const auto& edge: graph.getEdgesStorage()) {
    if (!edge.isAlive() || edge.isDisabled() || !edge.isDataDisabled()) {
      continue;
    }
    state.syncResets[edge.getTarget()] |= state.resets[edge.getSource()];
  }
  for (size_t id = 0; id < size; ++id) {
    if (state.syncResets[id].any()) {
      schedule(id);
    }
  }
  while (!pending.empty()) {
    auto id = pending.front(); pending.pop_front(); queued[id] = false;
    for (auto edgeId: graph.getNode(id)->getOutEdges()) {
      const auto& edge = *graph.getEdge(edgeId);
      if (!edge.isAlive() || edge.isDisabled() || edge.getKind() == TEdge::Kind::State) {
        continue;
      }
      auto to = edge.getTarget();
      if (state.syncResets[id].is_subset_of(state.syncResets[to])) {
        continue;
      }
      state.syncResets[to] |= state.syncResets[id]; schedule(to);
    }
  }
  for (size_t out = 0; out < size; ++out) {
    for (auto input : state.asyncInputs[out]) {
      state.asyncResets[out] |= state.resets[input];
    }
    for (auto input: state.dataInputs[out]) {
      state.syncResets[out] |= state.syncResets[input];
      auto role = graph.getNode(input)->getPinRole();
      if (role == TNode::PinRole::SyncReset || role == TNode::PinRole::SyncSet) {
        state.syncResets[out] |= state.resets[input];
      }
    }
  }
  for (size_t id = 0; id < size; ++id) {
    auto role = graph.getNode(id)->getPinRole();
    if (role == TNode::PinRole::AsyncReset || role == TNode::PinRole::AsyncSet) {
      for (auto out : state.cone[id].outputs) {
        state.asyncResets[out] |= state.resets[id];
      }
    }
    if (role == TNode::PinRole::SyncReset || role == TNode::PinRole::SyncSet) {
      for (auto out : state.cone[id].outputs) {
        state.syncResets[out] |= state.resets[id];
      }
    }
  }
  std::sort(state.synchronizers.begin(), state.synchronizers.end(), [](const auto& a, const auto& b) {
    return std::tie(a.output, a.domain) < std::tie(b.output, b.domain);
  });
}

// Provenance is a union DAG, not an expanded set at each intermediate pin.
// A reconvergent cone visits each vertex once; withCells=false allocates no DAG.
struct CellUnion {
  size_t cell {invalid};
  size_t left {0}, right {0};
};
// Summaries retain topology/provenance, not expanded transitive cell sets.
// The total storage across all nodes is O(V+E), even on a long chain. They
// survive result-cache eviction and are reused by overlapping cone queries.
size_t summarizeCone(SNLTimingAnalysis& state, ConeSummaries& cache, size_t root,
    bool forward, bool withCells) {
  auto found = cache.nodes.find(root);
  if (found != cache.nodes.end()) return found->second;
  struct Visit { size_t id, next; };
  std::vector<Visit> stack {{root, 0}};
  std::unordered_set<size_t> active {root};
  auto arcs = [&](size_t id) -> const std::vector<ConeArc>& {
    return forward ? state.cone[id].outgoing : state.cone[id].incoming;
  };
  // A write-side input may also be an asynchronous read address: report it
  // as a capture and continue along its explicit combinational read arcs.
  auto stop = [&](size_t id) { return forward && state.cone[id].stop &&
    std::string(state.cone[id].stop) != "register"; };
  while (!stack.empty()) {
    auto& frame = stack.back();
    const auto& edges = arcs(frame.id);
    if (!stop(frame.id) && frame.next < edges.size()) {
      auto next = edges[frame.next++].node;
      if (cache.nodes.find(next) != cache.nodes.end()) continue;
      if (!active.insert(next).second) throw NLException("Cycle in timing data cone");
      stack.push_back({next, 0});
      continue;
    }
    ConeSummary summary;
    if (forward ? state.cone[frame.id].stop : state.cone[frame.id].launch)
      summary.boundary = frame.id;
    if (!stop(frame.id)) {
      for (const auto& arc: edges) {
        auto child = cache.nodes.at(arc.node);
        if (child) summary.children.push_back({child, withCells ? arc.cell : invalid, arc.logic});
      }
    }
    size_t index = 0;
    if (summary.boundary == invalid && summary.children.size() == 1 &&
        summary.children.front().cell == invalid && !summary.children.front().logic) {
      index = summary.children.front().node;
    } else if (summary.boundary != invalid || !summary.children.empty()) {
      index = cache.storage.size();
      cache.storage.push_back(std::move(summary));
    }
    cache.nodes.emplace(frame.id, index);
    active.erase(frame.id);
    stack.pop_back();
  }
  return cache.nodes.at(root);
}
std::vector<ConeResult> walkCone(SNLTimingAnalysis& state, const std::vector<size_t>& roots,
    bool forward, bool withCells) {
  auto& summaries = state.coneSummaries[2 * forward + withCells];
  std::vector<size_t> summaryRoots;
  for (auto root: roots) summaryRoots.push_back(summarizeCone(state, summaries, root, forward, withCells));
  std::sort(summaryRoots.begin(), summaryRoots.end());
  summaryRoots.erase(std::unique(summaryRoots.begin(), summaryRoots.end()), summaryRoots.end());
  const auto key = SNLTimingAnalysis::ConeKey(summaryRoots.empty() ? 0 : summaryRoots.front(), forward, withCells);
  if (summaryRoots.size() == 1) {
    auto found = state.coneCache.find(key);
    if (found != state.coneCache.end()) return found->second;
  }
  // Evaluate shared summaries once per query, using a topological order. A
  // summary can contain a boundary and dependencies (asynchronous memory read).
  struct Visit { size_t id, next; };
  std::unordered_map<size_t, bool> visited;
  // A previous query can be an intermediate suffix of this query. Reuse its
  // materialized boundary/cell results instead of re-evaluating that suffix.
  // These references remain valid until result-cache eviction at the end.
  std::unordered_map<size_t, const std::vector<ConeResult>*> cached;
  auto visit = [&](size_t id) {
    visited[id] = true;
    auto found = state.coneCache.find(SNLTimingAnalysis::ConeKey(id, forward, withCells));
    if (found != state.coneCache.end()) cached.emplace(id, &found->second);
  };
  std::vector<size_t> order;
  std::vector<Visit> stack;
  auto arcs = [&](size_t id) -> const std::vector<ConeArc>& { return summaries.storage[id].children; };
  for (auto root: summaryRoots) {
    if (visited[root]) continue;
    visit(root);
    stack.push_back({root, 0});
    while (!stack.empty()) {
      auto& frame = stack.back();
      const auto& edges = arcs(frame.id);
      if (!cached.count(frame.id) && frame.next < edges.size()) {
        auto next = edges[frame.next++].node;
        if (!visited[next]) { visit(next); stack.push_back({next, 0}); }
      } else {
        order.push_back(frame.id);
        stack.pop_back();
      }
    }
  }
  struct Value { size_t cells {0}; bool logic {false}; };
  std::unordered_map<size_t, Value> values;
  std::vector<CellUnion> unions;
  if (withCells) unions.emplace_back();
  auto merge = [&](size_t left, size_t right) {
    if (!left || left == right) return right;
    if (!right) return left;
    unions.push_back({invalid, left, right});
    return unions.size() - 1;
  };
  std::map<size_t, size_t> cellLeaves;
  for (auto it = order.rbegin(); it != order.rend(); ++it) {
    auto id = *it;
    auto value = values[id];
    if (cached.count(id)) continue;
    for (const auto& arc: arcs(id)) {
      auto& next = values[arc.node];
      next.logic = next.logic || value.logic || arc.logic;
      if (withCells) {
        auto expression = value.cells;
        if (arc.cell != invalid) {
          auto [leaf, added] = cellLeaves.emplace(arc.cell, unions.size());
          if (added) unions.push_back({arc.cell, 0, 0});
          expression = merge(expression, leaf->second);
        }
        next.cells = merge(next.cells, expression);
      }
    }
  }
  auto expandCells = [&](size_t expression) {
    std::set<size_t> cells;
    if (withCells) {
      std::vector<size_t> pending {expression};
      std::unordered_set<size_t> seen;
      while (!pending.empty()) {
        auto index = pending.back(); pending.pop_back();
        if (!index || !seen.insert(index).second) continue;
        const auto& u = unions[index];
        if (u.cell != invalid) cells.insert(u.cell);
        else { pending.push_back(u.left); pending.push_back(u.right); }
      }
    }
    return cells;
  };
  std::map<size_t, ConeResult> combined;
  auto emit = [&](size_t pin, const std::set<size_t>& prefix, bool direct, const ConeResult* suffix = nullptr) {
    auto [it, added] = combined.try_emplace(pin, ConeResult{pin, {}, true});
    auto& row = it->second;
    row.direct = row.direct && direct && (!suffix || suffix->direct);
    if (withCells) {
      row.cells.insert(prefix.begin(), prefix.end());
      if (suffix) row.cells.insert(suffix->cells.begin(), suffix->cells.end());
    }
  };
  for (auto id: order) {
    const auto& value = values[id];
    auto hit = cached.find(id);
    if (hit != cached.end()) {
      auto prefix = expandCells(value.cells);
      for (const auto& suffix: *hit->second) emit(suffix.pin, prefix, !value.logic, &suffix);
    } else {
      auto boundary = summaries.storage[id].boundary;
      if (boundary != invalid) emit(boundary, expandCells(value.cells), !value.logic);
    }
  }
  std::vector<ConeResult> result;
  result.reserve(combined.size());
  for (auto& [pin, row]: combined) result.push_back(std::move(row));
  // Bound memoized results by returned records plus cell memberships, rather
  // than by query count: one enormous cone cannot fill a persistent cache.
  // Adjacency/boundary metadata above remains O(V+E) and serves both directions.
  auto weight = [](const auto& rows) {
    size_t sum = 1;
    for (const auto& row: rows) sum += 1 + row.cells.size();
    return sum;
  };
  auto cost = weight(result);
  constexpr size_t budget = 65536;
  if (summaryRoots.size() == 1 && cost <= budget) {
    while (state.coneCacheWeight + cost > budget) {
      auto oldest = state.coneCacheOrder.front(); state.coneCacheOrder.pop_front();
      state.coneCacheWeight -= weight(state.coneCache.at(oldest));
      state.coneCache.erase(oldest);
    }
    state.coneCache.emplace(key, result);
    state.coneCacheOrder.push_back(key);
    state.coneCacheWeight += cost;
  }
  return result;
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
PyObject* resetSynchronizers(View* self) {
  auto& state = **self->state;
  auto* list = PyList_New(0);
  if (!list) {
    return nullptr;
  }
  for (size_t i = 0; i < state.synchronizers.size();) {
    auto entry = state.synchronizers[i];
    TEdge::ClockMask mask(state.constructor.getResetDomains().size());
    while (i < state.synchronizers.size() && state.synchronizers[i].output == entry.output && state.synchronizers[i].stage == entry.stage) {
      mask.set(state.synchronizers[i++].domain);
    }
    auto* row = PyDict_New();
    if (!row || !put(row, "output", occurrence(self, entry.output)) || !put(row, "resets", resets(self, mask)) ||
        !put(row, "stage", PyLong_FromSize_t(entry.stage))) { Py_XDECREF(row); Py_DECREF(list); return nullptr; }
    if (!append(list, row)) { Py_DECREF(list); return nullptr; }
  }
  return list;
}
PyObject* resetControls(View* self) {
  auto& state = **self->state;
  const auto& graph = state.constructor.getGraph();
  struct Control { TEdge::ClockMask domains; std::set<size_t> other; };
  std::map<SNLOccurrence, Control> controls;
  for (const auto& edge: graph.getEdgesStorage()) {
    if (!edge.isAlive() || edge.isDisabled() || edge.getKind() == TEdge::Kind::State) {
      continue;
    }
    auto* arc = static_cast<const TSNLEdgeProxy*>(edge.getProxy());
    if (arc->getKind() == TSNLEdgeProxy::Kind::Connectivity) {
      continue;
    }
    auto [it, added] = controls.try_emplace(arc->getOccurrence(), Control{TEdge::ClockMask(state.constructor.getResetDomains().size()), {}});
    auto id = edge.getSource();
    if (state.resets[id].any()) {
      it->second.domains |= state.resets[id];
    } else {
      auto* net = proxy(state, id)->getOccurrence().getComponentBitNet();
      if (!net || !net->isConstant()) {
        it->second.other.insert(id);
      }
    }
  }
  auto* list = PyList_New(0);
  if (!list) {
    return nullptr;
  }
  for (const auto& [cell, control]: controls) {
    if (control.domains.none() || control.other.empty()) {
      continue;
    }
    auto* pins = PyTuple_New(control.other.size());
    if (!pins) { Py_DECREF(list); return nullptr; }
    size_t i = 0;
    for (auto id: control.other) {
      auto* pin = occurrence(self, id);
      if (!pin) { Py_DECREF(pins); Py_DECREF(list); return nullptr; }
      PyTuple_SET_ITEM(pins, i++, pin);
    }
    auto* row = PyDict_New();
    if (!row || !put(row, "occurrence", linkedOccurrence(self, cell)) ||
        !put(row, "resets", resets(self, control.domains)) || !put(row, "nonResetInputs", pins)) {
      Py_XDECREF(row); Py_DECREF(list); return nullptr;
    }
    if (!append(list, row)) { Py_DECREF(list); return nullptr; }
  }
  return list;
}
#define METHOD(name, function, flags, doc) {name, reinterpret_cast<PyCFunction>(function), flags, doc}
PyMethodDef resetMethods[] = {
  METHOD("getNLReset", nlReset, METH_NOARGS, "Return the NLReset declaration."),
  METHOD("getSources", resetSources, METH_NOARGS, "Return reset source occurrences."),
  {nullptr, nullptr, 0, nullptr}
};
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
    if (!next->constructor.getResetDomains().empty()) {
      std::vector<TConstantPropagator::Seed> seeds;
      for (const auto& domain : next->constructor.getResetDomains()) {
        for (auto id : domain.sources) {
          seeds.emplace_back(id, domain.declaration->getInactiveValue() ? TConstantPropagator::Value::One : TConstantPropagator::Value::Zero);
        }
      }
      next->values = TConstantPropagator::run(next->constructor, seeds);
    }
    next->result = TClockPropagator::run(next->constructor.getGraph());
    collectMasks(*next);
    collectCone(*next);
    collectResets(*next);
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
  if (query == 5) {
    auto* list = PyList_New(0);
    if (!list) {
      return nullptr;
    }
    for (size_t id = 0; id < state->constructor.getResetDomains().size(); ++id) {
      if (!append(list, view(resetType, state, id))) { Py_DECREF(list); return nullptr; }
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
  if (!put(dict, "noClock", unclocked(&temporary)) ||
      !put(dict, "generatedClocks", associations(&temporary, true)) ||
      !put(dict, "clockControls", associations(&temporary, false))) {
    Py_DECREF(dict); return nullptr; // LCOV_EXCL_LINE CPython allocation failure.
  }
  if (!state->constructor.getResetDomains().empty() &&
      (!put(dict, "resetSynchronizers", resetSynchronizers(&temporary)) ||
       !put(dict, "resetControls", resetControls(&temporary)))) {
    Py_DECREF(dict); return nullptr;
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
  TEdge::ClockMask mask(query == 3 ? state->constructor.getResetDomains().size() : state->constructor.getGraph().getClocksCount());
  const auto& masks = query == 3 ? state->resets : query == 0 ? state->driving : query == 1 ? state->receiving : state->result->clockOrigins;
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
  return query == 3 ? resets(&temporary, mask) : clocks(&temporary, mask);
  NLCATCH
}
PyObject* PySNLDesign_getEndpoints(PyObject* object, PyObject* args, PyObject* kwargs) {
  PyObject* check = Py_None;
  static const char* names[] = {"check", nullptr};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:getEndpoints", const_cast<char**>(names), &check)) return nullptr;
  int selection = 0;
  if (check != Py_None) {
    if (!PyUnicode_Check(check)) { setError("check must be None, 'setup' or 'recovery'"); return nullptr; }
    if (PyUnicode_CompareWithASCIIString(check, "setup") == 0) selection = 1;
    else if (PyUnicode_CompareWithASCIIString(check, "recovery") == 0) selection = 2;
    else { setError("check must be None, 'setup' or 'recovery'"); return nullptr; }
  }
  auto* design = PYSNLDesign_O(object);
  if (!design) { setError("Cannot query a destroyed design"); return nullptr; }
  TRY
  auto state = designState(design);
  if (!state) return nullptr;
  View temporary {}; temporary.state = &state;
  std::vector<size_t> ids;
  if (selection != 2) ids = state->endpoints;
  if (selection != 1) ids.insert(ids.end(), state->recovery.begin(), state->recovery.end());
  std::sort(ids.begin(), ids.end(), [&](auto a, auto b) {
    return state->names[a] < state->names[b];
  });
  auto* list = PyList_New(0);
  if (!list) return nullptr;
  for (auto id: ids) if (!append(list, occurrence(&temporary, id))) { Py_DECREF(list); return nullptr; }
  return list;
  NLCATCH
}

PyObject* PySNLOccurrence_getTimingCone(PyObject* object, PyObject* args, PyObject* kwargs, bool forward) {
  int withCells = 1;
  static const char* names[] = {"withCells", nullptr};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|p", const_cast<char**>(names), &withCells)) return nullptr;
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
  const auto& graph = state->constructor.getGraph();
  std::vector<size_t> roots;
  const bool netQuery = dynamic_cast<SNLBitNet*>(occ->getObject()) != nullptr;
  auto add = [&](const SNLOccurrence& pin) {
    auto id = forward ? state->constructor.getDriverNode(pin) : state->constructor.getNode(pin);
    // Sources use drive roles; destinations use load roles. A fanin query of a
    // split output uses its drive role, while an inout samples the load role.
    if (!forward && pin.getNetComponent()->getDirection() == SNLTerm::Direction::Output)
      id = state->constructor.getDriverNode(pin);
    if (id == invalid) return;
    if (!forward) {
      auto driver = state->constructor.getDriverNode(pin);
      // Clock declarations on inferred opaque outputs seed their drive role;
      // the public launch query includes that role as well as the sampled one.
      if (driver != invalid && driver != id && (state->seeds[driver].any() || (driver < state->resetSeeds.size() && state->resetSeeds[driver].any()))) {
        roots.push_back(driver);
      }
    }
    if (forward && !netQuery && !state->cone[id].outputs.empty() &&
        (graph.getNode(id)->getPinRole() == TNode::PinRole::Clock || graph.getNode(id)->getPinRole() == TNode::PinRole::Gate)) {
      const auto& outputs = state->cone[id].outputs;
      roots.insert(roots.end(), outputs.begin(), outputs.end());
    } else roots.push_back(id);
  };
  if (auto* net = dynamic_cast<SNLBitNet*>(occ->getObject())) {
    for (auto* component: net->getComponents()) add(SNLOccurrence(occ->getPath(), component));
    // An isolated constant still has a launch point.
    auto constant = state->constantNodes.find(*occ);
    if (constant != state->constantNodes.end()) roots.push_back(constant->second);
  } else if (occ->getNetComponent()) add(*occ);
  else { setError("Timing queries require a single-bit terminal or net occurrence"); return nullptr; }
  if (roots.empty() && !dynamic_cast<SNLBitNet*>(occ->getObject())) {
    setError("Occurrence is absent from the analyzed top"); return nullptr;
  }
  auto rows = walkCone(*state, roots, forward, withCells);
  if (!forward) {
    std::vector<ConeResult> expanded;
    for (const auto& row: rows) {
      const auto& info = state->cone[row.pin];
      if (std::string(info.launch) == "register" && !info.clocks.empty()) {
        for (auto clock: info.clocks) {
          auto copy = row;
          copy.startpoint = clock;
          expanded.push_back(std::move(copy));
        }
      } else expanded.push_back(row);
    }
    rows = std::move(expanded);
  }
  std::sort(rows.begin(), rows.end(), [&](const auto& a, const auto& b) {
    const auto& left = state->names[a.pin];
    const auto& right = state->names[b.pin];
    if (left != right) return left < right;
    return a.startpoint != invalid && b.startpoint != invalid &&
      state->names[a.startpoint] < state->names[b.startpoint];
  });
  View temporary {}; temporary.state = &state;
  auto* list = PyList_New(0);
  if (!list) return nullptr;
  for (const auto& row: rows) {
    const auto& info = state->cone[row.pin];
    auto* dict = PyDict_New();
    if (!dict) { Py_DECREF(list); return nullptr; }
    bool ok = put(dict, forward ? "endpoint" : "launchPin", occurrence(&temporary, row.pin)) &&
      put(dict, "kind", PyUnicode_FromString(forward ? info.stop : info.launch)) &&
      put(dict, "direct", PyBool_FromLong(row.direct));
    TEdge::ClockMask mask(graph.getClocksCount());
    if (forward) {
      if (std::string(info.stop) == "register") mask = state->receiving[row.pin];
      else if (std::string(info.stop) == "recovery" || std::string(info.stop) == "clock_gating")
        for (auto clock: info.clocks) mask |= state->result->clockOrigins[clock];
      const bool clockStop = std::string(info.stop) == "clock";
      const bool associated = clockStop || std::string(info.stop) == "register" || std::string(info.stop) == "recovery" || std::string(info.stop) == "clock_gating";
      ok = ok && put(dict, "captureClocks", clocks(&temporary, mask)) &&
        put(dict, "registerClock", clockStop ? occurrence(&temporary, row.pin) : associated && !info.clocks.empty() ? occurrence(&temporary, info.clocks.front()) : (Py_INCREF(Py_None), Py_None));
      auto* outputs = PyTuple_New(associated ? info.outputs.size() : 0);
      if (outputs && associated) {
        for (size_t i = 0; i < info.outputs.size(); ++i) {
          auto* output = occurrence(&temporary, info.outputs[i]);
          if (!output) { Py_DECREF(outputs); outputs = nullptr; break; }
          PyTuple_SET_ITEM(outputs, i, output);
        }
      }
      ok = ok && put(dict, "registerOutputs", outputs);
    } else {
      if (row.pin < state->driving.size()) mask = state->driving[row.pin];
      if (row.startpoint != invalid) mask = state->result->clockOrigins[row.startpoint] | state->seeds[row.pin];
      if (std::string(info.launch) == "clock") mask = state->seeds[row.pin];
      if (std::string(info.launch) == "reset") {
        mask.reset();
      }
      ok = ok && put(dict, "launchClocks", clocks(&temporary, mask)) &&
        put(dict, "startpoint", occurrence(&temporary, row.startpoint != invalid ? row.startpoint : row.pin));
    }
    if (!state->constructor.getResetDomains().empty()) {
      const std::string kind = forward ? info.stop : info.launch;
      if (kind == "reset") {
        ok = ok && put(dict, "resets", resets(&temporary, state->resetSeeds[row.pin]));
      }
      if (kind == "register" || (forward && kind == "recovery")) {
        TEdge::ClockMask async(state->constructor.getResetDomains().size()), sync(async);
        if (!forward) { async = state->asyncResets[row.pin]; sync = state->syncResets[row.pin];
        } else {
          for (auto out : info.outputs) {
            async |= state->asyncResets[out];
            sync |= state->syncResets[out];
          }
        }
        ok = ok && put(dict, "asyncResets", resets(&temporary, async)) && put(dict, "syncResets", resets(&temporary, sync));
      }
    }
    if (withCells) {
      auto* cells = PyFrozenSet_New(nullptr);
      if (cells) for (auto id: row.cells) {
        auto* cell = linkedOccurrence(&temporary, state->cells[id]);
        if (!cell || PySet_Add(cells, cell) < 0) { Py_XDECREF(cell); Py_DECREF(cells); cells = nullptr; break; }
        Py_DECREF(cell);
      }
      ok = ok && put(dict, "cells", cells);
    }
    if (!ok) { Py_DECREF(dict); Py_DECREF(list); return nullptr; }
    if (!append(list, dict)) { Py_DECREF(list); return nullptr; }
  }
  return list;
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
  resetType.tp_hash = reinterpret_cast<hashfunc>(clockHash);
  resetType.tp_richcompare = reinterpret_cast<richcmpfunc>(clockCompare);
  if (ready(resetType, "naja.SNLResetDomain", resetMethods) < 0 || PyModule_AddType(module, &resetType) < 0) {
    return -1;
  }
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
