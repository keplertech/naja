// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "NajaPrivateProperty.h"

namespace slang::ast {
class Compilation;
class Symbol;
}

namespace slang::driver {
class Driver;
}

namespace slang::syntax {
class SyntaxTree;
}

namespace naja::NL {

class NLDB;
class NLLibrary;
class NLObject;

class SNLSVLiveASTLink {
  public:
    using Objects = std::vector<NLObject*>;
    SNLSVLiveASTLink() = default;
    ~SNLSVLiveASTLink();

    SNLSVLiveASTLink(const SNLSVLiveASTLink&) = delete;
    SNLSVLiveASTLink& operator=(const SNLSVLiveASTLink&) = delete;

    void bind(NLObject* object, const slang::ast::Symbol& symbol);
    const slang::ast::Symbol* getSymbol(const NLObject* object) const;
    const Objects& getObjects(const slang::ast::Symbol* symbol) const;
    bool hasSymbol(const slang::ast::Symbol* symbol) const;
    slang::ast::Compilation* getCompilation() const { return compilation_.get(); }

    std::unique_ptr<slang::driver::Driver> driver_ {};
    std::unique_ptr<slang::ast::Compilation> compilation_ {};
    std::vector<std::shared_ptr<slang::syntax::SyntaxTree>> syntaxTrees_ {};

  private:
    std::unordered_map<const NLObject*, const slang::ast::Symbol*> symbolByObject_ {};
    std::unordered_map<const slang::ast::Symbol*, Objects> objectsBySymbol_ {};
};

/// \brief Owns an SNLSVLiveASTLink as a private property of the NLDB it was
/// built for, instead of a process-global map keyed by NLDB*.
///
/// This ties the (heavy: slang Driver/Compilation/SyntaxTrees) AST state's
/// lifetime directly to its NLDB's own lifetime via NajaPrivateProperty's
/// existing onReleasedBy/preDestroy hooks: when the NLDB is destroyed, this
/// property is destroyed automatically as part of that same object graph,
/// with no separate bookkeeping to forget. This also makes multiple
/// concurrently-loaded SystemVerilog designs safe by construction -- each
/// NLDB carries its own link, so there's no shared global state one load
/// could clobber for another.
class SNLSVLiveASTLinkProperty: public naja::NajaPrivateProperty {
  public:
    using super = naja::NajaPrivateProperty;
    static const inline std::string Name = "SNLSVLiveASTLink";

    static SNLSVLiveASTLinkProperty* create(NLDB* db, std::unique_ptr<SNLSVLiveASTLink> link);

    std::string getName() const override { return Name; }
    std::string getString() const override { return Name; }

    SNLSVLiveASTLink* getLink() const { return link_.get(); }

  protected:
    SNLSVLiveASTLinkProperty(std::unique_ptr<SNLSVLiveASTLink> link);
    void preDestroy() override;

  private:
    std::unique_ptr<SNLSVLiveASTLink> link_;
};

class SNLSVLiveASTLinkRegistry {
  public:
    static void clearAll();
    static void clear(NLDB* db);
    static void store(NLDB* db, std::unique_ptr<SNLSVLiveASTLink> link);
    static const SNLSVLiveASTLink* get(NLDB* db);
    static const SNLSVLiveASTLink* getLatest();
    static const SNLSVLiveASTLink* findForObject(const NLObject* object);
    static const SNLSVLiveASTLink* findForSymbol(const slang::ast::Symbol* symbol);
};

class SNLSVConstructor {
  public:
    using Paths = std::vector<std::filesystem::path>;
    struct ConstructOptions {
      /// If true, detect empty port-only modules as blackboxes.
      bool blackboxDetection {true};
      std::optional<std::filesystem::path> elaboratedASTJsonPath {};
      /// Incremental diagnostics report. By default it is written to this path;
      /// reset the optional to emit diagnostics to the console only. An empty
      /// path remains invalid.
      std::optional<std::filesystem::path> diagnosticsReportPath {
        "naja_sv_diagnostics.log"};
      bool prettyPrintElaboratedASTJson {true};
      bool includeSourceInfoInElaboratedASTJson {true};
      /// Request retention of live frontend AST to SNL object links after load.
      bool keepASTLink {false};
      /// SystemVerilog preprocessor defines (passed as -D<name>[=<value>] to the driver).
      std::vector<std::string> preprocessorDefines {};
      /// SystemVerilog frontend warning names to suppress.
      std::vector<std::string> suppressWarnings {};
      /// Treat instantiations of unresolved (unknown) modules as auto-blackboxes
      /// instead of failing the load. Slang's UnknownModule/UnknownPrimitive
      /// errors are demoted, and each unknown instance gets an AutoBlackBox model
      /// whose terms are inferred from the instantiation's port connections
      /// (direction defaults to InOut, width to the connected expression width).
      bool blackboxUnknownModules {false};
      /// Preserve only the ports of modules with unsupported overlapping or
      /// dynamic writes to a memory from multiple sequential processes.
      bool blackboxMultiWriterMemories {false};
      /// Backend-specific option tokens. Only the active frontend (currently "slang") is accepted.
      std::map<std::string, std::vector<std::string>> frontendOptions {};
    };

    SNLSVConstructor() = delete;
    SNLSVConstructor(const SNLSVConstructor&) = delete;
    SNLSVConstructor(NLLibrary* library);

    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);
    void construct(const Paths& paths, const ConstructOptions& options);
    void construct(const std::filesystem::path& path, const ConstructOptions& options);

  private:
    NLLibrary* library_ {nullptr};
};

}  // namespace naja::NL
