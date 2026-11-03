// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TClock.h"

#include <stdexcept>

#include "NLClock.h"
#include "SNLNetComponent.h"

namespace naja::TG {

TClock::TClock(ClockId index, const std::string& name, naja::NL::SNLNetComponent* target, NodeId node)
  : index_(index),
    name_(name),
    seeds_{{target, node}} {
}

TClock::TClock(ClockId index, const naja::NL::NLClock& clock, const std::vector<NodeId>& nodes)
  : index_(index), name_(clock.getName().getString()), nlClock_(&clock) {
  const auto sources = clock.getSources();
  if (nodes.empty() || sources.size() != nodes.size()) {
    throw std::invalid_argument("TClock: expected one seed node per NLClock source");
  }
  seeds_.reserve(nodes.size());
  for (auto* source : sources) {
    auto* target = dynamic_cast<naja::NL::SNLNetComponent*>(source);
    if (!target) throw std::invalid_argument("TClock: expected an SNL terminal source");
    seeds_.push_back({target, nodes[seeds_.size()]});
  }
}

}  // namespace naja::TG
