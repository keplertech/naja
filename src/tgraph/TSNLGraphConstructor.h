// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "SNLOccurrence.h"
#include "TGraph.h"
#include "TNodeProxy.h"
#include "TEdgeProxy.h"

namespace naja::NL {
class SNLDesign;
}

namespace naja::TG {

/// Terminal identity includes the containing hierarchy, even for shared models.
/// SNL objects and paths must outlive the graph and remain unchanged during use.
class TSNLNodeProxy final: public TNodeProxy {
  public:
    explicit TSNLNodeProxy(const naja::NL::SNLOccurrence& occurrence): occurrence_(occurrence) {}
    const naja::NL::SNLOccurrence& getOccurrence() const { return occurrence_; }
  private:
    naja::NL::SNLOccurrence occurrence_;
};

class TSNLEdgeProxy final: public TEdgeProxy {
  public:
    /// ClockPass remains the AND's physical combinational dependency; its
    /// classification identifies the clock-bearing input for analysis views.
    /// LatchGateToOutput is a state-control association, not a data-through arc.
    enum class Kind { Connectivity, Combinational, ClockToOutput, ClockPass, LatchGateToOutput };
    TSNLEdgeProxy(Kind kind, const naja::NL::SNLOccurrence& occurrence)
      : kind_(kind), occurrence_(occurrence) {}
    Kind getKind() const { return kind_; }
    /// Net occurrence for connectivity, instance occurrence for primitive arcs.
    const naja::NL::SNLOccurrence& getOccurrence() const { return occurrence_; }
  private:
    friend class TSNLGraphConstructor;
    Kind kind_;
    naja::NL::SNLOccurrence occurrence_;
};

/// Builds an occurrence-aware graph from a structural SNL design.
/// Hierarchical instance terminals and their internal port occurrences share
/// one node. Leaf nodes use instance-selected modeling roles and timing arcs.
/// Recognizes scalar canonical DB0 latch-plus-AND gates whose latch enable
/// is the inverse of the AND's clock input, through wires and DB0 buffers or
/// inverters. Physical nodes and edges are retained. Other latches, memories,
/// inouts, recursive hierarchy, ambiguous drivers and incomplete modeling fail.
/// Clock definitions are not registered by this constructor.
class TSNLGraphConstructor {
  public:
    /// Analysis association over existing terminals, not a replacement cell.
    struct ClockGate {
      TGraph::NodeId clock;       //AND clock input.
      TGraph::NodeId enable;      //Latch data input.
      TGraph::NodeId heldEnable;  //Latch state output.
      TGraph::NodeId latchGate;   //Latch transparency control, inverse of clock.
      TGraph::NodeId output;      //AND output.
      TGraph::EdgeId clockPassEdge;
    };
    explicit TSNLGraphConstructor(naja::NL::SNLDesign* top): top_(top) {}
    /// Replaces the previous result only on success. Rebuilding invalidates
    /// references to that previous graph. Netlist edits require rebuilding.
    void construct();
    TGraph& getGraph();
    const TGraph& getGraph() const;
    /// Lookup top ports or instance terminals in their containing path.
    /// Internal hierarchical port occurrences are accepted as aliases.
    /// The mapping describes the constructed graph; do not remove/reuse its nodes.
    TGraph::NodeId getNode(const naja::NL::SNLOccurrence& occurrence) const;
    /// Valid while the graph/netlist is unchanged. Rebuilding replaces these
    /// occurrence-aware associations atomically with the graph.
    const std::vector<ClockGate>& getClockGates() const;

  private:
    naja::NL::SNLDesign* top_;
    std::unique_ptr<TGraph> graph_;
    std::map<naja::NL::SNLOccurrence, TGraph::NodeId> nodes_;
    std::vector<ClockGate> clockGates_;
};

}  // namespace naja::TG
