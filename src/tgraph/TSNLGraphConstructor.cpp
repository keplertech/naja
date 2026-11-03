// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TSNLGraphConstructor.h"

#include <set>
#include <utility>
#include <vector>

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
using NodeMap = std::map<SNLOccurrence, TGraph::NodeId>;

[[noreturn]] void fail(const SNLOccurrence& occurrence, const std::string& reason) {
  throw NLException("TSNLGraphConstructor: " + reason + " at " + occurrence.getDescription());
}

void checkDirection(const SNLOccurrence& occurrence, SNLTerm::Direction direction) {
  if (direction != SNLTerm::Direction::Input && direction != SNLTerm::Direction::Output) {
    fail(occurrence, "unsupported terminal direction");
  }
}

TGraph::NodeId addNode(TGraph& graph, NodeMap& nodes, const SNLOccurrence& occurrence) {
  auto id = graph.createNode(new TSNLNodeProxy(occurrence));
  nodes.emplace(occurrence, id);
  return id;
}

PinRole pinRole(SNLInstTerm* term, bool sequential) {
  if (!sequential) {
    return term->getDirection() == SNLTerm::Direction::Input ? PinRole::DataInput : PinRole::DataOutput;
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
  std::vector<Latch> latches;
  std::vector<AndGate> conjunctions;
  std::vector<Transparent> transparent;
};

void buildLatch(TGraph& graph, const NodeMap& nodes, const SNLPath& path,
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
  auto nodeId = [&](SNLBitTerm* term) { return nodes.at(SNLOccurrence(path, instance->getInstTerm(term))); };
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
    const TGraph& graph, const Candidates& candidates) {
  if (candidates.latches.empty()) return {};
  const auto size = graph.getNodesStorage().size();
  struct Origin { TGraph::NodeId node {TGraph::kInvalidNode}; bool inverted {false}; };
  std::vector<Origin> predecessor(size), origin(size);
  for (const auto& edge : graph.getEdgesStorage()) {
    const auto* proxy = static_cast<const TSNLEdgeProxy*>(edge.getProxy());
    if (proxy->getKind() == TSNLEdgeProxy::Kind::Connectivity) {
      auto& previous = predecessor[edge.getTarget()];
      if (previous.node != TGraph::kInvalidNode) {
        throw NLException("TSNLGraphConstructor: ambiguous connectivity during clock-gate recognition");
      }
      previous.node = edge.getSource();
    }
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
        origin[current] = {current, false};
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
      matched[index] = true;
      result.push_back({gate.inputs[1 - held], latch.data, latch.output, latch.gate,
        gate.output, gate.arcs[1 - held]});
    }
    if (result.size() > firstMatch + 1) {
      const auto* proxy = static_cast<const TSNLEdgeProxy*>(graph.getEdge(gate.arcs[0])->getProxy());
      fail(proxy->getOccurrence(), "ambiguous clock-gate composition: both AND inputs match held states");
    }
  }
  for (size_t i = 0; i < matched.size(); ++i) {
    if (!matched[i]) fail(candidates.latches[i].occurrence,
      "latch analysis is unsupported: no matching low-phase DB0 latch-and-AND clock gate");
  }
  return result;
}

void buildLeaf(TGraph& graph, NodeMap& nodes, const SNLPath& path, SNLInstance* instance, Candidates& candidates) {
  auto* model = instance->getModel();
  const SNLOccurrence occurrence(path, instance);
  if (Model::hasMemoryInterface(model)) {
    fail(occurrence, "memory analysis is unsupported");
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
      buildLatch(graph, nodes, path, instance, seq, candidates);
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

  auto nodeId = [&](SNLInstTerm* term) { return nodes.at(SNLOccurrence(path, term)); };
  std::set<SNLInstTerm*> outputs;
  std::set<SNLInstTerm*> captures;
  for (auto* term : instance->getInstTerms()) {
    auto* node = graph.getNode(nodeId(term));
    node->setCellKind(sequential ? CellKind::FlipFlop : CellKind::Combinational);
    auto role = pinRole(term, sequential);
    if (role == PinRole::Unknown ||
        (term->getDirection() == SNLTerm::Direction::Output) != (role == PinRole::DataOutput) ||
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
      if (output->getDirection() != SNLTerm::Direction::Output) {
        continue;
      }
      const auto table = Model::getTruthTable(instance, output->getBitTerm()->getOrderID());
      if (table.isInitialized() && table.size() == 0) {
        continue;
      }
      auto inputs = Model::getCombinatorialInputs(output);
      if (inputs.empty()) {
        fail(SNLOccurrence(path, output), "output has no dependency model");
      }
      std::vector<TGraph::NodeId> inputIds;
      std::vector<TGraph::EdgeId> arcs;
      for (auto* input : inputs) {
        if (input->getDirection() != SNLTerm::Direction::Input) {
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

void buildNets(TGraph& graph, const NodeMap& nodes, SNLDesign* design, const SNLPath& path) {
  for (auto* net : design->getBitNets()) {
    std::vector<TGraph::NodeId> drivers;
    std::vector<TGraph::NodeId> readers;
    for (auto* term : net->getBitTerms()) {
      auto& endpoints = term->getDirection() == SNLTerm::Direction::Input ? drivers : readers;
      endpoints.push_back(nodes.at(SNLOccurrence(path, term)));
    }
    for (auto* term : net->getInstTerms()) {
      auto& endpoints = term->getDirection() == SNLTerm::Direction::Output ? drivers : readers;
      endpoints.push_back(nodes.at(SNLOccurrence(path, term)));
    }
    if (drivers.size() > 1 || (net->getType().isDriving() && !drivers.empty())) {
      fail(SNLOccurrence(path, net), "multiple net drivers are unsupported");
    }
    for (auto driver : drivers) {
      for (auto reader : readers) {
        graph.createEdge(driver, reader,
          new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Connectivity, SNLOccurrence(path, net)));
      }
    }
  }
}

}  // namespace

void TSNLGraphConstructor::construct() {
  if (!top_ || top_->isLeaf()) {
    throw NLException("TSNLGraphConstructor: expected a structural top design");
  }
  auto graph = std::make_unique<TGraph>();
  NodeMap nodes;
  Candidates candidates;
  for (auto* term : top_->getBitTerms()) {
    checkDirection(SNLOccurrence(term), term->getDirection());
    auto id = addNode(*graph, nodes, SNLOccurrence(term));
    graph->getNode(id)->setPinRole(term->getDirection() == SNLTerm::Direction::Input ?
      PinRole::DataInput : PinRole::DataOutput);
  }
  struct Context { SNLDesign* design; SNLPath path; };
  std::vector<Context> pending {{top_, SNLPath()}};
  while (!pending.empty()) {
    auto context = pending.back();
    pending.pop_back();
    for (auto* instance : context.design->getInstances()) {
      auto* model = instance->getModel();
      if (model == top_) {
        fail(SNLOccurrence(context.path, instance), "recursive hierarchy");
      }
      for (auto* ancestor : context.path.getInstances()) {
        if (ancestor->getModel() == model) {
          fail(SNLOccurrence(context.path, instance), "recursive hierarchy");
        }
      }
      SNLPath childPath(context.path, instance);
      for (auto* term : instance->getInstTerms()) {
        SNLOccurrence occurrence(context.path, term);
        checkDirection(occurrence, term->getDirection());
        auto id = addNode(*graph, nodes, occurrence);
        if (!model->isLeaf()) {
          graph->getNode(id)->setPinRole(term->getDirection() == SNLTerm::Direction::Input ?
            PinRole::DataInput : PinRole::DataOutput);
          nodes.emplace(SNLOccurrence(childPath, term->getBitTerm()), id);
        }
      }
      if (model->isLeaf()) {
        buildLeaf(*graph, nodes, context.path, instance, candidates);
      } else {
        pending.push_back({model, childPath});
      }
    }
    buildNets(*graph, nodes, context.design, context.path);
  }
  auto gates = recognizeClockGates(*graph, candidates);
  for (const auto& gate : gates) {
    for (auto id : graph->getNode(gate.output)->getInEdges()) {
      graph->getEdge(id)->setKind(id == gate.clockPassEdge ? TEdge::Kind::ClockPass : TEdge::Kind::DataOnly);
    }
    static_cast<TSNLEdgeProxy*>(graph->getEdge(gate.clockPassEdge)->getProxy())->kind_ = TSNLEdgeProxy::Kind::ClockPass;
    graph->createCaptureEdge(gate.clock, gate.enable, TCaptureEdge::Kind::ClockGateControl);
  }
  graph_ = std::move(graph);
  clockGates_ = std::move(gates);
  nodes_ = std::move(nodes);
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

}  // namespace naja::TG
