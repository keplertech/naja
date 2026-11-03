// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "NLContentHash.h"

#include <algorithm>
#include <tuple>

#include "sha-256.h"
#include "NajaDumpableProperty.h"
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLParameter.h"
#include "SNLInstParameter.h"
#include "SNLAttributes.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBundleTerm.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

namespace naja::NL {
namespace {

// All encoding lives here. See docs/content_hash.md before changing anything.
enum Tag : uint8_t {
  BitTerm = 1, Term, BitNet, Net, Instance, Design, Library, DB, Universe,
  Primitive, Attributes = 32, Properties, Terms, Nets, Instances, Designs,
  Libraries, DBs, Connections, Parameters, Bits, Members
};

// Explicit mappings freeze the wire codes independently of C++ enum order.
uint8_t code(SNLTerm::Direction direction) {
  switch (static_cast<SNLTerm::Direction::DirectionEnum>(direction)) {
    case SNLTerm::Direction::Input: return 0;
    case SNLTerm::Direction::Output: return 1;
    case SNLTerm::Direction::InOut: return 2;
    case SNLTerm::Direction::Undefined: return 3;
  }
  throw NLException("Unsupported content hash term direction"); // LCOV_EXCL_LINE Defensive fallback: all declared directions handled above.
}

uint8_t code(SNLNet::Type type) {
  switch (static_cast<SNLNet::Type::TypeEnum>(type)) {
    case SNLNet::Type::Standard: return 0;
    case SNLNet::Type::Assign0: return 1;
    case SNLNet::Type::Assign1: return 2;
    case SNLNet::Type::Supply0: return 3;
    case SNLNet::Type::Supply1: return 4;
    case SNLNet::Type::AssignX: return 5;
    case SNLNet::Type::AssignZ: return 6;
  }
  throw NLException("Unsupported content hash net type"); // LCOV_EXCL_LINE Defensive fallback: all declared net types handled above.
}

uint8_t code(SNLDesign::Type type) {
  switch (static_cast<SNLDesign::Type::TypeEnum>(type)) {
    case SNLDesign::Type::Standard: return 0;
    case SNLDesign::Type::UserBlackBox: return 1;
    case SNLDesign::Type::AutoBlackBox: return 2;
    case SNLDesign::Type::Primitive: return 3;
  }
  throw NLException("Unsupported content hash design type"); // LCOV_EXCL_LINE Defensive fallback: all declared design types handled above.
}

uint8_t code(NLLibrary::Type type) {
  switch (static_cast<NLLibrary::Type::TypeEnum>(type)) {
    case NLLibrary::Type::Standard: return 0;
    case NLLibrary::Type::InDB0: return 1;
    case NLLibrary::Type::Primitives: return 2;
  }
  throw NLException("Unsupported content hash library type"); // LCOV_EXCL_LINE Defensive fallback: all declared library types handled above.
}

uint8_t code(SNLParameter::Type type) {
  switch (static_cast<SNLParameter::Type::TypeEnum>(type)) {
    case SNLParameter::Type::Decimal: return 0;
    case SNLParameter::Type::Binary: return 1;
    case SNLParameter::Type::Boolean: return 2;
    case SNLParameter::Type::String: return 3;
  }
  throw NLException("Unsupported content hash parameter type"); // LCOV_EXCL_LINE Defensive fallback: all declared parameter types handled above.
}

class Encoder {
  public:
    explicit Encoder(Tag tag) { bytes_.reserve(512); byte(tag); u64(kContentHashVersion); }
    void byte(uint8_t value) { bytes_.push_back(value); }
    void u64(uint64_t value) {
      for (unsigned i = 0; i < 8; ++i) { byte(value & 255); value >>= 8; }
    }
    void string(const std::string& value) {
      u64(value.size()); bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
    void digest(const ContentHash& hash) { bytes_.insert(bytes_.end(), hash.begin(), hash.end()); }
    void sequence(Tag tag, size_t count) { byte(tag); u64(count); }
    void reference(const NLID::DesignReference& ref) {
      u64(ref.dbID_); u64(ref.libraryID_); u64(ref.designID_);
    }
    void connection(const SNLBitNet* net) {
      byte(net != nullptr);
      if (net) {
        u64(net->getID());
        auto bit = dynamic_cast<const SNLBusNetBit*>(net);
        byte(bit != nullptr); u64(bit ? static_cast<uint64_t>(bit->getBit()) : 0);
      }
    }
    void metadata(const NLObject* object) {
      std::vector<SNLAttribute> attributes;
      std::vector<naja::NajaDumpableProperty*> properties;
      if (object->hasProperties()) {
        for (const auto& attribute : SNLAttributes::getAttributes(object)) {
          const auto& names = getContentHashAttributeNames();
          if (std::find(names.begin(), names.end(), attribute.getName().getString()) != names.end()) {
            attributes.push_back(attribute);
          }
        }
        for (auto property : object->getDumpableProperties()) {
          const auto& names = getContentHashPropertyNames();
          if (std::find(names.begin(), names.end(), property->getName()) != names.end()) {
            // LCOV_EXCL_START Version 1 has an empty, fixed property registry.
            properties.push_back(property);
          }
          // LCOV_EXCL_STOP
        }
      }
      std::sort(attributes.begin(), attributes.end(), [](const auto& a, const auto& b) {
        const auto nameA = a.getName().getString(), nameB = b.getName().getString();
        return nameA != nameB ? nameA < nameB : a.getValue() < b.getValue();
      });
      sequence(Attributes, attributes.size());
      for (const auto& attribute : attributes) {
        string(attribute.getName().getString());
        byte(attribute.getValue().isString() ? 1 : 0);
        string(attribute.getValue().getString());
      }
      // LCOV_EXCL_START Version 1 has no registered properties to sort or encode.
      std::sort(properties.begin(), properties.end(), [](auto a, auto b) { return a->getName() < b->getName(); });
      // LCOV_EXCL_STOP
      sequence(Properties, properties.size());
      for (auto property : properties) {
        // LCOV_EXCL_START Version 1 has an empty, fixed property registry.
        string(property->getName());
        const auto values = property->getValues();
        u64(values.size());
        for (const auto& value : values) {
          if (auto str = std::get_if<std::string>(&value)) { byte(0); string(*str); }
          else { byte(1); u64(std::get<uint64_t>(value)); }
        }
      }
      // LCOV_EXCL_STOP
    }
    ContentHash finish() const {
      ContentHash result;
      calc_sha_256(result.data(), bytes_.data(), bytes_.size());
      return result;
    }
  private:
    std::vector<uint8_t> bytes_;
};

template<typename T> auto byID(const NajaCollection<T*>& collection) {
  std::vector<T*> objects(collection.begin(), collection.end());
  std::sort(objects.begin(), objects.end(), [](auto a, auto b) { return a->getID() < b->getID(); });
  return objects;
}

template<typename T> auto byName(const NajaCollection<T*>& collection) {
  std::vector<T*> objects(collection.begin(), collection.end());
  std::sort(objects.begin(), objects.end(), [](auto a, auto b) { return a->getName().getString() < b->getName().getString(); });
  return objects;
}

auto bitTerms(const NajaCollection<SNLBitTerm*>& collection) {
  std::vector<SNLBitTerm*> bits(collection.begin(), collection.end());
  std::sort(bits.begin(), bits.end(), SNLBitTerm::InDesignLess());
  return bits;
}

ContentHash hashBitTerm(const SNLBitTerm* term) {
  Encoder out(BitTerm);
  out.u64(term->getID()); out.u64(static_cast<uint64_t>(term->getBit()));
  out.byte(code(term->getDirection())); out.metadata(term);
  return out.finish();
}

ContentHash hashBitNet(const SNLBitNet* net) {
  Encoder out(BitNet);
  out.u64(net->getID());
  auto bit = dynamic_cast<const SNLBusNetBit*>(net);
  out.u64(bit ? static_cast<uint64_t>(bit->getBit()) : 0);
  out.byte(code(net->getType())); out.metadata(net);
  return out.finish();
}

bool excluded(const NLLibrary* library) {
  return library->isPrimitives() || library->isInDB0();
}

} // namespace

std::string toHex(const ContentHash& hash) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result(64, '0');
  for (size_t i = 0; i < hash.size(); ++i) {
    result[2*i] = digits[hash[i] >> 4]; result[2*i+1] = digits[hash[i] & 15];
  }
  return result;
}

const std::vector<std::string>& getContentHashAttributeNames() {
  static const std::vector<std::string> names {"dont_touch", "keep", "keep_hierarchy"};
  return names;
}

const std::vector<std::string>& getContentHashPropertyNames() {
  // Primitive definitions (including modeling) are deliberately excluded.
  static const std::vector<std::string> names {};
  return names;
}

ContentHash ContentHasher::hash(const SNLTerm* term) {
  if (auto bit = dynamic_cast<const SNLBusTermBit*>(term)) { return hashBitTerm(bit); }
  Encoder out(Term);
  out.u64(term->getID()); out.string(term->getName().getString()); out.byte(code(term->getDirection()));
  out.u64(term->getWidth());
  auto bus = dynamic_cast<const SNLBusTerm*>(term);
  auto bundle = dynamic_cast<const SNLBundleTerm*>(term);
  out.byte(bus ? 1 : bundle ? 2 : 0);
  if (bus) { out.u64(static_cast<uint64_t>(bus->getMSB())); out.u64(static_cast<uint64_t>(bus->getLSB())); }
  auto bits = bitTerms(term->getBits());
  out.sequence(Bits, bits.size());
  for (auto bit : bits) { out.digest(hashBitTerm(bit)); }
  if (bundle) {
    // Bundle member order is part of its interface, unlike container order.
    out.sequence(Members, bundle->getNumMembers());
    for (auto member : bundle->getMembers()) { out.digest(hash(member)); }
  }
  out.metadata(term);
  return out.finish();
}

ContentHash ContentHasher::hash(const SNLNet* net) {
  if (auto bit = dynamic_cast<const SNLBusNetBit*>(net)) { return hashBitNet(bit); }
  Encoder out(Net);
  out.u64(net->getID()); out.string(net->getName().getString()); out.u64(net->getWidth());
  auto bus = dynamic_cast<const SNLBusNet*>(net);
  out.byte(bus != nullptr);
  if (bus) { out.u64(static_cast<uint64_t>(bus->getMSB())); out.u64(static_cast<uint64_t>(bus->getLSB())); }
  auto bits = byID(net->getBits());
  std::sort(bits.begin(), bits.end(), [](auto a, auto b) {
    auto bitA = dynamic_cast<const SNLBusNetBit*>(a);
    auto bitB = dynamic_cast<const SNLBusNetBit*>(b);
    return (bitA ? bitA->getBit() : 0) < (bitB ? bitB->getBit() : 0);
  });
  out.sequence(Bits, bits.size());
  for (auto bit : bits) { out.digest(hashBitNet(bit)); }
  out.metadata(net);
  return out.finish();
}

const std::vector<SNLBitTerm*>& ContentHasher::orderedBitTerms(const SNLDesign* design) {
  auto it = bitTerms_.find(design);
  if (it != bitTerms_.end()) { return it->second; }
  return bitTerms_.emplace(design, bitTerms(design->getBitTerms())).first->second;
}

ContentHash ContentHasher::hash(const SNLInstance* instance) {
  Encoder out(Instance);
  out.u64(instance->getID()); out.string(instance->getName().getString());
  out.reference(instance->getModel()->getReference()); out.digest(hash(instance->getModel()));
  const auto parameters = byName(instance->getInstParameters());
  out.sequence(Parameters, parameters.size());
  for (auto parameter : parameters) { out.string(parameter->getName().getString()); out.string(parameter->getValue()); }
  const auto& terms = orderedBitTerms(instance->getModel());
  out.sequence(Connections, terms.size());
  for (auto term : terms) {
    out.u64(term->getID()); out.u64(static_cast<uint64_t>(term->getBit()));
    auto instTerm = instance->getInstTerm(term);
    out.connection(instTerm->getNet());
    out.metadata(instTerm);
  }
  out.metadata(instance);
  return out.finish();
}

ContentHash ContentHasher::hash(const SNLDesign* design) {
  if (auto it = designs_.find(design); it != designs_.end()) { return it->second; }
  if (!active_.insert(design).second) { throw NLException("Content hash requires an acyclic design hierarchy"); }
  try {
    Encoder out(design->isPrimitive() ? Primitive : Design);
    out.reference(design->getReference()); out.string(design->getName().getString()); out.byte(code(design->getType()));
    if (!design->isPrimitive()) {
      auto parameters = byName(design->getParameters());
      out.sequence(Parameters, parameters.size());
      for (auto parameter : parameters) {
        out.string(parameter->getName().getString()); out.byte(code(parameter->getType()));
        out.byte(parameter->hasDefaultValue());
        if (parameter->hasDefaultValue()) { out.string(parameter->getValue()); }
      }
      const auto terms = byID(design->getTerms());
      out.sequence(Terms, terms.size());
      for (auto term : terms) { out.digest(hash(term)); }
      const auto nets = byID(design->getNets());
      out.sequence(Nets, nets.size());
      for (auto net : nets) { out.digest(hash(net)); }
      const auto instances = byID(design->getInstances());
      out.sequence(Instances, instances.size());
      for (auto instance : instances) { out.digest(hash(instance)); }
      const auto& bits = orderedBitTerms(design);
      out.sequence(Connections, bits.size());
      for (auto bit : bits) {
        out.u64(bit->getID()); out.u64(static_cast<uint64_t>(bit->getBit())); out.connection(bit->getNet());
      }
      out.metadata(design);
    }
    auto result = out.finish();
    designs_.emplace(design, result); active_.erase(design);
    return result;
  } catch (...) { active_.erase(design); throw; }
}

ContentHash ContentHasher::hash(const NLLibrary* library) {
  Encoder out(Library);
  out.u64(library->getID()); out.string(library->getName().getString()); out.byte(code(library->getType()));
  // An explicitly requested primitive-library hash describes its identity only.
  if (!excluded(library)) {
    if (!library->getPNLDesigns().empty()) { throw NLException("Content hash supports SNL designs only"); }
    auto designs = byID(library->getSNLDesigns());
    std::erase_if(designs, [](auto design) { return design->isPrimitive(); });
    out.sequence(Designs, designs.size());
    for (auto design : designs) { out.digest(hash(design)); }
    auto libraries = byID(library->getLibraries());
    std::erase_if(libraries, [](auto lib) { return excluded(lib); });
    out.sequence(Libraries, libraries.size());
    for (auto lib : libraries) { out.digest(hash(lib)); }
    out.metadata(library);
  }
  return out.finish();
}

ContentHash ContentHasher::hash(const NLDB* db) {
  Encoder out(DB);
  out.u64(db->getID());
  auto libraries = byID(db->getLibraries());
  std::erase_if(libraries, [](auto lib) { return excluded(lib); });
  out.sequence(Libraries, libraries.size());
  for (auto library : libraries) { out.digest(hash(library)); }
  out.metadata(db);
  return out.finish();
}

ContentHash ContentHasher::hash(const NLUniverse* universe) {
  Encoder out(Universe);
  auto top = universe->getTopDesign();
  out.byte(top != nullptr);
  if (top) { out.reference(top->getReference()); }
  auto dbs = byID(universe->getUserDBs());
  std::erase_if(dbs, [](auto db) {
    const auto libraries = db->getLibraries();
    return !libraries.empty() && std::all_of(libraries.begin(), libraries.end(),
      [](auto library) { return excluded(library); });
  });
  out.sequence(DBs, dbs.size());
  for (auto db : dbs) { out.digest(hash(db)); }
  out.metadata(universe);
  return out.finish();
}

ContentHash NLUniverse::getContentHash() const { return ContentHasher().hash(this); }
ContentHash NLDB::getContentHash() const { return ContentHasher().hash(this); }
ContentHash NLLibrary::getContentHash() const { return ContentHasher().hash(this); }
ContentHash SNLDesign::getContentHash() const { return ContentHasher().hash(this); }
ContentHash SNLInstance::getContentHash() const { return ContentHasher().hash(this); }
ContentHash SNLTerm::getContentHash() const { return ContentHasher().hash(this); }
ContentHash SNLNet::getContentHash() const { return ContentHasher().hash(this); }

} // namespace naja::NL
