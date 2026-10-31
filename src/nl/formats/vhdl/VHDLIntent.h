// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace naja::NL {
class NLDB;
class NLObject;

// Value records own all their data. No parser, SNL, or Python pointers escape.
struct VHDLIntentValue {
  using List = std::vector<VHDLIntentValue>;
  using Dict = std::map<std::string, VHDLIntentValue>;
  std::variant<std::monostate, bool, int64_t, std::string, List, Dict> value;
  VHDLIntentValue() = default;
  VHDLIntentValue(bool v): value(v) {}
  VHDLIntentValue(int64_t v): value(v) {}
  VHDLIntentValue(const char* v): value(std::string(v)) {}
  VHDLIntentValue(std::string v): value(std::move(v)) {}
  VHDLIntentValue(List v): value(std::move(v)) {}
  VHDLIntentValue(Dict v): value(std::move(v)) {}
};

class VHDLIntent {
 public:
  struct Session { bool valid = true; };
  using SessionPtr = std::shared_ptr<Session>;
  // Starting any VHDL load invalidates the previous session, including on error.
  static SessionPtr begin(NLDB* db, bool retain);
  static void invalidate(NLDB* db);
  static void attach(NLObject* object, const SessionPtr& session, VHDLIntentValue record);
  static const VHDLIntentValue* get(const NLObject* object);
  static bool available();
};
}
