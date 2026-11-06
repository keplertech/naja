// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TClockPropagator.h"

#include <deque>
#include <limits>
#include <stdexcept>
#include <vector>

#include "TGraph.h"

namespace naja::TG {
namespace {

// Preserve the original graph-level convention for unclassified flop C->Q
// arcs. Builders explicitly mark state arcs, including recognized gate latches.
bool isState(const TGraph& graph, const TEdge& edge) {
  return edge.getKind() == TEdge::Kind::State ||
    graph.getNode(edge.getTarget())->isFlopDataOutput();
}

bool traversable(const TGraph& graph, const TEdge& edge, bool waveform) {
  return edge.isAlive() && !edge.isDisabled() && !isState(graph, edge) &&
    (!waveform || edge.getKind() != TEdge::Kind::DataOnly);
}

std::vector<TGraph::NodeId> topologicalOrder(const TGraph& graph, bool waveform) {
  std::vector<std::size_t> degree(graph.getNodesStorage().size(), 0);
  for (const auto& edge : graph.getEdgesStorage()) {
    if (traversable(graph, edge, waveform)) ++degree[edge.getTarget()];
  }
  std::vector<TGraph::NodeId> order;
  order.reserve(graph.getNodesCount());
  for (const auto& node : graph.getNodesStorage()) {
    if (node.isAlive() && degree[node.getIndex()] == 0) order.push_back(node.getIndex());
  }
  for (std::size_t i = 0; i < order.size(); ++i) {
    for (auto id : graph.getNode(order[i])->getOutEdges()) {
      const auto& edge = *graph.getEdge(id);
      if (traversable(graph, edge, waveform) && --degree[edge.getTarget()] == 0) {
        order.push_back(edge.getTarget());
      }
    }
  }
  if (order.size() != graph.getNodesCount()) {
    // Iterative DFS on Kahn's residual finds a node on an actual cycle.
    // A residual node alone may merely be downstream of a rejected SCC.
    std::vector<unsigned char> color(degree.size(), 0);
    std::vector<std::pair<TGraph::NodeId, size_t>> stack;
    for (const auto& node : graph.getNodesStorage()) {
      auto start = node.getIndex();
      if (!node.isAlive() || !degree[start] || color[start]) continue;
      stack.emplace_back(start, 0);
      color[start] = 1;
      while (!stack.empty()) {
        auto& [source, next] = stack.back();
        const auto& edges = graph.getNode(source)->getOutEdges();
        if (next == edges.size()) {
          color[source] = 2;
          stack.pop_back();
          continue;
        }
        const auto& edge = *graph.getEdge(edges[next++]);
        auto target = edge.getTarget();
        if (!traversable(graph, edge, waveform) || !degree[target]) continue;
        if (color[target] == 1) {
          const auto* proxy = graph.getNode(target)->getProxy();
          auto description = proxy ? proxy->getDescription() : std::string{};
          if (description.empty()) description = "node " + std::to_string(target);
          throw TClockPropagator::CycleError(target, description,
            proxy ? proxy->getRole() : TNodeProxy::Role::Single);
        }
        if (!color[target]) {
          color[target] = 1;
          stack.emplace_back(target, 0);
        }
      }
    }
    throw std::logic_error("TClockPropagator::run: inconsistent cycle traversal");
  }
  return order;
}

using Masks = std::vector<TEdge::ClockMask>;

void forward(const TGraph& graph, const std::vector<TGraph::NodeId>& order,
    Masks& masks, bool waveform) {
  for (auto n : order) {
    for (auto id : graph.getNode(n)->getOutEdges()) {
      const auto& edge = *graph.getEdge(id);
      if (traversable(graph, edge, waveform) && (waveform || !edge.isDataDisabled())) {
        masks[edge.getTarget()] |= masks[n];
      }
    }
  }
}

// Summarize the nearest waveform sources without a set per node. Explicit
// seeds and classified ClockPass outputs take precedence over unclassified
// flop Q inputs. ClockPass explicitly admits each classifiable input; otherwise
// only a sole Q source is classifiable. Distinct Qs are ambiguous.
// This join is associative, so neither node nor edge creation order matters.
void generatedWaveforms(const TGraph& graph,
    const std::vector<TGraph::NodeId>& order,
    const std::vector<TGraph::NodeId>& dataOrder, Masks& origins,
    std::vector<TEdge::EdgeId>& generated, std::vector<TEdge::EdgeId>& controls) {
  using NodeId = TGraph::NodeId;
  constexpr auto none = std::numeric_limits<NodeId>::max();
  constexpr auto primary = none - 1;
  constexpr auto ambiguous = none - 2;
  std::vector<NodeId> source(origins.size(), none);
  std::vector<bool> needed(origins.size(), false);
  for (const auto& node : graph.getNodesStorage()) {
    if (!node.isAlive()) continue;
    auto n = node.getIndex();
    if (node.isFlopDataOutput()) source[n] = n;
    if (origins[n].any()) source[n] = primary;
    needed[n] = node.isFlopClock();
  }
  for (const auto& capture : graph.getCaptureEdgesStorage()) {
    if (capture.isAlive()) needed[capture.getSource()] = true;
  }
  for (const auto& edge : graph.getEdgesStorage()) {
    if (edge.isAlive() && !edge.isDisabled() && isState(graph, edge)) {
      needed[edge.getSource()] = true;
    }
  }
  auto passes = [&](const TEdge& edge) {
    const auto incoming = source[edge.getSource()];
    return edge.getKind() == TEdge::Kind::ClockPass &&
      incoming != none && incoming != ambiguous;
  };
  for (auto n : order) {
    for (auto id : graph.getNode(n)->getOutEdges()) {
      const auto& edge = *graph.getEdge(id);
      if (!traversable(graph, edge, true)) continue;
      auto& target = source[edge.getTarget()];
      const auto incoming = passes(edge) ? primary : source[n];
      if (target == primary || incoming == none) continue;
      if (incoming == primary || target == none) target = incoming;
      else if (target != incoming) target = ambiguous;
    }
  }
  // Follow accepted waveform paths and rejected control paths separately.
  // The latter includes DataOnly dependencies, but stops at state outputs.
  auto clockPath = needed;
  std::vector<bool> control(origins.size(), false);
  auto accepts = [&](const TEdge& edge) {
    const auto from = source[edge.getSource()];
    const auto to = source[edge.getTarget()];
    return traversable(graph, edge, true) &&
      (passes(edge) || (from == to && to != none && to != ambiguous));
  };
  // Data order is also a valid waveform order, and includes DataOnly inputs.
  for (auto it = dataOrder.rbegin(); it != dataOrder.rend(); ++it) {
    auto n = *it;
    if (!needed[n] && !clockPath[n] && !control[n]) continue;
    for (auto id : graph.getNode(n)->getInEdges()) {
      const auto& edge = *graph.getEdge(id);
      if (!traversable(graph, edge, false)) continue;
      auto from = edge.getSource();
      const bool accepted = accepts(edge);
      if (needed[n] && accepted) needed[from] = true;
      if (clockPath[n]) {
        clockPath[from] = true;
        if (edge.getKind() == TEdge::Kind::DataOnly ||
            (source[n] == primary && !accepted)) control[from] = true;
      }
      if (control[n]) control[from] = true;
    }
  }
  std::vector<bool> admitted(graph.getEdgesStorage().size(), false);
  for (const auto& edge : graph.getEdgesStorage()) {
    if (!edge.isAlive() || edge.isDisabled()) continue;
    const auto from = edge.getSource();
    const auto to = edge.getTarget();
    if (isState(graph, edge)) {
      if (graph.getNode(to)->isFlopDataOutput() && control[to]) controls.push_back(edge.getIndex());
      if (needed[to] && graph.getNode(to)->isFlopDataOutput() && source[to] == to) {
        admitted[edge.getIndex()] = true;
        generated.push_back(edge.getIndex());
      }
    } else if (accepts(edge) && (source[from] == primary || needed[to])) {
      admitted[edge.getIndex()] = true;
    }
  }
  // State feedback may form waveform cycles. A monotone worklist computes the
  // least union fixed point; each clock bit can enter each node only once.
  // No recursion, graph mutation, or repeated whole-graph scans are needed.
  std::deque<NodeId> pending;
  std::vector<bool> queued(origins.size(), false);
  for (const auto& clock : graph.getClocksStorage()) {
    for (const auto& seed : clock.getSeeds()) {
      auto n = seed.node;
      if (!queued[n]) { pending.push_back(n); queued[n] = true; }
    }
  }
  while (!pending.empty()) {
    auto n = pending.front();
    pending.pop_front();
    queued[n] = false;
    for (auto id : graph.getNode(n)->getOutEdges()) {
      if (!admitted[id]) continue;
      auto to = graph.getEdge(id)->getTarget();
      if (origins[n].is_subset_of(origins[to])) continue;
      origins[to] |= origins[n];
      if (!queued[to]) { pending.push_back(to); queued[to] = true; }
    }
  }
}

}  // namespace

TClockPropagator::Result TClockPropagator::run(TGraph& graph) {
  const TEdge::ClockMask empty(graph.getClocksCount());
  const auto size = graph.getNodesStorage().size();
  for (const auto& edge : graph.getEdgesStorage()) {
    if (!edge.isAlive()) continue;
    auto* live = graph.getEdge(edge.getIndex());
    live->setFromClocks(empty);
    live->setToClocks(empty);
    live->setClockGateControlClocks(empty);
  }
  Masks origins(size, empty);
  for (const auto& clock : graph.getClocksStorage()) {
    for (const auto& seed : clock.getSeeds()) {
      if (!graph.isNodeValid(seed.node)) {
        throw std::logic_error("TClockPropagator::run: clock seed node was removed");
      }
      origins[seed.node].set(clock.getIndex());
    }
  }

  // Reject combinational cycles before publishing propagated edge masks.
  // Admitted State feedback is handled separately by the waveform worklist.
  const auto clockOrder = topologicalOrder(graph, true);
  const auto dataOrder = topologicalOrder(graph, false);
  Result result;
  generatedWaveforms(graph, clockOrder, dataOrder, origins,
    result.generatedClockStateArcs, result.controlStateArcs);
  for (const auto& node : graph.getNodesStorage()) {
    if (node.isAlive() && node.isFlopClock() && origins[node.getIndex()].none()) {
      result.unclockedFlopClockPins.push_back(node.getIndex());
    }
  }
  Masks launch(size, empty);
  for (const auto& clock : graph.getClocksStorage()) {
    for (const auto& seed : clock.getSeeds()) launch[seed.node].set(clock.getIndex());
  }
  for (const auto& edge : graph.getEdgesStorage()) {
    if (edge.isAlive() && !edge.isDisabled() && isState(graph, edge)) {
      launch[edge.getTarget()] |= origins[edge.getSource()];
    }
  }
  forward(graph, dataOrder, launch, false);

  Masks capture(size, empty);
  Masks control(size, empty);
  for (const auto& edge : graph.getCaptureEdgesStorage()) {
    if (!edge.isAlive()) continue;
    auto& masks = edge.getKind() == TCaptureEdge::Kind::ClockGateControl ? control : capture;
    masks[edge.getTarget()] |= origins[edge.getSource()];
  }
  for (auto it = dataOrder.rbegin(); it != dataOrder.rend(); ++it) {
    const auto n = *it;
    if (graph.getNode(n)->isFlopDataOutput() || graph.getNode(n)->isLatchDataOutput()) continue;
    for (auto id : graph.getNode(n)->getInEdges()) {
      auto* edge = graph.getEdge(id);
      if (!traversable(graph, *edge, false) || edge->isDataDisabled()) {
        continue;
      }
      edge->setToClocks(capture[n]);
      edge->setClockGateControlClocks(control[n]);
      capture[edge->getSource()] |= capture[n];
      control[edge->getSource()] |= control[n];
    }
  }
  for (const auto& edge : graph.getEdgesStorage()) {
    if (edge.isAlive() && !edge.isDisabled() && !edge.isDataDisabled()) {
      graph.getEdge(edge.getIndex())->setFromClocks(launch[edge.getSource()]);
    }
  }
  result.clockOrigins = std::move(origins);
  return result;
}

}  // namespace naja::TG
