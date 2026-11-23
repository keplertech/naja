// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "NajaProgress.h"

#include "VerilogConstructor.h"

#include <optional>
#include <unordered_map>

#include "NLDB0.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLTerm.h"

namespace naja::NL {

class NLLibrary;
class SNLDesign;
class SNLScalarNet;

/// Load gate-level (structural) Verilog connectivity into SNL.
/// For Verilog or SystemVerilog RTL, use SNLSVConstructor's supported subset.
class SNLVRLConstructor: public naja::verilog::VerilogConstructor {
  public:
    struct Config {
      enum class ConflictingDesignNamePolicy {
        Forbid,        ///< Throw if a design with the same name already exists in the library.
        FirstOne,      ///< Keep the first definition and ignore subsequent ones (emit a warning).
        LastOne,       ///< Override the previous definition with the last one (emit a warning).
        VerifyEquality ///< Load the new definition into a temporary design and verify it matches the existing one.
      };

      bool                        preprocessEnabled_            {false};  ///< If true, preprocess Verilog sources before parsing.
      bool                        blackboxDetection_            {true};   ///< If true, detect blackbox designs.
      bool                        blackboxUnknownModules_       {false};  ///< If true, unknown module instantiations are created as AutoBlackBox designs.
      ConflictingDesignNamePolicy conflictingDesignNamePolicy_  {ConflictingDesignNamePolicy::Forbid};
      bool                        yosysSourceAttributes_        {false};  ///< Decode src attributes with explicitly selected Yosys provenance.
    };

    Config  config_ {};

    //As we don't know how many ports are instanciated before reaching
    //the last one, so we need to store all gate instance characteristics
    //before creating it in endInstantiation; 
    struct GateInstance {
      using Connections = std::vector<naja::verilog::Expression>;
      bool isValid() const;
      void reset();
      std::string getString() const;

      NLDB0::GateType gateType_     { NLDB0::GateType::Unknown };
      std::string     instanceName_ {};
      Connections     connections_  {};
    };

    void setProgressCallback(NajaProgressCallback callback) { progress_.setCallback(std::move(callback)); }

    SNLVRLConstructor() = delete;
    SNLVRLConstructor(const SNLVRLConstructor&) = delete;
    SNLVRLConstructor(NLLibrary* library);

    static SNLNet::Type VRLTypeToSNLType(const naja::verilog::Net::Type& type);
    static SNLTerm::Direction VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction);

    using Paths = std::vector<std::filesystem::path>;
    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);

    void setParseAttributes(bool parseAttributes) { parseAttributes_ = parseAttributes; }
    bool getParseAttributes() const { return parseAttributes_; }

    bool inFirstPass() const { return firstPass_; }
    void setFirstPass(bool mode) { firstPass_ = mode; }
    void startModule(const naja::verilog::Identifier& module) override;
    void moduleInterfaceSimplePort(const naja::verilog::Identifier& port) override;
    void moduleImplementationPort(const naja::verilog::Port& port) override;
    void moduleInterfaceCompletePort(const naja::verilog::Port& port) override;
    void addNet(const naja::verilog::Net& net) override;
    void addAssign(
      const naja::verilog::RangeIdentifiers& identifiers,
      const naja::verilog::Expression& expression) override;
    void startInstantiation(const naja::verilog::Identifier& model) override;
    void addParameterAssignment(
      const naja::verilog::Identifier& parameter,
      const naja::verilog::Expression& expression) override;
    void addInstance(const naja::verilog::Identifier& instance) override;
    void endInstantiation() override;
    void addInstanceConnection(
      const naja::verilog::Identifier& port,
      const naja::verilog::Expression& expression) override;
    void addOrderedInstanceConnection(
      size_t portIndex,
      const naja::verilog::Expression& expression) override;
    void startGateInstantiation(
      const naja::verilog::GateType& gateType) override;
    void addGateInstance(const naja::verilog::Identifier& id) override;
    void addGateOutputInstanceConnection(
      size_t portIndex,
      const naja::verilog::RangeIdentifiers identifiers) override;
    void addGateInputInstanceConnection(
      size_t portIndex,
      const naja::verilog::Expression& expression) override;
    void endGateInstantiation() override;
    void addDefParameterAssignment(
      const naja::verilog::Identifiers& hierarchicalParameter,
      const naja::verilog::ConstantExpression& expression) override;

    using Attributes = std::vector<naja::verilog::Attribute>;
    void addAttribute(
      const naja::verilog::Identifier& attributeName,
      const naja::verilog::ConstantExpression& expression) override;
    void endModule() override;

  private:
    NajaProgressReporter progress_;
    void restoreDeclarationAttributes();
    SourceRange attributesRange_ {};
    Attributes declarationAttributes_ {};
    Attributes instantiationAttributes_ {};
    void populateSource(NLObject* object, const Attributes& attributes);
    void populateDeclaration(NLObject* object);
    SourceRange gateSourceRange_ {};
    using PortSources = std::map<std::string, std::pair<SourceRange, Attributes>>;
    PortSources portSources_ {};
    SNLNet* getNetOrCreateImplicitNet(
      SNLDesign* design,
      const naja::verilog::RangeIdentifier& identifier);
    void createCurrentModuleAssignNets();
    void createConstantNets(
      const naja::verilog::Number& number,
      SNLInstance::Nets& nets);
    void collectConcatenationBitNets(
      const naja::verilog::Concatenation& concatenation,
      SNLInstance::Nets& bitNets);
    void collectIdentifierNets(
      const naja::verilog::RangeIdentifier& identifier,
      SNLInstance::Nets& bitNets);
    void currentInstancePortConnection(
      SNLTerm* term,
      const naja::verilog::Expression& expression);
    
    std::string getLocationString() const;

    struct ModuleDefKey {
      std::filesystem::path path_  {};
      size_t                line_  {0};
      size_t                column_{0};

      bool operator==(const ModuleDefKey& other) const {
        return path_ == other.path_ && line_ == other.line_ && column_ == other.column_;
      }
      bool operator!=(const ModuleDefKey& other) const { return not (*this == other); }
    };

    ModuleDefKey getCurrentModuleDefKey() const;
    bool isCurrentModuleSelected(const NLName& moduleName) const;
    void resetPerModuleState();

    bool                firstPass_                      {true};
    using SelectedModuleDefs = std::unordered_map<std::string, ModuleDefKey>;
    SelectedModuleDefs  selectedModuleDefs_             {};
    bool                skipCurrentModule_              {false};
    bool                parseAttributes_                {true};
    NLLibrary*          library_                        {nullptr};
    Attributes          nextObjectAttributes_           {};
    SNLDesign*          currentModule_                  {nullptr};
    std::string         currentModelName_               {};
    GateInstance        currentGateInstance_            {};
    SNLInstance*        currentInstance_                {nullptr};
    struct ParameterValue {
      std::string                source;     // original source text
      std::optional<std::string> canonical;  // canonical sized-binary form
    };
    using ParameterValues = std::map<std::string, ParameterValue>;
    ParameterValues     currentInstanceParameterValues_ {};
    SNLScalarNet*       currentModuleAssign0_           {nullptr};
    SNLScalarNet*       currentModuleAssign1_           {nullptr};
    SNLScalarNet*       currentModuleAssignX_           {nullptr};
    SNLScalarNet*       currentModuleAssignZ_           {nullptr};
    using InterfacePorts = std::vector<std::unique_ptr<naja::verilog::Port>>;
    using InterfacePortsMap = std::map<std::string, size_t>;
    InterfacePorts      currentModuleInterfacePorts_    {};
    InterfacePortsMap   currentModuleInterfacePortsMap_ {};
};

} // namespace naja::NL
