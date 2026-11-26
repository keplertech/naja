// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"
using namespace naja::NL;

class NLDBTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(NLDBTest, test) {
  ASSERT_EQ(NLUniverse::get(), nullptr);
  NLUniverse::create();
  ASSERT_NE(NLUniverse::get(), nullptr);
  auto universe = NLUniverse::get();
  ASSERT_NE(universe, nullptr);
  NLDB* db1 = NLDB::create(universe);
  ASSERT_NE(db1, nullptr);
  EXPECT_EQ(1, db1->getID());
  EXPECT_EQ(universe->getDB(1), db1);
  EXPECT_EQ(NLID(1), db1->getNLID());
  EXPECT_EQ(db1, universe->getObject(NLID(1)));
  NLDB* db2 = NLDB::create(universe);
  ASSERT_NE(db2, nullptr);
  EXPECT_EQ(2, db2->getID());
  EXPECT_EQ(universe->getDB(2), db2);
  EXPECT_EQ(NLID(2), db2->getNLID());
  EXPECT_EQ(db2, universe->getObject(NLID(2)));

  EXPECT_FALSE(universe->getDBs().empty());
  EXPECT_EQ(3, universe->getDBs().size());
  auto it = universe->getDBs().begin();
  EXPECT_NE(it, universe->getDBs().end());
  EXPECT_EQ(NLUniverse::getDB0(), *it);
  ++it;
  EXPECT_EQ(db1, *it);
  ++it;
  EXPECT_EQ(db2, *it);
  ++it;
  EXPECT_EQ(it, universe->getDBs().end());
  EXPECT_FALSE(universe->getUserDBs().empty());
  EXPECT_EQ(2, universe->getUserDBs().size());

  EXPECT_TRUE(db1->getLibraries().empty());
  EXPECT_EQ(0, db1->getLibraries().size());
  EXPECT_TRUE(db2->getLibraries().empty());
  EXPECT_EQ(0, db2->getLibraries().size());
    
  db1->destroy();
  EXPECT_EQ(nullptr, universe->getDB(1));
  EXPECT_EQ(nullptr, universe->getObject(NLID(1)));
}

TEST_F(NLDBTest, testErrors) {
  EXPECT_THROW(NLDB::create(nullptr), NLException);
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  EXPECT_EQ(1, db->getID());
  EXPECT_THROW(NLDB::create(NLUniverse::get(), NLID::DBID(1)), NLException);
}

TEST_F(NLDBTest, caseInsensitiveLookup) {
  auto* db = NLDB::create(NLUniverse::create());
  auto* first = NLLibrary::create(db, NLName("Cells"));
  auto* nested = NLLibrary::create(first, NLName("Nested"));
  EXPECT_EQ(nullptr, db->getLibraryCaseInsensitive(NLName("nested")));
  EXPECT_EQ(first, db->getLibraryCaseInsensitive(NLName("CELLS")));
  EXPECT_EQ(nullptr, db->getLibrary(NLName("CELLS")));
  auto* second = NLLibrary::create(db, NLName("cells"));
  EXPECT_THROW(db->getLibraryCaseInsensitive(NLName("Cells")), NLException);
  second->setName(NLName("Other"));
  EXPECT_EQ(first, db->getLibraryCaseInsensitive(NLName("cells")));
  first->destroy();
  EXPECT_EQ(nullptr, db->getLibraryCaseInsensitive(NLName("cells")));
}
