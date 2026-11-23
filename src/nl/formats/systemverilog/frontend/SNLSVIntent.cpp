// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSVIntent.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <utility>

#include "NLName.h"
#include "NLException.h"
#include "SNLDesignObject.h"
#include "SNLSVConstructor.h"

#include "slang/ast/Compilation.h"
#include "slang/ast/Scope.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/ParameterSymbols.h"
#include "slang/ast/symbols/PortSymbols.h"
#include "slang/ast/symbols/ValueSymbol.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/AllTypes.h"
#include "slang/ast/types/DeclaredType.h"
#include "slang/ast/types/Type.h"
#include "slang/numeric/ConstantValue.h"
#include "slang/numeric/SVInt.h"
#include "slang/syntax/AllSyntax.h"
#include "slang/syntax/SyntaxNode.h"
#include "slang/text/SourceManager.h"

namespace naja::NL {

namespace {

const SNLSVLiveASTLink* getAvailableLink() {
  const auto* link = SNLSVLiveASTLinkRegistry::getLatest();
  if (!link || !link->getCompilation()) {
    return nullptr;
  }
  return link;
}

SNLSourceLoc sourceLocOf(
  const slang::ast::Compilation& compilation,
  const slang::SourceRange& range) {
  SNLSourceLoc result;
  const auto* sourceManager = compilation.getSourceManager();
  if (!sourceManager || !range.start().valid()) {
    return result;  // LCOV_EXCL_LINE -- retained links always have file-backed ranges
  }

  auto originalRange = sourceManager->getFullyOriginalRange(range);
  auto start = sourceManager->getFullyOriginalLoc(originalRange.start());
  auto end = originalRange.end().valid()
               ? sourceManager->getFullyOriginalLoc(originalRange.end())
               : originalRange.end();  // LCOV_EXCL_LINE -- parsed syntax has a valid end
  if (!start.valid() || !sourceManager->isFileLoc(start)) {
    return result;  // LCOV_EXCL_LINE -- parsed source locations are file locations
  }
  if (!end.valid() || !sourceManager->isFileLoc(end) || end < start) {
    end = start;  // LCOV_EXCL_LINE -- defensive normalization for malformed ranges
  } else if (end > start) {
    end -= 1;
  }

  std::string file(sourceManager->getFileName(start));
  if (file.empty()) {
    file = sourceManager->getRawFileName(start.buffer());  // LCOV_EXCL_LINE
  }
  result.file = NLName(file);
  result.line = sourceManager->getLineNumber(start);
  const auto column = sourceManager->getColumnNumber(start);
  const auto endColumn = sourceManager->getColumnNumber(end);
  if (column > std::numeric_limits<std::uint32_t>::max() ||
      endColumn > std::numeric_limits<std::uint32_t>::max()) {
    // LCOV_EXCL_START Defensive overflow guard: requires a source line larger than 4 GiB.
    throw NLException("SystemVerilog source column exceeds the 32-bit coordinate limit");
    // LCOV_EXCL_STOP
  }
  result.column = static_cast<std::uint32_t>(column);
  result.endLine = sourceManager->getLineNumber(end);
  result.endColumn = static_cast<std::uint32_t>(endColumn);
  return result;
}

SNLSourceLoc sourceLocOf(
  const slang::ast::Compilation& compilation,
  const slang::ast::Symbol& symbol) {
  slang::SourceRange range;
  if (symbol.location.valid()) {
    range = {symbol.location, symbol.location};
  }
  if (auto* syntax = symbol.getSyntax()) {
    auto syntaxRange = syntax->sourceRange();
    if (syntaxRange.start().valid()) {
      range = syntaxRange;
    }
  }
  return sourceLocOf(compilation, range);
}

std::string collapseWhitespace(std::string text) {
  std::string result;
  bool pendingSpace = false;
  for (unsigned char c : text) {
    if (std::isspace(c)) {
      pendingSpace = !result.empty();
      continue;
    }
    if (pendingSpace) {
      result.push_back(' ');
      pendingSpace = false;
    }
    result.push_back(static_cast<char>(c));
  }
  return result;
}

std::string trim(std::string text) {
  auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  auto begin = std::find_if(text.begin(), text.end(), notSpace);
  auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();
  if (begin >= end) {
    return {};  // LCOV_EXCL_LINE -- parameter syntax is never whitespace-only
  }
  return std::string(begin, end);
}

bool endsWith(const std::string& text, const std::string& suffix) {  // LCOV_EXCL_LINE
  return text.size() >= suffix.size() &&  // LCOV_EXCL_LINE
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;  // LCOV_EXCL_LINE
}

std::string stripToRHS(std::string text, const std::string& name) {
  text = trim(collapseWhitespace(std::move(text)));
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '=') {
      continue;
    }
    const char prev = i == 0 ? '\0' : text[i - 1];
    const char next = i + 1 < text.size() ? text[i + 1] : '\0';
    if (prev == '=' || prev == '<' || prev == '>' || prev == '!' || next == '=') {
      continue;  // LCOV_EXCL_LINE -- declaration assignment precedes initializer operators
    }
    const auto lhs = trim(text.substr(0, i));
    if (lhs == name || endsWith(lhs, " " + name) || endsWith(lhs, name)) {
      auto rhs = trim(text.substr(i + 1));
      while (!rhs.empty() && rhs.back() == ';') {
        rhs.pop_back();  // LCOV_EXCL_LINE -- syntax nodes omit the semicolon
        rhs = trim(std::move(rhs));  // LCOV_EXCL_LINE
      }
      return rhs;
    }
    return text;  // LCOV_EXCL_LINE -- syntax belongs to the queried parameter
  }
  return text;  // LCOV_EXCL_LINE -- parsed parameter syntax contains its assignment
}

std::string parameterExprText(const slang::ast::ParameterSymbol& parameter) {
  const auto name = std::string(parameter.name);
  if (auto* syntax = parameter.getSyntax()) {
    auto expr = stripToRHS(syntax->toString(), name);
    if (!expr.empty()) {
      return expr;
    }
  }
  if (auto* syntax = parameter.getDeclaredType()->getInitializerSyntax()) {  // LCOV_EXCL_LINE
    return trim(collapseWhitespace(syntax->toString()));  // LCOV_EXCL_LINE
  }
  return {};  // LCOV_EXCL_LINE -- successfully parsed parameters have syntax
}

std::string constantValueText(const slang::ConstantValue& value) {
  if (!value) {
    return {};  // LCOV_EXCL_LINE -- elaborated value parameters have a value
  }
  return value.toString();
}

std::string padBinaryEncoding(std::string encoding, unsigned width) {
  if (width == 0) {
    return encoding;  // LCOV_EXCL_LINE -- enum base types cannot have zero width
  }
  auto basePos = encoding.find("'b");
  if (basePos == std::string::npos) {
    basePos = encoding.find("'sb");  // LCOV_EXCL_LINE -- binary formatter emits 'b
    if (basePos != std::string::npos) {  // LCOV_EXCL_LINE
      basePos += 1;  // LCOV_EXCL_LINE
    }
  }
  if (basePos == std::string::npos) {
    return encoding;  // LCOV_EXCL_LINE -- caller requests binary formatting
  }
  const auto bitsPos = basePos + 2;
  if (bitsPos >= encoding.size()) {
    return encoding;  // LCOV_EXCL_LINE -- formatter always emits digits after the base
  }
  const auto bitCount = encoding.size() - bitsPos;
  if (bitCount >= width) {
    return encoding;
  }
  encoding.insert(bitsPos, width - bitCount, '0');
  return encoding;
}

SNLSVIntentParam buildParamRecord(
  const slang::ast::Compilation& compilation,
  const slang::ast::ParameterSymbol& parameter) {
  SNLSVIntentParam result;
  result.name = std::string(parameter.name);
  result.value = constantValueText(parameter.getValue());
  result.expr = parameterExprText(parameter);
  result.localParam = parameter.isLocalParam();
  result.loc = sourceLocOf(compilation, parameter);
  return result;
}

const slang::ast::Type* symbolType(const slang::ast::Symbol& symbol) {
  if (const auto* value = symbol.as_if<slang::ast::ValueSymbol>()) {
    return &value->getType();
  }
  if (const auto* port = symbol.as_if<slang::ast::PortSymbol>()) {
    return &port->getType();
  }
  if (const auto* multiPort = symbol.as_if<slang::ast::MultiPortSymbol>()) {
    return &multiPort->getType();  // LCOV_EXCL_LINE -- multi-ports are not lowered or linked
  }
  if (const auto* declaredType = symbol.getDeclaredType()) {
    return &declaredType->getType();
  }
  return nullptr;
}

std::string sourceTypeKind(const slang::ast::Type& type) {
  switch (type.kind) {
    case slang::ast::SymbolKind::PredefinedIntegerType:
      return "integer";
    case slang::ast::SymbolKind::ScalarType:
      return "scalar";
    case slang::ast::SymbolKind::FloatingType:
      return "floating";
    case slang::ast::SymbolKind::EnumType:
      return "enum";
    case slang::ast::SymbolKind::PackedArrayType:
      return "packed_array";
    case slang::ast::SymbolKind::FixedSizeUnpackedArrayType:
      return "unpacked_array";
    case slang::ast::SymbolKind::DynamicArrayType:
      return "dynamic_array";
    case slang::ast::SymbolKind::AssociativeArrayType:
      return "associative_array";
    case slang::ast::SymbolKind::QueueType:
      return "queue";
    case slang::ast::SymbolKind::PackedStructType:
      return "packed_struct";
    case slang::ast::SymbolKind::UnpackedStructType:
      return "unpacked_struct";
    case slang::ast::SymbolKind::PackedUnionType:
      return "packed_union";
    case slang::ast::SymbolKind::UnpackedUnionType:
      return "unpacked_union";
    case slang::ast::SymbolKind::ClassType:
      return "class";
    case slang::ast::SymbolKind::StringType:
      return "string";
    case slang::ast::SymbolKind::EventType:
      return "event";
    case slang::ast::SymbolKind::VoidType:
      return "void";
    case slang::ast::SymbolKind::VirtualInterfaceType:
      return "virtual_interface";
    default:
      return "unknown";
  }
}

void addEnumDetails(
  const slang::ast::Compilation& compilation,
  const slang::ast::EnumType& enumType,
  SNLSVIntentType& result) {
  result.isEnum = true;
  result.enumWidth = enumType.baseType.getBitWidth();
  result.enumDeclLoc = sourceLocOf(compilation, enumType);
  for (const auto& member : enumType.values()) {
    SNLSVIntentEnumMember enumMember;
    enumMember.name = std::string(member.name);
    const auto& value = member.getValue();
    if (value.isInteger()) {
      enumMember.encoding = padBinaryEncoding(
        value.integer().toString(
          slang::LiteralBase::Binary,
          true,
          slang::SVInt::MAX_BITS),
        result.enumWidth);
    } else {
      enumMember.encoding = constantValueText(value);  // LCOV_EXCL_LINE -- enums are integral
    }
    result.members.push_back(std::move(enumMember));
  }
}

template<typename TAggregate>
void addStructDetails(
  const slang::ast::Compilation& compilation,
  const TAggregate& aggregateType,
  SNLSVIntentType& result) {
  result.isStruct = true;
  result.structWidth = aggregateType.getBitWidth();
  result.structDeclLoc = sourceLocOf(compilation, aggregateType);
  for (const auto& member : aggregateType.members()) {
    const auto* field = member.template as_if<slang::ast::FieldSymbol>();
    if (!field) {
      continue;  // LCOV_EXCL_LINE -- aggregate members are FieldSymbols by construction
    }
    const auto& fieldType = field->getType();
    const auto fieldWidth = fieldType.getBitWidth();
    SNLSVIntentStructField structField;
    structField.name = std::string(field->name);
    structField.typeName = fieldType.toString();
    structField.lsb = field->bitOffset;
    structField.msb = field->bitOffset + (fieldWidth ? fieldWidth - 1 : 0);
    result.fields.push_back(std::move(structField));
  }
}

SNLSVIntentType buildTypeRecord(
  const slang::ast::Compilation& compilation,
  const slang::ast::Symbol& symbol,
  const slang::ast::Type& type) {
  SNLSVIntentType result;
  result.valid = true;
  result.typeName = type.toString();
  const auto& canonical = type.getCanonicalType();
  result.canonicalKind = sourceTypeKind(canonical);
  result.declLoc = sourceLocOf(compilation, symbol);
  if (canonical.isEnum()) {
    addEnumDetails(compilation, canonical.as<slang::ast::EnumType>(), result);
  } else if (canonical.kind == slang::ast::SymbolKind::PackedStructType) {
    addStructDetails(
      compilation,
      canonical.as<slang::ast::PackedStructType>(),
      result);
  } else if (canonical.isPackedUnion()) {
    addStructDetails(
      compilation,
      canonical.as<slang::ast::PackedUnionType>(),
      result);
  }
  return result;
}

const slang::ast::InstanceBodySymbol* bodyOfSymbol(const slang::ast::Symbol& symbol) {
  if (const auto* body = symbol.as_if<slang::ast::InstanceBodySymbol>()) {
    return body;
  }
  if (const auto* instance = symbol.as_if<slang::ast::InstanceSymbol>()) {
    return &instance->body;
  }
  return nullptr;
}

const slang::ast::Symbol* packageMemberSymbol(
  slang::ast::Compilation& compilation,
  const std::string& package,
  const std::string& member) {
  const auto* packageSymbol = compilation.getPackage(package);
  if (!packageSymbol) {
    return nullptr;
  }
  return packageSymbol->find(member);
}

SNLSVIntentType typeOfSymbol(
  const SNLSVLiveASTLink& link,
  const slang::ast::Symbol& symbol) {
  auto* compilation = link.getCompilation();
  if (!compilation) {
    return {};  // LCOV_EXCL_LINE -- registry links retain their compilation
  }
  const auto* type = symbolType(symbol);
  if (!type) {
    return {};
  }
  return buildTypeRecord(*compilation, symbol, *type);
}

SNLSVIntentParams parametersOfSymbol(
  const SNLSVLiveASTLink& link,
  const slang::ast::Symbol& symbol) {
  SNLSVIntentParams result;
  auto* compilation = link.getCompilation();
  if (!compilation) {
    return result;  // LCOV_EXCL_LINE -- registry links retain their compilation
  }
  const auto* body = bodyOfSymbol(symbol);
  if (!body) {
    return result;
  }
  result.valid = true;
  result.module = std::string(body->getDefinition().name);
  for (const auto& member : body->members()) {
    if (const auto* parameter = member.as_if<slang::ast::ParameterSymbol>()) {
      result.params.push_back(buildParamRecord(*compilation, *parameter));
    }
  }
  return result;
}

}  // namespace

bool SNLSVIntent::available() {
  return getAvailableLink() != nullptr;
}

SNLSVIntentType SNLSVIntent::typeOf(const SNLDesignObject* object) {
  return typeOf(static_cast<const NLObject*>(object));
}

SNLSVIntentType SNLSVIntent::typeOf(const NLObject* object) {
  try {
    if (!object) {
      return {};
    }
    const auto* link = SNLSVLiveASTLinkRegistry::findForObject(object);
    if (!link || !link->getCompilation()) {
      return {};
    }
    const auto* symbol = link->getSymbol(object);
    if (!symbol) {
      return {};
    }
    return typeOfSymbol(*link, *symbol);
  } catch (...) {  // LCOV_EXCL_LINE -- third-party AST failure containment
    return {};  // LCOV_EXCL_LINE
  }
}

SNLSVIntentParams SNLSVIntent::parametersOf(const SNLDesignObject* object) {
  return parametersOf(static_cast<const NLObject*>(object));
}

SNLSVIntentParams SNLSVIntent::parametersOf(const NLObject* object) {
  try {
    if (!object) {
      return {};
    }
    const auto* link = SNLSVLiveASTLinkRegistry::findForObject(object);
    if (!link || !link->getCompilation()) {
      return {};
    }
    const auto* symbol = link->getSymbol(object);
    if (!symbol) {
      return {};
    }
    return parametersOfSymbol(*link, *symbol);
  } catch (...) {  // LCOV_EXCL_LINE -- third-party AST failure containment
    return {};  // LCOV_EXCL_LINE
  }
}

SNLSVIntentType SNLSVIntent::packageMemberType(
  const std::string& package,
  const std::string& member) {
  try {
    const auto* link = getAvailableLink();
    if (!link) {
      return {};
    }
    auto* compilation = link->getCompilation();
    auto* symbol = packageMemberSymbol(*compilation, package, member);
    if (!symbol) {
      return {};
    }
    auto type = typeOfSymbol(*link, *symbol);
    if (type.valid) {
      type.typeName = package + "::" + member;
    }
    return type;
  } catch (...) {  // LCOV_EXCL_LINE -- third-party AST failure containment
    return {};  // LCOV_EXCL_LINE
  }
}

SNLSVIntentParam SNLSVIntent::packageMember(
  const std::string& package,
  const std::string& member) {
  try {
    const auto* link = getAvailableLink();
    if (!link) {
      return {};
    }
    auto* compilation = link->getCompilation();
    auto* symbol = packageMemberSymbol(*compilation, package, member);
    const auto* parameter =
      symbol ? symbol->as_if<slang::ast::ParameterSymbol>() : nullptr;
    if (!parameter) {
      return {};
    }
    return buildParamRecord(*compilation, *parameter);
  } catch (...) {  // LCOV_EXCL_LINE -- third-party AST failure containment
    return {};  // LCOV_EXCL_LINE
  }
}

}  // namespace naja::NL
