// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "DNL.h"
#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBitNet.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLUtils.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"
#include "SNLSVConstructorTestDetail.h"
#include "SNLSVConstructorException.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

size_t countSubstring(const std::string& text, const std::string& pattern) {
  size_t count = 0;
  size_t pos = 0;
  while ((pos = text.find(pattern, pos)) != std::string::npos) {
    ++count;
    pos += pattern.size();
  }
  return count;
}

class ScopedEnvVar {
  public:
    ScopedEnvVar(const char* name, const char* value): name_(name) {
      if (const char* existing = std::getenv(name)) {
        oldValue_ = existing;
      }
      setValue(value);
    }

    ~ScopedEnvVar() {
      if (oldValue_) {
        setValue(oldValue_->c_str());
      } else {
        unsetValue();
      }
    }

  private:
    void setValue(const char* value) const {
#if defined(_WIN32)
      _putenv_s(name_.c_str(), value);
#else
      setenv(name_.c_str(), value, 1);
#endif
    }

    void unsetValue() const {
#if defined(_WIN32)
      _putenv_s(name_.c_str(), "");
#else
      unsetenv(name_.c_str());
#endif
    }

    std::string name_;
    std::optional<std::string> oldValue_ {};
};

std::filesystem::path dumpTopAndGetVerilogPath(const SNLDesign* top,
                                               const std::string& outDirName,
                                               bool dumpRTLInfosAsAttributes = false) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= outDirName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  auto fileName = top->getName().getString() + ".v";
  dumper.setTopFileName(fileName);
  dumper.setSingleFile(true);
  dumper.setDumpRTLInfosAsAttributes(dumpRTLInfosAsAttributes);
  dumper.dumpDesign(top, outPath);
  return outPath / fileName;
}

size_t getPrimitiveWidth(const SNLInstance* instance) {
  if (!instance) {
    return 0;
  }
  if (auto* widthInstParam = instance->getInstParameter(NLName("WIDTH"))) {
    return static_cast<size_t>(std::stoull(widthInstParam->getValue()));
  }
  auto* model = instance->getModel();
  if (!model) {
    return 0;
  }
  if (auto* widthParam = model->getParameter(NLName("WIDTH"))) {
    return static_cast<size_t>(std::stoull(widthParam->getValue()));
  }
  return 1;
}

size_t countInstanceDrivers(const SNLBitNet* net) {
  size_t count = 0;
  for (auto* instTerm : net->getInstTerms()) {
    if (instTerm && instTerm->getDirection() == SNLTerm::Direction::Output) {
      ++count;
    }
  }
  return count;
}

std::vector<std::string> collectDanglingInternalInputTerms(SNLDesign* top) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::vector<std::string> offenders;
  for (naja::DNL::DNLID termID = 0; termID < dnl->getDNLTerms().size(); ++termID) {
    const auto& term = dnl->getDNLTerminalFromID(termID);
    if (term.isNull() || term.isTopPort()) {
      continue;
    }
    auto* bitTerm = term.getSnlBitTerm();
    if (bitTerm == nullptr || bitTerm->getDirection() != SNLTerm::Direction::Input) {
      continue;
    }
    const auto isoID = term.getIsoID();
    if (isoID == naja::DNL::DNLID_MAX) {
      continue;
    }
    const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
    if (iso.isConstant() || !iso.getDrivers().empty()) {
      continue;
    }

    std::ostringstream message;
    message << term.getDNLInstance().getFullPath() << "."
            << bitTerm->getName().getString() << "[" << bitTerm->getBit() << "]"
            << " iso=" << isoID
            << " readers=" << iso.getReaders().size()
            << " model=" << term.getDNLInstance().getSNLModel()->getName().getString();
    if (auto* snlInstance = term.getDNLInstance().getSNLInstance()) {
      if (snlInstance->hasRTLInfos()) {
        auto* rtlInfos = snlInstance->getRTLInfos();
        if (rtlInfos->hasInfo(NLName("sv_src_line"))) {
          message << " src_line=" << rtlInfos->getInfo(NLName("sv_src_line"));
        }
        if (rtlInfos->hasInfo(NLName("sv_src_file"))) {
          message << " src_file=" << rtlInfos->getInfo(NLName("sv_src_file"));
        }
      }
      if (auto* instTerm = snlInstance->getInstTerm(bitTerm)) {
        if (auto* net = instTerm->getNet()) {
          message << " net=" << net->getName().getString();
        }
      }
    }
    offenders.push_back(message.str());
  }
  naja::DNL::destroy();
  return offenders;
}

std::vector<std::string> collectUndrivenMemoryReadOutputs(SNLDesign* top) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::vector<std::string> offenders;
  for (auto* inst : top->getInstances()) {
    if (!NLDB0::isMemory(inst->getModel())) {
      continue;
    }
    auto* rdata = NLDB0::getMemoryReadData(inst->getModel());
    if (!rdata) {
      continue;
    }
    for (auto* bitTerm : rdata->getBits()) {
      auto* instTerm = inst->getInstTerm(bitTerm);
      if (!instTerm) {
        continue;
      }
      const auto termID = dnl->getTop().getChildInstance(inst).getTerminalFromBitTerm(bitTerm).getID();
      if (termID == naja::DNL::DNLID_MAX) {
        continue;
      }
      const auto& term = dnl->getDNLTerminalFromID(termID);
      const auto isoID = term.getIsoID();
      if (isoID == naja::DNL::DNLID_MAX) {
        continue;
      }
      const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
      if (!iso.getDrivers().empty()) {
        continue;
      }
      std::ostringstream message;
      message << inst->getName().getString() << "."
              << bitTerm->getName().getString() << "[" << bitTerm->getBit() << "]"
              << " iso=" << isoID
              << " readers=" << iso.getReaders().size();
      offenders.push_back(message.str());
    }
  }
  naja::DNL::destroy();
  return offenders;
}

std::string formatDNLTerm(const naja::DNL::DNLTerminalFull& term) {
  std::ostringstream message;
  if (term.isTopPort()) {
    message << term.getSnlBitTerm()->getDesign()->getName().getString() << ":"
            << term.getSnlBitTerm()->getName().getString() << "["
            << term.getSnlBitTerm()->getBit() << "]";
    return message.str();
  }
  message << term.getDNLInstance().getFullPath() << "."
          << term.getSnlBitTerm()->getName().getString() << "["
          << term.getSnlBitTerm()->getBit() << "]";
  return message.str();
}

std::vector<naja::DNL::DNLID> collectCombinationalInputTermIDs(
    const naja::DNL::DNLTerminalFull& driverTerm) {
  std::vector<naja::DNL::DNLID> termIDs;
  if (driverTerm.isNull() || driverTerm.isTopPort()) {
    return termIDs;
  }
  const auto combinationalInputs =
      SNLDesignModeling::getCombinatorialInputs(driverTerm.getSnlBitTerm());
  const auto& driverInstance = driverTerm.getDNLInstance();
  for (auto* inputBitTerm : combinationalInputs) {
    if (inputBitTerm == nullptr ||
        inputBitTerm->getDirection() == SNLTerm::Direction::Output) {
      continue;
    }
    const auto& inputTerm = driverInstance.getTerminalFromBitTerm(inputBitTerm);
    if (inputTerm.isNull()) {
      continue;
    }
    termIDs.push_back(inputTerm.getID());
  }
  std::sort(termIDs.begin(), termIDs.end());
  termIDs.erase(std::unique(termIDs.begin(), termIDs.end()), termIDs.end());
  return termIDs;
}

std::unordered_set<std::string> collectMemoryInputSupportNames(
    SNLDesign* top,
    SNLInstance* memoryInst,
    SNLBusTerm* memoryInput) {
  std::unordered_set<std::string> supportNames;
  if (!top || !memoryInst || !memoryInput) {
    return supportNames;
  }

  auto* universe = NLUniverse::get();
  if (!universe) {
    return supportNames;
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::unordered_set<naja::DNL::DNLID> visited;
  std::vector<naja::DNL::DNLID> pending;
  for (auto* bitTerm : memoryInput->getBits()) {
    if (auto* instTerm = memoryInst->getInstTerm(bitTerm)) {
      const auto& term = dnl->getTop().getChildInstance(memoryInst).getTerminal(instTerm);
      if (!term.isNull()) {
        pending.push_back(term.getID());
      }
    }
  }

  while (!pending.empty()) {
    const auto termID = pending.back();
    pending.pop_back();
    if (termID == naja::DNL::DNLID_MAX || !visited.insert(termID).second) {
      continue;
    }
    const auto& term = dnl->getDNLTerminalFromID(termID);
    if (term.isNull()) {
      continue;
    }
    if (term.isTopPort()) {
      supportNames.insert(term.getSnlBitTerm()->getName().getString());
      continue;
    }
    const auto isoID = term.getIsoID();
    if (isoID == naja::DNL::DNLID_MAX) {
      continue;
    }
    const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
    for (const auto driverID : iso.getDrivers()) {
      const auto& driver = dnl->getDNLTerminalFromID(driverID);
      if (driver.isTopPort()) {
        supportNames.insert(driver.getSnlBitTerm()->getName().getString());
        continue;
      }
      const auto inputs = collectCombinationalInputTermIDs(driver);
      pending.insert(pending.end(), inputs.begin(), inputs.end());
    }
  }
  naja::DNL::destroy();
  return supportNames;
}

bool collectMemoryBoundaryConeIssue(
    decltype(naja::DNL::get()) dnl,
    naja::DNL::DNLID currentTermID,
    std::unordered_set<naja::DNL::DNLID>& activeTerms,
    std::unordered_set<naja::DNL::DNLID>& provenTerms,
    std::vector<std::string>& path,
    std::string& issue) {
  if (currentTermID == naja::DNL::DNLID_MAX) {
    issue = "encountered an invalid DNL term while tracing a memory dependency";
    return true;
  }
  if (provenTerms.find(currentTermID) != provenTerms.end()) {
    return false;
  }
  if (!activeTerms.insert(currentTermID).second) {
    std::ostringstream message;
    message << "logical loop at " << formatDNLTerm(dnl->getDNLTerminalFromID(currentTermID));
    issue = message.str();
    return true;
  }
  const auto& term = dnl->getDNLTerminalFromID(currentTermID);
  path.push_back(formatDNLTerm(term));

  if (term.isTopPort() || term.isSequential()) {
    provenTerms.insert(currentTermID);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return false;
  }

  const auto isoID = term.getIsoID();
  if (isoID == naja::DNL::DNLID_MAX) {
    issue = "disconnected term " + formatDNLTerm(term);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }
  const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
  if (iso.isConstant()) {
    provenTerms.insert(currentTermID);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return false;
  }
  if (iso.getDrivers().empty()) {
    issue = "undriven term " + formatDNLTerm(term);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }
  if (iso.getDrivers().size() > 1) {
    std::ostringstream message;
    message << "multi-driver term " << formatDNLTerm(term);
    issue = message.str();
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }

  const auto& driverTerm = dnl->getDNLTerminalFromID(iso.getDrivers().front());
  if (driverTerm.isTopPort() || driverTerm.isSequential()) {
    provenTerms.insert(currentTermID);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return false;
  }
  const auto inputTermIDs = collectCombinationalInputTermIDs(driverTerm);
  if (inputTermIDs.empty()) {
    issue = "no combinational support terms for driver " + formatDNLTerm(driverTerm);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }

  for (const auto inputTermID : inputTermIDs) {
    if (collectMemoryBoundaryConeIssue(
            dnl, inputTermID, activeTerms, provenTerms, path, issue)) {
      path.pop_back();
      activeTerms.erase(currentTermID);
      return true;
    }
  }
  provenTerms.insert(currentTermID);
  path.pop_back();
  activeTerms.erase(currentTermID);
  return false;
}

std::optional<naja::DNL::DNLID> resolveTransparentDriverTarget(
    decltype(naja::DNL::get()) dnl,
    naja::DNL::DNLID termID) {
  std::unordered_set<naja::DNL::DNLID> visitedTerms;
  naja::DNL::DNLID currentTermID = termID;
  while (currentTermID != naja::DNL::DNLID_MAX &&
         visitedTerms.insert(currentTermID).second) {
    const auto& currentTerm = dnl->getDNLTerminalFromID(currentTermID);
    if (currentTerm.isNull()) {
      return std::nullopt;
    }

    if (currentTerm.getSnlBitTerm()->getDirection() != SNLTerm::Direction::Output) {
      const auto isoID = currentTerm.getIsoID();
      if (isoID == naja::DNL::DNLID_MAX) {
        return std::nullopt;
      }
      const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
      if (iso.isConstant() || iso.getDrivers().size() != 1) {
        return std::nullopt;
      }
      currentTermID = iso.getDrivers().front();
      continue;
    }

    auto* model = currentTerm.getDNLInstance().getSNLModel();
    if (model == nullptr || !NLDB0::isAssign(model)) {
      return currentTermID;
    }

    auto* inputBitTerm = NLDB0::getAssignInput();
    if (inputBitTerm == nullptr) {
      return std::nullopt;
    }
    const auto& inputTerm =
        currentTerm.getDNLInstance().getTerminalFromBitTerm(inputBitTerm);
    if (inputTerm.isNull()) {
      return std::nullopt;
    }
    currentTermID = inputTerm.getID();
  }
  return std::nullopt;
}

std::vector<std::string> collectTransparentCombinationalSelfReferences(
    SNLDesign* top,
    const std::vector<std::string>& outputNameFilters = {}) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();

  std::vector<std::string> offenders;
  for (naja::DNL::DNLID termID = 0; termID < dnl->getDNLTerms().size(); ++termID) {
    const auto& term = dnl->getDNLTerminalFromID(termID);
    if (term.isNull() || term.isTopPort()) {
      continue;
    }
    auto* bitTerm = term.getSnlBitTerm();
    if (bitTerm == nullptr || bitTerm->getDirection() != SNLTerm::Direction::Output) {
      continue;
    }
    const auto outputName = formatDNLTerm(term);
    if (!outputNameFilters.empty()) {
      bool matchesFilter = false;
      for (const auto& filter : outputNameFilters) {
        if (outputName.find(filter) != std::string::npos) {
          matchesFilter = true;
          break;
        }
      }
      if (!matchesFilter) {
        continue;
      }
    }

    const auto inputTermIDs = collectCombinationalInputTermIDs(term);
    if (inputTermIDs.empty()) {
      continue;
    }
    for (const auto inputTermID : inputTermIDs) {
      const auto resolvedDriver = resolveTransparentDriverTarget(dnl, inputTermID);
      if (!resolvedDriver.has_value() || *resolvedDriver != termID) {
        continue;
      }

      std::ostringstream message;
      message << outputName << " depends on transparent alias "
              << formatDNLTerm(dnl->getDNLTerminalFromID(inputTermID))
              << " that resolves back to the same output";
      offenders.push_back(message.str());
    }
  }

  naja::DNL::destroy();
  return offenders;
}

std::vector<std::string> collectUnsupportedMemoryBoundaryTerms(SNLDesign* top) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::vector<std::string> issues;
  std::unordered_set<naja::DNL::DNLID> provenTerms;

  auto addIssueForBitTerms = [&](SNLInstance* inst,
                                 const SNLDesignModeling::BitTerms& bitTerms,
                                 const char* role) {
    for (auto* bitTerm : bitTerms) {
      if (bitTerm == nullptr) {
        continue;
      }
      auto* instTerm = inst->getInstTerm(bitTerm);
      if (instTerm == nullptr) {
        continue;
      }
      const auto& dnlTerm =
          dnl->getTop().getChildInstance(inst).getTerminal(instTerm);
      if (dnlTerm.isNull()) {
        continue;
      }

      std::unordered_set<naja::DNL::DNLID> activeTerms;
      std::vector<std::string> path;
      std::string issue;
      if (!collectMemoryBoundaryConeIssue(
              dnl,
              dnlTerm.getID(),
              activeTerms,
              provenTerms,
              path,
              issue)) {
        continue;
      }

      std::ostringstream message;
      // Guard SEC memory-model regressions directly at the inferred-memory
      // boundary surface: every exposed dependency bit must have a supported
      // combinational cone so later PDR/SEC extraction does not have to skip
      // the memory term as opaque or broken.
      message << inst->getName().getString() << "."
              << bitTerm->getName().getString() << "[" << bitTerm->getBit() << "]"
              << " role=" << role << " issue=" << issue;
      if (!path.empty()) {
        message << " path=";
        for (size_t i = 0; i < path.size(); ++i) {
          if (i != 0) {
            message << " -> ";
          }
          message << path[i];
        }
      }
      issues.push_back(message.str());
    }
  };

  for (auto* inst : top->getInstances()) {
    if (!NLDB0::isMemory(inst->getModel()) ||
        !SNLDesignModeling::hasMemoryInterface(inst->getModel())) {
      continue;
    }
    const auto interface = SNLDesignModeling::getMemoryInterface(inst);
    for (const auto& readPort : interface.readPorts) {
      addIssueForBitTerms(inst, readPort.address, "read_address");
      addIssueForBitTerms(inst, readPort.data, "read_data");
      addIssueForBitTerms(inst, readPort.enables, "read_enable");
    }
    for (const auto& writePort : interface.writePorts) {
      addIssueForBitTerms(inst, writePort.address, "write_address");
      addIssueForBitTerms(inst, writePort.data, "write_data");
      addIssueForBitTerms(inst, writePort.mask, "write_mask");
      addIssueForBitTerms(inst, writePort.enables, "write_enable");
      for (const auto& extraInputs : writePort.extraWriteInputs) {
        addIssueForBitTerms(inst, extraInputs, "extra_write_input");
      }
    }
  }
  naja::DNL::destroy();
  return issues;
}

std::string formatStringVector(const std::vector<std::string>& values) {
  std::ostringstream stream;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      stream << "\n";
    }
    stream << values[i];
  }
  return stream.str();
}

void expectUnsupportedConstruct(
  SNLSVConstructor& constructor,
  const std::filesystem::path& svPath,
  std::initializer_list<const char*> expectedSubstrings,
  const SNLSVConstructor::ConstructOptions& options = {}) {
  try {
    constructor.construct(svPath, options);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    for (const auto* expected : expectedSubstrings) {
      EXPECT_NE(std::string::npos, reason.find(expected)) << reason;
    }
  }
}

size_t countMemoryInstances(SNLDesign* design) {
  size_t count = 0;
  for (auto* inst : design->getInstances()) {
    count += NLDB0::isMemory(inst->getModel()) ? 1 : 0;
  }
  return count;
}

}

class SNLSVConstructorTestMemoryInference: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
    if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ(0u, mux2Count);

  auto* model = memoryInst->getModel();
  ASSERT_NE(nullptr, model);
  EXPECT_TRUE(NLDB0::isMemory(model));
  EXPECT_EQ("4", model->getParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ("8", model->getParameter(NLName("WIDTH"))->getValue());

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  auto* abitsParam = memoryInst->getInstParameter(NLName("ABITS"));
  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  ASSERT_NE(nullptr, abitsParam);
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, rstEnableParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
  EXPECT_EQ("2", abitsParam->getValue());
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());
  EXPECT_EQ("0", rstEnableParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".DEPTH(4)"));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("naja_mux2"));
}

TEST_F(
    SNLSVConstructorTestMemoryInference,
    parseQDMemoryInferenceSharedSequentialKeepsVectorizedSideRegisterFlops) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_keeps_scalar_flops";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_keeps_scalar_flops.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_keeps_scalar_flops(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       inc_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] ptr_o,
  output logic [2:0] cnt_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [1:0] ptr_q;
  logic [1:0] ptr_n;
  logic [2:0] cnt_q;
  logic [2:0] cnt_n;

  always_comb begin
    mem_d = mem_q;
    ptr_n = ptr_q;
    cnt_n = cnt_q;
    if (inc_i) begin
      mem_d[addr_i] = data_i;
      ptr_n = ptr_q + 1'b1;
      cnt_n = cnt_q + 1'b1;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{default: '0};
      ptr_q <= '0;
      cnt_q <= '0;
    end else begin
      mem_q <= mem_d;
      ptr_q <= ptr_n;
      cnt_q <= cnt_n;
    end
  end

  assign data_o = mem_q[addr_i];
  assign ptr_o = ptr_q;
  assign cnt_o = cnt_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_keeps_scalar_flops"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  size_t dffrnBitCount = 0;
  size_t dffrnInstanceCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
    if (NLDB0::isDFFRN(inst->getModel())) {
      dffrnBitCount += getPrimitiveWidth(inst);
      ++dffrnInstanceCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);
  EXPECT_EQ(5u, dffrnBitCount);
  EXPECT_EQ(2u, dffrnInstanceCount);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWriteOnlySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_write_only_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_write_only_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_write_only_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = en_i;
    mem_d = mem_q;
    if (en_i) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_write_only_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());

  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_raddr")));
  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_rdata")));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNonBitstreamElementFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_non_bitstream_element_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_non_bitstream_element_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_non_bitstream_element_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct {
    logic [7:0] data;
    string      tag;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i].data = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i].data;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported SystemVerilog type not representable in SNL for symbol: mem_q"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00110011001000100001000100000000", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_reset_init_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ENABLE(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ASYNC(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ACTIVE_LOW(1)"));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00110011001000100001000100000000)"));

  const auto primitiveDumpPath = dumpedVerilog.parent_path() / "naja_primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitiveDumpPath));
  std::string primitiveDump = readTextFile(primitiveDumpPath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAsyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_async_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      mem_q <= '{8'hA0, 8'hB1, 8'hC2, 8'hD3};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_async_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b11010011110000101011000110100000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sync_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      mem_q <= '{8'h10, 8'h21, 8'h32, 8'h43};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_sync_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  auto* reset = NLDB0::getMemoryReset(memoryInst->getModel());
  ASSERT_NE(nullptr, reset);
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::SyncReset,
            SNLDesignModeling::getTermRole(reset));
  EXPECT_EQ(SNLDesignModeling::SNLActiveLevel::High,
            SNLDesignModeling::getResetActiveLevel(reset));


  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000011001100100010000100010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncLowBitwiseResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_low_bitwise_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (~rst_ni) begin
      mem_q <= '{8'h55, 8'h66, 8'h77, 8'h88};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_sync_low_bitwise_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  auto* reset = NLDB0::getMemoryReset(memoryInst->getModel());
  ASSERT_NE(nullptr, reset);
  EXPECT_EQ(SNLDesignModeling::SNLTermRole::SyncReset,
            SNLDesignModeling::getTermRole(reset));
  EXPECT_EQ(SNLDesignModeling::SNLActiveLevel::Low,
            SNLDesignModeling::getResetActiveLevel(reset));


  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b10001000011101110110011001010101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAsyncResetConditionMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_reset_condition_mismatch_fallback(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (~rst_i) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after async reset mismatch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncLogicalNotResetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_logical_not_reset_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_logical_not_reset_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_logical_not_reset_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (!rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(NLName("qd_memory_inference_sync_logical_not_reset_fallback"));
  ASSERT_NE(nullptr, top);
  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ("1", memoryInst->getInstParameter(NLName("RST_ENABLE"))->getValue());
  EXPECT_EQ("0", memoryInst->getInstParameter(NLName("RST_ASYNC"))->getValue());
  EXPECT_EQ("1", memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"))->getValue());
  EXPECT_EQ(
    "32'b00110011001000100001000100000000",
    memoryInst->getInstParameter(NLName("INIT"))->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitUnknownBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unknown_bits_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{8'hxx, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after unknown-bit reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitNonConstantFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_non_constant_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= data_i;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after non-constant reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitNarrowEntryZeroExtendSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_narrow_entry_zero_extend_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 4'h1;
        end else if (i == 1) begin
          mem_q[i] <= 4'h2;
        end else if (i == 2) begin
          mem_q[i] <= 4'h3;
        end else begin
          mem_q[i] <= 4'h4;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_narrow_entry_zero_extend_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000100000000110000001000000001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitOversizedEntryFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_oversized_entry_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 16'h1234;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after oversized reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitRealConstantsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_real_constants_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_real_constants_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_real_constants_supported(
  input  logic        clk_i,
  input  logic        rst_i,
  input  logic [1:0]  addr_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:3];
  logic [63:0] mem_d [0:3];

  localparam real ZERO_R  = 0.0;
  localparam real ONE_R   = 1.0;
  localparam real TWO_R   = 2.0;
  localparam real THREE_R = 3.0;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= ZERO_R;
        end else if (i == 1) begin
          mem_q[i] <= ONE_R;
        end else if (i == 2) begin
          mem_q[i] <= TWO_R;
        end else begin
          mem_q[i] <= THREE_R;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_real_constants_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);

  const std::array<uint64_t, 4> values{0, 1, 2, 3};
  std::string expectedInit = "256'b";
  for (auto it = values.rbegin(); it != values.rend(); ++it) {
    for (int bit = 63; bit >= 0; --bit) {
      expectedInit += ((*it >> bit) & 1ULL) ? '1' : '0';
    }
  }
  EXPECT_EQ(expectedInit, initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitPackedArrayElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_packed_array_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [0:3][7:0] INIT_PACKED = '{8'h11, 8'h22, 8'h33, 8'h44};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_PACKED[0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_PACKED[1];
        end else if (i == 2) begin
          mem_q[i] <= INIT_PACKED[2];
        end else begin
          mem_q[i] <= INIT_PACKED[3];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_packed_array_element_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitStructMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_struct_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_struct_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_struct_member_access_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [7:0] payload;
    logic       valid;
  } init_t;

  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam init_t INIT0 = '{payload: 8'h12, valid: 1'b0};
  localparam init_t INIT1 = '{payload: 8'h23, valid: 1'b1};
  localparam init_t INIT2 = '{payload: 8'h34, valid: 1'b0};
  localparam init_t INIT3 = '{payload: 8'h45, valid: 1'b1};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT0.payload;
        end else if (i == 1) begin
          mem_q[i] <= INIT1.payload;
        end else if (i == 2) begin
          mem_q[i] <= INIT2.payload;
        end else begin
          mem_q[i] <= INIT3.payload;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_struct_member_access_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_range_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [31:0] INIT_WORD = 32'h44332211;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_WORD[7:0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_WORD[15:8];
        end else if (i == 2) begin
          mem_q[i] <= INIT_WORD[23:16];
        end else begin
          mem_q[i] <= INIT_WORD[31:24];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_range_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitUnaryConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (!(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_unary_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitLiteralConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_literal_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (1'b1) begin
          mem_q[i] <= 8'hA5;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_literal_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100101101001011010010110100101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitBitwiseNotConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_bitwise_not_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_bitwise_not_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitUnaryNonConstantConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_non_constant_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~addr_i[0]) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitComparisonConstantConditionsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_comparison_constant_conditions_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i > 3) begin
          mem_q[i] <= 8'hF0;
        end else if (i >= 2) begin
          mem_q[i] <= 8'hA0;
        end else if (i == 1) begin
          mem_q[i] <= 8'hB0;
        end else if (i <= 0) begin
          mem_q[i] <= 8'hC0;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_comparison_constant_conditions_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100000101000001011000011000000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitInequalityConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_inequality_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i != 1) begin
          mem_q[i] <= 8'hD0;
        end else begin
          mem_q[i] <= 8'hE1;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_inequality_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b11010000110100001110000111010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitLogicalAndConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_logical_and_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if ((i < 2) && 1'b1) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_logical_and_condition_fallback"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00010001000100011010101010101010", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConditionalSideLogicSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_conditional_side_logic_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_conditional_side_logic_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_conditional_side_logic_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] alt_addr_i,
  input  logic [7:0] data_i,
  input  logic [7:0] alt_data_i,
  output logic [7:0] data_o,
  output logic       side_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    side_o = sel_i;
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[alt_addr_i] = alt_data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_conditional_side_logic_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("side_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSameGuardPartialWritesSingleWEDriver) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_guard_partial_writes_single_we_driver";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_same_guard_partial_writes_single_we_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_guard_partial_writes_single_we_driver(
  input  logic       clk_i,
  input  logic       valid_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] data_i,
  output logic [1:0] data_o
);
  logic [1:0] mem_q [0:3];
  logic [1:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (valid_i) begin
      mem_d[addr_i][0] = data_i[0];
      mem_d[addr_i][1] = data_i[1];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_guard_partial_writes_single_we_driver"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());

  size_t checkedWriteEnableNets = 0;
  for (auto net : top->getNets()) {
    auto* bitNet = dynamic_cast<SNLBitNet*>(net);
    if (!bitNet || bitNet->isUnnamed()) {
      continue;
    }
    const auto name = bitNet->getName().getString();
    if (name.find("mem_we_") == std::string::npos) {
      continue;
    }
    ++checkedWriteEnableNets;
    EXPECT_EQ(1u, countInstanceDrivers(bitNet)) << name;
  }
  EXPECT_EQ(2u, checkedWriteEnableNets);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopConditionalElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_conditional_else_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_conditional_else_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_conditional_else_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_d[i] = data_i;
      end else begin
        mem_d[i] = 8'h00;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_conditional_else_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopBreakSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_break_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_break_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_break_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      mem_d[i] = data_i;
      break;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_break_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCasezFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_casez_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_casez_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_casez_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    casez (mode_i)
      2'b1?: mem_d[0] = data_i;
      default: mem_d[addr_i] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIfFalseBranchFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_iffalse_branch_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[addr_i] += data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseItemFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[addr_i] += data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseDefaultFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_default_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_default_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_default_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[0] = data_i;
      default: mem_d[addr_i] += data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConditionGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_condition_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_condition_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_condition_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if ($urandom_range(1, 0)) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceNoIndexedWritesAfterLateConstantFalseGuardFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_no_indexed_writes_after_late_constant_false_guard(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (addr_i[0] && 1'b0) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseItemGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      $urandom_range(1, 0): mem_d[addr_i] = data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCompoundShadowActionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_compound_shadow_action_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_compound_shadow_action_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_compound_shadow_action_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNestedBaseCopyFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_base_copy_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_base_copy_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_base_copy_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d = mem_q;
    end
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseAndForSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_and_for_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_and_for_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_and_for_supported(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  input  logic [1:0] token_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = 1'b0;
    mem_d = mem_q;
    case (mode_i)
      2'b00: begin
        for (int i = 0; i < 2; i++) begin
          mem_d[i] = {7'b0, token_i[i]};
        end
      end
      2'b01: begin
        mem_d[addr_i] = data_i;
      end
      default: begin
        flag_o = 1'b1;
      end
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_case_and_for_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConstantConditionalsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_constant_conditionals_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_constant_conditionals_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_constant_conditionals_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (1'b0) begin
      mem_d[0] = 8'hAA;
    end
    if (1'b1) begin
      mem_d[1] = data_i;
    end else begin
      mem_d[1] = 8'h55;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_constant_conditionals_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSharedSequentialResetLoopSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_loop_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 8'h11;
        end else if (i == 1) begin
          mem_q[i] <= 8'h22;
        end else begin
          mem_q[i] <= 8'h00;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_loop_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000010001000010001", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top, "qd_memory_inference_shared_sequential_reset_loop_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00000000000000000010001000010001)"));
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresUnrelatedAlwaysFFChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      scratch_q <= 1'b0;
    else
      scratch_q <= en_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceUnpackedArrayEntrySupported) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "1");
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_unpacked_array_entry_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_unpacked_array_entry_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_unpacked_array_entry_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       flush_i,
  input  logic       we_i,
  input  logic [1:0] waddr_i,
  input  logic       wlane_i,
  input  logic [7:0] target_i,
  input  logic [1:0] raddr_i,
  input  logic       rlane_i,
  output logic       valid_o,
  output logic [7:0] target_o
);
  typedef struct packed {
    logic       valid;
    logic [7:0] target;
  } entry_t;

  entry_t mem_d [0:3][0:1];
  entry_t mem_q [0:3][0:1];

  always_comb begin
    mem_d = mem_q;
    if (we_i) begin
      mem_d[waddr_i][wlane_i].valid = 1'b1;
      mem_d[waddr_i][wlane_i].target = target_i;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++)
        mem_q[i] <= '{default: 0};
    end else if (flush_i) begin
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 2; j++)
          mem_q[i][j].valid <= 1'b0;
    end else begin
      mem_q <= mem_d;
    end
  end

  assign valid_o = mem_q[raddr_i][rlane_i].valid;
  assign target_o = mem_q[raddr_i][rlane_i].target;
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  try {
    constructor.construct(svPath);
  } catch (...) {
    (void)testing::internal::GetCapturedStdout();
    (void)testing::internal::GetCapturedStderr();
    throw;
  }
  const std::string output =
    testing::internal::GetCapturedStdout() + testing::internal::GetCapturedStderr();
  EXPECT_EQ(std::string::npos, output.find("was not inferred as naja_mem")) << output;

  auto* top = library_->getSNLDesign(
    NLName("qd_memory_inference_unpacked_array_entry_supported"));
  ASSERT_NE(nullptr, top);
  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ("18", memoryInst->getInstParameter(NLName("WIDTH"))->getValue());
  EXPECT_EQ("4", memoryInst->getInstParameter(NLName("DEPTH"))->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseGeneratedDirectSequentialMemoryWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "generated_direct_sequential_memory_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "generated_direct_sequential_memory_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module generated_direct_sequential_memory_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                    clk_i,
  input  logic                    w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]      w_data_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]      r_data_o
);
  if (width_p == 0 || els_p == 0) begin : z
    assign r_data_o = '0;
  end else begin : nz
    logic [width_p-1:0] mem [els_p-1:0];
    wire [addr_width_lp-1:0] r_addr_li = (els_p > 0) ? r_addr_i : '0;
    wire [addr_width_lp-1:0] w_addr_li = (els_p > 0) ? w_addr_i : '0;

    assign r_data_o = mem[r_addr_li];

    always_ff @(posedge clk_i) begin
      if (w_v_i) begin
        mem[w_addr_li] <= w_data_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("generated_direct_sequential_memory_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryMaskedBitWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_masked_bit_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_masked_bit_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_masked_bit_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                     clk_i,
  input  logic                     w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]       w_data_i,
  input  logic [width_p-1:0]       w_mask_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]       r_data_o
);
  logic [width_p-1:0] mem [els_p-1:0];

  assign r_data_o = mem[r_addr_i];

  always_ff @(posedge clk_i)
    if (w_v_i)
      for (integer i = 0; i < width_p; i=i+1)
        if (w_mask_i[i])
          mem[w_addr_i][i] <= w_data_i[i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_masked_bit_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialByteEnableMemoryWithRegisteredReadInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_byte_enable_memory_registered_read_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_byte_enable_memory_registered_read_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_byte_enable_memory_registered_read_supported #(
  parameter int unsigned ADDR_SIZE = 2,
  parameter int unsigned DATA_SIZE = 16,
  parameter int unsigned DEPTH = 4,
  parameter int unsigned NDATA = 2
) (
  input  logic                              clk_i,
  input  logic                              cs_i,
  input  logic                              we_i,
  input  logic [ADDR_SIZE-1:0]              addr_i,
  input  logic [NDATA-1:0][DATA_SIZE-1:0]   wdata_i,
  input  logic [NDATA-1:0][DATA_SIZE/8-1:0] wbyteenable_i,
  output logic [NDATA-1:0][DATA_SIZE-1:0]   rdata_o
);
  typedef logic [NDATA-1:0][DATA_SIZE-1:0] mem_t [DEPTH];
  mem_t mem;

  always_ff @(posedge clk_i) begin
    if (cs_i == 1'b1) begin
      if (we_i == 1'b1) begin
        for (int j = 0; j < NDATA; j++) begin
          for (int i = 0; i < DATA_SIZE/8; i++) begin
            if (wbyteenable_i[j][i]) begin
              mem[addr_i][j][i*8 +: 8] <= wdata_i[j][i*8 +: 8];
            end
          end
        end
      end else begin
        rdata_o <= mem[addr_i];
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_byte_enable_memory_registered_read_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("32", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
  EXPECT_EQ("4", wrPortsParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMultipleMemoriesPreservesResetAndWriteBranches) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_multiple_memories_preserve_branches";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_multiple_memories_preserve_branches.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_multiple_memories_preserve_branches(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       write_a0_i,
  input  logic       write_a1_i,
  input  logic       write_b_i,
  input  logic [1:0] addr_a0_i,
  input  logic [1:0] addr_a1_i,
  input  logic [1:0] addr_b_i,
  input  logic [7:0] data_a0_i,
  input  logic [7:0] data_a1_i,
  input  logic [7:0] data_b_i,
  input  logic [1:0] read_addr_i,
  output logic [7:0] read_a_o,
  output logic [7:0] read_b_o,
  output logic       side_o
);
  logic [7:0] mem_a [0:3];
  logic [7:0] mem_b [0:3];
  always @(posedge clk_i) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i = i + 1) begin
        mem_a[i] <= 8'h11;
        mem_b[i] <= 8'h22;
      end
      side_o <= 1'b0;
    end else begin
      if (write_a0_i)
        mem_a[addr_a0_i] <= data_a0_i;
      if (write_b_i)
        mem_b[addr_b_i] <= data_b_i;
      if (write_a1_i)
        mem_a[addr_a1_i] <= data_a1_i;
      side_o <= write_a0_i | write_b_i | write_a1_i;
    end
  end

  assign read_a_o = mem_a[read_addr_i];
  assign read_b_o = mem_b[read_addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("direct_sequential_multiple_memories_preserve_branches"));
  ASSERT_NE(nullptr, top);
  std::unordered_map<std::string, SNLInstance*> memories;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      memories.emplace(inst->getName().getString(), inst);
    }
  }
  ASSERT_EQ(2u, memories.size());
  ASSERT_TRUE(memories.contains("mem_a_mem"));
  ASSERT_TRUE(memories.contains("mem_b_mem"));

  auto verifyMemory = [&](const char* instanceName,
                          const char* expectedInit,
                          const char* expectedWritePorts,
                          std::initializer_list<const char*> expectedDataInputs) {
    auto* inst = memories.at(instanceName);
    auto* init = inst->getInstParameter(NLName("INIT"));
    auto* resetEnable = inst->getInstParameter(NLName("RST_ENABLE"));
    auto* resetAsync = inst->getInstParameter(NLName("RST_ASYNC"));
    auto* resetActiveLow = inst->getInstParameter(NLName("RST_ACTIVE_LOW"));
    auto* writePorts = inst->getInstParameter(NLName("WR_PORTS"));
    ASSERT_NE(nullptr, init);
    ASSERT_NE(nullptr, resetEnable);
    ASSERT_NE(nullptr, resetAsync);
    ASSERT_NE(nullptr, resetActiveLow);
    ASSERT_NE(nullptr, writePorts);
    EXPECT_EQ(expectedInit, init->getValue());
    EXPECT_EQ("1", resetEnable->getValue());
    EXPECT_EQ("0", resetAsync->getValue());
    EXPECT_EQ("1", resetActiveLow->getValue());
    EXPECT_EQ(expectedWritePorts, writePorts->getValue());

    const auto support = collectMemoryInputSupportNames(
      top,
      inst,
      NLDB0::getMemoryWriteData(inst->getModel()));
    for (const auto* expectedInput : expectedDataInputs) {
      EXPECT_TRUE(support.contains(expectedInput))
        << instanceName << " WDATA does not depend on " << expectedInput;
    }
  };

  verifyMemory(
    "mem_a_mem",
    "32'b00010001000100010001000100010001",
    "2",
    {"data_a0_i", "data_a1_i"});
  verifyMemory(
    "mem_b_mem",
    "32'b00100010001000100010001000100010",
    "1",
    {"data_b_i"});
  EXPECT_NE(nullptr, top->getNet(NLName("side_o")));
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  doNotWarnSuccessfullyInferredMultipleDirectSequentialMemories) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "1");

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "large_uninferred_direct_sequential_memory_warning";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "large_uninferred_direct_sequential_memory_warning.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module large_uninferred_direct_sequential_memory_warning(
  input  logic       clk_i,
  input  logic       we_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_a [0:3];
  logic [7:0] mem_b [0:3];

  always_ff @(posedge clk_i) begin
    if (we_i) begin
      mem_a[addr_i] <= data_i;
      mem_b[addr_i] <= data_i;
    end
  end

  assign data_o = mem_a[addr_i] ^ mem_b[addr_i];
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  try {
    constructor.construct(svPath);
  } catch (...) {
    (void)testing::internal::GetCapturedStdout();
    (void)testing::internal::GetCapturedStderr();
    throw;
  }
  const std::string output =
    testing::internal::GetCapturedStdout() + testing::internal::GetCapturedStderr();

  EXPECT_EQ(std::string::npos, output.find("was not inferred as naja_mem")) << output;

  auto top = library_->getSNLDesign(
    NLName("large_uninferred_direct_sequential_memory_warning"));
  ASSERT_NE(top, nullptr);
  size_t memoryCount = 0;
  for (auto inst : top->getInstances()) {
    memoryCount += NLDB0::isMemory(inst->getModel()) ? 1 : 0;
  }
  EXPECT_EQ(2u, memoryCount);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  rejectUnsupportedDirectMemoryCandidateWithoutPartialInferenceAndWarnOnce) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "1");
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unsupported_direct_memory_candidate_is_transactional";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "unsupported_direct_memory_candidate_is_transactional.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module unsupported_direct_memory_candidate_is_transactional(
  input logic clk_i, input logic we_i, input logic clear_i,
  input logic [1:0] addr_i, input logic [7:0] data_i,
  output logic [7:0] good_o, output logic [7:0] bad_o);
  logic [7:0] good_mem [0:3];
  logic [7:0] bad_mem [0:3];
  always_ff @(posedge clk_i) begin
    if (we_i) begin
      good_mem[addr_i] <= data_i;
      bad_mem[addr_i] <= data_i;
    end
    if (clear_i)
      bad_mem[addr_i] += 8'h01;
  end
  assign good_o = good_mem[addr_i];
  assign bad_o = bad_mem[addr_i];
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  std::string unsupportedReason;
  try {
    constructor.construct(svPath);
  } catch (const SNLSVConstructorException& e) {
    unsupportedReason = e.what();
  }
  const std::string output =
    testing::internal::GetCapturedStdout() + testing::internal::GetCapturedStderr();
  EXPECT_EQ(1u, countSubstring(output, "Memory 'bad_mem' was not inferred as naja_mem"))
    << output;
  EXPECT_NE(std::string::npos, output.find("unsupported direct inferred memory assignment action"))
    << output;
  EXPECT_NE(
    std::string::npos,
    unsupportedReason.find("mixed blocking and non-blocking assignments to procedural target 'bad_mem'"))
    << unsupportedReason;

  auto* top = library_->getSNLDesign(
    NLName("unsupported_direct_memory_candidate_is_transactional"));
  ASSERT_NE(nullptr, top);
  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (!NLDB0::isMemory(inst->getModel())) {
      continue;
    }
    ++memoryCount;
    EXPECT_EQ("good_mem_mem", inst->getName().getString());
  }
  EXPECT_EQ(1u, memoryCount);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseMultipleDirectMemoriesMaskedReadModifyWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "multiple_direct_memories_masked_read_modify_write";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "multiple_direct_memories_masked_read_modify_write.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module multiple_direct_memories_masked_read_modify_write(
  input logic clk_i, input logic we_a_i, input logic we_b_i,
  input logic [1:0] addr_i, input logic [7:0] data_a_i, input logic [7:0] data_b_i,
  input logic [7:0] mask_a_i, input logic [7:0] mask_b_i,
  output logic [7:0] data_a_o, output logic [7:0] data_b_o);
  logic [7:0] mem_a [0:3];
  logic [7:0] mem_b [0:3];
  always_ff @(posedge clk_i) begin
    if (we_a_i)
      mem_a[addr_i] <= (mem_a[addr_i] & ~mask_a_i) | (data_a_i & mask_a_i);
    if (we_b_i)
      mem_b[addr_i] <= (mem_b[addr_i] & ~mask_b_i) | (data_b_i & mask_b_i);
  end
  assign data_a_o = mem_a[addr_i];
  assign data_b_o = mem_b[addr_i];
endmodule
)";
  svFile.close();
  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("multiple_direct_memories_masked_read_modify_write"));
  ASSERT_NE(nullptr, top);
  std::unordered_map<std::string, SNLInstance*> memories;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      memories.emplace(inst->getName().getString(), inst);
    }
  }
  ASSERT_EQ(2u, memories.size());
  for (const auto& [name, dataInput] :
       std::array<std::pair<const char*, const char*>, 2>{{
         {"mem_a_mem", "data_a_i"}, {"mem_b_mem", "data_b_i"}}}) {
    ASSERT_TRUE(memories.contains(name));
    const auto support = collectMemoryInputSupportNames(
      top,
      memories.at(name),
      NLDB0::getMemoryWriteData(memories.at(name)->getModel()));
    EXPECT_TRUE(support.contains(dataInput));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  warnLargeUninferredMemoryDisabledByZeroThreshold) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "0");

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "large_uninferred_memory_warning_disabled";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "large_uninferred_memory_warning_disabled.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module large_uninferred_memory_warning_disabled(
  input  logic       clk_i,
  input  logic       we_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_a [0:3];
  logic [7:0] mem_b [0:3];

  always_ff @(posedge clk_i) begin
    if (we_i) begin
      mem_a[addr_i] <= data_i;
      mem_b[addr_i] <= data_i;
    end
  end

  assign data_o = mem_a[addr_i] ^ mem_b[addr_i];
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  try {
    constructor.construct(svPath);
  } catch (...) {
    (void)testing::internal::GetCapturedStdout();
    (void)testing::internal::GetCapturedStderr();
    throw;
  }
  const std::string output =
    testing::internal::GetCapturedStdout() + testing::internal::GetCapturedStderr();

  EXPECT_EQ(
    std::string::npos,
    output.find("was not inferred as naja_mem"))
    << output;
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  warnLargeUninferredMemoryInvalidThresholdUsesDefault) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "abc");

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "large_uninferred_memory_warning_invalid_threshold";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "large_uninferred_memory_warning_invalid_threshold.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module large_uninferred_memory_warning_invalid_threshold(
  input  logic       clk_i,
  input  logic       we_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_a [0:3];
  logic [7:0] mem_b [0:3];

  always_ff @(posedge clk_i) begin
    if (we_i) begin
      mem_a[addr_i] <= data_i;
      mem_b[addr_i] <= data_i;
    end
  end

  assign data_o = mem_a[addr_i] ^ mem_b[addr_i];
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  try {
    constructor.construct(svPath);
  } catch (...) {
    (void)testing::internal::GetCapturedStdout();
    (void)testing::internal::GetCapturedStderr();
    throw;
  }
  const std::string output =
    testing::internal::GetCapturedStdout() + testing::internal::GetCapturedStderr();

  EXPECT_EQ(
    std::string::npos,
    output.find("was not inferred as naja_mem"))
    << output;
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  warnLargeUninferredMemoryDeduplicatesPerSymbol) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "1");

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "large_uninferred_memory_warning_dedup";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "large_uninferred_memory_warning_dedup.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module large_uninferred_memory_warning_dedup(
  input  logic       clk_i,
  input  logic       we_i,
  input  logic       mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_a [0:3];
  logic [7:0] mem_b [0:3];

  always_ff @(posedge clk_i) begin
    if (we_i) begin
      case (mode_i)
        1'b0: begin
          mem_a[addr_i] <= data_i;
          mem_b[addr_i] <= data_i;
        end
        1'b1: begin
          mem_a[addr_i] <= data_i ^ 8'hff;
          mem_b[addr_i] <= data_i ^ 8'h55;
        end
      endcase
    end
  end

  assign data_o = mem_a[addr_i] ^ mem_b[addr_i];
endmodule
)";
  svFile.close();

  const auto reportPath = outPath / "diagnostics.txt";
  SNLSVConstructor::ConstructOptions options;
  options.diagnosticsReportPath = reportPath;

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  try {
    constructor.construct(svPath, options);
  } catch (...) {
    (void)testing::internal::GetCapturedStdout();
    (void)testing::internal::GetCapturedStderr();
    throw;
  }
  const std::string output =
    testing::internal::GetCapturedStdout() + testing::internal::GetCapturedStderr();

  EXPECT_EQ(
    1,
    countSubstring(output, "Memory 'mem_a' was not inferred as naja_mem"))
    << output;
  EXPECT_EQ(
    0,
    countSubstring(output, "Memory 'mem_b' was not inferred as naja_mem"))
    << output;
  const auto report = readTextFile(reportPath);
  EXPECT_EQ(
    1u,
    countSubstring(report, "Memory 'mem_a' was not inferred as naja_mem"));
  EXPECT_EQ(
    1u,
    countSubstring(report, "Memory 'mem_b' was not inferred as naja_mem"));
  EXPECT_EQ(
    2u,
    countSubstring(
      report,
      "code=\"uninferred_memory_generic_sequential_lowering\""));
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryAsyncLowResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_async_low_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_async_low_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_async_low_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       w_v_i,
  input  logic [1:0] w_addr_i,
  input  logic [7:0] w_data_i,
  output logic [7:0] r_data_o
);
  logic [7:0] mem [0:3];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem <= '{default: '0};
    end else begin
      for (int i = 0; i < 4; i++) begin
        if (w_v_i && (w_addr_i == i[1:0])) begin
          mem[i] <= w_data_i;
        end
      end
    end
  end

  assign r_data_o = mem[w_addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_async_low_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryAsyncResetConditionMismatchRejected) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_async_reset_condition_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_async_reset_condition_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_async_reset_condition_mismatch_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       w_v_i,
  input  logic [1:0] w_addr_i,
  input  logic [7:0] w_data_i,
  output logic [7:0] r_data_o
);
  logic [7:0] mem [0:3];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni) begin
      mem <= '{default: '0};
    end else begin
      for (int i = 0; i < 4; i++) begin
        if (w_v_i && (w_addr_i == i[1:0])) begin
          mem[i] <= w_data_i;
        end
      end
    end
  end

  assign r_data_o = mem[w_addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(constructor, svPath, {"unable to preserve asynchronous reset"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialWholeArrayCopyIgnoresUnaryExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    scratch_o = 2'b00;
    scratch_o++;
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMissingDefaultChainFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_missing_default_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_missing_default_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_missing_default_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialInvalidDefaultCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_invalid_default_commit_target(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_q;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialSimpleResetChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_simple_reset_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_simple_reset_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_simple_reset_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_simple_reset_chain"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000100001100110010001000010001", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetConstantFalseNoElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_constant_false_no_else(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
        if (1'b0) begin
          mem_q[i] <= 8'hAA;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_constant_false_no_else"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetIgnoresNonAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_ignores_non_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      case (addr_i)
        default: ;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_ignores_non_assignment"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetCaseAssignsTrackedLhsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o,
  output logic       other_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       other_q;
  logic       other_n;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    other_n = other_q;
    if (en_i) begin
      other_n = ~other_q;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      other_q <= 1'b0;
      case (addr_i)
        default: flag_q <= 1'b1;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      other_q <= other_n;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
  assign other_o = other_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialDynamicResetIndexFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_dynamic_reset_index_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[addr_i] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected dynamic reset index fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetMissingTargetAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_missing_target_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected missing reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetPartialInitializationFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_partial_initialization(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected partial reset initialization fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetCompoundAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_compound_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] += 8'h01;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected compound reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialSelfCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_self_commit_target_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_q;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected self commit target fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMismatchedCommitSignatureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_mismatched_commit_signature(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [9:0] mem_next_bad [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    for (int i = 0; i < 4; i++) begin
      mem_next_bad[i] = {2'b00, mem_d[i]};
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next_bad;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected mismatched commit signature fallback";
  } catch (const SNLSVConstructorException& e) {
    EXPECT_FALSE(std::string(e.what()).empty());
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresTopLevelConditionalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_top_level_conditional(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      scratch_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      if (en_i) begin
        scratch_q <= addr_i[0];
      end
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q ^ scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_top_level_conditional"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresNonAssignmentExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_non_assignment_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [1:0] scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      scratch_q <= 2'b00;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      ++scratch_q;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_non_assignment_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMultipleTargetAssignmentsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_multiple_target_assignments_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected shared sequential duplicate target assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitElseGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_else_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_else_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_else_guard_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_else_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceArithmeticIndexKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic [11:0] addr_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:63];
  logic [63:0] mem_d [0:63];
  logic [63:0] mem_next [0:63];
  logic [5:0]  slot_idx;

  always_comb begin
    slot_idx = addr_i - 12'hB03 + 12'd1;
    mem_d = mem_q;
    mem_d[slot_idx][31:0] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 64; i++) begin
      if (i < 32) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[slot_idx];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    generic_counter_d = generic_counter_q;
    data_o = '0;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      generic_counter_d[i] = generic_counter_q[i] + 1'b1;
    end

    if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBase + 1];
    end

    if (we_i) begin
      if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
        generic_counter_d[addr_i - CounterBase + 1][31:0] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleMemoryInferenceKeepsMemoryReadOutputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_memory_inference_keeps_memory_read_outputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_memory_inference_keeps_memory_read_outputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_memory_inference_keeps_memory_read_outputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    generic_counter_d = generic_counter_q;
    data_o = '0;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      generic_counter_d[i] = generic_counter_q[i] + 1'b1;
    end

    if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBase + 1];
    end

    if (we_i) begin
      if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
        generic_counter_d[addr_i - CounterBase + 1][31:0] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_memory_inference_keeps_memory_read_outputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto undrivenReadOutputs = collectUndrivenMemoryReadOutputs(top);
  EXPECT_TRUE(undrivenReadOutputs.empty()) << formatStringVector(undrivenReadOutputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseCva6PerfCounterStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  output logic [31:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBaseLo = 12'hB03;
  localparam logic [11:0] CounterBaseHi = 12'hB83;
  localparam logic [11:0] EventBase = 12'h323;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];
  logic [4:0]  mhpmevent_d[CounterNum:1];
  logic [4:0]  mhpmevent_q[CounterNum:1];
  logic [CounterNum:1] events;

  always_comb begin
    events = '0;
    for (int unsigned i = 1; i <= CounterNum; i++) begin
      events[i] = i[0];
    end
  end

  always_comb begin
    generic_counter_d = generic_counter_q;
    mhpmevent_d = mhpmevent_q;
    data_o = '0;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        if (events[i] && !mcountinhibit_i[i + 2]) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CounterBaseLo) && (addr_i < (CounterBaseLo + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBaseLo + 1][31:0];
    end else if ((addr_i >= CounterBaseHi) && (addr_i < (CounterBaseHi + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBaseHi + 1][63:32];
    end else if ((addr_i >= EventBase) && (addr_i < (EventBase + CounterNum))) begin
      data_o = {27'b0, mhpmevent_q[addr_i - EventBase + 1]};
    end

    if (we_i) begin
      if ((addr_i >= CounterBaseLo) && (addr_i < (CounterBaseLo + CounterNum))) begin
        generic_counter_d[addr_i - CounterBaseLo + 1][31:0] = data_i;
      end else if ((addr_i >= CounterBaseHi) && (addr_i < (CounterBaseHi + CounterNum))) begin
        generic_counter_d[addr_i - CounterBaseHi + 1][63:32] = data_i;
      end else if ((addr_i >= EventBase) && (addr_i < (EventBase + CounterNum))) begin
        mhpmevent_d[addr_i - EventBase + 1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  // Guard the CVA6 perf-counter regression: lower/upper CSR windows plus event
  // writes must not lower inferred-memory WE/WDATA inputs onto undriven internal
  // nets that later show up as SEC no-driver boundary failures.
  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseCva6PerfCounterFullDecodeStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [63:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  output logic [63:0] data_o
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  typedef logic [11:0] csr_addr_t;

  localparam csr_addr_t CSR_MHPM_COUNTER_3  = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3    = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3   = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H  = 12'hC83;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_d[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_q[MHPMCounterNum:1];
  logic        events[MHPMCounterNum:1];
  logic        read_access_exception;
  logic        update_access_exception;

  always_comb begin : mux_events
    events[MHPMCounterNum:1] = '{default: 1'b0};
    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 1'b0;
        5'b00001: events[i] = i[0];
        5'b00010: events[i] = addr_i[0];
        default:  events[i] = we_i;
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = '0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        if (events[i] && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CSR_MHPM_COUNTER_3) &&
        (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                 (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                 (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
      data_o[4:0] = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= CSR_HPM_COUNTER_3) &&
                 (addr_i < (CSR_HPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > CSR_HPM_COUNTER_3H) &&
                 (addr_i < (CSR_HPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= CSR_MHPM_COUNTER_3) &&
          (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i[31:0];
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                   (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i[31:0];
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                   (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  // Guard the fuller CVA6 performance-counter decode shape: mixed full-word,
  // lower-half, upper-half, and event-window accesses must not leave inferred
  // memory write enables or write-data mux inputs undriven.
  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseCva6PerfCounterEventEqualityStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [63:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  input  logic [7:0]  event_select_i,
  output logic [63:0] data_o
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  typedef logic [11:0] csr_addr_t;

  localparam csr_addr_t CSR_MHPM_COUNTER_3  = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3    = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3   = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H  = 12'hC83;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_d[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_q[MHPMCounterNum:1];
  logic        events[MHPMCounterNum:1];
  logic        read_access_exception;
  logic        update_access_exception;

  always_comb begin : mux_events
    events[MHPMCounterNum:1] = '{default: 1'b0};
    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 1'b0;
        5'b00001: events[i] = event_select_i[i];
        5'b00010: events[i] = addr_i[i % 12];
        5'b00011: events[i] = we_i;
        5'b00100: events[i] = debug_mode_i;
        default:  events[i] = event_select_i[(i + 1) % 8];
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = '0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        // Guard the CVA6 regression where the real perf-counter RTL uses an
        // explicit single-bit equality here. That comparison must not lower
        // inferred-memory write enables through an undriven xnor input.
        if (((events[i]) == 1'b1) && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CSR_MHPM_COUNTER_3) &&
        (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                 (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                 (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
      data_o[4:0] = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= CSR_HPM_COUNTER_3) &&
                 (addr_i < (CSR_HPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > CSR_HPM_COUNTER_3H) &&
                 (addr_i < (CSR_HPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= CSR_MHPM_COUNTER_3) &&
          (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i[31:0];
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                   (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i[31:0];
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                   (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseNearLiteralCva6PerfCountersMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven(
  input logic clk_i,
  input logic rst_ni,
  input logic debug_mode_i,
  input logic [11:0] addr_i,
  input logic we_i,
  input logic [63:0] data_i,
  output logic [63:0] data_o,
  input logic [1:0][3:0] commit_fu_i,
  input logic [1:0][3:0] commit_op_i,
  input logic [1:0][4:0] commit_rd_i,
  input logic [1:0] commit_ack_i,
  input logic l1_icache_miss_i,
  input logic l1_dcache_miss_i,
  input logic itlb_miss_i,
  input logic dtlb_miss_i,
  input logic sb_full_i,
  input logic if_empty_i,
  input logic ex_valid_i,
  input logic eret_i,
  input logic resolved_branch_valid_i,
  input logic resolved_branch_is_mispredict_i,
  input logic branch_exception_valid_i,
  input logic l1_icache_req_i,
  input logic [2:0] l1_dcache_req_i,
  input logic [2:0][7:0] miss_vld_bits_i,
  input logic i_tlb_flush_i,
  input logic stall_issue_i,
  input logic [31:0] mcountinhibit_i
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  localparam int unsigned NrCommitPorts = 2;
  localparam int unsigned NumPorts = 3;

  localparam logic [3:0] LOAD = 4'd1;
  localparam logic [3:0] STORE = 4'd2;
  localparam logic [3:0] CTRL_FLOW = 4'd3;
  localparam logic [3:0] ALU = 4'd4;
  localparam logic [3:0] MULT = 4'd5;
  localparam logic [3:0] FPU = 4'd6;
  localparam logic [3:0] FPU_VEC = 4'd7;
  localparam logic [3:0] ADD = 4'd8;
  localparam logic [3:0] JALR = 4'd9;

  typedef logic [11:0] csr_addr_t;
  localparam csr_addr_t CSR_MHPM_COUNTER_3 = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3 = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3 = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H = 12'hC83;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic read_access_exception, update_access_exception;
  logic events[MHPMCounterNum:1];
  logic [4:0] mhpmevent_d[MHPMCounterNum:1];
  logic [4:0] mhpmevent_q[MHPMCounterNum:1];
  logic [NrCommitPorts-1:0] load_event;
  logic [NrCommitPorts-1:0] store_event;
  logic [NrCommitPorts-1:0] branch_event;
  logic [NrCommitPorts-1:0] call_event;
  logic [NrCommitPorts-1:0] return_event;
  logic [NrCommitPorts-1:0] int_event;
  logic [NrCommitPorts-1:0] fp_event;

  always_comb begin : Mux
    events[MHPMCounterNum:1] = '{default: 0};
    load_event = '{default: 0};
    store_event = '{default: 0};
    branch_event = '{default: 0};
    call_event = '{default: 0};
    return_event = '{default: 0};
    int_event = '{default: 0};
    fp_event = '{default: 0};

    for (int unsigned j = 0; j < NrCommitPorts; j++) begin
      load_event[j] = commit_ack_i[j] & (commit_fu_i[j] == LOAD);
      store_event[j] = commit_ack_i[j] & (commit_fu_i[j] == STORE);
      branch_event[j] = commit_ack_i[j] & (commit_fu_i[j] == CTRL_FLOW);
      call_event[j] = commit_ack_i[j] & (commit_fu_i[j] == CTRL_FLOW &&
                                         (commit_op_i[j] == ADD || commit_op_i[j] == JALR) &&
                                         (commit_rd_i[j] == 5'd1 || commit_rd_i[j] == 5'd5));
      return_event[j] = commit_ack_i[j] & (commit_op_i[j] == JALR && commit_rd_i[j] == 5'd0);
      int_event[j] = commit_ack_i[j] & (commit_fu_i[j] == ALU || commit_fu_i[j] == MULT);
      fp_event[j] = commit_ack_i[j] & (commit_fu_i[j] == FPU || commit_fu_i[j] == FPU_VEC);
    end

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 0;
        5'b00001: events[i] = l1_icache_miss_i;
        5'b00010: events[i] = l1_dcache_miss_i;
        5'b00011: events[i] = itlb_miss_i;
        5'b00100: events[i] = dtlb_miss_i;
        5'b00101: events[i] = |load_event;
        5'b00110: events[i] = |store_event;
        5'b00111: events[i] = ex_valid_i;
        5'b01000: events[i] = eret_i;
        5'b01001: events[i] = |branch_event;
        5'b01010: events[i] = resolved_branch_valid_i && resolved_branch_is_mispredict_i;
        5'b01011: events[i] = branch_exception_valid_i;
        5'b01100: events[i] = |call_event;
        5'b01101: events[i] = |return_event;
        5'b01110: events[i] = sb_full_i;
        5'b01111: events[i] = if_empty_i;
        5'b10000: events[i] = l1_icache_req_i;
        5'b10001: events[i] = l1_dcache_req_i[0] || l1_dcache_req_i[1] || l1_dcache_req_i[2];
        5'b10010:
          events[i] = (l1_dcache_miss_i && miss_vld_bits_i[0] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[1] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[2] == 8'hFF);
        5'b10011: events[i] = i_tlb_flush_i;
        5'b10100: events[i] = |int_event;
        5'b10101: events[i] = |fp_event;
        5'b10110: events[i] = stall_issue_i;
        default: events[i] = 0;
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = 'b0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        // Guard the exact CVA6 perf-counter increment shape, including the
        // explicit single-bit equality that has shown up as an xnor gate in the
        // failing SEC memory-boundary report.
        if ((events[i]) == 1 && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
        (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum))) begin
      data_o = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= csr_addr_t'(CSR_HPM_COUNTER_3)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > csr_addr_t'(CSR_HPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
          (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i;
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                   (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i;
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                   (addr_i < csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum)) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);

  // Guard the exact CVA6 perf-counter regression that later surfaced in SEC:
  // inferred memory dependency pins such as WE/RADDR/WDATA must have a
  // supported combinational cone all the way back to top/seq/const terms, not
  // just "no globally dangling input terms" in the flattened netlist.
  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  EXPECT_TRUE(boundaryIssues.empty()) << formatStringVector(boundaryIssues);
}

std::string makeIndependentCva6PerfCountersSource(
    const std::string& moduleName,
    bool useConfigParameter = false) {
  std::ostringstream source;
  if (useConfigParameter) {
    source << R"(package independent_cva6_cfg_pkg;
  typedef struct packed {
    int unsigned XLEN;
    int unsigned MHPMCounterNum;
  } cva6_cfg_t;
  localparam cva6_cfg_t cva6_cfg = '{XLEN: 64, MHPMCounterNum: 3};
endpackage

module )" << moduleName << R"( import independent_cva6_cfg_pkg::*;
#(
  parameter cva6_cfg_t CVA6Cfg = cva6_cfg
) (
)";
  } else {
    source << "module " << moduleName << R"((
)";
  }
  source << R"(  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [63:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  input  logic [2:0]  event_inputs_i,
  input  logic [1:0][3:0] commit_fu_i,
  input  logic [1:0][3:0] commit_op_i,
  input  logic [1:0][4:0] commit_rd_i,
  input  logic [1:0] commit_ack_i,
  output logic [63:0] data_o
);
)";
  if (useConfigParameter) {
    source << R"(  localparam int unsigned XLEN = CVA6Cfg.XLEN;
  localparam int unsigned MHPMCounterNum = CVA6Cfg.MHPMCounterNum;
)";
  } else {
    source << R"(  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 3;
)";
  }
  source << R"(
  typedef logic [11:0] csr_addr_t;
  localparam csr_addr_t CSR_MHPM_COUNTER_3  = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3    = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3   = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H  = 12'hC83;

  localparam logic [3:0] LOAD      = 4'd1;
  localparam logic [3:0] STORE     = 4'd2;
  localparam logic [3:0] CTRL_FLOW = 4'd3;
  localparam logic [3:0] ADD       = 4'd4;
  localparam logic [3:0] JALR      = 4'd5;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_d[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_q[MHPMCounterNum:1];
  logic        events[MHPMCounterNum:1];
  logic [1:0]  load_event;
  logic [1:0]  store_event;
  logic [1:0]  branch_event;
  logic [1:0]  call_event;
  logic [1:0]  return_event;
  logic        read_access_exception;
  logic        update_access_exception;

  always_comb begin : Mux
    events[MHPMCounterNum:1] = '{default: 1'b0};
    load_event = '0;
    store_event = '0;
    branch_event = '0;
    call_event = '0;
    return_event = '0;

    for (int unsigned j = 0; j < 2; j++) begin
      load_event[j] = commit_ack_i[j] & (commit_fu_i[j] == LOAD);
      store_event[j] = commit_ack_i[j] & (commit_fu_i[j] == STORE);
      branch_event[j] = commit_ack_i[j] & (commit_fu_i[j] == CTRL_FLOW);
      call_event[j] = commit_ack_i[j] & (commit_fu_i[j] == CTRL_FLOW) &
                      ((commit_op_i[j] == ADD) | (commit_op_i[j] == JALR)) &
                      ((commit_rd_i[j] == 5'd1) | (commit_rd_i[j] == 5'd5));
      return_event[j] = commit_ack_i[j] & (commit_op_i[j] == JALR) &
                        (commit_rd_i[j] == 5'd0);
    end

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if (mhpmevent_q[i] == 5'b00000) begin
        events[i] = 1'b0;
      end else if (mhpmevent_q[i] == 5'b00001) begin
        events[i] = event_inputs_i[i-1];
      end else if (mhpmevent_q[i] == 5'b00010) begin
        events[i] = |load_event;
      end else if (mhpmevent_q[i] == 5'b00011) begin
        events[i] = |store_event;
      end else if (mhpmevent_q[i] == 5'b00100) begin
        events[i] = |branch_event;
      end else if (mhpmevent_q[i] == 5'b00101) begin
        events[i] = |call_event;
      end else if (mhpmevent_q[i] == 5'b00110) begin
        events[i] = |return_event;
      end else begin
        events[i] = we_i ^ addr_i[i % 12];
      end
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    mhpmevent_d = mhpmevent_q;
    data_o = '0;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        if (((events[i]) == 1'b1) && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CSR_MHPM_COUNTER_3) &&
        (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                 (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                 (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
      data_o[4:0] = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= CSR_HPM_COUNTER_3) &&
                 (addr_i < (CSR_HPM_COUNTER_3 + MHPMCounterNum))) begin
      data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
    end else if ((addr_i >= CSR_HPM_COUNTER_3H) &&
                 (addr_i < (CSR_HPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_HPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= CSR_MHPM_COUNTER_3) &&
          (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i[31:0];
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                   (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i[31:0];
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                   (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  return source.str();
}

SNLDesign* constructIndependentCva6PerfCounters(
    NLLibrary* library,
    const std::string& moduleName,
    bool useConfigParameter = false) {
  SNLSVConstructor constructor(library);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= moduleName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / (moduleName + ".sv");
  std::ofstream svFile(svPath);
  if (!svFile.good()) {
    ADD_FAILURE() << "Failed to create " << svPath.string();
    return nullptr;
  }
  svFile << makeIndependentCva6PerfCountersSource(
      moduleName, useConfigParameter);
  svFile.close();

  constructor.construct(svPath);
  return library->getSNLDesign(NLName(moduleName));
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseIndependentCva6PerfCountersModuleKeepsStructuredMemoryDependenciesSupported) {
  auto* top = constructIndependentCva6PerfCounters(
      library_,
      "independent_cva6_perf_counters_module_keeps_structured_memory_dependencies_supported");
  ASSERT_NE(top, nullptr);

  // Reproduce the CVA6 perf-counter inferred-memory boundary shape without an
  // external CVA6 checkout: lower/upper CSR windows, event CSR writes, explicit
  // event equality, and event muxing must all trace to supported roots.
  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  EXPECT_TRUE(boundaryIssues.empty()) << formatStringVector(boundaryIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseIndependentCva6PerfCountersModuleWithTargetConfigKeepsStructuredMemoryDependenciesSupported) {
  auto* top = constructIndependentCva6PerfCounters(
      library_,
      "independent_cva6_perf_counters_module_with_target_config_keeps_structured_memory_dependencies_supported",
      true);
  ASSERT_NE(top, nullptr);

  // Reproduce the target-config path from the real CVA6 test with a local
  // packed config parameter instead of cva6_config_pkg/build_config_pkg.
  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  EXPECT_TRUE(boundaryIssues.empty()) << formatStringVector(boundaryIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseIndependentCva6TopKeepsPerfCounterMemoryWriteDataBoundarySupported) {
  auto* top = constructIndependentCva6PerfCounters(
      library_,
      "independent_cva6_top_keeps_perf_counter_memory_write_data_boundary_supported");
  ASSERT_NE(top, nullptr);

  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  std::vector<std::string> perfCounterWriteDataIssues;
  for (const auto& issue : boundaryIssues) {
    if (issue.find("generic_counter_q_mem.WDATA") != std::string::npos) {
      perfCounterWriteDataIssues.push_back(issue);
    }
  }
  EXPECT_TRUE(perfCounterWriteDataIssues.empty())
      << formatStringVector(perfCounterWriteDataIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseIndependentCva6TopDoesNotCreateTransparentMuxSelfReferenceOnPerfCounterWriteDataPath) {
  auto* top = constructIndependentCva6PerfCounters(
      library_,
      "independent_cva6_top_no_transparent_mux_self_reference_on_perf_counter_write_data_path");
  ASSERT_NE(top, nullptr);

  const auto offenders = collectTransparentCombinationalSelfReferences(
      top,
      {"generic_counter_q_mem"});
  EXPECT_TRUE(offenders.empty()) << formatStringVector(offenders);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parsePerfCounterLikeArrayUpdateDoesNotCreateTransparentMuxSelfReference) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "perf_counter_like_array_update_no_transparent_mux_self_reference";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "perf_counter_like_array_update_no_transparent_mux_self_reference.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_like_array_update_no_transparent_mux_self_reference(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        we_i,
  input  logic [1:0]  addr_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] counter_d [3:1];
  logic [63:0] counter_q [3:1];

  always_comb begin
    counter_d = counter_q;
    for (int unsigned i = 1; i <= 3; i++) begin
      if (!we_i) begin
        counter_d[i] = counter_q[i] + 1'b1;
      end
    end
    if (we_i) begin
      counter_d[addr_i + 1][31:0] = data_i[31:0];
    end
    data_o = counter_q[addr_i + 1];
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      counter_q <= '{default: 0};
    end else begin
      counter_q <= counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("perf_counter_like_array_update_no_transparent_mux_self_reference"));
  ASSERT_NE(top, nullptr);

  const auto offenders = collectTransparentCombinationalSelfReferences(top);
  EXPECT_TRUE(offenders.empty()) << formatStringVector(offenders);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseNearLiteralCva6PerfCountersStructPortMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven(
  input logic clk_i,
  input logic rst_ni,
  input logic debug_mode_i,
  input logic [11:0] addr_i,
  input logic we_i,
  input logic [63:0] data_i,
  output logic [63:0] data_o,
  input logic [31:0] mcountinhibit_i,
  input logic l1_icache_miss_i,
  input logic l1_dcache_miss_i,
  input logic itlb_miss_i,
  input logic dtlb_miss_i,
  input logic sb_full_i,
  input logic if_empty_i,
  input logic eret_i,
  input logic i_tlb_flush_i,
  input logic stall_issue_i,
  input logic [2:0][7:0] miss_vld_bits_i,
  input logic [1:0] commit_ack_i
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  localparam int unsigned NrCommitPorts = 2;
  localparam int unsigned NumPorts = 3;
  localparam int unsigned DCACHE_SET_ASSOC = 8;

  localparam logic [3:0] LOAD = 4'd1;
  localparam logic [3:0] STORE = 4'd2;
  localparam logic [3:0] CTRL_FLOW = 4'd3;
  localparam logic [3:0] ALU = 4'd4;
  localparam logic [3:0] MULT = 4'd5;
  localparam logic [3:0] FPU = 4'd6;
  localparam logic [3:0] FPU_VEC = 4'd7;
  localparam logic [3:0] ADD = 4'd8;
  localparam logic [3:0] JALR = 4'd9;

  typedef logic [11:0] csr_addr_t;
  localparam csr_addr_t CSR_MHPM_COUNTER_3 = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3 = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3 = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H = 12'hC83;

  typedef struct packed {
    logic [3:0] fu;
    logic [3:0] op;
    logic [4:0] rd;
  } scoreboard_entry_t;

  typedef struct packed {
    logic valid;
  } exception_t;

  typedef struct packed {
    logic valid;
    logic is_mispredict;
  } bp_resolve_t;

  typedef struct packed {
    logic req;
  } icache_dreq_t;

  typedef struct packed {
    logic data_req;
  } dcache_req_i_t;

  scoreboard_entry_t [NrCommitPorts-1:0] commit_instr_i;
  exception_t ex_i;
  exception_t branch_exceptions_i;
  bp_resolve_t resolved_branch_i;
  icache_dreq_t l1_icache_access_i;
  dcache_req_i_t [2:0] l1_dcache_access_i;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic read_access_exception, update_access_exception;
  logic events[MHPMCounterNum:1];
  logic [4:0] mhpmevent_d[MHPMCounterNum:1];
  logic [4:0] mhpmevent_q[MHPMCounterNum:1];
  logic [NrCommitPorts-1:0] load_event;
  logic [NrCommitPorts-1:0] store_event;
  logic [NrCommitPorts-1:0] branch_event;
  logic [NrCommitPorts-1:0] call_event;
  logic [NrCommitPorts-1:0] return_event;
  logic [NrCommitPorts-1:0] int_event;
  logic [NrCommitPorts-1:0] fp_event;

  always_comb begin
    for (int unsigned j = 0; j < NrCommitPorts; j++) begin
      commit_instr_i[j].fu = (j == 0) ? LOAD : CTRL_FLOW;
      commit_instr_i[j].op = (j == 0) ? ADD : JALR;
      commit_instr_i[j].rd = (j == 0) ? 5'd1 : 5'd0;
    end
    ex_i.valid = l1_dcache_miss_i;
    branch_exceptions_i.valid = dtlb_miss_i;
    resolved_branch_i.valid = if_empty_i;
    resolved_branch_i.is_mispredict = sb_full_i;
    l1_icache_access_i.req = l1_icache_miss_i;
    for (int unsigned i = 0; i < NumPorts; i++) begin
      l1_dcache_access_i[i].data_req = miss_vld_bits_i[i][0];
    end
  end

  always_comb begin : Mux
    events[MHPMCounterNum:1] = '{default: 0};
    load_event = '{default: 0};
    store_event = '{default: 0};
    branch_event = '{default: 0};
    call_event = '{default: 0};
    return_event = '{default: 0};
    int_event = '{default: 0};
    fp_event = '{default: 0};

    for (int unsigned j = 0; j < NrCommitPorts; j++) begin
      load_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == LOAD);
      store_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == STORE);
      branch_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == CTRL_FLOW);
      call_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == CTRL_FLOW &&
                                         (commit_instr_i[j].op == ADD || commit_instr_i[j].op == JALR) &&
                                         (commit_instr_i[j].rd == 5'd1 || commit_instr_i[j].rd == 5'd5));
      return_event[j] = commit_ack_i[j] & (commit_instr_i[j].op == JALR && commit_instr_i[j].rd == 5'd0);
      int_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == ALU || commit_instr_i[j].fu == MULT);
      fp_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == FPU || commit_instr_i[j].fu == FPU_VEC);
    end

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 0;
        5'b00001: events[i] = l1_icache_miss_i;
        5'b00010: events[i] = l1_dcache_miss_i;
        5'b00011: events[i] = itlb_miss_i;
        5'b00100: events[i] = dtlb_miss_i;
        5'b00101: events[i] = |load_event;
        5'b00110: events[i] = |store_event;
        5'b00111: events[i] = ex_i.valid;
        5'b01000: events[i] = eret_i;
        5'b01001: events[i] = |branch_event;
        5'b01010: events[i] = resolved_branch_i.valid && resolved_branch_i.is_mispredict;
        5'b01011: events[i] = branch_exceptions_i.valid;
        5'b01100: events[i] = |call_event;
        5'b01101: events[i] = |return_event;
        5'b01110: events[i] = sb_full_i;
        5'b01111: events[i] = if_empty_i;
        5'b10000: events[i] = l1_icache_access_i.req;
        5'b10001: events[i] = l1_dcache_access_i[0].data_req ||
                              l1_dcache_access_i[1].data_req ||
                              l1_dcache_access_i[2].data_req;
        5'b10010:
          events[i] = (l1_dcache_miss_i && miss_vld_bits_i[0] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[1] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[2] == 8'hFF);
        5'b10011: events[i] = i_tlb_flush_i;
        5'b10100: events[i] = |int_event;
        5'b10101: events[i] = |fp_event;
        5'b10110: events[i] = stall_issue_i;
        default: events[i] = 0;
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = 'b0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        // Guard the full CVA6 perf-counter surface, including struct-field
        // event muxing and the single-bit equality compare that has shown up in
        // SEC as an undriven xnor input on inferred-memory write enables.
        if ((events[i]) == 1 && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
        (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum))) begin
      data_o = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= csr_addr_t'(CSR_HPM_COUNTER_3)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > csr_addr_t'(CSR_HPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
          (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i;
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                   (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i;
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                   (addr_i < csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum)) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseCva6StoreBufferStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        commit_i,
  input  logic        valid_i,
  input  logic        stall_st_pending_i,
  input  logic        data_gnt_i,
  input  logic        data_rvalid_i,
  input  logic [63:0] paddr_i,
  input  logic [63:0] data_i,
  input  logic [7:0]  be_i,
  input  logic [1:0]  data_size_i,
  input  logic [1:0]  speculative_read_pointer_q,
  input  logic [1:0]  speculative_write_pointer_q,
  input  logic [1:0]  commit_read_pointer_q,
  input  logic [1:0]  commit_write_pointer_q,
  output logic [63:0] req_data_o
);
  localparam int unsigned DEPTH = 4;
  typedef struct packed {
    logic [63:0] address;
    logic [63:0] data;
    logic [7:0]  be;
    logic [1:0]  data_size;
    logic        valid;
    logic        wait_rvalid;
  } entry_t;

  entry_t speculative_queue_n[DEPTH-1:0], speculative_queue_q[DEPTH-1:0];
  entry_t commit_queue_n[DEPTH-1:0],      commit_queue_q[DEPTH-1:0];

  always_comb begin : core_if
    speculative_queue_n = speculative_queue_q;
    if (valid_i) begin
      speculative_queue_n[speculative_write_pointer_q].address = paddr_i;
      speculative_queue_n[speculative_write_pointer_q].data = data_i;
      speculative_queue_n[speculative_write_pointer_q].be = be_i;
      speculative_queue_n[speculative_write_pointer_q].data_size = data_size_i;
      speculative_queue_n[speculative_write_pointer_q].valid = 1'b1;
      speculative_queue_n[speculative_write_pointer_q].wait_rvalid = 1'b0;
    end
    if (commit_i) begin
      speculative_queue_n[speculative_read_pointer_q].valid = 1'b0;
    end
  end

  always_comb begin : store_if
    commit_queue_n = commit_queue_q;

    if (commit_queue_q[commit_read_pointer_q].valid &&
        !stall_st_pending_i &&
        !commit_queue_q[commit_read_pointer_q].wait_rvalid) begin
      if (data_gnt_i) begin
        if (data_rvalid_i) begin
          commit_queue_n[commit_read_pointer_q].valid = 1'b0;
        end else begin
          commit_queue_n[commit_read_pointer_q].wait_rvalid = 1'b1;
        end
      end
    end

    if (commit_queue_q[commit_read_pointer_q].valid &&
        commit_queue_q[commit_read_pointer_q].wait_rvalid &&
        data_rvalid_i) begin
      commit_queue_n[commit_read_pointer_q].valid = 1'b0;
    end

    if (commit_i) begin
      commit_queue_n[commit_write_pointer_q] =
        speculative_queue_q[speculative_read_pointer_q];
    end
  end

  assign req_data_o = commit_queue_q[commit_read_pointer_q].data;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      speculative_queue_q <= '{default: 0};
      commit_queue_q <= '{default: 0};
    end else begin
      speculative_queue_q <= speculative_queue_n;
      commit_queue_q <= commit_queue_n;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  // Guard the CVA6 store-buffer regression: packed-struct memory entries that
  // are copied wholesale between queues and then partially updated must not
  // leave internal write-data mux inputs undriven.
  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleWriteOnlyMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  output logic        flag_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    flag_o = we_i;
    generic_counter_d = generic_counter_q;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      generic_counter_d[i] = generic_counter_q[i] + 1'b1;
    end

    if (we_i) begin
      if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
        generic_counter_d[addr_i - CounterBase + 1][31:0] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleReadOnlyMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    generic_counter_d = generic_counter_q;
    data_o = '0;

    if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBase + 1];
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(0u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseArithmeticDynamicElementSelectKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "arithmetic_dynamic_element_select_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "arithmetic_dynamic_element_select_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module arithmetic_dynamic_element_select_keeps_internal_inputs_driven(
  input  logic [11:0] addr_i,
  input  logic [31:0] data_i,
  output logic        flag_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] state_q[CounterNum:1];
  logic [63:0] state_d[CounterNum:1];

  always_comb begin
    flag_o = 1'b0;
    state_d = '{default: 64'h0};
    state_d[addr_i - CounterBase + 1][31:0] = data_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("arithmetic_dynamic_element_select_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceDirectArithmeticPartialWriteKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic [11:0] addr_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:63];
  logic [63:0] mem_d [0:63];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i - 12'hB03 + 12'd1][31:0] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceLoopIndexWritesKeepInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_index_writes_keep_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_loop_index_writes_keep_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_index_writes_keep_internal_inputs_driven(
  input  logic        clk_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;

  logic [63:0] mem_q[CounterNum:1];
  logic [63:0] mem_d[CounterNum:1];

  always_comb begin
    mem_d = mem_q;
    for (int unsigned i = 1; i <= CounterNum; i++) begin
      mem_d[i] = mem_q[i] + 1'b1;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_loop_index_writes_keep_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceDynamicSelfReadWriteKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven(
  input  logic       clk_i,
  input  logic [2:0] addr_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:7];
  logic [63:0] mem_d [0:7];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = mem_q[addr_i] + 1'b1;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (i == 0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitLateConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_late_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && 1'b0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_late_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitConstantSelectorFastPathsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_selector_fast_paths_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[1] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_constant_selector_fast_paths_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[0][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitRhsSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_rhs_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[0];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextDynamicIndexedCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_dynamic_indexed_commit_target_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] commit_idx_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[commit_idx_i] = mem_d[commit_idx_i];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed commit target";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIfTrueIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iftrue_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] += mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifTrue branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIfFalseIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iffalse_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] += mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifFalse branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIgnoresEmptyAndUnrelatedCommitCandidates) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_empty_and_unrelated_commit_candidates(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    ;
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_empty_and_unrelated_commit_candidates"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIgnoresNonAssignmentCommitCandidate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_non_assignment_commit_candidate";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_non_assignment_commit_candidate.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_non_assignment_commit_candidate(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    case (addr_i)
      default: ;
    endcase
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_non_assignment_commit_candidate"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextCompoundCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_compound_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_compound_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_compound_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextSelfCopyCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_self_copy_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_self_copy_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_self_copy_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_next;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitFieldFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_field_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_field_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_field_fallback_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i].mode == 2'b11) begin
        mem_next[i].mode = mem_q[i].mode;
      end
      if (mem_d[i].access == 2'b01) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_field_fallback_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialFieldWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_field_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_field_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_field_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [5:0] payload_i,
  output logic [5:0] payload_o,
  output logic [1:0] reserved_o
);
  typedef struct packed {
    logic [5:0] payload;
    logic [1:0] reserved;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].reserved = 2'b00;
    mem_d[addr_i].payload = payload_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign payload_o = mem_q[addr_i].payload;
  assign reserved_o = mem_q[addr_i].reserved;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_field_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialSliceWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_slice_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_slice_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_slice_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] upper_i,
  input  logic [1:0] lower_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][7:4] = upper_i;
    mem_d[addr_i][1:0] = lower_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_slice_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePackedElementWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_packed_element_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_packed_element_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_packed_element_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_packed_element_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceDynamicPackedElementWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_packed_element_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_packed_element_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_packed_element_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] bit_sel_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][bit_sel_i] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("qd_memory_inference_dynamic_packed_element_write_supported"));
  ASSERT_NE(nullptr, top);

  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ("8", memoryInst->getInstParameter(NLName("WIDTH"))->getValue());
  EXPECT_EQ("4", memoryInst->getInstParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ("8", memoryInst->getInstParameter(NLName("WR_PORTS"))->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangePackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][8] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedUpRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_up_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_up_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_up_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][2 +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_up_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_or_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if ((mem_d[i].mode == 2'b11) || (mem_d[i].access == 2'b01)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_or_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitSingleBitEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_equality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitSingleBitInequalitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_inequality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_inequality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_inequality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] != mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_inequality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitEqualityOperandFailureFallback) {
  for (const auto* side : {"lhs", "rhs"}) {
    SCOPED_TRACE(side);
    auto* library = NLLibrary::create(library_->getDB(), NLName(side));
    SNLSVConstructor constructor(library);
    std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
    outPath = outPath / (std::string("qd_next_local_commit_equality_failure_") + side);
    if (std::filesystem::exists(outPath)) {
      std::filesystem::remove_all(outPath);
    }
    std::filesystem::create_directory(outPath);

    const auto svPath = outPath / "qd_next_local_commit_equality_operand_failure_fallback.sv";
    std::ofstream svFile(svPath);
    ASSERT_TRUE(svFile.good());
    svFile
      << R"(module qd_next_local_commit_equality_operand_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if ()" << (std::string(side) == "lhs"
        ? "mem_d[0][0] == mem_d[i][0]"
        : "mem_d[i][0] == mem_d[0][0]") << R"() begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
    svFile.close();

    expectUnsupportedConstruct(
      constructor,
      svPath,
      {"unsupported statement pattern for sequential lowering"});
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_and_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndLhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_lhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[0][0] && mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, wildcardLocalConditionsKeepMemoryInference) {
  for (const auto* variant : {"eq", "ne", "signed_eq", "signed_ne"}) {
    SCOPED_TRACE(variant);
    const bool isSigned = std::string(variant).find("signed_") == 0;
    const auto* op = std::string(variant).find("eq") != std::string::npos ? "==?" : "!=?";
    auto* library = NLLibrary::create(library_->getDB(), NLName(variant));
    SNLSVConstructor constructor(library);
    const auto outPath = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) /
      (std::string("wildcard_memory_") + variant);
    std::filesystem::create_directories(outPath);
    const auto svPath = outPath / "input.sv";
    std::ofstream source(svPath);
    source << R"(module wildcard_memory(
  input logic clk,
  input logic [1:0] addr,
  input logic [6:0] data,
  output wire [6:0] result
);
  logic )" << (isSigned ? "signed " : "") << R"([6:0] current [0:3], updated [0:3], next_value [0:3];
  always_comb begin
    updated = current;
    updated[addr] = data;
  end
  always_comb begin
    for (int i = 0; i < 4; i++) begin
      next_value[i] = updated[i];
      if ((updated[i] )" << op << (isSigned ? " 7'sb1?x0z?1" : " 7'b1?x0z?1") << R"() && (updated[i] ==? 'z))
        next_value[i] = current[i];
    end
  end
  always_ff @(posedge clk) current <= next_value;
  assign result = current[addr];
endmodule
)";
    source.close();
    constructor.construct(svPath);
    auto* top = library->getSNLDesign(NLName("wildcard_memory"));
    ASSERT_NE(nullptr, top);
    size_t memories = 0;
    for (auto* instance : top->getInstances()) {
      memories += NLDB0::isMemory(instance->getModel());
    }
    EXPECT_EQ(1u, memories);
    SNLVRLDumper dumper;
    dumper.setTopFileName("netlist.v");
    dumper.dumpDesign(top, outPath);
    ASSERT_TRUE(std::filesystem::exists(outPath / "netlist.v"));
    const auto text = readTextFile(outPath / "netlist.v");
    EXPECT_EQ(std::string::npos, text.find("1'bz"));
    EXPECT_EQ(std::string::npos, text.find("1'bx"));
    if (std::system("command -v iverilog >/dev/null 2>&1") != 0 ||
        std::system("command -v vvp >/dev/null 2>&1") != 0) {
      continue;
    }
    auto reference = readTextFile(svPath);
    reference.replace(reference.find("module wildcard_memory"),
      std::string("module wildcard_memory").size(), "module memory_reference");
    // Icarus requires element-wise copies for unpacked arrays.
    const std::string copy = "updated = current;";
    reference.replace(reference.find(copy), copy.size(),
      "for (int k = 0; k < 4; k++) updated[k] = current[k];");
    const std::string commit = "current <= next_value;";
    reference.replace(reference.find(commit), commit.size(),
      "for (int k = 0; k < 4; k++) current[k] <= next_value[k];");
    std::ofstream(outPath / "reference.sv") << reference;
    std::ofstream(outPath / "tb.sv") << R"(module tb;
  reg clk = 0;
  reg [1:0] addr;
  reg [6:0] data;
  wire [6:0] result, expected;
  wildcard_memory dut(.*);
  memory_reference ref_dut(.clk(clk), .addr(addr), .data(data), .result(expected));
  integer i;
  initial begin
    for (i = 0; i < 256; i = i + 1) begin
      addr = i % 4; data = i % 128;
      #1; clk = 1; #1;
      if (result !== expected) $fatal(1, "memory mismatch data=%b", data);
      clk = 0;
    end
    $finish;
  end
endmodule
)";
    auto quote = [](const std::filesystem::path& path) {
      return "\"" + path.string() + "\"";
    };
    const auto executable = outPath / "simulation.vvp";
    const auto compile = "iverilog -g2012 -s tb -o " + quote(executable) +
      " " + quote(outPath / "tb.sv") + " " + quote(outPath / "reference.sv") +
      " " + quote(outPath / "netlist.v") + " " + quote(outPath / "naja_primitives.v");
    ASSERT_EQ(0, std::system(compile.c_str()));
    ASSERT_EQ(0, std::system(("vvp " + quote(executable)).c_str()));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitUnaryLocalConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_local_condition_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (!mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_unary_local_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDNextIndexedCommitUnaryConstantLocalConditionShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_constant_local_condition_shortcuts(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_q[i];
      if (!(mem_d[i][0] && 1'b0)) begin
        mem_next[i] = mem_d[i];
      end
      if (!(mem_d[i][0] || 1'b1)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_unary_constant_local_condition_shortcuts"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedDownRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_down_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_down_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_down_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][5 -: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_down_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedRangeReadSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_range_read_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_range_read_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_range_read_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [1:0] lo_o,
  output logic [1:0] hi_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign lo_o = mem_q[addr_i][2 +: 2];
  assign hi_o = mem_q[addr_i][5 -: 2];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_range_read_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  ASSERT_NE(nullptr, rdPortsParam);
  EXPECT_EQ("2", rdPortsParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDNextLocalCommitConstantVectorEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_constant_vector_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_local_commit_constant_vector_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_constant_vector_equality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][1:0] = 2'b10;
    mem_d[addr_i][3:2] = data_i[1:0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][1:0] == 2'b01) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_local_commit_constant_vector_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDynamicReadRangeFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_read_range_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_dynamic_read_range_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_read_range_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] start_i,
  input  logic [7:0] data_i,
  output logic [1:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i][start_i +: 2];
endmodule
  )";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_dynamic_read_range_fallback"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangeIndexedReadFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_indexed_read_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_out_of_range_indexed_read_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_indexed_read_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [1:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i][8 +: 2];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_out_of_range_indexed_read_fallback"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangeSimpleReadFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_simple_read_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_simple_read_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_simple_read_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [1:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i][8:7];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_out_of_range_simple_read_fallback"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedReadRangeOverflowFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_read_range_overflow_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_indexed_read_range_overflow_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_read_range_overflow_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [1:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i][32'sh80000000 -: 2];
endmodule
)";
  svFile.close();

  EXPECT_THROW(constructor.construct(svPath), SNLSVConstructorException);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConstantLocalSelfUpdateSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_constant_local_self_update_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_constant_local_self_update_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_constant_local_self_update_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[2'b01] = mem_d[2'b01] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_constant_local_self_update_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceMismatchedLocalSelectorFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_mismatched_local_selector_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_mismatched_local_selector_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_mismatched_local_selector_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = mem_d[addr_i ^ 2'b01] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_mismatched_local_selector_fallback"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceEquivalentConstantLocalSelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_equivalent_constant_local_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_equivalent_constant_local_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_equivalent_constant_local_selector_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[2'b01] = mem_d[2'd1] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_equivalent_constant_local_selector_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceMixedAddressEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_mixed_address_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_mixed_address_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_mixed_address_equality_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[2'b00] = data_i;
    if (en_i) begin
      mem_d[2'b01] = ~data_i;
    end
    mem_d[addr_i] = mem_d[addr_i] ^ data_i;
    mem_d[addr_i ^ 2'b10] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_mixed_address_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("4", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWriteAddressEqualityPaths) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_write_address_equality_paths";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_write_address_equality_paths.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_write_address_equality_paths(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] alt_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (en_i) begin
      mem_d[addr_i] = data_i;
      mem_d[alt_i] = ~data_i;
      mem_d[{addr_i[1], addr_i[0]}] = data_i ^ 8'h55;
      mem_d[{addr_i[1], alt_i[0]}] = data_i ^ 8'haa;
      mem_d[{addr_i[1], 1'b0}] = data_i ^ 8'h33;
      mem_d[{addr_i[1], 1'b1}] = data_i ^ 8'hcc;
      mem_d[2'd0] = data_i + 8'h01;
      mem_d[2'd1] = data_i + 8'h02;
    end else begin
      mem_d[addr_i] = data_i - 8'h01;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i] ^ mem_q[alt_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_write_address_equality_paths"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("9", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAddressCollisionEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_address_collision_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_address_collision_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_address_collision_equality_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (en_i) begin
      mem_d[addr_i] = data_i;
      mem_d[{addr_i[1], addr_i[0]}] = ~data_i;
      mem_d[{addr_i[1], ~addr_i[0]}] = data_i ^ 8'h55;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_address_collision_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConstantAddressMismatchBeforeSelfUpdate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_constant_address_mismatch_before_self_update";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_constant_address_mismatch_before_self_update.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_constant_address_mismatch_before_self_update(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[2'b00] = data_i;
    mem_d[2'b01] = mem_d[2'b01] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[2'b01];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_constant_address_mismatch_before_self_update"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSymbolicAddressEqualitySelfUpdate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_address_equality_self_update";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_symbolic_address_equality_self_update.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_address_equality_self_update(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[{addr_i[1], addr_i[0]}] = data_i;
    mem_d[{addr_i[1], addr_i[0]}] =
        mem_d[{addr_i[1], addr_i[0]}] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_address_equality_self_update"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSymbolicAddressConstOneEqualitySelfUpdate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_address_const_one_equality_self_update";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_symbolic_address_const_one_equality_self_update.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_address_const_one_equality_self_update(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[{addr_i[1], addr_i[0]}] = data_i;
    mem_d[{addr_i[1], addr_i[1]}] =
        mem_d[{addr_i[1], addr_i[1]}] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_address_const_one_equality_self_update"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSymbolicAddressConstZeroBreakSelfUpdate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_address_const_zero_break_self_update";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_symbolic_address_const_zero_break_self_update.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_address_const_zero_break_self_update(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[{1'b0, addr_i[0]}] = data_i;
    mem_d[{1'b1, addr_i[1]}] =
        mem_d[{1'b1, addr_i[1]}] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_address_const_zero_break_self_update"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLocalparamSelectorSelfUpdateSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_localparam_selector_self_update_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_localparam_selector_self_update_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_localparam_selector_self_update_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  localparam int IDX = 1;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[IDX] = mem_d[2'd1] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[IDX];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_localparam_selector_self_update_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopConstantSelectorSelfUpdateSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_constant_selector_self_update_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_loop_constant_selector_self_update_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_constant_selector_self_update_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      if (i == 2) begin
        mem_d[i] = mem_d[2'd2] ^ data_i;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[2'd2];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_loop_constant_selector_self_update_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceExpressionConstantLocalSelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_expression_constant_local_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_expression_constant_local_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_expression_constant_local_selector_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[2'd1] = mem_d[(2'd0 + 2'd1)] ^ data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_expression_constant_local_selector_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseSequentialResetPackedIndexedRangeLHSFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "sequential_reset_packed_indexed_range_lhs_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "sequential_reset_packed_indexed_range_lhs_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_reset_packed_indexed_range_lhs_fallback_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] data_i,
  output logic [7:0] data_o
);
  localparam int BASE = 2;
  localparam int WIDTH = 2;
  logic [7:0] q [0:1];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q[0][BASE +: WIDTH] <= 2'b00;
      q[1][5 -: WIDTH] <= 2'b00;
    end else begin
      q[0][BASE +: WIDTH] <= data_i;
      q[1][5 -: WIDTH] <= data_i;
    end
  end

  assign data_o = q[0] ^ q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("sequential_reset_packed_indexed_range_lhs_fallback_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseSequentialResetWholeThenPackedIndexedSlice) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "sequential_reset_whole_then_packed_indexed_slice";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "sequential_reset_whole_then_packed_indexed_slice.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_reset_whole_then_packed_indexed_slice(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] q [0:0];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q[0] <= 8'h00;
    end else begin
      q[0][2 +: 2] <= data_i;
    end
  end

  assign data_o = q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("sequential_reset_whole_then_packed_indexed_slice"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseCombinationalScalarLocalInitializerReplaySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "combinational_scalar_local_initializer_replay_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "combinational_scalar_local_initializer_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module combinational_scalar_local_initializer_replay_supported(
  input  logic sel_i,
  input  logic data_i,
  input  logic alt_i,
  output logic [3:0] data_o
);
  typedef struct packed {
    logic value;
  } bit_box_t;

  always_comb begin
    data_o = 4'h0;
    if (sel_i) begin
      bit_box_t tmp = '{value: data_i};
      data_o = {3'b000, tmp.value};
    end else begin
      bit_box_t tmp = '{value: alt_i};
      data_o = {tmp.value, 3'b000};
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("combinational_scalar_local_initializer_replay_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseUnrepresentableSelectionTrackedLHSFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unrepresentable_selection_tracked_lhs_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "unrepresentable_selection_tracked_lhs_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module unrepresentable_selection_tracked_lhs_fallback(
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  output logic       data_o
);
  real items [0:3];

  always_comb begin
    if (sel_i) begin
      items[addr_i] = 1.0;
    end else begin
      items[addr_i] = 0.0;
    end
    data_o = sel_i;
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseSequentialNestedPackedIndexedRangeLHSFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "sequential_nested_packed_indexed_range_lhs_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "sequential_nested_packed_indexed_range_lhs_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_nested_packed_indexed_range_lhs_fallback_supported(
  input  logic       clk_i,
  input  logic [1:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] q [0:1];

  always_ff @(posedge clk_i) begin
    q[0][2 +: 2] <= data_i;
    q[1][5 -: 2] <= data_i;
  end

  assign data_o = q[0] ^ q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("sequential_nested_packed_indexed_range_lhs_fallback_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseAscendingOutputTruncatedPortConnectionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "ascending_output_truncated_port_connection_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "ascending_output_truncated_port_connection_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module ascending_child(output logic [0:3] out_o);
  assign out_o = 4'b1010;
endmodule

module ascending_output_truncated_port_connection_supported(
  output logic [1:0] out_o
);
  ascending_child child(.out_o(out_o));
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("ascending_output_truncated_port_connection_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(1, top->getInstances().size());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDynamicIndexedRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_indexed_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] start_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][start_i +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCurrentEntryBitsSelectorFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_current_entry_bits_selector_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_current_entry_bits_selector_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_current_entry_bits_selector_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[$urandom_range(3, 0)][3:0] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported current-entry selector lowering";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangeSimpleRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_simple_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][9:8] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range simple range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWholeEntryWriteDataBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_whole_entry_write_data_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = $urandom_range(8'hFF, 8'h00);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialWriteAssignBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_write_assign_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3:0] = $urandom_range(4'hF, 4'h0);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDuplicateDynamicSelectorPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i][3:0] = lo_i;
      mem_d[addr_i][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDisjointConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_disjoint_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_lo_i,
  input  logic       sel_hi_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_lo_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (sel_hi_i) begin
      mem_d[1][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_disjoint_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSameConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[0][3:0] = lo_i;
    mem_d[0][7:4] = hi_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSymbolicConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic       en_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (en_i) begin
      mem_d[0][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNestedDuplicateGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_duplicate_guard_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      if (sel_i) begin
        mem_d[addr_i] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_nested_duplicate_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseGuardConstantTrueShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_guard_constant_true_shortcut_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      case (1'b0)
        1'b0: mem_d[addr_i] = data_i;
        default: mem_d[addr_i] = 8'h00;
      endcase
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_case_guard_constant_true_shortcut_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceUntimedSequentialCandidateIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_untimed_sequential_candidate_ignored(
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported statement while extracting sequential timing control",
     "unable to resolve a single-bit clock net"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSequentialNedgeClockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_nedge_clock_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(negedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSequentialClockNetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_clock_net_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_clock_net_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_clock_net_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(posedge (clk_i + 2'd1)) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve a single-bit clock net"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceResetInitUnaryPlusConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_plus_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_plus_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_plus_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (+addr_i[0]) begin
          mem_q[i] <= 8'hAA;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDNextIndexedCommitExpressionSelectorNameFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_expression_selector_name_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_expression_selector_name_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_expression_selector_name_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i + 0] = mem_d[i];
      end else begin
        mem_next[i + 0] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"direct multi-assignment LHS is not an integral signal"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceCombinationalAlwaysSequentialCandidateFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_comb_always_seq_candidate_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_comb_always_seq_candidate_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_comb_always_seq_candidate_fallback(
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @* begin
    mem_q = mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_comb_always_seq_candidate_fallback"));
  ASSERT_NE(top, nullptr);
  for (auto inst : top->getInstances()) {
    EXPECT_FALSE(NLDB0::isMemory(inst->getModel()));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceIgnorableSequentialCandidateFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_ignorable_sequential_candidate_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_ignorable_sequential_candidate_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_ignorable_sequential_candidate_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    $display("ignored");
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_ignorable_sequential_candidate_fallback"));
  ASSERT_NE(top, nullptr);
  for (auto inst : top->getInstances()) {
    EXPECT_FALSE(NLDB0::isMemory(inst->getModel()));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialNedgeClockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_nedge_clock_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_nedge_clock_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_nedge_clock_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(negedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseLargeDirectMemoryDoesNotMaterializeRawStorage) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "large_direct_memory_does_not_materialize_raw_storage";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "large_direct_memory_does_not_materialize_raw_storage.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module large_direct_memory_does_not_materialize_raw_storage(
  input  logic        clk_i,
  input  logic        write_i,
  input  logic [9:0]  addr_i,
  input  logic [31:0] data_i,
  output logic [31:0] data_o
);
  logic [31:0] mem [0:1023];

  always_ff @(posedge clk_i) begin
    if (write_i)
      mem[addr_i] <= data_i;
    data_o <= mem[addr_i];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("large_direct_memory_does_not_materialize_raw_storage"));
  ASSERT_NE(nullptr, top);
  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ("1024", memoryInst->getInstParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ(nullptr, top->getNet(NLName("mem")));
  EXPECT_EQ(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemorySeparateUniformInitialFillSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_separate_uniform_initial_fill_supported";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_separate_uniform_initial_fill_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_separate_uniform_initial_fill_supported(
  input logic clk_i, input logic we_i,
  input logic [1:0] addr_i, input logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem [0:3];
  integer i;
  initial begin
    integer unused;
    for (i = 0; i < 4; i = i + 1)
      mem[i] = 8'h5a;
  end
  always_ff @(posedge clk_i) begin
    if (we_i) mem[addr_i] <= data_i;
  end
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();
  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(
    NLName("direct_memory_separate_uniform_initial_fill_supported"));
  ASSERT_NE(nullptr, top);
  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) memoryInst = inst;
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT_ENABLE")));
  EXPECT_EQ("1", memoryInst->getInstParameter(NLName("INIT_ENABLE"))->getValue());
  EXPECT_EQ("32'b01011010010110100101101001011010",
            memoryInst->getInstParameter(NLName("INIT"))->getValue());
  const auto dumpedPath = dumpTopAndGetVerilogPath(
    top, "direct_memory_separate_uniform_initial_fill_supported_dump");
  auto dumped = readTextFile(dumpedPath);
  EXPECT_NE(std::string::npos,
            dumped.find(".INIT(32'b01011010010110100101101001011010)"));
  EXPECT_NE(std::string::npos, dumped.find(".INIT_ENABLE(1)"));
  const auto primitiveDump = readTextFile(
    dumpedPath.parent_path() / "naja_primitives.v");
  EXPECT_NE(std::string::npos, primitiveDump.find("if (INIT_ENABLE)"));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryNestedLargeSymbolicInitialFillSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_nested_large_symbolic_initial_fill_supported";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_nested_large_symbolic_initial_fill_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_nested_large_symbolic_initial_fill_supported #(
  parameter int ABITS = 16,
  parameter int DEPTH = 2**ABITS,
  parameter int CHUNK = 2**(ABITS/2)
) (input logic clk_i, input logic we_i,
   input logic [ABITS-1:0] addr_i, input logic data_i, output logic data_o);
  logic mem [0:DEPTH-1];
  integer i, j;
  initial begin
    for (i = 0; i < DEPTH; i = i + CHUNK)
      for (j = i - 0; j < i - (-CHUNK); j = j + 1)
        mem[j] = 1'b1;
  end
  always_ff @(posedge clk_i)
    if (we_i) mem[addr_i] <= data_i;
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();
  constructor.construct(svPath);
  auto* top = library_->getSNLDesign(
    NLName("direct_memory_nested_large_symbolic_initial_fill_supported"));
  ASSERT_NE(nullptr, top);
  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) memoryInst = inst;
  }
  ASSERT_NE(nullptr, memoryInst);
  auto* init = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, init);
  EXPECT_TRUE(init->getValue().starts_with("65536'b"));
  EXPECT_EQ(65536u + std::string("65536'b").size(), init->getValue().size());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryPartialInitialFillUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_partial_initial_fill_unsupported";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_partial_initial_fill_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_partial_initial_fill_unsupported(
  input logic clk_i, input logic we_i,
  input logic [1:0] addr_i, input logic [7:0] data_i, output logic [7:0] data_o);
  logic [7:0] mem [0:3];
  integer i;
  initial begin
    for (i = 1; i < 4; i = i + 1)
      mem[i] = 8'h00;
  end
  always_ff @(posedge clk_i)
    if (we_i) mem[addr_i] <= data_i;
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();
  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported initial memory initialization", "does not fully cover memory 'mem'"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryInitialFillValidationBranchesCovered) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_initial_fill_validation_branches_covered";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_initial_fill_validation_branches_covered.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(
module init_two_statements(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i;
  initial for (i=0; i<4; i++) begin mem[i]=0; mem[i]=1; end
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_negative_step(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i;
  initial for (i=3; i>=0; i=i-1) mem[i]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_nonlinear_step(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i;
  initial for (i=0; i<4; i=i+i+1) mem[i]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_bad_affine(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i, j;
  initial for (i=0; i<4; i=i+2) for (j=i*1; j<i+2; j++) mem[j]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_bad_nested_affine(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i, j;
  initial for (i=0; i<4; i=i+2) for (j=i+(i*1); j<i+2; j++) mem[j]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_bad_bound(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i;
  initial for (i=0; i<=3; i++) mem[i]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_nonblocking(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i;
  initial for (i=0; i<4; i++) mem[i] <= 0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_wrong_target(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; logic other [0:3]; integer i;
  initial for (i=0; i<4; i++) mem[0]=0;
  initial for (i=0; i<4; i++) other[i]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_nonconstant(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3]; integer i;
  initial for (i=0; i<4; i++) mem[i]=d;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_not_loop(input logic clk, we, input logic [1:0] a, input logic d);
  logic mem [0:3];
  initial mem[0]=0;
  always_ff @(posedge clk) if (we) mem[a] <= d;
endmodule
module init_multiple_memories(input logic clk, we, input logic [1:0] a, input logic d);
  logic m0 [0:3]; logic m1 [0:3]; integer i;
  initial for (i=0; i<4; i++) begin m0[i]=0; m1[i]=0; end
  always_ff @(posedge clk) if (we) begin m0[a] <= d; m1[a] <= d; end
endmodule
module init_not_inferred(input logic clk, input logic [1:0] a);
  logic mem [0:3]; integer i;
  initial for (i=0; i<4; i++) mem[i]=0;
  always_ff @(posedge clk) mem[0] <= mem[1];
endmodule
)";
  svFile.close();
  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported initial memory initialization"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryUnsupportedCandidateDiagnosticsCovered) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_unsupported_candidate_diagnostics_covered";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_unsupported_candidate_diagnostics_covered.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(
module direct_memory_multiple_async_events(
  input logic clk, rst_n, set_i, we, input logic [1:0] a, input logic d);
  logic mem [0:3];
  always_ff @(posedge clk or negedge rst_n or posedge set_i) begin
    if (!rst_n) mem[0] <= 0;
    else if (set_i) mem[0] <= 1;
    else if (we) mem[a] <= d;
  end
endmodule
module direct_memory_without_indexed_write(
  input logic clk, input logic [1:0] a, output logic d);
  logic mem [0:3];
  always_ff @(posedge clk) d <= mem[a];
endmodule
module direct_memory_constant_false_write(
  input logic clk, input logic [1:0] a, input logic d);
  logic mem [0:3];
  always_ff @(posedge clk)
    if (1'b0) mem[a] <= d;
endmodule
)";
  svFile.close();
  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"multiple async events"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryConstantConditionalWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_constant_conditional_write_supported";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_constant_conditional_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_constant_conditional_write_supported(
  input logic clk_i,
  input logic [1:0] addr_i,
  input logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem [0:3];
  always_ff @(posedge clk_i) begin
    if (1'b1)
      mem[addr_i] <= data_i;
  end
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("direct_memory_constant_conditional_write_supported"));
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(1u, countMemoryInstances(top));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryNestedPackedDynamicElementWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_nested_packed_dynamic_element_write_supported";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath =
    outPath / "direct_memory_nested_packed_dynamic_element_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_nested_packed_dynamic_element_write_supported(
  input logic clk_i,
  input logic [1:0] addr_i,
  input logic [1:0] bit_sel_i,
  input logic bit_i,
  output logic [7:0] data_o
);
  logic [1:0][3:0] mem [0:3];
  always_ff @(posedge clk_i)
    mem[addr_i][1][bit_sel_i] <= bit_i;
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("direct_memory_nested_packed_dynamic_element_write_supported"));
  ASSERT_NE(nullptr, top);
  SNLInstance* memoryInst = nullptr;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ("8", memoryInst->getInstParameter(NLName("WIDTH"))->getValue());
  EXPECT_EQ("4", memoryInst->getInstParameter(NLName("WR_PORTS"))->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryTwoDynamicPackedSelectorsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_two_dynamic_packed_selectors_fallback";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_two_dynamic_packed_selectors_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_two_dynamic_packed_selectors_fallback(
  input logic clk_i,
  input logic [1:0] addr_i,
  input logic lane_sel_i,
  input logic [1:0] bit_sel_i,
  input logic bit_i,
  output logic [7:0] data_o
);
  logic [1:0][3:0] mem [0:3];
  always_ff @(posedge clk_i)
    mem[addr_i][lane_sel_i][bit_sel_i] <= bit_i;
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"unsupported dynamic sequential assignment LHS"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseDirectMemoryDynamicSystemCallSelectorUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "direct_memory_dynamic_system_call_selector_fallback";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath = outPath / "direct_memory_dynamic_system_call_selector_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module direct_memory_dynamic_system_call_selector_fallback(
  input logic clk_i,
  input logic [1:0] addr_i,
  input logic bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem [0:3];
  always_ff @(posedge clk_i)
    mem[addr_i][$urandom_range(7, 0)] <= bit_i;
  assign data_o = mem[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"unsupported dynamic sequential assignment LHS"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseSharedSequentialNestedDuplicateShadowCommitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "shared_sequential_nested_duplicate_shadow_commits_fallback";
  std::filesystem::remove_all(outPath);
  std::filesystem::create_directory(outPath);
  const auto svPath =
    outPath / "shared_sequential_nested_duplicate_shadow_commits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << R"(module shared_sequential_nested_duplicate_shadow_commits_fallback(
  input logic clk_i,
  input logic rst_ni,
  input logic en_i,
  input logic [1:0] addr_i,
  input logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++)
        mem_q[i] <= '0;
    end else begin
      if (en_i)
        mem_q <= mem_d;
      else
        mem_q <= mem_d;
    end
  end
  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusGeneratedDisjointElements) {
  SNLSVConstructor constructor(library_);
  ScopedEnvVar threshold("NAJA_SV_UNINFERRED_MEMORY_WARNING_BITS", "1");
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "writer_census";
  std::filesystem::create_directories(directory);
  const auto source = directory / "writer_census.sv";
  std::ofstream(source) << R"(module writer_census(input logic clk, input logic [3:0] en, d, output logic [7:0] q);
  logic mem [0:3];
  logic other [0:3];
  for (genvar i = 0; i < 4; i++) begin
    always_ff @(posedge clk) begin
      if (en[i]) begin
        mem[i] <= ~d[i];
        mem[i] <= d[i];
        other[i] <= d[i];
      end
    end
    assign q[i] = mem[i];
    assign q[i+4] = other[i];
  end
endmodule
)";
  SNLSVConstructor::ConstructOptions options;
  options.diagnosticsReportPath = directory / "diagnostics.log";
  options.blackboxMultiWriterMemories = true;
  testing::internal::CaptureStderr();
  testing::internal::CaptureStdout();
  try {
    constructor.construct(source, options);
  } catch (...) {
    (void)testing::internal::GetCapturedStderr();
    (void)testing::internal::GetCapturedStdout();
    throw;
  }
  (void)testing::internal::GetCapturedStderr();
  (void)testing::internal::GetCapturedStdout();
  const auto warnings = readTextFile(*options.diagnosticsReportPath);
  EXPECT_EQ(1u, countSubstring(warnings, "Memory 'mem' was not inferred as naja_mem"));
  EXPECT_EQ(1u, countSubstring(warnings, "Memory 'other' was not inferred as naja_mem"));
  EXPECT_NE(std::string::npos, warnings.find("written from 4 sequential blocks"));
  auto* top = library_->getSNLDesign(NLName("writer_census"));
  ASSERT_NE(nullptr, top);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_EQ(0u, countMemoryInstances(top));
  for (auto* net : top->getBitNets()) {
    EXPECT_LE(countInstanceDrivers(net), 1u) << net->getString();
  }
  EXPECT_TRUE(collectDanglingInternalInputTerms(top).empty());
  const auto dumped = dumpTopAndGetVerilogPath(top, "writer_census_dump");
  if (std::system("command -v iverilog >/dev/null 2>&1") != 0 ||
      std::system("command -v vvp >/dev/null 2>&1") != 0) {
    GTEST_SKIP() << "Icarus Verilog is required for behavioral verification";
  }
  const auto tb = dumped.parent_path() / "tb.sv";
  std::ofstream(tb) << R"(module tb;
  reg clk = 0;
  reg [3:0] en = 15, d = 0;
  wire [7:0] q;
  reg [3:0] expected = 0;
  writer_census dut(.*);
  integer i, j;
  initial begin
    #1; clk = 1; #1; clk = 0;
    for (i = 0; i < 16; i = i + 1)
      for (j = 0; j < 16; j = j + 1) begin
        en = i; d = j; #1; clk = 1; #1;
        expected = (expected & ~en) | (d & en);
        if (q !== {expected, expected}) $fatal(1, "array writer behavior mismatch");
        clk = 0; #1;
      end
    $finish;
  end
endmodule
)";
  const auto quote = [](const std::filesystem::path& path) {
    return "'" + path.string() + "'";
  };
  const auto executable = dumped.parent_path() / "simulation.vvp";
  const auto command = "iverilog -g2012 -s tb -o " + quote(executable) + " " +
    quote(tb) + " " + quote(dumped) + " " +
    quote(dumped.parent_path() / "naja_primitives.v");
  ASSERT_EQ(0, std::system(command.c_str()));
  EXPECT_EQ(0, std::system(("vvp " + quote(executable)).c_str()));
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusLoopIndexedPartSelects) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "loop_writer_census";
  std::filesystem::create_directories(directory);
  for (bool reset : {false, true}) {
    const auto name = reset ? "kmac_shares" : "m3";
    const auto source = directory / (std::string(name) + ".sv");
    std::ofstream sv(source);
    if (!reset) {
      sv << R"(module m3(input logic clk, en0, en1, input logic [63:0] d0, d1,
          output logic [127:0] q);
  logic [127:0] r [2];
  always_ff @(posedge clk) if (en0) for (int j = 0; j < 2; j++) r[0][64*j+:64] <= d0;
  always_ff @(posedge clk) if (en1) for (int j = 0; j < 2; j++) r[1][64*j+:64] <= d1;
  assign q = r[0] ^ r[1];
endmodule
)";
    } else {
      sv << R"(module kmac_shares #(parameter SwKeyShare = 2)(
  input logic clk, rst_n, input logic [3:0] qe0, qe1,
  input logic [127:0] d0, d1, output logic [255:0] q);
  logic [127:0] sw_key_data_reg [SwKeyShare];
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) sw_key_data_reg[0] <= '0;
    else for (int j = 0; j < 4; j++)
      if (qe0[j]) sw_key_data_reg[0][32*j+:32] <= d0[32*j+:32];
  end
  if (SwKeyShare > 1) begin : gen_share
    always_ff @(posedge clk or negedge rst_n) begin
      if (!rst_n) sw_key_data_reg[1] <= '0;
      else for (int j = 0; j < 4; j++)
        if (qe1[j]) sw_key_data_reg[1][127-32*j-:32] <= d1[127-32*j-:32];
    end
  end
  assign q = {sw_key_data_reg[1], sw_key_data_reg[0]};
endmodule
)";
    }
    sv.close();
    SNLSVConstructor constructor(library_);
    constructor.construct(source);
    auto* top = library_->getSNLDesign(NLName(name));
    ASSERT_NE(nullptr, top);
    EXPECT_EQ(0u, countMemoryInstances(top));
    for (auto* net : top->getBitNets()) {
      EXPECT_LE(countInstanceDrivers(net), 1u) << net->getString();
    }
    EXPECT_TRUE(collectDanglingInternalInputTerms(top).empty());
    const auto dumped = dumpTopAndGetVerilogPath(top, name);
    if (std::system("command -v iverilog >/dev/null 2>&1") != 0 ||
        std::system("command -v vvp >/dev/null 2>&1") != 0) {
      GTEST_SKIP() << "Icarus Verilog is required for behavioral verification";
    }
    const auto tb = dumped.parent_path() / "tb.sv";
    std::ofstream bench(tb);
    bench << R"(module tb;
  reg clk = 0;
  integer i, j;
)";
    if (!reset) {
      bench << R"(  reg en0 = 1, en1 = 1;
  reg [63:0] d0 = 0, d1 = 0;
  reg [63:0] expected0 = 0, expected1 = 0;
  wire [127:0] q;
  m3 dut(.*);
  initial begin
    #1; clk = 1; #1; clk = 0;
    for (i = 0; i < 32; i = i + 1) begin
      en0 = i & 1; en1 = (i >> 1) & 1;
      d0 = {$random, $random}; d1 = {$random, $random};
      #1; clk = 1; #1;
      if (en0) expected0 = d0;
      if (en1) expected1 = d1;
      if (q !== {2{expected0 ^ expected1}}) $fatal(1, "m3 enable/data mismatch");
      clk = 0; #1;
    end
    $finish;
  end
)";
    } else {
      bench << R"(  reg rst_n = 1;
  reg [3:0] qe0 = 0, qe1 = 0;
  reg [127:0] d0 = 0, d1 = 0;
  reg [127:0] expected0 = 0, expected1 = 0;
  wire [255:0] q;
  kmac_shares dut(.*);
  initial begin
    #1; rst_n = 0; #1;
    if (q !== 256'b0) $fatal(1, "initial asynchronous reset mismatch");
    rst_n = 1;
    for (i = 0; i < 256; i = i + 1) begin
      qe0 = i; qe1 = i >> 4;
      d0 = {$random, $random, $random, $random};
      d1 = {$random, $random, $random, $random};
      #1; clk = 1; #1;
      for (j = 0; j < 4; j = j + 1) begin
        if (qe0[j]) expected0[32*j+:32] = d0[32*j+:32];
        if (qe1[j]) expected1[127-32*j-:32] = d1[127-32*j-:32];
      end
      if (q !== {expected1, expected0}) $fatal(1, "per-bit word enable/data mismatch");
      clk = 0; #1;
      if ((i % 17) == 16) begin
        // Reset without a clock edge, including words whose enables are low.
        rst_n = 0; #1;
        expected0 = 0; expected1 = 0;
        if (q !== 256'b0) $fatal(1, "per-bit asynchronous reset mismatch");
        rst_n = 1; #1;
      end
    end
    $finish;
  end
)";
    }
    bench << "endmodule\n";
    bench.close();
    const auto quote = [](const std::filesystem::path& path) {
      return "'" + path.string() + "'";
    };
    const auto executable = dumped.parent_path() / "simulation.vvp";
    const auto command = "iverilog -g2012 -s tb -o " + quote(executable) + " " +
      quote(tb) + " " + quote(dumped) + " " +
      quote(dumped.parent_path() / "naja_primitives.v");
    ASSERT_EQ(0, std::system(command.c_str()));
    EXPECT_EQ(0, std::system(("vvp " + quote(executable)).c_str()));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusUnrolledOwnership) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "unrolled_ownership";
  std::filesystem::create_directories(directory);
  for (bool overlap : {false, true}) {
    const auto name = overlap ? "overlapping_loop_slices" : "disjoint_loop_slices";
    const auto source = directory / (std::string(name) + ".sv");
    std::ofstream sv(source);
    sv << "module " << name << "(input logic clk, en0, en1, input logic [31:0] d0, d1, "
       << "output logic [127:0] q);\n"
       << "logic [127:0] r [2];\n"
       << "always_ff @(posedge clk) if (en0) for (int j = 0; j < 2; j++) "
       << "r[0][32*j+:32] <= d0;\n"
       << "always_ff @(posedge clk) if (en1) for (int j = 0; j < 2; j++) "
       << "r[0][" << (overlap ? "63" : "127") << "-32*j-:32] <= d1;\n"
       << "assign q = r[0]; endmodule\n";
    sv.close();
    SNLSVConstructor constructor(library_);
    if (overlap) {
      expectUnsupportedConstruct(constructor, source,
        {"multiple sequential writers require disjoint constant selections"});
    } else {
      constructor.construct(source);
      auto* top = library_->getSNLDesign(NLName(name));
      ASSERT_NE(nullptr, top);
      EXPECT_EQ(0u, countMemoryInstances(top));
      for (auto* net : top->getBitNets()) {
        EXPECT_LE(countInstanceDrivers(net), 1u) << net->getString();
      }
      EXPECT_TRUE(collectDanglingInternalInputTerms(top).empty());
    }
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusConstantSelectAndSingleLoopRegressions) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "constant_writer_census";
  std::filesystem::create_directories(directory);
  for (bool reset : {false, true}) {
    for (bool singleWriter : {false, true}) {
      const auto name = std::string(singleWriter ? "single_loop_writer" : "constant_slice_writers") +
        (reset ? "_reset" : "_enabled");
      const auto source = directory / (name + ".sv");
      std::ofstream sv(source);
      sv << "module " << name << "(input logic clk, rst_n, en0, en1, "
         << "input logic [63:0] d0, d1, output logic [63:0] q);\n"
         << "logic [63:0] r [2];\n";
      if (singleWriter) {
        sv << "always_ff @(posedge clk" << (reset ? " or negedge rst_n" : "") << ") begin\n";
        if (reset) sv << "if (!rst_n) begin r[0] <= '0; r[1] <= '0; end else begin\n";
        sv << "if (en0) for (int j=0; j<2; j++) r[0][32*j+:32] <= d0[32*j+:32];\n"
           << "if (en1) for (int j=0; j<2; j++) r[1][63-32*j-:32] <= d1[63-32*j-:32];\n";
        if (reset) sv << "end\n";
        sv << "end\n";
      } else {
        for (int share = 0; share < 2; ++share) {
          sv << "always_ff @(posedge clk" << (reset ? " or negedge rst_n" : "") << ") "
             << (reset ? "if (!rst_n) r[" + std::to_string(share) + "] <= '0; else " : "")
             << "if (en" << share << ") r[" << share << "][63:0] <= d" << share << ";\n";
        }
      }
      sv << "assign q = r[0] ^ r[1]; endmodule\n";
      sv.close();
      SNLSVConstructor constructor(library_);
      constructor.construct(source);
      auto* top = library_->getSNLDesign(NLName(name));
      ASSERT_NE(nullptr, top);
      for (auto* net : top->getBitNets()) {
        EXPECT_LE(countInstanceDrivers(net), 1u) << net->getString();
      }
      EXPECT_TRUE(collectDanglingInternalInputTerms(top).empty());
    }
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusRejectsOverlappingOwnership) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "writer_ownership";
  std::filesystem::create_directories(directory);
  for (bool shadow : {false, true}) {
    for (bool dynamic : {false, true}) {
      const auto name = std::string(shadow ? "shadow_ownership" : "whole_array_ownership") +
        (dynamic ? "_dynamic" : "_constant");
      const auto source = directory / (std::string(name) + ".sv");
      std::ofstream sv(source);
      sv << "module " << name << "(input logic clk, input logic [1:0] a, input logic d, output logic q);\n"
         << "logic mem [0:3]; logic next_mem [0:3];\n";
      if (shadow) {
        sv << "always_comb begin next_mem = mem; next_mem[a] = d; end\n"
           << "always_ff @(posedge clk) mem <= next_mem;\n";
      } else {
        sv << "always_ff @(posedge clk) mem <= '{default: 1'b0};\n";
      }
      const auto selector = dynamic ? "a" : "0";
      sv << "always_ff @(posedge clk) begin mem[" << selector
         << "] <= d; mem[" << selector << "] <= ~d; end\n"
         << "assign q = mem[a]; endmodule\n";
      sv.close();
      SNLSVConstructor constructor(library_);
      SNLSVConstructor::ConstructOptions options;
      options.diagnosticsReportPath = directory / (std::string(name) + ".log");
      testing::internal::CaptureStdout();
      testing::internal::CaptureStderr();
      expectUnsupportedConstruct(constructor, source,
        {"multiple sequential writers require disjoint constant selections",
         "overlapping or dynamic writes are unsupported"}, options);
      (void)testing::internal::GetCapturedStdout();
      (void)testing::internal::GetCapturedStderr();
      const auto warnings = readTextFile(*options.diagnosticsReportPath);
      EXPECT_EQ(1u, countSubstring(warnings, "Memory 'mem' was not inferred as naja_mem"));
      EXPECT_NE(std::string::npos, warnings.find("written from 2 sequential blocks"));
      auto* top = library_->getSNLDesign(NLName(name));
      ASSERT_NE(nullptr, top);
      EXPECT_EQ(0u, countMemoryInstances(top));
    }
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusSingleBlockMultipleWrites) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "single_writer";
  std::filesystem::create_directories(directory);
  const auto source = directory / "single_writer.sv";
  std::ofstream(source) << R"(module single_writer(input logic clk, input logic [1:0] a, input logic d, output logic q);
  logic mem [0:3];
  always_ff @(posedge clk) begin
    mem[a] <= ~d;
    mem[a] <= d;
  end
  assign q = mem[a];
endmodule
)";
  SNLSVConstructor constructor(library_);
  constructor.construct(source);
  auto* top = library_->getSNLDesign(NLName("single_writer"));
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(1u, countMemoryInstances(top));
  EXPECT_TRUE(collectUnsupportedMemoryBoundaryTerms(top).empty());
}


TEST_F(SNLSVConstructorTestMemoryInference, independentWriteClockMemoryBehavior) {
  for (const auto& variant : {"dual_clock", "shared_clock", "masked", "masked_shared", "ascending_mask", "priority", "negative_clock", "self_read", "wide_address", "signed_address", "three_writers", "request_gated"}) {
    SCOPED_TRACE(variant);
    const bool shared = std::string(variant) == "shared_clock" || std::string(variant) == "masked_shared";
    const bool ascending = std::string(variant) == "ascending_mask";
    const bool masked = std::string(variant) == "masked" ||
      std::string(variant) == "masked_shared" || ascending;
    const bool wide = std::string(variant) == "wide_address";
    const bool signedAddress = std::string(variant) == "signed_address";
    const bool threeWriters = std::string(variant) == "three_writers";
    const bool requestGated = std::string(variant) == "request_gated";
    const bool priority = std::string(variant) == "priority";
    const bool negative = std::string(variant) == "negative_clock";
    const bool selfRead = std::string(variant) == "self_read";
    const auto name = std::string("dp_") + variant;
    const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / name;
    std::filesystem::create_directories(directory);
    const auto source = directory / "source.sv";
    std::string rtl = R"(module MODULE(input logic clk_a, clk_b, we_a, we_b, req_a, req_b,
              input logic [2:0] addr_a, addr_b,
              input logic [7:0] wd_a, wd_b, input logic [1:0] mask_a, mask_b,
              output logic [7:0] rd_a, rd_b);
  logic [7:0] mem [8];
  initial for (int i = 0; i < 8; i++) mem[i] = 8'h00;
  always @(posedge clk_a) begin
    WRITE_A
    if (req_a) rd_a <= mem[addr_a];
  end
  always @(EDGE_B CLOCK_B) begin
    WRITE_B
    if (req_b) rd_b <= mem[addr_b];
  end
endmodule
)";
    auto replace = [](std::string& text, const std::string& token, const std::string& value) {
      const auto pos = text.find(token);
      ASSERT_NE(std::string::npos, pos);
      text.replace(pos, token.size(), value);
    };
    replace(rtl, "MODULE", name);
    replace(rtl, "CLOCK_B", shared ? "clk_a" : "clk_b");
    replace(rtl, "EDGE_B", negative ? "negedge" : "posedge");
    replace(rtl, "WRITE_A", masked
      ? "if (we_a) for (int i = 0; i < 2; i = i + 1) if (mask_a[i]) mem[addr_a][i*4 +: 4] <= wd_a[i*4 +: 4];"
      : priority
        ? "if (we_a) begin mem[addr_a] <= wd_a; if (mask_a[0]) mem[addr_a][3:0] <= wd_a[3:0] ^ 4'hf; end"
        : selfRead
          ? "if (we_a) begin mem[addr_a] <= wd_a; mem[addr_a] <= mem[addr_a] + 8'd1; end"
          : "if (we_a) mem[addr_a] <= wd_a;");
    replace(rtl, "WRITE_B", masked
      ? "if (we_b) for (int i = 0; i < 2; i++) if (mask_b[i]) mem[addr_b][i*4 +: 4] <= wd_b[i*4 +: 4];"
      : "if (we_b) mem[addr_b] <= wd_b;");
    if (wide || signedAddress)
      replace(rtl, "input logic [2:0] addr_a, addr_b",
        signedAddress ? "input logic signed [3:0] addr_a, addr_b" : "input logic [3:0] addr_a, addr_b");
    if (ascending) {
      replace(rtl, "logic [7:0] mem", "logic [0:7] mem");
      replace(rtl, "rd_a <= mem[addr_a];", "rd_a <= {mem[addr_a][0 +: 4], mem[addr_a][4 +: 4]};");
    }
    if (requestGated) {
      replace(rtl, "if (we_a) mem[addr_a] <= wd_a;\n    if (req_a) rd_a <= mem[addr_a];",
        "if (req_a) begin if (we_a) mem[addr_a] <= wd_a; else rd_a <= mem[addr_a]; end");
      replace(rtl, "if (we_b) mem[addr_b] <= wd_b;\n    if (req_b) rd_b <= mem[addr_b];",
        "if (req_b) begin if (we_b) mem[addr_b] <= wd_b; else rd_b <= mem[addr_b]; end");
    }
    if (threeWriters) replace(rtl, "endmodule",
      "always @(posedge clk_a) if (req_b) mem[addr_a + 1] <= wd_a ^ 8'h5a;\nendmodule");
    std::ofstream(source) << rtl;
    SNLSVConstructor constructor(library_);
    SNLSVConstructor::ConstructOptions options;
    options.diagnosticsReportPath = directory / "diagnostics.log";
    constructor.construct(source, options);
    auto* top = library_->getSNLDesign(NLName(name));
    ASSERT_NE(nullptr, top);
    EXPECT_FALSE(top->isBlackBox());
    EXPECT_EQ(1u, countMemoryInstances(top));
    auto* memory = top->getInstance(NLName("mem_mem"));
    ASSERT_NE(nullptr, memory);
    const auto signature = NLDB0::getMemorySignature(memory);
    EXPECT_TRUE(signature.independentWriteClocks);
    EXPECT_EQ(8u, signature.width);
    EXPECT_EQ(8u, signature.depth);
    EXPECT_EQ(masked ? 4u : (priority || selfRead || threeWriters ? 3u : 2u), signature.writePorts);
    EXPECT_EQ(signature.writePorts, memory->getModel()->getBusTerm(NLName("WCLK"))->getWidth());
    EXPECT_EQ(signature.width * signature.writePorts,
      memory->getModel()->getBusTerm(NLName("WMASK"))->getWidth());
    EXPECT_EQ(nullptr, top->getNet(NLName("mem")));
    EXPECT_TRUE(collectDanglingInternalInputTerms(top).empty());
    EXPECT_TRUE(collectUnsupportedMemoryBoundaryTerms(top).empty());
    const auto dumped = dumpTopAndGetVerilogPath(top, name + "_dump");
    const auto warnings = readTextFile(*options.diagnosticsReportPath);
    EXPECT_NE(std::string::npos, warnings.find("multi_clock_memory_collision"));
    EXPECT_EQ(std::string::npos, warnings.find("using generic sequential lowering"));
    if (std::system("command -v iverilog >/dev/null 2>&1") != 0 ||
        std::system("command -v vvp >/dev/null 2>&1") != 0) {
      GTEST_SKIP() << "Icarus Verilog is required for behavioral verification";
    }
    auto goldenRTL = rtl;
    replace(goldenRTL, "module " + name + "(", "module golden(");
    const auto golden = dumped.parent_path() / "golden.sv";
    std::ofstream(golden) << goldenRTL;
    const auto tb = dumped.parent_path() / "tb.sv";
    std::ofstream bench(tb);
    bench << R"(module tb;
  reg clk_a = 0, clk_b = 0, we_a = 0, we_b = 0, req_a = 1, req_b = 1;
  reg [)" << (wide || signedAddress ? 3 : 2) << R"(:0] addr_a = 0, addr_b = 1;
  reg [7:0] wd_a = 0, wd_b = 0;
  reg [1:0] mask_a = 0, mask_b = 0;
  wire [7:0] rd_a, rd_b, golden_a, golden_b;
)" << name << R"( dut(.*);
  golden reference(.rd_a(golden_a), .rd_b(golden_b), .*);
  task check;
    begin
      if (rd_a !== golden_a || rd_b !== golden_b)
        $fatal(1, "read mismatch: got %h %h expected %h %h", rd_a, rd_b, golden_a, golden_b);
    end
  endtask
  integer i;
  initial begin
    for (i = 0; i < 80; i = i + 1) begin
      addr_a = i % 8; addr_b = (i + 3) % 8;
      wd_a = i * 17 + 3; wd_b = i * 11 + 7;
      we_a = i % 3 != 0; we_b = i % 4 != 0;
      req_a = i % 5 != 0; req_b = i % 7 != 0;
      mask_a = i % 4; mask_b = (i + 1) % 4;
      #2; clk_a = 1; #2; check(); clk_a = 0;
      #2; clk_b = 1; #2; check(); clk_b = 0; #2; check();
    end
)";
    if (masked) {
      // Independent writes to disjoint bits of the same word must both survive.
      bench << R"(
    addr_a = 2; addr_b = 2; mask_a = 1; mask_b = 2;
    we_a = 1; we_b = 1; req_a = 1; req_b = 1;
    wd_a = 8'h0a; wd_b = 8'hb0;
    #2; clk_a = 1; clk_b = 1; #2; check(); clk_a = 0; clk_b = 0;
    we_a = 0; we_b = 0;
    #2; clk_a = 1; clk_b = 1; #2; check();
    if (rd_a !== golden_a || rd_b !== golden_b) $fatal(1, "disjoint masks lost data");
)";
    }
    if (wide || signedAddress) {
      // Invalid full-width addresses must not alias a valid truncated address.
      bench << R"(
    addr_a = 8; addr_b = 9; we_a = 1; we_b = 1; req_a = 0; req_b = 0;
    wd_a = 8'hff; wd_b = 8'hee;
    #2; clk_a = 1; clk_b = 1; #2; check(); clk_a = 0; clk_b = 0;
    addr_a = 0; addr_b = 1; we_a = 0; we_b = 0; req_a = 1; req_b = 1;
    #2; clk_a = 1; clk_b = 1; #2; check(); clk_a = 0; clk_b = 0;
)";
    }
    bench << "    $finish;\n  end\nendmodule\n";
    bench.close();
    const auto executable = dumped.parent_path() / "sim.out";
    const auto quote = [](const std::filesystem::path& path) {
      return "'" + path.string() + "'";
    };
    const auto compile = "iverilog -g2012 -s tb -o " + quote(executable) +
      " " + quote(dumped) + " " + quote(golden) + " " + quote(tb) +
      " " + quote(dumped.parent_path() / "naja_primitives.v");
    ASSERT_EQ(0, std::system(compile.c_str()));
    const auto run = "vvp " + quote(executable);
    ASSERT_EQ(0, std::system(run.c_str()));
  }
}


TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusRejectsDynamicPackedOverlapCoverage) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "dynamic_packed_ownership";
  std::filesystem::create_directories(directory);
  for (const auto& selection : {"[index +: 2]", "[index]", ".field[index]"}) {
    SCOPED_TRACE(selection);
    const auto source = directory / "source.sv";
    const bool member = std::string(selection).front() == '.';
    std::ofstream(source)
      << "module top(input logic clk, input logic [1:0] index, input logic d, output logic [7:0] q);\n"
      << (member ? "typedef struct packed { logic [3:0] field; } word_t; word_t mem[2];\n"
                 : "logic [3:0] mem[2];\n")
      << "always_ff @(posedge clk) mem[0]" << selection << " <= '0;\n"
      << "always_ff @(posedge clk) mem[0]" << selection << " <= '1;\n"
      << "assign q = {mem[1], mem[0]}; endmodule\n";
    SNLSVConstructor constructor(library_);
    expectUnsupportedConstruct(constructor, source,
      {"multiple sequential writers require disjoint constant selections"});
    auto* top = library_->getSNLDesign(NLName("top"));
    ASSERT_NE(nullptr, top);
    EXPECT_EQ(0u, countMemoryInstances(top));
    top->destroy();
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, memoryWriterCensusRejectsUnresolvedLoopsCoverage) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "unresolved_loop_ownership";
  std::filesystem::create_directories(directory);
  for (const auto& loop : {
      "for (int i = 0; i < limit; i++) mem[0][i] <= d;",
      "for (int i = 0; i < 4; i += limit) mem[0][i] <= d;"}) {
    SCOPED_TRACE(loop);
    const auto source = directory / "source.sv";
    std::ofstream(source)
      << "module top(input logic clk, input int limit, input logic d, output logic [7:0] q);\n"
      << "logic [3:0] mem[2];\n"
      << "always_ff @(posedge clk) " << loop << "\n"
      << "always_ff @(posedge clk) mem[1] <= '0;\n"
      << "assign q = {mem[1], mem[0]}; endmodule\n";
    SNLSVConstructor constructor(library_);
    expectUnsupportedConstruct(constructor, source,
      {"multiple sequential writers require disjoint constant selections"});
    auto* top = library_->getSNLDesign(NLName("top"));
    ASSERT_NE(nullptr, top);
    top->destroy();
  }
}


TEST_F(SNLSVConstructorTestMemoryInference, independentMemoryResolutionFailuresCoverage) {
  const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "independent_memory_failures";
  std::filesystem::create_directories(directory);
  for (const auto& variant : {"address", "clock", "data"}) {
    SCOPED_TRACE(variant);
    const auto source = directory / "source.sv";
    const auto address = std::string(variant) == "address" ? "$clog2(addr)" : "addr";
    const auto clock = std::string(variant) == "clock" ? "$clog2(clk_b)" : "clk_b";
    const auto data = std::string(variant) == "data" ? "$clog2(d)" : "d";
    std::ofstream(source)
      << "module top(input logic clk_a, clk_b, we, input logic [2:0] addr,\n"
      << "input logic [7:0] d, output logic [7:0] q); logic [7:0] mem[8];\n"
      << "always @(posedge clk_a) if (we) mem[" << address << "] <= " << data << ";\n"
      << "always @(posedge " << clock << ") if (we) mem[addr] <= d;\n"
      << "assign q = mem[addr]; endmodule\n";
    SNLSVConstructor constructor(library_);
    const auto reason = std::string(variant) == "address"
      ? "unable to resolve full independent memory write address"
      : std::string(variant) == "clock"
        ? "unable to resolve independent memory write clock"
        : "unable to resolve independent memory write data";
    expectUnsupportedConstruct(constructor, source,
      {"independent write ports could not be lowered", reason});
    auto* top = library_->getSNLDesign(NLName("top"));
    ASSERT_NE(nullptr, top);
    top->destroy();
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, wholeArrayMemoryReadsBehavior) {
  struct Case {
    std::string name;
    bool lanes = false;
    bool dynamic = false;
    int reset = 0; // 0: none, 1: synchronous, 2: asynchronous
    bool elementReset = false;
    std::string binding = "assign";
    bool mixed = false;
    bool descending = false;
    bool structure = false;
    bool multidimensional = false;
    bool wholeWrite = false;
    bool packed = false;
  };
  const std::vector<Case> cases {
    {"constant_output", false, false, 0, false, "port"},
    {"loop_output", true, false, 0, false, "port"},
    {"loop_async_assign", true, false, 2},
    {"constant_element_reads", false, false, 0, false, "elements"},
    {"whole_write_async", false, false, 2, false, "port", false, false, false, false, true},
    {"packed_loop", true, false, 0, false, "port", false, false, false, false, false, true},
    {"dynamic_assign", false, true},
    {"dynamic_sync_port", false, true, 1, false, "port"},
    {"dynamic_async_submodule", false, true, 2, false, "submodule"},
    {"constant_sync_element_reset", false, false, 1, true},
    {"loop_async_element_reset", true, false, 2, true},
    {"mixed_reads", false, true, 0, false, "assign", true},
    {"descending_output", false, true, 0, false, "port", false, true},
    {"struct_array", false, true, 2, false, "submodule", true, false, true},
    {"multidimensional_array", false, false, 0, false, "assign", false, false, false, true},
    {"aggregate_operand", false, true, 0, false, "aggregate"}
  };
  const bool simulatorAvailable = std::system("command -v iverilog >/dev/null 2>&1") == 0 &&
    std::system("command -v vvp >/dev/null 2>&1") == 0;
  for (const auto& c : cases) {
    for (bool singleUnit : {false, true}) {
      const auto name = c.name + (singleUnit ? "_single" : "_files");
      SCOPED_TRACE(name);
      auto* library = NLLibrary::create(library_->getDB(), NLName(name));
      const auto directory = std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / name;
      std::filesystem::create_directories(directory);
      const size_t width = c.lanes ? 32 : 8;
      const size_t depth = c.multidimensional ? 6 : 2;
      const size_t total = width * depth;
      const auto range = "[" + std::to_string(total - 1) + ":0]";
      const auto elementType = c.structure ? "word_t" : c.lanes ? "logic [3:0][7:0]" : "logic [7:0]";
      const std::string shape = c.packed ? "" : c.multidimensional ? " [2][3]" :
        c.descending ? " [1:0]" : " [2]";
      const std::string type = c.packed ? "logic [1:0][3:0][7:0]" : elementType;
      const auto helper = directory / "types.sv";
      std::ofstream(helper) << "package array_types; typedef struct packed { logic [2:0] a; logic [4:0] b; } word_t; endpackage\n";
      auto element = [&](size_t k) {
        return c.multidimensional ? "[" + std::to_string(k / 3) + "][" + std::to_string(k % 3) + "]" :
          "[" + std::to_string(k) + "]";
      };
      auto offset = [&](size_t k) { return (c.descending || c.packed ? k : depth - 1 - k) * width; };
      std::ostringstream rtl;
      if (c.binding == "submodule") {
        // Keep hierarchy and flattened connectivity observable.
        rtl << "module sink import array_types::*; (input " << type << " a" << shape
            << ", output " << type << " o" << shape << "); assign o = a; endmodule\n";
      }
      rtl << "module array_reads import array_types::*; (input logic clk, rst_n, input logic [7:0] we, "
          << "input logic addr, input logic " << range << " d, output "
          << type << " o" << shape << ", output logic [7:0] selected);\n";
      if (c.binding != "port") rtl << type << " q" << shape << ";\n";
      const std::string q = c.binding == "port" ? "o" : "q";
      rtl << type << " data" << shape << ";\n";
      for (size_t k = 0; k < depth; ++k)
        rtl << "assign data" << element(k) << " = d[" << offset(k) << " +: " << width << "];\n";
      rtl << "always_ff @(posedge clk" << (c.reset == 2 ? " or negedge rst_n" : "") << ") begin\n";
      if (c.reset) {
        rtl << "if (!rst_n) begin\n";
        if (c.elementReset) {
          for (size_t k = 0; k < depth; ++k) rtl << q << element(k) << " <= '0;\n";
        } else rtl << q << " <= '{default:'0};\n";
        rtl << "end else begin\n";
      }
      if (c.wholeWrite) rtl << "if (we[0]) " << q << " <= data;\n";
      else if (c.dynamic) rtl << "if (we[0]) " << q << "[addr] <= data[addr];\n";
      else if (c.lanes) rtl << "for (int s=0; s<2; s++) for (int i=0; i<4; i++) "
        << "if (we[s*4+i]) " << q << "[s][i] <= data[s][i];\n";
      else {
        for (size_t k = 0; k < depth; ++k)
          rtl << "if (we[0]) " << q << element(k) << " <= "
              << (k == 1 && !c.multidimensional ? "~" : "") << "data" << element(k) << ";\n";
      }
      if (c.reset) rtl << "end\n";
      rtl << "end\n";
      if (c.binding == "submodule") rtl << "sink u(.a(q), .o(o));\n";
      else if (c.binding == "aggregate") rtl << "logic [7:0] view [2][2]; assign view = '{q, data}; assign o = view[0];\n";
      else if (c.binding == "elements") {
        for (size_t k = 0; k < depth; ++k) rtl << "assign o" << element(k) << " = q" << element(k) << ";\n";
      } else if (c.binding != "port") rtl << "assign o = q;\n";
      rtl << "assign selected = " << (c.mixed ? q + "[addr]" : "8'b0") << ";\nendmodule\n";
      const auto source = directory / "array_reads.sv";
      std::ofstream(source) << rtl.str();
      SNLSVConstructor constructor(library);
      SNLSVConstructor::Paths paths {helper, source};
      if (singleUnit) paths.insert(paths.begin(), "--single-unit");
      ASSERT_NO_THROW(constructor.construct(paths));
      auto* top = library->getSNLDesign(NLName("array_reads"));
      ASSERT_NE(nullptr, top);
      EXPECT_FALSE(top->isBlackBox());
      EXPECT_EQ(c.wholeWrite || c.packed ? 0u : 1u, countMemoryInstances(top));
      for (auto* inst : top->getInstances()) {
        if (!NLDB0::isMemory(inst->getModel())) continue;
        // Constant taps share the existing element/RMW read ports. Only a
        // mixed dynamic read needs a third port for this two-word memory.
        EXPECT_EQ(c.mixed ? 3u : 2u, NLDB0::getMemorySignature(inst).readPorts);
      }
      EXPECT_TRUE(collectDanglingInternalInputTerms(top).empty());
      for (auto* net : top->getBitNets()) EXPECT_LE(countInstanceDrivers(net), 1u) << net->getString();
      // Include top outputs and submodule boundaries in the connectivity check.
      NLUniverse::get()->setTopDesign(top);
      naja::DNL::destroy();
      auto* dnl = naja::DNL::get();
      for (const auto& term : dnl->getDNLTerms()) {
        if (term.isNull() || term.getIsoID() == naja::DNL::DNLID_MAX) continue;
        const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(term.getIsoID());
        if (!iso.getReaders().empty() && !iso.isConstant()) EXPECT_EQ(1u, iso.getDrivers().size());
      }
      for (auto* inst : top->getInstances()) {
        if (!NLDB0::isMemory(inst->getModel())) continue;
        for (auto* bit : NLDB0::getMemoryReadData(inst->getModel())->getBits()) {
          const auto& term = dnl->getTop().getChildInstance(inst).getTerminalFromBitTerm(bit);
          const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(term.getIsoID());
          EXPECT_FALSE(iso.getReaders().empty()) << inst->getString() << " " << bit->getString();
        }
      }
      naja::DNL::destroy();
      const auto dumped = dumpTopAndGetVerilogPath(top, name + "_dump");
      if (!simulatorAvailable) continue;
      const auto golden = dumped.parent_path() / "golden.sv";
      std::ofstream reference(golden);
      reference << "module golden(input clk, rst_n, input [7:0] we, input addr, input " << range
        << " d, output reg " << range << " o, output [7:0] selected);\n";
      // Independent per-bit flops provide a reference without memory inference,
      // aggregate assignments, or unpacked-array simulation support.
      for (size_t k = 0; k < depth; ++k) {
        for (size_t bit = 0; bit < width; ++bit) {
          const auto b = offset(k) + bit;
          reference << "always @(posedge clk" << (c.reset == 2 ? " or negedge rst_n" : "") << ") ";
          if (c.reset) reference << "if (!rst_n) o[" << b << "] <= 0; else ";
          reference << "if (we[" << (c.lanes ? k * 4 + bit / 8 : 0) << "]"
            << (c.dynamic ? " && addr == " + std::to_string(k) : "") << ") o[" << b << "] <= "
            << (!c.dynamic && !c.lanes && !c.wholeWrite && !c.multidimensional && k == 1 ? "~" : "")
            << "d[" << b << "];\n";
        }
      }
      reference << "assign selected = " << (c.mixed ? "addr ? o[7:0] : o[15:8]" : "8'b0") << "; endmodule\n";
      reference.close();
      const auto tb = dumped.parent_path() / "tb.sv";
      std::ofstream bench(tb);
      bench << "module tb; reg clk=0, rst_n=1, addr=0; reg [7:0] we=255; reg " << range
        << " d=0; wire " << range << " o, expected; wire [7:0] selected, expected_selected;\n"
        << "array_reads dut(.*); golden ref_dut(.o(expected), .selected(expected_selected), .*);\n"
        << "task check; begin if (o !== expected || selected !== expected_selected) "
        << "$fatal(1, \"array read mismatch got %h expected %h\", o, expected); end endtask\n"
        << "integer i; initial begin\n"
        // Initialize every word before comparing uninitialized storage; vary
        // enables afterwards to exercise hold and partial-write behavior.
        << "for (i=0; i<48; i=i+1) begin addr=i%2; we=i<2 ? 255 : ((i*37) ^ (i/2)); "
        << "d={32'h13579bdf ^ (i*32'd12345),32'h2468ace0 ^ (i*32'd54321)}; "
        << "#2; clk=1; #2; if (i>=1) check(); clk=0; #2; end\n";
      if (c.reset) bench << "we=0; rst_n=0; #2; " << (c.reset == 2 ? "check(); " : "")
        << "clk=1; #2; check(); clk=0; rst_n=1; #2; check();\n";
      bench << "$finish; end endmodule\n";
      bench.close();
      const auto quote = [](const std::filesystem::path& path) { return "'" + path.string() + "'"; };
      const auto executable = dumped.parent_path() / "simulation.vvp";
      const auto compile = "iverilog -g2012 -s tb -o " + quote(executable) + " " + quote(tb) +
        " " + quote(golden) + " " + quote(dumped) + " " + quote(dumped.parent_path() / "naja_primitives.v");
      ASSERT_EQ(0, std::system(compile.c_str()));
      ASSERT_EQ(0, std::system(("vvp " + quote(executable)).c_str()));
    }
  }
  if (!simulatorAvailable) GTEST_SKIP() << "Connectivity checked; Icarus Verilog is required for simulation";
}
