// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLRTLConstructor.h"
#include "VHDLSourceInfo.h"

#include "NLException.h"
#include "NLDB0.h"
#include "SNLHDLLibrary.h"
#include "SNLInstParameter.h"
#include "NLLibrary.h"
#include "NajaPrivateProperty.h"
#include "NLName.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLBusTermBit.h"
#include <functional>
#include <fstream>
#include "SNLRTLPrimitives.h"
#include "SNLUtils.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <tuple>
#include <utility>

namespace naja::NL {
namespace {
using boost::multiprecision::int128_t;
// Boost 1.66 (Rocky Linux 8) expects allocator members removed from
// std::allocator in C++20. Keep its arbitrary-precision backend usable with
// both old and new Boost headers without changing allocation behavior.
template<typename T>
struct IntegerAllocator: std::allocator<T> {
  using pointer = T*;
  using const_pointer = const T*;
  template<typename U>
  struct rebind { using other = IntegerAllocator<U>; };
  IntegerAllocator() = default;
  template<typename U>
  IntegerAllocator(const IntegerAllocator<U>&) {}
};
using Integer = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
    0, 0, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked,
    IntegerAllocator<boost::multiprecision::limb_type>>>;
using Expr = vhdl::Expression;
using Statement = vhdl::SequentialStatement;
using Bits = std::vector<SNLBitNet*>;
constexpr size_t maximumHardwareBits = 1 << 20;

// Nested lowering restores the enclosing location, including on exceptions.
// Thread-local storage keeps independent constructors' diagnostics isolated.
thread_local vhdl::SourceSpan diagnosticSpan;
class DiagnosticScope {
 public:
  explicit DiagnosticScope(const vhdl::SourceSpan& span): previous_(diagnosticSpan) {
    diagnosticSpan = span;
  }
  ~DiagnosticScope() { diagnosticSpan = previous_; }
 private:
  vhdl::SourceSpan previous_;
};

[[noreturn]] void fail(const std::string& message) {
  throw VHDLRTLException(message, diagnosticSpan);
}

std::string key(const vhdl::Name& name) {
  return name.canonical.empty() ? name.spelling : name.canonical;
}

std::string key(const Expr& expression) {
  return expression.canonical.empty() ? expression.text : expression.canonical;
}

struct Range {
  int64_t left;
  int64_t right;
  bool ascending;
  const vhdl::EnumerationTypeDeclaration* indexEnumeration = nullptr;
  size_t size() const {
    if (ascending ? left > right : left < right) return 0;
    const auto width = ascending ? int128_t(right) - left + 1 : int128_t(left) - right + 1;
    if (width > 65536) fail("array range exceeds static elaboration limit");
    return static_cast<size_t>(width);
  }
  size_t position(int64_t index) const {
    if (ascending ? index < left || index > right : index > left || index < right)
      fail("static index " + std::to_string(index) + " is outside the declared range");
    return static_cast<size_t>(ascending ? index - left : left - index);
  }
};

struct Shape {
  // One nominal type at each dimension, followed by the scalar element type.
  std::vector<std::string> types;
  std::vector<Range> ranges;
  size_t integerWidth = 0;
  // Indirection keeps the recursive record shape complete when stored in a
  // standard-library container (std::pair cannot contain Shape by value here).
  std::vector<std::pair<std::string, std::shared_ptr<Shape>>> fields;
  const vhdl::EnumerationTypeDeclaration* enumeration = nullptr;
  // Identity of each user-defined array/record dimension, independent of spelling.
  std::vector<const void*> declarations;
  bool signedInteger = false;
  bool dynamicBounds = false;
  void removeOuterType() {
    types.erase(types.begin());
    if (!declarations.empty()) declarations.erase(declarations.begin());
  }
  size_t size() const {
    if (integerWidth) return integerWidth;
    size_t size = 1;
    if (enumeration) while ((size_t(1) << size) < enumeration->literals.size()) ++size;
    if (!fields.empty()) {
      size = 0;
      for (const auto& field : fields) {
        const auto width = field.second->size();
        if (size > maximumHardwareBits - width) fail("oversized hardware record");
        size += width;
      }
    }
    for (const auto& range : ranges) {
      const auto width = range.size();
      if (!width || size > maximumHardwareBits / width) fail("null or oversized hardware array");
      size *= width;
    }
    return size;
  }
};

bool compatible(const Shape& a, const Shape& b) {
  if (a.enumeration != b.enumeration) return false;
  if (a.integerWidth && b.integerWidth) return true;
  if (a.types != b.types || a.ranges.size() != b.ranges.size()) return false;
  for (size_t i = 0; i < std::max(a.declarations.size(), b.declarations.size()); ++i)
    if ((i < a.declarations.size() ? a.declarations[i] : nullptr) !=
        (i < b.declarations.size() ? b.declarations[i] : nullptr)) return false;
  if (a.fields.size() != b.fields.size()) return false;
  for (size_t i = 0; i < a.fields.size(); ++i)
    if (a.fields[i].first != b.fields[i].first || !compatible(*a.fields[i].second, *b.fields[i].second)) return false;
  for (size_t i = 0; i < a.ranges.size(); ++i)
    if (a.ranges[i].size() != b.ranges[i].size() ||
        a.ranges[i].indexEnumeration != b.ranges[i].indexEnumeration) return false;
  return true;
}

struct Value { Shape shape; Bits bits; };
struct Object {
  Value value;
  bool input = false;
  bool output = false;
  bool variable = false;
  bool constant = false;
};
struct Selection { std::string name; Shape shape; size_t offset = 0; };
struct MemoryWrite {
  Bits address, data;
  SNLBitNet* enable = nullptr;
  bool assigned = false;
  Bits mask;
};
struct VariableChoice {
  SNLBitNet* bit = nullptr;
  SNLBitNet* condition = nullptr;
  std::shared_ptr<VariableChoice> yes, no;
};
struct State {
  std::map<SNLBitNet*, bool> assumptions;
  std::map<std::string, std::vector<std::shared_ptr<VariableChoice>>> variableChoices;
  std::map<std::string, MemoryWrite> memoryWrites;
  std::map<std::string, Bits> variables;
  std::map<std::string, Bits> scheduled;
  std::map<std::string, std::vector<bool>> written;
};

class RTLSourceProperty final : public naja::NajaPrivateProperty {
 public:
  std::string signature;
  std::map<std::string, std::string> initializationFiles;
  std::string getName() const override { return "VHDLRTLSource"; }
  std::string getString() const override { return getName(); }
  static void attach(SNLDesign* design, const std::string& signature,
      const std::map<std::string, std::string>& initializationFiles) {
    auto* property = new RTLSourceProperty;
    property->signature = signature;
    property->initializationFiles = initializationFiles;
    property->postCreate(design);
  }
};

class LibraryContext {
 public:
  NLLibrary* destination;
  const std::vector<VHDLLibrarySource>& sources;
  const vhdl::DesignFile& syntax;
  const VHDLSourceMap& sourceMap;
  std::string_view source;
  VHDLIntent::SessionPtr intentSession;
  NLLibrary* owner(size_t offset) const {
    for (const auto& source : sources)
      if (offset >= source.offset && offset < source.offset + source.size) return source.library;
    return destination;
  }
  // Concatenation order between libraries is not VHDL declaration order.
  bool precedes(size_t use, size_t declaration) const {
    // Declaration order applies within a unit, not between registered units.
    const auto unitStart = [&](size_t offset) {
      size_t start = 0;
      const auto consider = [&](size_t candidate) {
        if (candidate <= offset) start = std::max(start, candidate);
      };
      for (const auto& package : syntax.packages) consider(package.name.span.start.offset);
      for (const auto& entity : syntax.entities) consider(entity.span.start.offset);
      for (const auto& architecture : syntax.architectures) consider(architecture.span.start.offset);
      return start;
    };
    return owner(use) == owner(declaration) && use < declaration &&
        unitStart(use) == unitStart(declaration);
  }
  size_t start(NLLibrary* library) const {
    for (const auto& source : sources) if (source.library == library) return source.offset;
    return 0;
  }
  NLLibrary* resolve(NLLibrary* work, const std::string& name) const {
    if (name == "work") return work;
    NLLibrary* result = nullptr;
    try {
      result = findHDLLibrary(work->getDB(), name);
    } catch (const NLException& error) {
      fail(error.what());
    }
    if (!result) fail("missing VHDL library: " + name);
    return result;
  }
};

struct SubtypeBinding {
  vhdl::TypeMark type;
  size_t offset;
};
using SubtypeMap = std::map<std::string, SubtypeBinding>;

struct StaticScalar {
  int64_t value = 0;
  bool boolean = false;
};
struct GenericValue {
  StaticScalar scalar;
  // Binary bits in declaration order, independent of the caller's netlist.
  std::optional<std::string> bits;
  std::optional<std::string> text;
};
using GenericValues = std::map<std::string, GenericValue>;

class RTLConstructor {
  struct LocalBindings;
 public:
  RTLConstructor(SNLDesign* design, const vhdl::EntityDeclaration& entity,
                 const vhdl::ArchitectureBody& architecture,
                 const vhdl::DesignFile& syntax, const LibraryContext& libraries,
                 GenericValues generics,
                 std::function<SNLDesign*(NLLibrary*, const std::string&, const GenericValues&)> build):
      design_(design), entity_(entity), architecture_(architecture), syntax_(syntax), libraryContext_(libraries),
      work_(libraries.owner(entity.span.start.offset)),
      genericOverrides_(std::move(generics)), build_(std::move(build)) {}

  void prepareEntity() {
    DiagnosticScope location(entity_.span);
    context(entity_.context);
    interfaceBindings_ = std::make_unique<LocalBindings>(*this);
    for (const auto& generic : entity_.generics) {
      DiagnosticScope location(generic.span);
      const auto type = key(generic.type.name);
      const bool vector = type == "bit_vector" || type == "std_logic_vector" ||
          type == "std_ulogic_vector" || type == "unsigned" || type == "signed";
      if (!vector && type != "integer" && type != "natural" && type != "positive" && type != "boolean" && type != "string")
        fail("RTL generics require an integer subtype, boolean, string, or constrained binary vector");
      std::optional<Shape> target;
      if (type == "string") {
        if (generic.type.constraint) fail("constrained string generics are unsupported");
        target = Shape{{"string"}, {}};
      } else if (vector) {
        target = shape(generic.type);
        target->size();
      }
      for (const auto& name : generic.names) {
        DiagnosticScope location(name.span);
        const auto id = key(name);
        if (!generic.defaultValue && !genericOverrides_.contains(id))
          fail("missing generic value: " + id);
        if (interfaceBindings_->names.contains(id)) fail("duplicate generic");
        const auto* expected = target ? &*target : nullptr;
        if (genericOverrides_.contains(id) && genericActual_)
          genericOverrides_.at(id) = genericActual_(id, expected);
        const auto value = genericOverrides_.contains(id) ? genericOverrides_.at(id) :
            genericValue(*generic.defaultValue, expected);
        if (retainingIntent()) {
          auto record = intentDeclaration(name, "generic");
          record["declared_type"] = declaredTypeIntent(generic.type, target ? &*target : nullptr);
          record["default_expression"] = expressionIntent(generic.defaultValue.get());
          record["overridden"] = genericOverrides_.contains(id);
          if (value.text) record["effective_value"] = *value.text;
          else if (value.bits) {
            record["effective_value"] = *value.bits;
            record["value_format"] = "bits_in_declaration_order";
          } else if (value.scalar.boolean) record["effective_value"] = bool(value.scalar.value);
          else record["effective_value"] = value.scalar.value;
          if (!genericOverrides_.contains(id)) record["default_value"] = record["effective_value"];
          else if (!generic.defaultValue) record["default_value_status"] = "absent";
          else if (generic.defaultValue && pureIntentScalar(*generic.defaultValue)) {
            try {
              const auto defaultValue = scalar(*generic.defaultValue);
              record["default_value"] = defaultValue.boolean ? Intent(bool(defaultValue.value)) : Intent(defaultValue.value);
            } catch (const NLException&) {
              // An overridden default need not be evaluable in this context.
              record["default_value_status"] = "unavailable";
            }
          } else if (generic.defaultValue && type == "string" && staticString(*generic.defaultValue)) {
            record["default_value"] = stringValue(*generic.defaultValue);
          } else record["default_value_status"] = "unsupported_without_additional_elaboration";
          intentDeclarations_.emplace_back(record);
        }
        interfaceBindings_->declare(id);
        if (retainingIntent()) intentBindings_[id] = intentDeclarations_.back();
        if (type == "string") {
          if (!value.text) fail("string generic requires a static string value: " + id);
          strings_.emplace(id, *value.text);
        } else if (vector) {
          if (!value.bits || value.bits->size() != target->size()) fail("vector generic type or length mismatch: " + id);
          Value object{*target, {}};
          for (char bit : *value.bits) object.bits.push_back(constant(bit == '1'));
          objects_.emplace(id, Object{std::move(object), false, false, false, true});
        } else {
          if (value.text || value.bits || value.scalar.boolean != (type == "boolean")) fail("generic value type mismatch: " + id);
          if (type == "boolean" && generic.type.constraint) fail("boolean generic constraints are unsupported");
          if (generic.type.constraint) range(*generic.type.constraint).position(value.scalar.value);
          if ((type == "positive" && value.scalar.value < 1) || (type == "natural" && value.scalar.value < 0))
            fail("generic value violates its subtype");
          if (value.scalar.boolean) booleans_.emplace(id, value.scalar.value);
          else integers_.emplace(id, value.scalar.value);
        }
      }
    }
  }

  bool staticString(const Expr& expr) const {
    return expr.kind == Expr::Kind::StringLiteral ||
        (expr.kind == Expr::Kind::Name && strings_.contains(key(expr))) ||
        (expr.kind == Expr::Kind::Binary && expr.text == "&" && staticString(*expr.left) && staticString(*expr.right));
  }

  std::string stringValue(const Expr& expr) const {
    if (expr.kind == Expr::Kind::Name && strings_.contains(key(expr))) return strings_.at(key(expr));
    if (expr.kind == Expr::Kind::Binary && expr.text == "&") return stringValue(*expr.left) + stringValue(*expr.right);
    if (expr.kind == Expr::Kind::StringLiteral) {
      std::string result;
      for (size_t i = 1; i + 1 < expr.text.size(); ++i) {
        result += expr.text[i];
        if (expr.text[i] == '"' && i + 2 < expr.text.size() && expr.text[i + 1] == '"') ++i;
      }
      return result;
    }
    fail("string generic actual must be a static string");
  }

  std::optional<StaticScalar> stringAttribute(const Expr& expr) const {
    if (expr.kind != Expr::Kind::Attribute || !expr.left || expr.left->kind != Expr::Kind::Name ||
        !strings_.contains(key(*expr.left))) return std::nullopt;
    const auto size = int64_t(strings_.at(key(*expr.left)).size());
    const auto name = key(expr);
    if (name == "length") return StaticScalar{size, false};
    if (name == "left" || name == "low") return StaticScalar{1, false};
    if (name == "right" || name == "high") return StaticScalar{size, false};
    if (name == "ascending") return StaticScalar{1, true};
    fail("unsupported string generic attribute: " + name);
  }

  GenericValue genericValue(const Expr& actual, const Shape* expected) {
    DiagnosticScope location(actual.span);
    if (expected && expected->types == std::vector<std::string>{"string"})
      return {{}, {}, stringValue(actual)};
    if (!expected) return {scalar(actual), {}};
    const auto value = expression(actual, State{}, expected);
    std::string bits;
    for (auto* bit : value.bits) {
      const auto binary = constantValue(bit);
      if (!binary) fail("vector generic actual must be static and binary");
      bits += *binary ? '1' : '0';
    }
    return {{}, std::move(bits)};
  }

  const auto& getInitializationFiles() const { return *initializationFiles_; }

  void retainPackages(NLLibrary* library) {
    if (!libraryContext_.intentSession) return;
    captureIntent_ = true;
    for (const auto& package : syntax_.packages) {
      if (package.body || libraryContext_.owner(package.name.span.start.offset) != library) continue;
      intentBindings_.clear();
      for (const auto& enumeration : package.enumerationTypes) {
        auto record = intentDeclaration(enumeration.name, "type");
        record["resolved_type"] = shapeIntent(Shape{{key(enumeration.name)}, {}, 0, {}, &enumeration});
        rememberIntent(enumeration.name, std::move(record));
      }
      for (const auto& subtype : package.subtypes) {
        auto record = intentDeclaration(subtype.name, "subtype");
        record["base_type"] = declaredTypeIntent(subtype.type);
        record["resolution_status"] = "unavailable_without_elaboration";
        rememberIntent(subtype.name, std::move(record));
      }
      for (const auto& constant : package.constants) for (const auto& name : constant.object.names) {
        auto record = intentDeclaration(name, "constant");
        record["declared_type"] = declaredTypeIntent(constant.object.type);
        record["expression"] = expressionIntent(constant.value.get());
        record["value_status"] = "unavailable_without_elaboration";
        rememberIntent(name, std::move(record));
      }
      for (const auto& array : package.arrayTypes) {
        auto record = intentDeclaration(array.name, "type");
        record["element_type"] = declaredTypeIntent(array.elementType);
        record["resolution_status"] = "unavailable_without_elaboration";
        rememberIntent(array.name, std::move(record));
      }
      for (const auto& recordType : package.recordTypes) {
        auto record = intentDeclaration(recordType.name, "type");
        IntentList fields;
        for (const auto& field : recordType.fields) for (const auto& name : field.names)
          fields.emplace_back(IntentDict{{"name", name.spelling}, {"declared_type", declaredTypeIntent(field.type)}});
        record["fields"] = std::move(fields);
        record["resolution_status"] = "unavailable_without_elaboration";
        rememberIntent(recordType.name, std::move(record));
      }
    }
    VHDLIntent::attach(library, libraryContext_.intentSession,
        IntentDict{{"status", "available"}, {"language", "vhdl"}, {"analysis", "package_declarations"},
            {"declarations", intentDeclarations_}});
  }

  void run() {
    DiagnosticScope location(entity_.span);
    captureIntent_ = true;
    prepareEntity();
    std::vector<Shape> portShapes;
    for (const auto& port : entity_.ports) {
      if (port.mode != vhdl::PortMode::In && port.mode != vhdl::PortMode::Out)
        fail("RTL ports must be in or out");
      if (port.defaultValue && port.mode != vhdl::PortMode::In)
        fail("port defaults are supported only for input ports");
      portShapes.push_back(shape(port.type));
    }
    // Ports become visible after the interface clause; generic objects are
    // already visible while resolving its bounds.
    for (size_t i = 0; i < entity_.ports.size(); ++i) {
      const auto& port = entity_.ports[i];
      for (const auto& name : port.names) {
        interfaceBindings_->declare(key(name));
        objectIntent(name, port.type, portShapes[i], "port");
        addObject(name, portShapes[i], port.mode == vhdl::PortMode::In,
                  port.mode == vhdl::PortMode::Out, false, true);
      }
    }
    context(architecture_.context);
    std::set<std::string> localComponents;
    for (const auto& component : architecture_.components) {
      const auto name = key(component.name);
      if (!localComponents.insert(name).second) fail("duplicate local component: " + name);
      components_[name] = &component;
    }
    LocalBindings architectureBindings(*this);
    architectureBindings.names = interfaceBindings_->names;
    declarations(architecture_.arrayTypes, architecture_.constants, architecture_.recordTypes,
                 architecture_.signals, architecture_.enumerationTypes, architecture_.functions, architecture_.subtypes,
                 &architectureBindings, architecture_.aliases);
    inferMemories();
    checkLabels(architecture_.generates, architecture_.instantiations);
    State empty;
    for (const auto& assignment : architecture_.assignments)
      concurrent(assignment, empty);
    for (const auto& generate : architecture_.generates) lowerGenerate(generate, empty);
    for (const auto& instance : architecture_.instantiations) lowerInstance(instance);
    for (const auto& process : architecture_.processes) lowerProcess(process);
    finalizeMemories();
    // A signal with no elaborated driver retains its declaration value for
    // the entire simulation, including when a generate removes its only writer.
    const auto initializeUndriven = [&](const Object& object) {
      if (object.input || object.variable || object.constant) return;
      for (auto* bit : object.value.bits) {
        const auto initial = initialValues_.find(bit);
        if (initial != initialValues_.end() && !drivers_.contains(bit))
          drive(bit, constant(initial->second));
      }
    };
    for (const auto& [name, object] : objects_) initializeUndriven(object);
    for (const auto& [name, object] : generatedObjects_) initializeUndriven(object);
    checkInitialValues();
    for (const auto& [name, object] : objects_)
      if (!memories_.contains(name)) checkDriven(name, object);
    for (const auto& [name, object] : generatedObjects_) checkDriven(name, object);
    if (retainingIntent()) VHDLIntent::attach(design_, libraryContext_.intentSession,
        IntentDict{{"status", "available"}, {"language", "vhdl"},
            {"entity", intentDeclaration(entity_.name, "entity")},
            {"architecture", intentDeclaration(architecture_.name, "architecture")},
            {"context", design_->getName().getString()}, {"declarations", intentDeclarations_}});
  }

 private:
  using Intent = VHDLIntentValue;
  using IntentDict = Intent::Dict;
  using IntentList = Intent::List;

  bool retainingIntent() const { return captureIntent_ && libraryContext_.intentSession; }

  IntentDict intentDeclaration(const vhdl::Name& name, const char* kind) const {
    const auto offset = name.span.start.offset;
    auto* owner = libraryContext_.owner(offset);
    std::string unit, unitKind;
    size_t closest = 0;
    const auto consider = [&](const vhdl::Name& candidate, const char* candidateKind) {
      const auto start = candidate.span.start.offset;
      if (start <= offset && start >= closest && libraryContext_.owner(start) == owner) {
        closest = start;
        unit = candidate.spelling;
        unitKind = candidateKind;
      }
    };
    for (const auto& package : syntax_.packages) consider(package.name, package.body ? "package_body" : "package");
    for (const auto& entity : syntax_.entities) consider(entity.name, "entity");
    for (const auto& architecture : syntax_.architectures) consider(architecture.name, "architecture");
    IntentList scopes;
    const auto addScope = [&](const vhdl::SourceSpan& span, const std::string& display, const char* kind) {
      if (span.start.offset <= offset && offset < span.end.offset)
        scopes.emplace_back(IntentDict{{"id", "scope:" + std::to_string(owner->getID()) + ":" +
            std::to_string(span.start.offset)}, {"name", display}, {"kind", kind}});
    };
    const auto visit = [&](const auto& self, const auto& scope) -> void {
      for (const auto& process : scope.processes) {
        addScope(process.span, process.label ? process.label->spelling : std::string{}, "process");
        for (const auto& function : process.functions) addScope(function.span, function.name.spelling, "subprogram");
      }
      for (const auto& function : scope.functions) addScope(function.span, function.name.spelling, "subprogram");
      for (const auto& generate : scope.generates) {
        self(self, generate);
        for (const auto& branch : generate.alternatives) self(self, branch);
      }
    };
    for (const auto& architecture : syntax_.architectures) visit(visit, architecture);
    const auto ref = libraryContext_.sourceMap.reference(name.span);
    return {{"id", "decl:" + std::to_string(owner->getID()) + ":" + std::to_string(offset)},
        {"name", name.spelling}, {"canonical_name", key(name)}, {"kind", kind},
        {"library", owner->getName().getString()}, {"owner", unit}, {"owner_kind", unitKind},
        {"scope", objectPrefix_}, {"lexical_scopes", std::move(scopes)},
        {"source", IntentDict{{"file", ref.range.file.getString()}, {"line", int64_t(ref.range.line)},
            {"column", int64_t(ref.range.column)}}}};
  }

  // Restrict additional default queries to the existing, side-effect-free
  // scalar evaluator. In particular, never call a user function or create bits.
  bool pureIntentScalar(const Expr& expression) const {
    if (expression.kind == Expr::Kind::IntegerLiteral) return true;
    if (expression.kind == Expr::Kind::Name)
      return integers_.contains(key(expression)) || booleans_.contains(key(expression)) ||
          key(expression) == "true" || key(expression) == "false";
    if (expression.kind == Expr::Kind::Unary)
      return expression.left && pureIntentScalar(*expression.left);
    if (expression.kind == Expr::Kind::Binary)
      return expression.left && expression.right && pureIntentScalar(*expression.left) && pureIntentScalar(*expression.right);
    return false;
  }

  Intent expressionIntent(const Expr* expression) const {
    if (!expression) return {};
    IntentList dependencies;
    const auto walk = [&](const auto& self, const Expr& expr) -> void {
      if (expr.kind == Expr::Kind::Name) {
        const auto found = intentBindings_.find(key(expr));
        IntentDict dependency{{"name", expr.text}, {"status", "unresolved"}};
        if (found != intentBindings_.end() && !(enumLiterals_.contains(key(expr)) && enumLiterals_.at(key(expr)).size() > 1)) {
          const auto& declaration = std::get<IntentDict>(found->second.value);
          dependency["declaration_id"] = declaration.at("id");
          dependency["scope"] = declaration.at("scope");
          dependency["status"] = "resolved";
        }
        dependencies.emplace_back(std::move(dependency));
      }
      const bool indexedCall = expr.kind == Expr::Kind::Indexed && expr.left &&
          expr.left->kind == Expr::Kind::Name && functions_.contains(key(*expr.left));
      if (expr.kind == Expr::Kind::Call || expr.kind == Expr::Kind::Selected || indexedCall)
        dependencies.emplace_back(IntentDict{{"name", expr.text}, {"status", "unsupported"},
            {"reason", "call or selected-name dependency resolution is not retained"}});
      if (expr.left) self(self, *expr.left);
      if (expr.right) self(self, *expr.right);
      if (expr.condition) self(self, *expr.condition);
      for (const auto& element : expr.elements) self(self, *element);
    };
    walk(walk, *expression);
    const auto start = std::min(expression->span.start.offset, libraryContext_.source.size());
    const auto end = std::min(expression->span.end.offset, libraryContext_.source.size());
    const auto ref = libraryContext_.sourceMap.reference(expression->span);
    return IntentDict{{"text", std::string(libraryContext_.source.substr(start, end >= start ? end - start : 0))},
        {"dependencies", std::move(dependencies)},
        {"source", IntentDict{{"file", ref.range.file.getString()}, {"line", int64_t(ref.range.line)},
            {"column", int64_t(ref.range.column)}}}};
  }

  Intent shapeIntent(const Shape& type) const {
    IntentDict result{{"status", "available"}, {"width", int64_t(type.size())},
        {"name", type.types.empty() ? std::string{} : type.types.front()}};
    IntentList ranges;
    for (const auto& range : type.ranges) {
      IntentDict bounds{{"left", range.left}, {"right", range.right},
          {"direction", range.ascending ? "to" : "downto"}, {"length", int64_t(range.size())},
          {"index_kind", range.indexEnumeration ? "enumeration" : "integer"}};
      if (range.indexEnumeration) {
        const auto definition = intentNominalTypes_.find(range.indexEnumeration);
        bounds["index_type"] = definition == intentNominalTypes_.end() ?
            Intent(intentDeclaration(range.indexEnumeration->name, "type")) : definition->second;
      }
      ranges.emplace_back(std::move(bounds));
    }
    result["ranges"] = std::move(ranges);
    if (!type.ranges.empty()) {
      result["kind"] = "array";
      auto element = type;
      element.ranges.erase(element.ranges.begin());
      element.removeOuterType();
      result["element_type"] = shapeIntent(element);
    } else if (type.enumeration) {
      result["kind"] = "enumeration";
      const auto definition = intentNominalTypes_.find(type.enumeration);
      result["declaration"] = definition == intentNominalTypes_.end() ?
          Intent(intentDeclaration(type.enumeration->name, "type")) : definition->second;
      result["encoding"] = "unsigned_binary_ordinal";
      IntentList literals;
      for (size_t i = 0; i < type.enumeration->literals.size(); ++i) {
        const auto& literal = type.enumeration->literals[i];
        literals.emplace_back(IntentDict{{"name", literal.spelling}, {"ordinal", int64_t(i)},
            {"representation", int64_t(i)}});
      }
      result["literals"] = std::move(literals);
    } else if (!type.fields.empty()) {
      result["kind"] = "record";
      IntentList fields;
      const auto declaration = type.declarations.empty() ? intentRecordFields_.end() :
          intentRecordFields_.find(type.declarations.front());
      size_t position = 0;
      for (const auto& [name, field] : type.fields) {
        IntentDict item{{"name", name}};
        if (declaration != intentRecordFields_.end() && position < declaration->second.size())
          item = std::get<IntentDict>(declaration->second[position].value);
        item["type"] = shapeIntent(*field);
        fields.emplace_back(std::move(item));
        ++position;
      }
      result["fields"] = std::move(fields);
    } else result["kind"] = type.integerWidth ? "integer" : "scalar";
    // Resolve nominal declarations using elaborator identity.
    const void* nominal = type.declarations.empty() ? nullptr : type.declarations.front();
    if (const auto found = intentNominalTypes_.find(nominal); found != intentNominalTypes_.end())
      result["declaration"] = found->second;
    return result;
  }

  Intent declaredTypeIntent(const vhdl::TypeMark& type, const Shape* resolved = nullptr) const {
    IntentDict result{{"name", type.name.spelling}, {"canonical_name", key(type.name)}};
    const auto found = intentBindings_.find(key(type.name));
    if (found != intentBindings_.end())
      result["declaration_id"] = std::get<IntentDict>(found->second.value).at("id");
    else {
      static const std::set<std::string> builtins{"bit", "bit_vector", "boolean", "integer", "natural", "positive",
          "string", "std_logic", "std_ulogic", "std_logic_vector", "std_ulogic_vector", "signed", "unsigned"};
      if (builtins.contains(key(type.name))) {
        result["identity_status"] = "builtin";
        result["builtin_type"] = key(type.name);
      } else result["identity_status"] = "unavailable";
    }
    if (type.constraint) {
      result["constraint"] = IntentDict{{"direction", type.constraint->ascending ? "to" : "downto"},
          {"left_expression", expressionIntent(type.constraint->leftExpression.get())},
          {"right_expression", expressionIntent(type.constraint->rightExpression.get())}};
    }
    if (resolved) {
      result["resolved"] = shapeIntent(*resolved);
      const auto subtype = subtypes_.find(key(type.name));
      if (subtype != subtypes_.end() && subtype->second.type.constraint) {
        const auto& bounds = *subtype->second.type.constraint;
        result["subtype_constraint"] = IntentDict{{"left", bounds.left}, {"right", bounds.right},
            {"direction", bounds.ascending ? "to" : "downto"}};
      }
    } else result["resolution_status"] = "unavailable";
    return result;
  }

  void rememberIntent(const vhdl::Name& name, IntentDict record) {
    if (!retainingIntent()) return;
    intentBindings_[key(name)] = record;
    intentDeclarations_.emplace_back(std::move(record));
  }

  void constantIntent(const vhdl::ConstantDeclaration& declaration) {
    if (!retainingIntent()) return;
    for (const auto& name : declaration.object.names) {
      auto record = intentDeclaration(name, "constant");
      record["expression"] = expressionIntent(declaration.value.get());
      record["declared_type"] = declaredTypeIntent(declaration.object.type);
      const auto id = key(name);
      if (integers_.contains(id)) record["effective_value"] = integers_.at(id);
      else if (booleans_.contains(id)) record["effective_value"] = bool(booleans_.at(id));
      else if (objects_.contains(id)) {
        const auto& value = objects_.at(id).value;
        record["resolved_type"] = shapeIntent(value.shape);
        std::string bits;
        for (auto* bit : value.bits)
          bits += bit->isConstant0() ? '0' : bit->isConstant1() ? '1' : bit->isConstantX() ? 'x' : 'z';
        record["effective_value"] = bits;
        record["value_format"] = "bits_in_declaration_order";
      } else record["value_status"] = "unsupported";
      rememberIntent(name, std::move(record));
    }
  }

  void objectIntent(const vhdl::Name& name, const vhdl::TypeMark& mark, const Shape& type, const char* kind) {
    if (!retainingIntent()) return;
    auto record = intentDeclaration(name, kind);
    record["declared_type"] = declaredTypeIntent(mark, &type);
    intentObjects_[name.span.start.offset] = record;
    rememberIntent(name, std::move(record));
  }

  void attachObjectIntent(NLObject* object, const vhdl::Name& name, const Shape& type) {
    if (!retainingIntent()) return;
    auto found = intentObjects_.find(name.span.start.offset);
    Intent record = found == intentObjects_.end() ? Intent(intentDeclaration(name, "object")) : found->second;
    auto& dict = std::get<IntentDict>(record.value);
    dict["resolved_type"] = shapeIntent(type);
    dict["status"] = "available";
    dict["language"] = "vhdl";
    dict["context"] = design_->getName().getString();
    VHDLIntent::attach(object, libraryContext_.intentSession, std::move(record));
  }

  template<typename T>
  T* origin(T* object) const {
    return libraryContext_.sourceMap.annotate(object, diagnosticSpan);
  }
  std::vector<SNLInstance*> origin(std::vector<SNLInstance*> instances) const {
    for (auto* instance : instances) origin(instance);
    return instances;
  }
  template<typename T>
  T* declaration(T* object, const vhdl::SourceSpan& span) const {
    return libraryContext_.sourceMap.annotate(object, span, true);
  }

  SNLBitNet* reuse(SNLBitNet* net) const {
    if (auto* infos = net->getRTLInfos(); infos && infos->getSourceDeclaration()) return net;
    origin(net);
    for (auto* term : net->getInstTerms())
      if (term->getDirection() == SNLTerm::Direction::Output) origin(term->getInstance());
    return net;
  }

  struct Sensitivity {
    std::set<std::string> names;
    std::set<SNLBitNet*> bits;
    std::map<SNLBitNet*, std::string> signals;
  };

  Sensitivity processSensitivity(const vhdl::ClockedProcess& process) {
    Sensitivity result;
    for (const auto& [name, object] : objects_)
      if (!object.variable && !object.constant)
        for (auto* bit : object.value.bits) result.signals.emplace(bit, name);
    if (process.allSensitivity) return result;
    const auto add = [&](const vhdl::Name& name, const std::vector<vhdl::Name>& fields) {
      DiagnosticScope location(name.span);
      auto selected = selection(key(name));
      const auto& object = objects_.at(selected.name);
      if (object.variable || object.constant) fail("process sensitivity requires signals");
      for (const auto& member : fields) field(selected, key(member));
      if (fields.empty()) result.names.insert(key(name));
      for (size_t i = 0; i < selected.shape.size(); ++i)
        result.bits.insert(object.value.bits[selected.offset + i]);
    };
    if (process.sensitivityList.empty()) add(process.sensitivity, {});
    else for (size_t i = 0; i < process.sensitivityList.size(); ++i)
      add(process.sensitivityList[i], process.sensitivityFields.empty() ?
          std::vector<vhdl::Name>{} : process.sensitivityFields[i]);
    return result;
  }

  void checkDriven(const std::string& name, const Object& object) {
    if (object.variable || object.input || object.constant) return;
    for (auto* bit : object.value.bits)
      if ((object.output || reads_.contains(bit)) && !drivers_.contains(bit))
        fail("signal bit has no driver: " + name);
  }

  struct InitializerScope {
    bool& active;
    bool previous;
    explicit InitializerScope(bool& flag): active(flag), previous(flag) { active = true; }
    ~InitializerScope() { active = previous; }
  };

  Value initializer(const Expr& expr, const Shape& type) {
    InitializerScope scope(initializerContext_);
    auto value = expression(expr, State{}, &type);
    for (auto* bit : value.bits)
      if (!bit || !bit->isConstant()) fail("initializer must be static");
    return value;
  }

  void addSignal(const vhdl::SignalDeclaration& signal, const Shape* declaredType = nullptr, const Value* initializer = nullptr) {
    DiagnosticScope location(signal.span);
    const auto type = declaredType ? *declaredType : shape(signal.type);
    const auto initial = signal.initializer ? std::optional<Value>{initializer ? *initializer :
        this->initializer(*signal.initializer, type)} : std::nullopt;
    const bool deferred = arrays_.contains(type.types.front()) && type.fields.empty();
    for (const auto& name : signal.names) {
      objectIntent(name, signal.type, type, "signal");
      addObject(name, type, false, false, false, false, deferred);
      if (deferred) pendingSignals_.emplace(key(name), PendingSignal{name, signal.span, initial ? initial->bits : Bits{}});
    }
    if (initial && !deferred) {
      const auto& value = *initial;
      for (const auto& name : signal.names) {
        const auto& bits = objects_.at(key(name)).value.bits;
        for (size_t i = 0; i < bits.size(); ++i) {
          const auto initial = constantValue(value.bits[i]);
          if (!initial) fail("signal initializer must be a constant binary value");
          initialValues_[bits[i]] = *initial;
          initialDeclarations_[bits[i]] = {name.spelling, signal.span};
        }
      }
    }
  }

  struct PendingSignal { vhdl::Name name; vhdl::SourceSpan span; Bits initial; };

  void materializeSignal(const std::string& name) {
    const auto found = pendingSignals_.find(name);
    if (found == pendingSignals_.end() || memories_.contains(name)) return;
    const auto pending = std::move(found->second);
    DiagnosticScope location(pending.span);
    pendingSignals_.erase(found);
    const auto type = objects_.at(name).value.shape;
    objects_.erase(name);
    addObject(pending.name, type);
    const auto& bits = objects_.at(name).value.bits;
    for (size_t i = 0; i < pending.initial.size(); ++i) {
      const auto value = constantValue(pending.initial[i]);
      if (!value) fail("register initializer must be a constant binary value");
      initialValues_[bits[i]] = *value;
      initialDeclarations_[bits[i]] = {pending.name.spelling, pending.span};
    }
  }

  static std::string compactMemoryInitial(const std::string& binary) {
    const auto first = binary.find("'b");
    if (first == std::string::npos) return binary;
    const auto size = binary.size() - first - 2;
    std::string digits;
    const auto padding = (4 - size % 4) % 4;
    for (size_t offset = 0; offset < size + padding; offset += 4) {
      char nibble[4];
      unsigned number = 0;
      bool numeric = true;
      for (size_t i = 0; i < 4; ++i) {
        nibble[i] = offset + i < padding ? '0' : binary[first + 2 + offset + i - padding];
        numeric &= nibble[i] == '0' || nibble[i] == '1';
        number = (number << 1) | (nibble[i] == '1');
      }
      if (numeric) digits += "0123456789abcdef"[number];
      else if (std::all_of(std::begin(nibble), std::end(nibble), [&](char c) { return c == nibble[0]; }))
        digits += nibble[0];
      else return binary;
    }
    return binary.substr(0, first) + "'h" + digits;
  }

  struct MemoryRead { Bits address, data; };
  struct Memory {
    const vhdl::ClockedProcess* process = nullptr;
    NLDB0::MemorySignature signature;
    std::vector<MemoryRead> reads;
    MemoryWrite write;
    SNLBitNet* clock = nullptr;
    std::string initial;
  };

  // Infer a single syntactic write site. Partial writes use canonical masks;
  // unrolled partial writes must share an address, checked during lowering.
  // Whole-word writes inside loops keep register lowering. Initial contents
  // belong to the inferred memory, without allocating per-word signal nets.
  void inferMemories(const vhdl::GenerateStatement* scope = nullptr, const std::set<std::string>* names = nullptr) {
    for (const auto& [name, object] : objects_) {
      if ((names && !names->contains(name)) || memories_.contains(name)) continue;
      const auto& type = object.value.shape;
      if (object.input || object.output || object.variable || object.constant ||
          !type.fields.empty() || type.ranges.empty() || type.ranges.front().indexEnumeration || type.ranges.size() > 2 ||
          !arrays_.contains(type.types.front()) ||
          std::min(type.ranges.front().left, type.ranges.front().right) < 0 ||
          std::max(type.ranges.front().left, type.ranges.front().right) > INT32_MAX ||
          std::any_of(aliases_.begin(), aliases_.end(), [&](const auto& alias) { return alias.second.name == name; })) continue;
      size_t writes = 0;
      bool supported = true, masked = false;
      const vhdl::ClockedProcess* owner = nullptr;
      const auto assignment = [&](const vhdl::Assignment& a,
                                  const vhdl::ClockedProcess* process, bool loop) {
        if (key(a.target) != name) return;
        ++writes;
        owner = process;
        const bool partial = a.indices.size() > 1;
        masked |= partial;
        supported &= process && !process->combinational && !process->asynchronousReset && (!loop || partial) &&
            a.kind == vhdl::AssignmentKind::Signal && !a.indices.empty() &&
            a.indices.front()->kind != Expr::Kind::Range && !isStatic(*a.indices.front());
      };
      std::function<void(const std::vector<Statement>&, const vhdl::ClockedProcess*, bool)> scan;
      scan = [&](const auto& statements, auto* process, bool loop) {
        for (const auto& statement : statements) {
          if (statement.kind == Statement::Kind::Assignment)
            assignment(statement.assignment, process, loop);
          scan(statement.statements, process, loop || statement.kind == Statement::Kind::For);
          scan(statement.alternative, process, loop || statement.kind == Statement::Kind::For);
          for (const auto& body : statement.caseBodies) scan(body, process, loop);
        }
      };
      for (const auto& process : scope ? scope->processes : architecture_.processes) {
        scan(process.statements, &process, false);
        scan(process.resetStatements, &process, false);
        for (const auto& a : process.assignments)
          assignment(a, process.sensitivityList.empty() ? &process : nullptr, false);
        for (const auto& a : process.resetAssignments) assignment(a, &process, false);
      }
      for (const auto& a : scope ? scope->assignments : architecture_.assignments) assignment(a, nullptr, false);
      const auto instances = [&](const auto& instances) {
        for (const auto& instance : instances)
          for (const auto& actual : instance.actuals)
            if (key(actual) == name) supported = false;
      };
      instances(scope ? scope->instantiations : architecture_.instantiations);
      std::function<void(const std::vector<vhdl::GenerateStatement>&)> generates;
      generates = [&](const auto& statements) {
        for (const auto& generate : statements) {
          for (const auto& a : generate.assignments) assignment(a, nullptr, true);
          instances(generate.instantiations);
          for (const auto& process : generate.processes) {
            scan(process.statements, &process, true);
            scan(process.resetStatements, &process, true);
            for (const auto& a : process.assignments) assignment(a, &process, true);
            for (const auto& a : process.resetAssignments) assignment(a, &process, true);
          }
          generates(generate.generates);
          generates(generate.alternatives);
        }
      };
      generates(scope ? scope->generates : architecture_.generates);
      if (!supported || writes != 1) continue;
      Memory memory;
      memory.process = owner;
      memory.signature.depth = type.ranges.front().size();
      memory.signature.width = type.size() / memory.signature.depth;
      memory.signature.abits = 1;
      while ((size_t(1) << memory.signature.abits) < memory.signature.depth)
        ++memory.signature.abits;
      memory.signature.writePorts = 1;
      memory.signature.independentWriteClocks = masked;
      if (const auto pending = pendingSignals_.find(name); pending != pendingSignals_.end() && !pending->second.initial.empty()) {
        const auto& data = pending->second.initial;
        // INIT's least significant word is address zero. VHDL arrays instead
        // store values in declaration order, which may be descending/nonzero.
        memory.initial = std::to_string(data.size()) + "'b";
        const auto& bounds = type.ranges.front();
        const auto low = std::min(bounds.left, bounds.right);
        for (size_t address = memory.signature.depth; address; --address) {
          const auto position = bounds.position(low + int64_t(address - 1)) * memory.signature.width;
          for (size_t bit = 0; bit < memory.signature.width; ++bit) {
            const auto* net = data[position + bit];
            if (!net || !net->isConstant()) fail("memory initializer must be static");
            memory.initial += net->isConstant0() ? '0' : net->isConstant1() ? '1' : net->isConstantX() ? 'x' : 'z';
          }
        }
      }
      if (!memory.initial.empty()) memory.initial = compactMemoryInitial(memory.initial);
      memories_.emplace(name, std::move(memory));
    }
    std::vector<std::string> pending;
    for (const auto& [name, signal] : pendingSignals_)
      if ((!names || names->contains(name)) && !memories_.contains(name)) pending.push_back(name);
    for (const auto& name : pending) materializeSignal(name);
  }

  // Full-width bounds checks prevent a wide or nonzero-based VHDL index from
  // wrapping onto a valid memory word when converted to the primitive address.
  std::pair<Bits, SNLBitNet*> memoryAddress(const std::string& name, Bits address) {
    const auto& bounds = objects_.at(name).value.shape.ranges.front();
    const auto low = std::min(bounds.left, bounds.right);
    const auto high = std::max(bounds.left, bounds.right);
    const auto width = std::max<size_t>(32, address.size());
    address = resize(std::move(address), width);
    const auto lessThan = [&](uint64_t limit) {
      auto* less = constant(false);
      for (size_t i = address.size(); i; --i) {
        const auto position = address.size() - i;
        const bool one = position < 64 && ((limit >> position) & 1);
        if (const auto bit = constantValue(address[i - 1])) {
          if (*bit != one) less = constant(one);
        } else if (const auto previous = constantValue(less)) {
          less = *previous == one ? constant(one) : gate("not", address[i - 1]);
        } else {
          less = one ? gate("or", gate("not", address[i - 1]), less)
                     : gate("and", gate("not", address[i - 1]), less);
        }
      }
      return less;
    };
    auto* valid = lessThan(uint64_t(high) + 1);
    if (low) valid = gate("and", gate("not", lessThan(low)), valid);
    if (low) address = add(address, number(low, {{"integer"}, {}, address.size()}).bits, true);
    return {resize(std::move(address), memories_.at(name).signature.abits), valid};
  }

  Value readMemory(const std::string& name, const Expr& index, const State& state) {
    if (sensitivity_) for (auto* bit : objects_.at(name).value.bits)
      if (!sensitivity_->bits.contains(bit))
        fail("process sensitivity omits a memory read: " + name);
    auto element = objects_.at(name).value.shape;
    if (isStatic(index)) element.ranges.front().position(integer(index));
    auto addressValue = expression(index, state);
    if (!addressValue.shape.integerWidth) fail("array index must have integer type");
    auto [address, valid] = memoryAddress(name, std::move(addressValue.bits));
    element.ranges.erase(element.ranges.begin());
    element.removeOuterType();
    auto& memory = memories_.at(name);
    // Reuse a port for identical resolved addresses (including constant words).
    for (const auto& read : memory.reads)
      if (read.address == address)
        return {element, mux(valid, read.data, Bits(element.size(), constant(false)))};
    auto* output = origin(SNLBusNet::create(design_, element.size() - 1, 0));
    Bits data;
    for (size_t i = 0; i < element.size(); ++i) data.push_back(output->getBitAtPosition(i));
    memory.reads.push_back({std::move(address), data});
    return {element, mux(valid, data, Bits(element.size(), constant(false)))};
  }

  void finalizeMemories(const std::set<std::string>* names = nullptr) {
    for (auto& [name, memory] : memories_) {
      if (names && !names->contains(name)) continue;
      DiagnosticScope location(memory.process->span);
      if (!memory.clock) fail("inferred memory has no clocked writer: " + name);
      memory.signature.readPorts = std::max<size_t>(1, memory.reads.size());
      auto* model = NLDB0::getOrCreateMemory(memory.signature);
      auto instanceName = objectPrefix_ + name + "_mem";
      for (size_t suffix = 1; design_->getInstance(NLName(instanceName)); ++suffix)
        instanceName = objectPrefix_ + name + "_mem_" + std::to_string(suffix);
      auto* instance = origin(SNLInstance::create(design_, model, NLName(instanceName)));
      // Export uses the shared parameterized naja_mem module, whose default
      // dimensions differ from this width-specific canonical model.
      for (const auto& [parameter, value] : std::vector<std::pair<std::string, size_t>>{
          {"WIDTH", memory.signature.width}, {"DEPTH", memory.signature.depth},
          {"ABITS", memory.signature.abits}, {"RD_PORTS", memory.signature.readPorts},
          {"WR_PORTS", memory.signature.writePorts}})
        SNLInstParameter::create(instance, model->getParameter(NLName(parameter)), std::to_string(value));
      if (!memory.initial.empty()) {
        SNLInstParameter::create(instance, model->getParameter(NLName("INIT_ENABLE")), "1");
        SNLInstParameter::create(instance, model->getParameter(NLName("INIT")), memory.initial);
      }
      instance->setTermNet(NLDB0::getMemoryClock(model), memory.clock);
      instance->setTermNet(NLDB0::getMemoryReset(model), constant(false));
      const auto connect = [&](SNLBusTerm* term, const Bits& bits, size_t offset = 0) {
        for (size_t i = 0; i < bits.size(); ++i)
          instance->setTermNet(term->getBit(offset + bits.size() - 1 - i), bits[i]);
      };
      connect(NLDB0::getMemoryWriteAddress(model), memory.write.address);
      connect(NLDB0::getMemoryWriteData(model), memory.write.data);
      connect(NLDB0::getMemoryWriteEnable(model), {memory.write.enable});
      if (memory.signature.independentWriteClocks) {
        SNLInstParameter::create(instance, model->getParameter(NLName("MULTI_CLOCK")), "1");
        connect(model->getBusTerm(NLName("WCLK")), {memory.clock});
        connect(model->getBusTerm(NLName("WMASK")), memory.write.mask);
      }
      if (memory.reads.empty()) {
        connect(NLDB0::getMemoryReadAddress(model), Bits(memory.signature.abits, constant(false)));
      } else for (size_t port = 0; port < memory.reads.size(); ++port) {
        connect(NLDB0::getMemoryReadAddress(model), memory.reads[port].address, port * memory.signature.abits);
        connect(NLDB0::getMemoryReadData(model), memory.reads[port].data, port * memory.signature.width);
      }
      // The declaration's temporary word nets are replaced entirely by RAM.
      std::set<SNLNet*> nets;
      for (auto* bit : objects_.at(name).value.bits) {
        auto* busBit = dynamic_cast<SNLBusNetBit*>(bit);
        nets.insert(busBit ? static_cast<SNLNet*>(busBit->getBus()) : bit);
      }
      for (auto* net : nets) net->destroy();
      objects_.at(name).value.bits.clear();
      pendingSignals_.erase(name);
    }
  }

  vhdl::TypeMark resolveSubtype(const vhdl::TypeMark& type, const SubtypeMap* environment = nullptr) {
    const auto& aliases = environment ? *environment : subtypes_;
    const auto found = aliases.find(key(type.name));
    if (found == aliases.end()) return type;
    if (libraryContext_.precedes(type.name.span.start.offset, found->second.offset))
      fail("subtype used before its declaration: " + key(type.name));
    auto result = found->second.type;
    if (type.constraint) {
      if (result.constraint) {
        const auto name = key(result.name);
        if (name != "integer" && name != "natural" && name != "positive")
          fail("reconstraining a constrained array subtype is unsupported");
        const auto outer = range(*result.constraint), inner = range(*type.constraint);
        outer.position(inner.left);
        outer.position(inner.right);
      }
      result.constraint = type.constraint;
    }
    result.name.span = type.name.span;
    return result;
  }

  void addSubtype(const vhdl::SubtypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto name = key(declaration.name);
    if (strings_.contains(name) || subtypes_.contains(name) || functions_.contains(name) || objects_.contains(name) ||
        integers_.contains(name) || booleans_.contains(name) || arrayDeclarations_.contains(name) ||
        records_.contains(name) || enumerations_.contains(name) || integerTables_.contains(name) ||
        name == "bit" || name == "bit_vector" || name == "boolean" || name == "integer" ||
        name == "natural" || name == "positive" || name == "std_logic" || name == "std_ulogic" ||
        name == "std_logic_vector" || name == "std_ulogic_vector" || name == "signed" || name == "unsigned")
      fail("duplicate or shadowing subtype: " + name);
    const bool alias = subtypes_.contains(key(declaration.type.name));
    auto type = resolveSubtype(declaration.type);
    const auto base = key(type.name);
    const bool integerType = base == "integer" || base == "natural" || base == "positive";
    const bool vectorType = base == "bit_vector" || ((stdLogic_ || alias) &&
        (base == "std_logic_vector" || base == "std_ulogic_vector")) ||
        ((numeric_ || alias) && (base == "signed" || base == "unsigned"));
    if (type.constraint) {
      if (!integerType && !vectorType && !arrayDeclarations_.contains(base))
        fail("unsupported subtype constraint");
      const auto bounds = range(*type.constraint);
      if (integerType && (std::min(bounds.left, bounds.right) <
          (base == "natural" ? 0 : base == "positive" ? 1 : INT32_MIN) ||
          std::max(bounds.left, bounds.right) > INT32_MAX)) fail("subtype bounds violate the base type");
      const auto literal = [&](int64_t value) {
        auto expression = std::make_shared<Expr>();
        expression->kind = Expr::Kind::IntegerLiteral;
        expression->text = std::to_string(value);
        expression->span = declaration.span;
        return expression;
      };
      type.constraint = vhdl::DiscreteRange{bounds.left, bounds.right, bounds.ascending,
          type.constraint->span, literal(bounds.left), literal(bounds.right)};
    }
    if (!integerType && !vectorType && !arrayDeclarations_.contains(base)) shape(type);
    if (retainingIntent()) {
      auto record = intentDeclaration(declaration.name, "subtype");
      record["base_type"] = declaredTypeIntent(declaration.type);
      // Bounds here have already been resolved by addSubtype.
      if (type.constraint && type.constraint->leftExpression && type.constraint->rightExpression)
        record["resolved_constraint"] = IntentDict{{"left", type.constraint->left}, {"right", type.constraint->right},
            {"direction", type.constraint->ascending ? "to" : "downto"}};
      rememberIntent(declaration.name, std::move(record));
    }
    subtypes_.emplace(name, SubtypeBinding{std::move(type), declaration.span.start.offset});
  }

  void addArray(const vhdl::ArrayTypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto name = key(declaration.name);
    if (strings_.contains(name) || subtypes_.contains(name) || functions_.contains(name) || booleans_.contains(name) || (enumerations_.contains(name) || records_.contains(name)) || arrayDeclarations_.contains(name) || integerTables_.contains(name) || integers_.contains(name) || objects_.contains(name) ||
        name == "bit" || name == "bit_vector" || name == "std_logic" ||
        name == "std_logic_vector" || name == "std_ulogic" || name == "std_ulogic_vector" ||
        name == "unsigned" || name == "signed" || name == "integer" ||
        name == "natural" || name == "positive" || name == "boolean") fail("duplicate or shadowing array type: " + name);
    if (declaration.indexSubtype) {
      const auto subtype = key(*declaration.indexSubtype);
      if (subtype != "integer" && subtype != "natural" && subtype != "positive" && !enumerations_.contains(subtype))
        fail("unsupported unconstrained array index subtype: " + subtype);
    }
    arrayDeclarations_.emplace(name, &declaration);
    if (retainingIntent()) intentNominalTypes_[&declaration] = intentDeclaration(declaration.name, "type");
    if (!declaration.indexSubtype) arrayTypeRanges_.emplace(name, arrayBounds(declaration));
    const auto elementName = key(declaration.elementType.name);
    // Integer tables are evaluated as static values, without synthesizing an
    // integer memory or losing signed values through a bit-vector conversion.
    if (elementName == "integer" || elementName == "natural" || elementName == "positive") {
      if ((declaration.indexType && enumerations_.contains(key(*declaration.indexType))) || (declaration.indexSubtype && enumerations_.contains(key(*declaration.indexSubtype))) ||
          (declaration.indexRange.leftExpression && enumIndexType(*declaration.indexRange.leftExpression)))
        fail("enumeration-indexed integer constant tables are not yet supported");
      if (retainingIntent()) {
        auto record = intentDeclaration(declaration.name, "type");
        record["element_type"] = declaredTypeIntent(declaration.elementType);
        record["resolution_status"] = "static_integer_table";
        if (!declaration.indexSubtype) {
          const auto& bounds = arrayTypeRanges_.at(name);
          record["index_range"] = IntentDict{{"left", bounds.left}, {"right", bounds.right},
              {"direction", bounds.ascending ? "to" : "downto"}};
        }
        rememberIntent(declaration.name, std::move(record));
      }
      return;
    }
    auto element = shape(declaration.elementType);
    element.types.insert(element.types.begin(), name);
    element.declarations.insert(element.declarations.begin(), &declaration);
    element.ranges.insert(element.ranges.begin(), arrayBounds(declaration));
    if (!declaration.indexSubtype) element.size();
    arrays_.emplace(name, std::move(element));
    arrayTypeOffsets_.emplace(name, declaration.span.start.offset);
    if (retainingIntent()) {
      auto record = intentDeclaration(declaration.name, "type");
      record["element_type"] = declaredTypeIntent(declaration.elementType);
      if (!declaration.indexSubtype) record["resolved_type"] = shapeIntent(arrays_.at(name));
      else record["resolution_status"] = "unconstrained";
      rememberIntent(declaration.name, std::move(record));
    }
  }

  void addRecord(const vhdl::RecordTypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto name = key(declaration.name);
    if (strings_.contains(name) || subtypes_.contains(name) || functions_.contains(name) || booleans_.contains(name) || (enumerations_.contains(name) || records_.contains(name)) || arrayDeclarations_.contains(name) ||
        objects_.contains(name) || integers_.contains(name) || integerTables_.contains(name) ||
        name == "bit" || name == "bit_vector" || name == "std_logic" || name == "std_ulogic" ||
        name == "std_logic_vector" || name == "std_ulogic_vector" || name == "unsigned" ||
        name == "signed" || name == "integer" || name == "natural" || name == "positive" || name == "boolean")
      fail("duplicate or shadowing record type: " + name);
    if (retainingIntent()) intentNominalTypes_[&declaration] = intentDeclaration(declaration.name, "type");
    Shape record{{name}, {}};
    record.declarations.push_back(&declaration);
    std::set<std::string> names;
    for (const auto& field : declaration.fields) {
      auto type = shape(field.type);
      for (const auto& fieldName : field.names) {
        if (!names.insert(key(fieldName)).second) fail("duplicate record field: " + key(fieldName));
        record.fields.emplace_back(key(fieldName), std::make_shared<Shape>(type));
      }
    }
    record.size();
    if (retainingIntent()) {
      // Field type marks bind at the record definition, even if a later scope
      // shadows those names where the record is used.
      IntentList fields;
      for (const auto& field : declaration.fields) for (const auto& fieldName : field.names)
        fields.emplace_back(IntentDict{{"name", fieldName.spelling}, {"declared_type", declaredTypeIntent(field.type)}});
      intentRecordFields_[&declaration] = std::move(fields);
      auto intent = intentDeclaration(declaration.name, "type");
      intent["resolved_type"] = shapeIntent(record);
      rememberIntent(declaration.name, std::move(intent));
    }
    records_.emplace(name, std::move(record));
    recordOffsets_[name] = declaration.span.start.offset;
  }

  void addEnumeration(const vhdl::EnumerationTypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto available = [&](const std::string& name, bool functionOverload = false) {
      static const std::set<std::string> builtins{"bit", "bit_vector", "std_logic", "std_ulogic",
          "std_logic_vector", "std_ulogic_vector", "integer", "natural", "positive", "boolean", "true", "false", "unsigned", "signed"};
      return !builtins.contains(name) && !objects_.contains(name) && !integers_.contains(name) &&
          !booleans_.contains(name) && (functionOverload || !functions_.contains(name)) && !records_.contains(name) &&
          !arrayDeclarations_.contains(name) && !integerTables_.contains(name) && !enumerations_.contains(name) && !subtypes_.contains(name);
    };
    const auto name = key(declaration.name);
    if (!available(name)) fail("duplicate or shadowing enumeration type: " + name);
    if (declaration.literals.empty() || declaration.literals.size() > 4096)
      fail("enumeration must contain 1 to 4096 identifier literals");
    enumerations_[name] = &declaration;
    if (retainingIntent()) {
      intentNominalTypes_[&declaration] = intentDeclaration(declaration.name, "type");
      auto record = intentDeclaration(declaration.name, "type");
      record["resolved_type"] = shapeIntent(Shape{{name}, {}, 0, {}, &declaration});
      rememberIntent(declaration.name, std::move(record));
    }
    Shape type{{name}, {}, 0, {}, &declaration};
    std::set<std::string> localLiterals;
    for (size_t i = 0; i < declaration.literals.size(); ++i) {
      const auto literal = key(declaration.literals[i]);
      if ((!available(literal, true) && !enumLiterals_.contains(literal)) || !localLiterals.insert(literal).second)
        fail("duplicate or shadowing enumeration literal: " + literal);
      const auto value = number(i, type);
      enumLiterals_[literal].push_back(value);
      objects_.emplace(literal, Object{value, false, false, false, true});
      if (retainingIntent()) {
        auto record = intentDeclaration(declaration.literals[i], "enumeration_literal");
        record["effective_value"] = int64_t(i);
        record["type_declaration"] = intentDeclaration(declaration.name, "type");
        rememberIntent(declaration.literals[i], std::move(record));
      }
    }
  }

  void declarations(const std::vector<vhdl::ArrayTypeDeclaration>& arrays,
                    const std::vector<vhdl::ConstantDeclaration>& constants,
                    const std::vector<vhdl::RecordTypeDeclaration>& records,
                    const std::vector<vhdl::SignalDeclaration>& signals = {},
                    const std::vector<vhdl::EnumerationTypeDeclaration>& enumerations = {},
                    const std::vector<vhdl::FunctionDeclaration>& functions = {},
                    const std::vector<vhdl::SubtypeDeclaration>& subtypes = {}, LocalBindings* bindings = nullptr,
                    const std::vector<vhdl::AliasDeclaration>& aliases = {}) {
    std::map<size_t, std::function<void()>> ordered;
    for (const auto& alias : aliases)
      ordered.emplace(alias.span.start.offset, [&alias, this, bindings] {
        DiagnosticScope location(alias.span);
        auto selected = selection(*alias.target);
        materializeSignal(selected.name);
        auto object = objects_.at(selected.name);
        auto value = readSelected(*alias.target, State{});
        if (alias.type) {
          const auto type = shape(*alias.type);
          if (!compatible(type, value.shape)) fail("alias subtype does not match its object");
          value.shape = type;
          selected.shape = type;
        }
        const auto name = key(alias.name);
        if (bindings) bindings->declare(name);
        if (objects_.contains(name) || aliases_.contains(name)) fail("duplicate alias: " + name);
        object.value = std::move(value);
        objects_.emplace(name, std::move(object));
        aliases_.emplace(name, std::move(selected));
      });
    for (const auto& array : arrays)
      ordered.emplace(array.span.start.offset, [&array, this, bindings] { addArray(array); if (bindings) bindings->names.insert(key(array.name)); });
    for (const auto& constant : constants)
      ordered.emplace(constant.object.span.start.offset, [&constant, this, bindings] {
        addConstant(constant);
        if (bindings) for (const auto& name : constant.object.names) bindings->names.insert(key(name));
      });
    for (const auto& record : records)
      ordered.emplace(record.span.start.offset, [&record, this, bindings] { addRecord(record); if (bindings) bindings->names.insert(key(record.name)); });
    for (const auto& signal : signals)
      ordered.emplace(signal.span.start.offset, [&signal, this, bindings] {
        const auto type = shape(signal.type);
        const auto initial = signal.initializer ? std::optional<Value>{this->initializer(*signal.initializer, type)} : std::nullopt;
        if (bindings) for (const auto& name : signal.names) bindings->declare(key(name));
        addSignal(signal, &type, initial ? &*initial : nullptr);
      });
    for (const auto& enumeration : enumerations)
      ordered.emplace(enumeration.span.start.offset, [&enumeration, this, bindings] {
        addEnumeration(enumeration);
        if (bindings) {
          bindings->names.insert(key(enumeration.name));
          for (const auto& literal : enumeration.literals) bindings->names.insert(key(literal));
        }
      });
    for (const auto& function : functions)
      ordered.emplace(function.span.start.offset, [&function, this, bindings] { addLocalFunction(function); if (bindings) bindings->names.insert(key(function.name)); });
    for (const auto& subtype : subtypes)
      ordered.emplace(subtype.span.start.offset, [&subtype, this, bindings] { addSubtype(subtype); if (bindings) bindings->names.insert(key(subtype.name)); });
    for (const auto& [offset, add] : ordered) add();
  }

  void checkInteger(int64_t value, const vhdl::TypeMark& original) {
    const auto type = resolveSubtype(original);
    const auto name = key(type.name);
    if (value < INT32_MIN || value > INT32_MAX || (name == "natural" && value < 0) ||
        (name == "positive" && value < 1)) fail("integer constant violates its subtype");
    if (type.constraint) range(*type.constraint).position(value);
  }

  Range constantBounds(const vhdl::ArrayTypeDeclaration& array,
                       const vhdl::TypeMark& type, const Expr& value) {
    if (!array.indexSubtype) {
      if (type.constraint) fail("reconstraining an array type is not supported");
      return arrayBounds(array);
    }
    Range bounds;
    const auto subtype = key(*array.indexSubtype);
    if (const auto found = enumerations_.find(subtype); found != enumerations_.end()) {
      if (!type.constraint) fail("unconstrained enumeration-indexed arrays require explicit bounds");
      return enumRange(*type.constraint, found->second);
    }
    const int64_t lower = subtype == "natural" ? 0 : subtype == "positive" ? 1 : INT32_MIN;
    if (type.constraint) bounds = range(*type.constraint);
    else {
      if (value.kind != Expr::Kind::Aggregate || value.elements.empty() ||
          std::any_of(value.elements.begin(), value.elements.end(),
              [](const auto& item) { return item->kind == Expr::Kind::Association; }))
        fail("unconstrained array constant requires a positional aggregate or explicit bounds");
      bounds = {lower, lower + int64_t(value.elements.size()) - 1, true};
    }
    if (bounds.left < lower || bounds.right < lower || bounds.left > INT32_MAX || bounds.right > INT32_MAX)
      fail("array bounds violate the index subtype");
    return bounds;
  }

  std::vector<const Expr*> arrayAggregateElements(const Expr& aggregate, const Range& bounds) {
    std::vector<const Expr*> actuals(bounds.size(), nullptr);
    if (aggregate.kind == Expr::Kind::Others) {
      std::fill(actuals.begin(), actuals.end(), aggregate.left.get());
      return actuals;
    }
    if (aggregate.kind != Expr::Kind::Aggregate) fail("array initializer requires an aggregate");
    size_t position = 0;
    bool named = false, others = false;
    const auto place = [&](size_t destination, const Expr* value) {
      if (destination >= actuals.size()) fail("array aggregate has too many positional elements");
      if (actuals[destination]) fail("overlapping array aggregate choices");
      actuals[destination] = value;
    };
    for (const auto& item : aggregate.elements) {
      DiagnosticScope location(item->span);
      if (others) fail("others must be the final array aggregate association");
      if (item->kind != Expr::Kind::Association) {
        if (named) fail("positional array element follows named association");
        place(position++, item.get());
        continue;
      }
      named = true;
      const auto& choice = *item->left;
      if (choice.kind == Expr::Kind::Name && key(choice) == "others") {
        others = true;
        for (auto& actual : actuals) if (!actual) actual = item->right.get();
        continue;
      }
      if (position) fail("named and positional associations cannot be mixed in array aggregates");
      const auto apply = [&](const Expr& index) {
        if (index.kind == Expr::Kind::Range || arrayAttribute(index)) {
          Range selected = arrayAttribute(index) ? attributeRange(index) :
              Range{arrayIndex(*index.left, bounds), arrayIndex(*index.right, bounds), index.text == "to"};
          bounds.position(selected.left);
          bounds.position(selected.right);
          for (size_t i = 0; i < selected.size(); ++i)
            place(bounds.position(selected.ascending ? selected.left + i : selected.left - i), item->right.get());
        } else place(bounds.position(arrayIndex(index, bounds)), item->right.get());
      };
      if (choice.kind == Expr::Kind::Aggregate)
        for (const auto& index : choice.elements) apply(*index);
      else apply(choice);
    }
    if (std::find(actuals.begin(), actuals.end(), nullptr) != actuals.end())
      fail("array aggregate does not cover every element");
    return actuals;
  }

  bool locallyStaticPortIndex(const Expr& expr) const {
    if (expr.kind == Expr::Kind::IntegerLiteral) return true;
    if (expr.kind == Expr::Kind::Name) {
      const auto found = objects_.find(key(expr));
      return locallyStaticIntegers_.contains(key(expr)) ||
          (found != objects_.end() && found->second.constant && found->second.value.shape.enumeration &&
           found->second.value.shape.ranges.empty());
    }
    return (expr.kind == Expr::Kind::Unary || expr.kind == Expr::Kind::Binary ||
            expr.kind == Expr::Kind::Range) && expr.left &&
        locallyStaticPortIndex(*expr.left) && (!expr.right || locallyStaticPortIndex(*expr.right));
  }

  void addConstant(const vhdl::ConstantDeclaration& declaration) {
    struct Capture {
      RTLConstructor& owner;
      const vhdl::ConstantDeclaration& declaration;
      int exceptions = std::uncaught_exceptions();
      ~Capture() noexcept(false) { if (std::uncaught_exceptions() == exceptions) owner.constantIntent(declaration); }
    } capture{*this, declaration};
    InitializerScope initializerScope(initializerContext_);
    const auto type = resolveSubtype(declaration.object.type);
    const auto name = key(type.name);
    std::set<std::string> names;
    for (const auto& id : declaration.object.names)
      if (strings_.contains(key(id)) || subtypes_.contains(key(id)) || functions_.contains(key(id)) || booleans_.contains(key(id)) || !names.insert(key(id)).second || objects_.contains(key(id)) || integers_.contains(key(id)) ||
          integerTables_.contains(key(id)) || (enumerations_.contains(key(id)) || records_.contains(key(id))) || arrayDeclarations_.contains(key(id)))
        fail("duplicate constant: " + key(id));
    if (name == "string") {
      // String constants are synthesis diagnostics metadata; retain no hardware value.
      return;
    }
    if (name == "boolean") {
      const auto value = scalar(*declaration.value);
      checkStaticType(value, type, nullptr);
      for (const auto& id : declaration.object.names) booleans_.emplace(key(id), value.value);
      return;
    }
    if (name == "integer" || name == "natural" || name == "positive") {
      const auto value = integer(*declaration.value);
      checkInteger(value, type);
      const bool local = locallyStaticPortIndex(*declaration.value) &&
          (!type.constraint ||
           ((!type.constraint->leftExpression || locallyStaticPortIndex(*type.constraint->leftExpression)) &&
            (!type.constraint->rightExpression || locallyStaticPortIndex(*type.constraint->rightExpression))));
      for (const auto& id : declaration.object.names) {
        integers_.emplace(key(id), value);
        if (local) locallyStaticIntegers_.insert(key(id));
      }
      return;
    }
    auto constantType = type;
    if (arrayDeclarations_.contains(name)) {
      const auto& array = *arrayDeclarations_.at(name);
      if (libraryContext_.precedes(type.name.span.start.offset, array.span.start.offset)) fail("array type used before declaration");
      const auto bounds = constantBounds(array, type, *declaration.value);
      if (!bounds.size()) fail("null array constant is unsupported");
      const auto element = key(array.elementType.name);
      if (element == "integer" || element == "natural" || element == "positive") {
        IntegerTable table{bounds, {}, &array};
        for (const auto* item : arrayAggregateElements(*declaration.value, bounds)) {
          const auto value = integer(*item);
          checkInteger(value, array.elementType);
          table.values.push_back(value);
        }
        for (const auto& id : declaration.object.names) integerTables_.emplace(key(id), table);
        return;
      }
      if (array.indexSubtype && !bounds.indexEnumeration) constantType.constraint = vhdl::DiscreteRange{
          bounds.left, bounds.right, bounds.ascending, {}, {}, {}};
    }
    const auto target = shape(constantType);
    const auto value = expression(*declaration.value, State{}, &target);
    for (auto* bit : value.bits) if (!bit->isConstant()) fail("constant initializer must be static");
    for (const auto& id : declaration.object.names)
      objects_.emplace(key(id), Object{value, false, false, false, true});
  }

  void checkLabels(const std::vector<vhdl::GenerateStatement>& generates,
                   const std::vector<vhdl::EntityInstantiation>& instances) {
    std::set<std::string> labels;
    for (const auto& instance : instances)
      if (!labels.insert(key(instance.label)).second) fail("duplicate concurrent label");
    for (const auto& generate : generates) {
      if (!generate.label.spelling.empty() && !labels.insert(key(generate.label)).second)
        fail("duplicate concurrent label");
      checkLabels(generate.generates, generate.instantiations);
      for (const auto& branch : generate.alternatives)
        checkLabels(branch.generates, branch.instantiations);
    }
  }

  void lowerGenerate(const vhdl::GenerateStatement& generate, const State& state,
                     const std::string& prefix = "") {
    DiagnosticScope location(generate.conditional ? generate.label.span : generate.iterator.span);
    const auto body = [&](const vhdl::GenerateStatement& branch, const std::string& scope) {
      if (++steps_ > 100000) fail("static generate elaboration limit exceeded");
      const auto outerPrefix = objectPrefix_;
      objectPrefix_ = scope;
      declarations(branch.arrayTypes, branch.constants, branch.recordTypes, branch.signals, branch.enumerationTypes, branch.functions, branch.subtypes);
      std::set<std::string> localSignals;
      for (const auto& signal : branch.signals) for (const auto& name : signal.names) localSignals.insert(key(name));
      inferMemories(&branch, &localSignals);
      for (const auto& assignment : branch.assignments) concurrent(assignment, state);
      for (const auto& child : branch.generates) lowerGenerate(child, state, scope);
      for (const auto& instance : branch.instantiations) lowerInstance(instance, scope);
      for (const auto& process : branch.processes) lowerProcess(process);
      finalizeMemories(&localSignals);
      for (const auto& name : localSignals) memories_.erase(name);
      // Bindings are local to this elaborated body; hardware and driver tracking
      // remain live after the scope ends, including reads through outer signals.
      for (const auto& signal : branch.signals) for (const auto& name : signal.names) {
        generatedObjects_.emplace_back(scope + name.spelling, objects_.at(key(name)));
        objects_.erase(key(name));
      }
      for (const auto& constant : branch.constants) for (const auto& name : constant.object.names) {
        objects_.erase(key(name));
        integers_.erase(key(name));
        locallyStaticIntegers_.erase(key(name));
        booleans_.erase(key(name));
        integerTables_.erase(key(name));
      }
      for (const auto& array : branch.arrayTypes) {
        arrays_.erase(key(array.name));
        arrayDeclarations_.erase(key(array.name));
        arrayTypeOffsets_.erase(key(array.name));
        arrayTypeRanges_.erase(key(array.name));
      }
      for (const auto& record : branch.recordTypes) {
        records_.erase(key(record.name));
        recordOffsets_.erase(key(record.name));
      }
      for (const auto& subtype : branch.subtypes) subtypes_.erase(key(subtype.name));
      for (const auto& function : branch.functions) {
        functions_.erase(key(function.name));
        localFunctionEnvironments_.erase(&function);
      }
      for (const auto& enumeration : branch.enumerationTypes) {
        for (const auto& literal : enumeration.literals) {
          const auto id = key(literal);
          auto& values = enumLiterals_.at(id);
          std::erase_if(values, [&](const auto& value) { return value.shape.enumeration == &enumeration; });
          if (values.empty()) { enumLiterals_.erase(id); objects_.erase(id); }
          else objects_.at(id).value = values.front();
        }
        enumerations_.erase(key(enumeration.name));
      }
      objectPrefix_ = outerPrefix;
    };
    if (generate.conditional) {
      const auto selected = [&](const vhdl::GenerateStatement& branch) {
        if (!branch.condition) return true;
        const auto value = scalar(*branch.condition, nullptr);
        if (!value.boolean) fail("if generate condition must be a static boolean");
        return value.value != 0;
      };
      const auto scope = prefix + generate.label.spelling + ".";
      if (selected(generate)) body(generate, scope);
      else for (const auto& branch : generate.alternatives) {
        if (selected(branch)) { body(branch, scope); break; }
      }
      return;
    }
    auto name = key(generate.iterator);
    if (integers_.contains(name) || objects_.contains(name) || integerTables_.contains(name) ||
        (enumerations_.contains(name) || records_.contains(name)) || arrayDeclarations_.contains(name)) fail("shadowed generate parameter");
    const auto bounds = range(generate.range);
    for (size_t i = 0; i < bounds.size(); ++i) {
      integers_[name] = bounds.ascending ? bounds.left + i : bounds.left - i;
      const auto scope = prefix + (generate.label.spelling.empty() ? name : generate.label.spelling) +
          "[" + std::to_string(integers_[name]) + "].";
      body(generate, scope);
    }
    integers_.erase(name);
  }

  void lowerInstance(const vhdl::EntityInstantiation& instance, const std::string& prefix = "") {
    DiagnosticScope location(instance.span);
    if (instance.architecture)
      fail("unsupported RTL entity binding");
    if (!libraries_.contains(key(instance.library))) fail("library is not visible: " + key(instance.library));
    auto* targetLibrary = libraryContext_.resolve(work_, key(instance.library));
    const vhdl::EntityDeclaration* entity = nullptr;
    for (const auto& candidate : syntax_.entities)
      if (libraryContext_.owner(candidate.span.start.offset) == targetLibrary &&
          key(candidate.name) == key(instance.entity)) entity = &candidate;
    if (!entity) fail("missing entity: " + key(instance.entity));
    if (instance.component && !components_.contains(key(instance.entity)))
      fail("component is not visible: " + key(instance.entity));
    const auto& interface = instance.component ? *components_.at(key(instance.entity)) : *entity;
    std::vector<std::string> genericNames;
    for (const auto& generic : interface.generics)
      for (const auto& name : generic.names) genericNames.push_back(key(name));
    GenericValues overrides;
    std::map<std::string, const Expr*> actuals;
    bool named = false;
    for (size_t i = 0; i < instance.generics.size(); ++i) {
      const auto& association = instance.generics[i];
      if (i >= genericNames.size()) fail("too many generic actuals");
      if (!association.formal && named) fail("positional generic follows named generic");
      named |= association.formal.has_value();
      const auto name = association.formal ? key(*association.formal) : genericNames[i];
      if (std::find(genericNames.begin(), genericNames.end(), name) == genericNames.end() ||
          !actuals.emplace(name, association.actual.get()).second) fail("invalid generic association");
      overrides.emplace(name, GenericValue{});
    }
    RTLConstructor childScope(design_, *entity, architecture_, syntax_, libraryContext_, overrides, build_);
    childScope.genericActual_ = [&](const std::string& name, const Shape* expected) {
      return genericValue(*actuals.at(name), expected);
    };
    childScope.prepareEntity();
    overrides = childScope.genericOverrides_;
    const auto parentObjects = objects_;
    const auto parentIntegers = integers_;
    const auto parentBooleans = booleans_;
    const auto parentStrings = strings_;
    for (const auto& generic : entity->generics)
      for (const auto& name : generic.names) {
        const auto id = key(name);
        integers_.erase(id);
        booleans_.erase(id);
        strings_.erase(id);
        objects_.erase(id);
        if (childScope.objects_.contains(id)) objects_[id] = childScope.objects_.at(id);
        else if (childScope.booleans_.contains(id)) booleans_[id] = childScope.booleans_.at(id);
        else if (childScope.strings_.contains(id)) strings_[id] = childScope.strings_.at(id);
        else integers_[id] = childScope.integers_.at(id);
      }
    std::map<std::string, Shape> formalShapes;
    for (const auto& port : entity->ports)
      for (const auto& name : port.names) formalShapes.emplace(key(name), childScope.shape(port.type));
    if (instance.component) {
      const auto& component = *components_.at(key(instance.entity));
      std::map<std::string, vhdl::PortMode> modes;
      for (const auto& port : entity->ports)
        for (const auto& name : port.names) modes.emplace(key(name), port.mode);
      std::set<std::string> declaredPorts;
      for (const auto& port : component.ports) for (const auto& name : port.names) {
        const auto id = key(name);
        const auto declaredShape = shape(port.type);
        if (!declaredPorts.insert(id).second || !modes.contains(id) || modes.at(id) != port.mode ||
            !compatible(declaredShape, formalShapes.at(id))) fail("component port interface mismatch");
        // Formal selections name the component interface; binding to the entity
        // preserves position even when their index bounds differ.
        formalShapes.at(id) = declaredShape;
      }
      if (declaredPorts.size() != modes.size() || component.generics.size() != entity->generics.size())
        fail("component declaration does not match entity interface");
      for (size_t i = 0; i < component.generics.size(); ++i) {
        const auto& declared = component.generics[i];
        const auto& actual = entity->generics[i];
        if (declared.names.size() != actual.names.size() || key(declared.type.name) != key(actual.type.name))
          fail("component generic interface mismatch");
        for (size_t j = 0; j < declared.names.size(); ++j)
          if (key(declared.names[j]) != key(actual.names[j])) fail("component generic name mismatch");
      }
    }
    objects_ = parentObjects;
    integers_ = parentIntegers;
    booleans_ = parentBooleans;
    strings_ = parentStrings;
    auto* model = build_(targetLibrary, key(instance.entity), overrides);
    if (const auto* property = dynamic_cast<RTLSourceProperty*>(model->getProperty("VHDLRTLSource")))
      for (const auto& [path, content] : property->initializationFiles) {
        const auto [dependency, inserted] = initializationFiles_->emplace(path, content);
        if (!inserted && dependency->second != content) fail("initialization file changed during elaboration: " + path);
      }
    std::vector<SNLTerm*> ports;
    for (const auto& declaration : interface.ports) for (const auto& name : declaration.names) {
      SNLTerm* matching = nullptr;
      for (auto* term : model->getTerms()) {
        std::string id = term->getName().getString();
        for (auto& c : id) c = std::tolower(static_cast<unsigned char>(c));
        if (id == key(name)) matching = term;
      }
      if (!matching) fail("missing bound port");
      ports.push_back(matching);
    }
    const auto instanceName = prefix + instance.label.spelling;
    if (!instanceNames_.insert(prefix + key(instance.label)).second) fail("duplicate instance label");
    auto* child = declaration(SNLInstance::create(design_, model, NLName(instanceName)), instance.span);
    if (retainingIntent()) {
      if (const auto* modelIntent = VHDLIntent::get(model)) {
        auto record = std::get<IntentDict>(modelIntent->value);
        record["instance"] = intentDeclaration(instance.label, "instance");
        record["context"] = design_->getName().getString() + "/" + instanceName;
        record["model_context"] = model->getName().getString();
        IntentList actuals;
        for (const auto& association : instance.generics)
          actuals.emplace_back(IntentDict{{"formal", association.formal ? association.formal->spelling : std::string{}},
              {"expression", expressionIntent(association.actual.get())}});
        record["generic_actuals"] = std::move(actuals);
        VHDLIntent::attach(child, libraryContext_.intentSession, std::move(record));
      }
    }
    std::map<SNLTerm*, std::vector<bool>> bound;
    SNLTerm* previousPort = nullptr;
    named = false;
    for (size_t i = 0; i < instance.actuals.size(); ++i) {
      auto* port = i < ports.size() ? ports[i] : nullptr;
      if (!instance.formals.empty() && instance.formals[i]) {
        named = true;
        port = nullptr;
        for (auto* candidate : ports) {
          std::string name = candidate->getName().getString();
          for (auto& c : name) c = std::tolower(static_cast<unsigned char>(c));
          if (name == key(*instance.formals[i])) port = candidate;
        }
      } else if (named) fail("positional port follows named port");
      if (!port) fail("invalid port association");
      if (bound.contains(port) && previousPort != port)
        fail("individual port associations must be consecutive");
      previousPort = port;
      std::string literalFormal = port->getName().getString();
      for (auto& c : literalFormal) c = std::tolower(static_cast<unsigned char>(c));
      Selection formal{literalFormal, formalShapes.at(literalFormal), 0};
      if (!instance.formalIndices.empty()) {
        for (const auto& indexExpr : instance.formalIndices[i]) {
          DiagnosticScope location(indexExpr->span);
          if (indexExpr->kind != Expr::Kind::Selected && !locallyStaticPortIndex(*indexExpr))
            fail("port formal index requires a locally static integer expression; "
                 "only literals, integer constants, and predefined arithmetic are supported");
          index(formal, *indexExpr);
        }
      }
      auto& covered = bound.try_emplace(port, port->getWidth(), false).first->second;
      std::vector<SNLBitTerm*> formalBits;
      size_t position = 0;
      for (auto* term : port->getBits()) {
        if (position >= formal.offset && position < formal.offset + formal.shape.size()) {
          if (covered[position]) fail("overlapping port associations");
          covered[position] = true;
          formalBits.push_back(term);
        }
        ++position;
      }
      if (formalBits.size() != formal.shape.size()) fail("port selection exceeds flattened port width");
      if (!instance.actualOpen.empty() && instance.actualOpen[i]) {
        DiagnosticScope location(instance.actuals[i].span);
        if (!instance.formalIndices.empty() && !instance.formalIndices[i].empty())
          fail("open cannot be associated with an individual port subelement or slice");
        if (port->getDirection() != SNLTerm::Direction::Output)
          fail("open input ports require defaults, which are not yet supported");
        // As in the SV loader, an explicit empty connection leaves inst terms unconnected.
        continue;
      }
      const auto actualName = key(instance.actuals[i]);
      const Expr* actualExpression = !instance.actualLiterals.empty() ? instance.actualLiterals[i].get() : nullptr;
      Expr namedExpression;
      if (!actualExpression && (enumLiterals_.contains(actualName) || functions_.contains(actualName) ||
          actualName == "true" || actualName == "false")) {
        namedExpression.kind = Expr::Kind::Name;
        namedExpression.text = instance.actuals[i].spelling;
        namedExpression.canonical = actualName;
        namedExpression.span = instance.actuals[i].span;
        actualExpression = &namedExpression;
      }
      if (actualExpression && port->getDirection() == SNLTerm::Direction::Input) {
        const auto& expected = formal.shape;
        const auto value = expression(*actualExpression, State{}, &expected);
        if (value.bits.size() != formalBits.size() || !compatible(value.shape, expected))
          fail("port type or width mismatch");
        size_t bit = 0;
        for (auto* term : formalBits) child->getInstTerm(term)->setNet(value.bits[bit++]);
        continue;
      }
      if (instance.actuals[i].spelling.empty()) fail("literal port actual requires an input port");
      auto selected = selection(key(instance.actuals[i]));
      if (!instance.actualIndices.empty())
        for (const auto& indexExpr : instance.actualIndices[i]) index(selected, *indexExpr);
      const auto& object = objects_.at(selected.name);
      if (selected.shape.size() != formalBits.size() || !compatible(selected.shape, formal.shape))
        fail("port type or width mismatch");
      size_t bit = selected.offset;
      for (auto* term : formalBits) {
        auto* net = object.value.bits[bit++];
        if (port->getDirection() == SNLTerm::Direction::Output) {
          if (object.input || object.constant || !drivers_.insert(net).second)
            fail("invalid component output driver");
        } else reads_.insert(net);
        child->getInstTerm(term)->setNet(net);
      }
    }
    size_t portIndex = 0;
    for (const auto& declaration : interface.ports) {
      for (const auto& name : declaration.names) {
        const auto id = key(name);
        auto* port = ports.at(portIndex++);
        const bool complete = bound.contains(port) &&
            std::find(bound.at(port).begin(), bound.at(port).end(), false) == bound.at(port).end();
        if (complete) continue;
        if (port->getDirection() == SNLTerm::Direction::Output && !bound.contains(port)) continue;
        const auto* defaultValue = declaration.defaultValue.get();
        if (!defaultValue) fail("incomplete port association: " + port->getName().getString());
        if (port->getDirection() != SNLTerm::Direction::Input) // LCOV_EXCL_START: run() rejects non-input defaults while building the child.
          fail("port defaults are supported only for input ports");
        // LCOV_EXCL_STOP
        const auto expected = formalShapes.at(id);
        const auto value = expression(*defaultValue, State{}, &expected);
        if (value.bits.size() != port->getWidth() || !compatible(value.shape, expected))
          fail("port default type or width mismatch");
        size_t bit = 0;
        for (auto* term : port->getBits()) child->getInstTerm(term)->setNet(value.bits[bit++]);
        bound[port].assign(port->getWidth(), true);
      }
    }
  }

  void context(const vhdl::ContextClause& clauses) {
    for (const auto& library : clauses.libraries)
      for (const auto& name : library.names) libraries_.insert(key(name));
    for (const auto& use : clauses.uses) {
      DiagnosticScope location(use.span);
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) != "ieee" &&
          key(use.selectedName[0]) != "std" &&
          key(use.selectedName[2]) == "all") {
        const auto name = key(use.selectedName[1]);
        const auto libraryName = key(use.selectedName[0]);
        if (!libraries_.contains(libraryName)) fail("library is not visible: " + libraryName);
        auto* targetLibrary = libraryContext_.resolve(work_, libraryName);
        if (!imported_.insert({targetLibrary, name}).second) continue;
        const vhdl::PackageDeclaration* package = nullptr;
        for (const auto& candidate : syntax_.packages)
          if (!candidate.body && key(candidate.name) == name &&
              libraryContext_.owner(candidate.name.span.start.offset) == targetLibrary) package = &candidate;
        if (!package) fail("missing package: " + name);
        const auto callerLibraries = libraries_;
        const bool callerLogic = stdLogic_, callerUnsigned = unsigned_, callerArith = arith_, callerNumeric = numeric_, callerSigned = signed_, callerTextio = textio_;
        auto* callerWork = work_;
        work_ = targetLibrary;
        libraries_ = {"std", "work"};
        context(package->context);
        for (const auto& function : package->functions) {
          const auto name = key(function.name);
          if (integers_.contains(name) || booleans_.contains(name) ||
              (objects_.contains(name) && !enumLiterals_.contains(name)) ||
              !functions_.emplace(name, FunctionBinding{package, &function}).second)
            fail("ambiguous or overloaded package function: " + name);
        }
        declarations(package->arrayTypes, package->constants, package->recordTypes, {}, package->enumerationTypes, {}, package->subtypes);
        for (const auto& component : package->components)
          if (!components_.emplace(key(component.name), &component).second) fail("ambiguous component");
        work_ = callerWork;
        libraries_ = callerLibraries;
        stdLogic_ = callerLogic;
        unsigned_ = callerUnsigned;
        arith_ = callerArith;
        numeric_ = callerNumeric;
        signed_ = callerSigned; textio_ = callerTextio;
        continue;
      }
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) == "std" &&
          key(use.selectedName[1]) == "standard" && key(use.selectedName[2]) == "all") continue;
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) == "std" &&
          key(use.selectedName[1]) == "textio" && key(use.selectedName[2]) == "all") textio_ = true;
      // These standard packages are commonly imported by RTL that also has
      // simulation branches. Visibility alone requires no hardware; live file
      // operations or calls still need an explicitly supported implementation.
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) == "std" &&
          ((key(use.selectedName[1]) == "textio" && key(use.selectedName[2]) == "all") ||
           (key(use.selectedName[1]) == "env" && (key(use.selectedName[2]) == "all" ||
            key(use.selectedName[2]) == "stop" || key(use.selectedName[2]) == "finish")))) continue;
      if (use.selectedName.size() != 3 || key(use.selectedName[0]) != "ieee" ||
          key(use.selectedName[2]) != "all" || !libraries_.contains("ieee"))
        fail("unsupported RTL package visibility");
      const auto package = key(use.selectedName[1]);
      if (package == "std_logic_1164") stdLogic_ = true;
      else if (package == "std_logic_unsigned") unsigned_ = true;
      else if (package == "std_logic_signed") signed_ = true;
      else if (package == "std_logic_arith") arith_ = true;
      else if (package == "numeric_std") numeric_ = true;
      else if (package == "math_real") {
        // An unused import is legal; real-valued operations remain diagnosed.
      } else fail("unsupported IEEE package: " + package);
    }
  }

  using FunctionBinding = std::pair<const vhdl::PackageDeclaration*, const vhdl::FunctionDeclaration*>;
  struct IntegerTable {
    Range bounds;
    std::vector<int64_t> values;
    const vhdl::ArrayTypeDeclaration* type = nullptr;
  };
  struct FunctionEnvironment {
    std::map<std::string, StaticScalar> values;
    SubtypeMap subtypes;
    std::map<std::string, Shape> records, arrayShapes;
    std::map<std::string, size_t> recordOffsets, arrayOffsets;
    std::map<std::string, Range> arrayRanges;
    std::map<std::string, const vhdl::EnumerationTypeDeclaration*> enumerations;
    std::map<std::string, FunctionBinding> functions;
    std::map<std::string, Object> constants;
    std::map<std::string, std::vector<Value>> enumLiterals;
    std::map<std::string, IntegerTable> arrays;
    std::map<std::string, const vhdl::ArrayTypeDeclaration*> arrayTypes;
    bool logic = false, numeric = false, arith = false, unsignedLogic = false, signedLogic = false;
    bool textio = false;
    std::map<std::string, std::string> strings;
  };
  struct StaticScope {
    const FunctionEnvironment* environment = nullptr;
    const vhdl::PackageDeclaration* package = nullptr;
    std::map<std::string, StaticScalar> values;
    std::map<std::string, const vhdl::TypeMark*> variables;
    std::map<std::string, IntegerTable> arrays;
  };

  const IntegerTable* staticArray(const std::string& name, const StaticScope* scope) const {
    if (scope) {
      if (scope->values.contains(name)) return nullptr;
      if (const auto found = scope->arrays.find(name); found != scope->arrays.end()) return &found->second;
      if (scope->environment) {
        const auto found = scope->environment->arrays.find(name);
        if (found != scope->environment->arrays.end()) return &found->second;
      }
    } else {
      const auto found = integerTables_.find(name);
      if (found != integerTables_.end()) return &found->second;
    }
    return nullptr;
  }

  const vhdl::ArrayTypeDeclaration* staticArrayType(const std::string& name, const StaticScope& scope) const {
    if (scope.environment) {
      const auto found = scope.environment->arrayTypes.find(name);
      if (found != scope.environment->arrayTypes.end()) return found->second;
    }
    if (scope.package) for (const auto& type : scope.package->arrayTypes)
      if (key(type.name) == name) return &type;
    return nullptr;
  }

  Range staticRange(const vhdl::DiscreteRange& range, StaticScope* scope) {
    if (range.subtype) {
      const auto type = resolveSubtype(vhdl::TypeMark{*range.subtype, std::nullopt},
          scope && scope->environment ? &scope->environment->subtypes : nullptr);
      const auto name = key(type.name);
      if (!type.constraint || (name != "integer" && name != "natural" && name != "positive"))
        fail("range requires a constrained integer subtype");
      return staticRange(*type.constraint, scope);
    }
    if (range.attribute) return attributeRange(*range.attribute, scope);
    return {staticInteger(*range.leftExpression, scope), staticInteger(*range.rightExpression, scope), range.ascending};
  }

  IntegerTable staticArrayActual(const Expr& expression, const vhdl::TypeMark& formal, StaticScope& declarationScope,
                                StaticScope* caller) {
    const auto* type = staticArrayType(key(formal.name), declarationScope);
    if (!type || !scalarFunctionType(type->elementType) || key(type->elementType.name) == "boolean")
      fail("static array parameters require a one-dimensional integer array type");
    if (libraryContext_.precedes(formal.name.span.start.offset, type->span.start.offset))
      fail("array type is used before its declaration");
    if (expression.kind != Expr::Kind::Name) fail("static array actual must name an integer array constant or parameter");
    const auto* actual = staticArray(key(expression), caller);
    if (!actual || actual->type != type) fail("static array argument type mismatch");
    if (actual->bounds.indexEnumeration) fail("enumeration-indexed integer array parameters are unsupported");
    IntegerTable value = *actual;
    if (formal.constraint || !type->indexSubtype) {
      if (formal.constraint && !type->indexSubtype) fail("reconstraining an array type is not supported");
      if (type->indexType) fail("enumeration-indexed integer array parameters are unsupported");
      const auto bounds = staticRange(formal.constraint ? *formal.constraint : type->indexRange, &declarationScope);
      if (bounds.size() != value.values.size()) fail("static array argument length mismatch");
      value.bounds = bounds;
    }
    const auto indexSubtype = type->indexSubtype ? key(*type->indexSubtype) : "integer";
    const int64_t lower = indexSubtype == "natural" ? 0 : indexSubtype == "positive" ? 1 : INT32_MIN;
    if (value.bounds.left < lower || value.bounds.right < lower ||
        value.bounds.left > INT32_MAX || value.bounds.right > INT32_MAX)
      fail("array bounds violate the index subtype");
    for (const auto element : value.values) checkStaticType({element, false}, type->elementType, &declarationScope);
    return value;
  }

  struct BooleanSelection {
    const Object* object;
    std::vector<const Expr*> indices;
  };
  std::optional<BooleanSelection> booleanSelection(const Expr& expr, const StaticScope* scope = nullptr) const {
    BooleanSelection result{nullptr, {}};
    const auto* prefix = &expr;
    while (prefix->kind == Expr::Kind::Indexed && prefix->right->kind != Expr::Kind::Range) {
      result.indices.push_back(prefix->right.get());
      prefix = prefix->left.get();
    }
    if (prefix->kind != Expr::Kind::Name) return std::nullopt;
    const auto name = key(*prefix);
    if (scope) {
      if (scope->values.contains(name) || scope->arrays.contains(name)) return std::nullopt;
      if (scope->environment) {
        const auto found = scope->environment->constants.find(name);
        if (found != scope->environment->constants.end()) result.object = &found->second;
      }
    } else {
      const auto found = objects_.find(name);
      if (found != objects_.end()) result.object = &found->second;
    }
    if (!result.object || !result.object->constant || result.object->value.shape.types.back() != "boolean" ||
        result.indices.empty() || result.indices.size() != result.object->value.shape.ranges.size()) return std::nullopt;
    std::reverse(result.indices.begin(), result.indices.end());
    return result;
  }

  static const Expr* arrayAttribute(const Expr& expression) {
    if (expression.kind == Expr::Kind::Attribute) return &expression;
    if (expression.kind == Expr::Kind::Indexed && expression.left->kind == Expr::Kind::Attribute)
      return expression.left.get();
    return nullptr;
  }

  Range attributeRange(const Expr& expression, StaticScope* scope = nullptr) {
    DiagnosticScope location(expression.span);
    const auto* attribute = arrayAttribute(expression);
    if (!attribute) fail("expected an array attribute");
    if (expression.kind == Expr::Kind::Indexed && staticInteger(*expression.right, scope) != 1)
      fail("array attribute dimension must be 1");
    const auto& prefix = *attribute->left;
    if (scope) {
      if (prefix.kind == Expr::Kind::Name)
        if (const auto* array = staticArray(key(prefix), scope)) return array->bounds;
      fail("static function array attribute requires an integer array constant or parameter");
    }
    if (prefix.kind == Expr::Kind::Name) {
      const auto name = key(prefix);
      if (const auto found = integerTables_.find(name); found != integerTables_.end())
        return found->second.bounds;
      if (subtypes_.contains(name)) {
        const auto type = shape(vhdl::TypeMark{{prefix.text, prefix.canonical, prefix.span}, std::nullopt});
        if (type.ranges.empty()) fail("array attribute subtype prefix must denote an array");
        return type.ranges.front();
      }
      if (const auto found = arrayDeclarations_.find(name); found != arrayDeclarations_.end()) {
        if (found->second->indexSubtype)
          fail("array attribute requires a constrained type or object");
        if (libraryContext_.precedes(prefix.span.start.offset, found->second->span.start.offset))
          fail("array type is used before its declaration: " + name);
        return arrayTypeRanges_.at(name);
      }
    }
    const auto selected = selection(prefix);
    if (selected.shape.ranges.empty()) fail("array attribute prefix must denote an array");
    return selected.shape.ranges.front();
  }

  StaticScalar attributeScalar(const Expr& expression, StaticScope* scope = nullptr) {
    const auto* attribute = arrayAttribute(expression);
    const auto name = key(*attribute);
    if (name == "pos") {
      if (scope) fail("enumeration attributes in static function bodies are unsupported");
      const auto value = this->expression(expression, State{});
      int64_t ordinal = 0;
      for (auto* bit : value.bits) {
        const auto binary = constantValue(bit);
        if (!binary) fail("enumeration position requires a static value in this context");
        ordinal = ordinal * 2 + *binary;
      }
      return {ordinal, false};
    }
    if (name != "left" && name != "right" && name != "low" && name != "high" &&
        name != "length" && name != "ascending") fail("unsupported scalar array attribute: " + name);
    const auto bounds = attributeRange(expression, scope);
    if (name == "length") return {int64_t(bounds.size()), false};
    if (name == "ascending") return {bounds.ascending, true};
    if (bounds.indexEnumeration) fail("enumeration-valued array attributes are not supported");
    if (name == "left") return {bounds.left, false};
    if (name == "right") return {bounds.right, false};
    if (name == "low") return {bounds.ascending ? bounds.left : bounds.right, false};
    return {bounds.ascending ? bounds.right : bounds.left, false};
  }
  void addLocalFunction(const vhdl::FunctionDeclaration& function) {
    const auto name = key(function.name);
    if (!function.body) fail("architecture function forward declarations are unsupported: " + name);
    if (strings_.contains(name) || subtypes_.contains(name) || functions_.contains(name) || integers_.contains(name) || booleans_.contains(name) ||
        (objects_.contains(name) && !enumLiterals_.contains(name)) || integerTables_.contains(name) || records_.contains(name) ||
        enumerations_.contains(name) || arrayDeclarations_.contains(name))
      fail("duplicate or overloaded architecture function: " + name);
    functions_.emplace(name, FunctionBinding{nullptr, &function});
    auto& environment = localFunctionEnvironments_[&function];
    for (const auto& [id, value] : integers_) environment.values.emplace(id, StaticScalar{value, false});
    for (const auto& [id, value] : booleans_) environment.values.emplace(id, StaticScalar{value, true});
    environment.enumLiterals = enumLiterals_;
    environment.subtypes = subtypes_;
    environment.records = records_;
    environment.recordOffsets = recordOffsets_;
    environment.arrayShapes = arrays_;
    environment.arrayOffsets = arrayTypeOffsets_;
    environment.arrayRanges = arrayTypeRanges_;
    environment.enumerations = enumerations_;
    environment.functions = functions_;
    environment.arrays = integerTables_;
    environment.arrayTypes = arrayDeclarations_;
    for (const auto& [id, object] : objects_) if (object.constant) environment.constants.emplace(id, object);
    environment.strings = strings_;
    environment.textio = textio_;
    environment.logic = stdLogic_;
    environment.numeric = numeric_;
    environment.arith = arith_;
    environment.unsignedLogic = unsigned_;
    environment.signedLogic = signed_;
  }

  const vhdl::PackageDeclaration* packageBody(const vhdl::PackageDeclaration& declaration) const {
    for (const auto& package : syntax_.packages)
      if (package.body && key(package.name) == key(declaration.name) &&
          libraryContext_.owner(package.name.span.start.offset) == libraryContext_.owner(declaration.name.span.start.offset)) return &package;
    return nullptr;
  }

  FunctionBinding staticFunction(const std::string& name, const StaticScope* scope) const {
    if (!scope) {
      const auto found = functions_.find(name);
      if (found != functions_.end()) return found->second;
    } else if (scope->environment) {
      const auto found = scope->environment->functions.find(name);
      if (found != scope->environment->functions.end()) return found->second;
    } else if (scope->package) {
      for (const auto& function : scope->package->functions)
        if (key(function.name) == name) return {scope->package, &function};
      const vhdl::FunctionDeclaration* found = nullptr;
      if (const auto* body = packageBody(*scope->package))
        for (const auto& function : body->functions) if (key(function.name) == name) {
          if (found) fail("overloaded private package functions are unsupported: " + name);
          found = &function;
        }
      if (found) return {scope->package, found};
    }
    return {};
  }

  static std::string staticSignature(const Expr* expression) {
    if (!expression) return {};
    std::string result = std::to_string(static_cast<int>(expression->kind)) + ":" + key(*expression) + "(";
    result += staticSignature(expression->left.get()) + "," + staticSignature(expression->right.get()) + "," +
        staticSignature(expression->condition.get());
    for (const auto& element : expression->elements) result += "," + staticSignature(element.get());
    return result + ")";
  }

  static bool sameStaticType(const vhdl::TypeMark& a, const vhdl::TypeMark& b) {
    if (key(a.name) != key(b.name) || bool(a.constraint) != bool(b.constraint)) return false;
    if (!a.constraint) return true;
    return a.constraint->ascending == b.constraint->ascending &&
        staticSignature(a.constraint->leftExpression.get()) == staticSignature(b.constraint->leftExpression.get()) &&
        staticSignature(a.constraint->rightExpression.get()) == staticSignature(b.constraint->rightExpression.get()) &&
        staticSignature(a.constraint->attribute.get()) == staticSignature(b.constraint->attribute.get());
  }

  const vhdl::FunctionDeclaration& functionBody(const FunctionBinding& binding) const {
    const auto& declaration = *binding.second;
    if (declaration.body) return declaration;
    if (!binding.first) fail("missing architecture function body");
    const auto* package = packageBody(*binding.first);
    const vhdl::FunctionDeclaration* found = nullptr;
    if (package) for (const auto& candidate : package->functions) {
      if (key(candidate.name) != key(declaration.name)) continue;
      if (found) fail("overloaded or duplicate function bodies are unsupported: " + key(declaration.name));
      found = &candidate;
    }
    if (!found) fail("missing package function body: " + key(declaration.name));
    bool match = found->pure == declaration.pure && found->procedure == declaration.procedure &&
        sameStaticType(found->returnType, declaration.returnType);
    std::vector<std::pair<std::string, const vhdl::TypeMark*>> declared, defined;
    for (const auto& parameter : declaration.parameters)
      for (const auto& name : parameter.names) declared.emplace_back(key(name), &parameter.type);
    for (const auto& parameter : found->parameters)
      for (const auto& name : parameter.names) defined.emplace_back(key(name), &parameter.type);
    std::vector<vhdl::PortMode> declaredModes, definedModes;
    for (const auto& parameter : declaration.parameters) for (const auto& name : parameter.names) declaredModes.push_back(parameter.mode);
    for (const auto& parameter : found->parameters) for (const auto& name : parameter.names) definedModes.push_back(parameter.mode);
    match &= declaredModes == definedModes;
    match &= declared.size() == defined.size();
    if (match) for (size_t i = 0; i < declared.size(); ++i)
      match &= declared[i].first == defined[i].first && sameStaticType(*declared[i].second, *defined[i].second);
    if (!match) fail("package function declaration/body profile mismatch: " + key(declaration.name));
    return *found;
  }

  int64_t staticInteger(const Expr& expression, StaticScope* scope) {
    const auto value = scalar(expression, scope);
    if (value.boolean) fail("static integer expression has boolean type");
    return value.value;
  }

  void checkStaticType(const StaticScalar& value, const vhdl::TypeMark& original, StaticScope* scope) {
    const auto type = resolveSubtype(original, scope && scope->environment ? &scope->environment->subtypes : nullptr);
    const auto name = key(type.name);
    if (name == "boolean") {
      if (!value.boolean || type.constraint) fail("static boolean type mismatch");
      return;
    }
    if (name != "integer" && name != "natural" && name != "positive")
      fail("static package functions currently require integer or boolean values: " + name);
    if (value.boolean) fail("static integer type mismatch");
    if (value.value < INT32_MIN || value.value > INT32_MAX ||
        (name == "natural" && value.value < 0) || (name == "positive" && value.value < 1))
      fail("static function value violates its subtype");
    if (type.constraint) {
      if (type.constraint->attribute) fail("scalar subtype cannot use an array range attribute");
      Range bounds{staticInteger(*type.constraint->leftExpression, scope),
          staticInteger(*type.constraint->rightExpression, scope), type.constraint->ascending};
      bounds.position(value.value);
    }
  }

  void validateStaticStatements(const std::vector<Statement>& statements, std::set<std::string> variables, size_t loopDepth = 0) {
    for (const auto& statement : statements) {
      DiagnosticScope location(statement.span);
      if (statement.kind == Statement::Kind::Assignment) {
        if (statement.assignment.kind != vhdl::AssignmentKind::Variable ||
            !statement.assignment.indices.empty() || !variables.contains(key(statement.assignment.target)))
          fail("function assignment requires a local scalar variable");
      }
      if (statement.kind == Statement::Kind::Exit && !loopDepth)
        fail("exit requires an enclosing function loop");
      if (statement.kind == Statement::Kind::Case)
        fail("case in static function bodies is not yet supported");
      auto nestedVariables = variables;
      if (statement.kind == Statement::Kind::For) nestedVariables.erase(key(statement.iterator));
      validateStaticStatements(statement.statements, std::move(nestedVariables),
          loopDepth + (statement.kind == Statement::Kind::For || statement.kind == Statement::Kind::While));
      validateStaticStatements(statement.alternative, variables, loopDepth);
    }
  }

  struct StaticFlow {
    std::optional<StaticScalar> returned;
    bool exited = false;
    explicit operator bool() const { return returned.has_value() || exited; }
  };

  StaticFlow staticStatements(const std::vector<Statement>& statements, StaticScope& scope) {
    for (const auto& statement : statements) {
      DiagnosticScope location(statement.span);
      if (++steps_ > 100000) fail("static function evaluation limit exceeded");
      if (statement.kind == Statement::Kind::Null) continue;
      if (statement.kind == Statement::Kind::Return) return {scalar(*statement.returnValue, &scope)};
      if (statement.kind == Statement::Kind::Exit) {
        if (!statement.condition) return {std::nullopt, true};
        const auto condition = scalar(*statement.condition, &scope);
        if (!condition.boolean) fail("exit condition must be boolean");
        if (condition.value) return {std::nullopt, true};
        continue;
      }
      if (statement.kind == Statement::Kind::If) {
        const auto condition = scalar(*statement.condition, &scope);
        if (!condition.boolean) fail("function if condition must be boolean");
        auto result = staticStatements(condition.value ? statement.statements : statement.alternative, scope);
        if (result) return result;
      } else if (statement.kind == Statement::Kind::While) {
        for (;;) {
          if (++steps_ > 100000) fail("static function evaluation limit exceeded");
          const auto condition = scalar(*statement.condition, &scope);
          if (!condition.boolean) fail("while condition must be boolean");
          if (!condition.value) break;
          auto result = staticStatements(statement.statements, scope);
          if (result.returned) return result;
          if (result.exited) break;
        }
      } else if (statement.kind == Statement::Kind::For) {
        const auto bounds = staticRange(statement.range, &scope);
        const auto name = key(statement.iterator);
        const auto previous = scope.values.find(name);
        const std::optional<StaticScalar> saved = previous == scope.values.end() ? std::nullopt :
            std::optional<StaticScalar>(previous->second);
        const auto variable = scope.variables.extract(name);
        const auto restore = [&] {
          if (saved) scope.values[name] = *saved;
          else scope.values.erase(name);
          if (!variable.empty()) scope.variables.emplace(name, variable.mapped());
        };
        for (size_t i = 0; i < bounds.size(); ++i) {
          if (++steps_ > 100000) fail("static function evaluation limit exceeded");
          scope.values[name] = {bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i), false};
          auto result = staticStatements(statement.statements, scope);
          if (result.returned) { restore(); return result; }
          if (result.exited) break;
        }
        restore();
      } else if (statement.kind == Statement::Kind::Assignment) {
        const auto& assignment = statement.assignment;
        const auto name = key(assignment.target);
        if (assignment.kind != vhdl::AssignmentKind::Variable || !assignment.indices.empty() ||
            !scope.variables.contains(name)) fail("function assignment requires a local scalar variable");
        const auto value = scalar(*assignment.value, &scope);
        checkStaticType(value, *scope.variables.at(name), &scope);
        scope.values[name] = value;
      } else fail("unsupported static function statement");
    }
    return {};
  }

  StaticScalar callStatic(const Expr& call, StaticScope* caller) {
    const auto& prefix = call.kind == Expr::Kind::Name ? call : *call.left;
    if (prefix.kind != Expr::Kind::Name) fail("function prefix must name a visible package function");
    if ((!caller && (integers_.contains(key(prefix)) || booleans_.contains(key(prefix)) || objects_.contains(key(prefix)))) ||
        (caller && (caller->values.contains(key(prefix)) || caller->arrays.contains(key(prefix))))) fail("function name is hidden by a local object");
    const auto binding = staticFunction(key(prefix), caller);
    if (!binding.second) fail("no visible static package function: " + key(prefix));
    const auto& declaration = *binding.second;
    if (libraryContext_.precedes(call.span.start.offset, declaration.span.start.offset))
      fail("function is used before its declaration: " + key(prefix));
    if (!declaration.pure) fail("impure package functions are unsupported");
    if (staticDepth_ >= 64) fail("static function recursion limit exceeded");
    ++staticDepth_;
    struct DepthGuard { size_t& depth; ~DepthGuard() { --depth; } } depth{staticDepth_};
    const auto& body = functionBody(binding);
    StaticScope local;
    local.package = binding.first;
    if (!binding.first) local.environment = &localFunctionEnvironments_.at(&declaration);
    std::vector<const vhdl::GenericDeclaration*> formals;
    std::vector<std::string> names;
    for (const auto& parameter : declaration.parameters) for (const auto& name : parameter.names) {
      if (std::find(names.begin(), names.end(), key(name)) != names.end()) fail("duplicate function parameter");
      names.push_back(key(name));
      formals.push_back(&parameter);
    }
    std::vector<const Expr*> arguments;
    if (call.kind == Expr::Kind::Indexed) arguments.push_back(call.right.get());
    else if (call.kind == Expr::Kind::Call)
      for (const auto& argument : call.elements) arguments.push_back(argument.get());
    std::vector<std::optional<StaticScalar>> values(names.size());
    std::vector<std::optional<IntegerTable>> arrays(names.size());
    bool named = false;
    size_t position = 0;
    for (const auto* argument : arguments) {
      size_t destination = position++;
      if (argument->kind == Expr::Kind::Association) {
        named = true;
        if (argument->left->kind != Expr::Kind::Name) fail("function formal must be a name");
        const auto found = std::find(names.begin(), names.end(), key(*argument->left));
        destination = found - names.begin();
        argument = argument->right.get();
      } else if (named) fail("positional function actual follows named association");
      if (destination >= names.size() || values[destination] || arrays[destination]) fail("invalid or duplicate function actual");
      if (staticArrayType(key(formals[destination]->type.name), local))
        arrays[destination] = staticArrayActual(*argument, formals[destination]->type, local, caller);
      else values[destination] = scalar(*argument, caller);
    }
    for (size_t i = 0; i < names.size(); ++i) {
      if (!values[i] && !arrays[i]) {
        if (!formals[i]->defaultValue) fail("missing function argument: " + names[i]);
        // Defaults are evaluated in the declaration scope, never in the caller.
        StaticScope declarationScope;
        declarationScope.package = binding.first;
        declarationScope.environment = local.environment;
        if (staticArrayType(key(formals[i]->type.name), local))
          arrays[i] = staticArrayActual(*formals[i]->defaultValue, formals[i]->type, declarationScope, &declarationScope);
        else values[i] = scalar(*formals[i]->defaultValue, &declarationScope);
      }
      if (arrays[i]) { local.arrays.emplace(names[i], *arrays[i]); continue; }
      checkStaticType(*values[i], formals[i]->type, &local);
      local.values.emplace(names[i], *values[i]);
    }
    std::map<size_t, std::pair<const vhdl::ObjectDeclaration*, const Expr*>> objects;
    for (const auto& variable : body.variables)
      objects.emplace(variable.span.start.offset, std::make_pair(&variable, variable.initializer.get()));
    for (const auto& constant : body.constants)
      objects.emplace(constant.object.span.start.offset, std::make_pair(&constant.object, constant.value.get()));
    for (const auto& [offset, entry] : objects) {
      const auto& object = *entry.first;
      StaticScalar value;
      const auto type = key(resolveSubtype(object.type, local.environment ? &local.environment->subtypes : nullptr).name);
      if (object.type.constraint && object.type.constraint->attribute)
        fail("array range attributes require vector function evaluation");
      if (entry.second) value = scalar(*entry.second, &local);
      else {
        value.boolean = type == "boolean";
        value.value = object.type.constraint ? staticInteger(*object.type.constraint->leftExpression, &local) :
            type == "integer" ? INT32_MIN : type == "positive" ? 1 : 0;
      }
      checkStaticType(value, object.type, &local);
      const bool variable = std::any_of(body.variables.begin(), body.variables.end(),
          [&](const auto& candidate) { return &candidate == &object; });
      for (const auto& name : object.names) {
        if (local.arrays.contains(key(name)) || !local.values.emplace(key(name), value).second) fail("duplicate function local declaration");
        if (variable) local.variables.emplace(key(name), &object.type);
      }
    }
    std::set<std::string> variables;
    for (const auto& [name, type] : local.variables) variables.insert(name);
    validateStaticStatements(body.statements, std::move(variables));
    const auto result = staticStatements(body.statements, local);
    if (!result.returned) fail("function completed without returning a value: " + key(prefix));
    checkStaticType(*result.returned, declaration.returnType, &local);
    return *result.returned;
  }

  bool scalarFunctionType(const vhdl::TypeMark& type) const {
    const auto found = subtypes_.find(key(type.name));
    const auto name = found == subtypes_.end() ? key(type.name) : key(found->second.type.name);
    return name == "integer" || name == "natural" || name == "positive" || name == "boolean";
  }

  bool scalarFunction(const vhdl::FunctionDeclaration& function) const {
    return function.pure && function.files.empty() && !function.procedure && scalarFunctionType(function.returnType) &&
        std::all_of(function.parameters.begin(), function.parameters.end(), [this](const auto& parameter) {
          return scalarFunctionType(parameter.type);
        });
  }

  Shape functionShape(const vhdl::TypeMark& original, const Shape* actual = nullptr, bool dynamicResult = false) {
    const auto type = resolveSubtype(original);
    const auto name = key(type.name);
    if (name == "boolean") return {{"boolean"}, {}};
    if (name == "integer" || name == "natural" || name == "positive") return shape(original);
    if (records_.contains(name) || enumerations_.contains(name) || arrays_.contains(name)) return shape(original);
    const bool vector = name == "bit_vector" || name == "std_logic_vector" ||
        name == "std_ulogic_vector" || name == "unsigned" || name == "signed";
    if (!vector && name != "bit" && name != "std_logic" && name != "std_ulogic")
      fail("runtime functions require binary scalar/vector or boolean values");
    if (vector && !type.constraint) {
      if (actual && actual->dynamicBounds && !dynamicResult) fail("runtime-dependent unconstrained function argument bounds are unsupported");
      if (!actual || actual->ranges.size() != 1 || actual->integerWidth || !actual->fields.empty())
        fail("unconstrained function vector requires a vector actual");
      auto result = *actual;
      // Resolve the nominal vector type and visibility using a bounded type.
      vhdl::TypeMark bounded{original.name, vhdl::DiscreteRange{0, 0, true}};
      const auto target = shape(bounded);
      if (result.types != target.types || !result.declarations.empty()) fail("function argument type mismatch");
      return result;
    }
    return shape(original);
  }

  Value mergeRuntimeResults(SNLBitNet* condition, const Value& yes, const Value& no) {
    if (!compatible(yes.shape, no.shape)) fail("function return branch type mismatch");
    for (size_t i = 0; i < yes.shape.ranges.size(); ++i) {
      const auto& a = yes.shape.ranges[i];
      const auto& b = no.shape.ranges[i];
      if (a.left != b.left || a.right != b.right || a.ascending != b.ascending)
        fail("runtime-dependent function return bounds are unsupported");
    }
    auto shape = yes.shape;
    shape.dynamicBounds |= no.shape.dynamicBounds;
    return {shape, mux(condition, yes.bits, no.bits)};
  }

  using RuntimeContinuation = std::function<std::optional<Value>(State&)>;

  std::optional<Value> runtimeStatements(std::span<const Statement> statements, State& state,
      const RuntimeContinuation& continuation = {}) {
    struct ControlDepthGuard { size_t& depth; ~ControlDepthGuard() { --depth; } } depth{*runtimeControlDepth_};
    if (++*runtimeControlDepth_ > 256) fail("runtime control-flow nesting limit exceeded");
    for (size_t position = 0; position < statements.size(); ++position) {
      const auto& statement = statements[position];
      DiagnosticScope location(statement.span);
      IntegerFacts integerFacts(*this, state);
      if (++*runtimeSteps_ > 100000) fail("runtime function elaboration limit exceeded");
      if (statement.kind == Statement::Kind::Null) continue;
      const RuntimeContinuation tail = [&](State& branch) {
        // A returning branch may elaborate later loop iterations. Restore the
        // current iterator bindings before elaborating its sibling branch.
        IntegerFacts bindings(*this, branch);
        auto result = runtimeStatements(statements.subspan(position+1), branch, continuation);
        return result || !continuation ? result : continuation(branch);
      };
      if (statement.kind == Statement::Kind::Return) {
        if (!statement.returnValue) fail("function return requires a value");
        if (runtimeReturnShape_ && runtimeReturnShape_->integerWidth && isStatic(*statement.returnValue))
          checkInteger(integer(*statement.returnValue), *runtimeReturnType_);
        return expression(*statement.returnValue, state, runtimeReturnShape_ ? &*runtimeReturnShape_ : nullptr);
      }
      if (statement.kind == Statement::Kind::Call) {
        callRuntime(*statement.procedureCall, state, &state);
      } else if (statement.kind == Statement::Kind::Assignment) {
        const auto& assignment = statement.assignment;
        const auto found = objects_.find(key(assignment.target));
        if (assignment.kind != vhdl::AssignmentKind::Variable || found == objects_.end() || !found->second.variable)
          fail("runtime function assignments require local variables");
        assign(assignment, state);
      } else if (statement.kind == Statement::Kind::Case) {
        if (auto result = caseStatement(statement, state, true, tail)) return result;
      } else if (statement.kind == Statement::Kind::For) {
        const auto bounds = range(statement.range);
        const auto name = key(statement.iterator);
        if (integers_.contains(name) || booleans_.contains(name) || objects_.contains(name))
          fail("shadowed runtime function loop parameters are unsupported");
        std::function<std::optional<Value>(size_t, State&)> iterations;
        iterations = [&](size_t first, State& current) -> std::optional<Value> {
          for (size_t i = first; i < bounds.size(); ++i) {
            integers_[name] = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
            const RuntimeContinuation next = [&, i](State& branch) {
              IntegerFacts bindings(*this, branch);
              auto result = iterations(i+1, branch);
              integers_.erase(name);
              return result ? result : tail(branch);
            };
            if (auto result = runtimeStatements(statement.statements, current, next)) return result;
          }
          integers_.erase(name);
          return std::nullopt;
        };
        auto result = iterations(0, state);
        integers_.erase(name);
        if (result) return result;
      } else if (statement.kind == Statement::Kind::If) {
        const auto condition = conditionValue(expression(*statement.condition, state));
        if (const auto selected = knownCondition(condition.bits.front(), state)) {
          if (auto result = runtimeStatements(*selected ? statement.statements : statement.alternative, state, tail)) return result;
          continue;
        }
        auto yes = state, no = state;
        assume(yes, condition.bits.front(), true); assume(no, condition.bits.front(), false);
        auto yesResult = runtimeStatements(statement.statements, yes, tail);
        auto noResult = runtimeStatements(statement.alternative, no, tail);
        if (bool(yesResult) != bool(noResult)) {
          if (!yesResult) yesResult = tail(yes);
          else noResult = tail(no);
          if (!yesResult || !noResult) fail("runtime function has a path without a return");
        }
        if (yesResult) return mergeRuntimeResults(condition.bits.front(), *yesResult, *noResult);
        merge(state, yes, no, condition.bits.front());
      } else fail("unsupported runtime function statement");
    }
    return std::nullopt;
  }

  bool initializerProcedure(const Expr& call, State& state) {
    const auto& prefix = call.kind == Expr::Kind::Name ? call : *call.left;
    if (prefix.kind != Expr::Kind::Name || functions_.contains(key(prefix))) return false;
    const auto name = key(prefix);
    if (name != "readline" && name != "read" && name != "hread") return false;
    if (!initializerContext_ || !textio_) fail("TextIO requires a static initializer and std.textio visibility");
    std::vector<const Expr*> arguments;
    if (call.kind == Expr::Kind::Indexed) arguments.push_back(call.right.get());
    else if (call.kind == Expr::Kind::Call) for (const auto& argument : call.elements) arguments.push_back(argument.get());
    if (arguments.size() != 2 || arguments[0]->kind != Expr::Kind::Name)
      fail("initializer TextIO currently requires two positional arguments");
    const auto source = key(*arguments[0]);
    if (name == "readline") {
      if (!files_.contains(source) || arguments[1]->kind != Expr::Kind::Name || !lines_.contains(key(*arguments[1])))
        fail("readline requires a text file and a local line variable");
      std::string line;
      if (!std::getline(*files_.at(source), line)) fail("readline reached EOF or failed reading the initialization file");
      if (!line.empty() && line.back() == '\r') line.pop_back();
      lines_.at(key(*arguments[1])) = std::move(line);
      return true;
    }
    if (!lines_.contains(source) || !lines_.at(source)) fail("read requires a non-null local line variable");
    const auto target = selection(*arguments[1]);
    const auto& object = objects_.at(target.name);
    if (!object.variable || target.shape.ranges.size() != 1 || !target.shape.fields.empty() || target.shape.integerWidth ||
        (target.shape.types.back() != "bit" && target.shape.types.back() != "std_logic"))
      fail("initializer read requires a local logic vector variable");
    if (target.shape.types.back() == "std_logic" && !stdLogic_)
      fail("logic-vector TextIO requires ieee.std_logic_1164 visibility");
    auto& line = *lines_.at(source);
    size_t cursor = 0;
    const auto whitespace = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (cursor < line.size() && whitespace(line[cursor])) ++cursor;
    const size_t width = target.shape.size();
    const size_t digits = name == "hread" ? (width + 3) / 4 : width;
    std::string data;
    bool underscore = false;
    for (size_t count = 0; count < digits;) {
      if (cursor >= line.size()) fail("initialization line is too short for " + name);
      const char c = line[cursor++];
      if (name == "hread" && c == '_') {
        if (!count || underscore) fail("invalid underscore in initialization data");
        underscore = true;
        continue;
      }
      underscore = false;
      if (name == "hread") {
        const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
            c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (digit >= 0) for (int bit = 3; bit >= 0; --bit) data += ((digit >> bit) & 1) ? '1' : '0';
        else if ((c == 'X' || c == 'Z') && target.shape.types.back() == "std_logic") data.append(4, c);
        else fail("invalid hexadecimal initialization data");
      } else {
        if (c != '0' && c != '1' && !(target.shape.types.back() == "std_logic" && (c == 'X' || c == 'Z')))
          fail("invalid or unsupported binary initialization data");
        data += c;
      }
      ++count;
    }
    const auto padding = data.size() - width;
    if (std::find(data.begin(), data.begin() + padding, '1') != data.begin() + padding)
      fail("hexadecimal initialization value exceeds the target width");
    auto element = target.shape;
    element.ranges.clear();
    element.removeOuterType();
    auto& bits = state.variables.at(target.name);
    for (size_t i = 0; i < width; ++i) bits[target.offset + i] = logicLiteral(data[padding + i], element);
    line.erase(0, cursor);
    return true;
  }

  struct InitializerFlow {
    std::optional<Value> returned;
    bool exited = false;
  };

  bool initializerCondition(const Expr& condition, const State& state) {
    IntegerFacts facts(*this, state);
    const auto value = conditionValue(expression(condition, state));
    const auto known = constantValue(value.bits.front());
    if (!known) fail("initializer control flow must be static");
    return *known;
  }

  InitializerFlow initializerStatements(const std::vector<Statement>& body, State& state) {
    struct DepthGuard { size_t& depth; ~DepthGuard() { --depth; } } depth{*runtimeControlDepth_};
    if (++*runtimeControlDepth_ > 256) fail("initializer control-flow nesting limit exceeded");
    for (const auto& statement : body) {
      DiagnosticScope location(statement.span);
      IntegerFacts integerFacts(*this, state);
      if (++*runtimeSteps_ > 1000000) fail("initializer evaluation limit exceeded");
      if (statement.kind == Statement::Kind::Null) continue;
      if (statement.kind == Statement::Kind::Return) {
        auto value = expression(*statement.returnValue, state, runtimeReturnShape_ ? &*runtimeReturnShape_ : nullptr);
        for (auto* bit : value.bits) if (!bit || !bit->isConstant()) fail("initializer function must return a static value");
        return {std::move(value)};
      }
      if (statement.kind == Statement::Kind::Exit) {
        if (!statement.condition || initializerCondition(*statement.condition, state)) return {std::nullopt, true};
      } else if (statement.kind == Statement::Kind::Call) {
        if (!initializerProcedure(*statement.procedureCall, state)) callRuntime(*statement.procedureCall, state, &state);
      } else if (statement.kind == Statement::Kind::Assignment) {
        const auto& assignment = statement.assignment;
        const auto found = objects_.find(key(assignment.target));
        if (assignment.kind != vhdl::AssignmentKind::Variable || found == objects_.end() || !found->second.variable)
          fail("initializer functions may assign only local variables");
        assign(assignment, state);
      } else if (statement.kind == Statement::Kind::If) {
        const auto flow = initializerStatements(initializerCondition(*statement.condition, state) ?
            statement.statements : statement.alternative, state);
        if (flow.returned || flow.exited) return flow;
      } else if (statement.kind == Statement::Kind::Case) {
        const auto selector = expression(*statement.condition, state);
        const auto matches = selectionMatches(selector, statement.choices);
        for (size_t i = 0; i < matches.size(); ++i) {
          const auto known = statement.choices[i].empty() ? std::optional<bool>{true} : constantValue(matches[i]);
          if (!known) fail("initializer case selector must be static");
          if (*known) {
            const auto flow = initializerStatements(statement.caseBodies[i], state);
            if (flow.returned || flow.exited) return flow;
            break;
          }
        }
      } else if (statement.kind == Statement::Kind::For) {
        const auto bounds = range(statement.range);
        const auto name = key(statement.iterator);
        if (integers_.contains(name) || booleans_.contains(name) || objects_.contains(name))
          fail("shadowed runtime function loop parameters are unsupported");
        LocalBindings bindings(*this);
        bindings.declare(name);
        for (size_t i = 0; i < bounds.size(); ++i) {
          if (++*runtimeSteps_ > 1000000) fail("initializer loop limit exceeded");
          integers_[name] = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
          const auto flow = initializerStatements(statement.statements, state);
          if (flow.returned) return flow;
          if (flow.exited) break;
        }
      } else if (statement.kind == Statement::Kind::While) {
        while (initializerCondition(*statement.condition, state)) {
          if (++*runtimeSteps_ > 1000000) fail("initializer loop limit exceeded");
          const auto flow = initializerStatements(statement.statements, state);
          if (flow.returned) return flow;
          if (flow.exited) break;
        }
      } else fail("unsupported initializer statement");
    }
    return {};
  }

  Value callRuntime(const Expr& call, const State& callerState, State* updatedState = nullptr) {
    const auto& prefix = call.kind == Expr::Kind::Name ? call : *call.left;
    const auto binding = staticFunction(key(prefix), nullptr);
    if (!binding.second) fail("no visible function: " + key(prefix));
    const auto& declaration = *binding.second;
    if (declaration.procedure != (updatedState != nullptr)) fail("function/procedure call kind mismatch");
    if (!declaration.pure && !initializerContext_) fail("impure functions require a static initializer");
    if (libraryContext_.precedes(call.span.start.offset, declaration.span.start.offset))
      fail("function is used before its declaration: " + key(prefix));
    if (runtimeDepth_ >= 64) fail("runtime function recursion limit exceeded");
    if (++*runtimeSteps_ > 100000) fail("runtime function elaboration limit exceeded");
    const auto& body = functionBody(binding);
    RTLConstructor frame(design_, entity_, architecture_, syntax_, libraryContext_, {}, build_);
    frame.initializerContext_ = initializerContext_;
    frame.initializationFiles_ = initializationFiles_;
    frame.runtimeDepth_ = runtimeDepth_ + 1;
    frame.runtimeSteps_ = runtimeSteps_;
    frame.runtimeControlDepth_ = runtimeControlDepth_;
    frame.zero_ = zero_;
    frame.one_ = one_;
    if (binding.first) {
      frame.work_ = libraryContext_.owner(binding.first->name.span.start.offset);
      frame.context(binding.first->context);
      for (const auto& function : binding.first->functions)
        if (!frame.functions_.emplace(key(function.name), FunctionBinding{binding.first, &function}).second)
          fail("ambiguous package function: " + key(function.name)); // LCOV_EXCL_LINE: context() rejects duplicate package functions before calls can reach this frame.
      if (const auto* package = packageBody(*binding.first)) {
        frame.context(package->context);
        std::set<std::string> defined;
        for (const auto& function : package->functions) {
          if (!defined.insert(key(function.name)).second) fail("overloaded runtime package functions are unsupported");
          if (!frame.functions_.contains(key(function.name)))
            frame.functions_.emplace(key(function.name), FunctionBinding{binding.first, &function});
        }
      }
      frame.declarations(binding.first->arrayTypes, binding.first->constants, binding.first->recordTypes,
          {}, binding.first->enumerationTypes, {}, binding.first->subtypes);
    } else {
      const auto& environment = localFunctionEnvironments_.at(&declaration);
      frame.enumLiterals_ = environment.enumLiterals;
      frame.subtypes_ = environment.subtypes;
      frame.records_ = environment.records;
      frame.recordOffsets_ = environment.recordOffsets;
      frame.arrays_ = environment.arrayShapes;
      frame.arrayTypeOffsets_ = environment.arrayOffsets;
      frame.arrayTypeRanges_ = environment.arrayRanges;
      frame.arrayDeclarations_ = environment.arrayTypes;
      frame.enumerations_ = environment.enumerations;
      frame.functions_ = environment.functions;
      frame.localFunctionEnvironments_ = localFunctionEnvironments_;
      frame.objects_ = environment.constants;
      for (const auto& [name, value] : environment.values) {
        if (value.boolean) frame.booleans_[name] = value.value;
        else frame.integers_[name] = value.value;
      }
      frame.strings_ = environment.strings;
      frame.textio_ = environment.textio;
      frame.stdLogic_ = environment.logic;
      frame.numeric_ = environment.numeric;
      frame.arith_ = environment.arith;
      frame.unsigned_ = environment.unsignedLogic;
      frame.signed_ = environment.signedLogic;
    }
    std::vector<std::pair<std::string, const vhdl::GenericDeclaration*>> formals;
    for (const auto& parameter : declaration.parameters) for (const auto& name : parameter.names) {
      if (std::any_of(formals.begin(), formals.end(), [&](const auto& formal) { return formal.first == key(name); }))
        fail("duplicate function parameter");
      formals.emplace_back(key(name), &parameter);
    }
    std::vector<const Expr*> definedDefaults;
    for (const auto& parameter : body.parameters) for (const auto& name : parameter.names)
      definedDefaults.push_back(parameter.defaultValue.get());
    for (size_t i = 0; i < formals.size(); ++i)
      if (staticSignature(formals[i].second->defaultValue.get()) != staticSignature(definedDefaults[i]))
        fail("runtime function declaration/body default mismatch");
    std::vector<const Expr*> actuals(formals.size(), nullptr), arguments;
    if (call.kind == Expr::Kind::Indexed) arguments.push_back(call.right.get());
    else if (call.kind == Expr::Kind::Call) for (const auto& argument : call.elements) arguments.push_back(argument.get());
    bool named = false;
    size_t position = 0;
    for (const auto* argument : arguments) {
      size_t destination = position++;
      if (argument->kind == Expr::Kind::Association) {
        named = true;
        if (argument->left->kind != Expr::Kind::Name) fail("function formal must be a name");
        destination = 0;
        while (destination < formals.size() && formals[destination].first != key(*argument->left)) ++destination;
        argument = argument->right.get();
      } else if (named) fail("positional function actual follows named association");
      if (destination >= formals.size() || actuals[destination]) fail("invalid or duplicate function actual");
      actuals[destination] = argument;
    }
    // Evaluate every actual in the caller before introducing formal names.
    std::vector<std::optional<Value>> values(formals.size());
    std::vector<std::optional<StaticScalar>> scalars(formals.size());
    std::vector<std::optional<std::string>> texts(formals.size());
    const State empty;
    std::map<size_t, Selection> outputs;
    for (size_t i = 0; i < formals.size(); ++i) {
      const auto& formal = *formals[i].second;
      const auto* argument = actuals[i] ? actuals[i] : formal.defaultValue.get();
      if (!argument) fail("missing function argument: " + formals[i].first);
      auto& evaluator = actuals[i] ? *this : frame;
      const auto& argumentState = actuals[i] ? callerState : empty;
      if (key(frame.resolveSubtype(formal.type).name) == "string") {
        if (!initializerContext_ || formal.mode != vhdl::PortMode::In || formal.type.constraint)
          fail("string function arguments require a static initializer and unconstrained mode in");
        texts[i] = evaluator.stringValue(*argument);
      } else if (formal.mode != vhdl::PortMode::In) {
        if (!updatedState || !actuals[i]) fail("procedure output requires a variable actual");
        const auto selected = selection(*argument);
        if (!objects_.at(selected.name).variable) fail("procedure output requires a variable actual");
        for (const auto& [prior, output] : outputs) {
          if (output.name == selected.name && output.offset < selected.offset + selected.shape.size() &&
              selected.offset < output.offset + output.shape.size()) fail("aliased procedure outputs are unsupported");
        }
        outputs.emplace(i, selected);
        const auto type = frame.functionShape(formal.type, &selected.shape);
        if (!compatible(type, selected.shape) || type.size() != selected.shape.size())
          fail("procedure argument type or length mismatch");
        const auto& source = callerState.variables.at(selected.name);
        Bits bits = formal.mode == vhdl::PortMode::Out ? Bits(type.size(), nullptr) :
            Bits(source.begin() + selected.offset, source.begin() + selected.offset + selected.shape.size());
        values[i] = Value{type, std::move(bits)};
      } else if (frame.scalarFunctionType(formal.type) && evaluator.isStatic(*argument))
        scalars[i] = evaluator.scalar(*argument);
      else {
        const auto formalType = frame.resolveSubtype(formal.type);
        const auto name = key(formalType.name);
        const bool vector = name == "bit_vector" || name == "std_logic_vector" ||
            name == "std_ulogic_vector" || name == "unsigned" || name == "signed";
        auto type = formalType.constraint || !vector ?
            std::optional<Shape>{frame.functionShape(formal.type)} : std::nullopt;
        values[i] = evaluator.expression(*argument, argumentState, type ? &*type : nullptr);
      }
    }
    State state;
    state.assumptions = callerState.assumptions;
    for (size_t i = 0; i < formals.size(); ++i) {
      const auto& [name, formal] = formals[i];
      frame.functions_.erase(name);
      frame.enumLiterals_.erase(name);
      if (!actuals[i] && !formal->defaultValue) fail("missing function argument: " + name);
      if (texts[i]) {
        frame.objects_.erase(name);
        frame.integers_.erase(name);
        frame.booleans_.erase(name);
        frame.strings_[name] = *texts[i];
      } else if (formal->mode == vhdl::PortMode::In && scalars[i]) {
        const auto value = *scalars[i];
        frame.checkStaticType(value, formal->type, nullptr);
        frame.objects_.erase(name);
        frame.integers_.erase(name);
        frame.booleans_.erase(name);
        if (value.boolean) frame.booleans_[name] = value.value;
        else frame.integers_[name] = value.value;
      } else {
        auto value = *values[i];
        const auto type = frame.functionShape(formal->type, &value.shape);
        if (!compatible(type, value.shape)) fail("function argument type or length mismatch");
        value.shape = type;
        frame.integers_.erase(name);
        frame.booleans_.erase(name);
        if (formal->mode != vhdl::PortMode::In) {
          state.variables[name] = value.bits;
          frame.objects_.insert_or_assign(name, Object{std::move(value), false, false, true});
        } else frame.objects_.insert_or_assign(name, Object{std::move(value), true});
      }
    }
    const auto returnType = frame.resolveSubtype(declaration.returnType);
    frame.runtimeReturnType_ = returnType;
    const auto resultName = key(returnType.name);
    const bool vectorResult = resultName == "bit_vector" || resultName == "std_logic_vector" ||
        resultName == "std_ulogic_vector" || resultName == "unsigned" || resultName == "signed";
    if (!declaration.procedure && (!vectorResult || returnType.constraint))
      frame.runtimeReturnShape_ = frame.functionShape(declaration.returnType);
    std::map<size_t, std::pair<const vhdl::ObjectDeclaration*, const Expr*>> locals;
    for (const auto& variable : body.variables)
      locals.emplace(variable.span.start.offset, std::make_pair(&variable, variable.initializer.get()));
    for (const auto& constant : body.constants)
      locals.emplace(constant.object.span.start.offset, std::make_pair(&constant.object, constant.value.get()));
    LocalBindings localBindings(frame);
    const auto declareLocal = [&](const vhdl::Name& name) {
      if (std::any_of(formals.begin(), formals.end(), [&](const auto& formal) { return formal.first == key(name); }))
        fail("shadowed runtime function locals are unsupported");
      localBindings.declare(key(name));
    };
    std::map<size_t, const vhdl::FileDeclaration*> files;
    for (const auto& file : body.files) files.emplace(file.span.start.offset, &file);
    const auto openFiles = [&](size_t before) {
      while (!files.empty() && files.begin()->first < before) {
        const auto& file = *files.begin()->second;
        DiagnosticScope location(file.span);
        if (!initializerContext_ || !frame.textio_ || key(file.type) != "text" || key(file.mode) != "read_mode" || !file.path)
          fail("only read-only text files in static initializers are supported");
        if (declaration.pure) fail("file operations require an impure function");
        const auto path = frame.stringValue(*file.path);
        for (const auto& name : file.names) {
          declareLocal(name);
          auto stream = std::make_shared<std::ifstream>(path, std::ios::binary);
          if (!stream->is_open()) fail("cannot open initialization file: " + path);
          std::string content((std::istreambuf_iterator<char>(*stream)), std::istreambuf_iterator<char>());
          if (stream->bad()) fail("cannot read initialization file: " + path);
          auto [dependency, inserted] = initializationFiles_->emplace(path, content);
          if (!inserted && dependency->second != content) fail("initialization file changed during elaboration: " + path);
          stream->clear();
          stream->seekg(0);
          frame.files_.emplace(key(name), std::move(stream));
        }
        files.erase(files.begin());
      }
    };
    for (const auto& [offset, entry] : locals) {
      openFiles(offset);
      const auto& object = *entry.first;
      const bool variable = std::any_of(body.variables.begin(), body.variables.end(),
          [&](const auto& candidate) { return &candidate == &object; });
      if (!variable) {
        for (const auto& name : object.names) declareLocal(name);
        frame.addConstant(*std::find_if(body.constants.begin(), body.constants.end(),
            [&](const auto& constant) { return &constant.object == &object; }));
        continue;
      }
      if (key(object.type.name) == "line") {
        if (!initializerContext_ || !frame.textio_ || entry.second) fail("line variables require a static TextIO initializer");
        for (const auto& name : object.names) { declareLocal(name); frame.lines_.emplace(key(name), std::nullopt); }
        continue;
      }
      const auto type = frame.functionShape(object.type);
      const auto initial = entry.second ? frame.expression(*entry.second, state, &type).bits : Bits(type.size(), nullptr);
      for (const auto& name : object.names) {
        declareLocal(name);
        frame.objects_.emplace(key(name), Object{{type, {}}, false, false, true});
        state.variables[key(name)] = initial;
      }
    }
    openFiles(std::numeric_limits<size_t>::max());
    const auto result = initializerContext_ ? frame.initializerStatements(body.statements, state).returned :
        frame.runtimeStatements(body.statements, state);
    if (declaration.procedure) {
      if (result) fail("procedure cannot return a value");
      for (const auto& [position, output] : outputs) {
        const auto& bits = state.variables.at(formals[position].first);
        if (std::any_of(bits.begin(), bits.end(), [](auto* bit) { return !bit; }))
          fail("procedure output is not definitely assigned");
        auto& actual = updatedState->variables.at(output.name);
        std::copy(bits.begin(), bits.end(), actual.begin() + output.offset);
        if (updatedState->variableChoices.contains(output.name))
          for (size_t i = 0; i < bits.size(); ++i) updatedState->variableChoices.at(output.name)[output.offset+i].reset();
      }
      return {};
    }
    if (!result) fail("function completed without returning a value: " + key(prefix));
    const auto type = frame.functionShape(declaration.returnType, &result->shape, true);
    if (!compatible(type, result->shape)) fail("function return type or length mismatch");
    return {type, result->bits};
  }

  bool staticBooleanType(const Expr& expression, StaticScope* scope) {
    if (const auto selected = booleanSelection(expression, scope)) {
      for (size_t i = 0; i < selected->indices.size(); ++i) {
        const auto enumeration = selected->object->value.shape.ranges[i].indexEnumeration;
        const auto actualEnumeration = enumIndexType(*selected->indices[i], enumeration);
        if (enumeration) {
          if (actualEnumeration != enumeration) fail("array index enumeration type mismatch");
        } else if (actualEnumeration || staticBooleanType(*selected->indices[i], scope))
          fail("array index must have integer type");
      }
      return true;
    }
    const auto scalarType = [](const std::string& name) {
      if (name == "boolean") return true;
      if (name == "integer" || name == "natural" || name == "positive") return false;
      fail("unsupported static scalar type: " + name);
    };
    if (expression.kind == Expr::Kind::IntegerLiteral) return false;
    if (!scope) {
      if (const auto value = stringAttribute(expression)) return value->boolean;
      if (expression.kind == Expr::Kind::Binary && staticString(*expression.left) && staticString(*expression.right))
        return expression.text == "=" || expression.text == "/=" || expression.text == "<" ||
            expression.text == "<=" || expression.text == ">" || expression.text == ">=";
    }
    if (arrayAttribute(expression)) return attributeScalar(expression, scope).boolean;
    if (expression.kind == Expr::Kind::Name) {
      const auto name = key(expression);
      if (scope) {
        if (const auto found = scope->values.find(name); found != scope->values.end()) return found->second.boolean;
        if (scope->arrays.contains(name)) fail("static scalar expression requires an array element");
        if (scope->environment) {
          const auto found = scope->environment->values.find(name);
          if (found != scope->environment->values.end()) return found->second.boolean;
        }
        if (scope->package) for (const auto& constant : scope->package->constants)
          for (const auto& id : constant.object.names)
            if (key(id) == name) return scalarType(key(resolveSubtype(constant.object.type).name));
      } else {
        if (integers_.contains(name)) return false;
        if (booleans_.contains(name)) return true;
        if (const auto found = objects_.find(name); found != objects_.end() &&
            found->second.value.shape.enumeration) return false;
      }
      if (const auto function = staticFunction(name, scope); function.second)
        return scalarType(key(resolveSubtype(function.second->returnType).name));
      if (!scope && objects_.contains(name))
        return objects_.at(name).value.shape.types == std::vector<std::string>{"boolean"};
      if (name == "true" || name == "false") return true;
    } else if ((expression.kind == Expr::Kind::Call || expression.kind == Expr::Kind::Indexed) &&
               expression.left->kind == Expr::Kind::Name) {
      if (expression.kind == Expr::Kind::Indexed && key(*expression.left) == "boolean") {
        if (!staticBooleanType(*expression.right, scope)) fail("boolean conversion requires a boolean operand");
        return true;
      }
      if (expression.kind == Expr::Kind::Indexed && staticArray(key(*expression.left), scope)) {
        if (staticBooleanType(*expression.right, scope)) fail("array index must have integer type");
        return false;
      }
      if (const auto function = staticFunction(key(*expression.left), scope); function.second)
        return scalarType(key(resolveSubtype(function.second->returnType).name));
    } else if (expression.kind == Expr::Kind::Unary) {
      const bool operand = staticBooleanType(*expression.left, scope);
      if (expression.text == "not" && operand) return true;
      if ((expression.text == "+" || expression.text == "-" || expression.text == "abs") && !operand) return false;
      fail("static unary operand type mismatch");
    } else if (expression.kind == Expr::Kind::Binary) {
      const bool left = staticBooleanType(*expression.left, scope);
      const bool right = staticBooleanType(*expression.right, scope);
      if (left != right) fail("static operand type mismatch");
      const auto& op = expression.text;
      if (op == "=" || op == "/=" || op == "<" || op == "<=" || op == ">" || op == ">=") return true;
      if (op == "and" || op == "or" || op == "nand" || op == "nor" || op == "xor" || op == "xnor") {
        if (!left) fail("static logical operands must be boolean");
        return true;
      }
      if (!left && (op == "+" || op == "-" || op == "*" || op == "/" || op == "mod" || op == "rem" || op == "**"))
        return false;
    }
    fail("unsupported static scalar expression type");
  }

  StaticScalar scalar(const Expr& expression, StaticScope* scope = nullptr) {
    DiagnosticScope location(expression.span);
    if (++steps_ > 100000) fail("static expression evaluation limit exceeded");
    if (const auto selected = booleanSelection(expression, scope)) {
      size_t offset = 0, stride = selected->object->value.shape.size();
      for (size_t i = 0; i < selected->indices.size(); ++i) {
        const auto& bounds = selected->object->value.shape.ranges[i];
        stride /= bounds.size();
        if (scope && bounds.indexEnumeration) fail("enumeration-indexed arrays in static functions are unsupported");
        const auto index = scope ? staticInteger(*selected->indices[i], scope) : arrayIndex(*selected->indices[i], bounds);
        offset += bounds.position(index) * stride;
      }
      const auto value = constantValue(selected->object->value.bits.at(offset));
      if (!value) fail("boolean constant element must be binary");
      return {*value, true};
    }
    if (!scope) {
      if (const auto value = stringAttribute(expression)) return *value;
      if (expression.kind == Expr::Kind::Binary && staticString(*expression.left) && staticString(*expression.right)) {
        const auto left = stringValue(*expression.left), right = stringValue(*expression.right);
        const auto& op = expression.text;
        if (op == "=") return {left == right, true};
        if (op == "/=") return {left != right, true};
        if (op == "<") return {left < right, true};
        if (op == "<=") return {left <= right, true};
        if (op == ">") return {left > right, true};
        if (op == ">=") return {left >= right, true};
      }
    }
    if (arrayAttribute(expression)) return attributeScalar(expression, scope);
    if (expression.kind == Expr::Kind::Name) {
      const auto name = key(expression);
      if (enumLiterals_.contains(name) && enumLiterals_.at(name).size() > 1)
        fail("ambiguous enumeration literal: " + name);
      if (scope) {
        if (const auto found = scope->values.find(name); found != scope->values.end()) return found->second;
        if (scope->arrays.contains(name)) fail("static scalar expression requires an array element");
        if (scope->environment) {
          const auto found = scope->environment->values.find(name);
          if (found != scope->environment->values.end()) return found->second;
        }
        if (scope->package) for (const auto& declaration : scope->package->constants)
          for (const auto& id : declaration.object.names) if (key(id) == name) {
            if (libraryContext_.precedes(expression.span.start.offset, declaration.object.span.start.offset))
              fail("package constant used before its declaration");
            if (!activeStaticConstants_.insert(&declaration).second) fail("cyclic static constant");
            StaticScope packageScope;
            packageScope.package = scope->package;
            const auto value = scalar(*declaration.value, &packageScope);
            checkStaticType(value, declaration.object.type, &packageScope);
            activeStaticConstants_.erase(&declaration);
            return value;
          }
      } else {
        if (const auto found = integers_.find(name); found != integers_.end()) return {found->second, false};
        if (const auto found = booleans_.find(name); found != booleans_.end()) return {found->second, true};
        if (const auto found = objects_.find(name); found != objects_.end() && found->second.constant &&
            found->second.value.shape.enumeration) {
          int64_t value = 0;
          for (size_t i = 0; i < found->second.value.bits.size(); ++i) {
            const auto bit = constantValue(found->second.value.bits[i]);
            if (!bit) fail("enumeration literal is not static");
            if (*bit) value |= int64_t(1) << i;
          }
          return {value, false};
        }
      }
      if (staticFunction(name, scope).second) return callStatic(expression, scope);
      if (!scope && objects_.contains(name)) fail("static scalar expression refers to a runtime object");
      if (name == "true" || name == "false") return {name == "true", true};
    } else if ((expression.kind == Expr::Kind::Indexed || expression.kind == Expr::Kind::Call) &&
               expression.left->kind == Expr::Kind::Name) {
      const auto builtin = key(*expression.left);
      const auto& arguments = expression.kind == Expr::Kind::Indexed ?
          std::vector<const Expr*>{expression.right.get()} : [&] { std::vector<const Expr*> result; for (const auto& e : expression.elements) result.push_back(e.get()); return result; }();
      if ((builtin == "minimum" || builtin == "maximum") && arguments.size() == 2 &&
          !staticFunction(builtin, scope).second) {
        if ((!scope && (objects_.contains(builtin) || integers_.contains(builtin) || booleans_.contains(builtin))) ||
            (scope && scope->values.contains(builtin))) fail("shadowed scalar minimum/maximum function");
        const auto left = scalar(*arguments[0], scope), right = scalar(*arguments[1], scope);
        if (left.boolean != right.boolean) fail("minimum/maximum operand type mismatch");
        return (builtin == "minimum" ? left.value < right.value : left.value > right.value) ? left : right;
      }
      if (builtin == "index_size_f" && arguments.size() == 1) {
        const auto n = scalar(*arguments[0], scope);
        if (n.boolean || n.value < 1) fail("index_size_f requires a positive integer");
        int64_t width = 0; for (int64_t v = n.value - 1; v; v >>= 1) ++width;
        return {std::max<int64_t>(1, width), false};
      }
      if (builtin == "sel_natural_f" && arguments.size() == 3) {
        const auto condition = scalar(*arguments[0], scope);
        if (!condition.boolean) fail("sel_natural_f condition must be boolean");
        const auto selected = scalar(*(condition.value ? arguments[1] : arguments[2]), scope);
        if (selected.boolean) fail("sel_natural_f branch must be natural");
        return selected;
      }
      if (expression.kind == Expr::Kind::Indexed && key(*expression.left) == "boolean") {
        if (!staticBooleanType(*expression.right, scope)) fail("boolean conversion requires a boolean operand");
        return scalar(*expression.right, scope);
      }
      if (expression.kind == Expr::Kind::Indexed) {
        if (const auto* table = staticArray(key(*expression.left), scope))
          return {table->values.at(table->bounds.position(staticInteger(*expression.right, scope))), false};
      }
      return callStatic(expression, scope);
    } else if (expression.kind == Expr::Kind::IntegerLiteral) {
      std::string digits;
      for (char c : expression.text) if (c != '_') digits += c;
      unsigned base = 10;
      size_t first = 0, last = digits.find_first_of("eE");
      const auto separator = digits.find('#');
      if (separator != std::string::npos) {
        const auto parsed = std::from_chars(digits.data(), digits.data() + separator, base);
        if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + separator || base < 2 || base > 16)
          fail("invalid based integer literal base");
        first = separator + 1;
        last = digits.find('#', first);
        if (last == std::string::npos) fail("unterminated based integer literal");
      } else if (last == std::string::npos) last = digits.size();
      int128_t value = 0;
      if (first == last) fail("integer literal requires digits");
      for (size_t i = first; i < last; ++i) {
        const char c = digits[i];
        const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
            c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
        if (digit >= base) fail("invalid based integer literal digit");
        value = value * base + digit;
        if (value > INT64_MAX) fail("integer literal overflow");
      }
      const size_t exponent = separator == std::string::npos ? last : last + 1;
      if (exponent < digits.size()) {
        if (digits[exponent] != 'e' && digits[exponent] != 'E') fail("invalid integer literal exponent");
        size_t start = exponent + 1;
        if (start < digits.size() && digits[start] == '+') ++start;
        unsigned power = 0;
        const auto parsed = std::from_chars(digits.data() + start, digits.data() + digits.size(), power);
        if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size() || power > 65536)
          fail("integer literal exponent must be a supported nonnegative value");
        for (unsigned i = 0; i < power && value; ++i) {
          value *= base;
          if (value > INT64_MAX) fail("integer literal overflow");
        }
      }
      return {int64_t(value), false};
    } else if (expression.kind == Expr::Kind::Unary || expression.kind == Expr::Kind::Binary) {
      const auto left = scalar(*expression.left, scope);
      const auto& op = expression.text;
      int128_t value;
      if (expression.kind == Expr::Kind::Unary) {
        if (op == "not" && left.boolean) return {!left.value, true};
        if (left.boolean || (op != "+" && op != "-" && op != "abs")) fail("unsupported static unary operand");
        value = left.value;
        if (op == "-" || (op == "abs" && value < 0)) value = -value;
      } else {
        if (left.boolean && (((op == "and" || op == "nand") && !left.value) ||
            ((op == "or" || op == "nor") && left.value))) {
          if (!staticBooleanType(*expression.right, scope)) fail("static operand type mismatch");
          return {op == "nand" || op == "or", true};
        }
        const auto right = scalar(*expression.right, scope);
        if (left.boolean != right.boolean) fail("static operand type mismatch");
        if (op == "=") return {left.value == right.value, true};
        if (op == "/=") return {left.value != right.value, true};
        if (op == "<") return {left.value < right.value, true};
        if (op == "<=") return {left.value <= right.value, true};
        if (op == ">") return {left.value > right.value, true};
        if (op == ">=") return {left.value >= right.value, true};
        if (left.boolean) {
          if (op == "and" || op == "nand") return {(left.value && right.value) != (op == "nand"), true};
          if (op == "or" || op == "nor") return {(left.value || right.value) != (op == "nor"), true};
          if (op == "xor" || op == "xnor") return {(left.value != right.value) != (op == "xnor"), true};
          fail("unsupported static boolean operator");
        }
        const int128_t a = left.value, b = right.value;
        if (op == "+") value = a + b;
        else if (op == "-") value = a - b;
        else if (op == "*") value = a * b;
        else if (op == "/" || op == "mod" || op == "rem") {
          if (!b) fail("division by zero in static expression");
          value = op == "/" ? a / b : a % b;
          if (op == "mod" && value != 0 && ((value < 0) != (b < 0))) value += b;
        } else if (op == "**") {
          if (b < 0 || b > 65536) fail("unsupported static exponent");
          value = 1;
          for (int64_t i = 0; i < right.value; ++i) {
            if (++steps_ > 100000) fail("static expression evaluation limit exceeded");
            value *= a;
            if (value < std::numeric_limits<int64_t>::min() || value > std::numeric_limits<int64_t>::max())
              fail("static integer overflow");
          }
        } else fail("unsupported static integer operator");
      }
      if (value < std::numeric_limits<int64_t>::min() || value > std::numeric_limits<int64_t>::max())
        fail("static integer overflow");
      return {static_cast<int64_t>(value), false};
    }
    fail("expression requires a supported static integer or boolean value");
  }

  int64_t integer(const Expr& expression) {
    return staticInteger(expression, nullptr);
  }

  const vhdl::EnumerationTypeDeclaration* enumIndexType(const Expr& expr,
      const vhdl::EnumerationTypeDeclaration* expected = nullptr) const {
    if (expr.kind != Expr::Kind::Name) return nullptr;
    if (const auto found = enumLiterals_.find(key(expr)); found != enumLiterals_.end()) {
      if (expected) for (const auto& value : found->second) if (value.shape.enumeration == expected) return expected;
      if (found->second.size() > 1) fail("ambiguous enumeration literal: " + key(expr));
    }
    const auto found = objects_.find(key(expr));
    if (found == objects_.end() || !found->second.value.shape.ranges.empty()) return nullptr;
    return found->second.value.shape.enumeration;
  }

  void checkIndexType(const Value& value, const Range& bounds) {
    if (bounds.indexEnumeration) {
      if (!value.shape.ranges.empty() || value.shape.enumeration != bounds.indexEnumeration)
        fail("array index enumeration type mismatch");
    } else if (!value.shape.integerWidth) fail("array index must have integer type");
  }

  int64_t arrayIndex(const Expr& expr, const Range& bounds) {
    if (!bounds.indexEnumeration) return integer(expr);
    if (!isStaticIndex(expr)) fail("array selection requires a static enumeration index");
    const Shape expected{{key(bounds.indexEnumeration->name)}, {}, 0, {}, bounds.indexEnumeration};
    const auto value = expression(expr, State{}, &expected);
    checkIndexType(value, bounds);
    int64_t ordinal = 0;
    for (auto* bit : value.bits) {
      const auto binary = constantValue(bit);
      if (!binary) fail("array selection requires a static enumeration index");
      ordinal = ordinal * 2 + *binary;
    }
    if (ordinal >= int64_t(bounds.indexEnumeration->literals.size()))
      fail("invalid enumeration index value");
    return ordinal;
  }

  Range enumRange(const vhdl::DiscreteRange& constraint,
                  const vhdl::EnumerationTypeDeclaration* type) {
    if (constraint.attribute || !constraint.leftExpression || !constraint.rightExpression)
      fail("enumeration array bounds require explicit enumeration values");
    Range bounds{0, int64_t(type->literals.size()) - 1, true, type};
    return {arrayIndex(*constraint.leftExpression, bounds), arrayIndex(*constraint.rightExpression, bounds),
            constraint.ascending, type};
  }

  Range arrayBounds(const vhdl::ArrayTypeDeclaration& declaration) {
    if (declaration.indexType) {
      const auto found = enumerations_.find(key(*declaration.indexType));
      if (found == enumerations_.end()) {
        const auto type = resolveSubtype(vhdl::TypeMark{*declaration.indexType, std::nullopt});
        const auto name = key(type.name);
        if (!type.constraint || (name != "integer" && name != "natural" && name != "positive"))
          fail("array index type must name a visible enumeration or constrained integer subtype");
        return range(*type.constraint);
      }
      return {0, int64_t(found->second->literals.size()) - 1, true, found->second};
    }
    if (declaration.indexSubtype) {
      const auto found = enumerations_.find(key(*declaration.indexSubtype));
      return {0, -1, true, found == enumerations_.end() ? nullptr : found->second};
    }
    if (declaration.indexRange.leftExpression) {
      if (const auto type = enumIndexType(*declaration.indexRange.leftExpression))
        return enumRange(declaration.indexRange, type);
    }
    return range(declaration.indexRange);
  }

  Range range(const vhdl::DiscreteRange& range) {
    DiagnosticScope location(range.span);
    if (range.subtype) {
      const auto type = resolveSubtype(vhdl::TypeMark{*range.subtype, std::nullopt});
      const auto name = key(type.name);
      if (!type.constraint || (name != "integer" && name != "natural" && name != "positive"))
        fail("range requires a constrained integer subtype");
      return this->range(*type.constraint);
    }
    if (range.attribute) {
      const auto* attribute = arrayAttribute(*range.attribute);
      if (!attribute || (key(*attribute) != "range" && key(*attribute) != "reverse_range"))
        fail("expected range or reverse_range array attribute"); // LCOV_EXCL_LINE: Parser::parseDiscreteRange stores only these two attributes.
      auto bounds = attributeRange(*range.attribute);
      if (bounds.indexEnumeration) fail("enumeration-valued array range attributes are not supported");
      if (key(*attribute) == "reverse_range") {
        std::swap(bounds.left, bounds.right);
        bounds.ascending = !bounds.ascending;
      }
      return bounds;
    }
    return {range.leftExpression ? integer(*range.leftExpression) : range.left,
            range.rightExpression ? integer(*range.rightExpression) : range.right,
            range.ascending};
  }

  Shape shape(const vhdl::TypeMark& original) {
    const bool alias = subtypes_.contains(key(original.name));
    const auto type = resolveSubtype(original);
    DiagnosticScope location(type.name.span);
    auto name = key(type.name);
    if ((stdLogic_ || alias) && name == "std_ulogic") name = "std_logic";
    if ((stdLogic_ || alias) && name == "std_ulogic_vector") name = "std_logic_vector";
    if (const auto found = enumerations_.find(name); found != enumerations_.end()) {
      if (type.constraint) fail("enumeration subtype constraints are not supported");
      if (libraryContext_.precedes(type.name.span.start.offset, found->second->span.start.offset))
        fail("enumeration used before its declaration");
      return {{name}, {}, 0, {}, found->second};
    }
    if (const auto found = records_.find(name); found != records_.end()) {
      if (type.constraint) fail("record type cannot have an array constraint");
      if (libraryContext_.precedes(type.name.span.start.offset, recordOffsets_.at(name)))
        fail("record type is used before its declaration: " + name);
      return found->second;
    }
    if (const auto found = arrays_.find(name); found != arrays_.end()) {
      if (libraryContext_.precedes(type.name.span.start.offset, arrayTypeOffsets_.at(name)))
        fail("array type is used before its declaration: " + name);
      auto result = found->second;
      const auto& declaration = *arrayDeclarations_.at(name);
      if (declaration.indexSubtype) {
        if (!type.constraint) fail("unconstrained array object requires bounds");
        Expr unused;
        result.ranges.front() = constantBounds(declaration, type, unused);
      } else if (type.constraint) fail("reconstraining an array type is not supported");
      return result;
    }
    if (name == "integer" || name == "natural" || name == "positive") {
      const auto bounds = type.constraint ? range(*type.constraint) :
          Range{name == "integer" ? INT32_MIN : name == "positive" ? 1 : 0, INT32_MAX, true};
      if (!bounds.ascending || bounds.left < INT32_MIN || bounds.right < bounds.left || bounds.right > INT32_MAX)
        fail("unsupported hardware integer range");
      size_t width = 1;
      while (bounds.left >= 0 && (uint64_t(1) << width) <= uint64_t(bounds.right)) ++width;
      Shape result{{"integer"}, {}, bounds.left < 0 ? 32 : width};
      result.signedInteger = bounds.left < 0;
      return result;
    }
    if (name == "boolean" || name == "bit" || ((stdLogic_ || alias) && name == "std_logic")) {
      if (type.constraint) fail("scalar type cannot have a range");
      return {{name}, {}};
    }
    if (name == "bit_vector" || ((stdLogic_ || alias) && name == "std_logic_vector") ||
        ((numeric_ || alias) && (name == "unsigned" || name == "signed"))) {
      if (!type.constraint) fail("vector requires a constraint");
      return {{name, name == "bit_vector" ? "bit" : "std_logic"}, {range(*type.constraint)}};
    }
    fail("unsupported or invisible RTL type: " + name);
  }

  void addObject(const vhdl::Name& name, const Shape& type, bool input = false,
                 bool output = false, bool variable = false, bool port = false, bool deferred = false) {
    const auto id = key(name);
    if (port && type.ranges.size() > 1)
      fail("RTL ports require scalar values or one-dimensional logic vectors");
    if (strings_.contains(id) || functions_.contains(id) || booleans_.contains(id) || objects_.contains(id) || integers_.contains(id) || (enumerations_.contains(id) || records_.contains(id)) || arrayDeclarations_.contains(id) || integerTables_.contains(id))
      fail("duplicate or shadowing object: " + id);
    Bits bits;
    const auto size = type.size();
    if (deferred) {} // Inference materializes only arrays that require registers.
    else if (variable) bits.resize(size, nullptr);
    else if (!type.ranges.empty() || type.integerWidth || type.enumeration || !type.fields.empty()) {
      const auto r = (type.integerWidth || type.enumeration || !type.fields.empty()) ? Range{int64_t(size - 1), 0, false} : type.ranges.back();
      if (r.left < std::numeric_limits<NLID::Bit>::min() || r.left > std::numeric_limits<NLID::Bit>::max() ||
          r.right < std::numeric_limits<NLID::Bit>::min() || r.right > std::numeric_limits<NLID::Bit>::max())
        fail("hardware vector bounds exceed net index range");
      // Keep each innermost array element as a bus, with its declared bounds.
      for (size_t offset = 0; offset < size; offset += r.size()) {
        auto busName = objectPrefix_ + name.spelling;
        auto position = offset;
        auto stride = size;
        for (size_t dimension = 0; !type.enumeration && type.fields.empty() && dimension + 1 < type.ranges.size(); ++dimension) {
          const auto& bounds = type.ranges[dimension];
          stride /= bounds.size();
          const auto index = static_cast<int64_t>(position / stride);
          busName += "(" + std::to_string(bounds.ascending ? bounds.left + index : bounds.left - index) + ")";
          position %= stride;
        }
        auto* net = SNLBusNet::create(design_, r.left, r.right, NLName(busName));
        declaration(net, name.span);
        attachObjectIntent(net, name, type);
        for (size_t i = 0; i < r.size(); ++i) bits.push_back(net->getBitAtPosition(i));
        if (port) {
          auto* term = declaration(SNLBusTerm::create(design_, input ? SNLTerm::Direction::Input :
              SNLTerm::Direction::Output, r.left, r.right, NLName(name.spelling)), name.span);
          term->setNet(net);
          attachObjectIntent(term, name, type);
        }
      }
    } else {
      for (size_t i = 0; i < size; ++i) {
        auto* bit = SNLScalarNet::create(design_, NLName(objectPrefix_ + name.spelling +
            (size == 1 ? "" : "_" + std::to_string(i))));
        declaration(bit, name.span);
        attachObjectIntent(bit, name, type);
        bits.push_back(bit);
      }
      if (port) {
        auto* term = declaration(SNLScalarTerm::create(design_, input ? SNLTerm::Direction::Input :
            SNLTerm::Direction::Output, NLName(name.spelling)), name.span);
        term->setNet(bits.front());
        attachObjectIntent(term, name, type);
      }
    }
    objects_.emplace(id, Object{{type, std::move(bits)}, input, output, variable});
  }

  Selection selection(const std::string& name) {
    if (const auto alias = aliases_.find(name); alias != aliases_.end()) return alias->second;
    const auto found = objects_.find(name);
    if (found == objects_.end()) fail("no declaration for object: " + name);
    return {name, found->second.value.shape, 0};
  }
  void field(Selection& selected, const std::string& name) {
    if (!selected.shape.ranges.empty() || selected.shape.fields.empty())
      fail("selected prefix is not a record");
    size_t offset = 0;
    for (const auto& [fieldName, type] : selected.shape.fields) {
      if (fieldName == name) {
        auto result = *type;
        selected.offset += offset;
        selected.shape = std::move(result);
        return;
      }
      offset += type->size();
    }
    fail("no record field: " + name);
  }
  std::optional<Range> sliceRange(const Expr& expression, const Range& original) {
    if (expression.kind == Expr::Kind::Range)
      return Range{arrayIndex(*expression.left, original), arrayIndex(*expression.right, original),
          expression.text == "to", original.indexEnumeration};
    const auto* attribute = arrayAttribute(expression);
    if (!attribute || (key(*attribute) != "range" && key(*attribute) != "reverse_range")) return std::nullopt;
    auto bounds = attributeRange(expression);
    if (bounds.indexEnumeration != original.indexEnumeration) fail("slice index type mismatch");
    if (key(*attribute) == "reverse_range") { std::swap(bounds.left, bounds.right); bounds.ascending = !bounds.ascending; }
    return bounds;
  }

  void index(Selection& selected, const Expr& expression) {
    if (expression.kind == Expr::Kind::Selected) {
      field(selected, key(expression));
      return;
    }
    if (selected.shape.ranges.empty()) fail("cannot index a scalar object");
    if (const auto bounds = sliceRange(expression, selected.shape.ranges.front())) {
      const auto& original = selected.shape.ranges.front();
      const auto slice = *bounds;
      if (slice.ascending != original.ascending || !slice.size()) fail("invalid slice direction or range");
      const auto position = original.position(slice.left);
      original.position(slice.right);
      const auto stride = selected.shape.size() / original.size();
      selected.offset += position * stride;
      selected.shape.ranges.front() = slice;
      return;
    }
    const auto position = selected.shape.ranges.front().position(arrayIndex(expression, selected.shape.ranges.front()));
    selected.shape.ranges.erase(selected.shape.ranges.begin());
    selected.shape.removeOuterType();
    selected.offset += position * selected.shape.size();
  }
  Selection selection(const Expr& expression) {
    if (expression.kind == Expr::Kind::Name) return selection(key(expression));
    if (expression.kind == Expr::Kind::Selected) {
      auto selected = selection(*expression.left);
      field(selected, key(expression));
      return selected;
    }
    if (expression.kind != Expr::Kind::Indexed) fail("indexed prefix must name an object");
    auto selected = selection(*expression.left);
    index(selected, *expression.right);
    return selected;
  }
  Selection selection(const vhdl::Assignment& assignment) {
    auto selected = selection(key(assignment.target));
    for (const auto& expression : assignment.indices) index(selected, *expression);
    return selected;
  }

  SNLBitNet* constant(bool one) {
    auto*& net = one ? one_ : zero_;
    if (!net) {
      net = SNLScalarNet::create(design_);
      net->setType(one ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0);
    }
    return origin(net);
  }
  SNLBitNet* logicLiteral(char value, const Shape& type) {
    if (value == '0' || value == '1') return constant(value == '1');
    if (type.types.back() != "std_logic" || (value != 'X' && value != 'Z'))
      fail("RTL literals require '0' or '1', or 'X'/'Z' for std_logic elements");
    auto*& net = value == 'X' ? unknown_ : highImpedance_;
    if (!net) {
      net = SNLScalarNet::create(design_);
      net->setType(value == 'X' ? SNLNet::Type::AssignX : SNLNet::Type::AssignZ);
    }
    return origin(net);
  }
  std::optional<bool> constantValue(SNLBitNet* net) {
    if (net && net->isConstant0()) return false;
    if (net && net->isConstant1()) return true;
    return std::nullopt;
  }
  Bits mux(SNLBitNet* condition, const Bits& yes, const Bits& no) {
    if (auto value = constantValue(condition)) return *value ? yes : no;
    const auto cacheKey = std::make_tuple(condition, yes, no);
    if (const auto found = muxes_.find(cacheKey); found != muxes_.end()) {
      for (auto* bit : found->second) if (bit) reuse(bit);
      return found->second;
    }
    Bits result(yes.size());
    for (size_t first = 0; first < yes.size();) {
      if (yes[first] == no[first] || !yes[first] || !no[first]) {
        // Preserve unavailable variables and avoid muxes for unchanged bits.
        result[first] = yes[first] == no[first] ? yes[first] : nullptr;
        ++first;
        continue;
      }
      size_t end = first + 1;
      while (end < yes.size() && yes[end] && no[end] && yes[end] != no[end]) ++end;
      const auto width = end - first;
      SNLNet* output = width == 1 ? static_cast<SNLNet*>(origin(SNLScalarNet::create(design_)))
          : origin(SNLBusNet::create(design_, width - 1, 0));
      // RTL values follow declaration order; canonical primitive inputs are LSB first.
      Bits a(no.begin() + first, no.begin() + end), b(yes.begin() + first, yes.begin() + end);
      std::reverse(a.begin(), a.end());
      std::reverse(b.begin(), b.end());
      origin(SNLRTLPrimitives::createMux(design_, condition, a, b, output));
      for (size_t i = first; i < end; ++i)
        result[i] = width == 1 ? static_cast<SNLBitNet*>(output)
            : static_cast<SNLBusNet*>(output)->getBitAtPosition(i - first);
      first = end;
    }
    muxes_.emplace(cacheKey, result);
    return result;
  }
  SNLBitNet* cachedGate(const std::string& op, Bits inputs) {
    // All supported multi-input gates are commutative.
    std::sort(inputs.begin(), inputs.end(), [](const auto* left, const auto* right) {
      return left->getNLID() < right->getNLID();
    });
    const auto cacheKey = std::make_pair(op, inputs);
    if (const auto found = gates_.find(cacheKey); found != gates_.end()) return reuse(found->second);
    using Gate = SNLRTLPrimitives::GateKind;
    static const std::map<std::string, Gate> kinds{{"and", Gate::And}, {"or", Gate::Or},
        {"xor", Gate::Xor}, {"nand", Gate::Nand}, {"nor", Gate::Nor},
        {"xnor", Gate::Xnor}, {"not", Gate::Not}};
    auto* output = origin(SNLScalarNet::create(design_));
    origin(SNLRTLPrimitives::createGate(design_, kinds.at(op),
        std::vector<SNLNet*>(inputs.begin(), inputs.end()), output));
    gates_.emplace(cacheKey, output);
    return output;
  }
  SNLBitNet* gate(const std::string& op, SNLBitNet* a, SNLBitNet* b = nullptr) {
    using Gate = SNLRTLPrimitives::GateKind;
    static const std::map<std::string, Gate> kinds{{"and", Gate::And}, {"or", Gate::Or},
        {"xor", Gate::Xor}, {"nand", Gate::Nand}, {"nor", Gate::Nor},
        {"xnor", Gate::Xnor}, {"not", Gate::Not}};
    const auto kind = kinds.find(op);
    if (kind == kinds.end()) fail("unsupported RTL operator: " + op);
    const auto av = constantValue(a), bv = constantValue(b);
    if (av && (bv || op == "not")) {
      bool value = op == "not" ? !*av : op == "and" || op == "nand" ? *av && *bv :
          op == "or" || op == "nor" ? *av || *bv : *av != *bv;
      if (op == "nand" || op == "nor" || op == "xnor") value = !value;
      return constant(value);
    }
    if (b && (av || bv)) {
      const bool value = av ? *av : *bv;
      auto* other = av ? b : a;
      if (op == "and" || op == "nand") {
        auto* result = value ? other : constant(false);
        return op == "nand" ? gate("not", result) : result;
      }
      if (op == "or" || op == "nor") {
        auto* result = value ? constant(true) : other;
        return op == "nor" ? gate("not", result) : result;
      }
      return (value == (op == "xor")) ? gate("not", other) : other;
    }
    return cachedGate(op, b ? Bits{a, b} : Bits{a});
  }

  Bits add(const Bits& left, const Bits& right, bool subtract) {
    const auto cacheKey = std::make_tuple(left, right, subtract);
    if (const auto found = adders_.find(cacheKey); found != adders_.end()) return found->second;
    Bits result(left.size());
    auto* carry = constant(subtract);
    for (size_t i = left.size(); i; --i) {
      auto* a = left[i - 1];
      auto* b = subtract ? gate("not", right[i - 1]) : right[i - 1];
      auto* sum = origin(SNLScalarNet::create(design_));
      auto* carryOut = i > 1 ? origin(SNLScalarNet::create(design_)) : nullptr;
      auto* instance = origin(SNLInstance::create(design_, NLDB0::getFA()));
      instance->getInstTerm(NLDB0::getFAInputA())->setNet(a);
      instance->getInstTerm(NLDB0::getFAInputB())->setNet(b);
      instance->getInstTerm(NLDB0::getFAInputCI())->setNet(carry);
      instance->getInstTerm(NLDB0::getFAOutputS())->setNet(sum);
      instance->getInstTerm(NLDB0::getFAOutputCO())->setNet(carryOut);
      result[i - 1] = sum;
      carry = carryOut;
    }
    adders_.emplace(cacheKey, result);
    return result;
  }

  bool isStaticIndex(const Expr& expr) const {
    return isStatic(expr) || (expr.kind == Expr::Kind::Name && enumIndexType(expr) && objects_.at(key(expr)).constant);
  }

  bool isStatic(const Expr& expr) const {
    if (expr.kind == Expr::Kind::Binary && staticString(*expr.left) && staticString(*expr.right) &&
        (expr.text == "=" || expr.text == "/=" || expr.text == "<" || expr.text == "<=" || expr.text == ">" || expr.text == ">=")) return true;
    if (const auto selected = booleanSelection(expr))
      return std::all_of(selected->indices.begin(), selected->indices.end(), [&](const auto* index) { return isStaticIndex(*index); });
    if (expr.kind == Expr::Kind::IntegerLiteral) return true;
    if (const auto* attribute = arrayAttribute(expr)) {
      if (key(*attribute) == "val") return false;
      if (key(*attribute) == "pos") return expr.kind == Expr::Kind::Indexed &&
          expr.right->kind == Expr::Kind::Name && enumLiterals_.contains(key(*expr.right));
      return true;
    }
    if (expr.kind == Expr::Kind::Name) return integers_.contains(key(expr)) || booleans_.contains(key(expr)) ||
        (functions_.contains(key(expr)) && scalarFunction(*functions_.at(key(expr)).second)) ||
        ((key(expr) == "true" || key(expr) == "false") && !objects_.contains(key(expr)) && !functions_.contains(key(expr)));
    if (expr.kind == Expr::Kind::Indexed && expr.left->kind == Expr::Kind::Name &&
        key(*expr.left) == "boolean") return isStatic(*expr.right);
    if ((expr.kind == Expr::Kind::Indexed || expr.kind == Expr::Kind::Call) &&
        expr.left->kind == Expr::Kind::Name && functions_.contains(key(*expr.left)) &&
        scalarFunction(*functions_.at(key(*expr.left)).second)) {
      if (expr.kind == Expr::Kind::Indexed) return isStatic(*expr.right);
      return std::all_of(expr.elements.begin(), expr.elements.end(), [&](const auto& argument) {
        return isStatic(argument->kind == Expr::Kind::Association ? *argument->right : *argument);
      });
    }
    if (expr.kind == Expr::Kind::Call && expr.left->kind == Expr::Kind::Name &&
        (key(*expr.left) == "minimum" || key(*expr.left) == "maximum") &&
        !functions_.contains(key(*expr.left)) && expr.elements.size() == 2)
      return std::all_of(expr.elements.begin(), expr.elements.end(), [&](const auto& argument) { return isStatic(*argument); });
    if (expr.kind == Expr::Kind::Indexed && expr.left->kind == Expr::Kind::Name &&
        integerTables_.contains(key(*expr.left))) return isStatic(*expr.right);
    return (expr.kind == Expr::Kind::Unary || expr.kind == Expr::Kind::Binary) &&
        (expr.text == "+" || expr.text == "-" || expr.text == "*" || expr.text == "/" ||
         expr.text == "mod" || expr.text == "rem" || expr.text == "**" || expr.text == "abs" ||
         expr.text == "=" || expr.text == "/=" || expr.text == "<" || expr.text == "<=" ||
         expr.text == ">" || expr.text == ">=" || expr.text == "not" || expr.text == "and" ||
         expr.text == "or" || expr.text == "xor" || expr.text == "nand" || expr.text == "nor" || expr.text == "xnor") &&
        isStatic(*expr.left) && (!expr.right || isStatic(*expr.right));
  }
  Value number(int64_t number, const Shape& type) {
    Value value{type, {}};
    if (number < 0 && !type.signedInteger) fail("negative value requires a signed integer target");
    for (size_t i = type.size(); i; --i)
      value.bits.push_back(constant(i <= 64 && ((uint64_t(number) >> (i - 1)) & 1)));
    return value;
  }
  Bits resize(Bits bits, size_t width) {
    if (bits.size() > width) bits.erase(bits.begin(), bits.end() - width);
    else bits.insert(bits.begin(), width - bits.size(), constant(false));
    return bits;
  }
  Bits integerBits(const Value& value, size_t width) {
    auto bits = value.bits;
    if (value.shape.signedInteger && bits.size() < width)
      bits.insert(bits.begin(), width - bits.size(), bits.front());
    return resize(std::move(bits), width);
  }
  SNLBitNet* equal(const Bits& a, const Bits& b) {
    if (a.size() != b.size()) fail("internal comparison width mismatch");
    Bits matches;
    for (size_t i = 0; i < a.size(); ++i) {
      auto* match = gate("xnor", a[i], b[i]);
      if (const auto value = constantValue(match)) {
        if (!*value) return constant(false);
      } else matches.push_back(match);
    }
    if (matches.empty()) return constant(true);
    if (matches.size() == 1) return matches.front();
    return cachedGate("and", std::move(matches));
  }
  SNLBitNet* compareBits(const Bits& left, const Bits& right, const std::string& op, bool signedOperands = false) {
    if (left.size() != right.size() || left.empty()) fail("internal comparison width mismatch");
    if (op == "=" || op == "/=") {
      auto* result = equal(left, right);
      return op == "=" ? result : gate("not", result);
    }
    const bool reverse = op == ">" || op == "<=";
    const auto& a = reverse ? right : left;
    const auto& b = reverse ? left : right;
    auto* less = constant(false);
    for (size_t i = a.size(); i; --i) {
      auto* x = a[i-1];
      auto* y = b[i-1];
      if (signedOperands && i == 1) {
        x = gate("not", x);
        y = gate("not", y);
      }
      less = gate("or", gate("and", gate("not", x), y), gate("and", gate("xnor", x, y), less));
    }
    return op == "<=" || op == ">=" ? gate("not", less) : less;
  }

  Value numericIntegerOperand(Value operand, const std::optional<int64_t>& literal,
      const Value& vector, bool comparison) {
    const bool signedOperand = vector.shape.types.front() == "signed";
    size_t width = vector.bits.size();
    if (literal) {
      if (*literal < INT32_MIN || *literal > INT32_MAX) fail("numeric operand exceeds the supported integer range");
      if (!signedOperand && *literal < 0) fail("unsigned numeric operators require a natural operand");
      if (comparison) {
        uint64_t magnitude = signedOperand && *literal < 0 ? uint64_t(-(*literal + 1)) : uint64_t(*literal);
        size_t needed = signedOperand ? 1 : 0;
        while (magnitude) { ++needed; magnitude >>= 1; }
        width = std::max(width, std::max<size_t>(1, needed));
      }
      operand.bits.clear();
      for (size_t i = width; i; --i)
        operand.bits.push_back(constant(i > 64 ? *literal < 0 : ((uint64_t(*literal) >> (i-1)) & 1)));
    } else {
      if (operand.shape.signedInteger) fail("dynamic signed integer numeric-vector operands are unsupported");
      if (!operand.shape.integerWidth) fail("numeric scalar operand must have integer type");
      if (comparison) width = std::max(width, operand.bits.size() + size_t(signedOperand));
      operand.bits = resize(std::move(operand.bits), width);
    }
    operand.shape = vectorShape(width);
    operand.shape.types.front() = vector.shape.types.front();
    return operand;
  }
  Shape vectorShape(size_t width, const std::string& scalar = "std_logic") {
    if (!width) fail("empty vector");
    return {{scalar == "bit" ? "bit_vector" : "std_logic_vector", scalar},
            {{int64_t(width - 1), 0, false}}};
  }
  std::shared_ptr<VariableChoice> variableChoice(const State& state, const std::string& name, size_t position) {
    if (const auto found = state.variableChoices.find(name); found != state.variableChoices.end() && found->second[position])
      return found->second[position];
    auto* bit = state.variables.at(name)[position];
    return bit ? std::make_shared<VariableChoice>(VariableChoice{bit}) : nullptr;
  }
  Value conditionValue(Value value) {
    if (!value.shape.ranges.empty() || !value.shape.fields.empty() ||
        value.shape.enumeration || value.shape.integerWidth || value.bits.size() != 1 ||
        (value.shape.types.front() != "boolean" && value.shape.types.front() != "bit" &&
         value.shape.types.front() != "std_logic"))
      fail("condition requires boolean, bit, or std_ulogic scalar type");
    if (value.bits.front()->isConstantX() || value.bits.front()->isConstantZ())
      value.bits.front() = constant(false);
    value.shape = {{"boolean"}, {}};
    return value;
  }
  std::optional<bool> knownCondition(SNLBitNet* condition, const State& state) {
    if (auto value = constantValue(condition)) return value;
    const auto found = state.assumptions.find(condition);
    return found == state.assumptions.end() ? std::nullopt : std::optional<bool>{found->second};
  }
  SNLBitNet* resolveVariableChoice(const std::shared_ptr<VariableChoice>& choice, const State& state, size_t depth = 0) {
    if (!choice) return nullptr;
    if (choice->bit) return choice->bit;
    if (depth > 256) fail("guarded variable dependency limit exceeded");
    const auto selected = knownCondition(choice->condition, state);
    return selected ? resolveVariableChoice(*selected ? choice->yes : choice->no, state, depth+1) : nullptr;
  }
  void materializeVariableChoices(State& state) {
    for (auto& [name, choices] : state.variableChoices) for (size_t i = 0; i < choices.size(); ++i)
      if (auto* bit = resolveVariableChoice(choices[i], state)) { state.variables.at(name)[i] = bit; choices[i].reset(); }
  }
  void assume(State& state, SNLBitNet* condition, bool value) {
    state.assumptions[condition] = value;
    materializeVariableChoices(state);
  }

  Value readObject(const std::string& name, const State& state) {
    materializeSignal(name);
    const auto& object = objects_.at(name);
    if (memories_.contains(name)) {
      Value value{object.value.shape, {}};
      const auto& bounds = value.shape.ranges.front();
      for (size_t i = 0; i < bounds.size(); ++i) {
        Expr index;
        index.kind = Expr::Kind::IntegerLiteral;
        index.text = std::to_string(bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i));
        const auto word = readMemory(name, index, state);
        value.bits.insert(value.bits.end(), word.bits.begin(), word.bits.end());
      }
      return value;
    }
    Value value = object.value;
    if (object.variable) {
      value.bits = state.variables.at(name);
      for (size_t i = 0; i < value.bits.size(); ++i) if (!value.bits[i])
        value.bits[i] = resolveVariableChoice(variableChoice(state, name, i), state);
    }
    for (auto* bit : value.bits) {
      if (!bit) fail("variable read before definite assignment: " + name);
    }
    if (sensitivity_ && object.variable && !combinational_) fail("variable read outside the clock guard is not supported");
    return value;
  }
  Value readDynamicSlice(Value source, const Expr& slice, const State& state) {
    if (source.shape.ranges.size() != 1 || source.shape.enumeration || !source.shape.fields.empty())
      fail("dynamic slices require a logic vector");
    struct Linear { std::map<std::string, Integer> terms; Integer constant = 0; };
    const auto linear = [&](const auto& self, const Expr& expression) -> Linear {
      if (isStatic(expression)) return {{}, integer(expression)};
      if (expression.kind == Expr::Kind::Binary && (expression.text == "+" || expression.text == "-")) {
        auto a = self(self, *expression.left), b = self(self, *expression.right);
        const int sign = expression.text == "+" ? 1 : -1;
        a.constant += sign * b.constant;
        for (const auto& [term, coefficient] : b.terms) a.terms[term] += sign * coefficient;
        return a;
      }
      if (expression.kind == Expr::Kind::Binary && expression.text == "*" &&
          (isStatic(*expression.left) || isStatic(*expression.right))) {
        const bool left = isStatic(*expression.left);
        const auto coefficient = integer(left ? *expression.left : *expression.right);
        auto result = self(self, left ? *expression.right : *expression.left);
        result.constant *= coefficient;
        for (auto& [term, value] : result.terms) value *= coefficient;
        return result;
      }
      return {{{staticSignature(&expression), 1}}, 0};
    };
    auto first = linear(linear, *slice.left), last = linear(linear, *slice.right);
    for (const auto& [term, coefficient] : last.terms) first.terms[term] -= coefficient;
    for (const auto& [term, coefficient] : first.terms) if (coefficient) fail("runtime-dependent slice widths are unsupported");
    const bool ascending = slice.text == "to";
    const auto length = (ascending ? last.constant-first.constant : first.constant-last.constant) + 1;
    const auto original = source.shape.ranges.front();
    if (length < 1 || length > original.size() || ascending != original.ascending)
      fail("invalid dynamic slice width or direction");
    const auto width = static_cast<size_t>(length), depth = original.size()-width+1;
    if (depth > maximumHardwareBits / width) fail("dynamic slice table exceeds hardware size limit");
    Shape table = source.shape;
    table.types.insert(table.types.begin(), "slice_table");
    table.ranges.front() = {int64_t(width-1), 0, false};
    table.ranges.insert(table.ranges.begin(), {original.left,
        original.ascending ? original.right-int64_t(width)+1 : original.right+int64_t(width)-1, original.ascending});
    Bits windows;
    windows.reserve(depth*width);
    for (size_t i = 0; i < depth; ++i) windows.insert(windows.end(), source.bits.begin()+i, source.bits.begin()+i+width);
    auto result = readIndex({table, std::move(windows)}, *slice.left, state);
    result.shape.dynamicBounds = true;
    return result;
  }

  Value readIndex(Value value, const Expr& indexExpr, const State& state) {
    if (value.shape.dynamicBounds) fail("indexing runtime-dependent slice bounds is unsupported");
    if (value.shape.ranges.empty()) fail("cannot index a scalar object");
    const auto bounds = value.shape.ranges.front();
    const auto stride = value.shape.size() / bounds.size();
    if (indexExpr.kind == Expr::Kind::Range && (!isStaticIndex(*indexExpr.left) || !isStaticIndex(*indexExpr.right)))
      return readDynamicSlice(std::move(value), indexExpr, state);
    if (const auto selected = sliceRange(indexExpr, bounds)) {
      const auto slice = *selected;
      if (!slice.size() || slice.ascending != bounds.ascending) fail("invalid slice range");
      const auto offset = bounds.position(slice.left) * stride;
      bounds.position(slice.right);
      value.shape.ranges.front() = slice;
      value.bits = Bits(value.bits.begin() + offset, value.bits.begin() + offset + value.shape.size());
      return value;
    }
    value.shape.ranges.erase(value.shape.ranges.begin());
    value.shape.removeOuterType();
    if (isStaticIndex(indexExpr)) {
      const auto offset = bounds.position(arrayIndex(indexExpr, bounds)) * stride;
      value.bits = Bits(value.bits.begin() + offset, value.bits.begin() + offset + stride);
      return value;
    }
    for (auto* bit : value.bits) readBit(bit);
    auto address = expression(indexExpr, state);
    checkIndexType(address, bounds);
    const auto low = std::min(bounds.left, bounds.right);
    const auto high = std::max(bounds.left, bounds.right);
    if (low < 0) fail("dynamic indexing of negative bounds is unsupported");
    // Retain every nonconstant address bit. Truncating to ceil(log2(depth))
    // would alias out-of-range reads onto valid entries. Widen narrow indices
    // too, so unreachable entries cannot alias in the primitive model.
    while (address.bits.size() > 1 && constantValue(address.bits.front()) == false)
      address.bits.erase(address.bits.begin());
    size_t width = 1;
    for (auto bound = uint64_t(high); bound >>= 1;) ++width;
    width = std::max(width, address.bits.size());
    address.bits = resize(address.bits, width);
    // Use the common bit-window family only for a provably valid index domain;
    // VHDL runtime bounds failures must not become clipped reads.
    const bool inRange = !address.shape.signedInteger && low == 0 &&
        width < 63 && ((uint64_t{1} << width) - 1) <= uint64_t(high);
    if (stride == 1 && inRange &&
        !std::all_of(value.bits.begin(), value.bits.end(), [](auto* bit) { return bit->isConstant(); })) {
      auto offset = resize(address.bits, width + 1);
      if (bounds.ascending)
        offset = add(number(bounds.right, {{"integer"}, {}, width + 1}).bits, offset, true);
      Bits data(value.bits.rbegin(), value.bits.rend());
      std::reverse(offset.begin(), offset.end());
      auto* output = origin(SNLBusNet::create(design_, 0, 0));
      origin(SNLRTLPrimitives::createVectorSelect(design_, data, offset, false, output));
      value.bits = {output->getBit(0)};
      return value;
    }
    if (low) address.bits = add(address.bits, number(low, {{"integer"}, {}, width}).bits, true);
    Bits data;
    for (size_t entry = 0; entry < bounds.size(); ++entry) {
      const auto position = bounds.ascending ? entry : bounds.size() - 1 - entry;
      for (size_t bit = stride; bit; --bit) {
        auto* net = value.bits[position * stride + bit - 1];
        if (const auto binary = constantValue(net)) net = constant(*binary);
        data.push_back(net);
      }
    }
    const bool rom = std::all_of(data.begin(), data.end(), [](auto* bit) { return bit->isConstant(); });
    auto* model = rom ? NLDB0::getOrCreateMemory({stride, bounds.size(), width, 1, 0})
        : NLDB0::getOrCreateTableSelect({stride, bounds.size(), width});
    Bits inputs = data;
    inputs.insert(inputs.end(), address.bits.begin(), address.bits.end());
    const auto cacheKey = std::make_pair(model, inputs);
    if (const auto found = tables_.find(cacheKey); found != tables_.end()) {
      value.bits = found->second;
      return value;
    }
    if (rom) {
      auto* output = origin(SNLBusNet::create(design_, stride - 1, 0));
      auto addressBits = address.bits;
      std::reverse(addressBits.begin(), addressBits.end());
      origin(SNLRTLPrimitives::createROM(design_, data, addressBits, output));
      value.bits.clear();
      for (size_t bit = stride; bit; --bit) value.bits.push_back(output->getBit(bit - 1));
      tables_.emplace(cacheKey, value.bits);
      return value;
    }
    auto* instance = origin(SNLInstance::create(design_, model));
    // Verilog export uses the shared parameterized module, not the canonical
    // model's name or default dimensions.
    for (const auto& [parameter, size] : std::vector<std::pair<std::string, size_t>>{
        {"WIDTH", stride}, {"DEPTH", bounds.size()}, {"ABITS", width}})
      SNLInstParameter::create(instance, model->getParameter(NLName(parameter)), std::to_string(size));
    for (size_t bit = 0; bit < data.size(); ++bit)
      instance->getInstTerm(NLDB0::getTableSelectData(model)->getBit(bit))->setNet(data[bit]);
    for (size_t bit = 0; bit < width; ++bit)
      instance->getInstTerm(NLDB0::getTableSelectAddress(model)->getBit(bit))->setNet(address.bits[width - 1 - bit]);
    auto* output = origin(SNLBusNet::create(design_, stride - 1, 0));
    value.bits.clear();
    for (size_t bit = stride; bit; --bit) {
      auto* net = output->getBit(bit - 1);
      instance->getInstTerm(NLDB0::getTableSelectOutput(model)->getBit(bit - 1))->setNet(net);
      value.bits.push_back(net);
    }
    tables_.emplace(cacheKey, value.bits);
    return value;
  }

  Value readSelected(const Expr& expr, const State& state) {
    if (expr.kind == Expr::Kind::Name) {
      if (const auto alias = aliases_.find(key(expr)); alias != aliases_.end()) {
        auto value = readObject(alias->second.name, state);
        const auto first = alias->second.offset;
        value.bits = Bits(value.bits.begin() + first, value.bits.begin() + first + alias->second.shape.size());
        value.shape = alias->second.shape;
        return value;
      }
      if (!objects_.contains(key(expr))) fail("no declaration for object: " + key(expr));
      return readObject(key(expr), state);
    }
    if (expr.kind == Expr::Kind::Selected) {
      auto value = readSelected(*expr.left, state);
      Selection selected{"", value.shape, 0};
      field(selected, key(expr));
      return {selected.shape, Bits(value.bits.begin() + selected.offset,
          value.bits.begin() + selected.offset + selected.shape.size())};
    }
    if (expr.kind != Expr::Kind::Indexed) fail("indexed prefix must name an object");
    if (expr.left->kind == Expr::Kind::Name && memories_.contains(key(*expr.left)) &&
        !sliceRange(*expr.right, objects_.at(key(*expr.left)).value.shape.ranges.front()))
      return readMemory(key(*expr.left), *expr.right, state);
    return readIndex(readSelected(*expr.left, state), *expr.right, state);
  }

  // Operands are extended to the result width before multiplying. Modulo
  // 2**width multiplication then serves both unsigned and two's-complement bits.
  Bits multiply(const Bits& left, const Bits& right) {
    const auto width = left.size();
    Bits result(width, constant(false));
    bool accumulated = false;
    for (size_t shift = 0; shift < width; ++shift) {
      if (constantValue(right[width - 1 - shift]) == false) continue;
      Bits row(width, constant(false));
      for (size_t i = shift; i < width; ++i)
        row[width - 1 - i] = gate("and", left[width - 1 - i + shift], right[width - 1 - shift]);
      if (std::all_of(row.begin(), row.end(), [&](auto* bit) {
            return constantValue(bit) == false;
          })) continue;
      // Match SV: the first nonzero partial product needs no addition.
      result = accumulated ? add(result, row, false) : std::move(row);
      accumulated = true;
    }
    return result;
  }

  Shape concatenatedAggregateShape(const Expr& aggregate, const Shape& context) {
    if (context.types.back() != "bit" && context.types.back() != "std_logic")
      fail("concatenated aggregate requires a logic element type");
    Range bounds{0, int64_t(aggregate.elements.size()) - 1, true};
    if (aggregate.elements.empty()) fail("empty concatenated aggregate");
    if (aggregate.elements.front()->kind == Expr::Kind::Association) {
      int64_t low = INT64_MAX, high = INT64_MIN;
      const auto add = [&](const auto& self, const Expr& choice) -> void {
        if (choice.kind == Expr::Kind::Aggregate) { for (const auto& item : choice.elements) self(self, *item); return; }
        if (choice.kind == Expr::Kind::Name && key(choice) == "others")
          fail("concatenated aggregate cannot infer bounds from others");
        const auto first = choice.kind == Expr::Kind::Range ? integer(*choice.left) : integer(choice);
        const auto last = choice.kind == Expr::Kind::Range ? integer(*choice.right) : first;
        low = std::min(low, std::min(first, last)); high = std::max(high, std::max(first, last));
      };
      for (const auto& item : aggregate.elements) {
        if (item->kind != Expr::Kind::Association) fail("mixed concatenated aggregate associations");
        add(add, *item->left);
      }
      bounds = {low, high, true};
      const auto& choice = *aggregate.elements.front()->left;
      if (aggregate.elements.size() == 1 && choice.kind == Expr::Kind::Range)
        bounds = {integer(*choice.left), integer(*choice.right), choice.text == "to"};
    }
    if (std::min(bounds.left, bounds.right) < 0 || std::max(bounds.left, bounds.right) > INT32_MAX || !bounds.size())
      fail("invalid concatenated aggregate bounds");
    auto result = vectorShape(bounds.size(), context.types.back());
    if (context.types.front() == "unsigned" || context.types.front() == "signed") result.types.front() = context.types.front();
    result.ranges.front() = bounds;
    return result;
  }

  std::optional<std::string> matchLiteral(const Expr& expression) {
    if (expression.kind != Expr::Kind::StringLiteral && expression.kind != Expr::Kind::CharacterLiteral &&
        expression.kind != Expr::Kind::BitStringLiteral) return std::nullopt;
    std::string result;
    for (size_t i = 1; i + 1 < expression.text.size(); ++i) {
      const char c = std::toupper(static_cast<unsigned char>(expression.text[i]));
      if (expression.kind == Expr::Kind::BitStringLiteral) {
        if (c == '_') continue;
        const unsigned width = expression.canonical == "x" ? 4 : expression.canonical == "o" ? 3 : 1;
        if (c == '-' || c == 'X' || c == 'Z') result.append(width, c);
        else {
          const int digit = c >= '0' && c <= '9' ? c-'0' : c >= 'A' && c <= 'F' ? c-'A'+10 : -1;
          if (digit < 0 || digit >= (1 << width)) fail("invalid matching bit-string digit");
          for (unsigned bit = width; bit; --bit) result += (digit >> (bit-1)) & 1 ? '1' : '0';
        }
      } else {
        if (std::string("UX01ZWLH-").find(c) == std::string::npos) fail("std_match requires logic literals");
        result += c == 'L' ? '0' : c == 'H' ? '1' : c;
      }
    }
    return result;
  }

  Value standardMatch(const Expr& call, const State& state) {
    if (!numeric_ || objects_.contains("std_match") || integers_.contains("std_match"))
      fail("unsupported or invisible std_match function");
    if (call.elements.size() != 2) fail("std_match requires two arguments");
    const Expr* arguments[2]{};
    bool named = false;
    for (size_t i = 0; i < 2; ++i) {
      const auto* argument = call.elements[i].get();
      size_t position = i;
      if (argument->kind == Expr::Kind::Association) {
        named = true;
        if (argument->left->kind != Expr::Kind::Name ||
            (key(*argument->left) != "l" && key(*argument->left) != "r")) fail("invalid std_match formal name");
        position = key(*argument->left) == "l" ? 0 : 1;
        argument = argument->right.get();
      } else if (named) fail("positional std_match argument follows a named argument");
      if (arguments[position]) fail("duplicate std_match argument");
      arguments[position] = argument;
    }
    const auto left = matchLiteral(*arguments[0]), right = matchLiteral(*arguments[1]);
    auto* matches = constant(true);
    if (left && right) {
      if ((arguments[0]->kind == Expr::Kind::CharacterLiteral) != (arguments[1]->kind == Expr::Kind::CharacterLiteral))
        fail("std_match operand type mismatch");
      if (arguments[0]->kind != Expr::Kind::CharacterLiteral)
        fail("ambiguous std_match vector literals require a typed operand");
      if (left->empty() || left->size() != right->size()) matches = constant(false);
      else for (size_t i = 0; i < left->size(); ++i) {
        const char a = (*left)[i], b = (*right)[i];
        if (a != '-' && b != '-' && !((a == '0' || a == '1') && a == b)) matches = constant(false);
      }
    } else {
      if (!left && !right) fail("std_match currently requires a literal pattern");
      const auto& pattern = left ? *left : *right;
      const auto& literal = *arguments[left ? 0 : 1];
      const auto value = expression(*arguments[left ? 1 : 0], state);
      const auto& type = value.shape;
      const bool scalar = type.types == std::vector<std::string>{"std_logic"} && type.ranges.empty();
      const bool vector = type.ranges.size() == 1 && (type.types.front() == "std_logic_vector" ||
          type.types.front() == "unsigned" || type.types.front() == "signed");
      if ((!scalar && !vector) || scalar != (literal.kind == Expr::Kind::CharacterLiteral)) fail("std_match operand type mismatch");
      if (pattern.empty() || pattern.size() != value.bits.size()) matches = constant(false);
      else for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == '-') continue;
        if (pattern[i] != '0' && pattern[i] != '1') {
          if (std::all_of(value.bits.begin(), value.bits.end(), [](auto* bit) { return bit->isConstant(); })) {
            matches = constant(false); continue;
          }
          fail("dynamic std_match requires a binary/don't-care pattern");
        }
        auto* bit = value.bits[i];
        if (bit->isConstantX() || bit->isConstantZ()) matches = constant(false);
        else matches = gate("and", matches, pattern[i] == '1' ? bit : gate("not", bit));
      }
    }
    return {{{"boolean"}, {}}, {matches}};
  }

  // usedWidth limits construction, not the VHDL result subtype. Unused high
  // positions are placeholders and must never escape the consuming operation.
  Value expression(const Expr& expr, const State& state, const Shape* expected = nullptr,
      size_t usedWidth = maximumHardwareBits) {
    DiagnosticScope location(expr.span);
    Value value;
    Shape inferred;
    if (initializerContext_ && expr.kind == Expr::Kind::Indexed && expr.left->kind == Expr::Kind::Name &&
        key(*expr.left) == "endfile" && !functions_.contains("endfile")) {
      if (!textio_ || expr.right->kind != Expr::Kind::Name || !files_.contains(key(*expr.right)))
        fail("endfile requires a visible text file");
      auto& file = *files_.at(key(*expr.right));
      const bool end = file.peek() == std::char_traits<char>::eof();
      if (file.bad()) fail("failed reading initialization file");
      value = {{{"boolean"}, {}}, {constant(end)}};
    } else if (expr.kind == Expr::Kind::Indexed && expr.left->kind == Expr::Kind::Attribute &&
        (key(*expr.left) == "pos" || key(*expr.left) == "val")) {
      const auto& prefix = *expr.left->left;
      if (prefix.kind != Expr::Kind::Name || !enumerations_.contains(key(prefix)))
        fail("pos/val prefix must name a visible enumeration type");
      auto* enumeration = enumerations_.at(key(prefix));
      if (libraryContext_.precedes(prefix.span.start.offset, enumeration->span.start.offset))
        fail("enumeration attribute used before its declaration");
      const Shape type{{key(prefix)}, {}, 0, {}, enumeration};
      if (key(*expr.left) == "pos") {
        value = expression(*expr.right, state, &type);
        value.shape = {{"integer"}, {}, value.bits.size()};
      } else {
        auto ordinal = expression(*expr.right, state);
        if (!ordinal.shape.integerWidth) fail("enumeration val requires an integer argument");
        bool known = true;
        uint64_t number = 0;
        for (auto* bit : ordinal.bits) {
          const auto binary = constantValue(bit);
          if (!binary) { known = false; break; }
          number = number * 2 + *binary;
        }
        value.shape = type;
        value.bits = resize(ordinal.bits, type.size());
        if (known) {
          if (number >= enumeration->literals.size()) fail("enumeration val argument is out of range");
        } else {
          const auto width = std::max<size_t>(32, ordinal.bits.size());
          const auto bits = integerBits(ordinal, width);
          auto* valid = compareBits(bits, this->number(enumeration->literals.size(), {{"integer"}, {}, width}).bits,
              "<", ordinal.shape.signedInteger);
          if (ordinal.shape.signedInteger) valid = gate("and", valid, gate("not", bits.front()));
          value.bits = mux(valid, value.bits, Bits(type.size(), logicLiteral('X', {{"std_logic"}, {}})));
        }
      }
    } else     if (expr.kind == Expr::Kind::Name && enumLiterals_.contains(key(expr))) {
      const auto& candidates = enumLiterals_.at(key(expr));
      const Value* selected = nullptr;
      for (const auto& candidate : candidates) {
        if (expected && candidate.shape.enumeration != expected->enumeration) continue;
        if (selected) fail("ambiguous enumeration literal: " + key(expr));
        selected = &candidate;
      }
      if (!selected) fail("enumeration literal type mismatch");
      value = *selected;
    } else if ((expr.kind == Expr::Kind::Indexed || expr.kind == Expr::Kind::Call) &&
        expr.left->kind == Expr::Kind::Name && key(*expr.left) == "is_x" &&
        !functions_.contains("is_x")) {
      if (!stdLogic_ || objects_.contains("is_x") || integers_.contains("is_x") || booleans_.contains("is_x"))
        fail("unsupported or invisible is_x function");
      const Expr* argument = expr.kind == Expr::Kind::Indexed ? expr.right.get() : nullptr;
      if (expr.kind == Expr::Kind::Call) {
        if (expr.elements.size() != 1) fail("is_x requires exactly one argument");
        argument = expr.elements.front().get();
        if (argument->kind == Expr::Kind::Association) {
          if (argument->left->kind != Expr::Kind::Name || key(*argument->left) != "s")
            fail("unsupported is_x formal name");
          argument = argument->right.get();
        }
      }
      auto operand = expression(*argument, state);
      if (!operand.shape.fields.empty() || operand.shape.integerWidth || operand.shape.enumeration ||
          operand.shape.ranges.size() > 1 || operand.shape.types.back() != "std_logic")
        fail("is_x requires a logic scalar or vector");
      bool unknown = false;
      const bool staticValue = std::all_of(operand.bits.begin(), operand.bits.end(), [](auto* bit) {
        return bit->isConstant();
      });
      if (staticValue) unknown = std::any_of(operand.bits.begin(), operand.bits.end(), [](auto* bit) {
        return bit->isConstantX() || bit->isConstantZ();
      });
      // Match synthesis: dynamic metavalue detection is simulation metadata.
      // Static operands still use the IEEE function's constant result.
      value = {{{"boolean"}, {}}, {constant(unknown)}};
    } else if (isStatic(expr)) {
      const auto result = scalar(expr);
      const Shape type = result.boolean ? Shape{{"boolean"}, {}} :
          expected ? *expected : Shape{{"integer"}, {}, 32, {}, nullptr, {}, true};
      if (!result.boolean && !type.integerWidth && !(unsigned_ && type.types.front() == "std_logic_vector"))
        fail("integer literal is incompatible with target");
      value = number(result.value, type);
    } else if ((expr.kind == Expr::Kind::Name && functions_.contains(key(expr)) && !objects_.contains(key(expr))) ||
               ((expr.kind == Expr::Kind::Indexed || expr.kind == Expr::Kind::Call) &&
                expr.left->kind == Expr::Kind::Name && functions_.contains(key(*expr.left)) &&
                (!objects_.contains(key(*expr.left)) || enumLiterals_.contains(key(*expr.left))) && !integers_.contains(key(*expr.left)) &&
                !booleans_.contains(key(*expr.left)))) {
      value = callRuntime(expr, state);
    } else if (expr.kind == Expr::Kind::Name) {
      const auto name = key(expr);
      if (!objects_.contains(name)) fail("no declaration for object: " + name);
      value = readObject(name, state);
      for (auto* bit : value.bits) readBit(bit);
    } else if (expr.kind == Expr::Kind::Selected) {
      value = readSelected(expr, state);
      for (auto* bit : value.bits) readBit(bit);
    } else if (expected && !expected->fields.empty() && expected->ranges.empty() &&
               (expr.kind == Expr::Kind::Aggregate || expr.kind == Expr::Kind::Others)) {
      value.shape = *expected;
      std::vector<const Expr*> actuals(expected->fields.size(), nullptr);
      if (expr.kind == Expr::Kind::Others) {
        std::fill(actuals.begin(), actuals.end(), expr.left.get());
      } else {
        size_t position = 0;
        bool named = false, others = false;
        for (const auto& element : expr.elements) {
          if (others) fail("others must be the final record aggregate association");
          if (element->kind != Expr::Kind::Association) {
            if (named) fail("positional record element follows named association");
            if (position >= actuals.size()) fail("too many record aggregate fields");
            actuals[position++] = element.get();
            continue;
          }
          named = true;
          const auto& choice = *element->left;
          if (choice.kind == Expr::Kind::Name && key(choice) == "others") {
            others = true;
            for (auto& actual : actuals) if (!actual) actual = element->right.get();
            continue;
          }
          const auto place = [&](const Expr& field) {
            if (field.kind != Expr::Kind::Name) fail("record association must name a field");
            size_t destination = 0;
            while (destination < expected->fields.size() && expected->fields[destination].first != key(field)) ++destination;
            if (destination >= actuals.size() || actuals[destination]) fail("invalid or duplicate record association");
            actuals[destination] = element->right.get();
          };
          if (choice.kind == Expr::Kind::Aggregate) for (const auto& field : choice.elements) place(*field);
          else place(choice);
        }
      }
      for (size_t i = 0; i < actuals.size(); ++i) {
        if (!actuals[i]) fail("missing record aggregate field");
        auto element = expression(*actuals[i], state, expected->fields[i].second.get());
        value.bits.insert(value.bits.end(), element.bits.begin(), element.bits.end());
      }
    } else if (expr.kind == Expr::Kind::Call) {
      if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "std_match") {
        value = standardMatch(expr, state);
      } else if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "sel_suv_f" &&
          expr.elements.size() == 3) {
        const auto condition = scalar(*expr.elements[0]);
        if (!condition.boolean) fail("sel_suv_f condition must be boolean");
        value = expression(*expr.elements[condition.value ? 1 : 2], state, expected, usedWidth);
      } else if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "resize" &&
          numeric_ && expr.elements.size() == 2) {
        if (objects_.contains("resize") || integers_.contains("resize") ||
            functions_.contains("resize")) fail("shadowed resize function");
        const auto width = integer(*expr.elements[1]);
        if (width < 1 || width > 65536) fail("resize requires a supported positive size");
        // Signed resize retains the original sign, including when narrowing.
        // Determine the subtype without constructing arithmetic first.
        auto operand = expression(*expr.elements[0], state, nullptr, 0);
        const auto type = operand.shape.types.front();
        if (operand.shape.ranges.size() != 1 || (type != "unsigned" && type != "signed"))
          fail("resize requires a signed or unsigned vector");
        const auto demand = !usedWidth ? size_t(0) : type == "signed" ? operand.bits.size() :
            std::min<size_t>(width, usedWidth);
        operand = expression(*expr.elements[0], state, nullptr, demand);
        value.shape = vectorShape(width);
        value.shape.types.front() = type;
        if (type == "signed") {
          auto* sign = operand.bits.front();
          value.bits = resize(std::move(operand.bits), width);
          if (size_t(width) > demand)
            std::fill(value.bits.begin(), value.bits.begin() + width - demand, sign);
          value.bits.front() = sign;
        } else value.bits = resize(std::move(operand.bits), width);
      } else if (expr.left->kind == Expr::Kind::Name && numeric_ && expr.elements.size() == 2 &&
          (key(*expr.left) == "shift_left" || key(*expr.left) == "shift_right" ||
           key(*expr.left) == "rotate_left" || key(*expr.left) == "rotate_right")) {
        const auto name = key(*expr.left);
        if (objects_.contains(name) || integers_.contains(name) || functions_.contains(name)) fail("shadowed numeric shift function");
        const bool staticCount = isStatic(*expr.elements[1]);
        const auto count = staticCount ? integer(*expr.elements[1]) : 0;
        if (count < 0 || count > INT32_MAX) fail("numeric shift count must be a natural value");
        value = expression(*expr.elements[0], state);
        if (value.shape.ranges.size() != 1 || (value.shape.types.front() != "unsigned" && value.shape.types.front() != "signed"))
          fail("numeric shift requires a signed or unsigned vector");
        const bool rotate = name == "rotate_left" || name == "rotate_right";
        const bool left = name == "shift_left" || name == "rotate_left";
        const size_t width = value.bits.size();
        if (!staticCount) {
          if (rotate) fail("numeric rotates currently require a static count");
          auto countValue = expression(*expr.elements[1], state);
          if (!countValue.shape.integerWidth || countValue.shape.signedInteger)
            fail("dynamic numeric shift count requires a nonnegative integer subtype");
          const NLDB0::ShiftSignature signature{width, countValue.bits.size(), !left,
              !left && value.shape.types.front() == "signed"};
          auto* model = NLDB0::getOrCreateShift(signature);
          auto* instance = origin(SNLInstance::create(design_, model));
          for (const auto& [parameter, size] : NLDB0::PrimitiveParameters{
              {"WIDTH", std::to_string(signature.width)},
              {"COUNT_WIDTH", std::to_string(signature.countWidth)},
              {"RIGHT", std::to_string(signature.right)},
              {"ARITHMETIC", std::to_string(signature.arithmetic)}})
            SNLInstParameter::create(instance, model->getParameter(NLName(parameter)), size);
          for (size_t bit = 0; bit < width; ++bit)
            instance->getInstTerm(NLDB0::getShiftData(model)->getBit(bit))->setNet(value.bits[width-1-bit]);
          for (size_t bit = 0; bit < countValue.bits.size(); ++bit)
            instance->getInstTerm(NLDB0::getShiftCount(model)->getBit(bit))->setNet(countValue.bits[countValue.bits.size()-1-bit]);
          auto* output = origin(SNLBusNet::create(design_, width-1, 0));
          value.bits.clear();
          for (size_t bit = width; bit; --bit) {
            auto* net = output->getBit(bit-1);
            instance->getInstTerm(NLDB0::getShiftOutput(model)->getBit(bit-1))->setNet(net);
            value.bits.push_back(net);
          }
        } else {
          const size_t amount = rotate ? size_t(count) % width : std::min(size_t(count), width);
          auto bits = value.bits;
          auto* fill = !rotate && !left && value.shape.types.front() == "signed" ? bits.front() : constant(false);
          for (size_t i = 0; i < width; ++i) {
            const auto source = left ? i + amount : i + width - amount;
            value.bits[i] = rotate ? bits[source % width] : left ?
                (source < width ? bits[source] : fill) : (i >= amount ? bits[i-amount] : fill);
          }
        }
        value.shape.ranges.front() = {int64_t(width-1), 0, false};
        value.shape.dynamicBounds = false;
      } else if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "to_signed" &&
          numeric_ && expr.elements.size() == 2) {
        if (objects_.contains("to_signed") || integers_.contains("to_signed") || functions_.contains("to_signed"))
          fail("shadowed to_signed function");
        const auto argument = integer(*expr.elements[0]);
        const auto width = integer(*expr.elements[1]);
        if (argument < INT32_MIN || argument > INT32_MAX || width < 1 || width > 65536)
          fail("to_signed requires a static integer argument and a supported positive size");
        value.shape = vectorShape(width);
        value.shape.types.front() = "signed";
        for (size_t i = width; i; --i)
          value.bits.push_back(constant(i > 64 ? argument < 0 : ((uint64_t(argument) >> (i-1)) & 1)));
      } else if (expr.left->kind != Expr::Kind::Name || key(*expr.left) != "to_unsigned" ||
          !numeric_ || expr.elements.size() != 2)
        fail("unsupported or invisible function call");
      else {
      if (objects_.contains("to_unsigned") || integers_.contains("to_unsigned") ||
          arrayDeclarations_.contains("to_unsigned") || integerTables_.contains("to_unsigned"))
        fail("shadowed to_unsigned function");
      const auto width = integer(*expr.elements[1]);
      if (width < 1 || width > 65536) fail("to_unsigned requires a supported positive size");
      auto type = vectorShape(width);
      type.types.front() = "unsigned";
      if (isStatic(*expr.elements[0])) {
        const auto argument = integer(*expr.elements[0]);
        if (argument < 0 || argument > INT32_MAX) fail("to_unsigned requires a natural argument");
        value = number(argument, type);
      } else {
        auto argument = expression(*expr.elements[0], state);
        if (!argument.shape.integerWidth || argument.shape.signedInteger)
          fail("dynamic to_unsigned requires a nonnegative integer subtype");
        value = {type, resize(std::move(argument.bits), width)};
      }
      }
    } else if (expr.kind == Expr::Kind::Indexed) {
      if (expr.left->kind == Expr::Kind::Name &&
          (key(*expr.left) == "std_logic_vector" || key(*expr.left) == "std_ulogic_vector" ||
           key(*expr.left) == "unsigned" || key(*expr.left) == "signed")) {
        const auto target = key(*expr.left) == "std_ulogic_vector" ? "std_logic_vector" : key(*expr.left);
        if (!numeric_ || (target == "std_logic_vector" && !stdLogic_))
          fail("vector conversion requires visible numeric_std and std_logic_1164 types");
        value = expression(*expr.right, state, nullptr, usedWidth);
        const auto source = value.shape.types.front();
        if (value.shape.ranges.size() != 1 || (source != "unsigned" && source != "signed" && source != "std_logic_vector"))
          fail("unsupported vector conversion operand");
        value.shape.types.front() = target;
      } else if (expr.left->kind == Expr::Kind::Name &&
          (key(*expr.left) == "conv_integer" || key(*expr.left) == "to_integer")) {
        const bool numeric = key(*expr.left) == "to_integer";
        if (numeric ? !numeric_ : (!unsigned_ || !arith_))
          fail(numeric ? "to_integer requires numeric_std" :
              "conv_integer requires std_logic_arith and std_logic_unsigned");
        value = expression(*expr.right, state);
        if (value.shape.ranges.size() != 1 ||
            value.shape.types.front() != (numeric ? "unsigned" : "std_logic_vector") || value.bits.size() > 31)
          fail(numeric ? "unsupported to_integer argument" : "unsupported conv_integer argument");
        value.shape = {{"integer"}, {}, value.bits.size()};
      } else if (expr.left->kind != Expr::Kind::Name &&
          (expr.right->kind == Expr::Kind::Range || isStaticIndex(*expr.right))) {
        const auto prefix = expression(*expr.left, state, nullptr, 0);
        if (prefix.shape.ranges.empty()) fail("expression selection requires an array");
        const auto bounds = prefix.shape.ranges.front();
        if (expr.right->kind == Expr::Kind::Range && (!isStaticIndex(*expr.right->left) || !isStaticIndex(*expr.right->right)))
          value = readDynamicSlice(expression(*expr.left, state), *expr.right, state);
        else {
          const auto slice = sliceRange(*expr.right, bounds);
          const auto last = slice ? slice->right : arrayIndex(*expr.right, bounds);
          const auto first = slice ? slice->left : last;
          const auto stride = prefix.shape.size() / bounds.size();
          const auto demand = prefix.bits.size() - std::min(bounds.position(first), bounds.position(last)) * stride;
          value = readIndex(expression(*expr.left, state, nullptr, usedWidth ? demand : 0), *expr.right, state);
        }
      } else value = readSelected(expr, state);
      for (auto* bit : value.bits) readBit(bit);
    } else if (expr.kind == Expr::Kind::Conditional) {
      const auto condition = conditionValue(expression(*expr.condition, state, nullptr, usedWidth ? maximumHardwareBits : 0));
      auto yes = expression(*expr.left, state, expected, usedWidth);
      auto no = expression(*expr.right, state, &yes.shape, usedWidth);
      if (!compatible(yes.shape, no.shape)) fail("conditional type mismatch");
      value.shape = yes.shape;
      const auto width = std::min(usedWidth, yes.bits.size());
      value.bits = resize(mux(condition.bits.front(), resize(yes.bits, width), resize(no.bits, width)), yes.bits.size());
    } else if (expr.kind == Expr::Kind::Aggregate) {
      if (!expected || expected->ranges.empty()) fail("array aggregate requires a constrained target");
      if (expected->dynamicBounds && std::any_of(expr.elements.begin(), expr.elements.end(),
          [](const auto& item) { return item->kind == Expr::Kind::Association; }))
        fail("named aggregate context has runtime-dependent bounds");
      value.shape = *expected;
      auto element = *expected;
      element.removeOuterType();
      element.ranges.erase(element.ranges.begin());
      std::map<const Expr*, std::pair<Range, Bits>> slices;
      if (expected->ranges.size() == 1) for (const auto& item : expr.elements) {
        if (item->kind != Expr::Kind::Association) continue;
        const auto& choice = *item->left;
        if (choice.kind != Expr::Kind::Range && !arrayAttribute(choice)) continue;
        const auto& rhs = *item->right;
        if (rhs.kind == Expr::Kind::CharacterLiteral) continue;
        const bool contextual = rhs.kind == Expr::Kind::StringLiteral || rhs.kind == Expr::Kind::BitStringLiteral ||
            rhs.kind == Expr::Kind::Aggregate || rhs.kind == Expr::Kind::Others;
        if (!contextual && expression(rhs, state, nullptr, 0).shape.ranges.empty()) continue;
        const auto& bounds = expected->ranges.front();
        const auto selected = arrayAttribute(choice) ? attributeRange(choice) :
            Range{arrayIndex(*choice.left, bounds), arrayIndex(*choice.right, bounds), choice.text == "to"};
        auto context = *expected;
        context.ranges.front() = selected;
        slices.emplace(&rhs, std::make_pair(selected, expression(rhs, state, &context).bits));
      }
      std::map<const Expr*, Bits> parts;
      const auto actuals = arrayAggregateElements(expr, expected->ranges.front());
      for (size_t i = 0; i < actuals.size(); ++i) {
        const auto* item = actuals[i];
        if (const auto found = slices.find(item); found != slices.end()) {
          const auto& bounds = expected->ranges.front();
          const auto index = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
          const auto first = found->second.first.position(index) * element.size();
          value.bits.insert(value.bits.end(), found->second.second.begin() + first,
              found->second.second.begin() + first + element.size());
        } else {
          auto [part, inserted] = parts.try_emplace(item);
          if (inserted) part->second = expression(*item, state, &element).bits;
          value.bits.insert(value.bits.end(), part->second.begin(), part->second.end());
        }
      }
    } else if (expr.kind == Expr::Kind::CharacterLiteral || expr.kind == Expr::Kind::StringLiteral ||
               expr.kind == Expr::Kind::Others || expr.kind == Expr::Kind::BitStringLiteral) {
      if (!expected && expr.kind != Expr::Kind::Others) {
        const auto digits = expr.kind == Expr::Kind::BitStringLiteral ?
            std::count_if(expr.text.begin() + 1, expr.text.end() - 1, [](char c) { return c != '_'; }) :
            expr.text.size() - 2;
        const auto width = digits * (expr.kind == Expr::Kind::BitStringLiteral ? (expr.canonical == "x" ? 4 : expr.canonical == "o" ? 3 : 1) : 1);
        inferred = expr.kind == Expr::Kind::CharacterLiteral ? Shape{{"std_logic"}, {}} : vectorShape(width);
        expected = &inferred;
      }
      if (!expected) fail("literal requires a constrained target type");
      value.shape = *expected;
      if (expr.kind == Expr::Kind::Others) {
        if (expected->ranges.empty()) fail("others aggregate requires an array target");
        auto element = *expected;
        element.ranges.erase(element.ranges.begin());
        element.removeOuterType();
        const auto bits = expression(*expr.left, state, &element).bits;
        for (size_t i = 0; i < expected->ranges.front().size(); ++i)
          value.bits.insert(value.bits.end(), bits.begin(), bits.end());
      } else {
        const bool scalar = expr.kind == Expr::Kind::CharacterLiteral;
        if ((scalar && !expected->ranges.empty()) || (!scalar && expected->ranges.size() != 1) ||
            (expected->types.back() != "bit" && expected->types.back() != "std_logic"))
          fail("literal type does not match target");
        for (size_t i = 1; i + 1 < expr.text.size(); ++i) {
          const auto c = expr.text[i];
          if (expr.kind == Expr::Kind::BitStringLiteral) {
            if (c == '_') continue;
            const int width = expr.canonical == "x" ? 4 : expr.canonical == "o" ? 3 : 1;
            if (c == 'X' || c == 'Z') {
              value.bits.insert(value.bits.end(), width, logicLiteral(c, *expected));
              continue;
            }
            const int digit = c >= '0' && c <= '9' ? c - '0' :
                std::tolower(static_cast<unsigned char>(c)) - 'a' + 10;
            if (digit < 0 || digit >= (1 << width)) fail("invalid bit string digit");
            for (int bit = width - 1; bit >= 0; --bit) value.bits.push_back(constant((digit >> bit) & 1));
          } else {
            value.bits.push_back(logicLiteral(c, *expected));
          }
        }
      }
    } else if (expr.kind == Expr::Kind::Unary && expr.text == "??") {
      auto operand = expression(*expr.left, state);
      if (operand.shape.types == std::vector<std::string>{"boolean"})
        fail("explicit condition conversion requires bit or std_ulogic");
      value = conditionValue(std::move(operand));
    } else if (expr.kind == Expr::Kind::Unary &&
        (expr.text == "and" || expr.text == "or" || expr.text == "nand" ||
         expr.text == "nor" || expr.text == "xor" || expr.text == "xnor")) {
      const auto operand = expression(*expr.left, state);
      if (operand.shape.ranges.size() != 1 || !operand.shape.fields.empty() ||
          operand.shape.enumeration || (operand.shape.types.back() != "bit" &&
          operand.shape.types.back() != "std_logic")) fail("reduction requires a logic vector");
      auto* result = operand.bits.size() == 1 ? operand.bits.front() :
          cachedGate(expr.text == "nand" ? "and" : expr.text == "nor" ? "or" :
              expr.text == "xnor" ? "xor" : expr.text, operand.bits);
      if (expr.text == "nand" || expr.text == "nor" || expr.text == "xnor") result = gate("not", result);
      value = {{{operand.shape.types.back()}, {}}, {result}};
    } else if (expr.kind == Expr::Kind::Unary) {
      value = expression(*expr.left, state, expr.text == "not" ? expected : nullptr, usedWidth);
      if (value.shape.integerWidth && expr.text != "not") {
        value.bits = integerBits(value, 32);
        value.shape.integerWidth = 32;
        value.shape.signedInteger = true;
      }
      if ((expr.text == "+" || expr.text == "-" || expr.text == "abs") &&
          (value.shape.integerWidth || value.shape.types.front() == "signed")) {
        for (auto* bit : value.bits) if (bit->isConstantX() || bit->isConstantZ())
          fail("explicit X/Z operands in arithmetic are unsupported");
        if (expr.text != "+" && usedWidth) {
          const auto negated = add(Bits(value.bits.size(), constant(false)), value.bits, true);
          value.bits = expr.text == "abs" ? mux(value.bits.front(), negated, value.bits) : negated;
        }
      } else {
        if (expr.text != "not" || value.shape.integerWidth || value.shape.enumeration || !value.shape.fields.empty())
          fail("unsupported hardware unary operator");
        for (size_t i = value.bits.size() - std::min(usedWidth, value.bits.size()); i < value.bits.size(); ++i)
          value.bits[i] = gate("not", value.bits[i]);
      }
    } else if (expr.kind == Expr::Kind::Binary) {
      const bool ordering = expr.text == "<" || expr.text == "<=" || expr.text == ">" || expr.text == ">=";
      const bool comparison = expr.text == "=" || expr.text == "/=" || ordering;
      const bool arithmetic = expr.text == "+" || expr.text == "-";
      if (expr.text == "mod") {
        if (!isStatic(*expr.right)) fail("hardware mod currently requires a static positive power-of-two divisor");
        const auto divisor = integer(*expr.right);
        if (divisor < 1 || divisor > INT32_MAX || (uint64_t(divisor) & (uint64_t(divisor)-1)))
          fail("hardware mod currently requires a static positive power-of-two divisor");
        size_t width = 0;
        for (auto n = divisor; n > 1; n >>= 1) ++width;
        auto operand = expression(*expr.left, state, nullptr, std::min(width, usedWidth));
        if (!operand.shape.integerWidth) fail("hardware mod requires an integer dividend");
        for (auto* bit : operand.bits) if (bit->isConstantX() || bit->isConstantZ())
          fail("explicit X/Z operands in arithmetic are unsupported");
        value.shape = {{"integer"}, {}, 32};
        value.bits = resize(resize(std::move(operand.bits), width), 32);
      } else if (expr.text == "&") {
        const auto literal = [](const Expr& operand) {
          return operand.kind == Expr::Kind::Aggregate || operand.kind == Expr::Kind::CharacterLiteral ||
              operand.kind == Expr::Kind::StringLiteral || operand.kind == Expr::Kind::BitStringLiteral;
        };
        const auto* context = expected;
        std::optional<Shape> inferredContext, leftType, rightType;
        if (literal(*expr.left) && !literal(*expr.right)) inferredContext = expression(*expr.right, state, nullptr, 0).shape;
        else if (literal(*expr.right) && !literal(*expr.left)) inferredContext = expression(*expr.left, state, nullptr, 0).shape;
        if (inferredContext) context = &*inferredContext;
        const auto operandType = [&](const Expr& operand) -> std::optional<Shape> {
          if (!literal(operand) || !context) return std::nullopt;
          if (operand.kind == Expr::Kind::Aggregate) return concatenatedAggregateShape(operand, *context);
          auto type = expression(operand, state, nullptr, 0).shape;
          if (type.ranges.empty()) type.types = {context->types.back()};
          else {
            type.types.back() = context->types.back();
            type.types.front() = context->types.front() == "unsigned" || context->types.front() == "signed" ?
                context->types.front() : context->types.back() == "bit" ? "bit_vector" : "std_logic_vector";
          }
          return type;
        };
        leftType = operandType(*expr.left); rightType = operandType(*expr.right);
        const auto rightShape = expression(*expr.right, state, rightType ? &*rightType : nullptr, 0).shape;
        auto left = expression(*expr.left, state, leftType ? &*leftType : nullptr,
            usedWidth > rightShape.size() ? usedWidth - rightShape.size() : 0);
        auto right = expression(*expr.right, state, rightType ? &*rightType : nullptr, usedWidth);
        if (left.shape.enumeration || right.shape.enumeration || !left.shape.fields.empty() || !right.shape.fields.empty() || left.shape.integerWidth || right.shape.integerWidth || left.shape.ranges.size() > 1 ||
            right.shape.ranges.size() > 1 || left.shape.types.back() != right.shape.types.back())
          fail("unsupported concatenation operands");
        value.bits = left.bits;
        value.bits.insert(value.bits.end(), right.bits.begin(), right.bits.end());
        value.shape = vectorShape(value.bits.size(), left.shape.types.back());
        for (const auto* numericType : {"unsigned", "signed"}) {
          if (left.shape.types.front() == numericType || right.shape.types.front() == numericType) {
            if ((left.shape.ranges.size() && left.shape.types.front() != numericType) ||
                (right.shape.ranges.size() && right.shape.types.front() != numericType))
              fail("concatenation operand type mismatch");
            value.shape.types.front() = numericType;
          }
        }
      } else {
        const bool numericOperation = (numeric_ || signed_) && (arithmetic || expr.text == "*" || comparison);
        const auto operandDemand = comparison && usedWidth ? maximumHardwareBits : usedWidth;
        const auto staticIntegerOperand = [&](const Expr& operand) -> std::optional<int64_t> {
          if (!numeric_ || !numericOperation || !isStatic(operand)) return std::nullopt;
          const auto value = scalar(operand);
          return value.boolean ? std::nullopt : std::optional<int64_t>{value.value};
        };
        const auto leftInteger = staticIntegerOperand(*expr.left);
        const auto rightInteger = staticIntegerOperand(*expr.right);
        // Defer static scalar bits until the numeric vector's width/sign is known.
        // In particular, negative integers are valid signed overload operands.
        std::optional<Shape> leftContext;
        if ((expr.left->kind == Expr::Kind::Name && enumLiterals_.contains(key(*expr.left)) &&
            enumLiterals_.at(key(*expr.left)).size() > 1) || expr.left->kind == Expr::Kind::Aggregate ||
            expr.left->kind == Expr::Kind::Others || (comparison &&
            (expr.left->kind == Expr::Kind::StringLiteral || expr.left->kind == Expr::Kind::BitStringLiteral)))
          leftContext = expression(*expr.right, state, nullptr, 0).shape;
        const auto aggregateContext = [&](const Expr& operand, const Shape& type) -> Shape {
          if (type.ranges.size() == 1 && type.declarations.empty() && !type.enumeration && type.fields.empty()) {
            if (operand.kind == Expr::Kind::Others) fail("comparison aggregate others has no constrained context");
            if (operand.kind == Expr::Kind::Aggregate) return concatenatedAggregateShape(operand, type);
          }
          return type;
        };
        if (leftContext) *leftContext = aggregateContext(*expr.left, *leftContext);
        auto left = leftInteger ? Value{Shape{{"integer"}, {}, 32}, {}} :
            expression(*expr.left, state, leftContext ? &*leftContext :
                numericOperation || comparison ? nullptr : expected, operandDemand);
        const bool leftNumeric = numeric_ &&
            (left.shape.types.front() == "unsigned" || left.shape.types.front() == "signed");
        const bool leftSigned = left.shape.types.front() == "signed" ||
            (signed_ && left.shape.types.front() == "std_logic_vector");
        const bool rightLiteral = expr.right->kind == Expr::Kind::StringLiteral ||
            expr.right->kind == Expr::Kind::BitStringLiteral || expr.right->kind == Expr::Kind::Others;
        std::optional<Shape> rightContext;
        if (comparison && (expr.right->kind == Expr::Kind::Aggregate || expr.right->kind == Expr::Kind::Others))
          rightContext = aggregateContext(*expr.right, left.shape);
        auto right = rightInteger ? Value{Shape{{"integer"}, {}, 32}, {}} : expression(*expr.right, state,
            rightContext ? &*rightContext : ((leftNumeric || leftSigned) && !rightLiteral) ||
            ((arithmetic || numericOperation || comparison) && (left.shape.integerWidth ||
                (numeric_ && left.shape.types == std::vector<std::string>{"std_logic"}))) ? nullptr : &left.shape, operandDemand);
        const bool rightNumeric = numeric_ &&
            (right.shape.types.front() == "unsigned" || right.shape.types.front() == "signed");
        if (leftNumeric && right.shape.integerWidth)
          right = numericIntegerOperand(std::move(right), rightInteger, left, comparison);
        else if (rightNumeric && left.shape.integerWidth)
          left = numericIntegerOperand(std::move(left), leftInteger, right, comparison);
        else {
          if (leftInteger) left = expression(*expr.left, state, numericOperation || comparison ? nullptr : expected, operandDemand);
          if (rightInteger) right = expression(*expr.right, state,
              ((leftNumeric || leftSigned) && !rightLiteral) ||
              ((arithmetic || comparison || expr.text == "*") && left.shape.integerWidth) ? nullptr : &left.shape, operandDemand);
        }
        if (arithmetic && numeric_) {
          const auto scalarOperand = [&](Value& scalar, const Value& vector) {
            if (scalar.shape.types != std::vector<std::string>{"std_logic"} || !scalar.shape.ranges.empty()) return;
            auto* bit = scalar.bits.front();
            scalar.shape = vector.shape;
            scalar.bits.assign(vector.bits.size(), constant(false));
            scalar.bits.back() = bit;
          };
          if (leftNumeric) scalarOperand(right, left);
          else if (rightNumeric) scalarOperand(left, right);
        }
        if (arithmetic || comparison || expr.text == "*") {
          const auto nonbinary = [](SNLBitNet* bit) { return bit->isConstantX() || bit->isConstantZ(); };
          if (std::any_of(left.bits.begin(), left.bits.end(), nonbinary) ||
              std::any_of(right.bits.begin(), right.bits.end(), nonbinary))
            fail("explicit X/Z operands in arithmetic and comparisons are unsupported");
        }
        const bool unsignedOperands = numeric_ && left.shape.types.front() == "unsigned";
        const bool numericSigned = numeric_ && left.shape.types.front() == "signed";
        const bool signedOperands = numericSigned || (signed_ && left.shape.types.front() == "std_logic_vector");
        if (signedOperands && !numericSigned && unsigned_ && (arithmetic || comparison || expr.text == "*"))
          fail("ambiguous signed/unsigned vector operators");
        if (signedOperands && (arithmetic || comparison || expr.text == "*")) {
          if (right.shape.types.front() != left.shape.types.front()) fail("signed vector operand type mismatch");
          const auto width = expr.text == "*" ? left.bits.size() + right.bits.size() :
              std::max(left.bits.size(), right.bits.size());
          left.bits.insert(left.bits.begin(), width - left.bits.size(), left.bits.front());
          right.bits.insert(right.bits.begin(), width - right.bits.size(), right.bits.front());
          value.shape = vectorShape(width);
          value.shape.types.front() = left.shape.types.front();
          const auto hardwareWidth = comparison ? width : std::min(width, usedWidth);
          if (!comparison) {
            left.bits = resize(std::move(left.bits), hardwareWidth);
            right.bits = resize(std::move(right.bits), hardwareWidth);
          }
          if (comparison) {
            auto* result = usedWidth ? compareBits(left.bits, right.bits, expr.text, true) : constant(false);
            value = {{{"boolean"}, {}}, {result}};
          } else if (expr.text == "*") {
            value.bits = multiply(left.bits, right.bits);
          } else value.bits = add(left.bits, right.bits, expr.text == "-");
          if (!comparison) value.bits = resize(std::move(value.bits), width);
          if (expected && !compatible(value.shape, *expected)) fail("assignment or expression type/length mismatch");
          return value;
        }
        if (unsignedOperands && (arithmetic || comparison || expr.text == "*")) {
          if (right.shape.types.front() != "unsigned") fail("unsigned operand type mismatch");
          const auto width = expr.text == "*" ? left.bits.size() + right.bits.size() :
              std::max(left.bits.size(), right.bits.size());
          left.bits = resize(left.bits, width);
          right.bits = resize(right.bits, width);
          value.shape = vectorShape(width);
          value.shape.types.front() = "unsigned";
          const auto hardwareWidth = comparison ? width : std::min(width, usedWidth);
          if (!comparison) {
            left.bits = resize(std::move(left.bits), hardwareWidth);
            right.bits = resize(std::move(right.bits), hardwareWidth);
          }
          if (comparison) {
            auto* result = usedWidth ? compareBits(left.bits, right.bits, expr.text) : constant(false);
            value = {{{"boolean"}, {}}, {result}};
          } else if (expr.text == "*") {
            value.bits = multiply(left.bits, right.bits);
          } else value.bits = add(left.bits, right.bits, expr.text == "-");
          if (!comparison) value.bits = resize(std::move(value.bits), width);
          if (expected && !compatible(value.shape, *expected))
            fail("assignment or expression type/length mismatch");
          return value;
        }
        if (comparison && left.shape.integerWidth && right.shape.integerWidth) {
          const auto width = std::max(left.bits.size() + size_t(!left.shape.signedInteger),
              right.bits.size() + size_t(!right.shape.signedInteger));
          left.bits = integerBits(left, width);
          right.bits = integerBits(right, width);
        }
        if (!compatible(left.shape, right.shape)) fail("binary operand type mismatch");
        if ((left.shape.enumeration || right.shape.enumeration) && !comparison)
          fail("enumeration operators currently support only equality and inequality");
        if (comparison) {
          if (ordering && !left.shape.integerWidth && !left.shape.enumeration &&
              !(unsigned_ && left.shape.types.front() == "std_logic_vector"))
            fail("ordering requires numeric vectors or hardware integers");
          auto* result = usedWidth ? compareBits(left.bits, right.bits, expr.text, left.shape.integerWidth != 0) : constant(false);
          value = {{{"boolean"}, {}}, {result}};
        } else if (arithmetic || (expr.text == "*" && left.shape.integerWidth)) {
          if (!left.shape.integerWidth && !(unsigned_ && left.shape.types.front() == "std_logic_vector"))
            fail("arithmetic requires integer or std_logic_unsigned operands");
          value.shape = left.shape;
          const auto resultWidth = left.shape.integerWidth ?
              (expected && expected->integerWidth ? expected->integerWidth : size_t(32)) : left.bits.size();
          if (left.shape.integerWidth) { value.shape.integerWidth = resultWidth; value.shape.signedInteger = true; }
          const auto width = std::min(usedWidth,
              resultWidth);
          value.bits = resize(expr.text == "*" ? multiply(integerBits(left, width), integerBits(right, width)) :
              add(integerBits(left, width), integerBits(right, width), expr.text == "-"), resultWidth);
        } else {
          if (left.shape.ranges.size() > 1 || left.shape.integerWidth || !left.shape.fields.empty())
            fail("bitwise operators require scalar logic, boolean or logic vectors");
          value.shape = left.shape;
          value.bits = Bits(left.bits.size(), constant(false));
          for (size_t i = left.bits.size() - std::min(usedWidth, left.bits.size()); i < left.bits.size(); ++i)
            value.bits[i] = gate(expr.text, left.bits[i], right.bits[i]);
        }
      }
    } else fail("unsupported RTL expression");
    if (expected && expected->integerWidth && value.shape.integerWidth) {
      value.bits = integerBits(value, expected->integerWidth);
      value.shape = *expected;
    }
    if (value.bits.size() != value.shape.size() || (expected && !compatible(value.shape, *expected)))
      fail("assignment or expression type/length mismatch");
    return value;
  }

  void checkInitialValues() {
    std::map<SNLBitNet*, std::optional<bool>> values;
    std::set<SNLBitNet*> visiting;
    std::function<std::optional<bool>(SNLBitNet*)> evaluate = [&](SNLBitNet* net) -> std::optional<bool> {
      if (!net) return std::nullopt;
      if (auto value = constantValue(net)) return value;
      if (const auto found = values.find(net); found != values.end()) return found->second;
      if (initializedFlops_.contains(net)) return initialValues_.at(net);
      if (!visiting.insert(net).second) return std::nullopt;
      SNLInstTerm* output = nullptr;
      for (auto* term : net->getInstTerms()) if (term->getDirection() == SNLTerm::Direction::Output) {
        if (output) { visiting.erase(net); return std::nullopt; }
        output = term;
      }
      std::optional<bool> result;
      if (output) {
        auto* instance = output->getInstance();
        auto* model = instance->getModel();
        const auto read = [&](SNLBitTerm* term) { return evaluate(instance->getInstTerm(term)->getNet()); };
        if (NLDB0::isAssign(model)) result = read(NLDB0::getAssignInput());
        else if (NLDB0::isMux2(model)) {
          const auto bit = static_cast<SNLBusTermBit*>(output->getBitTerm())->getBit();
          const auto select = read(NLDB0::getMux2Select(model));
          if (select) result = read((*select ? NLDB0::getMux2InputB(model) : NLDB0::getMux2InputA(model))->getBit(bit));
          else {
            const auto a = read(NLDB0::getMux2InputA(model)->getBit(bit));
            const auto b = read(NLDB0::getMux2InputB(model)->getBit(bit));
            if (a && b && *a == *b) result = a;
          }
        } else if (NLDB0::isFA(model)) {
          const auto a = read(NLDB0::getFAInputA()), b = read(NLDB0::getFAInputB()), c = read(NLDB0::getFAInputCI());
          if (output->getBitTerm() == NLDB0::getFAOutputS()) {
            if (a && b && c) result = *a ^ *b ^ *c;
          } else if (a && b && *a == *b) result = a;
          else if (a && c && *a == *c) result = a;
          else if (b && c && *b == *c) result = b;
        } else if (NLDB0::isGate(model)) {
          const auto op = NLDB0::getGateName(model);
          const bool conjunction = op == "and" || op == "nand";
          const bool disjunction = op == "or" || op == "nor";
          bool accumulator = conjunction, unknown = false, decisive = false;
          for (auto* term : model->getBitTerms()) if (term->getDirection() == SNLTerm::Direction::Input) {
            const auto input = read(term);
            if (!input) { unknown = true; continue; }
            if (conjunction) { accumulator &= *input; decisive |= !*input; }
            else if (disjunction) { accumulator |= *input; decisive |= *input; }
            else if (op == "xor" || op == "xnor") accumulator ^= *input;
            else accumulator = *input;
          }
          if ((!unknown || decisive) && (conjunction || disjunction || op == "xor" || op == "xnor" || op == "not" || op == "buf")) {
            if (op == "nand" || op == "nor" || op == "xnor" || op == "not") accumulator = !accumulator;
            result = accumulator;
          }
        }
      }
      visiting.erase(net);
      values.emplace(net, result);
      return result;
    };
    std::map<SNLBitNet*, bool> contained;
    std::set<SNLBitNet*> active;
    const auto behindRegister = [&](const auto& self, SNLBitNet* net) -> bool {
      if (const auto found = contained.find(net); found != contained.end()) return found->second;
      if (!active.insert(net).second) return false;
      bool safe = true;
      for (auto* term : net->getBitTerms()) if (term->getDirection() != SNLTerm::Direction::Input) safe = false;
      for (auto* term : net->getInstTerms()) if (safe && term->getDirection() == SNLTerm::Direction::Input) {
        auto* model = term->getInstance()->getModel();
        if (SNLDesignModeling::isSequential(model)) {
          const auto role = SNLDesignModeling::getTermRole(term);
          safe = role == SNLDesignModeling::SNLTermRole::DataInput || role == SNLDesignModeling::SNLTermRole::Enable;
        } else if (NLDB0::isAssign(model) || NLDB0::isGate(model) || NLDB0::isFA(model) ||
            NLDB0::isMux2(model) || NLDB0::isTableSelect(model) || NLDB0::isShift(model)) {
          for (auto* output : term->getInstance()->getInstTerms()) if (output->getDirection() == SNLTerm::Direction::Output)
            if (output->getNet() && !self(self, output->getNet())) { safe = false; break; }
        } else safe = false;
      }
      active.erase(net);
      contained.emplace(net, safe);
      return safe;
    };
    for (const auto& [bit, value] : initialValues_) if (!initializedFlops_.contains(bit)) {
      // A connected child's output supplies the driver and its initial value.
      // The actual signal's declaration default does not initialize that child.
      // Each RTL child has already checked its own drivers and initializers.
      const auto terms = bit->getInstTerms();
      const bool childDriven = std::any_of(terms.begin(), terms.end(), [](auto* term) {
        return term->getDirection() == SNLTerm::Direction::Output &&
            !term->getInstance()->getModel()->isPrimitive();
      });
      if (childDriven) continue;
      const auto actual = evaluate(bit);
      if ((!actual || *actual != value) && !behindRegister(behindRegister, bit)) {
        const auto found = initialDeclarations_.find(bit);
        if (found != initialDeclarations_.end()) {
          DiagnosticScope location(found->second.second);
          fail("signal initializer is neither registered nor provably redundant: " + found->second.first);
        }
        fail("signal initializer is neither registered nor provably redundant: " + bit->getName().getString()); // LCOV_EXCL_LINE: signal initial values carry declaration locations; retained-variable initial values belong to initialized flops.
      }
    }
  }

  void drive(SNLBitNet* target, SNLBitNet* source) {
    if (!drivers_.insert(target).second) fail("multiple drivers for one signal bit");
    origin(target);
    const auto terms = target->getBitTerms();
    const bool drivesPort = std::any_of(terms.begin(), terms.end(), [](auto* term) {
      return term->getDirection() != SNLTerm::Direction::Input;
    });
    if (source->isConstant() && !drivesPort)
      target->setType(source->getType());
    else origin(SNLRTLPrimitives::createAssign(design_, source, target));
  }
  void concurrent(const vhdl::Assignment& assignment, const State& state) {
    DiagnosticScope location(assignment.span);
    auto target = selection(assignment);
    const auto& object = objects_.at(target.name);
    if (object.input || object.constant || object.variable || assignment.kind != vhdl::AssignmentKind::Signal)
      fail("concurrent assignment must drive a writable signal");
    Value value;
    if (assignment.selector) {
      const auto selector = expression(*assignment.selector, state);
      const auto matches = selectionMatches(selector, assignment.choices);
      value = expression(*assignment.alternatives.back(), state, &target.shape);
      for (size_t i = assignment.alternatives.size() - 1; i-- > 0;) {
        const auto branch = expression(*assignment.alternatives[i], state, &target.shape);
        value.bits = mux(matches[i], branch.bits, value.bits);
      }
    } else value = expression(*assignment.value, state, &target.shape);
    for (size_t i = 0; i < value.bits.size(); ++i) drive(object.value.bits[target.offset + i], value.bits[i]);
  }

  void assign(const vhdl::Assignment& assignment, State& state) {
    DiagnosticScope location(assignment.span);
    if (memories_.contains(key(assignment.target))) {
      const auto name = key(assignment.target);
      auto& write = state.memoryWrites.at(name);
      auto element = objects_.at(name).value.shape;
      element.ranges.erase(element.ranges.begin());
      element.removeOuterType();
      auto address = expression(*assignment.indices.front(), state);
      if (!address.shape.integerWidth) fail("array index must be integer");
      auto [bits, valid] = memoryAddress(name, std::move(address.bits));
      Selection target{name, element, 0};
      for (size_t i = 1; i < assignment.indices.size(); ++i) index(target, *assignment.indices[i]);
      const auto data = expression(*assignment.value, state, &target.shape).bits;
      const bool masked = memories_.at(name).signature.independentWriteClocks;
      if (write.assigned && write.address != bits)
        fail("multiple memory writes must use the same address after loop unrolling");
      write.address = std::move(bits);
      if (masked) {
        for (size_t i = 0; i < data.size(); ++i) {
          write.data[target.offset+i] = data[i];
          write.mask[target.offset+i] = constant(true);
        }
      } else {
        if (target.offset || data.size() != element.size()) fail("partial memory write requires a mask");
        write.data = data;
      }
      write.enable = valid;
      write.assigned = true;
      return;
    }
    const bool dynamic = std::any_of(assignment.indices.begin(), assignment.indices.end(), [&](const auto& index) {
      return index->kind != Expr::Kind::Range && index->kind != Expr::Kind::Selected && !isStaticIndex(*index);
    });
    if (dynamic) {
      const auto root = selection(key(assignment.target));
      const auto& name = root.name;
      const auto& object = objects_.at(name);
      if (object.input || object.constant || object.variable != (assignment.kind == vhdl::AssignmentKind::Variable))
        fail("dynamic assignment requires a writable object of the matching kind");
      // Simple combinational array-element writes use the same language-neutral
      // update family as SV. Nested targets retain the established replay path.
      auto& destination = object.variable ? state.variables.at(name) : state.scheduled.at(name);
      if (combinational_ && assignment.indices.size() == 1 && root.shape.ranges.size() == 1 &&
          root.shape.fields.empty() && !root.shape.enumeration &&
          !state.variableChoices.contains(name) &&
          std::all_of(destination.begin(), destination.end(), [](auto* bit) { return bit; })) {
        const auto bounds = root.shape.ranges.front();
        if (std::min(bounds.left, bounds.right) < 0) fail("dynamic negative-bound writes are unsupported");
        auto element = root.shape;
        element.ranges.erase(element.ranges.begin());
        element.removeOuterType();
        const auto address = expression(*assignment.indices.front(), state);
        checkIndexType(address, bounds);
        // Only use clipping when the binary index domain is provably in range;
        // VHDL runtime bounds failures must not silently become clipped writes.
        const bool inRange = !address.shape.signedInteger && address.bits.size() < 63 &&
            std::min(bounds.left, bounds.right) == 0 &&
            ((uint64_t{1} << address.bits.size()) - 1) <= uint64_t(std::max(bounds.left, bounds.right));
        if (inRange) {
          const auto value = expression(*assignment.value, state, &element);
          if (!compatible(element, value.shape)) fail("dynamic assignment target type mismatch");
          size_t extra = 0;
          for (size_t n = element.size(); n > 1; n >>= 1) ++extra;
          const size_t width = std::max<size_t>(33, address.bits.size() + 1) + extra + 1;
          auto offset = integerBits(address, width);
          const auto connect = [](SNLInstance* instance, SNLBusTerm* port, const Bits& bits) {
            for (size_t bit = 0; bit < bits.size(); ++bit)
              instance->setTermNet(port->getBit(bits.size() - 1 - bit), bits[bit]);
          };
          if (bounds.ascending || bounds.right) {
            const auto bound = number(bounds.right, {{"integer"}, {}, width}).bits;
            auto* model = NLDB0::getOrCreateAddSub(width, true);
            auto* instance = origin(SNLInstance::create(design_, model));
            connect(instance, NLDB0::getAddSubInputA(model), bounds.ascending ? bound : offset);
            connect(instance, NLDB0::getAddSubInputB(model), bounds.ascending ? offset : bound);
            auto* output = origin(SNLBusNet::create(design_, width - 1, 0));
            instance->setTermNet(NLDB0::getAddSubOutput(model), output);
            offset.clear();
            for (auto* bit : output->getBits()) offset.push_back(bit);
          }
          if (element.size() != 1) {
            // The supported fast-path shapes above are one-dimensional logic
            // arrays. Vector elements add a range; records and enumerations
            // are excluded. Retain scaling for future multi-bit scalar shapes.
            // LCOV_EXCL_START
            auto* model = NLDB0::getOrCreateMultiply(width);
            auto* instance = origin(SNLInstance::create(design_, model));
            connect(instance, NLDB0::getMultiplyInputA(model), offset);
            connect(instance, NLDB0::getMultiplyInputB(model),
                number(element.size(), {{"integer"}, {}, width}).bits);
            auto* output = origin(SNLBusNet::create(design_, width - 1, 0));
            instance->setTermNet(NLDB0::getMultiplyOutput(model), output);
            offset.clear();
            for (auto* bit : output->getBits()) offset.push_back(bit);
            // LCOV_EXCL_STOP
          }
          Bits base(destination.rbegin(), destination.rend());
          Bits data(value.bits.rbegin(), value.bits.rend());
          std::reverse(offset.begin(), offset.end());
          auto* output = origin(SNLBusNet::create(design_, destination.size() - 1, 0));
          origin(SNLRTLPrimitives::createVectorUpdate(design_, base, data, offset, true, constant(true), output));
          destination.clear();
          for (auto* bit : output->getBits()) destination.push_back(bit);
          if (!object.variable) std::fill(state.written.at(name).begin(), state.written.at(name).end(), true);
          return;
        }
      }
      std::vector<std::pair<Selection, SNLBitNet*>> targets{{root, constant(true)}};
      for (const auto& indexExpression : assignment.indices) {
        if (indexExpression->kind == Expr::Kind::Selected || indexExpression->kind == Expr::Kind::Range || isStaticIndex(*indexExpression)) {
          for (auto& [selected, guard] : targets) index(selected, *indexExpression);
          continue;
        }
        std::vector<std::pair<Selection, SNLBitNet*>> expanded;
        const auto address = expression(*indexExpression, state);
        const auto width = std::max<size_t>(32, address.bits.size());
        for (const auto& [base, guard] : targets) {
          if (base.shape.ranges.empty()) fail("dynamic assignment target is not an array");
          const auto bounds = base.shape.ranges.front();
          checkIndexType(address, bounds);
          auto element = base.shape;
          element.ranges.erase(element.ranges.begin());
          element.removeOuterType();
          const auto stride = element.size();
          if (expanded.size() + bounds.size() > 65536) fail("dynamic assignment expansion limit exceeded");
          for (size_t i = 0; i < bounds.size(); ++i) {
            const auto value = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
            if (value < 0) fail("dynamic negative-bound writes are unsupported");
            auto* matches = equal(integerBits(address, width), number(value, {{"integer"}, {}, width}).bits);
            expanded.push_back({{name, element, base.offset+i*stride}, gate("and", guard, matches)});
          }
        }
        targets = std::move(expanded);
      }
      if (targets.empty()) fail("dynamic assignment has no target elements");
      const auto value = expression(*assignment.value, state, &targets.front().first.shape);
      auto& bits = object.variable ? state.variables.at(name) : state.scheduled.at(name);
      for (const auto& [target, guard] : targets) {
        if ((object.variable || combinational_) && constantValue(guard) == false) continue;
        if (!compatible(target.shape, value.shape)) fail("dynamic assignment target type mismatch");
        const auto merged = mux(guard, value.bits, Bits(bits.begin()+target.offset, bits.begin()+target.offset+value.bits.size()));
        for (size_t i = 0; i < value.bits.size(); ++i) {
          if (object.variable) {
            auto old = variableChoice(state, name, target.offset+i);
            auto& choices = state.variableChoices[name];
            choices.resize(bits.size());
            choices[target.offset+i] = merged[i] ? nullptr : std::make_shared<VariableChoice>(VariableChoice{
                nullptr, guard, std::make_shared<VariableChoice>(VariableChoice{value.bits[i]}), old});
          }
          bits[target.offset+i] = merged[i];
          if (!object.variable) state.written.at(name)[target.offset+i] = true;
        }
      }
      return;
    }
    auto target = selection(assignment);
    const auto& object = objects_.at(target.name);
    if (object.input || object.constant || object.variable != (assignment.kind == vhdl::AssignmentKind::Variable))
      fail("assignment operator does not match a writable object");
    const auto value = expression(*assignment.value, state, &target.shape);
    auto& bits = object.variable ? state.variables.at(target.name) : state.scheduled.at(target.name);
    for (size_t i = 0; i < value.bits.size(); ++i) {
      bits[target.offset + i] = value.bits[i];
      if (object.variable && state.variableChoices.contains(target.name))
        state.variableChoices.at(target.name)[target.offset+i].reset();
      if (!object.variable) state.written.at(target.name)[target.offset + i] = true;
    }
  }

  void merge(State& state, const State& yes, const State& no, SNLBitNet* condition) {
    for (auto& [name, write] : state.memoryWrites) {
      const auto& a = yes.memoryWrites.at(name);
      const auto& b = no.memoryWrites.at(name);
      // Address and data are don't-care on branches with no write.
      write.address = !a.assigned ? b.address : !b.assigned ? a.address : mux(condition, a.address, b.address);
      write.data = !a.assigned ? b.data : !b.assigned ? a.data : mux(condition, a.data, b.data);
      write.enable = mux(condition, {a.enable}, {b.enable}).front();
      if (!write.mask.empty()) write.mask = mux(condition, a.mask, b.mask);
      write.assigned = a.assigned || b.assigned;
    }
    for (auto& [name, bits] : state.variables) {
      bits = mux(condition, yes.variables.at(name), no.variables.at(name));
      auto& choices = state.variableChoices[name];
      choices.resize(bits.size());
      for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) { choices[i].reset(); continue; }
        auto a = variableChoice(yes, name, i), b = variableChoice(no, name, i);
        choices[i] = a == b ? a : std::make_shared<VariableChoice>(VariableChoice{nullptr, condition, a, b});
      }
    }
    for (auto& [name, bits] : state.scheduled) {
      bits = mux(condition, yes.scheduled.at(name), no.scheduled.at(name));
      for (size_t i = 0; i < bits.size(); ++i) {
        state.written.at(name)[i] = yes.written.at(name)[i] || no.written.at(name)[i];
      }
    }
  }

  std::vector<SNLBitNet*> selectionMatches(const Value& selector,
      const std::vector<std::vector<std::unique_ptr<Expr>>>& alternatives) {
    const auto& type = selector.shape;
    if (!type.fields.empty() || type.ranges.size() > 1 || (type.enumeration && !type.ranges.empty()) ||
        (!type.enumeration && !type.integerWidth && type.types.back() != "bit" && type.types.back() != "std_logic" &&
         type.types.back() != "boolean")) fail("unsupported case selector type");
    std::set<std::string> seen;
    std::vector<SNLBitNet*> matches;
    const bool otherwise = alternatives.back().empty();
    for (const auto& choices : alternatives) {
      auto* match = constant(false);
      const auto add = [&](const Value& choice) {
        if (seen.size() >= 4096) fail("case choice expansion limit exceeded");
        std::string bits;
        for (auto* bit : choice.bits) {
          const auto value = constantValue(bit);
          if (!value) fail("case choices must be static binary values");
          bits += *value ? '1' : '0';
        }
        if (!seen.insert(bits).second) fail("duplicate or overlapping case choices");
        match = gate("or", match, equal(selector.bits, choice.bits));
      };
      for (const auto& choice : choices) {
        const auto requireStatic = [&](const auto& self, const Expr& value) -> void {
          if (value.kind == Expr::Kind::Name && objects_.contains(key(value)) &&
              !objects_.at(key(value)).constant) fail("case choices must be static");
          if (value.left) self(self, *value.left);
          if (value.right && value.kind != Expr::Kind::Selected) self(self, *value.right);
          if (value.condition) self(self, *value.condition);
          for (const auto& element : value.elements) self(self, *element);
        };
        requireStatic(requireStatic, *choice);
        if (choice->kind == Expr::Kind::Range) {
          if (!type.integerWidth) fail("case ranges require an integer selector");
          const Range bounds{integer(*choice->left), integer(*choice->right), choice->text == "to"};
          if (bounds.size() > 4096) fail("case choice expansion limit exceeded");
          for (size_t i = 0; i < bounds.size(); ++i) {
            const auto value = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
            if (value < 0 || (type.integerWidth < 63 && uint64_t(value) >= (uint64_t(1) << type.integerWidth)))
              fail("case integer choice is outside the supported selector range");
            add(number(value, type));
          }
        } else {
          if (type.integerWidth) {
            const auto value = integer(*choice);
            if (value < 0 || (type.integerWidth < 63 && uint64_t(value) >= (uint64_t(1) << type.integerWidth)))
              fail("case integer choice is outside the supported selector range");
          }
          add(expression(*choice, State{}, &type));
        }
      }
      matches.push_back(match);
    }
    if (!otherwise && (type.integerWidth || selector.bits.size() > 12 ||
        seen.size() != (type.enumeration ? type.enumeration->literals.size() : (size_t(1) << selector.bits.size()))))
      fail("case alternatives must cover every selector value; add others");
    return matches;
  }

  std::optional<Value> caseStatement(const Statement& statement, State& state, bool runtime = false,
      const RuntimeContinuation& completion = {}) {
    const auto lower = [&](const auto& body, State& branch) -> std::optional<Value> {
      if (runtime) return runtimeStatements(body, branch, completion);
      this->statements(body, branch);
      return std::nullopt;
    };
    const auto selector = expression(*statement.condition, state);
    const auto matches = selectionMatches(selector, statement.choices);
    const auto branch = [&](size_t selected, State& value) {
      for (size_t i = 0; i < matches.size(); ++i) if (!statement.choices[i].empty())
        value.assumptions[matches[i]] = i == selected;
      materializeVariableChoices(value);
      return lower(statement.caseBodies[selected], value);
    };
    bool allKnown = true;
    for (size_t i = 0; i < matches.size(); ++i) if (!statement.choices[i].empty()) {
      const auto selected = knownCondition(matches[i], state);
      if (selected && *selected) return branch(i, state);
      allKnown &= selected.has_value();
    }
    if (allKnown && statement.choices.back().empty()) return branch(matches.size()-1, state);
    if (std::all_of(selector.bits.begin(), selector.bits.end(),
        [&](auto* bit) { return constantValue(bit).has_value(); })) {
      for (size_t i = 0; i < matches.size(); ++i) {
        if ((statement.choices[i].empty()) || constantValue(matches[i]).value_or(false)) {
          return branch(i, state);
        }
      }
    }
    // A proven exhaustive last alternative is a valid default, avoiding a
    // fictitious unassigned path in combinational definite-assignment checks.
    auto result = state;
    auto returned = branch(statement.caseBodies.size()-1, result);
    for (size_t i = statement.caseBodies.size() - 1; i-- > 0;) {
      auto alternative = state;
      auto branchReturn = branch(i, alternative);
      if (bool(branchReturn) != bool(returned)) {
        if (!completion) fail("runtime function case has a path without a return");
        if (!branchReturn) branchReturn = completion(alternative);
        else returned = completion(result);
        if (!branchReturn || !returned) fail("runtime function case has a path without a return");
      }
      if (returned) *returned = mergeRuntimeResults(matches[i], *branchReturn, *returned);
      auto merged = state;
      merge(merged, alternative, result, matches[i]);
      result = std::move(merged);
    }
    state = std::move(result);
    return returned;
  }

  struct IntegerFacts {
    RTLConstructor& owner;
    std::map<std::string, int64_t> saved;
    IntegerFacts(RTLConstructor& owner, const State& state): owner(owner), saved(owner.integers_) {
      for (const auto& [name, bits] : state.variables) {
        const auto& type = owner.objects_.at(name).value.shape;
        if (!type.integerWidth || bits.empty() || bits.size() > 32) continue;
        owner.integers_.erase(name);
        uint32_t value = 0;
        bool known = true;
        for (auto* bit : bits) {
          const auto digit = owner.constantValue(bit);
          if (!digit) { known = false; break; }
          value = (value << 1) | *digit;
        }
        if (known) owner.integers_[name] = type.signedInteger && (value & (uint32_t(1) << (bits.size()-1))) ?
            int64_t(value) - (int64_t(1) << bits.size()) : int64_t(value);
      }
    }
    ~IntegerFacts() { owner.integers_ = std::move(saved); }
  };

  void statements(const std::vector<Statement>& statements, State& state) {
    for (const auto& statement : statements) {
      DiagnosticScope location(statement.span);
      IntegerFacts integerFacts(*this, state);
      if (++steps_ > 100000) fail("static process elaboration limit exceeded");
      if (statement.kind == Statement::Kind::Null) continue;
      if (statement.kind == Statement::Kind::Call) callRuntime(*statement.procedureCall, state, &state);
      else if (statement.kind == Statement::Kind::Case) caseStatement(statement, state);
      else if (statement.kind == Statement::Kind::Assignment) assign(statement.assignment, state);
      else if (statement.kind == Statement::Kind::If) {
        const auto condition = conditionValue(expression(*statement.condition, state));
        if (auto selected = knownCondition(condition.bits.front(), state)) {
          this->statements(*selected ? statement.statements : statement.alternative, state);
          continue;
        }
        auto yes = state, no = state;
        assume(yes, condition.bits.front(), true); assume(no, condition.bits.front(), false);
        this->statements(statement.statements, yes);
        this->statements(statement.alternative, no);
        merge(state, yes, no, condition.bits.front());
      } else if (statement.kind == Statement::Kind::For) {
        const auto bounds = range(statement.range);
        const auto name = key(statement.iterator);
        if (integers_.contains(name) || objects_.contains(name))
          fail("shadowed loop parameters are not supported");
        for (size_t i = 0; i < bounds.size(); ++i) {
          integers_[name] = bounds.ascending ? bounds.left + static_cast<int64_t>(i)
                                            : bounds.left - static_cast<int64_t>(i);
          this->statements(statement.statements, state);
        }
        integers_.erase(name);
      } else fail("return is only supported inside functions");
    }
  }

  void readBit(SNLBitNet* bit) {
    if (sensitivity_ && sensitivity_->signals.contains(bit) && !sensitivity_->bits.contains(bit))
      fail("process sensitivity omits a signal bit read outside the clock guard: " + sensitivity_->signals.at(bit));
    reads_.insert(bit);
    if (processReads_) processReads_->insert(bit);
  }

  struct LocalBindings {
    RTLConstructor& owner;
    std::set<std::string> names;
    std::vector<std::function<void()>> restore;
    explicit LocalBindings(RTLConstructor& constructor): owner(constructor) {}
    ~LocalBindings() { for (auto it = restore.rbegin(); it != restore.rend(); ++it) (*it)(); }
    template<typename Map> void hide(Map& map, const std::string& name) {
      auto saved = std::make_shared<typename Map::node_type>(map.extract(name));
      restore.push_back([&map, name, saved] {
        map.erase(name);
        if (!saved->empty()) map.insert(std::move(*saved));
      });
    }
    void declare(const std::string& name) {
      if (!names.insert(name).second) fail("duplicate local declaration: " + name);
      hide(owner.intentBindings_, name);
      hide(owner.objects_, name);
      hide(owner.aliases_, name);
      hide(owner.integers_, name);
      hide(owner.strings_, name);
      hide(owner.booleans_, name);
      hide(owner.functions_, name);
      hide(owner.enumLiterals_, name);
      hide(owner.subtypes_, name);
      hide(owner.arrays_, name);
      hide(owner.arrayTypeOffsets_, name);
      hide(owner.arrayTypeRanges_, name);
      hide(owner.recordOffsets_, name);
      hide(owner.records_, name);
      hide(owner.enumerations_, name);
      hide(owner.integerTables_, name);
      hide(owner.arrayDeclarations_, name);
      hide(owner.memories_, name);
      const bool wasLocal = owner.locallyStaticIntegers_.erase(name);
      restore.push_back([this, name, wasLocal] {
        owner.locallyStaticIntegers_.erase(name);
        if (wasLocal) owner.locallyStaticIntegers_.insert(name);
      });
    }
  };

  void addProcessDeclarations(const vhdl::ClockedProcess& process, LocalBindings& bindings) {
    std::map<size_t, std::function<void()>> ordered;
    for (const auto& array : process.arrayTypes)
      ordered.emplace(array.span.start.offset, [&array, &bindings, this] {
        bindings.declare(key(array.name)); addArray(array);
      });
    for (const auto& record : process.recordTypes)
      ordered.emplace(record.span.start.offset, [&record, &bindings, this] {
        bindings.declare(key(record.name)); addRecord(record);
      });
    for (const auto& enumeration : process.enumerationTypes)
      ordered.emplace(enumeration.span.start.offset, [&enumeration, &bindings, this] {
        bindings.declare(key(enumeration.name));
        for (const auto& literal : enumeration.literals) bindings.declare(key(literal));
        addEnumeration(enumeration);
      });
    for (const auto& subtype : process.subtypes)
      ordered.emplace(subtype.span.start.offset, [&subtype, &bindings, this] {
        bindings.declare(key(subtype.name)); addSubtype(subtype);
      });
    for (const auto& function : process.functions)
      ordered.emplace(function.span.start.offset, [&function, &bindings, this] {
        bindings.declare(key(function.name)); addLocalFunction(function);
      });
    for (const auto& constant : process.constants)
      ordered.emplace(constant.object.span.start.offset, [&constant, &bindings, this] {
        for (const auto& name : constant.object.names) bindings.declare(key(name));
        addConstant(constant);
      });
    for (const auto& variable : process.variables)
      ordered.emplace(variable.span.start.offset, [&variable, &bindings, &process, this] {
        const auto type = shape(variable.type);
        if (variable.initializer) {
          const auto initial = expression(*variable.initializer, State{}, &type);
          for (auto* bit : initial.bits) if (!bit->isConstant()) fail("variable initializer must be static");
        }
        // Definite assignment remains mandatory. An initializer can be ignored
        // only for a temporary whose initial value is never observed.
        for (const auto& name : variable.names) {
          if (!process.combinational && (key(name) == key(process.eventSignal) ||
              (process.resetSignal && key(name) == key(*process.resetSignal)))) fail("local variable shadows a clock/reset signal");
          bindings.declare(key(name));
          addObject(name, type, false, false, true);
        }
      });
    for (const auto& [offset, add] : ordered) add();
  }

  void removeProcessDeclarations(const vhdl::ClockedProcess& process) {
    for (const auto& variable : process.variables)
      for (const auto& name : variable.names) objects_.erase(key(name));
    for (const auto& constant : process.constants) for (const auto& name : constant.object.names) {
      const auto id = key(name);
      objects_.erase(id);
      integers_.erase(id);
      booleans_.erase(id);
      integerTables_.erase(id);
      locallyStaticIntegers_.erase(id);
    }
  }

  void checkCombinationalSelfReads(const State& state, const std::set<SNLBitNet*>& reads) {
    std::map<SNLBitNet*, SNLBitNet*> pending;
    for (const auto& [name, written] : state.written) {
      const auto& targets = objects_.at(name).value.bits;
      const auto& values = state.scheduled.at(name);
      for (size_t i = 0; i < written.size(); ++i) if (written[i]) pending.emplace(targets[i], values[i]);
    }
    std::map<SNLBitTerm*, std::set<SNLBitTerm*>> dependencies;
    std::set<SNLBitTerm*> activeOutputs;
    std::function<const std::set<SNLBitTerm*>&(SNLBitTerm*)> inputs = [&](SNLBitTerm* output) -> const std::set<SNLBitTerm*>& {
      if (const auto found = dependencies.find(output); found != dependencies.end()) return found->second;
      if (!activeOutputs.insert(output).second) fail("combinational cycle in child instance");
      auto* model = output->getDesign();
      std::set<SNLBitTerm*> result;
      if (NLDB0::isAssign(model) || NLDB0::isGate(model) || NLDB0::isMux2(model) ||
          NLDB0::isFA(model) || NLDB0::isTableSelect(model) || NLDB0::isShift(model)) {
        for (auto* term : model->getBitTerms()) if (term->getDirection() == SNLTerm::Direction::Input) result.insert(term);
      } else if (NLDB0::isMemory(model)) {
        for (auto* term : SNLDesignModeling::getCombinatorialInputs(output)) result.insert(term);
      } else if (!SNLDesignModeling::isSequential(model)) {
        if (model->isLeaf()) fail("cannot prove an acyclic combinational self-read through instance model: " + model->getName().getString());
        std::set<SNLBitNet*> visited, active;
        const auto trace = [&](const auto& self, SNLBitNet* net) -> void {
          if (!net || net->isConstant() || visited.contains(net)) return;
          if (!active.insert(net).second) fail("combinational cycle in child instance");
          for (auto* term : net->getBitTerms()) if (term->getDirection() == SNLTerm::Direction::Input) result.insert(term);
          for (auto* term : net->getInstTerms()) if (term->getDirection() == SNLTerm::Direction::Output)
            for (auto* input : inputs(term->getBitTerm())) self(self, term->getInstance()->getInstTerm(input)->getNet());
          active.erase(net);
          visited.insert(net);
        };
        trace(trace, output->getNet());
      }
      activeOutputs.erase(output);
      return dependencies.emplace(output, std::move(result)).first->second;
    };
    std::set<SNLBitNet*> visiting, complete;
    const auto visit = [&](const auto& self, SNLBitNet* bit) -> void {
      if (!bit || bit->isConstant() || complete.contains(bit)) return;
      if (!visiting.insert(bit).second) fail("combinational signal read creates a feedback cycle");
      if (const auto found = pending.find(bit); found != pending.end()) self(self, found->second);
      else for (auto* term : bit->getInstTerms()) if (term->getDirection() == SNLTerm::Direction::Output) {
        for (auto* input : inputs(term->getBitTerm()))
          self(self, term->getInstance()->getInstTerm(input)->getNet());
      }
      visiting.erase(bit);
      complete.insert(bit);
    };
    // VHDL reads the signal's previous delta value. An acyclic dependency
    // settles to the same wiring; a cycle would require state or oscillate.
    for (const auto& [target, source] : pending) if (reads.contains(target)) visit(visit, target);
  }

  void lowerCombinationalProcess(const vhdl::ClockedProcess& process) {
    if (!process.allSensitivity && process.sensitivityList.empty())
      fail("process without sensitivity is not synthesizable");
    const auto sensitivity = processSensitivity(process);
    LocalBindings bindings(*this);
    addProcessDeclarations(process, bindings);
    State state;
    for (const auto& [name, object] : objects_) {
      if (object.variable) state.variables[name] = object.value.bits;
      else if (!object.input && !object.constant && !memories_.contains(name)) {
        state.scheduled[name] = Bits(object.value.bits.size(), nullptr);
        state.written[name].resize(object.value.bits.size(), false);
      }
    }
    std::set<SNLBitNet*> processReads;
    processReads_ = &processReads;
    combinational_ = true;
    sensitivity_ = process.allSensitivity ? nullptr : &sensitivity;
    statements(process.statements, state);
    sensitivity_ = nullptr;
    combinational_ = false;
    processReads_ = nullptr;
    checkCombinationalSelfReads(state, processReads);
    for (const auto& [name, written] : state.written) {
      const auto& values = state.scheduled.at(name);
      const auto& targets = objects_.at(name).value.bits;
      for (size_t i = 0; i < written.size(); ++i) if (written[i]) {
        if (!values[i]) fail("incomplete combinational assignment would infer a latch: " + name);
        drive(targets[i], values[i]);
      }
    }
    removeProcessDeclarations(process);
  }

  void lowerProcess(const vhdl::ClockedProcess& process) {
    DiagnosticScope location(process.span);
    if (process.combinational) { lowerCombinationalProcess(process); return; }
    const auto clockName = key(process.eventSignal);
    auto clock = selection(clockName);
    if (!objects_.at(clockName).input || clock.shape.size() != 1 ||
        !clock.shape.ranges.empty() || (process.level != "'1'" && process.level != "'0'"))
      fail("clock must name a scalar input with a binary edge level");
    const bool falling = process.level == "'0'";
    if (falling && process.asynchronousReset)
      fail("falling-edge registers with asynchronous reset require an unsupported primitive capability");
    const auto sensitivity = processSensitivity(process);
    if (!sensitivity.names.contains(clockName)) fail("clock is absent from process sensitivity");
    if (key(process.levelSignal) != clockName) fail("clock event and level must match");
    if (process.asynchronousReset) {
      const auto name = key(*process.resetSignal);
      auto reset = selection(name);
      if (name == clockName || !sensitivity.names.contains(name))
        fail("asynchronous reset must be distinct from the clock and present in sensitivity");
      if (!reset.shape.ranges.empty() || reset.shape.size() != 1 ||
          (reset.shape.types != std::vector<std::string>{"bit"} &&
           reset.shape.types != std::vector<std::string>{"std_logic"}))
        fail("asynchronous reset must name a scalar logic signal");
      if (!process.assignments.empty())
        fail("assignments outside an asynchronous clock/reset guard are unsupported");
    }
    LocalBindings bindings(*this);
    addProcessDeclarations(process, bindings);
    std::map<std::string, Bits> variableStorage;
    std::map<SNLBitNet*, std::optional<bool>> variableInitials;
    std::set<SNLBitNet*> unsupportedVariableInitials;
    const auto defaults = [&](const auto& self, const Shape& type) -> std::vector<int> {
      if (!type.ranges.empty()) {
        auto element = type;
        element.ranges.erase(element.ranges.begin());
        element.removeOuterType();
        const auto bits = self(self, element);
        std::vector<int> result;
        for (size_t i = 0; i < type.ranges.front().size(); ++i)
          result.insert(result.end(), bits.begin(), bits.end());
        return result;
      }
      if (!type.fields.empty()) {
        std::vector<int> result;
        for (const auto& [name, field] : type.fields) {
          const auto bits = self(self, *field);
          result.insert(result.end(), bits.begin(), bits.end());
        }
        return result;
      }
      // Scalar integer subtype bounds are not retained by Shape yet. Leave
      // their initial value unspecified here and reject if storage is needed.
      const bool binary = type.enumeration || type.types.back() == "bit" || type.types.back() == "boolean";
      return std::vector<int>(type.size(), type.integerWidth ? -2 : binary ? 0 : -1);
    };
    for (const auto& declaration : process.variables) for (const auto& name : declaration.names) {
      auto& object = objects_.at(key(name));
      auto* storage = this->declaration(
          SNLBusNet::create(design_, object.value.shape.size()-1, 0), name.span);
      Bits bits;
      auto initial = defaults(defaults, object.value.shape);
      if (declaration.initializer) {
        const auto value = expression(*declaration.initializer, State{}, &object.value.shape);
        for (size_t i = 0; i < initial.size(); ++i) {
          const auto bit = constantValue(value.bits[i]);
          initial[i] = bit ? int(*bit) : value.bits[i]->isConstantZ() ? -3 : -1;
        }
      }
      for (size_t i = 0; i < object.value.shape.size(); ++i) {
        auto* bit = storage->getBitAtPosition(i);
        bits.push_back(bit);
        variableInitials.emplace(bit, initial[i] >= 0 ? std::optional<bool>{initial[i] != 0} : std::nullopt);
        if (initial[i] <= -2) unsupportedVariableInitials.insert(bit);
      }
      object.value.bits = bits;
      variableStorage.emplace(key(name), std::move(bits));
    }
    State state;
    for (const auto& [name, object] : objects_) {
      if (const auto memory = memories_.find(name); memory != memories_.end()) {
        if (memory->second.process == &process) {
          const auto& signature = memory->second.signature;
          state.memoryWrites[name] = {Bits(signature.abits, constant(false)),
              Bits(signature.width, constant(false)), constant(false), false,
              signature.independentWriteClocks ? Bits(signature.width, constant(false)) : Bits{}};
        }
      } else if (object.variable) state.variables[name] = object.value.bits;
      else if (!object.input && !object.constant) {
        state.scheduled[name] = object.value.bits;
        state.written[name].resize(object.value.bits.size(), false);
      }
    }
    auto resetState = state;
    SNLBitNet* resetActive = nullptr;
    if (process.asynchronousReset) {
      resetActive = control(*process.resetSignal, process.resetLevel, state);
      statements(process.resetStatements, resetState);
    }
    if (!process.sensitivityList.empty()) statements(process.statements, state);
    else {
      auto data = state;
      for (const auto& assignment : process.assignments) assign(assignment, data);
      if (process.enableSignal) {
        auto enable = control(*process.enableSignal, process.enableLevel, state);
        merge(state, data, state, enable);
      } else state = data;
      if (process.resetSignal) {
        auto reset = state;
        for (const auto& assignment : process.resetAssignments) assign(assignment, reset);
        auto select = control(*process.resetSignal, process.resetLevel, state);
        merge(state, reset, state, select);
      }
    }
    std::set<SNLBitNet*> live;
    const auto traceVariableInputs = [&](const auto& self, SNLBitNet* net) -> void {
      if (!net || net->isConstant() || !live.insert(net).second) return;
      for (auto* term : net->getInstTerms()) if (term->getDirection() == SNLTerm::Direction::Output)
        for (auto* input : SNLDesignModeling::getCombinatorialInputs(term->getBitTerm()))
          self(self, term->getInstance()->getInstTerm(input)->getNet());
    };
    for (const auto& [name, bits] : state.scheduled)
      for (size_t i = 0; i < bits.size(); ++i) if (state.written.at(name)[i])
        traceVariableInputs(traceVariableInputs, bits[i]);
    for (const auto& [name, bits] : resetState.scheduled)
      for (size_t i = 0; i < bits.size(); ++i) if (resetState.written.at(name)[i])
        traceVariableInputs(traceVariableInputs, bits[i]);
    for (const auto& [name, write] : state.memoryWrites) if (write.assigned) {
      for (auto* bit : write.address) traceVariableInputs(traceVariableInputs, bit);
      for (auto* bit : write.data) traceVariableInputs(traceVariableInputs, bit);
      for (auto* bit : write.mask) traceVariableInputs(traceVariableInputs, bit);
      traceVariableInputs(traceVariableInputs, write.enable);
    }
    // Storage data may itself depend on another variable's incoming value.
    for (size_t previous = 0; previous != live.size();) {
      previous = live.size();
      for (const auto& [name, targets] : variableStorage)
        for (size_t i = 0; i < targets.size(); ++i) if (live.contains(targets[i])) {
          traceVariableInputs(traceVariableInputs, state.variables.at(name)[i]);
          traceVariableInputs(traceVariableInputs, resetState.variables.at(name)[i]);
        }
    }
    // A clocked process variable persists between activations. Only incoming
    // values used by the lowered graph require storage; immediate temporaries
    // whose incoming values are overwritten remain combinational.
    for (const auto& [name, targets] : variableStorage) {
      auto& written = state.written[name];
      written.resize(targets.size(), false);
      auto& resetWritten = resetState.written[name];
      resetWritten.resize(targets.size(), false);
      state.scheduled[name] = state.variables.at(name);
      resetState.scheduled[name] = resetState.variables.at(name);
      for (size_t i = 0; i < targets.size(); ++i) {
        const bool retained = live.contains(targets[i]);
        if (!retained) {
          // Dead conditional temporary merges may still mention the incoming
          // value; tie these unused cones rather than leaving dangling inputs.
          if (!targets[i]->getInstTerms().empty())
            drive(targets[i], variableInitials.at(targets[i]) ?
                constant(*variableInitials.at(targets[i])) : logicLiteral('X', {{"std_logic"}, {}}));
          continue;
        }
        if (unsupportedVariableInitials.contains(targets[i]))
          fail("retained variables require supported initial values; integer defaults and Z initializers are unsupported");
        written[i] = true;
        if (const auto initial = variableInitials.at(targets[i])) initialValues_[targets[i]] = *initial;
        resetWritten[i] = resetState.variables.at(name)[i] != targets[i];
      }
    }
    std::map<SNLBitNet*, bool> asynchronousValues;
    if (process.asynchronousReset) {
      for (auto& [name, bits] : state.scheduled) {
        const auto& targets = objects_.at(name).value.bits;
        for (size_t i = 0; i < bits.size(); ++i) {
          if (resetState.written.at(name)[i]) {
            const auto resetValue = constantValue(resetState.scheduled.at(name)[i]);
            if (!resetValue) fail("asynchronous reset assignments must produce constant binary values");
            asynchronousValues.emplace(targets[i], *resetValue);
            state.written.at(name)[i] = true;
          } else if (state.written.at(name)[i]) {
            // Bits omitted from the reset branch hold even on a clock edge
            // while reset is asserted. They do not acquire an invented reset.
            bits[i] = mux(resetActive, {targets[i]}, {bits[i]}).front();
          }
        }
      }
    }
    const auto resetKind = [&](SNLBitNet* net) {
      const auto found = asynchronousValues.find(net);
      return found == asynchronousValues.end() ? -1 : int(found->second);
    };
    for (const auto& [name, write] : state.memoryWrites) {
      if (falling) fail("falling-edge memory writes require an unsupported primitive capability");
      auto& memory = memories_.at(name);
      memory.write = write;
      memory.clock = objects_.at(clockName).value.bits.front();
    }
    for (const auto& [name, bits] : state.scheduled) {
      const auto& targets = objects_.at(name).value.bits;
      const auto& written = state.written.at(name);
      for (size_t first = 0; first < bits.size();) {
        if (!written[first]) { ++first; continue; }
        const bool initialized = initialValues_.contains(targets[first]);
        const auto asynchronousKind = resetKind(targets[first]);
        auto* busBit = dynamic_cast<SNLBusNetBit*>(targets[first]);
        size_t end = first + 1;
        while (busBit && end < bits.size() && written[end] &&
               initialValues_.contains(targets[end]) == initialized &&
               resetKind(targets[end]) == asynchronousKind) {
          auto* next = dynamic_cast<SNLBusNetBit*>(targets[end]);
          if (!next || next->getBus() != busBit->getBus()) break;
          ++end;
        }
        const auto width = end - first;
        auto* model = asynchronousKind < 0 ? (falling ? NLDB0::getOrCreateDFFN(width) : NLDB0::getOrCreateDFF(width)) :
            asynchronousKind == 1 ? NLDB0::getOrCreateDFFS(width) :
            process.resetLevel == "'0'" ? NLDB0::getOrCreateDFFRN(width) : NLDB0::getOrCreateDFFR(width);
        auto* flop = origin(SNLInstance::create(design_, model));
        flop->setTermNet(model->getScalarTerm(NLName("C")), objects_.at(clockName).value.bits.front());
        if (asynchronousKind >= 0) {
          const auto* pin = asynchronousKind == 1 ? "S" : process.resetLevel == "'0'" ? "RN" : "R";
          auto* reset = asynchronousKind == 0 && process.resetLevel == "'0'" ?
              objects_.at(key(*process.resetSignal)).value.bits.front() : resetActive;
          flop->setTermNet(model->getScalarTerm(NLName(pin)), reset);
        }
        std::string initialBits;
        for (size_t i = first; i < end; ++i) {
          auto* target = targets[i];
          if (!drivers_.insert(target).second) fail("multiple drivers for clocked signal");
          const auto position = end - 1 - i;
          SNLBitTerm* dataTerm = width == 1 ? static_cast<SNLBitTerm*>(model->getScalarTerm(NLName("D")))
              : model->getBusTerm(NLName("D"))->getBit(position);
          SNLBitTerm* outputTerm = width == 1 ? static_cast<SNLBitTerm*>(model->getScalarTerm(NLName("Q")))
              : model->getBusTerm(NLName("Q"))->getBit(position);
          flop->setTermNet(dataTerm, bits[i]);
          flop->setTermNet(outputTerm, target);
          if (initialized) {
            initialBits += initialValues_.at(target) ? '1' : '0';
            initializedFlops_.insert(target);
          }
        }
        if (initialized)
          SNLInstParameter::create(flop, model->getParameter(NLName("INIT")),
              NLDB0::formatDFFInitValue(width, initialBits));
        first = end;
      }
    }
    if (!process.sensitivityList.empty()) {
      sensitivity_ = &sensitivity;
      for (const auto& assignment : process.assignments) concurrent(assignment, state);
      sensitivity_ = nullptr;
    }
    removeProcessDeclarations(process);
  }
  SNLBitNet* control(const vhdl::Name& name, const std::string& level, const State& state) {
    Expr expr;
    expr.kind = Expr::Kind::Name;
    expr.span = name.span;
    expr.canonical = key(name);
    auto value = expression(expr, state);
    if (!value.shape.ranges.empty() || (level != "'0'" && level != "'1'"))
      fail("control must compare a scalar signal to '0' or '1'");
    return level == "'1'" ? value.bits.front() : gate("not", value.bits.front());
  }

  bool captureIntent_ = false;
  VHDLIntentValue::List intentDeclarations_;
  std::map<std::string, VHDLIntentValue> intentBindings_;
  std::map<size_t, VHDLIntentValue> intentObjects_;
  std::map<const void*, VHDLIntentValue> intentNominalTypes_;
  std::map<const void*, VHDLIntentValue::List> intentRecordFields_;
  SNLDesign* design_;
  const vhdl::EntityDeclaration& entity_;
  const vhdl::ArchitectureBody& architecture_;
  const vhdl::DesignFile& syntax_;
  const LibraryContext& libraryContext_;
  NLLibrary* work_;
  GenericValues genericOverrides_;
  std::function<GenericValue(const std::string&, const Shape*)> genericActual_;
  std::function<SNLDesign*(NLLibrary*, const std::string&, const GenericValues&)> build_;
  std::set<std::pair<NLLibrary*, std::string>> imported_;
  std::map<std::string, const vhdl::EntityDeclaration*> components_;
  bool unsigned_ = false, arith_ = false, numeric_ = false, signed_ = false;
  std::map<std::string, IntegerTable> integerTables_;
  std::map<std::string, const vhdl::ArrayTypeDeclaration*> arrayDeclarations_;
  std::set<std::string> instanceNames_;
  std::map<std::string, int64_t> integers_;
  std::map<std::string, std::string> strings_;
  std::set<std::string> locallyStaticIntegers_;
  SubtypeMap subtypes_;
  std::map<std::string, FunctionBinding> functions_;
  std::map<const vhdl::FunctionDeclaration*, FunctionEnvironment> localFunctionEnvironments_;
  std::map<std::string, bool> booleans_;
  std::set<const vhdl::ConstantDeclaration*> activeStaticConstants_;
  size_t staticDepth_ = 0;
  size_t runtimeDepth_ = 0;
  std::shared_ptr<size_t> runtimeSteps_ = std::make_shared<size_t>(0);
  std::shared_ptr<size_t> runtimeControlDepth_ = std::make_shared<size_t>(0);
  std::optional<vhdl::TypeMark> runtimeReturnType_;
  std::optional<Shape> runtimeReturnShape_;
  std::map<std::string, Shape> arrays_, records_;
  std::map<std::string, const vhdl::EnumerationTypeDeclaration*> enumerations_;
  std::map<std::string, size_t> recordOffsets_;
  std::map<std::string, size_t> arrayTypeOffsets_;
  std::map<std::string, Range> arrayTypeRanges_;
  std::map<std::string, Object> objects_;
  std::map<std::string, std::vector<Value>> enumLiterals_;
  std::string objectPrefix_;
  std::vector<std::pair<std::string, Object>> generatedObjects_;
  std::map<std::string, Memory> memories_;
  std::map<std::string, PendingSignal> pendingSignals_;
  bool initializerContext_ = false;
  std::shared_ptr<std::map<std::string, std::string>> initializationFiles_ =
      std::make_shared<std::map<std::string, std::string>>();
  bool textio_ = false;
  std::map<std::string, std::shared_ptr<std::ifstream>> files_;
  std::map<std::string, std::optional<std::string>> lines_;
  std::map<std::string, Selection> aliases_;
  std::map<std::pair<std::string, Bits>, SNLBitNet*> gates_;
  std::map<std::tuple<Bits, Bits, bool>, Bits> adders_;
  std::map<std::tuple<SNLBitNet*, Bits, Bits>, Bits> muxes_;
  std::map<std::pair<SNLDesign*, Bits>, Bits> tables_;
  std::set<SNLBitNet*> drivers_, reads_;
  std::map<SNLBitNet*, bool> initialValues_;
  std::map<SNLBitNet*, std::pair<std::string, vhdl::SourceSpan>> initialDeclarations_;
  std::set<SNLBitNet*> initializedFlops_;
  std::set<std::string> libraries_{"std", "work"};
  const Sensitivity* sensitivity_ = nullptr;
  std::set<SNLBitNet*>* processReads_ = nullptr;
  bool combinational_ = false;
  bool stdLogic_ = false;
  size_t steps_ = 0;
  SNLBitNet* zero_ = nullptr;
  SNLBitNet* one_ = nullptr;
  SNLBitNet* unknown_ = nullptr;
  SNLBitNet* highImpedance_ = nullptr;
  // Destroy first, while the symbol maps referenced by restoration remain live.
  std::unique_ptr<LocalBindings> interfaceBindings_;
};
}

bool requiresVHDLRTL(const vhdl::DesignFile& syntax) {
  if (!syntax.packages.empty()) return true;
  const auto rtlContext = [](const vhdl::ContextClause& context) {
    return std::any_of(context.uses.begin(), context.uses.end(), [](const auto& use) {
      return use.selectedName.size() == 3 && (key(use.selectedName[0]) != "ieee" ||
          key(use.selectedName[1]) == "std_logic_signed" ||
          key(use.selectedName[1]) == "std_logic_unsigned" ||
          key(use.selectedName[1]) == "std_logic_arith");
    }); // LCOV_EXCL_LINE: compiler-generated exception cleanup for the context predicate.
  };
  const auto extendedExpression = [](const auto& self, const Expr* expression) -> bool {
    return expression && ((expression->kind == Expr::Kind::IntegerLiteral && expression->text.find_first_of("#eE") != std::string::npos) ||
        expression->kind == Expr::Kind::Selected || expression->kind == Expr::Kind::Indexed || expression->kind == Expr::Kind::Call ||
        expression->kind == Expr::Kind::Others || expression->kind == Expr::Kind::Aggregate ||
        expression->kind == Expr::Kind::BitStringLiteral || expression->kind == Expr::Kind::Attribute ||
        ((expression->kind == Expr::Kind::CharacterLiteral || expression->kind == Expr::Kind::StringLiteral) &&
         expression->text.find_first_of("XZ") != std::string::npos) ||
        self(self, expression->left.get()) ||
        self(self, expression->right.get()) || self(self, expression->condition.get()));
  };
  for (const auto& entity : syntax.entities) {
    if (rtlContext(entity.context)) return true;
    for (const auto& generic : entity.generics)
      if (generic.type.constraint || !generic.defaultValue || extendedExpression(extendedExpression, generic.defaultValue.get()) ||
          key(generic.type.name) == "boolean" ||
          key(generic.type.name) == "bit_vector" || key(generic.type.name) == "std_logic_vector" ||
          key(generic.type.name) == "std_ulogic_vector" || key(generic.type.name) == "unsigned" ||
          key(generic.type.name) == "signed") return true;
    for (const auto& port : entity.ports) if (key(port.type.name) == "boolean" || (port.type.constraint &&
        (port.type.constraint->attribute ||
         extendedExpression(extendedExpression, port.type.constraint->leftExpression.get()) ||
         extendedExpression(extendedExpression, port.type.constraint->rightExpression.get()))) || port.defaultValue || key(port.type.name) == "std_ulogic" || key(port.type.name) == "std_ulogic_vector" || key(port.type.name) == "unsigned" || key(port.type.name) == "signed") return true;
  }
  const auto extendedAssignment = [&](const vhdl::Assignment& assignment) {
    return assignment.selector || !assignment.indices.empty() || extendedExpression(extendedExpression, assignment.value.get());
  };
  for (const auto& architecture : syntax.architectures) {
    std::set<std::string> outputs;
    for (const auto& entity : syntax.entities) if (key(entity.name) == key(architecture.entity))
      for (const auto& port : entity.ports) if (port.mode == vhdl::PortMode::Out)
        for (const auto& name : port.names) outputs.insert(key(name));
    const auto readsOutput = [&](const auto& self, const Expr* expression) -> bool {
      if (!expression) return false;
      if (expression->kind == Expr::Kind::Name && outputs.contains(key(*expression))) return true;
      if (self(self, expression->left.get()) || self(self, expression->right.get()) || self(self, expression->condition.get())) return true;
      return std::any_of(expression->elements.begin(), expression->elements.end(),
          [&](const auto& element) { return self(self, element.get()); });
    };
    for (const auto& assignment : architecture.assignments)
      if (readsOutput(readsOutput, assignment.value.get())) return true;
    for (const auto& process : architecture.processes) for (const auto& assignment : process.assignments)
      if (readsOutput(readsOutput, assignment.value.get())) return true;
    if (!architecture.functions.empty() || !architecture.subtypes.empty()) return true;
    if (!architecture.arrayTypes.empty() || !architecture.recordTypes.empty() || !architecture.enumerationTypes.empty()) return true;
    if (rtlContext(architecture.context)) return true;
    if (!architecture.generates.empty() || !architecture.components.empty() || !architecture.constants.empty()) return true;
    for (const auto& signal : architecture.signals) if (signal.initializer || key(signal.type.name) == "boolean" || key(signal.type.name) == "std_ulogic" || key(signal.type.name) == "std_ulogic_vector" || key(signal.type.name) == "unsigned" || key(signal.type.name) == "signed") return true;
    for (const auto& instance : architecture.instantiations) {
      if (key(instance.library) != "work" || std::any_of(instance.actualOpen.begin(), instance.actualOpen.end(),
          [](bool open) { return open; }) || std::any_of(instance.actualLiterals.begin(), instance.actualLiterals.end(),
          [](const auto& literal) { return bool(literal); }) || std::any_of(instance.actualIndices.begin(), instance.actualIndices.end(),
          [](const auto& indices) { return !indices.empty(); }) || instance.component || std::any_of(instance.formals.begin(), instance.formals.end(),
          [](const auto& formal) { return formal.has_value(); })) return true;
      // The structural loader requires every actual. Omitted trailing outputs
      // need the RTL loader's direction-aware association handling.
      for (const auto& entity : syntax.entities) if (key(entity.name) == key(instance.entity)) {
        size_t portCount = 0;
        for (const auto& port : entity.ports) portCount += port.names.size();
        if (instance.actuals.size() < portCount) return true;
      }
    }
    for (const auto& assignment : architecture.assignments)
      if (extendedAssignment(assignment)) return true;
    for (const auto& process : architecture.processes) {
      if (!process.constants.empty() || std::any_of(process.variables.begin(), process.variables.end(),
          [](const auto& variable) { return bool(variable.initializer); })) return true;
      if (process.combinational || !process.sensitivityList.empty()) return true;
      for (const auto& assignment : process.assignments)
        if (extendedAssignment(assignment)) return true;
      for (const auto& assignment : process.resetAssignments)
        if (extendedAssignment(assignment)) return true;
    }
  }
  return false;
}

SNLDesign* constructVHDLRTL(NLLibrary* library, const vhdl::DesignFile& syntax,
                          std::string_view top, std::string_view source,
                          const std::vector<VHDLLibrarySource>& libraries,
                          const VHDLSourceMap& sourceMap, const VHDLIntent::SessionPtr& intentSession) {
  const LibraryContext context{library, libraries, syntax, sourceMap, source, intentSession};
  std::set<std::pair<NLLibrary*, std::string>> packages, bodies;
  for (const auto& package : syntax.packages) {
    DiagnosticScope location(package.name.span);
    const auto name = std::make_pair(context.owner(package.name.span.start.offset), key(package.name));
    if (!package.body && !packages.insert(name).second) fail("duplicate package");
    if (package.body && !bodies.insert(name).second)
      fail("duplicate package body or body without declaration");
  }
  for (const auto& package : syntax.packages) {
    DiagnosticScope location(package.name.span);
    if (package.body && !packages.contains(
        {context.owner(package.name.span.start.offset), key(package.name)}))
      fail("package body without declaration");
  }
  // Resolve package dependencies from the complete index before importing them.
  std::set<const vhdl::PackageDeclaration*> visiting, visited;
  std::function<void(const vhdl::PackageDeclaration&)> visitPackage;
  visitPackage = [&](const auto& package) {
    if (visited.contains(&package)) return;
    visiting.insert(&package);
    auto* owner = context.owner(package.name.span.start.offset);
    for (const auto& use : package.context.uses) {
      DiagnosticScope location(use.span);
      if (use.selectedName.size() != 3) continue;
      const auto libraryName = key(use.selectedName[0]);
      if (libraryName == "std" || libraryName == "ieee") continue;
      auto* target = context.resolve(owner, libraryName);
      const auto name = key(use.selectedName[1]);
      const vhdl::PackageDeclaration* dependency = nullptr;
      for (const auto& candidate : syntax.packages)
        if (!candidate.body && key(candidate.name) == name &&
            context.owner(candidate.name.span.start.offset) == target) dependency = &candidate;
      if (!dependency) fail("missing package: " + name);
      if (visiting.contains(dependency)) fail("package dependency cycle: " + name);
      visitPackage(*dependency);
    }
    visiting.erase(&package);
    visited.insert(&package);
  };
  for (const auto& package : syntax.packages) visitPackage(package);
  if (syntax.entities.empty() && syntax.architectures.empty()) {
    if (!top.empty()) fail("package source does not define a top entity");
    if (intentSession) {
      // Package-only loads have no SNL design or effective generic environment.
      const vhdl::EntityDeclaration entity;
      const vhdl::ArchitectureBody architecture;
      RTLConstructor catalogue(nullptr, entity, architecture, syntax, context, {}, {});
      catalogue.retainPackages(library);
    }
    return nullptr;
  }
  bool noRoot = false;
  std::string selected(top);
  for (char& c : selected) c = std::tolower(static_cast<unsigned char>(c));
  if (selected.empty()) {
    std::set<std::string> candidates;
    for (const auto& entity : syntax.entities)
      if (context.owner(entity.span.start.offset) == library) candidates.insert(key(entity.name));
    const auto removeChildren = [&](const auto& self, const auto& scope) -> void {
      for (const auto& instance : scope.instantiations)
        if (key(instance.library) == "work" ||
            context.resolve(library, key(instance.library)) == library) candidates.erase(key(instance.entity));
      for (const auto& generate : scope.generates) self(self, generate);
      if constexpr (requires { scope.alternatives; })
        for (const auto& branch : scope.alternatives) self(self, branch);
    };
    for (const auto& architecture : syntax.architectures)
      if (context.owner(architecture.span.start.offset) == library) removeChildren(removeChildren, architecture);
    // With no root, follow a member of the cycle so elaboration reports the
    // offending reference, including its source location.
    if (candidates.empty()) {
      noRoot = true;
      for (const auto& entity : syntax.entities)
        if (context.owner(entity.span.start.offset) == library) {
          candidates.insert(key(entity.name));
          break;
        }
    }
    if (candidates.size() != 1)
      fail("cannot infer a unique RTL top entity; specify an explicit top");
    selected = *candidates.begin();
  }
  std::vector<SNLDesign*> created;
  std::set<std::pair<NLLibrary*, std::string>> active;
  std::function<SNLDesign*(NLLibrary*, const std::string&, const GenericValues&)> build;
  build = [&](NLLibrary* owner, const std::string& name, const GenericValues& overrides) -> SNLDesign* {
    if (!active.insert({owner, name}).second) fail("entity dependency cycle (recursive RTL hierarchy): " + name);
    const vhdl::EntityDeclaration* entity = nullptr;
    const vhdl::ArchitectureBody* architecture = nullptr;
    for (const auto& candidate : syntax.entities) if (context.owner(candidate.span.start.offset) == owner && key(candidate.name) == name) {
      DiagnosticScope location(candidate.span);
      if (entity) fail("duplicate RTL entity");
      entity = &candidate;
    }
    for (const auto& candidate : syntax.architectures) if (context.owner(candidate.span.start.offset) == owner && key(candidate.entity) == name) {
      DiagnosticScope location(candidate.span);
      if (architecture) fail("multiple RTL architectures are unsupported");
      architecture = &candidate;
    }
    if (!entity || !architecture) fail("missing RTL entity or architecture: " + name);
    std::string modelName = entity->name.spelling;
    for (const auto& [generic, value] : overrides) {
      modelName += "__" + generic + "_";
      if (value.text) {
        modelName += "s" + std::to_string(value.text->size()) + "_";
        for (const unsigned char c : *value.text) {
          modelName += "0123456789abcdef"[c >> 4];
          modelName += "0123456789abcdef"[c & 15];
        }
      } else modelName += value.bits ? "v" + *value.bits : value.scalar.boolean ?
          (value.scalar.value ? "true" : "false") : std::to_string(value.scalar.value);
    }
    const auto signature = std::string(source.substr(context.start(owner),
        std::max(entity->span.end.offset, architecture->span.end.offset) - context.start(owner))) + modelName;
    if (auto* existing = owner->getSNLDesign(NLName(modelName))) {
      auto* property = dynamic_cast<RTLSourceProperty*>(existing->getProperty("VHDLRTLSource"));
      if (!property || property->signature != signature) fail("conflicting RTL design: " + modelName);
      for (const auto& [path, previous] : property->initializationFiles) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) fail("cannot open initialization file for cached design: " + path);
        const std::string current((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (file.bad() || current != previous)
          fail("initialization file changed for existing design; use a fresh library: " + path);
      }
      active.erase({owner, name});
      return existing;
    }
    auto* design = SNLDesign::create(owner, NLName(modelName));
    created.push_back(design);
    sourceMap.annotate(design, entity->span, true);
    sourceMap.annotate(design, architecture->span);
    RTLConstructor constructor(design, *entity, *architecture, syntax, context, overrides, build);
    constructor.run();
    SNLUtils::mergeRepeatedMuxSelections(design);
    RTLSourceProperty::attach(design, signature, constructor.getInitializationFiles());
    active.erase({owner, name});
    return design;
  };
  try {
    auto* design = build(library, selected, {});
    if (noRoot) fail("cannot infer a unique RTL top entity; specify an explicit top");
    return design;
  } catch (...) {
    for (auto* design : created) design->destroy();
    throw;
  }
}
}
