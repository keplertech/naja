// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "TClock.h"
#include "TCaptureEdge.h"
#include "TEdge.h"
#include "TNode.h"

namespace naja::NL {

class SNLNetComponent;

}  // namespace naja::NL

namespace naja::TG {

class TEdgeProxy;
class TNodeProxy;

class TGraph {
  public:
    using NodeId = std::size_t;
    using EdgeId = std::size_t;
    using ClockId = TClock::ClockId;
    using CaptureEdgeId = TCaptureEdge::CaptureEdgeId;

    static constexpr NodeId kInvalidNode = static_cast<NodeId>(-1);
    static constexpr EdgeId kInvalidEdge = static_cast<EdgeId>(-1);
    static constexpr CaptureEdgeId kInvalidCaptureEdge = static_cast<CaptureEdgeId>(-1);

    TGraph() = default;
    ~TGraph() = default;

    TGraph(const TGraph&) = delete;
    TGraph& operator=(const TGraph&) = delete;
    TGraph(TGraph&&) = delete;
    TGraph& operator=(TGraph&&) = delete;

    NodeId createNode(TNodeProxy* proxy);
    EdgeId createEdge(NodeId source, NodeId target, TEdgeProxy* proxy);

    /// Creates a separate clock-to-capture relationship; returns
    /// kInvalidCaptureEdge for invalid nodes. Endpoint roles and the capture kind
    /// are assigned by the builder; gate controls are distinct from data captures.
    CaptureEdgeId createCaptureEdge(NodeId source, NodeId target,
      TCaptureEdge::Kind kind = TCaptureEdge::Kind::Data);
    bool removeCaptureEdge(CaptureEdgeId edge);
    /// \warning transient view, invalidated by capture storage reallocation.
    /// CaptureEdgeId belongs to a separate ID space from ordinary EdgeId.
    TCaptureEdge* getCaptureEdge(CaptureEdgeId edge);
    const TCaptureEdge* getCaptureEdge(CaptureEdgeId edge) const;
    bool isCaptureEdgeValid(CaptureEdgeId edge) const;
    std::size_t getCaptureEdgesCount() const { return liveCaptureEdges_; }
    const std::vector<TCaptureEdge>& getCaptureEdgesStorage() const { return captureEdges_; }

    bool removeNode(NodeId node);
    bool removeEdge(EdgeId edge);

    /// \warning the returned pointer is a transient view into vector-backed
    /// storage: it is invalidated by any later createNode()/createEdge()
    /// call that reallocates that storage. NodeId is the stable handle;
    /// re-fetch getNode() after any graph mutation rather than caching it.
    TNode* getNode(NodeId node);
    const TNode* getNode(NodeId node) const;

    /// \warning see getNode()'s pointer-invalidation warning; EdgeId is the
    /// stable handle, not the returned pointer.
    TEdge* getEdge(EdgeId edge);
    const TEdge* getEdge(EdgeId edge) const;

    std::size_t getNodesCount() const { return liveNodes_; }
    std::size_t getEdgesCount() const { return liveEdges_; }

    const std::vector<TNode>& getNodesStorage() const { return nodes_; }
    const std::vector<TEdge>& getEdgesStorage() const { return edges_; }

    bool isNodeValid(NodeId node) const;
    bool isEdgeValid(EdgeId edge) const;

    /// \brief register a clock reaching `target` (a top terminal or an
    /// instance terminal), seeded at `node` - the graph node TClockPropagator
    /// starts its forward pass from for this clock. The number of clocks is
    /// unbounded. Grows every live edge's clock masks to
    /// match, so all edges always carry same-size masks, sized to
    /// getClocksCount().
    /// \throws std::invalid_argument if `node` is not a valid node.
    ClockId createClock(const std::string& name, naja::NL::SNLNetComponent* target, NodeId node);
    /// One clock identity seeded at all declaration sources, in source order.
    /// NLClock is referenced, never modified, and must outlive the graph.
    /// Throws for missing/invalid seed nodes. Does not infer clock relations.
    ClockId createClock(const naja::NL::NLClock& clock, const std::vector<NodeId>& nodes);

    const TClock* getClock(ClockId clock) const;
    std::size_t getClocksCount() const { return clocks_.size(); }
    const std::vector<TClock>& getClocksStorage() const { return clocks_; }
    bool isClockValid(ClockId clock) const { return clock < clocks_.size(); }

  private:
    std::vector<TNode> nodes_;
    std::vector<TEdge> edges_;
    std::vector<TCaptureEdge> captureEdges_;
    std::vector<NodeId> freeNodes_;
    std::vector<EdgeId> freeEdges_;
    std::vector<CaptureEdgeId> freeCaptureEdges_;
    std::vector<TClock> clocks_;
    std::size_t liveNodes_ {0};
    std::size_t liveEdges_ {0};
    std::size_t liveCaptureEdges_ {0};
};

}  // namespace naja::TG
