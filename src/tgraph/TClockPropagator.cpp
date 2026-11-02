// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TClockPropagator.h"

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
    throw std::logic_error("TClockPropagator::run: enabled combinational cycle; no edges were disabled");
  }
  return order;
}

using Masks = std::vector<TEdge::ClockMask>;

void forward(const TGraph& graph, const std::vector<TGraph::NodeId>& order,
    Masks& masks, bool waveform) {
  for (auto n : order) {
    for (auto id : graph.getNode(n)->getOutEdges()) {
      const auto& edge = *graph.getEdge(id);
      if (traversable(graph, edge, waveform)) masks[edge.getTarget()] |= masks[n];
    }
  }
}

}  // namespace

TClockPropagator::Result TClockPropagator::run(TGraph& graph) {
  const TEdge::ClockMask empty(graph.getClocksCount());
  const auto size = graph.getNodesStorage().size();
  std::vector<bool> gateState(size, false);
  for (const auto& edge : graph.getEdgesStorage()) {
    if (!edge.isAlive()) continue;
    auto* live = graph.getEdge(edge.getIndex());
    live->setFromClocks(empty);
    live->setToClocks(empty);
    live->setClockGateControlClocks(empty);
    if (edge.getKind() == TEdge::Kind::State &&
        graph.getNode(edge.getSource())->isLatchGate() &&
        graph.getNode(edge.getTarget())->isLatchDataOutput()) {
      gateState[edge.getSource()] = gateState[edge.getTarget()] = true;
    }
  }
  for (const auto& capture : graph.getCaptureEdgesStorage()) {
    if (capture.isAlive() && capture.getKind() == TCaptureEdge::Kind::ClockGateControl) {
      gateState[capture.getTarget()] = true;
    }
  }
  for (const auto& node : graph.getNodesStorage()) {
    if (node.isAlive() && node.isLatch() && !gateState[node.getIndex()]) {
      throw std::logic_error("TClockPropagator::run: latch analysis is unsupported outside recognized clock gates");
    }
  }
  Masks origins(size, empty);
  for (const auto& clock : graph.getClocksStorage()) {
    if (!graph.isNodeValid(clock.getNode())) {
      throw std::logic_error("TClockPropagator::run: clock seed node was removed");
    }
    origins[clock.getNode()].set(clock.getIndex());
  }

  // Both views must be acyclic before publishing any propagated edge masks.
  const auto clockOrder = topologicalOrder(graph, true);
  const auto dataOrder = topologicalOrder(graph, false);
  forward(graph, clockOrder, origins, true);
  Masks launch(size, empty);
  for (const auto& clock : graph.getClocksStorage()) launch[clock.getNode()].set(clock.getIndex());
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
      if (!traversable(graph, *edge, false)) continue;
      edge->setToClocks(capture[n]);
      edge->setClockGateControlClocks(control[n]);
      capture[edge->getSource()] |= capture[n];
      control[edge->getSource()] |= control[n];
    }
  }
  for (const auto& edge : graph.getEdgesStorage()) {
    if (edge.isAlive() && !edge.isDisabled()) {
      graph.getEdge(edge.getIndex())->setFromClocks(launch[edge.getSource()]);
    }
  }
  return {std::move(origins)};
}

}  // namespace naja::TG
