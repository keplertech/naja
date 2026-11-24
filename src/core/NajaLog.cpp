// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaLog.h"

#include <cstdlib>
#include <algorithm>
#include <atomic>
#include <vector>
#include <mutex>

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace naja::log {
namespace {
thread_local unsigned callback_suppression_ = 0;
std::atomic<SinkHandle> next_handle_{1};

// The logger's sink vector never changes. Configuration affects only this
// dispatcher's destinations; no configuration lock is held during delivery.
class Dispatcher final: public spdlog::sinks::sink {
 private:
  struct Entry {
    SinkHandle handle;
    spdlog::sink_ptr sink;
    bool console;
  };

 public:
  void log(const spdlog::details::log_msg& message) override {
    for (const auto& entry: snapshot()) {
      if (entry.sink->should_log(message.level)) entry.sink->log(message);
    }
  }
  void flush() override {
    for (const auto& entry: snapshot()) entry.sink->flush();
  }
  void set_pattern(const std::string& value) override {
    std::lock_guard<std::mutex> lock(mutex_);
    pattern_ = value;
    for (const auto& entry: entries_) entry.sink->set_pattern(value);
  }
  void set_formatter(std::unique_ptr<spdlog::formatter> formatter) override {
    for (const auto& entry: snapshot()) entry.sink->set_formatter(formatter->clone());
  }
  SinkHandle add(spdlog::sink_ptr sink, bool console = false) {
    std::lock_guard<std::mutex> lock(mutex_);
    sink->set_pattern(pattern_);
    auto handle = next_handle_++;
    entries_.push_back({handle, std::move(sink), console});
    return handle;
  }
  void remove(SinkHandle handle) {
    // Destroy retired callbacks outside the configuration lock.
    std::vector<Entry> retired;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->handle == handle) {
          retired.push_back(std::move(*it));
          it = entries_.erase(it);
        } else ++it;
      }
    }
  }

  void setConsoleLogging(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
      [](const auto& entry) { return entry.console; }), entries_.end());
    if (enabled) {
      auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      sink->set_level(spdlog::level::info);
      sink->set_pattern(pattern_);
      entries_.push_back({next_handle_++, sink, true});
    }
  }

  void clear() {
    flush();
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(spdlog::level::info);
    std::vector<Entry> retired;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      retired.swap(entries_);
      sink->set_pattern(pattern_);
      entries_.push_back({next_handle_++, sink, true});
    }
  }

 private:
  std::vector<Entry> snapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
  }
  std::mutex mutex_;
  std::vector<Entry> entries_;
  std::string pattern_ = "%Y-%m-%d %H:%M:%S,%e [naja] [%^%l%$] %v";
};

class CallbackSink final: public spdlog::sinks::sink {
 public:
  explicit CallbackSink(
    std::function<void(spdlog::level::level_enum, std::string_view)> callback):
    callback_(std::move(callback)) {}
  void log(const spdlog::details::log_msg& message) override {
    if (callback_suppression_) return;
    ScopedCallbackSuppression suppress;
    callback_(message.level, {message.payload.data(), message.payload.size()});
  }
  void flush() override {}
  void set_pattern(const std::string&) override {}
  void set_formatter(std::unique_ptr<spdlog::formatter>) override {}
 private:
  std::function<void(spdlog::level::level_enum, std::string_view)> callback_;
};

std::shared_ptr<spdlog::logger> logger_;
std::mutex logger_mutex_;

std::shared_ptr<spdlog::logger> ensureLogger() {
  std::lock_guard<std::mutex> lock(logger_mutex_);
  if (logger_) return logger_;
  auto dispatcher = std::make_shared<Dispatcher>();
  auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console->set_level(spdlog::level::info);
  dispatcher->add(console, true);
  logger_ = std::make_shared<spdlog::logger>("naja", dispatcher);
  logger_->set_level(spdlog::level::trace);
  spdlog::set_default_logger(logger_);
  return logger_;
}

std::shared_ptr<Dispatcher> dispatcher() {
  return std::static_pointer_cast<Dispatcher>(ensureLogger()->sinks().front());
}
}  // namespace

void init() {
  (void)ensureLogger();
}

void initFromEnv() {
  init();

  if (const char* pattern = std::getenv("NAJA_LOG_PATTERN")) {
    setPattern(pattern);
  }

  if (const char* file = std::getenv("NAJA_LOG_FILE")) {
    addFileSink(file);
  }

  spdlog::cfg::load_env_levels();

  if (const char* level = std::getenv("NAJA_LOG_LEVEL")) {
    setLevel(levelFromString(level));
  }
}

std::shared_ptr<spdlog::logger> get() {
  return ensureLogger();
}

void setLevel(spdlog::level::level_enum level) {
  ensureLogger()->set_level(level);
}

void setPattern(const std::string& pattern) {
  dispatcher()->set_pattern(pattern);
}

void addConsoleSink(spdlog::level::level_enum level) {
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  sink->set_level(level);
  dispatcher()->add(sink, true);
}

void addFileSink(const std::string& path, spdlog::level::level_enum level) {
  auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path);
  sink->set_level(level);
  dispatcher()->add(sink);
}

//LCOV_EXCL_START
void flushEvery(std::chrono::seconds interval) {
  spdlog::flush_every(interval);
}
//LCOV_EXCL_STOP

ScopedCallbackSuppression::ScopedCallbackSuppression() { ++callback_suppression_; }
ScopedCallbackSuppression::~ScopedCallbackSuppression() { --callback_suppression_; }

SinkHandle addCallbackSink(
  std::function<void(spdlog::level::level_enum, std::string_view)> callback,
  spdlog::level::level_enum level) {
  auto sink = std::make_shared<CallbackSink>(std::move(callback));
  sink->set_level(level);
  return dispatcher()->add(sink);
}

void removeSink(SinkHandle handle) {
  dispatcher()->remove(handle);
}

void setConsoleLogging(bool enabled) {
  dispatcher()->setConsoleLogging(enabled);
}

void clearSinks() {
  dispatcher()->clear();
}

void shutdown() {
  std::lock_guard<std::mutex> lock(logger_mutex_);
  logger_.reset();
  spdlog::shutdown();
}

spdlog::level::level_enum levelFromString(const std::string& level) {
  return spdlog::level::from_str(level);
}

}  // namespace naja::log
