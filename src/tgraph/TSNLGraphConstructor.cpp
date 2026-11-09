// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TSNLGraphConstructor.h"

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#include "DNL.h"
#include "NLUniverse.h"
#include "NLClock.h"
#include "NLException.h"
#include "NLDB0.h"
#include "SNLBitNet.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLPath.h"
#include "SNLScalarTerm.h"

namespace naja::TG {
namespace {

using namespace naja::NL;
using PinRole = TNode::PinRole;
using CellKind = TNode::CellKind;
using Model = SNLDesignModeling;
using Constructor = TSNLGraphConstructor;
using Reason = Constructor::CoverageReason;
using Treatment = Constructor::Treatment;
struct Coverage {
  std::vector<Constructor::CoverageEntry> entries;
  void record(const SNLOccurrence& occurrence, const SNLDesign* model, Reason reason,
      Treatment treatment, const std::string& detail) {
    entries.push_back({occurrence, model->getName().getString(), reason, treatment, detail});
  }
};
// DNL uses term-range start + model bit-term OrderID for terminal lookup.
// Mirror that indexing against the graph's DNL-terminal-to-node table, without
// depending on DNL's global terminal helpers or repeated occurrence searches.
class LeafNodes {
  public:
    LeafNodes(naja::DNL::DNLID first, const std::vector<TGraph::NodeId>& nodes)
      : first_(first), nodes_(nodes) {}
    TGraph::NodeId at(const SNLInstTerm* term) const {
      return nodes_.at(first_ + term->getBitTerm()->getOrderID());
    }
  private:
    naja::DNL::DNLID first_;
    const std::vector<TGraph::NodeId>& nodes_;
};

[[noreturn]] void fail(const SNLOccurrence& occurrence, const std::string& reason) {
  throw NLException("TSNLGraphConstructor: " + reason + " at " + occurrence.getDescription());
}

void checkDirection(const SNLOccurrence& occurrence, SNLTerm::Direction direction,
    const SNLDesign* model, Coverage& coverage) {
  if (direction != SNLTerm::Direction::Input && direction != SNLTerm::Direction::Output) {
    coverage.record(occurrence, model, Reason::InOut, Treatment::Bidirectional, "inout terminal (load/drive roles)");
  }
}


PinRole pinRole(SNLInstTerm* term, bool sequential) {
  if (!sequential) {
    return term->getDirection() == SNLTerm::Direction::Output ? PinRole::DataOutput : PinRole::DataInput;
  }
  switch (Model::getTermRole(term)) {
    case Model::SNLTermRole::Clock: return PinRole::Clock;
    case Model::SNLTermRole::DataInput: return PinRole::DataInput;
    case Model::SNLTermRole::DataOutput: return PinRole::DataOutput;
    case Model::SNLTermRole::Enable: return PinRole::Enable;
    case Model::SNLTermRole::SyncReset: return PinRole::SyncReset;
    case Model::SNLTermRole::SyncSet: return PinRole::SyncSet;
    case Model::SNLTermRole::AsyncReset: return PinRole::AsyncReset;
    case Model::SNLTermRole::AsyncSet: return PinRole::AsyncSet;
    default: return PinRole::Unknown;
  }
}

bool isCaptureRole(PinRole role) {
  return role == PinRole::DataInput || role == PinRole::Enable ||
    role == PinRole::SyncReset || role == PinRole::SyncSet;
}

const Model::BooleanExpression::Node* expressionNode(
    const Model::BooleanExpression& expression, size_t index,
    Model::BooleanExpression::Operator operation, size_t operands) {
  if (index >= expression.nodes.size()) return nullptr;
  const auto& node = expression.nodes[index];
  return node.operation == operation && node.operands.size() == operands ? &node : nullptr;
}

struct Latch {
  TGraph::NodeId gate;
  TGraph::NodeId data;
  TGraph::NodeId output;
  SNLOccurrence occurrence;
};
struct AndGate {
  TGraph::NodeId inputs[2];
  TGraph::NodeId output;
  TGraph::EdgeId arcs[2];
};
struct Transparent {
  TGraph::NodeId input;
  TGraph::NodeId output;
  bool inverted;
};
struct Candidates {
  std::vector<TSNLGraphConstructor::ClockGate> canonicalGates;
  std::vector<Latch> latches;
  std::vector<AndGate> conjunctions;
  std::vector<Transparent> transparent;
};

// Stage one cell's arcs until its complete model is validated. Discarding an
// unsupported wide cell is linear in its pins/arcs, without repeatedly erasing
// adjacency lists or leaving holes in the published graph.
class LeafArcsBuilder {
  public:
    struct Arc {
      TGraph::NodeId source;
      TGraph::NodeId target;
      std::unique_ptr<TSNLEdgeProxy> proxy;
      TEdge::Kind kind {TEdge::Kind::Combinational};
      void setKind(TEdge::Kind value) { kind = value; }
    };
    explicit LeafArcsBuilder(TGraph& graph): graph_(graph) {}
    TNode* getNode(TGraph::NodeId id) { return graph_.getNode(id); }
    TGraph::EdgeId createEdge(TGraph::NodeId from, TGraph::NodeId to, TSNLEdgeProxy* proxy) {
      arcs_.push_back({from, to, std::unique_ptr<TSNLEdgeProxy>(proxy)});
      return arcs_.size() - 1;
    }
    Arc* getEdge(TGraph::EdgeId id) { return &arcs_[id]; }
    void createCaptureEdge(TGraph::NodeId from, TGraph::NodeId to) {
      captures_.emplace_back(from, to);
    }
    void publish(Candidates& candidates) {
      std::vector<TGraph::EdgeId> ids;
      ids.reserve(arcs_.size());
      for (auto& arc : arcs_) {
        const auto* proxy = graph_.getNode(arc.target)->getProxy();
        auto target = proxy ? proxy->getDriverNode(arc.target) : arc.target;
        auto id = graph_.createEdge(arc.source, target, arc.proxy.release());
        graph_.getEdge(id)->setKind(arc.kind);
        ids.push_back(id);
      }
      for (const auto& capture : captures_) graph_.createCaptureEdge(capture.first, capture.second);
      auto driver = [&](TGraph::NodeId id) { return graph_.getNode(id)->getProxy()->getDriverNode(id); };
      for (auto& gate : candidates.canonicalGates) {
        gate.output = driver(gate.output);
        gate.clockPassEdge = ids[gate.clockPassEdge];
      }
      for (auto& latch : candidates.latches) latch.output = driver(latch.output);
      for (auto& transparent : candidates.transparent) transparent.output = driver(transparent.output);
      for (auto& gate : candidates.conjunctions) {
        gate.output = driver(gate.output);
        for (auto& arc : gate.arcs) arc = ids[arc];
      }
    }
  private:
    TGraph& graph_;
    std::vector<Arc> arcs_;
    std::vector<std::pair<TGraph::NodeId, TGraph::NodeId>> captures_;
};

void buildLatch(LeafArcsBuilder& graph, const LeafNodes& nodes, const SNLPath& path,
    SNLInstance* instance, const Model::SequentialModel& sequence, Candidates& candidates) {
  using Operator = Model::BooleanExpression::Operator;
  const SNLOccurrence occurrence(path, instance);
  auto unsupported = [&]() {
    fail(occurrence, "latch analysis is unsupported: expected a scalar DB0 latch in a low-phase clock gate");
  };
  if (!NLDB0::isDLatch(instance->getModel()) || instance->getInstTerms().size() != 3 ||
      sequence.states.size() != 1 || sequence.outputs.size() != 1 ||
      sequence.states.front().clear || sequence.states.front().preset) unsupported();
  const auto* active = expressionNode(sequence.clockedOn, sequence.clockedOn.root, Operator::Term, 0);
  const auto& next = sequence.states.front().nextState;
  const auto* data = expressionNode(next, next.root, Operator::Term, 0);
  const auto& function = sequence.outputs.front().function;
  const auto* state = expressionNode(function, function.root, Operator::State, 0);
  if (!active || !data || !state || state->state != 0 ||
      active->term != NLDB0::getDLatchEnable() || data->term != NLDB0::getDLatchData() ||
      sequence.outputs.front().term != NLDB0::getDLatchOutput()) unsupported();
  auto* gatePin = instance->getInstTerm(active->term);
  auto* dataPin = instance->getInstTerm(data->term);
  auto* outputPin = instance->getInstTerm(sequence.outputs.front().term);
  const auto captures = Model::getClockRelatedInputs(gatePin);
  const auto outputs = Model::getClockRelatedOutputs(gatePin);
  if (Model::getTermRole(gatePin) != Model::SNLTermRole::Enable ||
      Model::getTermRole(dataPin) != Model::SNLTermRole::DataInput ||
      Model::getTermRole(outputPin) != Model::SNLTermRole::DataOutput ||
      captures.size() != 1 || *captures.begin() != dataPin ||
      outputs.size() != 1 || *outputs.begin() != outputPin) unsupported();
  auto nodeId = [&](SNLBitTerm* term) { return nodes.at(instance->getInstTerm(term)); };
  Latch latch {nodeId(active->term), nodeId(data->term), nodeId(sequence.outputs.front().term), occurrence};
  for (auto id : {latch.gate, latch.data, latch.output}) graph.getNode(id)->setCellKind(CellKind::Latch);
  graph.getNode(latch.gate)->setPinRole(PinRole::Gate);
  graph.getNode(latch.data)->setPinRole(PinRole::DataInput);
  graph.getNode(latch.output)->setPinRole(PinRole::DataOutput);
  auto stateEdge = graph.createEdge(latch.gate, latch.output,
    new TSNLEdgeProxy(TSNLEdgeProxy::Kind::LatchGateToOutput, occurrence));
  graph.getEdge(stateEdge)->setKind(TEdge::Kind::State);
  candidates.latches.push_back(latch);
}

//Normalize only wires and canonical DB0 buffers/inverters. Memoized iterative
//walks are linear in nodes plus edges, including shared cones and cycles.
std::vector<TSNLGraphConstructor::ClockGate> recognizeClockGates(
    TGraph& graph, const Candidates& candidates, Coverage& coverage) {
  if (candidates.latches.empty()) return {};
  const auto size = graph.getNodesStorage().size();
  struct Origin { TGraph::NodeId node {TGraph::kInvalidNode}; bool inverted {false}; };
  std::vector<Origin> predecessor(size), origin(size);
  std::vector<bool> ambiguous(size, false);
  for (const auto& edge : graph.getEdgesStorage()) {
    if (!edge.isAlive()) continue;
    const auto* proxy = static_cast<const TSNLEdgeProxy*>(edge.getProxy());
    if (proxy->getKind() == TSNLEdgeProxy::Kind::Connectivity) {
      auto& previous = predecessor[edge.getTarget()];
      if (previous.node != TGraph::kInvalidNode) {
        ambiguous[edge.getTarget()] = true;
      }
      previous.node = edge.getSource();
    }
  }
  for (size_t i = 0; i < size; ++i) {
    if (ambiguous[i]) predecessor[i] = {};
  }
  for (const auto& link : candidates.transparent) predecessor[link.output] = {link.input, link.inverted};
  std::vector<unsigned char> color(size, 0);
  std::vector<TGraph::NodeId> path;
  for (TGraph::NodeId start = 0; start < size; ++start) {
    if (color[start] != 0) continue;
    auto current = start;
    while (color[current] == 0) {
      color[current] = 1;
      path.push_back(current);
      if (predecessor[current].node == TGraph::kInvalidNode) {
        origin[current] = ambiguous[current] ? Origin{} : Origin{current, false};
        color[current] = 2;
        break;
      }
      current = predecessor[current].node;
    }
    //An unfinished predecessor denotes a combinational wire/buffer cycle;
    //its source is unknown, so it cannot prove the required clock phase.
    Origin resolved = color[current] == 2 ? origin[current] : Origin{};
    while (!path.empty()) {
      auto node = path.back();
      path.pop_back();
      if (color[node] == 2) continue;
      resolved.inverted = resolved.inverted != predecessor[node].inverted;
      origin[node] = resolved;
      color[node] = 2;
    }
  }
  std::vector<size_t> latchAt(size, TGraph::kInvalidNode);
  std::vector<bool> matched(candidates.latches.size(), false);
  for (size_t i = 0; i < candidates.latches.size(); ++i) latchAt[candidates.latches[i].output] = i;
  std::vector<TSNLGraphConstructor::ClockGate> result;
  for (const auto& gate : candidates.conjunctions) {
    const auto firstMatch = result.size();
    for (size_t held = 0; held < 2; ++held) {
      const auto q = origin[gate.inputs[held]];
      const auto c = origin[gate.inputs[1 - held]];
      if (q.node == TGraph::kInvalidNode || q.inverted ||
          c.node == TGraph::kInvalidNode || latchAt[q.node] == TGraph::kInvalidNode) continue;
      const auto index = latchAt[q.node];
      const auto& latch = candidates.latches[index];
      const auto enable = origin[latch.gate];
      if (enable.node != c.node || enable.inverted == c.inverted) continue;
      result.push_back({gate.inputs[1 - held], latch.data, latch.output, latch.gate,
        gate.output, gate.arcs[1 - held]});
    }
    if (result.size() > firstMatch + 1) {
      const auto* proxy = static_cast<const TSNLEdgeProxy*>(graph.getEdge(gate.arcs[0])->getProxy());
      const auto& occurrence = proxy->getOccurrence();
      coverage.record(occurrence, static_cast<SNLInstance*>(occurrence.getObject())->getModel(),
        Reason::AmbiguousClockGate, Treatment::Approximated,
        "ambiguous clock-gate composition: both AND inputs match held states");
      result.resize(firstMatch);
    }
  }
  for (const auto& gate : result) matched[latchAt[gate.heldEnable]] = true;
  for (size_t i = 0; i < matched.size(); ++i) {
    if (!matched[i]) {
      const auto& latch = candidates.latches[i];
      coverage.record(latch.occurrence, static_cast<SNLInstance*>(latch.occurrence.getObject())->getModel(),
        Reason::UnsupportedLatch, Treatment::Approximated,
        "latch analysis is unsupported: no matching low-phase DB0 latch-and-AND clock gate");
      graph.createCaptureEdge(latch.gate, latch.data);
    }
  }
  return result;
}

void buildPlainLatch(LeafArcsBuilder& graph, const LeafNodes& nodes, const SNLPath& path,
    SNLInstance* instance, const Model::SequentialModel& sequence) {
  const SNLOccurrence occurrence(path, instance);
  auto root = sequence.clockedOn.root;
  while (root < sequence.clockedOn.nodes.size() &&
      sequence.clockedOn.nodes[root].operation == Model::BooleanExpression::Operator::Not &&
      sequence.clockedOn.nodes[root].operands.size() == 1) {
    auto next = sequence.clockedOn.nodes[root].operands.front();
    if (next >= root) fail(occurrence, "latch analysis is unsupported: cyclic gate expression");
    root = next;
  }
  const auto* active = expressionNode(sequence.clockedOn, root, Model::BooleanExpression::Operator::Term, 0);
  if (!active || !active->term) fail(occurrence, "latch analysis is unsupported: composite gate");
  auto* gate = instance->getInstTerm(active->term);
  auto id = [&](SNLInstTerm* term) { return nodes.at(term); };
  for (auto* term : instance->getInstTerms()) {
    auto role = pinRole(term, true);
    if (term == gate) role = PinRole::Gate;
    if (role == PinRole::Unknown) fail(occurrence, "latch analysis is unsupported: missing role");
    graph.getNode(id(term))->setCellKind(CellKind::Latch);
    graph.getNode(id(term))->setPinRole(role);
  }
  std::set<SNLBitTerm*> outputs;
  for (const auto& output : sequence.outputs) {
    const auto* state = expressionNode(output.function, output.function.root,
      Model::BooleanExpression::Operator::State, 0);
    if (!state || state->state >= sequence.states.size())
      fail(occurrence, "latch analysis is unsupported: non-state output");
    const auto& value = sequence.states[state->state];
    if (value.clear || value.preset) fail(occurrence, "latch analysis is unsupported: reset state");
    outputs.insert(output.term);
  }
  for (auto* term : instance->getInstTerms()) {
    if (term == gate) continue;
    if (term->getDirection() == SNLTerm::Direction::Output) {
      if (!outputs.count(term->getBitTerm())) fail(occurrence, "latch analysis is unsupported: missing state");
      auto edge = graph.createEdge(id(gate), id(term),
        new TSNLEdgeProxy(TSNLEdgeProxy::Kind::LatchGateToOutput, occurrence));
      graph.getEdge(edge)->setKind(TEdge::Kind::State);
    } else if (isCaptureRole(pinRole(term, true))) {
      graph.createCaptureEdge(id(gate), id(term));
    } else fail(occurrence, "latch analysis is unsupported: control role");
  }
}

void buildMemory(LeafArcsBuilder& graph, const LeafNodes& nodes, const SNLPath& path, SNLInstance* instance) {
  // Use the declared interface, retaining disconnected pins as for flops.
  const auto memory = Model::getMemoryInterface(instance->getModel());
  const SNLOccurrence occurrence(path, instance);
  const bool db0Memory = NLDB0::isMemory(instance->getModel());
  if (db0Memory &&
      !(NLDB0::getMemorySignature(instance) == NLDB0::getMemorySignature(instance->getModel())))
    fail(occurrence, "memory analysis is unsupported: instance signature differs from declared ports");
  if (!memory.isValid()) fail(occurrence, "memory analysis is unsupported: invalid interface");
  auto id = [&](SNLBitTerm* term) {
    return nodes.at(instance->getInstTerm(term));
  };
  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> classified;
  auto classify = [&](SNLBitTerm* term, PinRole role) {
    classified.insert(term);
    graph.getNode(id(term))->setPinRole(role);
  };
  std::vector<SNLBitTerm*> clocks;
  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> seenClocks;
  for (const auto& port : memory.writePorts) {
    auto* clock = port.clock ? port.clock : memory.clock;
    if (!clock) fail(occurrence, "memory analysis is unsupported: missing write clock");
    classify(clock, PinRole::Clock);
    if (seenClocks.insert(clock).second) clocks.push_back(clock);
    auto capture = [&](const Model::BitTerms& terms, PinRole role) {
      for (auto* term : terms) {
        classify(term, role);
        graph.createCaptureEdge(id(clock), id(term));
      }
    };
    capture(port.address, PinRole::DataInput);
    capture(port.data, PinRole::DataInput);
    capture(port.mask, PinRole::Enable);
    capture(port.enables, PinRole::Enable);
    for (const auto& extra : port.extraWriteInputs) capture(extra, PinRole::DataInput);
  }
  if (memory.clock) classify(memory.clock, PinRole::Clock);
  // Multi-write-clock DB0 models retain unused legacy CLK/RST terminals.
  if (db0Memory) {
    if (auto* clock = NLDB0::getMemoryClock(instance->getModel())) classify(clock, PinRole::Clock);
    if (!memory.reset) {
      if (auto* reset = NLDB0::getMemoryReset(instance->getModel())) classify(reset, PinRole::Unknown);
    }
  }
  if (memory.reset) {
    auto role = PinRole::Unknown;
    if (memory.resetMode == Model::MemoryResetMode::AsyncLow ||
        memory.resetMode == Model::MemoryResetMode::AsyncHigh) role = PinRole::AsyncReset;
    if (memory.resetMode == Model::MemoryResetMode::SyncLow ||
        memory.resetMode == Model::MemoryResetMode::SyncHigh) role = PinRole::SyncReset;
    classify(memory.reset, role);
    if (role == PinRole::SyncReset) {
      for (auto* clock : clocks) graph.createCaptureEdge(id(clock), id(memory.reset));
    }
  }
  for (const auto& port : memory.readPorts) {
    for (auto* address : port.address) classify(address, PinRole::DataInput);
    for (auto* enable : port.enables) classify(enable, PinRole::Enable);
    std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> allowed;
    if (!db0Memory) {
      allowed.insert(port.address.begin(), port.address.end());
      allowed.insert(port.enables.begin(), port.enables.end());
    }
    for (auto* data : port.data) {
      classify(data, PinRole::DataOutput);
      // MemoryInterface describes asynchronous reads. Require an explicit
      // address dependency to avoid mistaking a synchronous macro for one.
      if (!db0Memory) {
        const auto inputs = Model::getCombinatorialInputs(data);
        if (inputs.empty()) fail(occurrence, "memory analysis is unsupported: missing asynchronous read timing");
        for (auto* input : inputs) {
          if (!allowed.count(input)) fail(occurrence, "memory analysis is unsupported: mixed read dependency");
          graph.createEdge(id(input), id(data),
            new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Combinational, occurrence));
        }
      } else {
        for (const auto& inputs : {port.address, port.enables}) {
          for (auto* input : inputs) graph.createEdge(id(input), id(data),
            new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Combinational, occurrence));
        }
      }
      // Unknown cell kind preserves address traversal, unlike implicit flop Q
      // state boundaries. Explicit State arcs seed stored-data launch origins.
      for (auto* clock : clocks) {
        auto edge = graph.createEdge(id(clock), id(data),
          new TSNLEdgeProxy(TSNLEdgeProxy::Kind::ClockToOutput, occurrence));
        graph.getEdge(edge)->setKind(TEdge::Kind::State);
      }
    }
  }
  for (auto* term : instance->getInstTerms()) {
    if (!classified.count(term->getBitTerm()))
      fail(occurrence, "memory analysis is unsupported: unclassified pin");
  }
}

void buildLeaf(LeafArcsBuilder& graph, const LeafNodes& nodes, const SNLPath& path, SNLInstance* instance, Candidates& candidates, Coverage& coverage) {
  auto* model = instance->getModel();
  const SNLOccurrence occurrence(path, instance);
  if (NLDB0::isClockGate(model)) {
    auto pin = [&](SNLBitTerm* term, PinRole role) {
      auto id = nodes.at(instance->getInstTerm(term));
      graph.getNode(id)->setCellKind(CellKind::Combinational);
      graph.getNode(id)->setPinRole(role);
      return id;
    };
    auto clock = pin(NLDB0::getClockGateClock(), PinRole::Clock);
    auto enable = pin(NLDB0::getClockGateEnable(), PinRole::Enable);
    auto test = pin(NLDB0::getClockGateTestEnable(), PinRole::Enable);
    auto output = pin(NLDB0::getClockGateOutput(), PinRole::DataOutput);
    auto pass = graph.createEdge(clock, output,
      new TSNLEdgeProxy(TSNLEdgeProxy::Kind::ClockPass, occurrence));
    graph.getEdge(pass)->setKind(TEdge::Kind::ClockPass);
    candidates.canonicalGates.push_back({clock, enable, TGraph::kInvalidNode,
      TGraph::kInvalidNode, output, pass, test});
    return;
  }
  if (Model::hasMemoryInterface(model)) {
    buildMemory(graph, nodes, path, instance);
    return;
  }
  const bool sequential = Model::isSequential(model);
  SNLInstTerm* clock = nullptr;
  std::set<SNLBitTerm*> stateOutputs;
  if (sequential) {
    if (!Model::hasSequentialModel(model)) {
      fail(occurrence, "sequential cell has no explicit flip-flop/latch model");
    }
    const auto& seq = Model::getSequentialModel(instance);
    if (seq.kind == Model::SequentialModel::Kind::Latch) {
      if (NLDB0::isDLatch(model) && instance->getInstTerms().size() == 3) {
        try {
          // All recognition validation precedes graph mutation in buildLatch.
          buildLatch(graph, nodes, path, instance, seq, candidates);
          return;
        } catch (const NLException&) {
          // A valid latch can still be unsuitable for clock-gate recognition.
        }
      }
      buildPlainLatch(graph, nodes, path, instance, seq);
      coverage.record(occurrence, model, Reason::UnsupportedLatch, Treatment::Approximated,
        "latch modeled as level-sensitive state without time borrowing");
      return;
    }
    //A single term, optionally inverted, is supported. Composite clock
    //expressions cannot be faithfully represented as a single C -> Q arc.
    const auto& expression = seq.clockedOn;
    auto root = expression.root;
    std::set<size_t> visited;
    while (root < expression.nodes.size() && visited.insert(root).second) {
      const auto& node = expression.nodes[root];
      if (node.operation == Model::BooleanExpression::Operator::Term && node.term) {
        clock = instance->getInstTerm(node.term);
        break;
      }
      if (node.operation != Model::BooleanExpression::Operator::Not || node.operands.size() != 1) {
        break;
      }
      root = node.operands.front();
    }
    if (!clock || pinRole(clock, true) != PinRole::Clock) {
      fail(occurrence, "unsupported clock expression or missing clock role");
    }
    for (const auto& output : seq.outputs) {
      auto index = output.function.root;
      std::set<size_t> seen;
      bool stateOutput = false;
      while (index < output.function.nodes.size() && seen.insert(index).second) {
        const auto& node = output.function.nodes[index];
        if (node.operation == Model::BooleanExpression::Operator::State) {
          stateOutput = node.state < seq.states.size();
          break;
        }
        if (node.operation != Model::BooleanExpression::Operator::Not || node.operands.size() != 1) {
          break;
        }
        index = node.operands.front();
      }
      if (!stateOutput) {
        fail(occurrence, "mixed combinational/sequential output is unsupported");
      }
      stateOutputs.insert(output.term);
    }
  } else if (!Model::hasModeling(model) && Model::getTruthTableCount(model) == 0) {
    fail(occurrence, "leaf cell has no combinational modeling");
  }

  auto nodeId = [&](SNLInstTerm* term) { return nodes.at(term); };
  std::set<SNLInstTerm*> outputs;
  std::set<SNLInstTerm*> captures;
  for (auto* term : instance->getInstTerms()) {
    auto* node = graph.getNode(nodeId(term));
    node->setCellKind(sequential ? CellKind::FlipFlop : CellKind::Combinational);
    auto role = pinRole(term, sequential);
    if (role == PinRole::Unknown ||
        (term->getDirection() != SNLTerm::Direction::InOut &&
          (term->getDirection() == SNLTerm::Direction::Output) != (role == PinRole::DataOutput)) ||
        (role == PinRole::Clock && term != clock)) {
      fail(SNLOccurrence(path, term), "unsupported or inconsistent pin role");
    }
    node->setPinRole(role);
  }

  if (sequential) {
    for (auto* output : Model::getClockRelatedOutputs(clock)) {
      if (pinRole(output, true) != PinRole::DataOutput) {
        fail(occurrence, "clock-to-output arc targets a non-output role");
      }
      if (!stateOutputs.count(output->getBitTerm())) {
        fail(occurrence, "clock-to-output arc has no sequential state output");
      }
      auto stateEdge = graph.createEdge(nodeId(clock), nodeId(output),
        new TSNLEdgeProxy(TSNLEdgeProxy::Kind::ClockToOutput, occurrence));
      graph.getEdge(stateEdge)->setKind(TEdge::Kind::State);
      outputs.insert(output);
    }
    for (auto* input : Model::getClockRelatedInputs(clock)) {
      if (isCaptureRole(pinRole(input, true))) {
        graph.createCaptureEdge(nodeId(clock), nodeId(input));
        captures.insert(input);
      }
    }
    for (auto* term : instance->getInstTerms()) {
      const auto role = pinRole(term, true);
      if ((role == PinRole::DataOutput && !outputs.count(term)) ||
          (isCaptureRole(role) && !captures.count(term))) {
        fail(SNLOccurrence(path, term), "missing sequential timing relationship");
      }
    }
  } else {
    for (auto* output : instance->getInstTerms()) {
      if (output->getDirection() == SNLTerm::Direction::Input) {
        continue;
      }
      const auto table = Model::getTruthTable(instance, output->getBitTerm()->getOrderID());
      if (table.isInitialized() && table.size() == 0) {
        continue;
      }
      auto inputs = Model::getCombinatorialInputs(output);
      if (inputs.empty() && output->getDirection() == SNLTerm::Direction::InOut) continue;
      if (inputs.empty()) {
        fail(SNLOccurrence(path, output), "output has no dependency model");
      }
      std::vector<TGraph::NodeId> inputIds;
      std::vector<TGraph::EdgeId> arcs;
      for (auto* input : inputs) {
        if (input->getDirection() == SNLTerm::Direction::Output) {
          fail(occurrence, "combinational arc source is not an input");
        }
        inputIds.push_back(nodeId(input));
        arcs.push_back(graph.createEdge(nodeId(input), nodeId(output),
          new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Combinational, occurrence)));
        if (NLDB0::isMux2(model)) {
          graph.getEdge(arcs.back())->setKind(input->getBitTerm() == NLDB0::getMux2Select(model) ?
            TEdge::Kind::DataOnly : TEdge::Kind::ClockPass);
        }
      }
      if (NLDB0::isDB0Primitive(model)) {
        if (NLDB0::isGate(model) && table.isAnd() && table.size() == 2 && inputs.size() == 2) {
          candidates.conjunctions.push_back({{inputIds[0], inputIds[1]}, nodeId(output), {arcs[0], arcs[1]}});
        } else if (inputs.size() == 1 && table.size() == 1 &&
            (table.bits() == SNLTruthTable::Buf().bits() || table.bits() == SNLTruthTable::Inv().bits())) {
          candidates.transparent.push_back({inputIds[0], nodeId(output), table.bits() == SNLTruthTable::Inv().bits()});
        }
      }
    }
  }
}

// DNL supplies every terminal occurrence, including hierarchy boundaries.
// Partition bindings by physical net occurrence. Only hierarchical inouts
// join partitions into one equipotential; other boundaries retain their original
// connectivity and provenance. Never expand SNL hierarchy here.
class Connections {
  public:
    void add(naja::DNL::DNLID context, const SNLPath& path, SNLBitNet* net,
        TGraph::NodeId node, SNLTerm::Direction direction, bool internal, bool hierarchical = false) {
      if (!net) return;
      auto key = std::make_pair(context, net);
      auto [it, inserted] = indexes_.emplace(key, nets_.size());
      if (inserted) {
        roots_.push_back(nets_.size());
        nets_.push_back({SNLOccurrence(path, net), {}, {}, net->getType().isDriving()});
      }
      auto& endpoints = nets_[it->second];
      if (hierarchical) hierarchy_.insert(node);
      const bool input = direction == SNLTerm::Direction::Input;
      const bool inout = direction == SNLTerm::Direction::InOut;
      if (inout) inouts_.insert(node);
      if (inout && hierarchical) {
        boundaries_.insert(node);
        auto [boundary, first] = boundaryNets_.emplace(node, it->second);
        if (!first) {
          auto a = root(boundary->second), b = root(it->second);
          roots_[std::max(a, b)] = std::min(a, b);
        }
      }
      if ((internal ? input : !input) || inout) endpoints.drivers.push_back(node);
      if ((internal ? !input : input) || inout) endpoints.readers.push_back(node);
    }
    // Hierarchical inouts identify one equipotential across both bindings.
    // Merge these partitions before splitting, so no boundary traversal can
    // reflect a signal back into a net-only cycle. Other boundaries stay intact.
    void mergeInoutNets() {
      if (boundaryNets_.empty()) return;
      std::vector<Net> merged;
      std::unordered_map<size_t, size_t> indexes;
      std::vector<std::set<TGraph::NodeId>> drivers, readers;
      for (size_t i = 0; i < nets_.size(); ++i) {
        auto [it, first] = indexes.emplace(root(i), merged.size());
        if (first) {
          merged.push_back({nets_[i].occurrence, {}, {}, false});
          drivers.emplace_back(); readers.emplace_back();
        }
        auto& target = merged[it->second];
        target.constantDriver = target.constantDriver || nets_[i].constantDriver;
        for (auto id : nets_[i].drivers)
          if (drivers[it->second].insert(id).second) target.drivers.push_back(id);
        for (auto id : nets_[i].readers)
          if (readers[it->second].insert(id).second) target.readers.push_back(id);
      }
      nets_ = std::move(merged);
    }
    void markSplits(std::vector<bool>& split) const {
      for (const auto& net : nets_) {
        if (net.drivers.size() + net.constantDriver <= 1) continue;
        for (auto id : net.drivers) {
          if (inouts_.count(id)) split[id] = true;
        }
      }
    }
    void publish(TGraph& graph, Coverage& coverage, const std::vector<bool>& opaque,
        Constructor::ConstructionStats& stats) const {
      for (const auto& endpoints : nets_) {
        auto* net = static_cast<SNLBitNet*>(endpoints.occurrence.getObject());
        // A directional boundary can occur on both sides of a joined net.
        // These single-node aliases forward to real loads, never to one
        // another: alias-to-alias edges would recreate net-only cycles.
        std::set<TGraph::NodeId> aliases;
        for (auto id : endpoints.drivers) {
          if (hierarchy_.count(id) && !inouts_.count(id) &&
              std::find(endpoints.readers.begin(), endpoints.readers.end(), id) != endpoints.readers.end())
            aliases.insert(id);
        }
        size_t modeledDrivers = 0;
        for (auto id : endpoints.drivers) {
          if (opaque[id] || boundaries_.count(id) || aliases.count(id)) continue;
          const auto* proxy = static_cast<const TSNLNodeProxy*>(graph.getNode(id)->getProxy());
          auto* pin = dynamic_cast<SNLInstTerm*>(proxy->getOccurrence().getObject());
          // A sampled-only inout has no modeled output dependency. Its declared
          // drive role is retained, but does not establish a real driver conflict.
          if (pin && pin->getDirection() == SNLTerm::Direction::InOut &&
              graph.getNode(proxy->getDriverNode(id))->getInDegree() == 0 &&
              !Model::getTruthTable(pin->getInstance(), pin->getBitTerm()->getOrderID()).isInitialized()) continue;
          ++modeledDrivers;
        }
        if (modeledDrivers > 1 || (endpoints.constantDriver && modeledDrivers)) {
          coverage.record(endpoints.occurrence, net->getDesign(), Reason::MultipleDrivers,
            Treatment::AllDrivers, "multiple net drivers are unsupported");
        }
        size_t count = 0;
        for (auto pin : endpoints.drivers) {
          const auto driver = graph.getNode(pin)->getProxy()->getDriverNode(pin);
          for (auto reader : endpoints.readers) {
            if (pin == reader || (aliases.count(pin) && aliases.count(reader))) continue;
            ++count;
            graph.createEdge(driver, reader,
              new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Connectivity, endpoints.occurrence));
          }
        }
        stats.connectivityEdges += count;
        stats.largestNetEdges = std::max(stats.largestNetEdges, count);
        if (count > 1000000) ++stats.netsOverMillionEdges;
      }
    }
  private:
    struct Net {
      SNLOccurrence occurrence;
      std::vector<TGraph::NodeId> drivers;
      std::vector<TGraph::NodeId> readers;
      bool constantDriver;
    };
    size_t root(size_t index) {
      while (roots_[index] != index) {
        roots_[index] = roots_[roots_[index]];
        index = roots_[index];
      }
      return index;
    }
    std::vector<size_t> roots_;
    std::unordered_map<TGraph::NodeId, size_t> boundaryNets_;
    std::set<TGraph::NodeId> hierarchy_;
    std::set<TGraph::NodeId> boundaries_;
    std::set<TGraph::NodeId> inouts_;
    using Key = std::pair<naja::DNL::DNLID, SNLBitNet*>;
    struct Hash {
      size_t operator()(const Key& key) const {
        return std::hash<naja::DNL::DNLID>{}(key.first) ^ (std::hash<SNLBitNet*>{}(key.second) << 1);
      }
    };
    // Identity hashing is used only to find a net partition. The vector, in
    // first-terminal order, drives every emitted edge and coverage entry.
    std::unordered_map<Key, size_t, Hash> indexes_;
    std::vector<Net> nets_;
};

}  // namespace

std::string TSNLNodeProxy::getDescription() const {
  auto path = occurrence_.getPath().getString('.');
  if (!path.empty()) path += '.';
  auto* object = occurrence_.getObject();
  auto* pin = dynamic_cast<SNLInstTerm*>(object);
  auto name = pin ? pin->getInstance()->getString() + "." + pin->getBitTerm()->getString() :
    object ? object->getString() : "<null>";
  const auto suffix = getRole() == Role::Drive ? " (drive)" :
    getRole() == Role::Load ? " (load)" : " (single)";
  return path + name + suffix;
}

size_t TSNLGraphConstructor::OccurrenceHash::operator()(const naja::NL::SNLOccurrence& occurrence) const {
  if (!occurrence.getObject()) return 0;
  auto result = std::hash<SNLDesignObject*>{}(occurrence.getObject());
  for (auto* instance : occurrence.getPath().getInstances()) {
    result ^= std::hash<SNLInstance*>{}(instance) + 0x9e3779b9u + (result << 6) + (result >> 2);
  }
  return result;
}

void TSNLGraphConstructor::construct() {
  auto* universe = NLUniverse::get();
  auto* top = universe ? universe->getTopDesign() : nullptr;
  if (!top || top->isLeaf()) {
    throw NLException("TSNLGraphConstructor: expected a structural top design");
  }
  const auto* dnl = naja::DNL::get();
  if (dnl->getTopDesign() != top) {
    throw NLException("TSNLGraphConstructor: active DNL has a different top; rebuild DNL");
  }
  construct(*dnl);
}

void TSNLGraphConstructor::construct(const naja::DNL::DNLFull& dnl) {
  const auto* top = dnl.getTopDesign();
  if (!top || top->isLeaf()) {
    throw NLException("TSNLGraphConstructor: expected a structural top design");
  }
  const auto& instances = dnl.getDNLInstances();
  if (instances.size() < 2 || dnl.getDNLTerms().empty()) {
    throw NLException("TSNLGraphConstructor: expected a processed DNL");
  }
  auto graph = std::make_unique<TGraph>();
  decltype(nodes_) nodes;
  nodes.reserve(dnl.getNBterms());
  std::vector<TGraph::NodeId> terminalNodes(dnl.getNBterms(), TGraph::kInvalidNode);
  Candidates candidates;
  Coverage coverage;
  Connections connections;
  std::vector<SNLPath> paths(instances.size());
  // DNL orders parents before their children. Cache paths once rather than
  // asking terminal helpers to walk the global DNL for every occurrence.
  for (const auto& instance : instances) {
    if (instance.isNull()) continue;
    const auto id = instance.getID();
    auto* snl = instance.getSNLInstance();
    const auto parent = instance.getParentID();
    auto* model = snl ? snl->getModel() : top;
    if (snl) paths[id] = SNLPath(paths[parent], snl);
    const auto terms = instance.getTermIndexes();
    if (terms.first == naja::DNL::DNLID_MAX) continue;
    for (auto termId = terms.first; termId <= terms.second; ++termId) {
      const auto& term = dnl.getDNLTerminalFromID(termId);
      auto* bit = term.getSnlBitTerm();
      auto* pin = term.getSnlTerm();
      const auto direction = bit->getDirection();
      const auto occurrence = snl ? SNLOccurrence(paths[parent], pin) : SNLOccurrence(bit);
      const auto node = graph->createNode(new TSNLNodeProxy(occurrence));
      terminalNodes[termId] = node;
      nodes.emplace(occurrence, node);
      if (!snl || !model->isLeaf()) {
        checkDirection(occurrence, direction, model, coverage);
        graph->getNode(node)->setPinRole(direction == SNLTerm::Direction::Input ?
          PinRole::DataInput : PinRole::DataOutput);
        if (snl) nodes.emplace(SNLOccurrence(paths[id], bit), node);
        connections.add(id, paths[id], bit->getNet(), node, direction, true, snl != nullptr);
      }
      if (snl) connections.add(parent, paths[parent], pin->getNet(), node, direction, false, !model->isLeaf());
    }
  }
  std::vector<bool> split(graph->getNodesCount(), false);
  std::vector<bool> opaque(graph->getNodesCount(), false);
  connections.mergeInoutNets();
  connections.markSplits(split);
  std::vector<std::unique_ptr<LeafArcsBuilder>> leaves;
  std::vector<Candidates> leafCandidates;
  for (const auto& instance : instances) {
    if (instance.isNull() || instance.isTop()) continue;
    auto* snl = instance.getSNLInstance();
    auto* model = snl->getModel();
    if (!model->isLeaf()) continue;
    const auto& path = paths[instance.getParentID()];
    const LeafNodes leafNodes(instance.getTermIndexes().first, terminalNodes);
    auto leaf = std::make_unique<LeafArcsBuilder>(*graph);
    Candidates localCandidates;
    try {
      buildLeaf(*leaf, leafNodes, path, snl, localCandidates, coverage);
      for (auto* term : snl->getInstTerms()) checkDirection(SNLOccurrence(path, term), term->getDirection(), model, coverage);
      leaves.push_back(std::move(leaf));
      leafCandidates.push_back(std::move(localCandidates));
    } catch (const NLException& error) {
      size_t inouts = 0;
      for (auto* term : snl->getInstTerms()) {
        auto id = leafNodes.at(term);
        opaque[id] = true;
        if (term->getDirection() == SNLTerm::Direction::InOut) { split[id] = true; ++inouts; }
        graph->getNode(id)->setCellKind(CellKind::Unknown);
        graph->getNode(id)->setPinRole(pinRole(term, false));
      }
      std::string detail = error.what();
      auto reason = Reason::IncompleteModeling;
      if (Model::hasMemoryInterface(model)) reason = Reason::Memory;
      else if (detail.find("latch analysis") != std::string::npos) reason = Reason::UnsupportedLatch;
      else if (detail.find("terminal direction") != std::string::npos) reason = Reason::InOut;
      else if (detail.find("no combinational modeling") != std::string::npos) reason = Reason::Blackbox;
      if (inouts) reason = Reason::Blackbox;
      if (reason == Reason::Blackbox)
        detail += "; " + std::to_string(inouts) + " inout pins (load/drive roles)";
      coverage.record(SNLOccurrence(path, snl), model, reason, Treatment::Opaque, detail);
    }
  }
  ConstructionStats stats;
  // One decision per physical pin, shared by its external/internal aliases.
  // Append drive roles in original node order; existing single-node IDs remain stable.
  for (size_t id = 0; id < split.size(); ++id) {
    if (!split[id]) continue;
    auto* proxy = static_cast<TSNLNodeProxy*>(graph->getNode(id)->getProxy());
    auto* driveProxy = new TSNLNodeProxy(proxy->getOccurrence());
    driveProxy->setRole(TNodeProxy::Role::Drive);
    auto drive = graph->createNode(driveProxy);
    proxy->setRole(TNodeProxy::Role::Load, drive);
    auto* load = graph->getNode(id);
    // A sampled flop pin's drive side is not another state output or clock pin.
    if (load->getCellKind() == CellKind::Combinational || load->getPinRole() == PinRole::DataOutput)
      graph->getNode(drive)->setCellKind(load->getCellKind());
    graph->getNode(drive)->setPinRole(PinRole::DataOutput);
    if (load->getPinRole() == PinRole::DataOutput) {
      load->setPinRole(PinRole::Unknown);
      load->setCellKind(CellKind::Unknown);
    }
    ++stats.splitPins;
  }
  for (auto& entry : coverage.entries) {
    if (entry.reason != Reason::InOut) continue;
    auto id = nodes.at(entry.occurrence);
    const auto* proxy = graph->getNode(id)->getProxy();
    entry.detail = proxy->getDescription();
    if (proxy->getRole() == TNodeProxy::Role::Load)
      entry.detail += ", " + graph->getNode(proxy->getDriverNode(id))->getProxy()->getDescription();
  }
  for (size_t i = 0; i < leaves.size(); ++i) {
    auto& local = leafCandidates[i];
    leaves[i]->publish(local);
    candidates.canonicalGates.insert(candidates.canonicalGates.end(),
      local.canonicalGates.begin(), local.canonicalGates.end());
    candidates.latches.insert(candidates.latches.end(), local.latches.begin(), local.latches.end());
    candidates.conjunctions.insert(candidates.conjunctions.end(), local.conjunctions.begin(), local.conjunctions.end());
    candidates.transparent.insert(candidates.transparent.end(), local.transparent.begin(), local.transparent.end());
  }
  std::vector<ResetDomain> resets;
  // Bind declarations per DNL occurrence; shared models have independent seeds.
  // Several source terminals of one declaration share one clock-mask bit.
  for (const auto& instance : instances) {
    if (instance.isNull()) continue;
    auto* snl = instance.getSNLInstance();
    auto* model = snl ? snl->getModel() : top;
    const auto first = instance.getTermIndexes().first;
    std::vector<NLReset*> declarations;
    for (auto* reset : model->getResets()) {
      declarations.push_back(reset);
    }
    std::sort(declarations.begin(), declarations.end(), [](auto* a, auto* b) { return a->getName().getString() < b->getName().getString(); });
    for (const auto* reset: declarations) {
      ResetDomain domain {reset, {}};
      for (auto* source: reset->getSources()) {
        auto* term = static_cast<SNLBitTerm*>(source);
        auto node = terminalNodes.at(first + term->getOrderID());
        domain.sources.push_back(graph->getNode(node)->getProxy()->getDriverNode(node));
      }
      resets.push_back(std::move(domain));
    }
    for (const auto* clock : model->getClocks()) {
      std::vector<TGraph::NodeId> seeds;
      for (auto* source : clock->getSources()) {
        auto* term = dynamic_cast<SNLBitTerm*>(source);
        if (!term || term->getDesign() != model) {
          throw NLException("TSNLGraphConstructor: expected an NLClock source terminal of its owning design");
        }
        seeds.push_back(terminalNodes.at(first + term->getOrderID()));
      }
      graph->createClock(*clock, seeds);
    }
  }
  connections.publish(*graph, coverage, opaque, stats);
  auto gates = recognizeClockGates(*graph, candidates, coverage);
  gates.insert(gates.end(), candidates.canonicalGates.begin(), candidates.canonicalGates.end());
  for (const auto& gate : gates) {
    for (auto id : graph->getNode(gate.output)->getInEdges()) {
      graph->getEdge(id)->setKind(id == gate.clockPassEdge ? TEdge::Kind::ClockPass : TEdge::Kind::DataOnly);
    }
    static_cast<TSNLEdgeProxy*>(graph->getEdge(gate.clockPassEdge)->getProxy())->kind_ = TSNLEdgeProxy::Kind::ClockPass;
    graph->createCaptureEdge(gate.clock, gate.enable, TCaptureEdge::Kind::ClockGateControl);
    if (gate.testEnable != TGraph::kInvalidNode)
      graph->createCaptureEdge(gate.clock, gate.testEnable, TCaptureEdge::Kind::ClockGateControl);
  }
  CoverageCounts counts;
  for (const auto& entry : coverage.entries) ++counts[entry.reason];
  resets_ = std::move(resets);
  stats_ = stats;
  coverage_ = std::move(coverage.entries);
  coverageCounts_ = std::move(counts);
  graph_ = std::move(graph);
  clockGates_ = std::move(gates);
  nodes_ = std::move(nodes);
}

const std::vector<TSNLGraphConstructor::CoverageEntry>& TSNLGraphConstructor::getCoverageReport() const {
  getGraph();
  return coverage_;
}

const TSNLGraphConstructor::CoverageCounts& TSNLGraphConstructor::getCoverageCounts() const {
  getGraph();
  return coverageCounts_;
}

TGraph& TSNLGraphConstructor::getGraph() {
  if (!graph_) throw NLException("TSNLGraphConstructor: construct() has not succeeded");
  return *graph_;
}

const TGraph& TSNLGraphConstructor::getGraph() const {
  if (!graph_) throw NLException("TSNLGraphConstructor: construct() has not succeeded");
  return *graph_;
}

const std::vector<TSNLGraphConstructor::ClockGate>& TSNLGraphConstructor::getClockGates() const {
  if (!graph_) throw NLException("TSNLGraphConstructor: construct() has not succeeded");
  return clockGates_;
}

TGraph::NodeId TSNLGraphConstructor::getNode(const naja::NL::SNLOccurrence& occurrence) const {
  const auto it = nodes_.find(occurrence);
  return it == nodes_.end() ? TGraph::kInvalidNode : it->second;
}

TGraph::NodeId TSNLGraphConstructor::getDriverNode(const naja::NL::SNLOccurrence& occurrence) const {
  auto id = getNode(occurrence);
  return id == TGraph::kInvalidNode ? id : getGraph().getNode(id)->getProxy()->getDriverNode(id);
}

TGraph::ClockId TSNLGraphConstructor::createClock(const std::string& name,
    const naja::NL::SNLOccurrence& occurrence) {
  return getGraph().createClock(name, dynamic_cast<SNLNetComponent*>(occurrence.getObject()), getDriverNode(occurrence));
}

}  // namespace naja::TG
