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
/// State arcs seed Q launch masks from their clock/gate's waveform origins;
/// waveform propagation never crosses state arcs or DataOnly gate inputs.
/// Data and control captures stop at state outputs and remain independent.
/// Enabled combinational cycles are rejected without changing edge enablement.
/// TLoopBreaker remains an explicit, lossy caller policy, never invoked here.
/// Masks are recomputed on every run; caller-disabled edges have empty masks.
/// Only latches classified as recognized clock-gate state are supported.
class TClockPropagator {
  public:
    TClockPropagator() = delete;

    struct Result {
      /// Waveform origins indexed by NodeId, including clock pins with no
      /// outgoing edges. Snapshot of this run; invalidated logically by graph
      /// mutation or clock registration. Removed slots have empty masks.
      std::vector<TEdge::ClockMask> clockOrigins;
    };

    static Result run(TGraph& graph);
};

}  // namespace naja::TG
