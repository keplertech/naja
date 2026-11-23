// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLConstructor.h"

#include <cctype>
#include <iostream>
#include <charconv>
#include <optional>
#include <sstream>
#include <vector>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NajaLog.h"
#include "NajaPerf.h"

#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLBundleTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"
#include "SNLInstParameter.h"
#include "SNLParameter.h"
#include "SNLAttributes.h"
#include "SNLRTLInfos.h"
#include "SNLExceptions.h"

#include "SNLVRLConstructorUtils.h"
#include "SNLVRLConstructorException.h"

namespace {

std::vector<naja::NL::SNLTerm*> getConnectableTerms(naja::NL::SNLDesign* design) {
  std::vector<naja::NL::SNLTerm*> terms;
  for (auto term: design->getTerms()) {
    if (auto bundle = dynamic_cast<naja::NL::SNLBundleTerm*>(term)) {
      for (auto member: bundle->getMembers()) {
        terms.push_back(member);
      }
    } else {
      terms.push_back(term);
    }
  }
  return terms;
}

naja::NL::SNLRTLInfos* sourceInfos(naja::NL::NLObject* object) {
  if (auto design = dynamic_cast<naja::NL::SNLDesign*>(object)) {
    return design->getRTLInfos() ? design->getRTLInfos() : naja::NL::SNLRTLInfos::create(design);
  }
  if (auto designObject = dynamic_cast<naja::NL::SNLDesignObject*>(object)) {
    return designObject->getRTLInfos() ? designObject->getRTLInfos() : naja::NL::SNLRTLInfos::create(designObject);
  }
  return nullptr; // LCOV_EXCL_LINE: callers only pass designs or design objects.
}

std::optional<naja::NL::SNLSourceLoc> parseYosysSource(const std::string& entry) {
  // The last colon separates the numeric suffix, leaving drive letters and
  // any other colons in the opaque producer path untouched.
  const auto colon = entry.rfind(':');
  if (colon == std::string::npos || colon == 0) { return std::nullopt; }
  const char* cursor = entry.data() + colon + 1;
  const char* end = entry.data() + entry.size();
  auto coordinate = [&](std::uint32_t& value) {
    auto result = std::from_chars(cursor, end, value);
    if (result.ec != std::errc() || result.ptr == cursor) { return false; }
    cursor = result.ptr;
    return true;
  };
  naja::NL::SNLSourceLoc loc;
  if (not coordinate(loc.line)) { return std::nullopt; }
  if (cursor != end && *cursor == '.') {
    ++cursor;
    if (not coordinate(loc.column)) { return std::nullopt; }
  }
  if (cursor != end && *cursor == '-') {
    ++cursor;
    if (not coordinate(loc.endLine)) { return std::nullopt; }
    if (cursor != end && *cursor == '.') {
      ++cursor;
      if (not coordinate(loc.endColumn)) { return std::nullopt; }
    }
  }
  if (cursor != end || (loc.line && loc.endLine &&
      (loc.endLine < loc.line || (loc.endLine == loc.line && loc.column &&
       loc.endColumn && loc.endColumn < loc.column)))) { return std::nullopt; }
  // Attributes retain their raw Verilog spelling. Decode escapes only for
  // the origin path, so a quoted Windows path denotes its actual bytes.
  std::string path;
  for (size_t i = 0; i < colon; ++i) {
    char c = entry[i];
    if (c == '\\') {
      if (++i == colon) { return std::nullopt; }
      c = entry[i];
      if (c == 'n') { c = '\n'; }
      else if (c == 't') { c = '\t'; }
      else if (c >= '0' && c <= '7') {
        unsigned value = c - '0';
        for (unsigned n = 1; n < 3 && i + 1 < colon && entry[i + 1] >= '0' && entry[i + 1] <= '7'; ++n) {
          value = value * 8 + entry[++i] - '0';
        }
        if (value > 255 || value == 0) { return std::nullopt; }
        c = static_cast<char>(value);
      } else if (c != '\\' && c != '"') { return std::nullopt; }
    }
    path.push_back(c);
  }
  loc.file = naja::NL::NLName(path);
  return loc;
}

void collectAttributes(
  naja::NL::NLObject* object,
  const naja::NL::SNLVRLConstructor::Attributes& attributes) {
  if (auto design = dynamic_cast<naja::NL::SNLDesign*>(object)) {
    for (const auto& attribute: attributes) {
      naja::NL::NLName attributeName(attribute.name_.getString());
      std::string expression;
      naja::NL::SNLAttributeValue::Type valueType;
      switch (attribute.expression_.getType()) {
        case naja::verilog::ConstantExpression::Type::STRING:
          valueType = naja::NL::SNLAttributeValue::Type::STRING;
          break;
        case naja::verilog::ConstantExpression::Type::NUMBER:
          valueType = naja::NL::SNLAttributeValue::Type::NUMBER;
          break;
      }
      if (attribute.expression_.valid_) {
        expression = attribute.expression_.getString();
      }
      naja::NL::SNLAttributes::addAttribute(
        design,
        naja::NL::SNLAttribute(
          attributeName,
          naja::NL::SNLAttributeValue(valueType, expression)));
    }
  } else if (auto designObject = dynamic_cast<naja::NL::SNLDesignObject*>(object)) {
    for (const auto& attribute: attributes) {
      naja::NL::NLName attributeName(attribute.name_.getString());
      std::string expression;
      naja::NL::SNLAttributeValue::Type valueType;
      switch (attribute.expression_.getType()) {
        case naja::verilog::ConstantExpression::Type::STRING:
          valueType = naja::NL::SNLAttributeValue::Type::STRING;
          break;
        case naja::verilog::ConstantExpression::Type::NUMBER:
          valueType = naja::NL::SNLAttributeValue::Type::NUMBER;
          break;
      }
      if (attribute.expression_.valid_) {
        expression = attribute.expression_.getString();
      }
      naja::NL::SNLAttributes::addAttribute(
        designObject,
        naja::NL::SNLAttribute(
          attributeName,
          naja::NL::SNLAttributeValue(valueType, expression)));
    }
  }
}

// Canonicalizes a sized, non-negative based-number literal to the shared
// canonical bit-vector parameter form defined by NLDB0::formatBitVectorValue:
// "<width>['s]'b<msb...lsb>". Downstream consumers can then read parameter
// bits without re-implementing Verilog literal parsing.
//
// Returns nullopt — the caller keeps the source text — for values that have
// no faithful literal-level canonical form:
//   - plain integers, strings, concatenations, identifiers,
//   - unsized literals ('hF): their width is context-dependent (at least 32),
//   - negated literals (-4'b1): unary minus is evaluated at the context
//     width, which the frontend cannot see (in a 16-bit context -4'b0001 is
//     16'hffff, not the 4-bit two's complement 4'b1101 zero-extended),
//   - digits that do not match the declared base.
std::optional<std::string> canonicalizeBasedNumber(
    const naja::verilog::Number& number) {
  if (number.value_.index() != naja::verilog::Number::Type::BASED) {
    return std::nullopt;
  }
  const auto& based =
      std::get<naja::verilog::Number::Type::BASED>(number.value_);
  if (not based.hasSize_ or not number.sign_) {
    return std::nullopt;
  }
  const size_t width = based.size_;
  if (width == 0) {
    return std::nullopt;  // LCOV_EXCL_LINE
  }
  std::string digits;
  for (char c : based.digits_) {
    if (c != '_') {
      digits.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
  }
  if (digits.empty()) {
    return std::nullopt;  // LCOV_EXCL_LINE
  }
  std::string bits;  // LSB first
  switch (based.base_) {
    case naja::verilog::BasedNumber::BINARY:
      for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (*it != '0' && *it != '1' && *it != 'x' && *it != 'z') {
          return std::nullopt;  // LCOV_EXCL_LINE
        }
        bits.push_back(*it);
      }
      break;
    case naja::verilog::BasedNumber::OCTAL:
    case naja::verilog::BasedNumber::HEX: {
      const size_t bitsPerDigit =
          based.base_ == naja::verilog::BasedNumber::HEX ? 4 : 3;
      for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        const char digit = *it;
        if (digit == 'x' || digit == 'z') {
          bits.append(bitsPerDigit, digit);
          continue;
        }
        int digitValue = -1;
        if (digit >= '0' && digit <= '9') {
          digitValue = digit - '0';
        } else if (bitsPerDigit == 4 && digit >= 'a' && digit <= 'f') {
          digitValue = 10 + digit - 'a';
        }
        if (digitValue < 0 || digitValue >= (1 << bitsPerDigit)) {
          return std::nullopt;  // LCOV_EXCL_LINE
        }
        for (size_t i = 0; i < bitsPerDigit; ++i) {
          bits.push_back((digitValue >> i) & 1 ? '1' : '0');
        }
      }
      break;
    }
    case naja::verilog::BasedNumber::DECIMAL: {
      if (digits == "x" || digits == "z") {
        // A lone x/z digit marks every bit as unknown.
        bits.push_back(digits.front());
        break;
      }
      for (char c : digits) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
          return std::nullopt;  // LCOV_EXCL_LINE
        }
      }
      // Only the least-significant width bits survive the sizing below.
      std::string remaining = digits;
      for (size_t i = 0; i < width && remaining != "0"; ++i) {
        int carry = 0;
        std::string quotient;
        for (char c : remaining) {
          const int current = carry * 10 + (c - '0');
          carry = current % 2;
          if (!quotient.empty() || current >= 2) {
            quotient.push_back(static_cast<char>('0' + current / 2));
          }
        }
        bits.push_back(carry ? '1' : '0');
        remaining = quotient.empty() ? "0" : quotient;
      }
      break;
    }
  }
  // Sized literals are padded on the left to their declared width with '0',
  // or with x/z when the leftmost digit is x/z, and extra leftmost bits are
  // truncated (IEEE 1364 sized-number rules). The signed marker only governs
  // later expression-context extension, so it does not change the literal's
  // own bit pattern and is preserved in the canonical form instead.
  const char pad =
      digits.front() == 'x' || digits.front() == 'z' ? digits.front() : '0';
  if (bits.size() < width) {
    bits.append(width - bits.size(), pad);
  } else if (bits.size() > width) {
    bits.resize(width);
  }
  return naja::NL::NLDB0::formatBitVectorValue(
      width, std::string(bits.rbegin(), bits.rend()), based.signed_);
}

std::optional<std::string> canonicalizeParameterExpression(
    const naja::verilog::Expression& expression) {
  if (!expression.valid_ ||
      expression.value_.index() != naja::verilog::Expression::Type::NUMBER) {
    return std::nullopt;
  }
  return canonicalizeBasedNumber(
      std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_));
}

// Number::getString() drops the sign of a negated number, so a negated value
// kept in source form gets its '-' back here; storing "4'b1" for -4'b1 would
// silently change the value.
std::string parameterExpressionText(
    const naja::verilog::Expression& expression) {
  std::string text = expression.getString();
  // The expression grammar has no unary minus, so a NUMBER here is always
  // non-negative today; keep the sign restoration symmetric with the
  // constant-expression path in case the grammar grows one.
  if (expression.valid_ &&
      expression.value_.index() == naja::verilog::Expression::Type::NUMBER &&
      not std::get<naja::verilog::Expression::Type::NUMBER>(
          expression.value_).sign_) {
    text.insert(text.begin(), '-');
  }
  return text;
}

std::string parameterConstantExpressionText(
    const naja::verilog::ConstantExpression& expression) {
  std::string text = expression.getString();
  if (expression.valid_ &&
      expression.getType() == naja::verilog::ConstantExpression::Type::NUMBER &&
      not std::get<naja::verilog::ConstantExpression::Type::NUMBER>(
          expression.value_).sign_) {
    text.insert(text.begin(), '-');
  }
  return text;
}

void createPort(
  naja::NL::SNLDesign* design,
  const naja::verilog::Port& port,
  const naja::NL::SNLVRLConstructor::Attributes& attributes) {
  //spdlog::trace("Module {} create port: {}", design->getDescription(), port.getString());
  naja::NL::SNLTerm* term = nullptr;
  if (port.isBus()) {
    term = naja::NL::SNLBusTerm::create(
      design,
      naja::NL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      port.range_.msb_,
      port.range_.lsb_,
      naja::NL::NLName(port.identifier_.name_));
  } else {
    term = naja::NL::SNLScalarTerm::create(
      design,
      naja::NL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      naja::NL::NLName(port.identifier_.name_));
  }
  collectAttributes(term, attributes);
}

void createPortNet(naja::NL::SNLDesign* design, const naja::verilog::Port& port) {
  //spdlog::trace("Module {} create port net: {}", design->getDescription(), port.getString());
  if (port.isBus()) {
    auto term = design->getBusTerm(naja::NL::NLName(port.identifier_.name_));
    auto net = naja::NL::SNLBusNet::create(
      design,
      port.range_.msb_,
      port.range_.lsb_,
      naja::NL::NLName(port.identifier_.name_));
    term->setNet(net);
  } else {
    auto term = design->getScalarTerm(naja::NL::NLName(port.identifier_.name_));
    assert(term);
    auto net = naja::NL::SNLScalarNet::create(design,
      naja::NL::NLName(port.identifier_.name_));
    term->setNet(net);
  }
}


bool allNetsArePortNets(naja::NL::SNLDesign* design) {
  for (auto net: design->getBitNets()) {
    if (net->isAssignConstant()) {
      continue;
    }
    if (net->getBitTerms().empty()) {
      //LCOV_EXCL_START
      //spdlog::info("Net {} in {} is not a port net", 
        //net->getName().getString(),
        //design->getName().getString());
      return false;
      //LCOV_EXCL_STOP
    }
  }
  return true;
}

naja::NL::NLDB0::GateType najaVerilogToNLDB0GateType(const naja::verilog::GateType& gateType) {
  switch (gateType) {
    case naja::verilog::GateType::And:
      return naja::NL::NLDB0::GateType::And;
    case naja::verilog::GateType::Nand:
      return naja::NL::NLDB0::GateType::Nand;
    case naja::verilog::GateType::Or:
      return naja::NL::NLDB0::GateType::Or;
    case naja::verilog::GateType::Nor:
      return naja::NL::NLDB0::GateType::Nor;
    case naja::verilog::GateType::Xor:
      return naja::NL::NLDB0::GateType::Xor;
    case naja::verilog::GateType::Xnor:
      return naja::NL::NLDB0::GateType::Xnor;
    case naja::verilog::GateType::Buf:
      return naja::NL::NLDB0::GateType::Buf;
    case naja::verilog::GateType::Not:
      return naja::NL::NLDB0::GateType::Not;
    //LCOV_EXCL_START
    default: {
      std::ostringstream reason;
      reason << "Unsupported verilog gate type: ";
      reason << gateType.getString();
      throw naja::NL::SNLVRLConstructorException(reason.str());
    }
    //LCOV_EXCL_STOP
  }
  return naja::NL::NLDB0::GateType::Unknown; //LCOV_EXCL_LINE
}

naja::NL::NLLibrary* getOrCreateAutoBlackBoxLibrary(naja::NL::NLLibrary* library) {
  const std::string AUTO_BLACKBOX = "AUTO_BLACKBOX";
  auto autoBlackBoxLibrary = library->getLibrary(naja::NL::NLName(AUTO_BLACKBOX));
  if (not autoBlackBoxLibrary) {
    autoBlackBoxLibrary = naja::NL::NLLibrary::create(library, naja::NL::NLName(AUTO_BLACKBOX));
  }
  return autoBlackBoxLibrary;
}

naja::NL::SNLDesign* getOrCreateAutoBlackBox(
  naja::NL::NLLibrary* autoBlackBoxLibrary,
  const naja::NL::NLName& modelName) {
  auto model = autoBlackBoxLibrary->getSNLDesign(modelName);
  if (not model) {
    model = naja::NL::SNLDesign::create(
      autoBlackBoxLibrary,
      naja::NL::SNLDesign::Type::AutoBlackBox,
      modelName
    );
  }
  return model;
}



} //end anonymous namespace

namespace naja::NL {

//4.5 Implicit declarations
//- If an identifier is used in the terminal list of a primitive instance or a module instance, and that
//  identifier has not been declared previously in the scope where the instantiation appears or in any
//  scope whose declarations can be directly referenced from the scope where the instantiation appears
//  (see 12.7), then an implicit scalar net of default net type shall be assumed.
//— If an identifier appears on the left-hand side of a continuous assignment statement, and that identifier
//  has not been declared previously in the scope where the continuous assignment statement appears or
//  in any scope whose declarations can be directly referenced from the scope where the continuous
//  assignment statement appears (see 12.7), then an implicit scalar net of default net type shall be
//  assumed. See 6.1.2 for a discussion of continuous assignment statements.
naja::NL::SNLNet* SNLVRLConstructor::getNetOrCreateImplicitNet(
  naja::NL::SNLDesign* design,
  const naja::verilog::RangeIdentifier& identifier) {
  naja::NL::NLName netName(identifier.identifier_.name_);
  auto net = design->getNet(netName);
  if (not net) {
    if (identifier.range_.valid_) {
      std::ostringstream reason;
      reason << getLocationString()
        << ": bus net \"" << netName.getString()
        << "\" cannot be found in "
        << design->getName().getString();
      throw SNLVRLConstructorException(reason.str());
    } else {
      net = naja::NL::SNLScalarNet::create(design, netName);
    }
  }
  return net;
}

void SNLVRLConstructor::GateInstance::reset() {
  gateType_ = NLDB0::GateType::Unknown;
  instanceName_ = std::string();
  connections_.clear();
}

bool SNLVRLConstructor::GateInstance::isValid() const {
  return gateType_ != NLDB0::GateType::Unknown;
}

//LCOV_EXCL_START
std::string SNLVRLConstructor::GateInstance::getString() const {
  std::ostringstream stream;
  stream << "GateInstance: " << gateType_.getString();
  if (not instanceName_.empty()) {
    stream  << " " << instanceName_;
  } else {
    stream << " <anonymous>";
  }
  return stream.str();
}
//LCOV_EXCL_STOP

SNLTerm::Direction
SNLVRLConstructor::VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction) {
  switch (direction) {
    case naja::verilog::Port::Direction::Input:
      return naja::NL::SNLTerm::Direction::Input;
    case naja::verilog::Port::Direction::Output:
      return naja::NL::SNLTerm::Direction::Output;
    case naja::verilog::Port::Direction::InOut:
      return naja::NL::SNLTerm::Direction::InOut;
    case naja::verilog::Port::Direction::Unknown: {
      std::ostringstream reason;
      reason << "Unsupported verilog direction";
      throw naja::NL::SNLVRLConstructorException(reason.str());
    }
  }
  return naja::NL::SNLTerm::Direction::Input; //LCOV_EXCL_LINE
}

SNLNet::Type
SNLVRLConstructor::VRLTypeToSNLType(const naja::verilog::Net::Type& type) {
  switch(type) {
    case naja::verilog::Net::Type::Wire:
      return naja::NL::SNLNet::Type::Standard;
    case naja::verilog::Net::Type::Supply0:
      return naja::NL::SNLNet::Type::Supply0;
    case naja::verilog::Net::Type::Supply1:
      return naja::NL::SNLNet::Type::Supply1;
    case naja::verilog::Net::Type::Unknown: {
      std::ostringstream reason;
      reason << "Unsupported verilog net type";
      throw naja::NL::SNLVRLConstructorException(reason.str());
    }
  }
  return naja::NL::SNLNet::Type::Standard; //LCOV_EXCL_LINE
}

void SNLVRLConstructor::createCurrentModuleAssignNets() {
  currentModuleAssign0_ = naja::NL::SNLScalarNet::create(currentModule_);
  currentModuleAssign0_->setType(naja::NL::SNLNet::Type::Assign0);
  currentModuleAssign1_ = naja::NL::SNLScalarNet::create(currentModule_);
  currentModuleAssign1_->setType(naja::NL::SNLNet::Type::Assign1);
} 

void SNLVRLConstructor::createConstantNets(
  const naja::verilog::Number& number,
  naja::NL::SNLInstance::Nets& nets) {
  if (number.value_.index() != naja::verilog::Number::BASED) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": Only base numbers are supported"; 
    throw naja::NL::SNLVRLConstructorException(reason.str());
  }
  auto basedNumber = std::get<naja::verilog::Number::BASED>(number.value_);
  auto bits = SNLVRLConstructorUtils::numberToNetTypes(basedNumber);
  //LCOV_EXCL_START
  if (bits.size() != basedNumber.size_) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": " << "Size";
    throw naja::NL::SNLVRLConstructorException(reason.str());
  }
  //LCOV_EXCL_STOP
  for (int i=bits.size()-1; i>=0; i--) {
    switch (bits[i]) {
      case SNLNet::Type::Assign0: nets.push_back(currentModuleAssign0_); break;
      case SNLNet::Type::Assign1: nets.push_back(currentModuleAssign1_); break;
      case SNLNet::Type::AssignX:
        if (!currentModuleAssignX_) {
          currentModuleAssignX_ = SNLScalarNet::create(currentModule_);
          currentModuleAssignX_->setType(SNLNet::Type::AssignX);
        }
        nets.push_back(currentModuleAssignX_);
        break;
      case SNLNet::Type::AssignZ:
        if (!currentModuleAssignZ_) {
          currentModuleAssignZ_ = SNLScalarNet::create(currentModule_);
          currentModuleAssignZ_->setType(SNLNet::Type::AssignZ);
        }
        nets.push_back(currentModuleAssignZ_);
        break;
      // numberToNetTypes returns only the four assign-constant types handled above.
      default: throw SNLVRLConstructorException("Invalid constant bit type"); // LCOV_EXCL_LINE
    }
  }
}

std::string SNLVRLConstructor::getLocationString() const {
  std::ostringstream stream;
  auto location = getCurrentLocation();
  stream << "In "
    << location.currentPath_.string()
    << " at line " << location.line_
    << ", column " << location.column_;
  return stream.str();
}

SNLVRLConstructor::ModuleDefKey SNLVRLConstructor::getCurrentModuleDefKey() const {
  ModuleDefKey key;
  auto location = getCurrentLocation();
  key.path_ = location.currentPath_;
  key.line_ = location.line_;
  key.column_ = location.column_;
  return key;
}

bool SNLVRLConstructor::isCurrentModuleSelected(const NLName& moduleName) const {
  if (config_.conflictingDesignNamePolicy_ == Config::ConflictingDesignNamePolicy::Forbid
      or config_.conflictingDesignNamePolicy_ == Config::ConflictingDesignNamePolicy::VerifyEquality) {
    return true;
  }
  auto it = selectedModuleDefs_.find(moduleName.getString());
  if (it == selectedModuleDefs_.end()) {
    //LCOV_EXCL_START
    std::ostringstream reason;
    reason << getLocationString()
      << ": no selected module definition found for "
      << moduleName.getString()
      << " (inconsistent pass state)";
    throw SNLVRLConstructorException(reason.str());
    //LCOV_EXCL_STOP
  }
  return it->second == getCurrentModuleDefKey();
}

void SNLVRLConstructor::resetPerModuleState() {
  portSources_.clear();
  declarationAttributes_.clear();
  attributesRange_ = {};
  instantiationAttributes_.clear();
  currentModule_ = nullptr;
  currentModuleInterfacePortsMap_.clear();
  currentModuleInterfacePorts_.clear();
  currentModelName_.clear();
  currentGateInstance_.reset();
  currentInstance_ = nullptr;
  currentInstanceParameterValues_.clear();
  currentModuleAssign0_ = nullptr;
  currentModuleAssign1_ = nullptr;
  currentModuleAssignX_ = nullptr;
  currentModuleAssignZ_ = nullptr;
  nextObjectAttributes_.clear();
  skipCurrentModule_ = false;
}

SNLVRLConstructor::SNLVRLConstructor(NLLibrary* library):
  library_(library)
{}

void SNLVRLConstructor::construct(const Paths& paths) {
  NajaPerf::Scope scope("Verilog construct");
  setPreprocessEnabled(config_.preprocessEnabled_);
  try {
    progress_.begin("parse");
    progress_.finish();
    progress_.begin("first_pass");
    setFirstPass(true);
    parse(paths);
    progress_.finish();
    progress_.begin("second_pass");
    setFirstPass(false);
    parse(paths);
    progress_.finish();
    resetPerModuleState();
    selectedModuleDefs_.clear();
    progress_.begin("done");
  } catch (const NajaCancelled&) {
    selectedModuleDefs_.clear();
    throw;
  } catch (const SNLVRLConstructorException& e) {
    selectedModuleDefs_.clear();
    throw;
  } catch (const std::exception& e) {
    selectedModuleDefs_.clear();
    std::ostringstream reason;
    reason << "In SNLVRLConstructor construct (" << getCurrentPath() << "): "
      << e.what();
    throw SNLVRLConstructorException(reason.str());
  }
}

void SNLVRLConstructor::construct(const std::filesystem::path& path) {
  construct(Paths{path});
}

void SNLVRLConstructor::restoreDeclarationAttributes() {
  // Grouped declarations (e.g. (* src="..." *) wire a, b; or input a, b;)
  // produce one callback per object with the same source range. Each callback
  // consumes nextObjectAttributes_, so reuse the declaration's attributes for
  // later objects instead of attaching them only to the first one.
  const auto range = getCurrentSourceRange();
  if (nextObjectAttributes_.empty() && range.line == attributesRange_.line &&
      range.column == attributesRange_.column && range.endLine == attributesRange_.endLine &&
      range.endColumn == attributesRange_.endColumn) {
    nextObjectAttributes_ = declarationAttributes_;
  } else {
    attributesRange_ = range;
    declarationAttributes_ = nextObjectAttributes_;
  }
}

void SNLVRLConstructor::populateDeclaration(NLObject* object) {
  // Expanded input has no original-file source map. Never fabricate a span.
  if (isPreprocessEnabled()) { return; }
  auto range = getCurrentSourceRange();
  if (not range.line || not range.column || not range.endLine || not range.endColumn) { return; }
  SNLSourceLoc loc;
  loc.file = NLName(getCurrentPath());
  loc.line = range.line;
  loc.column = range.column;
  loc.endLine = range.endLine;
  loc.endColumn = range.endColumn > 1 ? range.endColumn - 1 : 0;
  if (auto infos = sourceInfos(object)) {
    infos->setSourceDeclaration({loc, NLName("naja-verilog"), NLName("verilog"), NLName()});
  }
}

void SNLVRLConstructor::populateSource(NLObject* object, const Attributes& attributes) {
  populateDeclaration(object);
  if (not config_.yosysSourceAttributes_) { return; }
  for (const auto& attribute: attributes) {
    if (attribute.name_.getString() != "src") { continue; }
    if (not attribute.expression_.valid_ || attribute.expression_.getType() != naja::verilog::ConstantExpression::Type::STRING) {
      NAJA_LOG_WARN("{}: ignoring Yosys src without a string value", getLocationString());
      continue;
    }
    std::istringstream input(attribute.expression_.getString());
    std::string entry;
    while (std::getline(input, entry, '|')) {
      auto loc = parseYosysSource(entry);
      if (not loc) {
        NAJA_LOG_WARN("{}: ignoring malformed or overflowing Yosys src entry '{}'", getLocationString(), entry);
        continue;
      }
      if (auto infos = sourceInfos(object)) {
        infos->addSourceOrigin({*loc, NLName("yosys"), NLName(), NLName()});
      }
    }
    if (attribute.expression_.getString().empty() || attribute.expression_.getString().back() == '|') {
      NAJA_LOG_WARN("{}: ignoring empty Yosys src entry", getLocationString());
    }
  }
}

void SNLVRLConstructor::startModule(const naja::verilog::Identifier& module) {
  progress_.advance(module.name_);
  const NLName moduleName(module.name_);
  const auto defKey = getCurrentModuleDefKey();

  if (inFirstPass()) {
    // Deterministic selection of the retained definition for a given module name.
    auto selIt = selectedModuleDefs_.find(moduleName.getString());
    if (selIt == selectedModuleDefs_.end()) {
      selectedModuleDefs_[moduleName.getString()] = defKey;
      currentModule_ = SNLDesign::create(library_, moduleName);
      skipCurrentModule_ = false;
    } else {
      switch (config_.conflictingDesignNamePolicy_) {
        case Config::ConflictingDesignNamePolicy::Forbid: {
          std::ostringstream reason;
          reason << getLocationString()
            << ": NLLibrary " << library_->getString()
            << " contains already a SNLDesign named: " << moduleName.getString();
          throw SNLVRLConstructorException(reason.str());
        }
        case Config::ConflictingDesignNamePolicy::FirstOne: {
          NAJA_LOG_WARN(
            "In SNLVRLConstructor first pass, {} module already exists in "
            "library {}, ignoring this definition.",
            module.getString(),
            library_->getDescription());
          skipCurrentModule_ = true;
          currentModule_ = library_->getSNLDesign(moduleName);
          break;
        }
        case Config::ConflictingDesignNamePolicy::LastOne: {
          NAJA_LOG_WARN(
            "In SNLVRLConstructor first pass, {} module already exists in "
            "library {}, overriding with this definition.",
            module.getString(),
            library_->getDescription());
          auto existingModule = library_->getSNLDesign(moduleName);
          if (existingModule and not existingModule->getSlaveInstances().empty()) {
            //LCOV_EXCL_START
            std::ostringstream reason;
            reason << getLocationString();
            reason << ": cannot override module " << module.getString();
            reason << " because it has instances in other designs.";
            throw SNLVRLConstructorException(reason.str());
            //LCOV_EXCL_STOP
          }
          if (existingModule) {
            existingModule->destroy();
          }
          selectedModuleDefs_[moduleName.getString()] = defKey;
          currentModule_ = SNLDesign::create(library_, moduleName);
          skipCurrentModule_ = false;
          break;
        }
        case Config::ConflictingDesignNamePolicy::VerifyEquality: {
          NAJA_LOG_WARN(
            "In SNLVRLConstructor first pass, {} module already exists in "
            "library {}, verifying equality (not implemented), skipping.",
            module.getString(),
            library_->getDescription());
          skipCurrentModule_ = true;
          currentModule_ = library_->getSNLDesign(moduleName);
          break;
        }
      }
    }
    if (not skipCurrentModule_) {
      populateSource(currentModule_, nextObjectAttributes_);
      collectAttributes(currentModule_, nextObjectAttributes_);
    }
    NAJA_LOG_TRACE("Construct Module: {}", module.getString());
  } else {
    // In second pass, only elaborate the module definition that was selected in first pass.
    if (not isCurrentModuleSelected(moduleName)) {
      skipCurrentModule_ = true;
      nextObjectAttributes_.clear();
      return;
    }
    skipCurrentModule_ = false;

    currentModule_ = library_->getSNLDesign(moduleName);
    if (not currentModule_) {
      std::ostringstream reason;
      reason << "In SNLVRLConstructor second pass, ";
      reason << module.getString() << " module cannot be found in library: ";
      reason << library_->getDescription();
      throw SNLVRLConstructorException(reason.str());
    }
    if (not currentModule_->getNets().empty()) {
      std::ostringstream reason;
      reason << "In SNLVRLConstructor second pass, ";
      reason << module.getString() << " module should no contain any net";
      throw SNLVRLConstructorException(reason.str());
    }
    NAJA_LOG_TRACE("Second pass Construct Module: {}", currentModule_->getDescription());
    createCurrentModuleAssignNets();
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::moduleInterfaceSimplePort(const naja::verilog::Identifier& port) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (inFirstPass()) {
    NAJA_LOG_TRACE("Add port: {}", port.getString());
    if (currentModuleInterfacePortsMap_.find(port.name_) != currentModuleInterfacePortsMap_.end()) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": port collision in module " << currentModule_->getName().getString();
      reason << ", " << port.getString() << " has already been declared.";
      throw SNLVRLConstructorException(reason.str());
    }
    auto index = currentModuleInterfacePortsMap_.size();
    currentModuleInterfacePorts_.push_back(nullptr);
    currentModuleInterfacePortsMap_[port.name_] = index;
  }
}

void SNLVRLConstructor::moduleImplementationPort(const naja::verilog::Port& port) {
  restoreDeclarationAttributes();
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (inFirstPass()) {
    NAJA_LOG_TRACE("Add implementation port: {}", port.getString());
    auto it = currentModuleInterfacePortsMap_.find(port.identifier_.name_);
    if (it == currentModuleInterfacePortsMap_.end()) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": undeclared port in module " << currentModule_->getName().getString();
      reason << ", " << port.getString() << " is unknown in module interface.";
      throw SNLVRLConstructorException(reason.str());
    }
    auto index = it->second;
    currentModuleInterfacePorts_[index] = std::make_unique<naja::verilog::Port>(port);
    portSources_[port.identifier_.name_] = {getCurrentSourceRange(), nextObjectAttributes_};
  } else {
    createPortNet(currentModule_, port);
    auto net = currentModule_->getNet(NLName(port.identifier_.name_));
    populateSource(net, nextObjectAttributes_);
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::moduleInterfaceCompletePort(const naja::verilog::Port& port) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (inFirstPass()) {
    createPort(currentModule_, port, {});
    auto term = currentModule_->getTerm(NLName(port.identifier_.name_));
    populateSource(term, nextObjectAttributes_);
    collectAttributes(term, nextObjectAttributes_);
  } else {
    createPortNet(currentModule_, port);
    auto net = currentModule_->getNet(NLName(port.identifier_.name_));
    populateSource(net, nextObjectAttributes_);
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::addNet(const naja::verilog::Net& net) {
  if (progress_) progress_.checkpoint(
    currentModule_ ? std::string_view(currentModule_->getName().getString()) : std::string_view{});
  restoreDeclarationAttributes();
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (not inFirstPass()) {
    NAJA_LOG_TRACE("Add net: {}", net.getString());
    auto netName = NLName(net.identifier_.name_);
    auto existingNet = currentModule_->getNet(netName);
    if (existingNet) {
      //net might be existing because of a port declaration
      auto term = currentModule_->getTerm(netName);
      if (term) {
        populateSource(existingNet, nextObjectAttributes_);
        collectAttributes(existingNet, nextObjectAttributes_);
        nextObjectAttributes_.clear();
        return;
      } else {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": wire collision for net " << net.identifier_.name_;
        throw SNLVRLConstructorException(reason.str());
      }
    }
    try {
      SNLNet* snlNet = nullptr;
      if (net.isBus()) {
        snlNet = SNLBusNet::create(currentModule_, net.range_.msb_, net.range_.lsb_, NLName(net.identifier_.name_));
      } else {
        snlNet = SNLScalarNet::create(currentModule_, NLName(net.identifier_.name_));
      }
      snlNet->setType(VRLTypeToSNLType(net.type_));
      populateSource(snlNet, nextObjectAttributes_);
      collectAttributes(snlNet, nextObjectAttributes_);
    //LCOV_EXCL_START
    } catch (const NLException& exception) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << exception.getReason();
      throw SNLVRLConstructorException(reason.str());
    }
    //LCOV_EXCL_STOP
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::addAssign(
  const naja::verilog::RangeIdentifiers& identifiers,
  const naja::verilog::Expression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (inFirstPass()) {
    nextObjectAttributes_.clear();
    return;
  }
  using BitNets = std::vector<SNLBitNet*>;
  BitNets leftNets;
  BitNets rightNets;
  NAJA_LOG_TRACE("Assign:");
  for (auto id: identifiers) {
    collectIdentifierNets(id, leftNets);
    NAJA_LOG_TRACE("ID: {}", id.getString());
  }
  NAJA_LOG_TRACE("{}", expression.getString());
  if (expression.valid_) {
    if (not expression.supported_) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << expression.getString() << " is not currently supported";
      throw SNLVRLConstructorException(reason.str());
    }
    switch (expression.value_.index()) {
      case naja::verilog::Expression::Type::NUMBER: {
        auto number =
          std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_);
        createConstantNets(number, rightNets);
        break;
      }
      case naja::verilog::Expression::Type::RANGEIDENTIFIER: {
        auto identifier = std::get<naja::verilog::RangeIdentifier>(expression.value_);
        std::string name = identifier.identifier_.name_;
        auto net = getNetOrCreateImplicitNet(currentModule_, identifier);
        if (identifier.range_.valid_) {
          SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
          if (not busNet) {
            std::ostringstream reason;
            reason << getLocationString()
              << ": net " << name << " is not a bus";
            throw SNLVRLConstructorException(reason.str());
          }
          int netMSB = identifier.range_.msb_;
          int netLSB = netMSB;
          if (not identifier.range_.singleValue_) {
            netLSB = identifier.range_.lsb_;
          }
          busNet->insertBits(rightNets, rightNets.begin(), netMSB, netLSB);
        } else {
          rightNets.insert(rightNets.end(),
            net->getBits().begin(), net->getBits().end());
        }
        break;
      }
      case naja::verilog::Expression::Type::CONCATENATION: {
        SNLInstance::Nets bitNets;
        auto concatenation = std::get<std::shared_ptr<naja::verilog::Concatenation>>(expression.value_);
        collectConcatenationBitNets(*concatenation, rightNets);
        break;
      }
      default: {
        std::ostringstream reason;
        reason << expression.getString() << " is not supported in instance connection";
        throw SNLVRLConstructorException(reason.str());
      }
    }
  }
  assert(leftNets.size() == rightNets.size());
  for (size_t i=0; i<leftNets.size(); ++i) {
    auto leftNet = leftNets[i];
    auto rightNet = rightNets[i];
    auto assign = SNLInstance::create(currentModule_, NLDB0::getAssign());
    assign->setTermNet(NLDB0::getAssignOutput(), leftNet);
    assign->setTermNet(NLDB0::getAssignInput(), rightNet);
    populateSource(assign, nextObjectAttributes_);
    collectAttributes(assign, nextObjectAttributes_);
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::startInstantiation(const naja::verilog::Identifier& model) {
  if (skipCurrentModule_) { return; }
  if (not inFirstPass()) {
    instantiationAttributes_ = nextObjectAttributes_;
    currentModelName_ = model.name_;
    NAJA_LOG_TRACE("Start Instantiation: {}", model.getString());
  }
}

void SNLVRLConstructor::addInstance(const naja::verilog::Identifier& instance) {
  if (progress_) progress_.checkpoint(
    currentModule_ ? std::string_view(currentModule_->getName().getString()) : std::string_view{});
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (not inFirstPass()) {
    assert(not currentModelName_.empty());
    NLName modelName(currentModelName_);
    SNLDesign* model = library_->getSNLDesign(modelName);
    if (not model) {
      model = library_->getDB()->getSNLDesign(modelName);
    }
    if (not model) {
      model = NLUniverse::get()->getSNLDesign(modelName);
    }
    if (not model) {
      if (config_.blackboxUnknownModules_) {
        NAJA_LOG_TRACE("Unknown design: {}", modelName.getString());
        auto autoBlackBoxLibrary = getOrCreateAutoBlackBoxLibrary(library_);
        model = getOrCreateAutoBlackBox(autoBlackBoxLibrary, modelName);
      } else {
        std::ostringstream reason;
        reason << getLocationString()
          << ": " << currentModelName_
          << " cannot be found in SNL while constructing instance "
          << instance.getString();
        throw SNLVRLConstructorException(reason.str());
      }
    }
    //FIXME
    //might be a good idea to create a cache <Name, SNLDesign*> here
    //in particular for primitives
    currentInstance_ = SNLInstance::create(currentModule_, model, NLName(instance.name_));
    NAJA_LOG_TRACE("Current Instance: {}", currentInstance_->getDescription());
    populateSource(currentInstance_, instantiationAttributes_);
    collectAttributes(currentInstance_, instantiationAttributes_);
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::addParameterAssignment(
  const naja::verilog::Identifier& parameter,
  const naja::verilog::Expression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (not inFirstPass()) {
    auto& stored = currentInstanceParameterValues_[parameter.name_];
    stored.source = parameterExpressionText(expression);
    stored.canonical = canonicalizeParameterExpression(expression);
  }
}

void SNLVRLConstructor::endInstantiation() {
  if (skipCurrentModule_) { return; }
  if (not inFirstPass()) {
    assert(currentInstance_);
    NAJA_LOG_TRACE("End {} instantiation", currentInstance_->getString());
    //Save current Parameter assignments
    for (auto parameterValue: currentInstanceParameterValues_) {
      auto parameterName = NLName(parameterValue.first);
      auto parameter = currentInstance_->getModel()->getParameter(parameterName);
      if (not parameter) {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": " << currentModelName_
          << " does not contain any Parameter named " << parameterValue.first;
        throw SNLVRLConstructorException(reason.str());
      }
      SNLInstParameter::create(
          currentInstance_,
          parameter,
          // Canonicalize only bit-typed parameters; numeric parameters keep
          // the source form for consumers that read them as numbers.
          (parameter->getType() == naja::NL::SNLParameter::Type::Binary &&
           parameterValue.second.canonical.has_value())
              ? *parameterValue.second.canonical
              : parameterValue.second.source);
    }
    currentInstanceParameterValues_.clear();
    currentInstance_ = nullptr;
    currentModelName_ = std::string();
  }
}

void SNLVRLConstructor::currentInstancePortConnection(
  naja::NL::SNLTerm* term,
  const naja::verilog::Expression& expression) {
  if (expression.valid_) {
    if (term == nullptr) {
      std::ostringstream reason;
      reason << getLocationString() << ": null term in instance connection";
      throw SNLVRLConstructorException(reason.str());
    }
    if (dynamic_cast<SNLBundleTerm*>(term)) {
      std::ostringstream reason;
      reason << getLocationString() << ": direct connection to bundle term "
             << term->getString() << " is not supported";
      throw SNLVRLConstructorException(reason.str());
    }
    if (not expression.supported_) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << expression.getString() << " is not currently supported";
      throw SNLVRLConstructorException(reason.str());
    }
    switch (expression.value_.index()) {
      case naja::verilog::Expression::NUMBER: {
        auto number =
          std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_);
        SNLInstance::Terms bitTerms;
        if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
          bitTerms.push_back(scalarTerm);
        } else {
          auto busTerm = static_cast<SNLBusTerm*>(term);
          bitTerms = SNLInstance::Terms(busTerm->getBits().begin(), busTerm->getBits().end());
        }
        SNLInstance::Nets bitNets;
        createConstantNets(number, bitNets);
        currentInstance_->setTermsNets(bitTerms, bitNets);
        break;
      }
      case naja::verilog::Expression::Type::RANGEIDENTIFIER: {
        auto identifier = std::get<naja::verilog::RangeIdentifier>(expression.value_);
        std::string name = identifier.identifier_.name_;
        auto net = getNetOrCreateImplicitNet(currentInstance_->getDesign(), identifier);
        if (identifier.range_.valid_) {
          SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
          if (not busNet) {
            std::ostringstream reason;
            reason << getLocationString()
              << ": net \"" << name << "\" is not a bus net (expected SNLBusNet) for range access";
            throw SNLVRLConstructorException(reason.str());
          }
          int netMSB = identifier.range_.msb_;
          int netLSB = netMSB;
          if (not identifier.range_.singleValue_) {
            netLSB = identifier.range_.lsb_;
          }
          currentInstance_->setTermNet(term, net, netMSB, netLSB);
        } else {
          currentInstance_->setTermNet(term, net);
        }
        break;
      }
      case naja::verilog::Expression::Type::CONCATENATION: {
        SNLInstance::Nets bitNets;
        auto concatenation = std::get<std::shared_ptr<naja::verilog::Concatenation>>(expression.value_);
        collectConcatenationBitNets(*concatenation, bitNets);
        using BitTerms = std::vector<SNLBitTerm*>;
        auto busTerm = dynamic_cast<SNLBusTerm*>(term);
        if (not busTerm) {
          std::ostringstream reason;
            reason << getLocationString()
               << ": term \"" << (term ? term->getString() : "<null>")
               << "\" is not a bus term (expected SNLBusTerm) for concatenation connection";
          throw SNLVRLConstructorException(reason.str());
        }
        BitTerms bitTerms(busTerm->getBits().begin(), busTerm->getBits().end());
        if (bitTerms.size() != bitNets.size()) {
          std::ostringstream reason;
          reason << getLocationString();
          reason << ": " << term->getString() << " and " << expression.getString();
          reason << " do not have the same number of bits";
          throw SNLVRLConstructorException(reason.str());
        }
        currentInstance_->setTermsNets(bitTerms, bitNets);
        break;
      }
      default: {
        std::ostringstream reason;
        reason << expression.getString() << " is not supported in instance connection";
        throw SNLVRLConstructorException(reason.str());
      }
    }
  }
}

void SNLVRLConstructor::addInstanceConnection(
  const naja::verilog::Identifier& port,
  const naja::verilog::Expression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (not inFirstPass()) {
    NAJA_LOG_TRACE("addInstanceConnection port: {} expresion: {}",
      port.getDescription(), expression.getDescription());
    assert(currentInstance_);
    SNLDesign* model = currentInstance_->getModel();
    SNLTerm* term = model->getTerm(NLName(port.name_));
    if (not term) {
      if (model->isAutoBlackBox()) {
        if (not expression.valid_ or expression.getSize() <= 1) {
          term = SNLScalarTerm::create(
            model,
            SNLTerm::Direction::Undefined,
            NLName(port.name_));
        } else {
          term = SNLBusTerm::create(
            model,
            SNLTerm::Direction::Undefined,
            expression.getSize()-1,
            0,
            NLName(port.name_));
        }
      } else {
        std::ostringstream reason;
        reason << getLocationString()
          << ": " << port.getString()
          << " port cannot be found in " << model->getName().getString()
          << " model";
        throw SNLVRLConstructorException(reason.str());
      }
    }
    currentInstancePortConnection(term, expression);
    NAJA_LOG_TRACE("Instance connection: {}  - connection",
      currentInstance_->getString(),  term->getString());
  }
}

void SNLVRLConstructor::addOrderedInstanceConnection(
  size_t portIndex,
  const naja::verilog::Expression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (not inFirstPass()) {
    assert(currentInstance_);
    SNLDesign* model = currentInstance_->getModel();
    auto connectableTerms = getConnectableTerms(model);
    if (portIndex >= connectableTerms.size()) {
      std::ostringstream reason;
      reason << getLocationString()
        << ": ordered port index " << portIndex
        << " exceeds model interface size for " << model->getName().getString();
      throw SNLVRLConstructorException(reason.str());
    }
    SNLTerm* term = connectableTerms[portIndex];
    currentInstancePortConnection(term, expression);
    NAJA_LOG_TRACE("Instance connection: {}  - connection",
      currentInstance_->getString(),  term->getString());
  }
}

void SNLVRLConstructor::startGateInstantiation(const naja::verilog::GateType& gateType) {
  if (skipCurrentModule_) { return; }
  if (not inFirstPass()) {
    instantiationAttributes_ = nextObjectAttributes_;
    gateSourceRange_ = getCurrentSourceRange();
    currentGateInstance_.gateType_ = najaVerilogToNLDB0GateType(gateType);
    NAJA_LOG_TRACE("Start Instantiation: {}", gateType.getString());
  }
}

void SNLVRLConstructor::addGateInstance(const naja::verilog::Identifier& id) {
  if (skipCurrentModule_) { return; }
  if (not id.empty()) { gateSourceRange_ = getCurrentSourceRange(); }
  if (not inFirstPass() and not id.empty()) {
    currentGateInstance_.instanceName_ = id.name_;
  }
}

void SNLVRLConstructor::addGateOutputInstanceConnection(
  size_t portIndex,
  const naja::verilog::RangeIdentifiers identifiers) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (inFirstPass()) {
    return;
  }
  if (identifiers.size() != 1) {
    //LCOV_EXCL_START
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": " << naja::verilog::getRangeIdentifiersString(identifiers) << " is not supported";
    throw SNLVRLConstructorException(reason.str());
    //LCOV_EXCL_STOP
  }
  auto identifier = identifiers[0];
  naja::verilog::Expression expression;
  expression.value_ = identifier;
  expression.valid_ = true;
  expression.supported_ = true;
  currentGateInstance_.connections_.push_back(expression);
}

void SNLVRLConstructor::addGateInputInstanceConnection(
  size_t portIndex,
  const naja::verilog::Expression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (inFirstPass()) {
    return;
  }
  currentGateInstance_.connections_.push_back(expression);
}

void SNLVRLConstructor::endGateInstantiation() {
  if (progress_) progress_.checkpoint(
    currentModule_ ? std::string_view(currentModule_->getName().getString()) : std::string_view{});
  if (skipCurrentModule_) { return; }
  if (not inFirstPass()) {
    assert(currentGateInstance_.isValid());
    NAJA_LOG_TRACE("End {} instantiation", currentGateInstance_.getString());
    //create gate instance
    SNLDesign* gate = nullptr;
    size_t nbTerms = currentGateInstance_.connections_.size();
    if (currentGateInstance_.gateType_.isNInput()) {
      gate = NLDB0::getOrCreateNInputGate(currentGateInstance_.gateType_, nbTerms-1);
    } else {
      gate = NLDB0::getOrCreateNOutputGate(currentGateInstance_.gateType_, nbTerms-1);
    }
    currentInstance_ = SNLInstance::create(
      currentModule_,
      gate,
      NLName(currentGateInstance_.instanceName_));

    using Terms = std::vector<SNLBitTerm*>;
    Terms terms;
    if (currentGateInstance_.gateType_.isNInput()) {
      //first term is the gate output single term
      //then n-inputs (bus)
      terms.push_back(NLDB0::getGateSingleTerm(gate));
      auto nterm = NLDB0::getGateNTerms(gate);
      terms.insert(terms.end(), nterm->getBits().begin(), nterm->getBits().end());
    } else {
      //first term is the gate n-output (bus)
      //then single input
      auto nterm = NLDB0::getGateNTerms(gate);
      terms.insert(terms.end(), nterm->getBits().begin(), nterm->getBits().end());
      terms.push_back(NLDB0::getGateSingleTerm(gate));
    }

    for (size_t i=0; i<currentGateInstance_.connections_.size(); ++i) {
      currentInstancePortConnection(
        terms[i],
        currentGateInstance_.connections_[i]);
    }
    setCurrentSourceRange(gateSourceRange_.line, gateSourceRange_.column, gateSourceRange_.endLine, gateSourceRange_.endColumn);
    populateSource(currentInstance_, instantiationAttributes_);
    collectAttributes(currentInstance_, instantiationAttributes_);
    currentInstance_ = nullptr;
    currentGateInstance_.reset();
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::addAttribute(
  const naja::verilog::Identifier& attributeName,
  const naja::verilog::ConstantExpression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (getParseAttributes()) {
    nextObjectAttributes_.push_back(naja::verilog::Attribute(attributeName, expression));
  }
}

void SNLVRLConstructor::endModule() {
  if (skipCurrentModule_) {
   resetPerModuleState();
    return;
  }
  NAJA_LOG_TRACE("End module: {}", currentModule_->getString());
  if (inFirstPass()) {
    //construct interface declared ports if existing
    //verify that all ports declared interface
    //have their declaration in implementation part: no nullptr in vector
    auto findIt = find(currentModuleInterfacePorts_.begin(), currentModuleInterfacePorts_.end(), nullptr);
    if (findIt != currentModuleInterfacePorts_.end()) {
      auto position = findIt - currentModuleInterfacePorts_.begin();
      //undeclared port
      //search in all map for same index, very long
      //but this is just to produce message error
      std::string portName;
      for (const auto& [key, value]: currentModuleInterfacePortsMap_) {
        if (value == static_cast<size_t>(position)) {
          portName = key;
        }
      }
      std::ostringstream reason;
      reason << portName << " declared in interface has no declaration in module implementation.";
      throw SNLVRLConstructorException(reason.str());
    }
    for (const auto& port: currentModuleInterfacePorts_) {
      const auto& source = portSources_.at(port->identifier_.name_);
      setCurrentSourceRange(source.first.line, source.first.column, source.first.endLine, source.first.endColumn);
      createPort(currentModule_, *port, {});
      auto term = currentModule_->getTerm(NLName(port->identifier_.name_));
      populateSource(term, source.second);
      collectAttributes(term, source.second);
    }
  } else {
    //Blackbox detection
    if (config_.blackboxDetection_ and currentModule_->getType() == SNLDesign::Type::Standard) {
      if (currentModule_->getInstances().empty()) {
        if (allNetsArePortNets(currentModule_)) {
          currentModule_->setType(SNLDesign::Type::UserBlackBox);
          //spdlog::info("Blackbox detected: {}", currentModule_->getName().getString());
        }
      }
    }
    //Allow unknown designs
    if (config_.blackboxUnknownModules_ and currentModule_->isStandard()) {
      for (auto instance: currentModule_->getInstances()) {
        if (instance->isAutoBlackBox()) {
          //spdlog::info("Auto blackbox detected: {}", instance->getDesign()->getName().getString());
          auto model = instance->getModel();
          for (auto term: model->getTerms()) {
            if (term->getDirection() == SNLTerm::Direction::Undefined) {
              if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
                auto instanceTerm = instance->getInstTerm(scalarTerm);
                if (instanceTerm) {
                  auto net = instanceTerm->getNet();
                  if (net and net->isConstant()) {
                    scalarTerm->setDirection(SNLTerm::Direction::Input);
                  }
                }
              }
              //else if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
              //  busTerm->setDirection(SNLTerm::Direction::InOut);
              //} else {
              //  std::ostringstream reason;
              //  reason << getLocationString();
              //  reason << ": " << term->getString() << " is not a bus or scalar term";
              //  throw SNLVRLConstructorException(reason.str());
              //}
            }
          }
        }
      }
    }
  }
  resetPerModuleState();
}

void SNLVRLConstructor::collectConcatenationBitNets(
  const naja::verilog::Concatenation& concatenation,
  SNLInstance::Nets& bitNets) {
  for (auto expression: concatenation.expressions_) {
    if (not expression.supported_ or not expression.valid_) {
      //LCOV_EXCL_START
      std::ostringstream reason;
      reason << naja::NL::SNLVRLConstructor::getLocationString()
        << ": " << expression.getString() << " is not supported in concatenation";
      throw SNLVRLConstructorException(reason.str());
      //LCOV_EXCL_STOP
    }
    switch (expression.value_.index()) {
      case naja::verilog::Expression::NUMBER: {
        auto number =
          std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_);
        createConstantNets(number, bitNets);
        break;
      }
      case naja::verilog::Expression::RANGEIDENTIFIER: {
        auto identifier =
          std::get<naja::verilog::Expression::Type::RANGEIDENTIFIER>(expression.value_);
        auto net = getNetOrCreateImplicitNet(currentModule_, identifier);
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          if (identifier.range_.valid_) {
            std::ostringstream reason;
            reason << getLocationString() << ": "
              << expression.getString() << ", wrong range connection to a scalar net";
            throw SNLVRLConstructorException(reason.str());
          }
          bitNets.push_back(scalarNet);
        } else {
          auto busNet = static_cast<SNLBusNet*>(net);
          if (not identifier.range_.valid_) {
            bitNets.insert(bitNets.end(), busNet->getBits().begin(), busNet->getBits().end());
          } else {
            int msb = identifier.range_.msb_;
            if (identifier.range_.singleValue_) {
              bitNets.push_back(busNet->getBit(msb));
            } else {
              int lsb = identifier.range_.lsb_;
              int incr = msb<lsb?+1:-1;
              for (int i=msb; i!=lsb+incr; i+=incr) {
                auto bit = busNet->getBit(i);
                bitNets.push_back(bit);
              }
            }
          }
        }
        break;
      }
      default: {
        std::ostringstream reason;
        reason << getLocationString()
          << ": "
          << expression.getString() << " is not supported in concatenation";
        throw SNLVRLConstructorException(reason.str());
      }
    }
  }
}

void SNLVRLConstructor::collectIdentifierNets(
  const naja::verilog::RangeIdentifier& identifier,
  SNLInstance::Nets& bitNets) {
  auto net = getNetOrCreateImplicitNet(currentModule_, identifier);
  if (identifier.range_.valid_) {
    SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
    if (not busNet) {
      std::ostringstream reason;
      reason << getLocationString() << " NOT BUSNET"; 
      throw SNLVRLConstructorException(reason.str());
    }
    int netMSB = identifier.range_.msb_;
    int netLSB = netMSB;
    if (not identifier.range_.singleValue_) {
      netLSB = identifier.range_.lsb_;
    }
    busNet->insertBits(bitNets, bitNets.end(), netMSB, netLSB);
  } else {
    bitNets.insert(bitNets.end(),
      net->getBits().begin(), net->getBits().end());
  }
}

void SNLVRLConstructor::addDefParameterAssignment(
  const naja::verilog::Identifiers& hierarchicalParameter,
  const naja::verilog::ConstantExpression& expression) {
  if (skipCurrentModule_) { nextObjectAttributes_.clear(); return; }
  if (not inFirstPass()) {
    if (hierarchicalParameter.size() != 2) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": Only Hierarchical parameter of size 2 assignment are supported";
      throw SNLVRLConstructorException(reason.str());
    }
    auto instanceName = hierarchicalParameter[0];
    auto parameterName = hierarchicalParameter[1];
    auto instance = currentModule_->getInstance(NLName(instanceName.name_));
    if (not instance) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": instance " << instanceName.getString();
      reason << " cannot be found in " << currentModule_->getName().getString();
      throw SNLVRLConstructorException(reason.str());
    }
    auto parameter = instance->getModel()->getParameter(NLName(parameterName.name_));
    if (not parameter) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": parameter " << parameterName.getString();
      reason << " cannot be found in " << instance->getModel()->getName().getString();
      throw SNLVRLConstructorException(reason.str());
    }
    std::string parameterValue = parameterConstantExpressionText(expression);
    // Same rule as instance overrides: only bit-typed parameters get the
    // canonical sized-binary form.
    if (parameter->getType() == naja::NL::SNLParameter::Type::Binary &&
        expression.getType() == naja::verilog::ConstantExpression::Type::NUMBER) {
      if (auto canonical = canonicalizeBasedNumber(
              std::get<naja::verilog::ConstantExpression::Type::NUMBER>(
                  expression.value_))) {
        parameterValue = std::move(*canonical);
      }
    }
    SNLInstParameter::create(instance, parameter, parameterValue);
  }
}

}  // namespace naja::NL
