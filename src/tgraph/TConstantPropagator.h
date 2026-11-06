// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <utility>
#include <vector>
#include "TGraph.h"
namespace naja::TG {
class TSNLGraphConstructor;
/// Three-valued combinational evaluation. Explicit seeds are the only policy
/// input; callers may use this engine for future case-analysis constraints.
/// Does not modify waveform arcs, physical connectivity or the netlist.
class TConstantPropagator {
 public:
  enum class Value { Unknown, Zero, One };
  using Seed = std::pair<TGraph::NodeId, Value>;
  static std::vector<Value> run(TSNLGraphConstructor& constructor, const std::vector<Seed>& seeds);
};
}
