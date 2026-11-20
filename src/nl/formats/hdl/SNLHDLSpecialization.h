// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace naja::NL {
class NLLibrary;
class SNLDesign;

// Frontends supply resolved values in declaration order. Identity text must
// include type/shape information; literal text is valid in the source language.
struct SNLHDLParameter {
  std::string name;
  std::string identity;
  std::string literal;
  std::optional<std::string> nameValue;
  bool isDefault = false;
};

class SNLHDLSpecialization {
 public:
  static std::string signature(const std::vector<SNLHDLParameter>& parameters, const std::string& context = {});
  static std::string name(NLLibrary* library, const std::string& source,
      const std::vector<SNLHDLParameter>& parameters,
      const std::set<std::string>& sourceNames, const std::string& context = {},
      bool isTop = false);
  static void annotate(SNLDesign* design, const std::string& source,
      const std::vector<SNLHDLParameter>& parameters, const std::string& context = {});
  static bool isElaborated(const SNLDesign* design);
  static std::string attribute(const SNLDesign* design, const std::string& name);
  static std::string identifierValue(std::string value);
  static std::string integerValue(std::string value);
  static std::string bitValue(const std::string& bits, bool scalar = false);
};
}
