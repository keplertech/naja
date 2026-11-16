// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBundleTerm.h"
#include "SNLUtils.h"
#include "NLException.h"
using namespace naja::NL;

namespace {
void checkTermOrder(SNLDesign* design, const std::vector<SNLBitTerm*>& expected) {
  EXPECT_EQ(expected, std::vector<SNLBitTerm*>(
    design->getBitTerms().begin(), design->getBitTerms().end()));
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(i, expected[i]->getOrderID());
  }
}
}

class SNLDesignTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB*  db_;
};

TEST_F(SNLDesignTest, testTermOrderIDsAfterAppendAndInsertion) {
  auto design = SNLDesign::create(db_->getLibrary(NLName("MYLIB")), NLName("design"));
  auto first = SNLBusTerm::create(design, 10, SNLTerm::Direction::Input, 4, 2);
  first->getBit(3)->destroy();
  auto scalar = SNLScalarTerm::create(design, SNLTerm::Direction::Input);
  auto last = SNLBusTerm::create(design, SNLTerm::Direction::Output, -2, 0);
  checkTermOrder(design, {
    first->getBit(4), first->getBit(2), scalar,
    last->getBit(-2), last->getBit(-1), last->getBit(0)});

  auto inserted = SNLScalarTerm::create(design, 5, SNLTerm::Direction::Input);
  checkTermOrder(design, {
    inserted, first->getBit(4), first->getBit(2), scalar,
    last->getBit(-2), last->getBit(-1), last->getBit(0)});

  last->getBit(0)->destroy();
  auto appended = SNLScalarTerm::create(design, SNLTerm::Direction::Output);
  checkTermOrder(design, {
    inserted, first->getBit(4), first->getBit(2), scalar,
    last->getBit(-2), last->getBit(-1), appended});

  appended->destroy();
  last->getBit(-1)->destroy();
  last->getBit(-2)->destroy();
  auto afterEmpty = SNLScalarTerm::create(design, SNLTerm::Direction::Output);
  checkTermOrder(design, {inserted, first->getBit(4), first->getBit(2), scalar, afterEmpty});
}

TEST_F(SNLDesignTest, testTermOrderIDsWithBundleMembers) {
  auto library = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto design = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("design"));
  auto bundle = SNLBundleTerm::create(design, SNLTerm::Direction::Input);
  auto member = SNLBusTerm::create(bundle, SNLTerm::Direction::Input, 1, 0);
  checkTermOrder(design, {member->getBit(1), member->getBit(0)});

  auto addedMember = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input);
  checkTermOrder(design, {member->getBit(1), member->getBit(0), addedMember});
  auto addedBus = SNLBusTerm::create(bundle, SNLTerm::Direction::Input, -1, 0);
  auto scalar = SNLScalarTerm::create(design, SNLTerm::Direction::Output);
  auto afterMember = SNLScalarTerm::create(design, SNLTerm::Direction::Output);
  checkTermOrder(design, {
    member->getBit(1), member->getBit(0), addedMember, addedBus->getBit(-1), addedBus->getBit(0),
    scalar, afterMember});

  auto empty = SNLBundleTerm::create(design, SNLTerm::Direction::Output);
  auto afterEmpty = SNLScalarTerm::create(design, SNLTerm::Direction::Output);
  checkTermOrder(design, {
    member->getBit(1), member->getBit(0), addedMember, addedBus->getBit(-1), addedBus->getBit(0),
    scalar, afterMember, afterEmpty});
  empty->destroy();
  checkTermOrder(design, {
    member->getBit(1), member->getBit(0), addedMember, addedBus->getBit(-1), addedBus->getBit(0),
    scalar, afterMember, afterEmpty});
}

TEST_F(SNLDesignTest, testCreation0) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(NLName("design"), design->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_FALSE(design->isUnnamed());
  EXPECT_EQ(design, library->getSNLDesign(0));
  EXPECT_EQ(design, library->getSNLDesign(NLName("design")));
  EXPECT_EQ(library, design->getLibrary());
  EXPECT_EQ(1, library->getSNLDesigns().size());
  EXPECT_FALSE(library->getSNLDesigns().empty());
  EXPECT_EQ(db_, design->getDB());
  EXPECT_EQ(NLID(1, 0, 0), design->getNLID());
  EXPECT_EQ(design, NLUniverse::get()->getSNLDesign(NLID::DesignReference(1, 0, 0)));
  EXPECT_EQ(design, NLUniverse::get()->getObject(NLID(1, 0, 0)));
  EXPECT_TRUE(design->isStandard());
  EXPECT_TRUE(design->getTerms().empty());
  EXPECT_TRUE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_TRUE(design->getBitTerms().empty());
  auto designReference = NLID::DesignReference(1, 0, 0);
  EXPECT_EQ(designReference, design->getReference());
  EXPECT_EQ(design, NLUniverse::get()->getSNLDesign(designReference));
  designReference = NLID::DesignReference(1, 1, 0);
  EXPECT_EQ(nullptr, NLUniverse::get()->getSNLDesign(designReference)); 
  EXPECT_NE(designReference, design->getReference());
  designReference = NLID::DesignReference(1, 0, 1);
  EXPECT_EQ(nullptr, NLUniverse::get()->getSNLDesign(designReference)); 
  EXPECT_NE(designReference, design->getReference());

  SNLScalarTerm* term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term0"));
  ASSERT_NE(term0, nullptr);
  EXPECT_EQ(NLName("term0"), term0->getName());
  ASSERT_FALSE(term0->isUnnamed());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 0, 0, 0), term0->getNLID());
  EXPECT_EQ(design, term0->getDesign());
  EXPECT_EQ(term0, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 0)));
  EXPECT_EQ(term0, NLUniverse::get()->getObject(term0->getNLID()));
  EXPECT_EQ(SNLTerm::Direction::Input, term0->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBitTerms().size());

  SNLScalarTerm* term1 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("term1"));
  ASSERT_NE(term1, nullptr);
  EXPECT_EQ(NLName("term1"), term1->getName());
  ASSERT_FALSE(term1->isUnnamed());
  EXPECT_EQ(1, term1->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 1, 0, 0), term1->getNLID());
  EXPECT_LT(term0->getNLID(), term1->getNLID());
  EXPECT_EQ(design, term1->getDesign());
  EXPECT_EQ(term1, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 1)));
  EXPECT_EQ(term1, NLUniverse::get()->getObject(term1->getNLID()));
  EXPECT_EQ(SNLTerm::Direction::Output, term1->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_EQ(2, design->getBitTerms().size());
  EXPECT_EQ(term1, design->getScalarTerm(NLName("term1")));
  EXPECT_EQ(term1, design->getScalarTerm(1));
  EXPECT_EQ(term1, design->getBitTerm(1, 0));
  EXPECT_EQ(term1, design->getBitTerm(1, 11)); //bit irrelevant
  EXPECT_EQ(nullptr, design->getScalarTerm(NLName("term2")));
  EXPECT_EQ(nullptr, design->getBitTerm(2, -100)); //bit irrelevant

  //anonymous scalar term
  SNLScalarTerm* term2 = SNLScalarTerm::create(design, SNLTerm::Direction::InOut);
  ASSERT_NE(term2, nullptr);
  EXPECT_TRUE(term2->getName().empty());
  ASSERT_TRUE(term2->isUnnamed());
  EXPECT_EQ(2, term2->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 2, 0, 0), term2->getNLID());
  EXPECT_LT(term1->getNLID(), term2->getNLID());
  EXPECT_EQ(term2, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 2)));
  EXPECT_EQ(term2, NLUniverse::get()->getObject(term2->getNLID()));
  EXPECT_EQ(SNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_EQ(design, term2->getDesign());
  EXPECT_EQ(SNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBitTerms().size());
  EXPECT_EQ(term2, design->getScalarTerm(2));

  SNLBusTerm* term3 = SNLBusTerm::create(design, SNLTerm::Direction::Input, 4, 0, NLName("term3"));
  ASSERT_NE(term3, nullptr);
  EXPECT_EQ(NLName("term3"), term3->getName());
  ASSERT_FALSE(term3->isUnnamed());
  EXPECT_EQ(3, term3->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 3, 0, 0), term3->getNLID());
  EXPECT_EQ(term3, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 3)));
  EXPECT_EQ(term3, NLUniverse::get()->getObject(term3->getNLID()));
  EXPECT_EQ(SNLTerm::Direction::Input, term3->getDirection());
  EXPECT_EQ(design, term3->getDesign()); 
  EXPECT_EQ(4, term3->getMSB());
  EXPECT_EQ(0, term3->getLSB());
  EXPECT_EQ(5, term3->getWidth());
  EXPECT_FALSE(term3->getBits().empty());
  EXPECT_EQ(5, term3->getBits().size());
  for (auto bit: term3->getBusBits()) {
    EXPECT_EQ(SNLTerm::Direction::Input, bit->getDirection());
    EXPECT_EQ(term3, bit->getBus());
  }
  EXPECT_EQ(term3, design->getBusTerm(NLName("term3")));
  EXPECT_EQ(term3, design->getBusTerm(3));
  EXPECT_EQ(nullptr, design->getBusTerm(NLName("term4")));
  EXPECT_EQ(nullptr, design->getBusTerm(4));

  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_FALSE(design->getBusTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBusTerms().size());
  EXPECT_EQ(1+1+1+5, design->getBitTerms().size());

  EXPECT_THAT(std::vector(design->getTerms().begin(), design->getTerms().end()),
    ElementsAre(term0, term1, term2, term3));
  auto scalarTermsBegin = design->getScalarTerms().begin();
  auto scalarTermsEnd = design->getScalarTerms().end();
  EXPECT_THAT(std::vector(scalarTermsBegin, scalarTermsEnd), ElementsAre(term0, term1, term2));
  EXPECT_THAT(std::vector(design->getScalarTerms().begin(), design->getScalarTerms().end()),
    ElementsAre(term0, term1, term2));
  auto busTermsBegin = design->getBusTerms().begin();
  auto busTermsEnd = design->getBusTerms().end();
  EXPECT_THAT(std::vector(busTermsBegin, busTermsEnd), ElementsAre(term3));
  EXPECT_THAT(std::vector(design->getBitTerms().begin(), design->getBitTerms().end()),
    ElementsAre(term0, term1, term2,
      term3->getBit(4), term3->getBit(3), term3->getBit(2), term3->getBit(1), term3->getBit(0)));
  auto bitTermsBegin = design->getBitTerms().begin();
  auto bitTermsEnd = design->getBitTerms().end();
  EXPECT_THAT(std::vector(bitTermsBegin, bitTermsEnd),
    ElementsAre(term0, term1, term2,
      term3->getBit(4), term3->getBit(3), term3->getBit(2), term3->getBit(1), term3->getBit(0)));

  SNLDesign* model = SNLDesign::create(library, NLName("model"));
  ASSERT_NE(model, nullptr);
  EXPECT_EQ("model", model->getName().getString());
  EXPECT_EQ("model", model->getString());
  EXPECT_EQ(1, model->getID());
  EXPECT_EQ(NLID(1, 0, 1), model->getNLID());
  EXPECT_LT(design->getNLID(), model->getNLID());
  EXPECT_EQ(model, NLUniverse::get()->getSNLDesign(NLID::DesignReference(1, 0, 1)));
  EXPECT_EQ(model, NLUniverse::get()->getObject(NLID(1, 0, 1)));
  EXPECT_FALSE(model->isUnnamed());
  EXPECT_EQ(model, library->getSNLDesign(1));
  EXPECT_EQ(model, library->getSNLDesign(NLName("model")));
  EXPECT_EQ(2, library->getSNLDesigns().size());
  EXPECT_FALSE(library->getSNLDesigns().empty());
  EXPECT_THAT(std::vector(library->getSNLDesigns().begin(), library->getSNLDesigns().end()),
    ElementsAre(design, model));

  //anonymous design
  SNLDesign* anon = SNLDesign::create(library);
  ASSERT_NE(anon, nullptr);
  EXPECT_EQ(2, anon->getID());
  EXPECT_EQ(NLID(1, 0, 2), anon->getNLID());
  EXPECT_EQ(anon, NLUniverse::get()->getSNLDesign(NLID::DesignReference(1, 0, 2)));
  EXPECT_EQ(anon, NLUniverse::get()->getObject(NLID(1, 0, 2)));
  EXPECT_TRUE(anon->isUnnamed());
  EXPECT_EQ("<design:2>", anon->getString());
  EXPECT_EQ(anon, library->getSNLDesign(2));
  EXPECT_EQ(library, anon->getLibrary());
  EXPECT_EQ(db_, anon->getDB());
  EXPECT_EQ(NLID(1, 0, 2), anon->getNLID());
  EXPECT_TRUE(anon->isStandard());
  EXPECT_EQ(3, library->getSNLDesigns().size());
  EXPECT_FALSE(library->getSNLDesigns().empty());
  EXPECT_THAT(std::vector(library->getSNLDesigns().begin(), library->getSNLDesigns().end()),
    ElementsAre(design, model, anon));
}

TEST_F(SNLDesignTest, testCreation1) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());

  //anonymous design
  SNLDesign* anon = SNLDesign::create(library);
  ASSERT_NE(anon, nullptr);
  EXPECT_EQ(0, anon->getID());
  EXPECT_TRUE(anon->isUnnamed());
  EXPECT_EQ("<design:0>", anon->getString());
  EXPECT_EQ(anon, library->getSNLDesign(0));
  EXPECT_EQ(library, anon->getLibrary());
  EXPECT_EQ(db_, anon->getDB());
  EXPECT_EQ(NLID(1, 0, 0), anon->getNLID());
  EXPECT_TRUE(anon->isStandard());
  EXPECT_EQ(1, library->getSNLDesigns().size());
  EXPECT_FALSE(library->getSNLDesigns().empty());
  EXPECT_THAT(std::vector(library->getSNLDesigns().begin(), library->getSNLDesigns().end()),
    ElementsAre(anon));
}

TEST_F(SNLDesignTest, testPrimitives) {
  EXPECT_THROW(
    SNLDesign::create(nullptr, NLName("ERROR")),
    NLException);

  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);

  EXPECT_THROW(
    SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("ERROR")),
    NLException);

  auto design = SNLDesign::create(library);
  EXPECT_EQ(NLID::DesignID(0), design->getID());
  EXPECT_THROW(
    SNLDesign::create(library, NLID::DesignID(0), SNLDesign::Type::Standard),
    NLException);
  EXPECT_THROW(design->setType(SNLDesign::Type::Primitive), NLException);

  auto prims = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("Primitives"));
  EXPECT_TRUE(prims->isPrimitives());
  auto prim1 = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("Primitive"));
  EXPECT_NE(nullptr, prim1);
  EXPECT_FALSE(prim1->isStandard());
  EXPECT_FALSE(prim1->isBlackBox());
  EXPECT_TRUE(prim1->isPrimitive());
  EXPECT_TRUE(prim1->isLeaf());
  EXPECT_EQ(nullptr, SNLUtils::findTop(prims));

  EXPECT_THROW(
    SNLDesign::create(library, SNLDesign::Type::Primitive),
    NLException);
  EXPECT_THROW(
    SNLDesign::create(prims),
    NLException);
  EXPECT_THROW(
    SNLDesign::create(prims, NLName("NonPrimitive")),
    NLException);
}

TEST_F(SNLDesignTest, testSetTop) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design0 = SNLDesign::create(library, NLName("design0"));
  ASSERT_NE(design0, nullptr);
  EXPECT_EQ(design0, SNLUtils::findTop(library));
  SNLDesign* design1 = SNLDesign::create(library, NLName("design1"));
  ASSERT_NE(design1, nullptr);
  EXPECT_EQ(nullptr, SNLUtils::findTop(library));

  EXPECT_EQ(nullptr, NLUniverse::get()->getTopDesign());
  EXPECT_EQ(nullptr, NLUniverse::get()->getTopDB());
  EXPECT_FALSE(design0->isTopDesign());
  EXPECT_FALSE(design1->isTopDesign());
  design0->getDB()->setTopDesign(design0);
  EXPECT_TRUE(design0->isTopDesign());
  EXPECT_FALSE(design1->isTopDesign());
  EXPECT_EQ(design0, design0->getDB()->getTopDesign());
  EXPECT_FALSE(NLUniverse::get()->getTopDesign());
  EXPECT_FALSE(NLUniverse::get()->getTopDB());
  NLUniverse::get()->setTopDB(design0->getDB());
  EXPECT_TRUE(design0->getDB()->isTopDB());
  EXPECT_EQ(NLUniverse::get()->getTopDB(), design0->getDB());
  EXPECT_EQ(NLUniverse::get()->getTopDesign(), design0);
  NLUniverse::get()->setTopDesign(design1);
  EXPECT_FALSE(design0->isTopDesign());
  EXPECT_TRUE(design1->isTopDesign());
  EXPECT_TRUE(design0->getDB()->isTopDB());

  auto altDB = NLDB::create(NLUniverse::get());
  auto altLibrary = NLLibrary::create(altDB);
  auto altDesign = SNLDesign::create(altLibrary);
  EXPECT_THROW(design0->getDB()->setTopDesign(altDesign), NLException);
}

TEST_F(SNLDesignTest, testSetName) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design0 = SNLDesign::create(library, NLName("design0"));
  EXPECT_EQ(NLName("design0"), design0->getName());
  design0->setName(NLName("design0"));
  EXPECT_EQ(NLName("design0"), design0->getName());
  EXPECT_EQ(design0, library->getSNLDesign(NLName("design0")));

  design0->setName(NLName("design1"));
  EXPECT_EQ(NLName("design1"), design0->getName());
  EXPECT_EQ(design0, library->getSNLDesign(NLName("design1")));
  EXPECT_EQ(nullptr, library->getSNLDesign(NLName("design0")));
  design0->setName(NLName("design0"));
  EXPECT_EQ(NLName("design0"), design0->getName());
  EXPECT_EQ(design0, library->getSNLDesign(NLName("design0")));
  EXPECT_EQ(nullptr, library->getSNLDesign(NLName("design1")));

  auto design1 = SNLDesign::create(library, NLName("design1"));
  EXPECT_EQ(NLName("design1"), design1->getName());
  EXPECT_EQ(design1, library->getSNLDesign(NLName("design1")));
  EXPECT_THROW(design1->setName(NLName("design0")), NLException);
}
