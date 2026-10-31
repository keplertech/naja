// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLLibrary.h"

#include <filesystem>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

namespace naja::NL {

class SNLDesign;

class VHDLSourceMap;

/// Temporary Naja adapter for scalar/vector combinational, scalar
/// clocked-register, and one-level direct-entity hierarchy proofs.
/// The standalone VHDL frontend remains independent of this class and SNL.
class VHDLConstructor {
  public:
    struct ConstructOptions {
      /// Retain copied semantic intent from RTL elaboration; no live AST pointers.
      bool keepASTLink {false};
      /// All warning occurrences, overwritten per load; nullopt is console-only.
      std::optional<std::filesystem::path> diagnosticsReportPath {"naja_vhdl_diagnostics.log"};
    };

    explicit VHDLConstructor(NLLibrary* library) : library_(library) {}
    VHDLConstructor(NLLibrary* library, const ConstructOptions& options)
      : library_(library), options_(options) {}

    /// Parse and lower one entity/architecture with one bit or constrained
    /// bit_vector expression, including logical gates and a conditional assignment,
    /// or one positive-edge process, using `rising_edge(clk)` or the equivalent
    /// explicit event/level guard, with an optional active-high scalar clock
    /// enable or active-high synchronous reset-to-zero, over bit ports and
    /// internal signals, with one
    /// scheduled write per signal destination.
    /// Scalar process variables may be temporary or retained state; retained
    /// variables must be assigned on every activation.
    SNLDesign* construct(std::string_view source) const;

    /// Package-only loads return nullptr and retain declarations in the library.
    /// Subsequent dependent RTL loads share those declarations. A single RTL
    /// entity requiring generic values is retained without elaboration when no
    /// top is specified, so a later parent can supply its generic actuals.
    /// Read, parse, and lower one VHDL source file, inferring a unique
    /// uninstantiated root entity unless top is specified.
    SNLDesign* constructFile(
      const std::filesystem::path& path, std::string_view top = {}) const;

    /// Register all files before elaboration; file order is immaterial.
    SNLDesign* constructFiles(
      const std::vector<std::filesystem::path>& paths, std::string_view top = {}) const;

    /// Parse a multi-unit source and lower the selected structural top plus its
    /// directly instantiated leaf entities. The hierarchy slice accepts
    /// positional, name-only `entity work.<name> port map (...)` associations.
    SNLDesign* construct(std::string_view source, std::string_view top) const;

  private:
    SNLDesign* constructSource(std::string_view source, std::string_view top,
                               const std::string& path) const;
    SNLDesign* constructInputs(
      const std::vector<std::pair<std::string, std::string>>& inputs,
      std::string_view top, bool batch, const VHDLSourceMap* mappedSource = nullptr) const;
    NLLibrary* library_;
    ConstructOptions options_;
};

}  // namespace naja::NL
