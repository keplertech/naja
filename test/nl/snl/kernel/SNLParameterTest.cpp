// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLParameter.h"
using namespace naja::NL;

class SNLParameterTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      designsLib_ = NLLibrary::create(db);
      design_ = SNLDesign::create(designsLib_);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      design_ = nullptr;
    }
    NLLibrary*   designsLib_ {nullptr};
    SNLDesign*    design_     {nullptr};
};

TEST_F(SNLParameterTest, test) {
  ASSERT_NE(NLUniverse::get(), nullptr);
  ASSERT_NE(design_, nullptr);
  EXPECT_TRUE(design_->getParameters().empty());

  auto param1 = SNLParameter::create(design_, NLName("PARAM1"), SNLParameter::Type::Decimal, "45");
  ASSERT_NE(nullptr, param1);
  EXPECT_EQ(NLName("PARAM1"), param1->getName());
  EXPECT_EQ(SNLParameter::Type::Decimal, param1->getType());
  EXPECT_EQ("45", param1->getValue());
  EXPECT_EQ(design_, param1->getDesign());

  EXPECT_FALSE(design_->getParameters().empty());
  EXPECT_EQ(1, design_->getParameters().size());

  auto designParam1 = design_->getParameter(NLName("PARAM1"));
  ASSERT_NE(designParam1, nullptr);

  auto param2 = SNLParameter::create(design_, NLName("PARAM2"), SNLParameter::Type::Binary, "56");
  ASSERT_NE(nullptr, param2);
  EXPECT_EQ(NLName("PARAM2"), param2->getName());
  EXPECT_EQ(SNLParameter::Type::Binary, param2->getType());
  EXPECT_EQ("56", param2->getValue());
  EXPECT_EQ(design_, param2->getDesign());

  EXPECT_FALSE(design_->getParameters().empty());
  EXPECT_EQ(2, design_->getParameters().size());

  auto designParam2 = design_->getParameter(NLName("PARAM2"));
  ASSERT_NE(designParam2, nullptr);

  EXPECT_EQ(param1, designParam1);
  EXPECT_EQ(param2, designParam2);

  //Instance Parameters
  auto top = SNLDesign::create(design_->getLibrary());
  auto instance = SNLInstance::create(top, design_);
  EXPECT_TRUE(instance->getInstParameters().empty());
  auto instParam1 = SNLInstParameter::create(instance, param1, "73");
  auto instParam2 = SNLInstParameter::create(instance, param2, "87");
  EXPECT_EQ(2, instance->getInstParameters().size());
  EXPECT_EQ("PARAM1", instParam1->getName().getString());
  EXPECT_EQ("PARAM2", instParam2->getName().getString());
  EXPECT_EQ("73", instParam1->getValue());
  EXPECT_EQ("87", instParam2->getValue());
  EXPECT_EQ(instParam1, instance->getInstParameter(NLName("PARAM1")));
  EXPECT_EQ(instParam2, instance->getInstParameter(NLName("PARAM2")));

  //Change value
  instParam1->setValue("99");
  EXPECT_EQ("99", instParam1->getValue());

  //InstParam destruction
  instParam1->destroy();
  EXPECT_EQ(1, instance->getInstParameters().size());
  EXPECT_EQ(nullptr, instance->getInstParameter(NLName("PARAM1")));

  using ParamsVector = std::vector<SNLParameter*>;
  ParamsVector paramsVector(design_->getParameters().begin(), design_->getParameters().end());
  ASSERT_EQ(2, paramsVector.size());
  EXPECT_EQ(param1, paramsVector[0]);
  EXPECT_EQ(param2, paramsVector[1]);
  EXPECT_THAT(std::vector(design_->getParameters().begin(), design_->getParameters().end()),
    ElementsAre(param1, param2));

  EXPECT_THROW(SNLParameter::create(design_, NLName("PARAM1"), SNLParameter::Type::Decimal, "56"), NLException);

  param1->destroy();
  EXPECT_EQ(nullptr, design_->getParameter(NLName("PARAM1")));
  EXPECT_NE(nullptr, design_->getParameter(NLName("PARAM2")));
  EXPECT_EQ(1, design_->getParameters().size());
  EXPECT_THAT(std::vector(design_->getParameters().begin(), design_->getParameters().end()),
    ElementsAre(param2));
}

TEST_F(SNLParameterTest, testInstanceParameterCreationError) {
  auto model1 = SNLDesign::create(designsLib_);
  auto param = SNLParameter::create(model1, NLName("PARAM"), SNLParameter::Type::String, "TEST");
  auto model2 = SNLDesign::create(designsLib_);
  auto top = SNLDesign::create(designsLib_);
  auto instance = SNLInstance::create(top, model2);
  EXPECT_THROW(
    SNLInstParameter::create(instance, param, "ERROR"),
    NLException);
}

TEST_F(SNLParameterTest, requiredParameters) {
  auto parameter = SNLParameter::create(design_, NLName("WIDTH"), SNLParameter::Type::Decimal);
  EXPECT_FALSE(parameter->hasDefaultValue());
  EXPECT_THROW(parameter->getValue(), NLException);
  EXPECT_NE(std::string::npos, parameter->getDescription().find("no default"));
  EXPECT_THROW(SNLParameter::create(design_, NLName("WIDTH"), SNLParameter::Type::Decimal), NLException);

  auto empty = SNLParameter::create(design_, NLName("TEXT"), SNLParameter::Type::String, "");
  EXPECT_TRUE(empty->hasDefaultValue());
  EXPECT_EQ("", empty->getValue());
  auto clone = design_->clone(NLName("clone"));
  std::string reason;
  EXPECT_TRUE(parameter->deepCompare(clone->getParameter(NLName("WIDTH")), reason));
  EXPECT_FALSE(clone->getParameter(NLName("WIDTH"))->hasDefaultValue());
  EXPECT_TRUE(clone->getParameter(NLName("TEXT"))->hasDefaultValue());
  EXPECT_EQ("", clone->getParameter(NLName("TEXT"))->getValue());

  auto other = SNLDesign::create(designsLib_);
  auto requiredText = SNLParameter::create(other, NLName("TEXT"), SNLParameter::Type::String);
  EXPECT_FALSE(empty->deepCompare(requiredText, reason));
  auto instance = SNLInstance::create(other, design_);
  auto actual = SNLInstParameter::create(instance, parameter, "8");
  EXPECT_EQ("8", actual->getValue());
}

TEST_F(SNLParameterTest, lexicalOrderIgnoresInterningAndInsertionOrder) {
  const auto lastName = NLName("zeta");
  const auto firstName = NLName("alpha");
  const auto middleName = NLName("middle");
  auto* last = SNLParameter::create(design_, lastName, SNLParameter::Type::Decimal, "1");
  auto* first = SNLParameter::create(design_, firstName, SNLParameter::Type::Decimal, "2");
  auto* middle = SNLParameter::create(design_, middleName, SNLParameter::Type::Decimal, "3");
  EXPECT_THAT(std::vector(design_->getParameters().begin(), design_->getParameters().end()),
    ElementsAre(first, middle, last));
  auto* instance = SNLInstance::create(SNLDesign::create(designsLib_), design_);
  auto* lastOverride = SNLInstParameter::create(instance, last, "4");
  auto* middleOverride = SNLInstParameter::create(instance, middle, "5");
  auto* firstOverride = SNLInstParameter::create(instance, first, "6");
  EXPECT_THAT(std::vector(instance->getInstParameters().begin(), instance->getInstParameters().end()),
    ElementsAre(firstOverride, middleOverride, lastOverride));
  for (auto* parameter : {first, middle, last}) {
    EXPECT_EQ(parameter, design_->getParameter(parameter->getName()));
    auto* override = instance->getInstParameter(parameter->getName());
    ASSERT_NE(nullptr, override);
    EXPECT_EQ(parameter, override->getParameter());
  }
}
