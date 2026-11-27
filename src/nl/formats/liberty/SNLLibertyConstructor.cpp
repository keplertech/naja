// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibertyConstructor.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "YosysLibertyParser.h"
#include "YosysLibertyException.h"

#include "NajaLog.h"
#include "NajaPerf.h"

#include "NLLibrary.h"
#include "NLLibraryTruthTables.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBundleTerm.h"
#include "SNLBooleanTree.h"
#include "SNLDesignModeling.h"
#include "SNLLibertyConstructorException.h"

#include <zlib.h>

namespace {

using namespace naja::NL;
using ConflictingCellNamePolicy =
  SNLLibertyConstructor::Config::ConflictingCellNamePolicy;

SNLTerm::Direction getSNLDirection(const std::string& direction) {
  if (direction == "input") {
    return SNLTerm::Direction::Input;
  } else if (direction == "output") {
    return SNLTerm::Direction::Output;
  } else if (direction == "inout") {
    return SNLTerm::Direction::InOut;
  } else {
    std::ostringstream reason;
    reason << "Unknown direction: " << direction;
    throw SNLLibertyConstructorException(reason.str());
  }
}

const Yosys::LibertyAst* findDirectChild(const Yosys::LibertyAst* node, const std::string& id) {
  for (auto child: node->children) {
    if (child->id == id) {
      return child;
    }
  }
  return nullptr;
}

bool containsLibertyNode(
    const Yosys::LibertyAst* node,
    const std::string& id) {
  for (auto* child : node->children) {
    if (child->id == "test_cell") continue;
    if (child->id == id || containsLibertyNode(child, id)) {
      return true;
    }
  }
  return false;
}

const Yosys::LibertyAst* findDirectionNode(const Yosys::LibertyAst* node) {
  auto directionNode = findDirectChild(node, "direction");
  if (directionNode != nullptr or node->id != "bus") {
    return directionNode;
  }

  const Yosys::LibertyAst* inferredDirectionNode = nullptr;
  for (auto child: node->children) {
    if (child->id != "pin") {
      continue;
    }
    auto childDirectionNode = findDirectChild(child, "direction");
    if (childDirectionNode == nullptr) {
      continue;
    }
    if (inferredDirectionNode == nullptr) {
      inferredDirectionNode = childDirectionNode;
      continue;
    }
    if (inferredDirectionNode->value != childDirectionNode->value) {
      std::ostringstream reason;
      reason << "Inconsistent child pin directions for bus " << node->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  return inferredDirectionNode;
}

struct BusType {
  std::string name  {};
  int msb           {0};
  int lsb           {0};
};

using TermFunctions = std::map<SNLScalarTerm*, std::string, SNLScalarTerm::PointerLess>;
using TermPins = std::map<SNLScalarTerm*, std::string, SNLScalarTerm::PointerLess>;
using TermSourceLines = std::map<SNLScalarTerm*, int, SNLScalarTerm::PointerLess>;
using SequentialClockNames = std::set<std::string>;
using SequentialTermClocks =
  std::map<SNLBitTerm*, SequentialClockNames, SNLBitTerm::InDesignLess>;
using TimingOnlyOutputInputs =
  std::map<SNLBitTerm*, std::set<std::string>, SNLBitTerm::InDesignLess>;

struct ConstructedTerm {
  SNLScalarTerm* scalar{nullptr};
  SNLBusTerm* bus{nullptr};
};

struct LibertyMemoryReadPort {
  SNLBusTerm* dataBus{nullptr};
  std::string addressName {};
  std::string clockName   {};
};

struct LibertyMemoryWriteBus {
  SNLBusTerm* dataBus{nullptr};
  std::string addressName {};
  std::string clockName   {};
};

struct LibertyMemoryInterface {
  bool present {false};
  size_t wordWidth {0};
  size_t addressWidth {0};
  std::vector<LibertyMemoryReadPort> readPorts {};
  std::vector<LibertyMemoryWriteBus> writeBuses {};
};

BusType findBusType(const Yosys::LibertyAst* ast, const std::string& busType) {
  for (auto child: ast->children) {
    if (child->id == "type" and not child->args.empty() and child->args[0] == busType) {
      int from = 0;
      int to = 0;
      bool downto = true;
      for (auto subChild: child->children) {
        if (subChild->id == "bit_from") {
          from = std::stoi(subChild->value);
        } else if (subChild->id == "bit_to") {
          to = std::stoi(subChild->value);
        } else if (subChild->id == "downto") {
          downto = subChild->value == "true";
        }
      }
      return BusType{busType, downto ? from : to, downto ? to : from};
    }
  }
  return BusType();
}

enum class FunctionParsingType {
  Ignore,
  TimingOnly,
  Sequential,
  Combinational
};

bool shouldParseSequentialTiming(FunctionParsingType type) {
  return type == FunctionParsingType::TimingOnly ||
         type == FunctionParsingType::Sequential;
}

bool shouldParseOutputFunctions(FunctionParsingType type) {
  return type == FunctionParsingType::Sequential ||
         type == FunctionParsingType::Combinational;
}

std::string buildFunctionParseErrorReason(
  const std::string& pinName,
  int sourceLine,
  const std::string& function,
  const std::string& parserReason) {
  std::ostringstream reason;
  reason << "Invalid `function` expression for pin `" << pinName << "`";
  if (sourceLine > 0) {
    reason << " at line " << sourceLine;
  }
  reason << ": `" << function << "`: " << parserReason;
  return reason.str();
}

std::string buildCellErrorReason(
  const Yosys::LibertyAst* cell,
  const std::filesystem::path& sourcePath,
  const std::string& constructorReason) {
  std::ostringstream reason;
  reason << "Liberty construction error in file `" << sourcePath.string() << "`";
  if (cell->line > 0) {
    reason << ", line " << cell->line;
  }
  reason << ", cell `"
         << (cell->args.empty() ? "<unnamed>" : cell->args[0])
         << "`: " << constructorReason;
  return reason.str();
}

std::string buildParserFileErrorReason(
  const std::filesystem::path& sourcePath,
  const std::string& parserReason) {
  std::ostringstream reason;
  reason << "Liberty parser error in file `";
  reason << sourcePath.string();
  reason << "`: ";
  reason << parserReason;
  return reason.str();
}

std::string ensureFileContext(
  const std::filesystem::path& sourcePath,
  const std::string& reasonWithoutGuaranteedPath) {
  if (reasonWithoutGuaranteedPath.find(sourcePath.string()) !=
      std::string::npos) {
    return reasonWithoutGuaranteedPath;
  }
  std::ostringstream reason;
  reason << "Liberty loading error in file `" << sourcePath.string()
         << "`: " << reasonWithoutGuaranteedPath;
  return reason.str();
}

BusType findChildBusType(
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* child,
  const SNLDesign* primitive) {
  BusType busType;
  if (child->id != "bus") {
    return busType;
  }
  for (auto busTypeChild: child->children) {
    if (busTypeChild->id == "bus_type") {
      auto busTypeName = busTypeChild->value;
      busType = findBusType(top, busTypeName);
      if (busType.name.empty()) {
        std::string message;
        message += "While constructing " + primitive->getName().getString() + " interface,";
        message += " cannot find bus type: " + busTypeName;
        throw SNLLibertyConstructorException(message);
      }
    }
  }
  return busType;
}

ConstructedTerm constructTerm(
  SNLDesign* primitive,
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* child,
  SNLBundleTerm* bundleTerm = nullptr,
  std::optional<SNLTerm::Direction> inheritedDirection = std::nullopt) {
  auto pinName = child->args[0];
  auto busType = findChildBusType(top, child, primitive);
  auto directionNode = findDirectionNode(child);
  auto snlDirection = inheritedDirection;
  if (directionNode) {
    auto direction = directionNode->value;
    if (direction == "internal") {
      return {};
    }
    snlDirection = getSNLDirection(direction);
  }
  if (not snlDirection) {
    std::ostringstream reason;
    reason << "Direction not found for " << child->id << " " << pinName;
    throw SNLLibertyConstructorException(reason.str());
  }
  ConstructedTerm term;
  if (busType.name.empty()) {
    term.scalar = bundleTerm
      ? SNLScalarTerm::create(bundleTerm, *snlDirection, NLName(pinName))
      : SNLScalarTerm::create(primitive, *snlDirection, NLName(pinName));
  } else {
    term.bus = bundleTerm
      ? SNLBusTerm::create(bundleTerm, *snlDirection, busType.msb, busType.lsb, NLName(pinName))
      : SNLBusTerm::create(primitive, *snlDirection, busType.msb, busType.lsb, NLName(pinName));
  }
  return term;
}

bool isSequentialTimingType(const std::string& timingType) {
  return timingType == "hold_rising" || timingType == "setup_rising" ||
         timingType == "rising_edge";
}

void registerSequentialClockForBitTerm(
    SNLBitTerm* term,
    const std::string& clockName,
    SequentialTermClocks& seqTermClocks) {
  // SNLDesignModeling does not represent Liberty `when` conditions. Retain
  // the conservative union of all declared dependencies while deduplicating
  // setup/hold and conditional timing groups for the same term/clock pair.
  seqTermClocks[term].insert(clockName);
}

void registerSequentialClockFromTiming(
    const Yosys::LibertyAst* child,
    SNLBitTerm* term,
    SequentialTermClocks& seqTermClocks) {
  for (auto timingNode : child->children) {
    if (timingNode->id != "timing") {
      continue;
    }
    auto timingTypeNode = timingNode->find("timing_type");
    if (timingTypeNode == nullptr ||
        !isSequentialTimingType(timingTypeNode->value)) {
      continue;
    }
    auto relatedPin = timingNode->find("related_pin");
    if (relatedPin == nullptr) {
      continue;
    }
    registerSequentialClockForBitTerm(term, relatedPin->value, seqTermClocks);
  }
}

bool isCombinationalOutputTiming(const Yosys::LibertyAst* timingNode) {
  auto timingType = findDirectChild(timingNode, "timing_type");
  return timingType == nullptr ||
         timingType->value.rfind("combinational", 0) == 0;
}

void registerTimingOnlyOutputInputs(
    const Yosys::LibertyAst* child,
    SNLBitTerm* output,
    TimingOnlyOutputInputs& outputInputs) {
  if (output == nullptr ||
      output->getDirection() == SNLTerm::Direction::Input) {
    return;
  }
  for (auto* timingNode : child->children) {
    if (timingNode->id != "timing" ||
        !isCombinationalOutputTiming(timingNode)) {
      continue;
    }
    auto relatedPin = findDirectChild(timingNode, "related_pin");
    if (relatedPin == nullptr) {
      continue;
    }
    std::istringstream pinNames(relatedPin->value);
    for (std::string pinName; pinNames >> pinName;) {
      outputInputs[output].insert(pinName);
    }
  }
}

std::string getChildValueOrThrow(
    const Yosys::LibertyAst* node,
    const std::string& childID,
    const std::string& context) {
  auto child = findDirectChild(node, childID);
  if (child == nullptr) {
    std::ostringstream reason;
    reason << "Missing `" << childID << "` while processing " << context;
    throw SNLLibertyConstructorException(reason.str());
  }
  return child->value;
}

std::optional<LibertyMemoryReadPort> extractMemoryReadPort(
    const Yosys::LibertyAst* child,
    SNLBusTerm* constructedBusTerm) {
  if (constructedBusTerm == nullptr) {
    return std::nullopt;
  }
  auto memoryRead = findDirectChild(child, "memory_read");
  if (memoryRead == nullptr) {
    return std::nullopt;
  }
  LibertyMemoryReadPort port;
  port.dataBus = constructedBusTerm;
  port.addressName = getChildValueOrThrow(
      memoryRead, "address", "Liberty memory_read for `" + child->args[0] + "`");
  for (auto timingNode : child->children) {
    if (timingNode->id != "timing") {
      continue;
    }
    auto timingTypeNode = timingNode->find("timing_type");
    if (timingTypeNode == nullptr || timingTypeNode->value != "rising_edge") {
      continue;
    }
    auto relatedPin = timingNode->find("related_pin");
    if (relatedPin != nullptr) {
      port.clockName = relatedPin->value;
      break;
    }
  }
  return port;
}

std::optional<LibertyMemoryWriteBus> extractMemoryWriteBus(
    const Yosys::LibertyAst* child,
    SNLBusTerm* constructedBusTerm) {
  if (constructedBusTerm == nullptr) {
    return std::nullopt;
  }
  auto memoryWrite = findDirectChild(child, "memory_write");
  if (memoryWrite == nullptr) {
    return std::nullopt;
  }
  LibertyMemoryWriteBus bus;
  bus.dataBus = constructedBusTerm;
  bus.addressName = getChildValueOrThrow(
      memoryWrite, "address", "Liberty memory_write for `" + child->args[0] + "`");
  bus.clockName = getChildValueOrThrow(
      memoryWrite, "clocked_on", "Liberty memory_write for `" + child->args[0] + "`");
  return bus;
}

void populateMemoryInterfaceHeader(
    const Yosys::LibertyAst* cell,
    LibertyMemoryInterface& memoryInterface) {
  auto memoryNode = findDirectChild(cell, "memory");
  if (memoryNode == nullptr) {
    return;
  }
  memoryInterface.present = true;
  auto addressWidthNode = findDirectChild(memoryNode, "address_width");
  auto wordWidthNode = findDirectChild(memoryNode, "word_width");
  if (addressWidthNode != nullptr) {
    memoryInterface.addressWidth = static_cast<size_t>(
        std::stoull(addressWidthNode->value));
  }
  if (wordWidthNode != nullptr) {
    memoryInterface.wordWidth = static_cast<size_t>(
        std::stoull(wordWidthNode->value));
  }
}

void registerConstructedTermModeling(
  const Yosys::LibertyAst* child,
  const std::string& pinName,
  FunctionParsingType functionParsingType,
  SNLScalarTerm* constructedScalarTerm,
  SNLBusTerm* constructedBusTerm,
  TermFunctions& termFunctions,
  TermPins& termFunctionPins,
  TermSourceLines& termFunctionLines,
  SequentialTermClocks& seqTermClocks,
  TimingOnlyOutputInputs& timingOnlyOutputInputs,
  LibertyMemoryInterface& memoryInterface,
  const Yosys::LibertyAst* functionFallback = nullptr) {
  auto registerNextStateRole = [&](SNLBitTerm* term) {
    auto nextStateType = findDirectChild(child, "nextstate_type");
    if (term == nullptr || nextStateType == nullptr) {
      return;
    }
    using Role = SNLDesignModeling::SNLTermRole;
    if (nextStateType->value == "data") {
      SNLDesignModeling::setTermRole(term, Role::DataInput);
    } else if (nextStateType->value == "scan_in") {
      SNLDesignModeling::setTermRole(term, Role::ScanInput);
    } else if (nextStateType->value == "scan_enable") {
      SNLDesignModeling::setTermRole(term, Role::ScanEnable);
    }
  };
  registerNextStateRole(constructedScalarTerm);
  if (constructedBusTerm) {
    for (auto* bit : constructedBusTerm->getBits()) {
      registerNextStateRole(bit);
    }
  }

  if (shouldParseSequentialTiming(functionParsingType)) {
    if (constructedScalarTerm) {
      registerSequentialClockFromTiming(child, constructedScalarTerm, seqTermClocks);
    } else if (constructedBusTerm) {
      for (auto* bit : constructedBusTerm->getBits()) {
        registerSequentialClockFromTiming(child, bit, seqTermClocks);
      }
    }
  }

  if (functionParsingType == FunctionParsingType::TimingOnly) {
    if (constructedScalarTerm) {
      registerTimingOnlyOutputInputs(
          child, constructedScalarTerm, timingOnlyOutputInputs);
    } else if (constructedBusTerm) {
      for (auto* bit : constructedBusTerm->getBits()) {
        registerTimingOnlyOutputInputs(child, bit, timingOnlyOutputInputs);
      }
    }
  }

  if (memoryInterface.present) {
    if (auto readPort = extractMemoryReadPort(child, constructedBusTerm);
        readPort.has_value()) {
      memoryInterface.readPorts.push_back(*readPort);
    }
    if (auto writeBus = extractMemoryWriteBus(child, constructedBusTerm);
        writeBus.has_value()) {
      memoryInterface.writeBuses.push_back(*writeBus);
    }
  }
  if (shouldParseOutputFunctions(functionParsingType)
    and constructedScalarTerm
    and constructedScalarTerm->getDirection() == SNLTerm::Direction::Output) {
    termFunctionPins[constructedScalarTerm] = pinName;
    auto functionNode = const_cast<Yosys::LibertyAst*>(child)->find("function");
    if (functionNode == nullptr && functionFallback != nullptr
        && functionParsingType == FunctionParsingType::Sequential) {
      // Liberty allows `function` on a bundle group; it applies to every
      // member. Only the sequential path resolves bundle-name identifiers
      // (per-bit aliases), so combinational bundles keep their old behavior.
      functionNode = const_cast<Yosys::LibertyAst*>(functionFallback)->find("function");
    }
    if (functionNode) {
      termFunctions[constructedScalarTerm] = functionNode->value;
      termFunctionLines[constructedScalarTerm] = functionNode->line;
    }
  } else if (shouldParseOutputFunctions(functionParsingType)
    and constructedBusTerm
    and constructedBusTerm->getDirection() == SNLTerm::Direction::Output
    and const_cast<Yosys::LibertyAst*>(child)->find("function") != nullptr) {
    std::ostringstream reason;
    reason << "No support for function for bus term while processing " << child->id << " " << pinName;
    throw SNLLibertyConstructorException(reason.str());
  }
}

void addTimingOnlyCombinatorialArcs(
    SNLDesign* primitive,
    const std::vector<SNLBitTerm*>& outputs,
    const TimingOnlyOutputInputs& declaredOutputInputs,
    const SequentialTermClocks& sequentialTermClocks) {
  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> allInputs;
  for (auto* term : primitive->getBitTerms()) {
    if (term->getDirection() != SNLTerm::Direction::Output) {
      allInputs.insert(term);
    }
  }

  for (auto* output : outputs) {
    std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> inputs;
    if (auto declared = declaredOutputInputs.find(output);
        declared != declaredOutputInputs.end()) {
      for (const auto& inputName : declared->second) {
        auto* relatedTerm = primitive->getTerm(NLName(inputName));
        auto addInput = [&](SNLBitTerm* input) {
          if (input->getDirection() == SNLTerm::Direction::Output) {
            std::ostringstream reason;
            reason << "Combinational timing `related_pin` `" << inputName
                   << "` for term `" << output->getName().getString()
                   << "` resolves to an output term";
            throw SNLLibertyConstructorException(reason.str());
          }
          if (input != output) {
            inputs.insert(input);
          }
        };
        if (auto* scalar = dynamic_cast<SNLScalarTerm*>(relatedTerm)) {
          addInput(scalar);
        } else if (auto* bus = dynamic_cast<SNLBusTerm*>(relatedTerm)) {
          for (auto* bit : bus->getBits()) {
            addInput(bit);
          }
        } else {
          std::ostringstream reason;
          reason << "Combinational timing `related_pin` `" << inputName
                 << "` for term `" << output->getName().getString()
                 << "` does not resolve to an input or inout term";
          throw SNLLibertyConstructorException(reason.str());
        }
      }
    } else if (sequentialTermClocks.find(output) ==
               sequentialTermClocks.end()) {
      // Timing arcs create an explicit modeling property, which disables the
      // normal opaque-cell fallback in dependency queries. Preserve that
      // conservative fallback for outputs with no usable declared timing.
      inputs = allInputs;
      inputs.erase(output);
    }

    if (!inputs.empty()) {
      SNLDesignModeling::BitTerms inputTerms(inputs.begin(), inputs.end());
      SNLDesignModeling::addCombinatorialArcs(inputTerms, {output});
    }
  }
}

std::vector<std::string> getBundleMembers(const Yosys::LibertyAst* bundleNode) {
  auto membersNode = findDirectChild(bundleNode, "members");
  if (membersNode == nullptr or membersNode->args.empty()) {
    std::ostringstream reason;
    reason << "Bundle " << bundleNode->args[0] << " does not define members(...)";
    throw SNLLibertyConstructorException(reason.str());
  }
  return membersNode->args;
}

std::map<std::string, const Yosys::LibertyAst*> collectBundleMemberDefinitions(
  const Yosys::LibertyAst* bundleNode) {
  std::map<std::string, const Yosys::LibertyAst*> members;
  for (auto child: bundleNode->children) {
    if (child->id == "bundle") {
      std::ostringstream reason;
      reason << "Nested Liberty bundles are not supported in bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
    if (child->id != "pin" and child->id != "bus") {
      continue;
    }
    if (child->args.empty()) {
      std::ostringstream reason;
      reason << "Malformed Liberty bundle member definition in bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
    auto inserted = members.emplace(child->args[0], child);
    if (not inserted.second) {
      std::ostringstream reason;
      reason << "Duplicate Liberty bundle member definition " << child->args[0]
             << " in bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  return members;
}

SNLTerm::Direction inferBundleDirection(
  const Yosys::LibertyAst* bundleNode,
  const std::vector<std::string>& orderedMembers,
  const std::map<std::string, const Yosys::LibertyAst*>& memberDefinitions) {
  auto bundleDirectionNode = findDirectChild(bundleNode, "direction");
  if (bundleDirectionNode != nullptr) {
    for (const auto& memberName: orderedMembers) {
      auto it = memberDefinitions.find(memberName);
      if (it == memberDefinitions.end()) {
        std::ostringstream reason;
        reason << "Bundle " << bundleNode->args[0] << " lists missing member " << memberName;
        throw SNLLibertyConstructorException(reason.str());
      }
      auto directionNode = findDirectionNode(it->second);
      if (directionNode != nullptr and directionNode->value != bundleDirectionNode->value) {
        std::ostringstream reason;
        reason << "Inconsistent child directions for bundle " << bundleNode->args[0];
        throw SNLLibertyConstructorException(reason.str());
      }
    }
    return getSNLDirection(bundleDirectionNode->value);
  }

  const Yosys::LibertyAst* inferredDirectionNode = nullptr;
  for (const auto& memberName: orderedMembers) {
    auto it = memberDefinitions.find(memberName);
    if (it == memberDefinitions.end()) {
      std::ostringstream reason;
      reason << "Bundle " << bundleNode->args[0] << " lists missing member " << memberName;
      throw SNLLibertyConstructorException(reason.str());
    }
    auto directionNode = findDirectionNode(it->second);
    if (directionNode == nullptr) {
      std::ostringstream reason;
      reason << "Direction not found for bundle member " << memberName;
      throw SNLLibertyConstructorException(reason.str());
    }
    if (inferredDirectionNode == nullptr) {
      inferredDirectionNode = directionNode;
      continue;
    }
    if (inferredDirectionNode->value != directionNode->value) {
      std::ostringstream reason;
      reason << "Inconsistent child directions for bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  return getSNLDirection(inferredDirectionNode->value);
}

SNLDesignModeling::BooleanExpression parseSequentialExpression(
    const SNLDesign* primitive,
    const std::string& expression,
    const SNLBooleanTree::StateIdentifiers& stateIdentifiers,
    const SNLBooleanTree::TermAliases& termAliases,
    const std::string& context,
    int sourceLine) {
  try {
    SNLBooleanTree tree;
    tree.parse(primitive, expression, stateIdentifiers, termAliases);
    return tree.getBooleanExpression();
  } catch (const SNLLibertyConstructorException& e) {
    std::ostringstream reason;
    reason << "Invalid " << context;
    if (sourceLine > 0) {
      reason << " at line " << sourceLine;
    }
    reason << ": `" << expression << "`: " << e.getReason();
    throw SNLLibertyConstructorException(reason.str());
  }
}

std::string getGroupDescription(const Yosys::LibertyAst* group) {
  std::ostringstream description;
  description << "`" << group->id;
  if (group->args.empty()) {
    description << "`";
    return description.str();
  }
  description << " (";
  for (size_t i = 0; i < group->args.size(); ++i) {
    if (i > 0) {
      description << ", ";
    }
    description << group->args[i];
  }
  description << ")`";
  return description.str();
}

std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> getExpressionTerms(
    const SNLDesignModeling::BooleanExpression& expression) {
  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> terms;
  for (const auto& node : expression.nodes) {
    if (node.operation == SNLDesignModeling::BooleanExpression::Operator::Term &&
        node.term != nullptr) {
      terms.insert(node.term);
    }
  }
  return terms;
}

// Evaluate all eight assignments of metadata-identified D, SI and SE at once.
// Unknown terms or state references make this model uncheckable.
std::optional<uint8_t> scanTruthTable(
    const SNLDesignModeling::BooleanExpression& expression,
    SNLBitTerm* data, SNLBitTerm* scan, SNLBitTerm* enable) {
  using Operator = SNLDesignModeling::BooleanExpression::Operator;
  std::vector<uint8_t> values;
  for (const auto& node : expression.nodes) {
    uint8_t value = 0;
    switch (node.operation) {
      case Operator::Constant: value = node.constant ? 0xff : 0; break;
      case Operator::Term:
        if (node.term == data) value = 0xaa;
        else if (node.term == scan) value = 0xcc;
        else if (node.term == enable) value = 0xf0;
        else return std::nullopt;
        break;
      case Operator::State: return std::nullopt;
      default:
        value = node.operation == Operator::And ? 0xff : 0;
        for (auto operand : node.operands) {
          if (operand >= values.size()) return std::nullopt;
          switch (node.operation) {
            case Operator::Not: value = ~values[operand]; break;
            case Operator::And: value &= values[operand]; break;
            case Operator::Or: value |= values[operand]; break;
            case Operator::Xor: value ^= values[operand]; break;
            default: return std::nullopt;
          }
        }
    }
    values.push_back(value);
  }
  if (expression.root >= values.size()) return std::nullopt;
  return values[expression.root];
}

void registerScanModeling(SNLDesign* primitive, const Yosys::LibertyAst* cell) {
  using Modeling = SNLDesignModeling;
  using Role = Modeling::SNLTermRole;
  using Level = Modeling::SNLActiveLevel;
  std::map<SNLBitTerm*, Modeling::TermRole, SNLBitTerm::InDesignLess> testRoles;
  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> conflicts;
  auto addRole = [&](SNLBitTerm* term, Role role, Level level) {
    auto [it, inserted] = testRoles.emplace(term, Modeling::TermRole{role, level});
    if (!inserted && (it->second.role != role || it->second.activeLevel != level)) {
      conflicts.insert(term);
    }
  };
  for (auto* testCell : cell->children) {
    if (testCell->id != "test_cell") continue;
    bool hasScan = false;
    bool hasEnable = false;
    for (auto* pin : testCell->children) {
      if (pin->id != "pin" || pin->args.empty()) continue;
      auto* term = dynamic_cast<SNLBitTerm*>(primitive->getTerm(NLName(pin->args[0])));
      if (!term) {
        NAJA_LOG_WARN("Liberty cell {} test_cell pin {} has no matching cell bit term; skipping.",
            primitive->getName().getString(), pin->args[0]);
        continue;
      }
      auto* type = findDirectChild(pin, "signal_type");
      if (!type) continue;
      if (type->value == "test_scan_in_inverted") {
        NAJA_LOG_WARN("Liberty cell {} test_cell pin {} inverted scan input is not represented; "
            "the pin gets no scan role from the test cell.",
            primitive->getName().getString(), pin->args[0]);
        continue;
      }
      if (type->value == "test_scan_in") {
        addRole(term, Role::ScanInput, Level::NA);
        hasScan = true;
      } else if (type->value == "test_scan_enable" ||
                 type->value == "test_scan_enable_inverted") {
        addRole(term, Role::ScanEnable, type->value == "test_scan_enable" ? Level::High : Level::Low);
        hasEnable = true;
      }
    }
    if (!hasScan || !hasEnable) continue;
    for (auto* ff : testCell->children) {
      if (ff->id != "ff") continue;
      auto* next = findDirectChild(ff, "next_state");
      if (!next) continue;
      try {
        SNLBooleanTree tree;
        tree.parse(primitive, next->value);
        auto terms = getExpressionTerms(tree.getBooleanExpression());
        std::vector<SNLBitTerm*> candidates;
        for (auto* term : terms) {
          if (term->getDirection() == SNLTerm::Direction::Input && !testRoles.count(term)) {
            candidates.push_back(term);
          }
        }
        if (candidates.size() == 1) addRole(candidates.front(), Role::DataInput, Level::NA);
      } catch (const SNLLibertyConstructorException& error) {
        NAJA_LOG_WARN("Liberty cell {} cannot read test_cell next_state: {}",
            primitive->getName().getString(), error.getReason());
      }
    }
  }
  for (const auto& [term, testRole] : testRoles) {
    auto role = Modeling::getTermRole(term);
    if (role != Role::Other && role != testRole.role) {
      // Data inference must not displace an existing non-scan role.
      if (testRole.role == Role::DataInput && role != Role::ScanInput && role != Role::ScanEnable) continue;
      conflicts.insert(term);
    }
    if (!conflicts.count(term)) Modeling::setTermRole(term, testRole.role, testRole.activeLevel);
  }
  for (auto* term : conflicts) {
    NAJA_LOG_WARN("Liberty cell {} pin {} has conflicting scan metadata; clearing its scan role.",
        primitive->getName().getString(), term->getName().getString());
    Modeling::setTermRole(term, Role::Other);
  }

  std::vector<SNLBitTerm*> data, scan, enables;
  for (auto* term : primitive->getBitTerms()) {
    switch (Modeling::getTermRole(term)) {
      case Role::DataInput: data.push_back(term); break;
      case Role::ScanInput: scan.push_back(term); break;
      case Role::ScanEnable: enables.push_back(term); break;
      default: break;
    }
  }
  if (scan.empty() || enables.empty()) return;
  Level level = Level::NA;
  if (data.size() == 1 && scan.size() == 1 && enables.size() == 1 &&
      Modeling::hasSequentialModel(primitive)) {
    const auto& model = Modeling::getSequentialModel(primitive);
    if (model.kind == Modeling::SequentialModel::Kind::FlipFlop && model.states.size() == 1) {
      auto table = scanTruthTable(model.states.front().nextState, data[0], scan[0], enables[0]);
      // D=0xaa, SI=0xcc, SE=0xf0: SE ? SI : D = 0xca; the
      // reverse selection is 0xac. Complements invert both branches.
      if (table == 0xca || table == 0x35) level = Level::High;
      else if (table == 0xac || table == 0x53) level = Level::Low;
    }
  }
  for (auto* enable : enables) {
    auto declared = Modeling::getActiveLevel(enable);
    if (level == Level::NA) {
      NAJA_LOG_WARN("Liberty cell {} pin {} next_state is not a checkable scan mux; active level is NA.",
          primitive->getName().getString(), enable->getName().getString());
    } else if (declared != Level::NA && declared != level) {
      NAJA_LOG_WARN("Liberty cell {} pin {} scan-enable level contradicts next_state; using the functional model level.",
          primitive->getName().getString(), enable->getName().getString());
    }
    Modeling::setTermRole(enable, Role::ScanEnable, level);
  }
}

SNLDesignModeling::SequentialState::ClearPresetValue getClearPresetValue(
    const Yosys::LibertyAst* ff) {
  using Value = SNLDesignModeling::SequentialState::ClearPresetValue;
  auto valueNode = findDirectChild(ff, "clear_preset_var1");
  if (valueNode == nullptr || valueNode->value.empty()) {
    return Value::Unknown;
  }
  const char value = static_cast<char>(
      std::toupper(static_cast<unsigned char>(valueNode->value.front())));
  switch (value) {
    case 'L': return Value::Zero;
    case 'H': return Value::One;
    case 'N': return Value::Hold;
    case 'T': return Value::Toggle;
    default: return Value::Unknown;
  }
}

bool hasRelatedTerm(
    const naja::NajaCollection<SNLBitTerm*>& terms,
    const SNLBitTerm* expected) {
  return std::any_of(terms.begin(), terms.end(),
      [expected](const SNLBitTerm* term) { return term == expected; });
}

void addSequentialModelArcs(
    const SNLDesignModeling::SequentialModel& model) {
  auto clockTerms = getExpressionTerms(model.clockedOn);
  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> updateTerms;
  for (const auto& state : model.states) {
    const auto nextStateTerms = getExpressionTerms(state.nextState);
    updateTerms.insert(nextStateTerms.begin(), nextStateTerms.end());
    if (state.clear.has_value()) {
      const auto clearTerms = getExpressionTerms(*state.clear);
      updateTerms.insert(clearTerms.begin(), clearTerms.end());
    }
    if (state.preset.has_value()) {
      const auto presetTerms = getExpressionTerms(*state.preset);
      updateTerms.insert(presetTerms.begin(), presetTerms.end());
    }
  }

  for (auto* clock : clockTerms) {
    SNLDesignModeling::setTermRole(
        clock, SNLDesignModeling::SNLTermRole::Clock);
    for (auto* input : updateTerms) {
      if (!hasRelatedTerm(
              SNLDesignModeling::getInputRelatedClocks(input), clock)) {
        SNLDesignModeling::addInputsToClockArcs({input}, clock);
      }
    }
    for (const auto& output : model.outputs) {
      SNLDesignModeling::setTermRole(
          output.term, SNLDesignModeling::SNLTermRole::DataOutput);
      if (!hasRelatedTerm(
              SNLDesignModeling::getOutputRelatedClocks(output.term), clock)) {
        SNLDesignModeling::addClockToOutputsArcs(clock, {output.term});
      }
    }
  }
}

void populateSequentialModel(
    SNLDesign* primitive,
    const Yosys::LibertyAst* cell,
    const TermFunctions& termFunctions,
    const TermPins& termFunctionPins,
    const TermSourceLines& termFunctionLines) {
  // Each `ff` group contributes one state and each `ff_bank (IQ, IQN, N)`
  // group contributes N states. Bank bits are addressed through bundle
  // members: bit i reads a bundle `D` as member `D<i>`, and an output bundle
  // declared `function : "IQ"` maps member i to state i.
  struct StateGroup {
    const Yosys::LibertyAst* node;
    size_t base;
    size_t width;
  };
  std::vector<StateGroup> groups;
  size_t stateCount = 0;
  bool hasBank = false;
  for (auto* child : cell->children) {
    size_t width = 0;
    if (child->id == "ff" || child->id == "latch") {
      width = 1;
    } else if (child->id == "ff_bank") {
      if (child->args.size() < 3) {
        return;
      }
      try {
        width = std::stoul(child->args[2]);
      } catch (const std::exception&) {
        return;
      }
      if (width == 0) {
        return;
      }
      hasBank = true;
    } else {
      continue;
    }
    groups.push_back({child, stateCount, width});
    stateCount += width;
  }
  if (groups.empty() || termFunctions.empty()) {
    return;
  }

  // A `latch` group is level-sensitive: `enable` takes the role of
  // `clocked_on` and `data_in` that of `next_state`. SequentialModel has one
  // shared control, so a cell mixing flip-flop and latch state, or using
  // `latch_bank`, keeps no model and stays loadable.
  const bool hasLatch = std::any_of(
      groups.begin(), groups.end(),
      [](const StateGroup& group) { return group.node->id == "latch"; });
  const bool allLatches = std::all_of(
      groups.begin(), groups.end(),
      [](const StateGroup& group) { return group.node->id == "latch"; });
  if ((hasLatch && !allLatches) ||
      findDirectChild(cell, "latch_bank") != nullptr) {
    return;
  }
  const char* const controlName = hasLatch ? "enable" : "clocked_on";
  const char* const dataName = hasLatch ? "data_in" : "next_state";

  const Yosys::LibertyAst* firstClockedOn = nullptr;
  for (const auto& group : groups) {
    if (group.node->args.empty()) {
      return;
    }
    auto* clockedOn = findDirectChild(group.node, controlName);
    auto* nextState = findDirectChild(group.node, dataName);
    if (clockedOn == nullptr || nextState == nullptr) {
      return;
    }
    if (firstClockedOn == nullptr) {
      firstClockedOn = clockedOn;
    } else if (clockedOn->value != firstClockedOn->value) {
      // SequentialModel has one clock expression shared by all states. Keep
      // the primitive loadable when Liberty describes independently-clocked
      // state groups, but do not attach a model that would misrepresent them.
      return;
    }
  }

  // State identifiers as seen from bank bit `bit`: `ff` groups keep their
  // single state, `ff_bank` groups expose their bit-th state.
  auto stateIdentifiersForBit = [&groups](size_t bit) {
    SNLBooleanTree::StateIdentifiers identifiers;
    for (const auto& group : groups) {
      const size_t index =
          group.base + (group.width > 1 ? std::min(bit, group.width - 1) : 0);
      identifiers.emplace(
          group.node->args[0], SNLBooleanTree::StateIdentifier{index, false});
      if (group.node->args.size() > 1) {
        identifiers.emplace(
            group.node->args[1], SNLBooleanTree::StateIdentifier{index, true});
      }
    }
    return identifiers;
  };
  // Bundle names used in expressions resolve to the bundle member for `bit`.
  auto termAliasesForBit = [primitive](size_t bit) {
    SNLBooleanTree::TermAliases aliases;
    for (auto* bundle : primitive->getBundleTerms()) {
      if (bit >= bundle->getNumMembers()) {
        continue;
      }
      if (auto* member = dynamic_cast<SNLBitTerm*>(bundle->getMember(bit))) {
        aliases.emplace(bundle->getName().getString(), member);
      }
    }
    return aliases;
  };
  auto bitOfTerm = [](const SNLScalarTerm* term) -> size_t {
    if (auto* bundle = term->getBundleOwner()) {
      for (size_t i = 0; i < bundle->getNumMembers(); ++i) {
        if (bundle->getMember(i) == term) {
          return i;
        }
      }
    }
    return 0;
  };

  auto buildModel = [&]() {
    SNLDesignModeling::SequentialModel model;
    model.kind = hasLatch ? SNLDesignModeling::SequentialModel::Kind::Latch
                          : SNLDesignModeling::SequentialModel::Kind::FlipFlop;
    model.clockedOn = parseSequentialExpression(
        primitive,
        firstClockedOn->value,
        stateIdentifiersForBit(0),
        termAliasesForBit(0),
        "`" + std::string(controlName) + "` expression for " +
            getGroupDescription(groups.front().node),
        firstClockedOn->line);
    for (const auto& group : groups) {
      auto* nextState = findDirectChild(group.node, dataName);
      auto* clear = findDirectChild(group.node, "clear");
      auto* preset = findDirectChild(group.node, "preset");
      for (size_t bit = 0; bit < group.width; ++bit) {
        const auto identifiers = stateIdentifiersForBit(bit);
        const auto aliases = termAliasesForBit(bit);
        SNLDesignModeling::SequentialState state;
        state.nextState = parseSequentialExpression(
            primitive,
            nextState->value,
            identifiers,
            aliases,
            "`" + std::string(dataName) + "` expression for " +
                getGroupDescription(group.node),
            nextState->line);
        if (clear != nullptr) {
          state.clear = parseSequentialExpression(
              primitive,
              clear->value,
              identifiers,
              aliases,
              "`clear` expression for " + getGroupDescription(group.node),
              clear->line);
        }
        if (preset != nullptr) {
          state.preset = parseSequentialExpression(
              primitive,
              preset->value,
              identifiers,
              aliases,
              "`preset` expression for " + getGroupDescription(group.node),
              preset->line);
        }
        state.clearPresetValue = getClearPresetValue(group.node);
        model.states.push_back(std::move(state));
      }
    }
    for (const auto& [term, function] : termFunctions) {
      const size_t bit = bitOfTerm(term);
      model.outputs.push_back({
          term,
          parseSequentialExpression(
              primitive,
              function,
              stateIdentifiersForBit(bit),
              termAliasesForBit(bit),
              "`function` expression for pin `" +
                  termFunctionPins.at(term) + "`",
              termFunctionLines.at(term))});
    }
    return model;
  };

  SNLDesignModeling::SequentialModel model;
  if (hasBank) {
    try {
      model = buildModel();
    } catch (const SNLLibertyConstructorException& e) {
      NAJA_LOG_WARN(
        "In SNLLibertyConstructor, cell {} uses `ff_bank` modeling that could "
        "not be attached ({}); leaving its sequential model unset.",
        primitive->getName().getString(),
        e.getReason());
      return;
    }
  } else {
    model = buildModel();
  }
  SNLDesignModeling::setSequentialModel(primitive, model);
  addSequentialModelArcs(model);
}

void parseTerms(
  SNLDesign* primitive,
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* cell,
  FunctionParsingType functionParsingType = FunctionParsingType::Ignore) {
  TermFunctions termFunctions;
  TermPins termFunctionPins;
  TermSourceLines termFunctionLines;
  SequentialTermClocks seqTermClocks;
  TimingOnlyOutputInputs timingOnlyOutputInputs;
  LibertyMemoryInterface memoryInterface;
  populateMemoryInterfaceHeader(cell, memoryInterface);
  for (auto child: cell->children) {
    try {
      if ((child->id == "pin" || child->id == "bus" ||
           child->id == "bundle") && child->args.empty()) {
        throw SNLLibertyConstructorException(
            "The `" + child->id + "` group must specify a name.");
      }
      if (child->id == "pin" or child->id == "bus") {
        auto constructedTerm = constructTerm(primitive, top, child);
        registerConstructedTermModeling(
          child,
          child->args[0],
          functionParsingType,
          constructedTerm.scalar,
          constructedTerm.bus,
          termFunctions,
          termFunctionPins,
          termFunctionLines,
          seqTermClocks,
          timingOnlyOutputInputs,
          memoryInterface);
      } else if (child->id == "bundle") {
        auto bundleDirectionNode = findDirectionNode(child);
        if (bundleDirectionNode != nullptr and bundleDirectionNode->value == "internal") {
          continue;
        }
        auto orderedMembers = getBundleMembers(child);
        auto memberDefinitions = collectBundleMemberDefinitions(child);
        std::set<std::string> orderedMemberSet(orderedMembers.begin(), orderedMembers.end());
        for (const auto& [memberName, _]: memberDefinitions) {
          if (orderedMemberSet.find(memberName) == orderedMemberSet.end()) {
            std::ostringstream reason;
            reason << "Bundle " << child->args[0] << " defines extra member " << memberName;
            throw SNLLibertyConstructorException(reason.str());
          }
        }
        auto bundleDirection = inferBundleDirection(child, orderedMembers, memberDefinitions);
        auto bundleTerm = SNLBundleTerm::create(primitive, bundleDirection, NLName(child->args[0]));
        for (const auto& memberName: orderedMembers) {
          auto memberIt = memberDefinitions.find(memberName);
          auto memberNode = memberIt->second;
          auto constructedTerm = constructTerm(primitive, top, memberNode, bundleTerm, bundleDirection);
          registerConstructedTermModeling(
            memberNode,
            memberName,
            functionParsingType,
            constructedTerm.scalar,
            constructedTerm.bus,
            termFunctions,
            termFunctionPins,
            termFunctionLines,
            seqTermClocks,
            timingOnlyOutputInputs,
            memoryInterface,
            child);
        }
      }
    } catch (const SNLLibertyConstructorException& e) {
      std::ostringstream reason;
      reason << "While processing " << getGroupDescription(child);
      if (child->line > 0) {
        reason << " at line " << child->line;
      }
      reason << ": " << e.getReason();
      throw SNLLibertyConstructorException(reason.str());
    } catch (const std::exception& e) {
      std::ostringstream reason;
      reason << "While processing " << getGroupDescription(child);
      if (child->line > 0) {
        reason << " at line " << child->line;
      }
      reason << ": " << e.what();
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  std::vector<SNLBitTerm*> outputs;
  for (auto term: primitive->getBitTerms()) {
    if (term->getDirection() != SNLTerm::Direction::Input) {
      outputs.push_back(term);
    }
  }
  if (functionParsingType == FunctionParsingType::Combinational &&
      termFunctions.size() != outputs.size()) {
    NAJA_LOG_WARN(
      "In SNLLibertyConstructor, cell {} defines a `function` for {} of {} "
      "non-input terms; leaving all truth tables uninitialized.",
      primitive->getName().getString(),
      termFunctions.size(),
      outputs.size());
    return;
  }
  if (functionParsingType == FunctionParsingType::TimingOnly) {
    addTimingOnlyCombinatorialArcs(
        primitive, outputs, timingOnlyOutputInputs, seqTermClocks);
  }
  if (seqTermClocks.size() > 0) {
    for (const auto& [term, clockNames]: seqTermClocks) {
      for (const auto& clockName: clockNames) {
        auto* relatedTerm = primitive->getTerm(NLName(clockName));
        auto* clock = dynamic_cast<SNLScalarTerm*>(relatedTerm);
        if (clock == nullptr ||
            clock->getDirection() != SNLTerm::Direction::Input) {
          std::ostringstream reason;
          reason << "Sequential timing `related_pin` `" << clockName
                 << "` for term `" << term->getName().getString()
                 << "` does not resolve to a scalar input term";
          throw SNLLibertyConstructorException(reason.str());
        }
        if (functionParsingType == FunctionParsingType::TimingOnly) {
          SNLDesignModeling::setTermRole(
              clock, SNLDesignModeling::SNLTermRole::Clock);
        }
        if (term->getDirection() == SNLTerm::Direction::Input) {
          SNLDesignModeling::addInputsToClockArcs({term}, clock);
        } else if (term->getDirection() == SNLTerm::Direction::Output) {
          SNLDesignModeling::addClockToOutputsArcs(clock, {term});
        }
      }
    }
    if (memoryInterface.present) {
      // Liberty describes memories with separate memory_read/memory_write
      // attributes on data buses plus timing arcs on address/control pins.
      // Normalize those pieces into SNLDesignModeling::MemoryInterface so
      // later passes can discover memories through one generic API.
      SNLDesignModeling::MemoryInterface modelingInterface;
      modelingInterface.width = memoryInterface.wordWidth;
      modelingInterface.abits = memoryInterface.addressWidth;
      modelingInterface.depth = memoryInterface.addressWidth == 0
          ? 0
          : (static_cast<size_t>(1) << memoryInterface.addressWidth);
      if (!memoryInterface.readPorts.empty()) {
        const auto& firstReadPort = memoryInterface.readPorts.front();
        if (!firstReadPort.clockName.empty()) {
          // LCOV_EXCL_START
          modelingInterface.clock = dynamic_cast<SNLScalarTerm*>(
              primitive->getTerm(NLName(firstReadPort.clockName)));
          // LCOV_EXCL_STOP
        }
      }

      std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> usedInputTerms;
      for (const auto& readPort : memoryInterface.readPorts) {
        SNLDesignModeling::MemoryReadPort modelingReadPort;
        auto* addressTerm = primitive->getTerm(NLName(readPort.addressName));
        auto* addressBus = dynamic_cast<SNLBusTerm*>(addressTerm);
        if (addressBus == nullptr) {
          std::ostringstream reason;
          reason << "Memory read address `" << readPort.addressName
                 << "` is not a bus in cell `" << primitive->getName().getString()
                 << "`";
          throw SNLLibertyConstructorException(reason.str());
        }
        for (auto* bit : addressBus->getBits()) {
          modelingReadPort.address.push_back(bit);
          usedInputTerms.insert(bit);
        }
        for (auto* bit : readPort.dataBus->getBits()) {
          modelingReadPort.data.push_back(bit);
        }
        SNLDesignModeling::addCombinatorialArcs(
            modelingReadPort.address, modelingReadPort.data);
        modelingInterface.readPorts.push_back(std::move(modelingReadPort));
      }

      struct WritePortGroupKey {
        std::string addressName;
        std::string clockName;

        bool operator<(const WritePortGroupKey& other) const {
          return std::tie(addressName, clockName) <
                 std::tie(other.addressName, other.clockName);
        }
      };
      std::map<WritePortGroupKey, std::vector<SNLBusTerm*>> groupedWriteBuses;
      for (const auto& writeBus : memoryInterface.writeBuses) {
        groupedWriteBuses[{writeBus.addressName, writeBus.clockName}].push_back(
            writeBus.dataBus);
      }
      for (const auto& [key, buses] : groupedWriteBuses) {
        SNLDesignModeling::MemoryWritePort modelingWritePort;
        auto* addressTerm = primitive->getTerm(NLName(key.addressName));
        auto* addressBus = dynamic_cast<SNLBusTerm*>(addressTerm);
        if (addressBus == nullptr) {
          std::ostringstream reason;
          reason << "Memory write address `" << key.addressName
                 << "` is not a bus in cell `" << primitive->getName().getString()
                 << "`";
          throw SNLLibertyConstructorException(reason.str());
        }
        for (auto* bit : addressBus->getBits()) {
          modelingWritePort.address.push_back(bit);
          usedInputTerms.insert(bit);
        }
        if (!buses.empty()) {
          for (auto* bit : buses.front()->getBits()) {
            modelingWritePort.data.push_back(bit);
            usedInputTerms.insert(bit);
          }
        }
        if (buses.size() >= 2) {
          for (auto* bit : buses[1]->getBits()) {
            modelingWritePort.mask.push_back(bit);
            usedInputTerms.insert(bit);
          }
        }
        for (size_t busIndex = 2; busIndex < buses.size(); ++busIndex) {
          SNLDesignModeling::BitTerms extraWriteInput;
          for (auto* bit : buses[busIndex]->getBits()) {
            extraWriteInput.push_back(bit);
            usedInputTerms.insert(bit);
          }
          modelingWritePort.extraWriteInputs.push_back(std::move(extraWriteInput));
        }
        for (const auto& [timedTerm, timedClockNames] : seqTermClocks) {
          if (timedClockNames.find(key.clockName) == timedClockNames.end() ||
              timedTerm->getDirection() == SNLTerm::Direction::Output ||
              usedInputTerms.find(timedTerm) != usedInputTerms.end()) {
            continue;
          }
          modelingWritePort.enables.push_back(timedTerm);
        }
        modelingInterface.writePorts.push_back(std::move(modelingWritePort));
      }

      if (modelingInterface.clock != nullptr && modelingInterface.isValid()) {
        SNLDesignModeling::setMemoryInterface(primitive, modelingInterface);
      }
    }
  } else if (functionParsingType == FunctionParsingType::Combinational &&
             termFunctions.size() == 1 && outputs.size() == 1) {
    auto outputTerm = termFunctions.begin()->first;
    auto pinName = termFunctionPins[outputTerm];
    auto function = termFunctions.begin()->second;
    auto tree = std::make_unique<naja::NL::SNLBooleanTree>();
    try {
      //std::cerr << "Parsing function: " << function << std::endl;
      tree->parse(primitive, function);
      naja::NL::SNLBooleanTree::Terms terms;
      for (auto term: primitive->getBitTerms()) {
        if (term->getDirection() == SNLTerm::Direction::Input) {
          terms.push_back(term);
        }
      }
      std::reverse(terms.begin(), terms.end());
      auto truthTable = tree->getTruthTable(terms);
      naja::NL::SNLDesignModeling::setTruthTable(primitive, truthTable);
      if (auto gate = tree->getInvertedInputGate()) {
        SNLDesignModeling::setInvertedInputGate(primitive, *gate);
      }
      SNLDesignModeling::setFunction(outputTerm, tree->getBooleanExpression());
    } catch (const SNLLibertyConstructorException& e) {
      auto reason = buildFunctionParseErrorReason(
        pinName, termFunctionLines[outputTerm], function, e.getReason());
      throw SNLLibertyConstructorException(reason);
    }
  } else if (functionParsingType == FunctionParsingType::Combinational &&
             termFunctions.size() > 0) {
    std::vector<SNLTruthTable> truthTables;
    std::vector<std::pair<SNLBitTerm*, SNLDesignModeling::BooleanExpression>> functions;
    for (auto term: primitive->getBitTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Input) {
        continue;
      }
      auto* scalarTerm = dynamic_cast<SNLScalarTerm*>(term);
      auto& function = termFunctions.at(scalarTerm);
      auto pinName = termFunctionPins.at(scalarTerm);
      auto tree = std::make_unique<naja::NL::SNLBooleanTree>();
      try {
        //std::cerr << "Parsing function: " << function << std::endl;
        tree->parse(primitive, function);
        naja::NL::SNLBooleanTree::Terms terms;
        for (auto term: primitive->getBitTerms()) {
          if (term->getDirection() == SNLTerm::Direction::Input) {
            terms.push_back(term);
          }
        }
        std::reverse(terms.begin(), terms.end());
        auto truthTable = tree->getTruthTable(terms);
        truthTables.push_back(truthTable);
        functions.emplace_back(term, tree->getBooleanExpression());
      } catch (const SNLLibertyConstructorException& e) {
        auto reason = buildFunctionParseErrorReason(
          pinName, termFunctionLines.at(scalarTerm), function, e.getReason());
        throw SNLLibertyConstructorException(reason);
      }
    }
    naja::NL::SNLDesignModeling::setTruthTables(primitive, truthTables);
    for (const auto& [term, function] : functions) SNLDesignModeling::setFunction(term, function);
  }
  if (functionParsingType == FunctionParsingType::Sequential) {
    populateSequentialModel(
        primitive,
        cell,
        termFunctions,
        termFunctionPins,
        termFunctionLines);
  }
  registerScanModeling(primitive, cell);
}

void parseCell(
  NLLibrary* library,
  const Yosys::LibertyAst* top,
  Yosys::LibertyAst* cell,
  const std::filesystem::path& sourcePath,
  ConflictingCellNamePolicy conflictingCellNamePolicy) {
  try {
    if (cell->args.empty()) {
      throw SNLLibertyConstructorException(
          "The `cell` group must specify a cell name.");
    }
    const auto& cellName = cell->args[0];
    if (library->getSNLDesign(NLName(cellName))) {
      if (conflictingCellNamePolicy == ConflictingCellNamePolicy::FirstOne) {
        NAJA_LOG_WARN(
          "In SNLLibertyConstructor, cell {} from {} already exists in "
          "library {}, ignoring this definition.",
          cellName,
          sourcePath.string(),
          library->getDescription());
        return;
      }
      std::ostringstream reason;
      reason << "Library `" << library->getString()
             << "` already contains a design named `" << cellName << "`.";
      throw SNLLibertyConstructorException(reason.str());
    }
    auto primitive = SNLDesign::create(
        library, SNLDesign::Type::Primitive, NLName(cellName));
    const bool hasUnsupportedStateModel =
        containsLibertyNode(cell, "statetable") ||
        containsLibertyNode(cell, "state_function");
    if (hasUnsupportedStateModel) {
      NAJA_LOG_WARN(
        "In SNLLibertyConstructor, cell {} from {} uses unsupported Liberty "
        "statetable/state_function behavioral modeling; leaving its truth "
        "table uninitialized while preserving independently supported "
        "timing/modeling.",
        cellName,
        sourcePath.string());
    }
    FunctionParsingType type = FunctionParsingType::Combinational;
    if (cell->find("ff") || cell->find("ff_bank") || cell->find("memory") ||
        cell->find("latch")) {
      type = FunctionParsingType::Sequential;
    } else if (cell->find("latch_bank")) {
      type = FunctionParsingType::Ignore;
    } else if (hasUnsupportedStateModel) {
      type = FunctionParsingType::TimingOnly;
    }
    parseTerms(primitive, top, cell, type);
  } catch (const SNLLibertyConstructorException& e) {
    throw SNLLibertyConstructorException(
        buildCellErrorReason(cell, sourcePath, e.getReason()));
  } catch (const std::exception& e) {
    throw SNLLibertyConstructorException(
        buildCellErrorReason(cell, sourcePath, e.what()));
  }
}

void parseCells(
  NLLibrary* library,
  const Yosys::LibertyAst* ast,
  const std::filesystem::path& sourcePath,
  ConflictingCellNamePolicy conflictingCellNamePolicy, naja::NajaProgressReporter& progress) {
  uint64_t total = 0;
  if (progress) {
    for (auto child : ast->children) if (child->id == "cell") ++total;
    progress.begin("liberty", sourcePath.string(), total);
  }
  for (auto child: ast->children) {
    if (child->id == "cell") {
      parseCell(library, ast, child, sourcePath, conflictingCellNamePolicy);
      if (progress) progress.advance(child->args.empty() ? std::string_view{} : child->args.front());
    }
  }
}

class GzipStreamBuf final : public std::streambuf {
  public:
    explicit GzipStreamBuf(const std::filesystem::path& path) {
      file_ = gzopen(path.string().c_str(), "rb");
      if (file_ == nullptr) {
        std::string reason("Liberty parser: failed to open gzip file: " + path.string());
        throw SNLLibertyConstructorException(reason);
      }
      setg(buffer_, buffer_ + kBufferSize, buffer_ + kBufferSize);
    }
    ~GzipStreamBuf() override {
      if (file_ != nullptr) {
        gzclose(file_);
      }
    }
  protected:
    int_type underflow() override {
      if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr()); // LCOV_EXCL_LINE
      }
      if (file_ == nullptr) {
        return traits_type::eof(); // LCOV_EXCL_LINE
      }
      int bytesRead = gzread(file_, buffer_, kBufferSize);
      if (bytesRead == 0) {
        return traits_type::eof();
      }
      if (bytesRead < 0) {
        int errnum = 0;
        const char* errorMessage = gzerror(file_, &errnum);
        std::ostringstream reason;
        reason << "Liberty parser: failed to read gzip file";
        if (errorMessage != nullptr and errnum != Z_OK) {
          reason << " (" << errorMessage << ")";
        }
        throw SNLLibertyConstructorException(reason.str());
      }
      setg(buffer_, buffer_, buffer_ + bytesRead);
      return traits_type::to_int_type(*gptr());
    }
  private:
    static constexpr int kBufferSize = 64 * 1024;
    gzFile file_{nullptr};
    char buffer_[kBufferSize];
};

class GzipIStream final : public std::istream {
  public:
    explicit GzipIStream(const std::filesystem::path& path)
      : std::istream(nullptr), buffer_(path) {
      rdbuf(&buffer_);
    }
  private:
    GzipStreamBuf buffer_;
};

std::vector<unsigned char> readFileHeader(
  const std::filesystem::path& path,
  size_t size) {
  std::ifstream input(path, std::ios::binary);
  if (not input.is_open()) {
    std::string reason("Liberty parser: failed to open file: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  std::vector<unsigned char> header(size);
  input.read(reinterpret_cast<char*>(header.data()), header.size());
  header.resize(input.gcount());
  return header;
}

bool hasGzipSignature(const std::vector<unsigned char>& header) {
  return header.size() >= 2
    and header[0] == 0x1F
    and header[1] == 0x8B;
}

bool hasZipSignature(const std::vector<unsigned char>& header) {
  return header.size() >= 4
    and header[0] == 'P'
    and header[1] == 'K'
    and header[2] == 0x03
    and header[3] == 0x04;
}

std::vector<unsigned char> readBinaryFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (not input.is_open()) {
    // LCOV_EXCL_START
    std::string reason("Liberty parser: failed to open zip file: " + path.string());
    throw SNLLibertyConstructorException(reason);
    // LCOV_EXCL_STOP
  }
  std::string contents(
    (std::istreambuf_iterator<char>(input)),
    std::istreambuf_iterator<char>());
  return std::vector<unsigned char>(contents.begin(), contents.end());
}

uint16_t readLE16(const std::vector<unsigned char>& bytes, size_t offset) {
  if (offset + 2 > bytes.size()) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive"); // LCOV_EXCL_LINE
  }
  return static_cast<uint16_t>(bytes[offset])
    | (static_cast<uint16_t>(bytes[offset + 1]) << 8);
}

uint32_t readLE32(const std::vector<unsigned char>& bytes, size_t offset) {
  if (offset + 4 > bytes.size()) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive"); // LCOV_EXCL_LINE
  }
  return static_cast<uint32_t>(bytes[offset])
    | (static_cast<uint32_t>(bytes[offset + 1]) << 8)
    | (static_cast<uint32_t>(bytes[offset + 2]) << 16)
    | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

struct ZipEntry {
  std::string name {};
  uint16_t flags {0};
  uint16_t method {0};
  uint32_t compressedSize {0};
  uint32_t uncompressedSize {0};
  uint32_t localHeaderOffset {0};

  bool isDirectory() const {
    return not name.empty() and name.back() == '/';
  }
};

size_t findEndOfCentralDirectory(const std::vector<unsigned char>& bytes) {
  constexpr uint32_t eocdSignature = 0x06054B50;
  constexpr size_t minEocdSize = 22;
  constexpr size_t maxCommentSize = 0xFFFF;
  if (bytes.size() < minEocdSize) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  auto minPos = bytes.size() > minEocdSize + maxCommentSize
    ? bytes.size() - (minEocdSize + maxCommentSize)
    : 0;
  for (size_t pos = bytes.size() - minEocdSize; pos + 1 > minPos; --pos) {
    if (readLE32(bytes, pos) == eocdSignature) {
      auto commentSize = readLE16(bytes, pos + 20);
      if (pos + minEocdSize + commentSize == bytes.size()) {
        return pos;
      }
    }
    if (pos == minPos) {
      break;
    }
  }
  throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
}

std::vector<ZipEntry> readZipEntries(const std::vector<unsigned char>& bytes) {
  constexpr uint32_t centralDirectorySignature = 0x02014B50;
  constexpr uint32_t zip64Marker = 0xFFFFFFFF;
  constexpr uint16_t zip64EntryMarker = 0xFFFF;

  auto eocdOffset = findEndOfCentralDirectory(bytes);
  auto entriesCount = readLE16(bytes, eocdOffset + 10);
  auto centralDirectorySize = readLE32(bytes, eocdOffset + 12);
  auto centralDirectoryOffset = readLE32(bytes, eocdOffset + 16);
  if (entriesCount == zip64EntryMarker
      or centralDirectorySize == zip64Marker
      or centralDirectoryOffset == zip64Marker) {
    throw SNLLibertyConstructorException("Liberty parser: zip64 archives are not supported");
  }
  auto centralDirectoryOffsetSize = static_cast<size_t>(centralDirectoryOffset);
  auto centralDirectorySizeSize = static_cast<size_t>(centralDirectorySize);
  if (centralDirectoryOffsetSize > bytes.size()
      or centralDirectorySizeSize > bytes.size() - centralDirectoryOffsetSize) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }

  std::vector<ZipEntry> entries;
  auto offset = centralDirectoryOffsetSize;
  auto centralDirectoryEnd = offset + centralDirectorySizeSize;
  for (uint16_t entryIndex = 0; entryIndex < entriesCount; ++entryIndex) {
    if (offset + 46 > centralDirectoryEnd
        or readLE32(bytes, offset) != centralDirectorySignature) {
      throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
    }

    auto fileNameSize = readLE16(bytes, offset + 28);
    auto extraSize = readLE16(bytes, offset + 30);
    auto commentSize = readLE16(bytes, offset + 32);
    auto nextOffset = offset + 46 + fileNameSize + extraSize + commentSize;
    if (nextOffset > centralDirectoryEnd) {
      throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
    }

    ZipEntry entry;
    entry.flags = readLE16(bytes, offset + 8);
    entry.method = readLE16(bytes, offset + 10);
    entry.compressedSize = readLE32(bytes, offset + 20);
    entry.uncompressedSize = readLE32(bytes, offset + 24);
    entry.localHeaderOffset = readLE32(bytes, offset + 42);
    entry.name = std::string(
      bytes.begin() + static_cast<std::ptrdiff_t>(offset + 46),
      bytes.begin() + static_cast<std::ptrdiff_t>(offset + 46 + fileNameSize));
    entries.push_back(std::move(entry));
    offset = nextOffset;
  }

  return entries;
}

bool hasLibertyName(const std::string& name) {
  return name.find(".lib") != std::string::npos;
}

const ZipEntry* selectZipEntry(const std::vector<ZipEntry>& entries) {
  const ZipEntry* firstRegularEntry = nullptr;
  size_t regularEntries = 0;
  for (const auto& entry: entries) {
    if (entry.isDirectory()) {
      continue;
    }
    ++regularEntries;
    if (firstRegularEntry == nullptr) {
      firstRegularEntry = &entry;
    }
    if (hasLibertyName(entry.name)) {
      return &entry;
    }
  }
  if (regularEntries == 1) {
    return firstRegularEntry;
  }
  return nullptr;
}

std::string inflateZipEntry(
  const std::vector<unsigned char>& compressedBytes,
  uint32_t uncompressedSize) {
  std::string output(uncompressedSize, '\0');
  if (uncompressedSize == 0) {
    return output;
  }

  z_stream stream {};
  int initStatus = inflateInit2(&stream, -MAX_WBITS);
  if (initStatus != Z_OK) {
    throw SNLLibertyConstructorException("Liberty parser: failed to initialize zip inflater"); // LCOV_EXCL_LINE
  }

  stream.next_in = const_cast<Bytef*>(
    reinterpret_cast<const Bytef*>(compressedBytes.data()));
  stream.avail_in = static_cast<uInt>(compressedBytes.size());
  stream.next_out = reinterpret_cast<Bytef*>(output.data());
  stream.avail_out = static_cast<uInt>(output.size());

  int inflateStatus = inflate(&stream, Z_FINISH);
  auto producedBytes = stream.total_out;
  inflateEnd(&stream);
  if (inflateStatus != Z_STREAM_END or producedBytes != uncompressedSize) {
    throw SNLLibertyConstructorException("Liberty parser: failed to inflate zip entry");
  }
  return output;
}

std::string extractZipEntry(
  const std::vector<unsigned char>& bytes,
  const ZipEntry& entry) {
  constexpr uint32_t localFileHeaderSignature = 0x04034B50;
  constexpr uint16_t storedMethod = 0;
  constexpr uint16_t deflateMethod = 8;
  constexpr uint16_t encryptedFlag = 1;

  if ((entry.flags & encryptedFlag) != 0) {
    throw SNLLibertyConstructorException("Liberty parser: encrypted zip entries are not supported");
  }
  auto localHeaderOffset = static_cast<size_t>(entry.localHeaderOffset);
  if (localHeaderOffset + 30 > bytes.size()
      or readLE32(bytes, localHeaderOffset) != localFileHeaderSignature) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  auto fileNameSize = readLE16(bytes, localHeaderOffset + 26);
  auto extraSize = readLE16(bytes, localHeaderOffset + 28);
  auto dataOffset = localHeaderOffset + 30 + fileNameSize + extraSize;
  auto compressedSize = static_cast<size_t>(entry.compressedSize);
  if (dataOffset + compressedSize > bytes.size()) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  auto dataBegin = bytes.begin() + static_cast<std::ptrdiff_t>(dataOffset);
  auto dataEnd = dataBegin + static_cast<std::ptrdiff_t>(compressedSize);
  std::vector<unsigned char> compressedBytes(dataBegin, dataEnd);

  if (entry.method == storedMethod) {
    if (entry.compressedSize != entry.uncompressedSize) {
      throw SNLLibertyConstructorException("Liberty parser: malformed stored zip entry");
    }
    return std::string(compressedBytes.begin(), compressedBytes.end());
  }
  if (entry.method == deflateMethod) {
    return inflateZipEntry(compressedBytes, entry.uncompressedSize);
  }

  std::ostringstream reason;
  reason << "Liberty parser: unsupported zip compression method "
         << entry.method << " for " << entry.name;
  throw SNLLibertyConstructorException(reason.str());
}

std::string extractLibertyFromZip(const std::filesystem::path& path) {
  auto bytes = readBinaryFile(path);
  auto entries = readZipEntries(bytes);
  auto selectedEntry = selectZipEntry(entries);
  if (selectedEntry == nullptr) {
    std::string reason("Liberty parser: no unambiguous Liberty file in zip archive: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  return extractZipEntry(bytes, *selectedEntry);
}

std::unique_ptr<std::istream> openLibertyStream(const std::filesystem::path& path) {
  auto header = readFileHeader(path, 4);
  if (hasGzipSignature(header)) {
    return std::make_unique<GzipIStream>(path);
  }
  if (hasZipSignature(header)) {
    auto zipContents = extractLibertyFromZip(path);
    auto zipStream = std::make_unique<std::istringstream>();
    zipStream->str(std::move(zipContents));
    return zipStream;
  }
  auto inFile = std::make_unique<std::ifstream>(path);
  if (not inFile->is_open()) {
    std::string reason("Liberty parser: failed to open file: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  return inFile;
}

}

namespace naja::NL {

SNLLibertyConstructor::SNLLibertyConstructor(NLLibrary* library):
  library_(library)
{}

bool SNLLibertyConstructor::hasLibertyExtension(const std::filesystem::path& path) {
  auto remaining = path.filename();
  while (not remaining.empty() and not remaining.extension().empty()) {
    if (remaining.extension().string().starts_with(".lib")) {
      return true;
    }
    remaining = remaining.stem();
  }
  return false;
}

bool SNLLibertyConstructor::hasCompressedLibertySignature(
  const std::filesystem::path& path) {
  try {
    auto header = readFileHeader(path, 4);
    return hasGzipSignature(header) or hasZipSignature(header);
  } catch (const SNLLibertyConstructorException&) {
    return false;
  }
}

bool SNLLibertyConstructor::isLibertyPath(const std::filesystem::path& path) {
  auto extension = path.extension();
  return hasLibertyExtension(path)
    or extension == ".gz"
    or extension == ".zip"
    or hasCompressedLibertySignature(path);
}

void SNLLibertyConstructor::construct(const std::filesystem::path& path) {
  construct(Paths{path});
}

void SNLLibertyConstructor::construct(const Paths& paths) {
  NajaPerf::Scope scope("Liberty construct");
  for (const auto& path : paths) {
    if (not std::filesystem::exists(path)) {
      std::string reason(
          "Liberty file `" + path.string() + "` does not exist.");
      throw SNLLibertyConstructorException(reason);
    }
    try {
      if (progress_) progress_.begin("parse", path.string(), 1);
      auto inStream = openLibertyStream(path);
      std::unique_ptr<Yosys::LibertyParser> parser;
      try {
        NajaPerf::Scope parseScope("Liberty parse AST");
        parser = std::make_unique<Yosys::LibertyParser>(
          *inStream, Yosys::LibertyParser::ParseMode::Structural);
      } catch (const naja::liberty::YosysLibertyException& e) {
        auto reason = buildParserFileErrorReason(path, e.getReason());
        throw SNLLibertyConstructorException(reason);
      }
      progress_.advance();
      progress_.finish();
      auto ast = parser->ast;
      //LCOV_EXCL_START
      if (ast == nullptr) {
        throw SNLLibertyConstructorException(
            "The Liberty parser produced an empty syntax tree.");
      }
      //LCOV_EXCL_STOP
      if (ast->id != "library") {
        throw SNLLibertyConstructorException(
            "Expected a top-level `library` group, found `" + ast->id + "`.");
      }
      if (ast->args.empty()) {
        throw SNLLibertyConstructorException(
            "The top-level `library` group must specify a library name.");
      }
      const auto& libraryName = ast->args[0];
      {
        NajaPerf::Scope constructScope("Liberty build SNL primitives");
        library_->setName(NLName(libraryName));
        parseCells(library_, ast, path, config_.conflictingCellNamePolicy_, progress_);
        progress_.finish();
      }
    } catch (const naja::NajaCancelled&) {
      throw;
    } catch (const SNLLibertyConstructorException& e) {
      throw SNLLibertyConstructorException(
          ensureFileContext(path, e.getReason()));
    } catch (const std::exception& e) {
      throw SNLLibertyConstructorException(
          ensureFileContext(path, e.what()));
    }
  }
  NLLibraryTruthTables::construct(library_);
  progress_.begin("done");
}

}  // namespace naja::NL
