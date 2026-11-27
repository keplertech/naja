// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLBooleanTree.h"
#include "SNLLibertyConstructorException.h"
using namespace naja::NL;

class SNLBooleanTreeTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*      library_;
};

namespace {

class InvalidBooleanTreeNode final: public SNLBooleanTreeNode {
  public:
    bool getValue() const override { return false; }
};

}

TEST_F(SNLBooleanTreeTest1, testError0) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  auto aNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("A")));
  auto bNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("B")));
  auto invNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
  tree->setRoot(invNode);
  invNode->addInput(aNode);
  invNode->addInput(bNode);
  EXPECT_THROW(invNode->getValue(), SNLLibertyConstructorException);
}

TEST_F(SNLBooleanTreeTest1, testError1) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  auto aNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("A")));
  auto bNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("B")));
  auto bufNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::BUFFER);
  tree->setRoot(bufNode);
  bufNode->addInput(aNode);
  bufNode->addInput(bNode);
  EXPECT_THROW(bufNode->getValue(), SNLLibertyConstructorException);
}

TEST_F(SNLBooleanTreeTest1, testBooleanExpressionNodes) {
  auto primitive = SNLDesign::create(
      library_, SNLDesign::Type::Primitive, NLName("EXPRESSION"));
  SNLScalarTerm::create(
      primitive, SNLTerm::Direction::Input, NLName("A"));
  SNLScalarTerm::create(
      primitive, SNLTerm::Direction::Input, NLName("B"));

  SNLBooleanTree tree;
  auto* state = tree.getOrCreateStateInputNode(0, false);
  EXPECT_EQ(state, tree.getOrCreateStateInputNode(0, false));
  tree.parse(
      primitive,
      "Q + QN + 0 + 1 + (A ^ B)",
      {{"Q", {0, false}}, {"QN", {0, true}}});

  const auto expression = tree.getBooleanExpression();
  EXPECT_TRUE(expression.isValid());
  bool hasFalse = false;
  bool hasTrue = false;
  bool hasState = false;
  bool hasNot = false;
  bool hasXor = false;
  for (const auto& node : expression.nodes) {
    if (node.operation ==
        SNLDesignModeling::BooleanExpression::Operator::Constant) {
      hasFalse |= !node.constant;
      hasTrue |= node.constant;
    } else if (node.operation ==
               SNLDesignModeling::BooleanExpression::Operator::State) {
      hasState = true;
    } else if (node.operation ==
               SNLDesignModeling::BooleanExpression::Operator::Not) {
      hasNot = true;
    } else if (node.operation ==
               SNLDesignModeling::BooleanExpression::Operator::Xor) {
      hasXor = true;
    }
  }
  EXPECT_TRUE(hasFalse);
  EXPECT_TRUE(hasTrue);
  EXPECT_TRUE(hasState);
  EXPECT_TRUE(hasNot);
  EXPECT_TRUE(hasXor);
}

TEST_F(SNLBooleanTreeTest1, testBooleanExpressionErrors) {
  SNLBooleanTree unparsed;
  EXPECT_THROW(
      unparsed.getBooleanExpression(), SNLLibertyConstructorException);

  {
    SNLBooleanTree tree;
    auto* root = new SNLBooleanTreeFunctionNode(
        SNLBooleanTreeFunctionNode::Type::BUFFER);
    root->addInput(tree.getOrCreateConstantInputNode(false));
    root->addInput(tree.getOrCreateConstantInputNode(true));
    tree.setRoot(root);
    EXPECT_THROW(tree.getBooleanExpression(), SNLLibertyConstructorException);
  }

  {
    SNLBooleanTree tree;
    auto* root = new SNLBooleanTreeFunctionNode(
        SNLBooleanTreeFunctionNode::Type::AND);
    root->addInput(new InvalidBooleanTreeNode());
    tree.setRoot(root);
    EXPECT_THROW(tree.getBooleanExpression(), SNLLibertyConstructorException);
  }

  {
    SNLBooleanTree tree;
    auto* root = new SNLBooleanTreeFunctionNode(
        static_cast<SNLBooleanTreeFunctionNode::Type>(100));
    tree.setRoot(root);
    EXPECT_THROW(tree.getBooleanExpression(), SNLLibertyConstructorException);
  }

  {
    auto invalidInput = std::make_unique<SNLBooleanTreeInputNode>(
        static_cast<SNLBooleanTreeInputNode::Type>(100));
    SNLBooleanTree tree;
    auto* root = new SNLBooleanTreeFunctionNode(
        SNLBooleanTreeFunctionNode::Type::AND);
    root->addInput(invalidInput.get());
    tree.setRoot(root);
    EXPECT_THROW(tree.getBooleanExpression(), SNLLibertyConstructorException);
  }
}

TEST_F(SNLBooleanTreeTest1, LiteralGateShapes) {
  auto* cell = SNLDesign::create(library_, SNLDesign::Type::Primitive);
  for (auto name : {"P", "Q", "R"})
    SNLScalarTerm::create(cell, SNLTerm::Direction::Input, NLName(name));
  SNLScalarTerm::create(cell, SNLTerm::Direction::Output, NLName("Y"));
  using Gate = SNLTruthTable::GenericType;
  for (const auto& [text, type] : std::vector<std::pair<std::string, Gate>>{
      {"P*!Q*R", Gate::AND}, {"(P&Q'&R)'", Gate::NAND},
      {"P|Q'|R", Gate::OR}, {"!(P+Q'+R)", Gate::NOR}}) {
    SNLBooleanTree tree;
    tree.parse(cell, text);
    auto gate = tree.getInvertedInputGate();
    ASSERT_TRUE(gate) << text;
    EXPECT_EQ(type, gate->type);
    EXPECT_EQ((std::vector<SNLBitTerm*>{cell->getScalarTerm(NLName("Q"))}), gate->invertedInputs);
  }
  for (auto text : {"P*P*!Q*R", "P*(Q+R)", "!(P^Q^R)",
                    "P*!(Q*R)", "P*Q", "P*Q*R*1", "P*!!Q*R"}) {
    SNLBooleanTree tree;
    tree.parse(cell, text);
    EXPECT_FALSE(tree.getInvertedInputGate()) << text;
  }
}
