// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "SNLClockGating.h"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "NLDB0.h"
#include "NLClock.h"
#include "SNLAttributes.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLRTLInfos.h"

namespace naja::NL {
namespace {
using Role = SNLDesignModeling::SNLTermRole;
using Net = SNLBitNet*;

SNLInstTerm* driver(Net net) {
  if (!net || net->isAssignConstant()) return nullptr;
  SNLInstTerm* result = nullptr;
  for (auto* term : net->getBitTerms())
    if (term->getDirection() != SNLTerm::Direction::Output) return nullptr;
  for (auto* term : net->getInstTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Input) continue;
    if (term->getDirection() != SNLTerm::Direction::Output || result) return nullptr;
    result = term;
  }
  return result;
}

bool uniqueDrivers(Net net) {
  if (!net) return false;
  size_t count = net->isAssignConstant() ? 1 : 0;
  for (auto* term : net->getBitTerms()) {
    if (term->getDirection() == SNLTerm::Direction::InOut) return false;
    count += term->getDirection() == SNLTerm::Direction::Input;
  }
  for (auto* term : net->getInstTerms()) {
    if (term->getDirection() == SNLTerm::Direction::InOut) return false;
    count += term->getDirection() == SNLTerm::Direction::Output;
  }
  return count <= 1;
}

// Only real wires, DB0 buffers/inverters and assigns establish phase equality.
struct Origin { Net net; bool inverted {false}; };
Origin origin(Net net) {
  std::unordered_set<Net> visited;
  bool inverted = false;
  while (net && visited.insert(net).second) {
    auto* source = driver(net);
    if (!source) return {net, inverted};
    auto* instance = source->getInstance();
    auto* model = instance->getModel();
    if (NLDB0::isAssign(model)) {
      net = instance->getInstTerm(NLDB0::getAssignInput())->getNet();
    } else if (NLDB0::isGate(model) &&
        (NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Buf ||
         NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Not) &&
        instance->getInstTerms().size() == 2) {
      for (auto* term : instance->getInstTerms())
        if (term->getDirection() == SNLTerm::Direction::Input) net = term->getNet();
      inverted = inverted != (NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Not);
    } else return {net, inverted};
  }
  return {nullptr, false};
}

bool same(Net a, Net b) {
  const auto x = origin(a), y = origin(b);
  return x.net && x.net == y.net && x.inverted == y.inverted;
}

struct Pattern {
  SNLInstance* gate;
  SNLInstance* latch;
  Net clock;
  Net enable;
  Net held;
  Net output;
};

std::vector<Pattern> patterns(SNLDesign* design) {
  std::vector<Pattern> result;
  const auto* scalarLatch = NLDB0::getDLatch();
  bool hasLatch = false;
  for (auto* instance : design->getInstances())
    if (instance->getModel() == scalarLatch) { hasLatch = true; break; }
  if (!hasLatch) return result;
  for (auto* gate : design->getInstances()) {
    auto* model = gate->getModel();
    if (!NLDB0::isGate(model) || NLDB0::GateType(NLDB0::getGateName(model)) != NLDB0::GateType::And ||
        gate->getInstTerms().size() != 3) continue;
    auto* inputs = NLDB0::getGateNTerms(model);
    if (!inputs || inputs->getWidth() != 2) continue;
    auto* out = gate->getInstTerm(NLDB0::getGateSingleTerm(model));
    if (!out || driver(out->getNet()) != out) continue;
    std::array<Net, 2> nets {};
    size_t index = 0;
    for (auto* bit : inputs->getBits()) nets[index++] = gate->getInstTerm(bit)->getNet();
    const auto before = result.size();
    for (size_t held = 0; held < 2; ++held) {
      auto* source = driver(nets[held]);
      if (!source || !NLDB0::isDLatch(source->getInstance()->getModel())) continue;
      auto* latch = source->getInstance();
      if (latch->getInstTerms().size() != 3 || source->getBitTerm()->getName() != NLName("Q")) continue;
      auto* enable = latch->getInstTerm(latch->getModel()->getScalarTerm(NLName("E")));
      auto* data = latch->getInstTerm(latch->getModel()->getScalarTerm(NLName("D")));
      const auto e = origin(enable->getNet()), c = origin(nets[1-held]);
      if (!e.net || e.net != c.net || e.inverted == c.inverted || !data->getNet()) continue;
      // The state must be private to this gate, including at module boundaries.
      if (!nets[held]->getBitTerms().empty() || nets[held]->getInstTerms().size() != 2) continue;
      result.push_back({gate, latch, nets[1-held], data->getNet(), nets[held], out->getNet()});
    }
    if (result.size() > before + 1) result.resize(before);
  }
  return result;
}

// A complete scalar wrapper can be replaced per parent instance. This avoids
// changing a shared gate model that is also instantiated on a data path.
struct Wrapper {
  SNLBitTerm* clock {nullptr};
  SNLBitTerm* enable {nullptr};
  SNLBitTerm* test {nullptr};
  SNLBitTerm* output {nullptr};
};
Wrapper wrapper(SNLDesign* design, const Pattern& pattern) {
  Wrapper result;
  std::vector<SNLBitTerm*> inputs;
  for (auto* term : design->getBitTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Input) inputs.push_back(term);
    else if (term->getDirection() == SNLTerm::Direction::Output && !result.output &&
        same(term->getNet(), pattern.output)) result.output = term;
    else return {};
  }
  if (!result.output || (inputs.size() != 2 && inputs.size() != 3)) return {};
  for (auto* term : inputs) if (same(term->getNet(), pattern.clock)) {
    if (result.clock) return {};
    result.clock = term;
  }
  if (!result.clock) return {};
  auto input = [&](Net net) -> SNLBitTerm* {
    SNLBitTerm* found = nullptr;
    for (auto* term : inputs) if (term != result.clock && same(term->getNet(), net)) {
      if (found) return nullptr;
      found = term;
    }
    return found;
  };
  std::unordered_set<SNLInstance*> allowed {pattern.gate, pattern.latch};
  auto aliases = [&](Net net) {
    std::unordered_set<Net> seen;
    while (net && seen.insert(net).second) {
      auto* source = driver(net);
      if (!source) break;
      auto* instance = source->getInstance();
      auto* model = instance->getModel();
      if (NLDB0::isAssign(model)) {
        allowed.insert(instance);
        net = instance->getInstTerm(NLDB0::getAssignInput())->getNet();
      } else if (NLDB0::isGate(model) && instance->getInstTerms().size() == 2 &&
          (NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Buf ||
           NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Not)) {
        allowed.insert(instance);
        for (auto* pin : instance->getInstTerms())
          if (pin->getDirection() == SNLTerm::Direction::Input) net = pin->getNet();
      } else break;
    }
  };
  aliases(pattern.clock);
  aliases(pattern.latch->getInstTerm(pattern.latch->getModel()->getScalarTerm(NLName("E")))->getNet());
  aliases(pattern.enable);
  aliases(result.output->getNet());
  if (inputs.size() == 2) result.enable = input(pattern.enable);
  else {
    auto o = origin(pattern.enable);
    auto* source = o.inverted ? nullptr : driver(o.net);
    auto* combine = source ? source->getInstance() : nullptr;
    if (!combine || !NLDB0::isGate(combine->getModel()) ||
        NLDB0::GateType(NLDB0::getGateName(combine->getModel())) != NLDB0::GateType::Or ||
        combine->getInstTerms().size() != 3) return {};
    std::vector<SNLBitTerm*> controls;
    for (auto* pin : combine->getInstTerms()) if (pin->getDirection() == SNLTerm::Direction::Input) {
      auto* term = input(pin->getNet());
      if (!term) return {};
      controls.push_back(term);
      aliases(pin->getNet());
    }
    if (controls.size() != 2 || controls[0] == controls[1]) return {};
    // Term IDs, not pointer order or source-language names, determine EN/TEST_EN.
    if (controls[1]->getID() < controls[0]->getID()) std::swap(controls[0], controls[1]);
    result.enable = controls[0]; result.test = controls[1];
    allowed.insert(combine);
  }
  if (!result.enable) return {};
  for (auto* instance : design->getInstances()) if (!allowed.contains(instance)) return {};
  return result;
}

struct Occurrence {
  SNLDesign* model;
  size_t parent;
  SNLInstance* instance;
  std::unordered_map<SNLInstance*, size_t> children;
};
struct Point {
  size_t occurrence;
  Net net;
  bool operator==(const Point&) const = default;
};
struct Hash {
  size_t operator()(const Point& point) const {
    return std::hash<Net>{}(point.net) ^ (point.occurrence * size_t(0x9e3779b9));
  }
};
using Points = std::unordered_set<Point, Hash>;
constexpr size_t NoParent = size_t(-1);

class Inference {
  public:
    std::vector<Occurrence> occurrences;
    std::vector<SNLDesign*> models;
    std::unordered_map<SNLDesign*, std::vector<Pattern>> candidates;
    std::unordered_map<SNLDesign*, std::vector<size_t>> uses;
    std::unordered_set<SNLInstance*> seenInstances;
    std::unordered_set<SNLDesign*> incomplete;
    bool hasCandidates {false};
    Points clocks, declared;
    std::deque<Point> pending;

    void add(size_t occurrence, Net net) {
      if (net && !net->isAssignConstant() && clocks.insert({occurrence, net}).second)
        pending.push_back({occurrence, net});
    }

    // Discover candidates once per model before expanding shared occurrences.
    // Most elaborated designs contain no clock-gating pattern at all.
    void discover(SNLDesign* model) {
      if (!model || model->isPrimitive() || model->isBlackBox() || candidates.contains(model)) return;
      auto found = patterns(model);
      hasCandidates |= !found.empty();
      candidates.emplace(model, std::move(found));
      models.push_back(model);
      for (auto* child : model->getInstances()) discover(child->getModel());
    }

    void collect(SNLDesign* model, size_t parent, SNLInstance* instance,
        std::unordered_set<SNLDesign*>& active) {
      if (!model || model->isPrimitive() || model->isBlackBox() || !active.insert(model).second) return;
      const auto id = occurrences.size();
      occurrences.push_back({model, parent, instance, {}});
      if (parent != NoParent) occurrences[parent].children.emplace(instance, id);
      uses[model].push_back(id);
      for (auto* child : model->getInstances()) {
        seenInstances.insert(child);
        collect(child->getModel(), id, child, active);
      }
      active.erase(model);
    }

    // Enumerate aliases across hierarchy without constructing a flattened netlist.
    void boundary(Point p, const std::function<void(Point)>& visit) const {
      const auto& occurrence = occurrences[p.occurrence];
      if (occurrence.parent != NoParent) {
        for (auto* term : p.net->getBitTerms()) {
          if (auto* actual = occurrence.instance->getInstTerm(term)->getNet())
            visit({occurrence.parent, actual});
        }
      }
      for (auto* term : p.net->getInstTerms()) {
        const auto child = occurrence.children.find(term->getInstance());
        if (child != occurrence.children.end())
          if (auto* formal = term->getBitTerm()->getNet()) visit({child->second, formal});
      }
    }

    std::vector<SNLInstTerm*> routing(SNLInstance* instance) const {
      auto* model = instance->getModel();
      if (NLDB0::isAssign(model) || (NLDB0::isGate(model) &&
          (NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Buf ||
           NLDB0::GateType(NLDB0::getGateName(model)) == NLDB0::GateType::Not) &&
          instance->getInstTerms().size() == 2))
        return {instance->getInstTerms().begin(), instance->getInstTerms().end()};
      // Frontends may coerce a clock-valued condition through a 0/1 mux. This
      // establishes provenance only; inference never removes this conversion.
      if (NLDB0::isMux2(model) && instance->getInstTerms().size() == 4) {
        auto* a = instance->getInstTerm(*model->getTerm(NLName("A"))->getBits().begin());
        auto* b = instance->getInstTerm(*model->getTerm(NLName("B"))->getBits().begin());
        auto* select = instance->getInstTerm(model->getScalarTerm(NLName("S")));
        auto* output = instance->getInstTerm(*model->getTerm(NLName("Y"))->getBits().begin());
        if (a && b && select && output && a->getNet() && b->getNet() &&
            ((a->getNet()->isAssign0() && b->getNet()->isAssign1()) ||
             (a->getNet()->isAssign1() && b->getNet()->isAssign0()))) return {select, output};
      }
      return {};
    }

    void classify() {
      for (size_t id = 0; id < occurrences.size(); ++id) {
        auto* design = occurrences[id].model;
        for (auto* clock : design->getClocks())
          for (auto* source : clock->getSources())
            if (auto* term = dynamic_cast<SNLBitTerm*>(source); term && term->getNet()) {
              declared.insert({id, term->getNet()});
              add(id, term->getNet());
            }
        for (auto* instance : design->getInstances())
          if (instance->getModel()->isPrimitive())
            for (auto* term : instance->getInstTerms())
              if (term->getDirection() == SNLTerm::Direction::Input &&
                  SNLDesignModeling::getTermRole(term) == Role::Clock) add(id, term->getNet());
      }
      while (!pending.empty()) {
        const auto p = pending.front(); pending.pop_front();
        boundary(p, [&](Point next) { add(next.occurrence, next.net); });
        for (auto* term : p.net->getInstTerms()) {
          auto* instance = term->getInstance();
          const auto wires = routing(instance);
          if (std::find(wires.begin(), wires.end(), term) != wires.end())
            for (auto* pin : wires) add(p.occurrence, pin->getNet());
          if (NLDB0::isClockGate(instance->getModel()) &&
              (term->getBitTerm() == NLDB0::getClockGateClock() ||
               term->getBitTerm() == NLDB0::getClockGateOutput())) {
            add(p.occurrence, instance->getInstTerm(NLDB0::getClockGateClock())->getNet());
            add(p.occurrence, instance->getInstTerm(NLDB0::getClockGateOutput())->getNet());
          }
        }
        for (const auto& pattern : candidates.at(occurrences[p.occurrence].model)) {
          if (same(p.net, pattern.clock)) add(p.occurrence, pattern.output);
          if (same(p.net, pattern.output)) add(p.occurrence, pattern.clock);
        }
      }
    }

    bool clockOnly(Point start) const {
      std::vector<Point> queue {start}; Points visited;
      bool sink = false;
      for (size_t i = 0; i < queue.size(); ++i) {
        const auto p = queue[i];
        if (!visited.insert(p).second) continue;
        const auto& occurrence = occurrences[p.occurrence];
        if (!uniqueDrivers(p.net)) return false;
        boundary(p, [&](Point next) { queue.push_back(next); });
        if (occurrence.parent == NoParent && !p.net->getBitTerms().empty()) {
          if (!declared.contains(p)) return false;
          sink = true;
        }
        for (auto* term : p.net->getInstTerms()) {
          if (term->getDirection() == SNLTerm::Direction::Output) continue;
          if (term->getDirection() != SNLTerm::Direction::Input) return false;
          auto* instance = term->getInstance();
          if (occurrence.children.contains(instance)) continue;
          const auto wires = routing(instance);
          if (std::find(wires.begin(), wires.end(), term) != wires.end()) {
            for (auto* pin : wires)
              if (pin->getDirection() == SNLTerm::Direction::Output && pin->getNet())
                queue.push_back({p.occurrence, pin->getNet()});
            continue;
          }
          const auto role = SNLDesignModeling::getTermRole(term);
          if (role == Role::Clock || (NLDB0::isDLatch(instance->getModel()) && role == Role::Enable)) {
            sink = true;
            continue;
          }
          bool clockLeg = false;
          for (const auto& pattern : candidates.at(occurrence.model))
            if (instance == pattern.gate && term->getNet() == pattern.clock) {
              queue.push_back({p.occurrence, pattern.output}); clockLeg = true;
            }
          if (!clockLeg) return false;
        }
      }
      return sink;
    }

    bool sourceSafe(Point start) const {
      std::vector<Point> queue {start}; Points visited;
      for (size_t i = 0; i < queue.size(); ++i) {
        const auto p = queue[i];
        if (!visited.insert(p).second) continue;
        if (!uniqueDrivers(p.net)) return false;
        boundary(p, [&](Point next) { queue.push_back(next); });
        for (auto* term : p.net->getInstTerms()) {
          const auto wires = routing(term->getInstance());
          if (std::find(wires.begin(), wires.end(), term) != wires.end())
            for (auto* pin : wires) if (pin->getNet()) queue.push_back({p.occurrence, pin->getNet()});
        }
      }
      return true;
    }

    bool eligible(const Pattern& pattern, size_t occurrence) const {
      return clocks.contains({occurrence, pattern.clock}) &&
        sourceSafe({occurrence, pattern.clock}) && clockOnly({occurrence, pattern.output});
    }
};

Net zero(SNLDesign* design) {
  for (auto* net : design->getBitNets()) if (net->isAssign0()) return net;
  auto* net = SNLScalarNet::create(design);
  net->setType(SNLNet::Type::Assign0);
  return net;
}

SNLInstance* replace(SNLInstance* previous, Net clk, Net en, Net test, Net output, const SNLClockGating::Hooks& hooks) {
  auto* owner = previous->getDesign();
  const auto id = previous->getID();
  const auto order = previous->getOrderID();
  const auto name = previous->getName();
  std::vector<SNLAttribute> attributes;
  for (const auto& attribute : SNLAttributes::getAttributes(previous)) attributes.push_back(attribute);
  std::optional<SNLSourceLoc> location;
  std::optional<SNLSourceReference> declaration;
  std::vector<SNLSourceReference> origins;
  SNLRTLInfos::Infos infos;
  const bool hasInfos = previous->getRTLInfos();
  if (auto* metadata = previous->getRTLInfos()) {
    location = metadata->getSourceLoc();
    declaration = metadata->getSourceDeclaration();
    origins = metadata->getSourceOrigins();
    infos = metadata->getInfos();
  }
  if (hooks.beforeReplace) hooks.beforeReplace(previous);
  previous->destroy();
  auto* final = SNLInstance::create(owner, NLDB0::getOrCreateClockGate(), id, name);
  final->setOrderID(order);
  if (hasInfos) {
    auto* metadata = SNLRTLInfos::create(final);
    if (location) metadata->setSourceLoc(*location);
    if (declaration) metadata->setSourceDeclaration(*declaration);
    for (const auto& reference : origins) metadata->addSourceOrigin(reference);
    for (const auto& [key, value] : infos) metadata->setInfo(key, value);
  }
  for (const auto& attribute : attributes) SNLAttributes::addAttribute(final, attribute);
  final->setTermNet(NLDB0::getClockGateClock(), clk);
  final->setTermNet(NLDB0::getClockGateEnable(), en);
  final->setTermNet(NLDB0::getClockGateTestEnable(), test ? test : zero(owner));
  final->setTermNet(NLDB0::getClockGateOutput(), output);
  SNLAttributes::addAttribute(final, SNLAttribute(NLName("naja_clock_gate_inference"),
    SNLAttributeValue("low_phase_pattern_and_clock_use")));
  if (hooks.afterReplace) hooks.afterReplace(final);
  return final;
}
}

size_t SNLClockGating::infer(const std::vector<SNLDesign*>& roots, const Hooks& hooks) {
  Inference inference;
  for (auto* root : roots) inference.discover(root);
  if (!inference.hasCandidates) return 0;
  std::unordered_set<SNLDesign*> active;
  for (auto* root : roots) inference.collect(root, NoParent, nullptr, active);
  // A shared owner referenced outside the analyzed hierarchy is ambiguous too.
  for (auto* design : inference.models)
    for (auto* instance : design->getSlaveInstances())
      if (!inference.seenInstances.contains(instance)) inference.incomplete.insert(design);
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto* design : inference.models)
      if (inference.incomplete.contains(design))
        for (auto* instance : design->getInstances())
          if (inference.candidates.contains(instance->getModel()))
            changed |= inference.incomplete.insert(instance->getModel()).second;
  }
  inference.classify();
  struct Replacement { SNLInstance* instance; Net clock; Net enable; Net test; Net output; };
  std::vector<Replacement> wrappers;
  std::vector<Pattern> selected;
  for (auto* design : inference.models) {
    const auto& candidates = inference.candidates.at(design);
    bool wrapped = false;
    if (candidates.size() == 1) {
      const auto pins = wrapper(design, candidates.front());
      if (pins.clock) {
        std::vector<SNLInstance*> order;
        std::unordered_map<SNLInstance*, bool> eligible;
        for (auto occurrence : inference.uses.at(design)) {
          const auto& use = inference.occurrences[occurrence];
          if (!use.instance) continue;
          if (!eligible.contains(use.instance)) {
            eligible.emplace(use.instance, true);
            order.push_back(use.instance);
          }
          eligible[use.instance] = eligible[use.instance] &&
            !inference.incomplete.contains(use.instance->getDesign()) &&
            inference.eligible(candidates.front(), occurrence);
        }
        for (auto* instance : order) if (eligible.at(instance)) {
          auto net = [&](SNLBitTerm* pin) { return pin ? instance->getInstTerm(pin)->getNet() : nullptr; };
          if (net(pins.clock) && net(pins.enable) && net(pins.output) && (!pins.test || net(pins.test))) {
            wrappers.push_back({instance, net(pins.clock), net(pins.enable), net(pins.test), net(pins.output)});
            wrapped = true;
          }
        }
      }
    }
    if (wrapped || inference.incomplete.contains(design)) continue;
    for (const auto& pattern : candidates) {
      bool eligible = true;
      for (auto occurrence : inference.uses.at(design))
        eligible &= inference.eligible(pattern, occurrence);
      if (eligible) selected.push_back(pattern);
    }
  }
  for (const auto& plan : wrappers)
    replace(plan.instance, plan.clock, plan.enable, plan.test, plan.output, hooks);
  for (const auto& pattern : selected) {
    replace(pattern.gate, pattern.clock, pattern.enable, nullptr, pattern.output, hooks);
    if (hooks.beforeRemove) hooks.beforeRemove(pattern.latch);
    pattern.latch->destroy();
  }
  return selected.size() + wrappers.size();
}
}
