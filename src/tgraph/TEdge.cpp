// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TEdge.h"

#include "TEdgeProxy.h"

namespace naja::TG {

TEdge::TEdge(EdgeId index, NodeId source, NodeId target, TEdgeProxy* proxy)
  : index_(index),
    source_(source),
    target_(target),
    alive_(true),
    proxy_(proxy) {
}

TEdge::TEdge(TEdge&&) = default;
TEdge& TEdge::operator=(TEdge&&) = default;

TEdge::~TEdge() = default;

TEdge TEdge::create(EdgeId index, NodeId source, NodeId target, TEdgeProxy* proxy) {
  return TEdge(index, source, target, proxy);
}

void TEdge::reset(EdgeId index, NodeId source, NodeId target, TEdgeProxy* proxy) {
  index_ = index;
  source_ = source;
  target_ = target;
  alive_ = true;
  proxy_.reset(proxy);
  fromClocks_.clear();
  toClocks_.clear();
  controlClocks_.clear();
  kind_ = Kind::Combinational;
  disabled_ = false;
}

void TEdge::deactivate() {
  alive_ = false;
  proxy_.reset();
  fromClocks_.clear();
  toClocks_.clear();
  controlClocks_.clear();
  kind_ = Kind::Combinational;
  disabled_ = false;
}

}  // namespace naja::TG
