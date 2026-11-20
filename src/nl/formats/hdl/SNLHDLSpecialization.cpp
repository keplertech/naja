// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "SNLHDLSpecialization.h"

#include "NLLibrary.h"
#include "NLException.h"
#include "SNLDesign.h"
#include "SNLAttributes.h"
#include "SNLParameter.h"
#include "sha-256.h"

namespace naja::NL {
namespace {
constexpr auto sourceAttribute = "naja.source_module";
constexpr auto signatureAttribute = "naja.elaboration_signature";
std::string digest(const std::string& text) {
  uint8_t bytes[32];
  calc_sha_256(bytes, text.data(), text.size());
  std::string result;
  for (auto byte : bytes) {
    result += "0123456789abcdef"[byte >> 4];
    result += "0123456789abcdef"[byte & 15];
  }
  return result;
}
void append(std::string& result, const std::string& value) {
  result += std::to_string(value.size()) + ":" + value;
}
}

std::string SNLHDLSpecialization::signature(const std::vector<SNLHDLParameter>& parameters, const std::string& context) {
  std::string result = "naja-elaboration-v1:";
  for (const auto& parameter : parameters) {
    append(result, parameter.name);
    append(result, parameter.identity);
  }
  if (!context.empty()) append(result, context);
  return result;
}

std::string SNLHDLSpecialization::attribute(const SNLDesign* design, const std::string& name) {
  for (const auto& attr : SNLAttributes::getAttributes(design))
    if (attr.getName().getString() == name) return attr.getValue().getString();
  return {};
}
bool SNLHDLSpecialization::isElaborated(const SNLDesign* design) {
  return !attribute(design, sourceAttribute).empty();
}

std::string SNLHDLSpecialization::name(NLLibrary* library, const std::string& source,
    const std::vector<SNLHDLParameter>& parameters, const std::set<std::string>& sourceNames,
    const std::string& context) {
  const auto identity = signature(parameters, context);
  auto candidate = source;
  bool defaults = context.empty(), readable = context.empty();
  for (const auto& parameter : parameters) {
    if (parameter.isDefault) continue;
    defaults = false;
    if (!parameter.nameValue) readable = false;
    else {
      const auto safe = [](const std::string& value) {
        return value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") == std::string::npos;
      };
      if (!safe(parameter.name) || !safe(*parameter.nameValue)) readable = false;
      candidate += "__" + parameter.name + "_" + *parameter.nameValue;
    }
  }
  const auto matches = [&](SNLDesign* design) {
    return attribute(design, sourceAttribute) == source &&
        attribute(design, signatureAttribute) == identity;
  };
  const auto available = [&](const std::string& value) {
    auto* existing = library->getSNLDesign(NLName(value));
    return (!existing || matches(existing)) && (value == source || !sourceNames.contains(value));
  };
  if (defaults) {
    if (!available(source)) throw NLException("Conflicting default HDL specialization: " + source);
    return source;
  }
  if (readable && candidate.size() <= 64 && available(candidate)) return candidate;
  const auto hash = digest(identity);
  candidate = source + "__h" + hash.substr(0, 8);
  // Once a prefix has collided, all members of that bucket use 64 bits.
  for (auto* design : library->getSNLDesigns()) {
    if (attribute(design, sourceAttribute) != source) continue;
    const auto previousHash = digest(attribute(design, signatureAttribute));
    if (previousHash.substr(0, 8) == hash.substr(0, 8) &&
        design->getName().getString() == source + "__h" + previousHash.substr(0, 16)) {
      const auto longer = source + "__h" + hash.substr(0, 16);
      if (available(longer)) return longer;
      throw NLException("Conflicting hashed HDL specialization: " + longer);
    }
  }
  if (available(candidate)) return candidate;
  // A genuine truncated-digest collision promotes both specializations. User
  // names never get renamed and cannot be bypassed with a longer digest.
  auto* previous = library->getSNLDesign(NLName(candidate));
  if (previous && attribute(previous, sourceAttribute) == source && !sourceNames.contains(candidate)) {
    const auto previousIdentity = attribute(previous, signatureAttribute);
    const auto previousHash = digest(previousIdentity);
    if (previousHash.substr(0, 8) == hash.substr(0, 8) && previousIdentity != identity) {
      const auto longer = source + "__h" + hash.substr(0, 16);
      const auto previousLonger = source + "__h" + previousHash.substr(0, 16);
      if (longer != previousLonger && available(longer) &&
          !sourceNames.contains(previousLonger) && !library->getSNLDesign(NLName(previousLonger))) {
        previous->setName(NLName(previousLonger));
        return longer;
      }
    }
  }
  // Reuse an already-promoted specialization on a later load.
  const auto longer = source + "__h" + hash.substr(0, 16);
  if (auto* existing = library->getSNLDesign(NLName(longer)); existing && matches(existing)) return longer;
  throw NLException("Conflicting hashed HDL specialization: " + candidate);
}

void SNLHDLSpecialization::annotate(SNLDesign* design, const std::string& source,
    const std::vector<SNLHDLParameter>& parameters, const std::string& context) {
  SNLAttributes::addAttribute(design, SNLAttribute(NLName(sourceAttribute), SNLAttributeValue(source)));
  SNLAttributes::addAttribute(design, SNLAttribute(NLName(signatureAttribute), SNLAttributeValue(signature(parameters, context))));
  for (const auto& parameter : parameters)
    SNLParameter::create(design, NLName(parameter.name), SNLParameter::Type::Decimal, parameter.literal);
}

std::string SNLHDLSpecialization::identifierValue(std::string value) {
  for (auto& c : value)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) c = '_';
  return value;
}
std::string SNLHDLSpecialization::integerValue(std::string value) {
  std::string result;
  for (char c : value) {
    if (c == '+') continue;
    if (c == '-') c = 'n';
    else if (c == '.') c = 'p';
    else if (c == 'E') c = 'e';
    result += c;
  }
  return result;
}
std::string SNLHDLSpecialization::bitValue(const std::string& bits, bool scalar) {
  if (scalar && bits.size() == 1 && (bits[0] == '0' || bits[0] == '1')) return bits;
  if (bits.find_first_not_of("01") != std::string::npos) {
    std::string result = "b" + bits;
    for (auto& c : result) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return result;
  }
  std::string padded((4 - bits.size() % 4) % 4, '0');
  padded += bits;
  std::string result = "h";
  for (size_t i = 0; i < padded.size(); i += 4) {
    unsigned digit = 0;
    for (size_t j = 0; j < 4; ++j) digit = digit * 2 + (padded[i + j] == '1');
    result += "0123456789ABCDEF"[digit];
  }
  return result;
}
}
