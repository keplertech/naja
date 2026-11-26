// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"
#include "SNLDesign.h"
using namespace naja::NL;

class NLLibraryTest: public ::testing::Test {
  protected:
    void SetUp() override {
      universe_ = NLUniverse::create();
    }
    void TearDown() override {
      universe_->destroy();
    }
    NLUniverse*  universe_;
};

TEST_F(NLLibraryTest, test0) {
  NLDB* db = NLDB::create(universe_);
  EXPECT_EQ(0, db->getLibraries().size());
  EXPECT_TRUE(db->getLibraries().empty());

  NLLibrary* lib1 = db->getLibrary(NLName("LIB1"));
  EXPECT_FALSE(lib1);
  lib1 = NLLibrary::create(db, NLName("LIB1"));
  ASSERT_TRUE(lib1);
  EXPECT_EQ(0, lib1->getID());
  EXPECT_EQ(NLID(1, 0), lib1->getNLID());
  NLLibrary* testLib1 = db->getLibrary(NLName("LIB1"));
  ASSERT_TRUE(testLib1);
  EXPECT_EQ(db->getLibrary(0), lib1);
  EXPECT_EQ(lib1, NLUniverse::get()->getLibrary(1, 0));
  EXPECT_EQ(lib1, NLUniverse::get()->getObject(NLID(1, 0)));
  EXPECT_EQ(nullptr, db->getLibrary(1));
  EXPECT_EQ(nullptr, NLUniverse::get()->getLibrary(1, 1));
  EXPECT_EQ(nullptr, NLUniverse::get()->getObject(NLID(1, 1)));
  EXPECT_EQ(nullptr, NLUniverse::get()->getLibrary(5, 1));
  EXPECT_EQ(nullptr, NLUniverse::get()->getObject(NLID(5, 1)));
  EXPECT_EQ(nullptr, db->getLibrary(2));

  EXPECT_EQ(testLib1, lib1);
  EXPECT_EQ("LIB1", lib1->getName().getString());

  EXPECT_EQ(1, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_THAT(std::vector(db->getLibraries().begin(), db->getLibraries().end()), ElementsAre(lib1));
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getSNLDesigns().size());
  EXPECT_TRUE(lib1->getSNLDesigns().empty());
  EXPECT_EQ(nullptr, lib1->getLibrary(2));
  EXPECT_EQ(nullptr, lib1->getLibrary(NLName("UNKNOWN")));
  EXPECT_EQ(nullptr, lib1->getLibrary(3));

  NLLibrary* lib2 = NLLibrary::create(db, NLName("LIB2"));
  EXPECT_EQ(NLID(1, 1), lib2->getNLID());
  EXPECT_LT(lib1->getNLID(), lib2->getNLID());
  ASSERT_TRUE(lib2);
  EXPECT_EQ(1, lib2->getID());
  NLLibrary* testLib2 = db->getLibrary(NLName("LIB2"));
  ASSERT_TRUE(testLib2);
  EXPECT_EQ(testLib2, lib2);
  EXPECT_EQ("LIB2", lib2->getName().getString());
  EXPECT_EQ(lib2, NLUniverse::get()->getLibrary(1, 1));
  EXPECT_EQ(lib2, NLUniverse::get()->getObject(NLID(1, 1)));

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_THAT(std::vector(db->getLibraries().begin(), db->getLibraries().end()),
    ElementsAre(lib1, lib2));
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getSNLDesigns().size());
  EXPECT_TRUE(lib1->getSNLDesigns().empty());
  EXPECT_EQ(0, lib2->getLibraries().size());
  EXPECT_TRUE(lib2->getLibraries().empty());
  EXPECT_EQ(0, lib2->getSNLDesigns().size());
  EXPECT_TRUE(lib2->getSNLDesigns().empty());

  NLLibrary* lib3 = NLLibrary::create(lib2, NLName("LIB1"));
  ASSERT_TRUE(lib3);
  EXPECT_EQ(2, lib3->getID());
  EXPECT_LT(lib2->getNLID(), lib3->getNLID());
  NLLibrary* testLib3 = lib2->getLibrary(NLName("LIB1"));
  ASSERT_TRUE(testLib3);
  EXPECT_EQ(testLib3, lib3);
  EXPECT_EQ("LIB1", lib3->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(3, db->getGlobalLibraries().size());
  EXPECT_FALSE(db->getGlobalLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(1, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_THAT(std::vector(lib2->getLibraries().begin(), lib2->getLibraries().end()), ElementsAre(lib3));
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());
  EXPECT_EQ(0, lib3->getSNLDesigns().size());
  EXPECT_TRUE(lib3->getSNLDesigns().empty());

  NLLibrary* lib4 = NLLibrary::create(lib2, NLName("LIB2"));
  ASSERT_TRUE(lib4);
  NLLibrary* testLib4 = lib2->getLibrary(NLName("LIB2"));
  ASSERT_TRUE(testLib4);
  EXPECT_EQ(testLib4, lib4);
  EXPECT_EQ("LIB2", lib4->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(2, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_THAT(std::vector(lib2->getLibraries().begin(), lib2->getLibraries().end()), ElementsAre(lib3, lib4));
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());
  EXPECT_EQ(0, lib4->getLibraries().size());
  EXPECT_TRUE(lib4->getLibraries().empty());

  lib4->destroy();
  testLib4 = lib2->getLibrary(NLName("LIB2"));
  EXPECT_FALSE(testLib4);

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(1, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_THAT(std::vector(lib2->getLibraries().begin(), lib2->getLibraries().end()), ElementsAre(lib3));
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());

  lib2->destroy();
  testLib2 = db->getLibrary(NLName("LIB2"));
  EXPECT_FALSE(testLib2);
  EXPECT_THAT(std::vector(db->getLibraries().begin(), db->getLibraries().end()), ElementsAre(lib1));
  EXPECT_EQ(1, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getSNLDesigns().size());
  EXPECT_TRUE(lib1->getSNLDesigns().empty());
}

TEST_F(NLLibraryTest, test1) {
  NLDB* db = NLDB::create(universe_);
  ASSERT_TRUE(db);
  NLLibrary* root = NLLibrary::create(db, NLLibrary::Type::Primitives);
  ASSERT_TRUE(root);
  EXPECT_TRUE(root->isUnnamed());
  EXPECT_EQ(0, root->getID());
  EXPECT_EQ(nullptr, root->getParentLibrary());
  EXPECT_EQ(1, db->getPrimitiveLibraries().size());

  NLLibrary* primitives0 = NLLibrary::create(root, NLLibrary::Type::Primitives, NLName("Primitives0"));
  EXPECT_FALSE(primitives0->isUnnamed());
  EXPECT_EQ(NLName("Primitives0"), primitives0->getName());
  EXPECT_EQ(1, primitives0->getID());
  EXPECT_EQ(2, db->getPrimitiveLibraries().size());

  NLLibrary* primitives1 = NLLibrary::create(root, NLLibrary::Type::Primitives);
  EXPECT_TRUE(primitives1->isUnnamed());
  EXPECT_EQ(2, primitives1->getID());
  EXPECT_EQ(3, db->getPrimitiveLibraries().size());

  EXPECT_EQ(primitives0, root->getLibrary(NLName("Primitives0")));
  EXPECT_EQ(primitives0, root->getLibrary(1));
  EXPECT_EQ(primitives1, root->getLibrary(2));

  EXPECT_FALSE(root->getLibrary(3));
  EXPECT_FALSE(root->getLibrary(NLName("UNKNOWN")));

  EXPECT_EQ(primitives0, universe_->getLibrary(1, 1));
  EXPECT_EQ(primitives1, universe_->getLibrary(1, 2));
  EXPECT_EQ(nullptr, universe_->getLibrary(1, 5));
}

TEST_F(NLLibraryTest, testDesignSearch) {
  NLDB* db = NLDB::create(universe_);
  EXPECT_NE(nullptr, db);
  NLLibrary* root = NLLibrary::create(db);
  EXPECT_NE(nullptr, root);
  SNLDesign* design = SNLDesign::create(root, NLName("DESIGN"));
  EXPECT_NE(nullptr, design);
  EXPECT_EQ(design, root->getSNLDesign(NLName("DESIGN")));
  EXPECT_EQ(design, root->getSNLDesign(0));
  EXPECT_EQ(nullptr, root->getSNLDesign(NLName("UNKNOWN")));
  EXPECT_EQ(nullptr, root->getSNLDesign(1));
}

TEST_F(NLLibraryTest, testRename) {
  NLDB* db = NLDB::create(universe_);
  NLLibrary* root = NLLibrary::create(db);
  EXPECT_TRUE(root->isUnnamed());
  root->setName(NLName("ROOT"));
  EXPECT_FALSE(root->isUnnamed());
  EXPECT_EQ("ROOT", root->getName().getString());
}

TEST_F(NLLibraryTest, testCompare) {
  NLDB* db = NLDB::create(universe_);
  NLLibrary* root = NLLibrary::create(db);
  NLLibrary* lib1 = NLLibrary::create(root);
  NLLibrary* lib2 = NLLibrary::create(root);
  std::string reason;
  EXPECT_TRUE(lib1->deepCompare(lib1, reason));
  EXPECT_TRUE(reason.empty());
  EXPECT_FALSE(lib1->deepCompare(lib2, reason));
  EXPECT_FALSE(reason.empty());
}

TEST_F(NLLibraryTest, testErrors) {
  NLDB* db = NLDB::create(universe_);
  ASSERT_TRUE(db);
  NLLibrary* root = NLLibrary::create(db);
  ASSERT_TRUE(root);
  EXPECT_TRUE(root->isUnnamed());
  EXPECT_EQ(NLID::LibraryID(0), root->getID());

  NLDB* nullDB = nullptr;
  EXPECT_THROW(NLLibrary::create(nullDB), NLException);
  NLLibrary* nullLibrary = nullptr;
  EXPECT_THROW(NLLibrary::create(nullLibrary), NLException);

  NLLibrary* prims = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  //library and parent library must share same type
  EXPECT_THROW(NLLibrary::create(prims, NLName("SUB_PRIMS")), NLException);
  EXPECT_EQ(NLID::LibraryID(1), prims->getID());

  //no effect
  prims->setName(NLName("PRIMS"));
  EXPECT_EQ(NLName("PRIMS"), prims->getName());

  //name collision error
  EXPECT_THROW(NLLibrary::create(db, NLName("PRIMS")), NLException);
  //rename collision
  NLLibrary* prims1 = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS1"));
  ASSERT_NE(nullptr, prims1);
  EXPECT_EQ(NLID::LibraryID(2), prims1->getID());
  EXPECT_THROW(prims1->setName(NLName("PRIMS")), NLException);

  auto subPrims = NLLibrary::create(prims, NLLibrary::Type::Primitives, NLName("SUB_PRIMS"));
  ASSERT_NE(nullptr, subPrims);
  EXPECT_EQ(NLID::LibraryID(3), subPrims->getID());
  //name collision
  EXPECT_THROW(NLLibrary::create(prims, NLLibrary::Type::Primitives, NLName("SUB_PRIMS")), NLException);
  EXPECT_EQ(prims, subPrims->getParentLibrary());

  //rename collision
  auto subPrims1 = NLLibrary::create(prims, NLLibrary::Type::Primitives, NLName("SUB_PRIMS1"));
  ASSERT_NE(nullptr, subPrims1);
  EXPECT_THROW(subPrims1->setName(NLName("SUB_PRIMS")), NLException);
  EXPECT_EQ(NLID::LibraryID(4), subPrims1->getID());

  //rename SUB_PRIMS
  subPrims->setName(NLName("SUB_PRIMS2"));
  EXPECT_EQ(NLName("SUB_PRIMS2"), subPrims->getName());
  EXPECT_EQ(NLID::LibraryID(3), subPrims->getID());
  auto findSubPrims2 = db->getLibrary(NLID::LibraryID(3));
  EXPECT_EQ(findSubPrims2, subPrims);

  //ID collision
  EXPECT_THROW(NLLibrary::create(db, NLID::LibraryID(0), NLLibrary::Type::Standard), NLException);

  EXPECT_EQ(NLID::LibraryID(3), subPrims->getID());
  EXPECT_EQ(prims, subPrims->getParentLibrary());
  EXPECT_EQ(subPrims, prims->getLibrary(NLID::LibraryID(3)));
  EXPECT_EQ(nullptr, prims->getLibrary(NLID::LibraryID(5)));
  //ID collision
  EXPECT_THROW(NLLibrary::create(prims, NLID::LibraryID(2), NLLibrary::Type::Primitives), NLException);
}

TEST_F(NLLibraryTest, caseInsensitiveLookup) {
  auto* library = NLLibrary::create(NLDB::create(universe_), NLName("Cells"));
  auto* first = SNLDesign::create(library, NLName("Gate"));
  EXPECT_EQ(first, library->getSNLDesignCaseInsensitive(NLName("GATE")));
  EXPECT_EQ(nullptr, library->getSNLDesign(NLName("GATE")));
  auto* second = SNLDesign::create(library, NLName("gate"));
  EXPECT_THROW(library->getSNLDesignCaseInsensitive(NLName("Gate")), NLException);
  second->destroy();
  EXPECT_EQ(first, library->getSNLDesignCaseInsensitive(NLName("gate")));
}
