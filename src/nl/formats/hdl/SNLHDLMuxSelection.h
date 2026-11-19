// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace naja::NL {

class SNLBitNet;

// A private whole-vector branch result. It has no placeholder nets and is never
// entered in expression caches. The frontend must materialize it before a read,
// sharing, replay, partial update or other escape from the enclosing merge.
template<typename Source>
class SNLHDLMuxSelection {
 public:
  using Bits = std::vector<SNLBitNet*>;
  using Sources = std::vector<Source>;

  SNLHDLMuxSelection() = default;
  explicit SNLHDLMuxSelection(Bits bits): no_(std::move(bits)) {}

  std::size_t width() const { return no_.size(); }

  bool hasSelection() const { return !no_.empty(); }

  template<typename Emit>
  Bits materialize(const Emit& emit) {
    if (condition_) {
      no_ = emit(condition_, no_, yes_, sources_);
      condition_ = nullptr;
      yes_.clear();
      sources_.clear();
    }
    return no_;
  }

  // Only the same-arm identities are used, matching mux cleanup:
  // s ? b : (t ? b : a) = (s OR t) ? b : a
  // s ? (t ? b : a) : a = (s AND t) ? b : a.
  // Gate and mux emission, bit order and source annotation stay frontend-owned.
  template<typename Gate, typename Emit>
  static SNLHDLMuxSelection merge(SNLBitNet* condition,
      SNLHDLMuxSelection no, SNLHDLMuxSelection yes, const Source& source,
      const Gate& gate, const Emit& emit) {
    if (no.condition_ && !yes.condition_ && no.yes_ == yes.no_) {
      no.sources_.push_back(source);
      if (condition != no.condition_)
        no.condition_ = gate(false, no.condition_, condition, no.sources_);
      return no;
    }
    if (yes.condition_ && !no.condition_ && yes.no_ == no.no_) {
      yes.sources_.push_back(source);
      if (condition != yes.condition_)
        yes.condition_ = gate(true, yes.condition_, condition, yes.sources_);
      return yes;
    }
    auto a = no.materialize(emit);
    auto b = yes.materialize(emit);
    if (a == b) return SNLHDLMuxSelection(std::move(a));
    SNLHDLMuxSelection result(std::move(a));
    result.yes_ = std::move(b);
    result.condition_ = condition;
    result.sources_.push_back(source);
    // Keep the complete vectors, including identical positions, until the
    // owning merge emits the changed runs. Matching still requires the entire
    // common arm in the same order; unavailable values and invalid widths flush.
    bool whole = condition && !result.no_.empty() && result.no_.size() == result.yes_.size();
    for (std::size_t i = 0; whole && i < result.no_.size(); ++i)
      whole = result.no_[i] && result.yes_[i];
    if (!whole)
      return SNLHDLMuxSelection(emit(condition, result.no_, result.yes_, result.sources_));
    return result;
  }

 private:
  Bits no_, yes_;
  SNLBitNet* condition_ = nullptr;
  Sources sources_;
};

}  // namespace naja::NL
