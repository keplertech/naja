// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace naja::NL {

class SNLNetComponent;
class NLClock;

}  // namespace naja::NL

namespace naja::TG {

class TClock {
  public:
    using ClockId = std::size_t;
    /// \brief same underlying type as TGraph::NodeId (TClock can't include
    /// TGraph.h - TGraph.h includes TClock.h).
    using NodeId = std::size_t;

    TClock(ClockId index, const std::string& name, naja::NL::SNLNetComponent* target, NodeId node);
    /// Binds every declared source, in NLClock source order, to a graph node.
    /// The declaration and its source terminals must outlive this clock.
    TClock(ClockId index, const naja::NL::NLClock& clock, const std::vector<NodeId>& nodes);

    struct Seed {
      naja::NL::SNLNetComponent* target;
      NodeId node;
    };
    /// Null for clocks created without a user declaration. No reverse link.
    const naja::NL::NLClock* getNLClock() const { return nlClock_; }
    const std::vector<Seed>& getSeeds() const { return seeds_; }

    ClockId getIndex() const { return index_; }
    const std::string& getName() const { return name_; }

    /// \return this clock's target: either a top terminal (SNLBitTerm) or
    /// an instance terminal (SNLInstTerm) - both are SNLNetComponent.
    /// For a multi-source declaration, returns the first source terminal.
    naja::NL::SNLNetComponent* getTarget() const { return seeds_.front().target; }

    /// \return the TGraph node this clock is seeded at - TClockPropagator's
    /// forward pass starts from here.
    /// For a multi-source declaration, returns the first seed node.
    NodeId getNode() const { return seeds_.front().node; }

  private:
    ClockId index_;
    std::string name_;
    const naja::NL::NLClock* nlClock_ {nullptr};
    std::vector<Seed> seeds_;
};

}  // namespace naja::TG
