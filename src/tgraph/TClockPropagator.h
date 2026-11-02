// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <vector>

#include "TEdge.h"

namespace naja::TG {

class TGraph;

/// Separate waveform, launch, data-capture and gate-control views of one graph.
/// State arcs seed Q launch masks from their clock/gate's waveform origins.
/// A flop Q used as the sole inferred waveform source on a clock path inherits
/// its C origins. At merges, explicit seed waveforms take precedence over Q
/// controls; multiple distinct Q sources without a seed are left unresolved.
/// Recognized gate latch state and DataOnly inputs never pass waveforms.
/// Data and control captures stop at state outputs and remain independent.
/// Enabled combinational cycles are rejected without changing edge enablement.
/// TLoopBreaker remains an explicit, lossy caller policy, never invoked here.
/// Masks are recomputed on every run; caller-disabled edges have empty masks.
/// Bare combinational gate enable crossings are not separately captured;
/// recognized clock gates retain their independent gate-control captures.
/// Source classification uses O(nodes + edges) work and storage. For a fixed
/// number of clock indices, waveform union propagation is also linear; each
/// newly added clock bit can schedule a node at most once.
/// Only latches classified as recognized clock-gate state are supported.
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
    };

    static Result run(TGraph& graph);
};

}  // namespace naja::TG
