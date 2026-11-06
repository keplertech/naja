// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "SNLOccurrence.h"
#include "TGraph.h"
#include "TNodeProxy.h"
#include "TEdgeProxy.h"

namespace naja::DNL {
class DNLInstanceFull;
class DNLTerminalFull;
template<class Instance, class Terminal> class DNL;
using DNLFull = DNL<DNLInstanceFull, DNLTerminalFull>;
}

namespace naja::NL { class NLReset; }

namespace naja::TG {

/// Terminal identity includes the containing hierarchy, even for shared models.
/// SNL objects and paths must outlive the graph and remain unchanged during use.
class TSNLNodeProxy final: public TNodeProxy {
  public:
    explicit TSNLNodeProxy(const naja::NL::SNLOccurrence& occurrence): occurrence_(occurrence) {}
    const naja::NL::SNLOccurrence& getOccurrence() const { return occurrence_; }
    std::string getDescription() const override;
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
    /// Joined hierarchical inout nets use their first net occurrence in DNL order.
    const naja::NL::SNLOccurrence& getOccurrence() const { return occurrence_; }
  private:
    friend class TSNLGraphConstructor;
    Kind kind_;
    naja::NL::SNLOccurrence occurrence_;
};

/// Builds an occurrence-aware graph from a processed DNL of a structural SNL design.
/// Hierarchical instance terminals and their internal port occurrences share
/// one node, or one drive/load pair for split inouts. Split opaque-cell inouts
/// and inouts on nets with several drivers. Hierarchical inout bindings join
/// net partitions into one equipotential; no load-to-drive bypass is introduced.
/// Leaf nodes use instance-selected modeling roles and timing arcs.
/// Recognizes scalar canonical DB0 latch-plus-AND gates whose latch enable
/// is the inverse of the AND's clock input, through wires and DB0 buffers or
/// inverters. Physical nodes and edges are retained. Unsupported elements are
/// reported and left opaque. Hierarchy expansion/validation belongs to DNL.
/// Imports NLClock declarations per hierarchy occurrence, with one clock
/// identity for all sources of a declaration. Declarations must outlive the graph.
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
    enum class CoverageReason {
      Memory, InOut, Blackbox, UnsupportedLatch, MultipleDrivers,
      IncompleteModeling, AmbiguousClockGate
    };
    enum class Treatment { Opaque, Approximated, Bidirectional, AllDrivers };
    struct CoverageEntry {
      naja::NL::SNLOccurrence occurrence;
      std::string modelName;
      CoverageReason reason;
      Treatment treatment;
      std::string detail;
    };
    /// Counts are entry counts, not pin counts; absent reasons have zero entries.
    using CoverageCounts = std::map<CoverageReason, size_t>;
    struct ResetDomain {
      const naja::NL::NLReset* declaration;
      std::vector<TGraph::NodeId> sources;
    };
    const std::vector<ResetDomain>& getResetDomains() const { getGraph(); return resets_; }
    TSNLGraphConstructor() = default;
    /// Published atomically with the graph; entries retain netlist occurrences.
    /// Throws before the first successful construct(). An empty report means
    /// every element was modeled. Fully modeled DB0 memories are not reported.
    /// Opaque entries describe whole cells; inout entries describe boundary pins.
    /// MultipleDrivers entries describe bit-net occurrences.
    const std::vector<CoverageEntry>& getCoverageReport() const;
    const CoverageCounts& getCoverageCounts() const;
    /// Obtains the active DNL on demand from the universe's top design.
    /// Callers must invalidate/rebuild DNL after SNL edits.
    void construct();
    /// Consumes an already processed DNL. No hierarchy expansion or validation
    /// is performed here. The DNL is only needed during construction; referenced
    /// SNL objects must outlive the resulting graph. Replaces the previous
    /// result only on success, invalidating references to that previous result.
    void construct(const naja::DNL::DNLFull& dnl);
    TGraph& getGraph();
    const TGraph& getGraph() const;
    /// Lookup top ports or instance terminals in their containing path.
    /// Internal hierarchical port occurrences are accepted as aliases.
    /// The mapping describes the constructed graph; do not remove/reuse its nodes.
    /// Returns the load role when split, the only node otherwise.
    TGraph::NodeId getNode(const naja::NL::SNLOccurrence& occurrence) const;
    TGraph::NodeId getDriverNode(const naja::NL::SNLOccurrence& occurrence) const;
    /// Role-aware occurrence seeding (including hierarchical aliases).
    TGraph::ClockId createClock(const std::string& name, const naja::NL::SNLOccurrence& occurrence);
    struct ConstructionStats {
      /// Additional nodes: each physical split pin adds exactly one drive role.
      size_t splitPins {0};
      size_t connectivityEdges {0};
      size_t largestNetEdges {0};
      /// Direct driver x load edges are retained even above this reporting bound.
      size_t netsOverMillionEdges {0};
    };
    const ConstructionStats& getConstructionStats() const { getGraph(); return stats_; }
    /// Valid while the graph/netlist is unchanged. Rebuilding replaces these
    /// occurrence-aware associations atomically with the graph.
    const std::vector<ClockGate>& getClockGates() const;

  private:
    std::vector<ResetDomain> resets_;
    ConstructionStats stats_;
    std::vector<CoverageEntry> coverage_;
    CoverageCounts coverageCounts_;
    std::unique_ptr<TGraph> graph_;
    struct OccurrenceHash {
      size_t operator()(const naja::NL::SNLOccurrence& occurrence) const;
    };
    // Lookup only: creation and emission always follow DNL table order.
    std::unordered_map<naja::NL::SNLOccurrence, TGraph::NodeId, OccurrenceHash> nodes_;
    std::vector<ClockGate> clockGates_;
};

}  // namespace naja::TG
