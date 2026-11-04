// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "TEdge.h"
#include "TNodeProxy.h"

namespace naja::TG {

class TGraph;

/// Separate waveform, launch, data-capture and gate-control views of one graph.
/// State arcs seed Q launch masks from their clock/gate's waveform origins.
/// A sole inferred flop Q waveform inherits its C origins. ClockPass inputs
/// (DB0 mux data arms and recognized gate clocks) admit each classifiable source
/// and union their origins, including Qs alongside explicit seeds. DataOnly
/// inputs (mux selects and held gate enables) never pass waveforms. At other
/// merges, seeds/classified waveforms dominate Qs; distinct unclassified Qs
/// without such a waveform remain ambiguous. Gate-decomposed/custom muxes are
/// not recognized; their suppressed Qs are reported as controlStateArcs, not
/// proof that those Qs are enables. Ambiguous/unseeded clock pins are reported
/// in unclockedFlopClockPins. Recognized gate latch state never passes waveforms.
/// Data and control captures stop at state outputs and remain independent.
/// Enabled combinational cycles are rejected without changing edge enablement.
/// TLoopBreaker remains an explicit, lossy caller policy, never invoked here.
/// Masks are recomputed on every run; caller-disabled edges have empty masks.
/// Bare combinational gate enable crossings are not separately captured;
/// recognized clock gates retain their independent gate-control captures.
/// Source classification uses O(nodes + edges) work and storage. For a fixed
/// number of clock indices, waveform union propagation is also linear; each
/// newly added clock bit can schedule a node at most once.
/// Plain latch State/capture associations use the gate waveform without time
/// borrowing; recognized clock-gate control captures retain their own view.
class TClockPropagator {
  public:
    TClockPropagator() = delete;

    struct Result {
      /// Waveform origins indexed by NodeId, including clock pins with no
      /// outgoing edges. Snapshot of this run; invalidated logically by graph
      /// mutation or clock registration. Removed slots have empty masks.
      std::vector<TEdge::ClockMask> clockOrigins;
      /// Live flop Clock-role nodes with no waveform origin, in NodeId order.
      /// Includes unseeded and ambiguous inferred-clock cones.
      std::vector<TEdge::NodeId> unclockedFlopClockPins;
      /// Enabled C->Q associations admitted on inferred clock paths, in EdgeId
      /// order (including legacy implicit State arcs targeting flop outputs).
      /// Admission does not guarantee a seeded origin; see unclockedFlopClockPins.
      std::vector<TEdge::EdgeId> generatedClockStateArcs;
      /// Enabled flop C->Q associations whose Q reaches a clock-use cone through
      /// a DataOnly input or is suppressed at a seed/classified-waveform merge,
      /// in EdgeId order. This is conservative control classification, not proof
      /// of enable function. A Q used as both waveform and control may appear in
      /// both reports. Disabled and removed associations are excluded.
      std::vector<TEdge::EdgeId> controlStateArcs;
    };

    /// A node on the first rejected cycle, rather than a downstream blocked node.
    /// Snapshots proxy text/role; NodeId refers to the graph supplied to run().
    class CycleError: public std::logic_error {
      public:
        CycleError(TEdge::NodeId node, std::string description, TNodeProxy::Role role)
          : std::logic_error("TClockPropagator::run: enabled combinational cycle at " +
              description + "; no edges were disabled"), node_(node),
            description_(std::move(description)), role_(role) {}
        TEdge::NodeId getNode() const { return node_; }
        const std::string& getDescription() const { return description_; }
        TNodeProxy::Role getRole() const { return role_; }
      private:
        TEdge::NodeId node_;
        std::string description_;
        TNodeProxy::Role role_;
    };
    static Result run(TGraph& graph);
};

}  // namespace naja::TG
