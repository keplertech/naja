// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>
#include <functional>
#include <cstdint>
#include <string_view>
#include <string>

#include <spdlog/spdlog.h>

namespace naja::log {

void init();
void initFromEnv();
std::shared_ptr<spdlog::logger> get();

void setLevel(spdlog::level::level_enum level);
void setPattern(const std::string& pattern);
void addConsoleSink(spdlog::level::level_enum level = spdlog::level::info);
void addFileSink(const std::string& path,
                 spdlog::level::level_enum level = spdlog::level::trace);
using SinkHandle = std::uint64_t;
// Callbacks receive the formatted payload on the logging thread. The view is
// valid only during the call. An in-flight dispatch may invoke a removed sink.
SinkHandle addCallbackSink(
  std::function<void(spdlog::level::level_enum, std::string_view)> callback,
  spdlog::level::level_enum level = spdlog::level::info);
void removeSink(SinkHandle handle);
void setConsoleLogging(bool enabled);

// Suppress callback delivery for this thread, including nested native logging.
class ScopedCallbackSuppression {
 public:
  ScopedCallbackSuppression();
  ~ScopedCallbackSuppression();
  ScopedCallbackSuppression(const ScopedCallbackSuppression&) = delete;
  ScopedCallbackSuppression& operator=(const ScopedCallbackSuppression&) = delete;
};

void flushEvery(std::chrono::seconds interval);
void clearSinks();
void shutdown();

spdlog::level::level_enum levelFromString(const std::string& level);

}  // namespace naja::log

#define NAJA_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(naja::log::get(), __VA_ARGS__)
#define NAJA_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(naja::log::get(), __VA_ARGS__)
#define NAJA_LOG_INFO(...) SPDLOG_LOGGER_INFO(naja::log::get(), __VA_ARGS__)
#define NAJA_LOG_WARN(...) SPDLOG_LOGGER_WARN(naja::log::get(), __VA_ARGS__)
#define NAJA_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(naja::log::get(), __VA_ARGS__)
#define NAJA_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(naja::log::get(), __VA_ARGS__)
