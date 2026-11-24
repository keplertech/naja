// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace naja {

/// Views are valid only during the callback. A zero total means indeterminate.
struct NajaProgress {
  std::string_view stage;
  std::string_view detail;
  uint64_t current;
  uint64_t total;
};

class NajaCancelled : public std::runtime_error {
 public:
  NajaCancelled(): std::runtime_error("Design loading cancelled") {}
};

/// Invoked synchronously on the loading thread; false requests cancellation.
using NajaProgressCallback = std::function<bool(const NajaProgress&)>;

/// Stage boundaries are unconditional. Intermediate reports require both 64
/// work units and 50 ms since the previous report. No clock reads without a callback.
class NajaProgressReporter {
 public:
  void setCallback(NajaProgressCallback callback) { callback_ = std::move(callback); }
  explicit operator bool() const { return bool(callback_); }
  void begin(std::string_view stage, std::string_view detail = {}, uint64_t total = 0) {
    if (!callback_) return;
    stage_ = stage;
    current_ = checks_ = 0;
    total_ = total;
    emit(detail);
  }
  void advance(std::string_view detail = {}) {
    if (!callback_) return;
    ++current_;
    checkpoint(detail);
  }
  /// Check cancellation during a large unit without changing its work count.
  void checkpoint(std::string_view detail = {}) {
    if (!callback_) return;
    if (++checks_ % 64 == 0 &&
        std::chrono::steady_clock::now() - lastTime_ >= std::chrono::milliseconds(50))
      emit(detail);
  }
  void finish(std::string_view detail = {}) {
    if (callback_) emit(detail);
  }
 private:
  void emit(std::string_view detail) {
    lastTime_ = std::chrono::steady_clock::now();
    checks_ = 0;
    if (!callback_({stage_, detail, current_, total_})) throw NajaCancelled();
  }
  NajaProgressCallback callback_;
  std::string_view stage_;
  uint64_t current_ = 0, total_ = 0, checks_ = 0;
  std::chrono::steady_clock::time_point lastTime_ {};
};
} // namespace naja
