// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "SNLDesign.h"

namespace naja::NL {

class NLLibrary;
class SNLDesign;

class SNLUtils {
  public:
    struct InstanceCount {
      // User-visible hierarchy count: shared models are counted once per
      // occurrence path from the top design.
      size_t totalInstances {0};
      size_t leafInstances {0};
      // Folded count: shared models are traversed once, matching the in-memory
      // model graph.
      size_t foldedTotalInstances {0};
      size_t foldedLeafInstances {0};
      size_t reachableModels {0};
    };

    using DesignsLevel = std::map<const SNLDesign*, unsigned, SNLDesign::PointerLess>;
    static unsigned levelize(const SNLDesign* design, DesignsLevel& designsLevel);
    using DesignLevel = std::pair<const SNLDesign*, unsigned>;
    using SortedDesigns = std::vector<DesignLevel>;
    static void getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns);
    static InstanceCount countReachableInstances(const SNLDesign* top);
    /// Remove canonical mux instances with no path through muxes to a design
    /// terminal or non-mux consumer. Other instances and all nets are retained.
    /// Operates on this model only; returns the number of removed instances.
    static size_t removeUnconsumedMuxes(SNLDesign* design);
    /// Combine mux(t, mux(s, a, b), b) into mux(s | t, a, b) when
    /// the inner mux has no other connections. Also combines the dual common-A
    /// pattern using AND. Preserves four-state values.
    /// Operates on this model only; returns the number of removed muxes.
    static size_t mergeRepeatedMuxSelections(SNLDesign* design);
    static NLID::Bit getWidth(int msb, int lsb);
    static SNLDesign* findTop(const NLLibrary* library);
    static void prepareForConcurrentAccess(const SNLDesign* design);
    static std::string getUnnamedIDString(const char* tag, std::uint64_t id);
    static std::string getNamedString(const NLName& name, const char* tag, std::uint64_t id);
    static std::string getBusNamedString(
        const NLName& name,
        const char* tag,
        std::uint64_t id,
        NLID::Bit msb,
        NLID::Bit lsb);
    static std::string getBitNamedString(
        const NLName& name,
        const char* tag,
        std::uint64_t id,
        NLID::Bit bit);
};

} // namespace naja::NL
