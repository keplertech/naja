// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "SNLTerm.h"

namespace naja::NL {

class SNLDesign;
class SNLParameter;
class SNLInstance;
class SNLInstTerm;
class SNLTerm;
class SNLNet;
class SNLBitNet;
class SNLBusNetBit;

struct SNLVRLDumperException: public std::exception {
  public:
    SNLVRLDumperException() = delete;
    SNLVRLDumperException(const SNLVRLDumperException&) = default;

    SNLVRLDumperException(const std::string& reason):
      std::exception(),
      reason_(reason)
    {}

    std::string getReason() const {
      return reason_;
    }

    //LCOV_EXCL_START
    const char* what() const noexcept override {
      return reason_.c_str();
    }
    //LCOV_EXCL_STOP

  private:
    const std::string reason_;
};

class SNLVRLDumper {
  public:
    SNLVRLDumper();

    enum class RTLInfoDumpMode {
      None,
      VerboseAttributes,
      CompactAttribute
    };

    class Configuration {
      public:
        Configuration() = default;
        Configuration(const Configuration&) = default;
        Configuration(Configuration&&) = default;
        Configuration& operator=(const Configuration&) = default;
        void setSingleFile(bool mode) { singleFile_ = mode; }
        bool isSingleFile() const { return singleFile_; }
        void setTopFileName(const std::string& name) { topFileName_ = name; }
        std::string getTopFileName() const { return topFileName_; }
        bool hasTopFileName() const { return not topFileName_.empty(); }
        void setLibraryFileName(const std::string& name) { libraryFileName_ = name; }
        std::string getLibraryFileName() const { return libraryFileName_; }
        bool hasLibraryFileName() const { return not libraryFileName_.empty(); }
        void setDumpHierarchy(bool mode) { dumpHierarchy_ = mode; }
        bool isDumpHierarchy() const { return dumpHierarchy_; }
        void setDumpRTLInfosAsAttributes(bool mode) {
          rtlInfoDumpMode_ = mode ? RTLInfoDumpMode::CompactAttribute : RTLInfoDumpMode::None;
        }
        bool isDumpRTLInfosAsAttributes() const {
          return rtlInfoDumpMode_ != RTLInfoDumpMode::None;
        }
        void setRTLInfoDumpMode(RTLInfoDumpMode mode) { rtlInfoDumpMode_ = mode; }
        RTLInfoDumpMode getRTLInfoDumpMode() const { return rtlInfoDumpMode_; }
        void setDumpAssignsAsInstances(bool mode) { dumpAssignsAsInstances_ = mode; }
        bool isDumpAssignsAsInstances() const { return dumpAssignsAsInstances_; }
        void setVerilatorSplitPackedSignals(bool mode) { verilatorSplitPackedSignals_ = mode; }
        bool isVerilatorSplitPackedSignals() const { return verilatorSplitPackedSignals_; }
      private:
        bool        singleFile_       {true};
        std::string topFileName_      {};
        std::string libraryFileName_  {};
        bool        dumpHierarchy_    {true};
        RTLInfoDumpMode rtlInfoDumpMode_ {RTLInfoDumpMode::CompactAttribute};
        bool        dumpAssignsAsInstances_ {false};
        bool        verilatorSplitPackedSignals_ {false};
    }; 
    void setConfiguration(const Configuration& configuration) { configuration_ = configuration; }
    // controls if dumper will dump a single file or a file per module. 
    void setSingleFile(bool mode);
    /**
     * \param name top file will be named "<name>.v".
     * controls the top file name or all design hierarchy file name, if "single file" is on. 
     * \sa setSingleFile
     */
    void setTopFileName(const std::string& name);
    void setLibraryFileName(const std::string& name);
    void setDumpHierarchy(bool mode);
    void setDumpRTLInfosAsAttributes(bool mode);
    void setRTLInfoDumpMode(RTLInfoDumpMode mode);
    void setDumpAssignsAsInstances(bool mode);
    void setVerilatorSplitPackedSignals(bool mode) {
      configuration_.setVerilatorSplitPackedSignals(mode);
    }

    /**
     * \param design SNLDesign to dump.
     * \param path directory path in which the dump will be created.
     * dump design in directory path
     * \sa setSingleFile setTopFileName
     */
    void dumpDesign(const SNLDesign* design, const std::filesystem::path& path);
    //dump design in stream o
    void dumpDesign(const SNLDesign* design, std::ostream& o);

    void dumpLibrary(const NLLibrary*, const std::filesystem::path& path);
    void dumpLibrary(const NLLibrary*, std::ostream& o);

    static std::string binStrToHexStr(std::string binStr);
  private:
    enum class AttributeDumpSite {
      Design,
      Instance,
      Net
    };

    void dumpAttributes(const NLObject*, std::ostream& o, AttributeDumpSite site);

    struct DetailedPerfReport {
      bool enabled {false};
      bool sessionActive {false};
      std::filesystem::path reportPath {};
      std::string context {};
      std::chrono::steady_clock::time_point sessionStart {};
      std::chrono::nanoseconds totalDuration {0};

      std::chrono::nanoseconds dumpAttributesDuration {0};
      size_t dumpAttributesCalls {0};
      size_t dumpAttributesEmptyCalls {0};
      size_t dumpAttributesNonEmptyCalls {0};
      size_t dumpedAttributesCount {0};
      std::chrono::nanoseconds dumpAttributesSNLAttributesDuration {0};
      size_t dumpedSNLAttributesCount {0};
      std::chrono::nanoseconds dumpAttributesRTLInfosDuration {0};
      size_t dumpedRTLInfosCount {0};

      std::chrono::nanoseconds dumpAttributesDesignDuration {0};
      size_t dumpAttributesDesignCalls {0};
      size_t dumpedAttributesDesignCount {0};

      std::chrono::nanoseconds dumpAttributesInstanceDuration {0};
      size_t dumpAttributesInstanceCalls {0};
      size_t dumpedAttributesInstanceCount {0};

      std::chrono::nanoseconds dumpAttributesNetDuration {0};
      size_t dumpAttributesNetCalls {0};
      size_t dumpedAttributesNetCount {0};

      std::chrono::nanoseconds dumpInterfaceDuration {0};
      size_t dumpInterfaceCalls {0};

      std::chrono::nanoseconds dumpNetsDuration {0};
      size_t dumpNetsCalls {0};

      std::chrono::nanoseconds dumpInstancesDuration {0};
      size_t dumpInstancesCalls {0};

      std::chrono::nanoseconds dumpTermAssignsDuration {0};
      size_t dumpTermAssignsCalls {0};

      std::chrono::nanoseconds dumpParametersDuration {0};
      size_t dumpParametersCalls {0};

      std::chrono::nanoseconds dumpOneDesignDuration {0};
      size_t dumpOneDesignCalls {0};

      std::chrono::nanoseconds dumpDesignStreamDuration {0};
      size_t dumpDesignStreamCalls {0};

      std::chrono::nanoseconds dumpDesignPathDuration {0};
      size_t dumpDesignPathCalls {0};

      std::chrono::nanoseconds dumpLibraryStreamDuration {0};
      size_t dumpLibraryStreamCalls {0};

      std::chrono::nanoseconds dumpLibraryPathDuration {0};
      size_t dumpLibraryPathCalls {0};
    };

    class DetailedPerfScopedTimer {
      public:
        DetailedPerfScopedTimer(
          DetailedPerfReport& report,
          std::chrono::nanoseconds& bucket,
          size_t& calls);
        ~DetailedPerfScopedTimer();

      private:
        DetailedPerfReport* report_ {nullptr};
        std::chrono::nanoseconds* bucket_ {nullptr};
        std::chrono::steady_clock::time_point start_ {};
    };

    class DetailedPerfSessionGuard {
      public:
        DetailedPerfSessionGuard(SNLVRLDumper& dumper, const std::string& context);
        ~DetailedPerfSessionGuard();

      private:
        SNLVRLDumper* dumper_ {nullptr};
        bool started_ {false};
    };

    void initializeDetailedPerfConfig();
    bool beginDetailedPerfSession(const std::string& context);
    void finalizeDetailedPerfSession();

    std::string getTopFileName(const SNLDesign* top) const;
    std::string getPrimitiveFileName() const;
    std::string getLibraryFileName(const NLLibrary* library) const;
    struct DesignAnonymousNaming {
      using TermNames = std::map<NLID::DesignObjectID, std::string>;
      std::string name_;
      TermNames   termNames_;
    };
    using DesignsAnonynousNaming = std::map<NLID, DesignAnonymousNaming>;
    struct DesignInsideAnonymousNaming {
      using InstanceNames = std::map<NLID::DesignObjectID, std::string>;
      using InstanceNameSet = std::set<std::string>;
      using NetNames = std::map<NLID::DesignObjectID, NLName>;
      using NetTermNameSet = std::set<NLName>;
      using UnusedWireKey = std::pair<NLID::DesignObjectID, size_t>;
      using UnusedWireNames = std::map<UnusedWireKey, NLName>;
      InstanceNames   instanceNames_    {};
      InstanceNameSet instanceNameSet_  {};
      NetNames        netNames_         {};
      NetTermNameSet  netTermNameSet_   {};
      UnusedWireNames unusedWireNames_   {};
    };
    static std::string createDesignName(const SNLDesign* design);
    static std::string createInstanceName(const SNLInstance* instance, DesignInsideAnonymousNaming& naming);
    static NLName createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming);
    static NLName createUnusedWireName(const SNLInstTerm* instTerm, DesignInsideAnonymousNaming& naming);
    static NLName getNetName(const SNLNet* net, const DesignInsideAnonymousNaming& naming);
    static NLName getUnusedWireName(const SNLInstTerm* instTerm, const DesignInsideAnonymousNaming& naming);
    static std::string getBitNetString(const SNLBitNet* bitNet, const DesignInsideAnonymousNaming& naming);
    void dumpOneDesign(const SNLDesign* design, std::ostream& o);
    void dumpNajaFAModel(std::ostream& o);
    void dumpNajaMux2Model(std::ostream& o);
    void dumpNajaAddSubModels(std::ostream& o);
    void dumpNajaMultiplyModel(std::ostream& o);
    void dumpNajaShiftModel(std::ostream& o);
    void dumpNajaVectorUpdateModel(std::ostream& o);
    void dumpNajaVectorSelectModel(std::ostream& o);
    void dumpNajaTableSelectModel(std::ostream& o);
    void dumpNajaDFFModel(std::ostream& o);
    void dumpNajaDLatchModel(std::ostream& o);
    void dumpNajaDFFNModel(std::ostream& o);
    void dumpNajaDFFRNModel(std::ostream& o);
    void dumpNajaDFFRModel(std::ostream& o);
    void dumpNajaDFFSModel(std::ostream& o);
    void dumpNajaDFFEModel(std::ostream& o);
    void dumpNajaDFFREModel(std::ostream& o);
    void dumpNajaDFFSEModel(std::ostream& o);
    void dumpNajaDFFSRModel(std::ostream& o);
    void dumpNajaDFFSRNModel(std::ostream& o);
    void dumpNajaDFFSSModel(std::ostream& o);
    void dumpNajaDFFSSNModel(std::ostream& o);
    void dumpNajaDFFSREModel(std::ostream& o);
    void dumpNajaDFFSRNEModel(std::ostream& o);
    void dumpNajaDFFSSEModel(std::ostream& o);
    void dumpNajaDFFSSNEModel(std::ostream& o);
    void dumpNajaDivModModel(std::ostream& o);
    void dumpNajaMemModel(std::ostream& o);
    void dumpNajaPrimitiveFile(const std::filesystem::path& path);
    void dumpParameter(const SNLParameter* parameter, std::ostream& o);
    void dumpParameters(const SNLDesign* design, std::ostream& o);
    void dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    bool dumpAssignInstance(const SNLInstance* instance, std::ostream& o, DesignInsideAnonymousNaming& naming);
    bool dumpInstance(const SNLInstance* instance, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void dumpInstParameters(const SNLInstance* instance, std::ostream& o);
    void dumpInstanceInterface(const SNLInstance* instance, std::ostream& o, const DesignInsideAnonymousNaming& naming);
    void dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    bool dumpNet(const SNLNet* net, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void collectUnusedWireNames(const SNLDesign* design, DesignInsideAnonymousNaming& naming);
    void dumpUnusedWireDeclarations(std::ostream& o, const DesignInsideAnonymousNaming& naming);

    void dumpTermNetAssign(
      const SNLDesign* design,
      const SNLTerm::Direction& direction,
      const std::string& termNetName,
      const std::string& netName,
      std::ostream& o);
    /**
     * Special method to dump assign representations of nets connected to terms
     */
    void dumpTermAssigns(const SNLDesign* design, std::ostream& o);
    void dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    using BitNetVector = std::vector<SNLBitNet*>;
    using InstTermVector = std::vector<const SNLInstTerm*>;
    void dumpInsTermConnectivity(
      const SNLTerm* term,
      BitNetVector& termNets,
      const InstTermVector& instTerms,
      std::ostream& o,
      const DesignInsideAnonymousNaming& naming);

    const SNLDesign*        publicDesign_           {nullptr};
    Configuration           configuration_          {};
    DesignsAnonynousNaming  designsAnonymousNaming_ {};
    DetailedPerfReport      detailedPerfReport_     {};
    bool                    emitNajaMemModel_       {false};
    bool                    emitNajaMux2Model_      {false};
    bool                    emitNajaTableSelectModel_ {false};
    bool                    emitNajaPrimitiveModels_ {false};
};

} // namespace naja::NL
