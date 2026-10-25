// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Parser.h"
#include "NLException.h"
#include <cstddef>
#include <vector>

namespace naja::NL {
class NLLibrary;
class SNLDesign;

// Preserve the syntax location until the adapter maps it to an input file.
struct VHDLRTLException : NLException {
  VHDLRTLException(const std::string& message, vhdl::SourceSpan location):
      NLException("VHDL constructor: " + message), span(location) {}
  vhdl::SourceSpan span;
};

// Offsets identify ownership in the adapter's combined parsing buffer.
class VHDLSourceMap;

struct VHDLLibrarySource {
  NLLibrary* library;
  size_t offset;
  size_t size;
};

// Statically indexed, two-state RTL profile. This path performs its own
// elaboration and type checks before returning a completed design.
bool requiresVHDLRTL(const vhdl::DesignFile& syntax);
SNLDesign* constructVHDLRTL(NLLibrary* library, const vhdl::DesignFile& syntax,
                          std::string_view top, std::string_view source,
                          const std::vector<VHDLLibrarySource>& libraries,
                          const VHDLSourceMap& sourceMap);
}
