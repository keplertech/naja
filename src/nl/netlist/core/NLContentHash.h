// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace naja::NL {

using ContentHash = std::array<uint8_t, 32>;
inline constexpr unsigned kContentHashVersion = 1;
std::string toHex(const ContentHash& hash);
const std::vector<std::string>& getContentHashAttributeNames();
const std::vector<std::string>& getContentHashPropertyNames();

class NLUniverse;
class NLDB;
class NLLibrary;
class SNLDesign;
class SNLInstance;
class SNLTerm;
class SNLBitTerm;
class SNLNet;

/** One computation with shared design memoization. Do not mutate the universe
 * while using a hasher; create a new hasher after any edit. No persistent cache.
 */
class ContentHasher {
  public:
    ContentHash hash(const NLUniverse* object);
    ContentHash hash(const NLDB* object);
    ContentHash hash(const NLLibrary* object);
    ContentHash hash(const SNLDesign* object);
    ContentHash hash(const SNLInstance* object);
    ContentHash hash(const SNLTerm* object);
    ContentHash hash(const SNLNet* object);
  private:
    const std::vector<SNLBitTerm*>& orderedBitTerms(const SNLDesign* design);
    std::unordered_map<const SNLDesign*, std::vector<SNLBitTerm*>> bitTerms_;
    std::unordered_map<const SNLDesign*, ContentHash> designs_;
    std::unordered_set<const SNLDesign*> active_;
};

} // namespace naja::NL
