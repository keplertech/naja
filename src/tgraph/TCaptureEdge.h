// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

namespace naja::TG {

class TGraph;

/// Associates a clock with a data capture or clock-gate control endpoint.
/// Stored separately from signal edges; never traversed by ordinary graph passes.
/// The builder is responsible for selecting compatible endpoint roles.
class TCaptureEdge {
  public:
    enum class Kind { Data, ClockGateControl };

    using NodeId = std::size_t;
    using CaptureEdgeId = std::size_t;

    CaptureEdgeId getIndex() const { return index_; }
    NodeId getSource() const { return source_; }
    NodeId getTarget() const { return target_; }
    Kind getKind() const { return kind_; }
    bool isAlive() const { return alive_; }

  private:
    friend class TGraph;

    TCaptureEdge(CaptureEdgeId index, NodeId source, NodeId target, Kind kind)
      : index_(index), source_(source), target_(target), kind_(kind) {}

    void reset(CaptureEdgeId index, NodeId source, NodeId target, Kind kind) {
      index_ = index;
      source_ = source;
      target_ = target;
      kind_ = kind;
      alive_ = true;
    }
    void deactivate() { alive_ = false; }

    CaptureEdgeId index_;
    NodeId source_;
    NodeId target_;
    Kind kind_ {Kind::Data};
    bool alive_ {true};
};

}  // namespace naja::TG
