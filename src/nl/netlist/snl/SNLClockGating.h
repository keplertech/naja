// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace naja::NL {
class SNLDesign;
class SNLInstance;

/// Conservative, language-neutral post-elaboration clock-gate inference.
/// Roots are analyzed together. No clock declarations are manufactured.
class SNLClockGating {
  public:
    struct Hooks {
      std::function<void(SNLInstance*)> beforeReplace;
      std::function<void(SNLInstance*)> afterReplace;
      std::function<void(SNLInstance*)> beforeRemove;
    };
    static size_t infer(const std::vector<SNLDesign*>& roots, const Hooks& hooks = {});
    static size_t infer(SNLDesign* root, const Hooks& hooks = {}) {
      return infer(std::vector<SNLDesign*>{root}, hooks);
    }
};
}
