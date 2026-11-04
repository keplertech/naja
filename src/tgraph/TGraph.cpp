// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TGraph.h"

#include <stdexcept>
#include <vector>

#include "TEdge.h"
#include "TNode.h"
#include "TNodeProxy.h"

namespace naja::TG {

TGraph::NodeId TGraph::createNode(TNodeProxy* proxy) {
  NodeId id = kInvalidNode;
  if (!freeNodes_.empty()) {
    id = freeNodes_.back();
    freeNodes_.pop_back();
    nodes_[id].reset(id, proxy);
  } else {
    id = nodes_.size();
    nodes_.emplace_back(TNode::create(id, proxy));
  }
  ++liveNodes_;
  return id;
}

TGraph::EdgeId TGraph::createEdge(NodeId source, NodeId target, TEdgeProxy* proxy) {
  if (!isNodeValid(source) || !isNodeValid(target)) {
    return kInvalidEdge;
  }

  EdgeId id = kInvalidEdge;
  if (!freeEdges_.empty()) {
    id = freeEdges_.back();
    freeEdges_.pop_back();
    edges_[id].reset(id, source, target, proxy);
  } else {
    id = edges_.size();
    edges_.emplace_back(TEdge::create(id, source, target, proxy));
  }

  edges_[id].resizeClockMasks(clocks_.size());

  nodes_[source].addOutEdge(id);
  nodes_[target].addInEdge(id);
  ++liveEdges_;
  return id;
}

TGraph::CaptureEdgeId TGraph::createCaptureEdge(NodeId source, NodeId target, TCaptureEdge::Kind kind) {
  if (!isNodeValid(source) || !isNodeValid(target)) {
    return kInvalidCaptureEdge;
  }
  CaptureEdgeId id = kInvalidCaptureEdge;
  if (!freeCaptureEdges_.empty()) {
    id = freeCaptureEdges_.back();
    freeCaptureEdges_.pop_back();
    captureEdges_[id].reset(id, source, target, kind);
  } else {
    id = captureEdges_.size();
    captureEdges_.push_back(TCaptureEdge(id, source, target, kind));
  }
  nodes_[source].addCaptureOutEdge(id);
  nodes_[target].addCaptureInEdge(id);
  ++liveCaptureEdges_;
  return id;
}

bool TGraph::removeCaptureEdge(CaptureEdgeId edge) {
  if (!isCaptureEdgeValid(edge)) {
    return false;
  }
  auto& edgeRef = captureEdges_[edge];
  nodes_[edgeRef.getSource()].removeCaptureOutEdge(edge);
  nodes_[edgeRef.getTarget()].removeCaptureInEdge(edge);
  edgeRef.deactivate();
  freeCaptureEdges_.push_back(edge);
  --liveCaptureEdges_;
  return true;
}

TCaptureEdge* TGraph::getCaptureEdge(CaptureEdgeId edge) {
  return isCaptureEdgeValid(edge) ? &captureEdges_[edge] : nullptr;
}

const TCaptureEdge* TGraph::getCaptureEdge(CaptureEdgeId edge) const {
  return isCaptureEdgeValid(edge) ? &captureEdges_[edge] : nullptr;
}

bool TGraph::isCaptureEdgeValid(CaptureEdgeId edge) const {
  return edge < captureEdges_.size() && captureEdges_[edge].isAlive();
}

bool TGraph::removeEdge(EdgeId edge) {
  if (!isEdgeValid(edge)) {
    return false;
  }

  TEdge& edgeRef = edges_[edge];
  const NodeId source = edgeRef.getSource();
  const NodeId target = edgeRef.getTarget();
  if (isNodeValid(source)) {
    nodes_[source].removeOutEdge(edge);
  }
  if (isNodeValid(target)) {
    nodes_[target].removeInEdge(edge);
  }

  edgeRef.deactivate();
  freeEdges_.push_back(edge);
  --liveEdges_;
  return true;
}

bool TGraph::removeNode(NodeId node) {
  if (!isNodeValid(node)) {
    return false;
  }

  TNode& nodeRef = nodes_[node];
  const std::vector<EdgeId> outEdges = nodeRef.getOutEdges();
  const std::vector<EdgeId> inEdges = nodeRef.getInEdges();
  const auto captureOutEdges = nodeRef.getCaptureOutEdges();
  const auto captureInEdges = nodeRef.getCaptureInEdges();
  for (CaptureEdgeId edge : captureOutEdges) {
    removeCaptureEdge(edge);
  }
  for (CaptureEdgeId edge : captureInEdges) {
    removeCaptureEdge(edge);
  }
  for (EdgeId edge : outEdges) {
    removeEdge(edge);
  }
  for (EdgeId edge : inEdges) {
    removeEdge(edge);
  }

  nodeRef.deactivate();
  freeNodes_.push_back(node);
  --liveNodes_;
  return true;
}

TNode* TGraph::getNode(NodeId node) {
  if (!isNodeValid(node)) {
    return nullptr;
  }
  return &nodes_[node];
}

const TNode* TGraph::getNode(NodeId node) const {
  if (!isNodeValid(node)) {
    return nullptr;
  }
  return &nodes_[node];
}

TEdge* TGraph::getEdge(EdgeId edge) {
  if (!isEdgeValid(edge)) {
    return nullptr;
  }
  return &edges_[edge];
}

const TEdge* TGraph::getEdge(EdgeId edge) const {
  if (!isEdgeValid(edge)) {
    return nullptr;
  }
  return &edges_[edge];
}

bool TGraph::isNodeValid(NodeId node) const {
  return node < nodes_.size() && nodes_[node].isAlive();
}

bool TGraph::isEdgeValid(EdgeId edge) const {
  return edge < edges_.size() && edges_[edge].isAlive();
}

TGraph::ClockId TGraph::createClock(const std::string& name, naja::NL::SNLNetComponent* target, NodeId node) {
  if (!isNodeValid(node)) {
    throw std::invalid_argument("TGraph::createClock: node is not a valid node");
  }
  if (const auto* proxy = getNode(node)->getProxy()) node = proxy->getDriverNode(node);
  if (!isNodeValid(node)) throw std::invalid_argument("TGraph::createClock: invalid paired driver");
  ClockId id = clocks_.size();
  clocks_.emplace_back(id, name, target, node);
  //Keep every live edge's masks sized to the new clock count, so all edges
  //always carry same-size FromClocks/ToClocks masks.
  for (TEdge& edge : edges_) {
    if (edge.isAlive()) {
      edge.resizeClockMasks(clocks_.size());
    }
  }
  return id;
}

TGraph::ClockId TGraph::createClock(const naja::NL::NLClock& clock, const std::vector<NodeId>& nodes) {
  auto seeds = nodes;
  for (auto& node : seeds) {
    if (!isNodeValid(node)) {
      throw std::invalid_argument("TGraph::createClock: node is not a valid node");
    }
    if (const auto* proxy = getNode(node)->getProxy()) node = proxy->getDriverNode(node);
    if (!isNodeValid(node)) throw std::invalid_argument("TGraph::createClock: invalid paired driver");
  }
  ClockId id = clocks_.size();
  clocks_.emplace_back(id, clock, seeds);
  for (TEdge& edge : edges_) {
    if (edge.isAlive()) edge.resizeClockMasks(clocks_.size());
  }
  return id;
}

const TClock* TGraph::getClock(ClockId clock) const {
  if (!isClockValid(clock)) {
    return nullptr;
  }
  return &clocks_[clock];
}

}  // namespace naja::TG
