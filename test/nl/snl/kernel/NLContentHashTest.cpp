// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <functional>
#include <set>
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBundleTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLParameter.h"
#include "SNLInstParameter.h"
#include "SNLAttributes.h"
#include "SNLRTLInfos.h"
#include "SNLDesignModeling.h"
#include "NajaDumpableProperty.h"

using namespace naja::NL;

namespace {
class Annotation: public naja::NajaPrivateProperty {
  public:
    static void create(NLObject* object) {
      preCreate(object, "private_annotation");
      (new Annotation())->postCreate(object);
    }
    std::string getName() const override { return "private_annotation"; }
    std::string getString() const override { return getName(); }
};
} // namespace

class NLContentHashTest: public ::testing::Test {
  protected:
    void SetUp() override {
      universe = NLUniverse::create();
      db = NLDB::create(universe);
      library = NLLibrary::create(db, NLName("logic"));
      top = SNLDesign::create(library, NLName("top"));
      primitives = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
      model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("cell"));
      input = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
      term = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("a"));
      net = SNLScalarNet::create(top, NLName("wire"));
      term->setNet(net);
      instance = SNLInstance::create(top, model, NLName("u"));
      instance->getInstTerm(input)->setNet(net);
      SNLAttributes::addAttribute(top, SNLAttribute(NLName("keep"), SNLAttributeValue("yes")));
      SNLDesignModeling::setTruthTable(model, SNLTruthTable(1, 2, SNLTruthTable::fullDependencies(1)));
      universe->setTopDesign(top);
    }
    void TearDown() override { NLUniverse::get()->destroy(); }
    void changes(const std::function<void()>& edit) {
      auto before = universe->getContentHash();
      auto designBefore = top->getContentHash();
      edit();
      EXPECT_NE(before, universe->getContentHash());
      EXPECT_NE(designBefore, top->getContentHash());
    }
    NLUniverse* universe;
    NLDB* db;
    NLLibrary* library;
    NLLibrary* primitives;
    SNLDesign* top;
    SNLDesign* model;
    SNLScalarTerm* input;
    SNLScalarTerm* term;
    SNLScalarNet* net;
    SNLInstance* instance;
};

TEST_F(NLContentHashTest, GoldenV1) {
  EXPECT_EQ(1u, kContentHashVersion);
  EXPECT_EQ("464123ab0366d80817af5cec91e2d71b6b7bf4cd27a7cf44a9f716246c51df19", toHex(universe->getContentHash()));
  EXPECT_EQ(top->getContentHash(), ContentHasher().hash(top));
  EXPECT_EQ(64u, toHex(top->getContentHash()).size());
  EXPECT_EQ((std::vector<std::string>{"dont_touch", "keep", "keep_hierarchy"}), getContentHashAttributeNames());
  EXPECT_TRUE(getContentHashPropertyNames().empty());
}

TEST_F(NLContentHashTest, Sensitivity) {
  changes([&] { instance->setName(NLName("v")); });
  changes([&] { term->setName(NLName("b")); });
  changes([&] { net->setName(NLName("other")); });
  changes([&] { term->setDirection(SNLTerm::Direction::Output); });
  changes([&] { instance->getInstTerm(input)->setNet(nullptr); });
  changes([&] { instance->getInstTerm(input)->setNet(net); });
  changes([&] { term->setNet(nullptr); });
  changes([&] { net->setType(SNLNet::Type::Assign0); });
  auto other = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("other_cell"));
  SNLScalarTerm::create(other, SNLTerm::Direction::Input, NLName("i"));
  SNLScalarTerm::create(other, SNLTerm::Direction::Output, NLName("o"));
  changes([&] { instance->setModel(other); });
  auto parameter = SNLParameter::create(other, NLName("MODE"), SNLParameter::Type::Decimal, "0");
  changes([&] { SNLInstParameter::create(instance, parameter, "1"); });
  changes([&] { instance->getInstParameter(NLName("MODE"))->setValue("2"); });
  changes([&] { SNLParameter::create(top, NLName("SIZE"), SNLParameter::Type::Decimal); });
  auto bus = SNLBusTerm::create(top, SNLTerm::Direction::Input, 3, -1, NLName("bus"));
  changes([&] { bus->setMSB(2); });
}

TEST_F(NLContentHashTest, DirectionAndNetTypeCodes) {
  std::set<ContentHash> hashes;
  for (auto direction : {SNLTerm::Direction::Input, SNLTerm::Direction::Output,
      SNLTerm::Direction::InOut, SNLTerm::Direction::Undefined}) {
    term->setDirection(direction);
    EXPECT_TRUE(hashes.insert(term->getContentHash()).second);
  }
  hashes.clear();
  for (auto type : {SNLNet::Type::Standard, SNLNet::Type::Assign0,
      SNLNet::Type::Assign1, SNLNet::Type::Supply0, SNLNet::Type::Supply1,
      SNLNet::Type::AssignX, SNLNet::Type::AssignZ}) {
    net->setType(type);
    EXPECT_TRUE(hashes.insert(net->getContentHash()).second);
  }
}

TEST_F(NLContentHashTest, DesignAndParameterTypeCodes) {
  std::set<ContentHash> hashes;
  for (auto type : {SNLDesign::Type::Standard, SNLDesign::Type::UserBlackBox,
      SNLDesign::Type::AutoBlackBox}) {
    // Keep identity and contents fixed so only the design type differs.
    auto design = SNLDesign::create(library, 10, type, NLName("typed"));
    EXPECT_TRUE(hashes.insert(design->getContentHash()).second);
    design->destroy();
  }
  hashes.clear();
  for (auto type : {SNLParameter::Type::Decimal, SNLParameter::Type::Binary,
      SNLParameter::Type::Boolean, SNLParameter::Type::String}) {
    auto parameter = SNLParameter::create(top, NLName("VALUE"), type, "1");
    EXPECT_TRUE(hashes.insert(top->getContentHash()).second);
    parameter->destroy();
  }
}

TEST_F(NLContentHashTest, ExplicitExcludedLibraryHashes) {
  auto primitiveBefore = primitives->getContentHash();
  auto db0Library = NLLibrary::create(NLUniverse::getDB0(), NLLibrary::Type::InDB0, NLName("internal"));
  ASSERT_EQ(NLLibrary::Type::InDB0, db0Library->getType());
  auto db0Before = db0Library->getContentHash();
  EXPECT_EQ(primitiveBefore, ContentHasher().hash(primitives));
  EXPECT_EQ(db0Before, ContentHasher().hash(db0Library));
  EXPECT_NE(primitiveBefore, db0Before);
  SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("extra"));
  SNLAttributes::addAttribute(model, SNLAttribute(NLName("keep")));
  EXPECT_EQ(primitiveBefore, primitives->getContentHash());
  EXPECT_EQ(db0Before, db0Library->getContentHash());
}

TEST_F(NLContentHashTest, AttributeRegistry) {
  std::vector<NLObject*> objects {top, instance, term, net, instance->getInstTerm(input)};
  for (auto object : objects) {
    for (const auto& name : getContentHashAttributeNames()) {
      SNLAttributes::clearAttributes(object);
      auto add = [&](const std::string& value) {
        const auto attribute = SNLAttribute(NLName(name), SNLAttributeValue(value));
        if (auto design = dynamic_cast<SNLDesign*>(object)) SNLAttributes::addAttribute(design, attribute);
        else SNLAttributes::addAttribute(static_cast<SNLDesignObject*>(object), attribute);
      };
      auto before = universe->getContentHash();
      changes([&] { add("a"); });
      changes([&] { SNLAttributes::clearAttributes(object); add("b"); });
      changes([&] { SNLAttributes::clearAttributes(object); });
      EXPECT_EQ(before, universe->getContentHash());
      add("a"); add("b");
      auto ordered = universe->getContentHash();
      SNLAttributes::clearAttributes(object); add("b"); add("a");
      EXPECT_EQ(ordered, universe->getContentHash());
      changes([&] { add("a"); }); // duplicate multiplicity is content
    }
  }
}

TEST_F(NLContentHashTest, ExclusionsAndPrimitiveDefinitions) {
  auto before = universe->getContentHash();
  Annotation::create(top);
  auto primitiveBefore = model->getContentHash();
  SNLAttributes::addAttribute(top, SNLAttribute(NLName("KEEP"), SNLAttributeValue("1")));
  SNLAttributes::addAttribute(net, SNLAttribute(NLName("LOC"), SNLAttributeValue("site")));
  auto property = naja::NajaDumpableProperty::create(top, "annotation");
  property->addStringValue("note"); property->addUInt64Value(7);
  SNLRTLInfos::create(top)->setSourceLoc({NLName("source.v"), 1, 2, 3, 4});
  SNLDesignModeling::addCombinatorialArcs({input}, {model->getScalarTerm(NLName("o"))});
  auto truth = model->getProperty("SNLDesignTruthTableProperty");
  truth->destroy();
  SNLDesignModeling::setTruthTable(model, SNLTruthTable(1, 1, SNLTruthTable::fullDependencies(1)));
  auto extra = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("extra"));
  SNLDesign::create(extra, SNLDesign::Type::Primitive, NLName("unused"));
  auto primitiveDB = NLDB::create(universe);
  auto primitiveLib = NLLibrary::create(primitiveDB, NLLibrary::Type::Primitives, NLName("separate"));
  SNLDesign::create(primitiveLib, SNLDesign::Type::Primitive, NLName("unused"));
  EXPECT_EQ(primitiveBefore, model->getContentHash());
  EXPECT_EQ(before, universe->getContentHash());
}

TEST_F(NLContentHashTest, MerkleLocalityAndMemoScope) {
  auto b = SNLDesign::create(library, NLName("b"));
  auto c = SNLDesign::create(library, NLName("c"));
  auto a = SNLDesign::create(library, NLName("a"));
  SNLInstance::create(a, b, NLName("b"));
  SNLInstance::create(top, a, NLName("a"));
  SNLInstance::create(top, c, NLName("c"));
  auto hashA = a->getContentHash(), hashB = b->getContentHash(), hashC = c->getContentHash();
  changes([&] { SNLScalarNet::create(b, NLName("new_net")); });
  EXPECT_NE(hashA, a->getContentHash()); EXPECT_NE(hashB, b->getContentHash());
  EXPECT_EQ(hashC, c->getContentHash());
  ContentHasher hasher;
  EXPECT_EQ(a->getContentHash(), hasher.hash(a));
  EXPECT_EQ(top->getContentHash(), hasher.hash(top));
  auto cyclic = SNLInstance::create(b, a, NLName("cycle"));
  EXPECT_THROW(top->getContentHash(), NLException);
  cyclic->destroy();
  EXPECT_EQ(top->getContentHash(), ContentHasher().hash(top));
}

TEST_F(NLContentHashTest, AnonymousInstancesAndExplicitOrder) {
  auto first = SNLInstance::create(top, model, 20);
  auto second = SNLInstance::create(top, model, 10);
  EXPECT_NE(first->getContentHash(), second->getContentHash());
  auto before = universe->getContentHash();
  first->destroy(); second->destroy();
  SNLInstance::create(top, model, 10);
  SNLInstance::create(top, model, 20);
  EXPECT_EQ(before, universe->getContentHash());
  auto other = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("different"));
  // Same interface, different model identity.
  SNLScalarTerm::create(other, SNLTerm::Direction::Input, NLName("i"));
  SNLScalarTerm::create(other, SNLTerm::Direction::Output, NLName("o"));
  top->getInstance(10)->setModel(other);
  before = universe->getContentHash();
  top->getInstance(10)->setModel(model); top->getInstance(20)->setModel(other);
  EXPECT_NE(before, universe->getContentHash());
}

TEST_F(NLContentHashTest, BusBitsAndSubtrees) {
  auto busTerm = SNLBusTerm::create(top, SNLTerm::Direction::Output, -1, 2, NLName("port"));
  auto busNet = SNLBusNet::create(top, -1, 2, NLName("bus"));
  busTerm->setNet(busNet);
  auto termBefore = busTerm->getContentHash();
  changes([&] { busNet->getBit(0)->setType(SNLNet::Type::Supply1); });
  EXPECT_EQ(termBefore, busTerm->getContentHash()); // connectivity belongs to the design
  changes([&] { busTerm->getBit(1)->setNet(nullptr); });
  auto bitBefore = busTerm->getBit(1)->getContentHash();
  changes([&] { SNLAttributes::addAttribute(busTerm->getBit(1), SNLAttribute(NLName("keep"))); });
  EXPECT_NE(bitBefore, busTerm->getBit(1)->getContentHash());
  EXPECT_NE(busNet->getBit(0)->getContentHash(), busNet->getBit(1)->getContentHash());
  EXPECT_NE(busTerm->getBit(0)->getContentHash(), busTerm->getBit(1)->getContentHash());
  auto before = universe->getContentHash();
  NLLibrary::create(library, NLName("nested"));
  EXPECT_NE(before, universe->getContentHash());
}


TEST_F(NLContentHashTest, ExplicitIDsAndNameInterningOrder) {
  universe->destroy();
  const auto build = [](bool reverse) {
    auto u = NLUniverse::create();
    auto db = NLDB::create(u);
    auto library = NLLibrary::create(db, NLName("ordered"));
    SNLDesign* designs[2];
    for (unsigned n = 0; n < 2; ++n) {
      unsigned id = reverse ? 1 - n : n;
      designs[id] = SNLDesign::create(library, id, SNLDesign::Type::Standard, NLName(id ? "leaf" : "top"));
    }
    auto top = designs[0];
    for (unsigned n = 0; n < 2; ++n) {
      unsigned id = reverse ? 1 - n : n;
      SNLScalarTerm::create(top, id, SNLTerm::Direction::Input, NLName(id ? "b" : "a"));
      SNLScalarNet::create(top, id, NLName(id ? "n1" : "n0"));
      SNLInstance::create(top, designs[1], id, NLName(id ? "u1" : "u0"));
      SNLParameter::create(top, NLName(id ? "BETA" : "ALPHA"), SNLParameter::Type::String, "value");
      SNLAttributes::addAttribute(top, SNLAttribute(NLName(id ? "keep" : "dont_touch"), SNLAttributeValue("1")));
    }
    u->setTopDesign(top);
    auto hash = u->getContentHash();
    u->destroy();
    return hash;
  };
  auto first = build(false), second = build(true);
  EXPECT_EQ(first, second);
  universe = NLUniverse::create(); // preserve fixture teardown
}

TEST_F(NLContentHashTest, BundleMembersAndAttributeTypes) {
  auto bundled = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("bundle_cell"));
  auto bundle = SNLBundleTerm::create(bundled, SNLTerm::Direction::Input, NLName("record"));
  auto member = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("member"));
  auto before = bundle->getContentHash();
  member->setName(NLName("renamed"));
  EXPECT_NE(before, bundle->getContentHash());
  SNLAttributes::clearAttributes(top);
  SNLAttributes::addAttribute(top, SNLAttribute(NLName("keep"), SNLAttributeValue("1")));
  changes([&] {
    SNLAttributes::clearAttributes(top);
    SNLAttributes::addAttribute(top, SNLAttribute(NLName("keep"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "1")));
  });
  auto hash = universe->getContentHash();
  db->setTopDesign(nullptr);
  EXPECT_NE(hash, universe->getContentHash());
}

TEST_F(NLContentHashTest, Timing27000Instances) {
  // About 108k instance pins. Report rather than assert a machine-dependent time.
  auto cell = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("wide_cell"));
  SNLBusTerm::create(cell, SNLTerm::Direction::Input, 3, 0, NLName("pins"));
  auto wires = SNLBusNet::create(top, 3, 0, NLName("signals"));
  for (unsigned i = 0; i < 27000; ++i) {
    auto instance = SNLInstance::create(top, cell);
    for (auto pin : instance->getInstTerms()) {
      pin->setNet(wires->getBit(pin->getBitTerm()->getBit()));
    }
  }
  const auto start = std::chrono::steady_clock::now();
  const auto result = universe->getContentHash();
  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  RecordProperty("hash_seconds", std::to_string(elapsed));
  EXPECT_NE(ContentHash{}, result);
}
