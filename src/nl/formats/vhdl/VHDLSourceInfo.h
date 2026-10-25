// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLException.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLRTLInfos.h"
#include "vhdl/Lexer.h"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace naja::NL {

struct VHDLSourceInput {
  size_t offset;
  size_t size;
  std::string path;
};

// Coordinates belong to the original file, even when parsing concatenated units.
class VHDLSourceMap {
 public:
  VHDLSourceMap(std::string_view source, std::vector<VHDLSourceInput> inputs):
      source_(source), inputs_(std::move(inputs)) {
    for (size_t i = 0; i < source.size(); ++i)
      if (source[i] == '\n') lines_.push_back(i + 1);
  }

  // The legacy structural specialization rewrites generic defaults and extracts
  // units. Preserve its byte mapping instead of reporting rewritten coordinates.
  VHDLSourceMap(std::string_view source, const VHDLSourceMap& parent,
                std::vector<size_t> offsets):
      VHDLSourceMap(source, {}) {
    parent_ = &parent;
    offsets_ = std::move(offsets);
  }

  SNLSourceReference reference(const vhdl::SourceSpan& span) const {
    auto start = std::min(span.start.offset, source_.size());
    auto end = std::min(span.end.offset, source_.size());
    if (end > start) --end; // AST ends are exclusive; SNL ends are inclusive.
    else end = start;
    if (parent_ && !offsets_.empty()) {
      vhdl::SourceSpan original;
      original.start.offset = offsets_[std::min(start, offsets_.size() - 1)];
      original.end.offset = offsets_[std::min(end, offsets_.size() - 1)] + 1;
      return parent_->reference(original);
    }
    size_t base = 0;
    size_t limit = source_.size();
    std::string path = "<source>";
    const auto inputIt = std::upper_bound(inputs_.begin(), inputs_.end(), start,
        [](size_t offset, const auto& input) { return offset < input.offset; });
    if (inputIt != inputs_.begin()) {
      const auto& input = *(inputIt - 1);
      base = input.offset;
      path = input.path.empty() ? "<source>" : input.path;
      limit = input.offset + input.size;
    }
    if (limit > base) end = std::min(end, limit - 1);
    const auto [line, column] = position(start, base);
    const auto [endLine, endColumn] = position(std::max(start, end), base);
    return {{NLName(path), line, endLine, column, endColumn},
        NLName("naja-vhdl"), NLName("vhdl"), NLName("rtl")};
  }

  template<typename T>
  T* annotate(T* object, const vhdl::SourceSpan& span, bool declaration = false) const {
    if (!object) return object;
    const auto ref = reference(span);
    const auto attach = [&](auto* target) {
      auto* infos = target->getRTLInfos();
      if (!infos) infos = SNLRTLInfos::create(target);
      infos->setSourceLoc(ref.range);
      if (declaration) infos->setSourceDeclaration(ref);
      else infos->addSourceOrigin(ref);
    };
    attach(object);
    if (auto* bus = dynamic_cast<SNLBusNet*>(object))
      for (auto* bit : bus->getBits()) attach(bit);
    if (auto* bus = dynamic_cast<SNLBusTerm*>(object))
      for (auto* bit : bus->getBusBits()) attach(bit);
    return object;
  }

 private:
  std::pair<uint32_t, uint32_t> position(size_t offset, size_t base) const {
    const auto line = std::upper_bound(lines_.begin(), lines_.end(), offset);
    const auto first = std::upper_bound(lines_.begin(), lines_.end(), base);
    const auto number = size_t(line - first) + 1;
    const auto column = offset - std::max(base, *(line - 1)) + 1;
    if (number > std::numeric_limits<uint32_t>::max() ||
        column > std::numeric_limits<uint32_t>::max())
      throw NLException("VHDL source coordinate exceeds the 32-bit coordinate limit");
    return {static_cast<uint32_t>(number), static_cast<uint32_t>(column)};
  }

  std::string_view source_;
  std::vector<VHDLSourceInput> inputs_;
  std::vector<size_t> lines_ {0};
  const VHDLSourceMap* parent_ = nullptr;
  std::vector<size_t> offsets_;
};

}  // namespace naja::NL
