// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLDB.h"
#include "NLException.h"
#include "NLLibrary.h"
#include <cctype>
#include <string>

namespace naja::NL {
// Basic logical names are case-insensitive; extended names retain their case.
inline std::string canonicalHDLLibraryName(std::string name) {
  if (name.empty() || name.front() != '\\')
    for (auto& c : name) c = (c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
  return name;
}

// Search only roots of this database. Never pick an arbitrary ambiguous match.
inline NLLibrary* findHDLLibrary(NLDB* db, const std::string& name) {
  const auto canonical = canonicalHDLLibraryName(name);
  NLLibrary* result = nullptr;
  for (auto* library : db->getLibraries()) {
    if (canonicalHDLLibraryName(library->getName().getString()) != canonical) continue;
    if (result) throw NLException("ambiguous HDL library: " + name);
    result = library;
  }
  return result;
}
}
