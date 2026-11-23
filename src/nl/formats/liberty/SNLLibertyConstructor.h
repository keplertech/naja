// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NajaProgress.h"
#include <filesystem>
#include <vector>

namespace naja::NL {

class NLLibrary;

class SNLLibertyConstructor {
  public:
    struct Config {
      enum class ConflictingCellNamePolicy {
        Forbid,   ///< Throw if a cell with the same name already exists in the library.
        FirstOne  ///< Keep the first definition and ignore subsequent ones (emit a warning).
      };

      ConflictingCellNamePolicy conflictingCellNamePolicy_ {
        ConflictingCellNamePolicy::FirstOne
      };
    };

    Config config_ {};

    void setProgressCallback(NajaProgressCallback callback) { progress_.setCallback(std::move(callback)); }

    SNLLibertyConstructor() = delete;
    SNLLibertyConstructor(const SNLLibertyConstructor&) = delete;
    SNLLibertyConstructor(NLLibrary* library);

    using Paths = std::vector<std::filesystem::path>;
    static bool hasLibertyExtension(const std::filesystem::path& path);
    static bool hasCompressedLibertySignature(const std::filesystem::path& path);
    static bool isLibertyPath(const std::filesystem::path& path);

    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);
  private:
    NajaProgressReporter progress_;
    NLLibrary*  library_;
};

} // namespace naja::NL
