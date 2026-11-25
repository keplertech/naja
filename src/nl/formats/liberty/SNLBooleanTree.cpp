// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBooleanTree.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include "NLBitDependencies.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLLibertyConstructorException.h"

namespace {
  
//Function decoding code taken from Yosys:
//https://github.com/YosysHQ/yosys/blob/main/frontends/liberty/liberty.cc

struct Token {
  char                            type_;
  naja::NL::SNLBooleanTreeNode*  node_ {nullptr};
  
  Token(char t) : type_(t) {}
  Token(char t, naja::NL::SNLBooleanTreeNode* n) : type_(t), node_(n) {}
};

using Stack = std::vector<Token>;

using namespace naja::NL;

void cleanupStack(Stack& stack) {
  std::set<SNLBooleanTreeFunctionNode*> functionNodes;
  for (const auto& token: stack) {
    auto functionNode = dynamic_cast<SNLBooleanTreeFunctionNode*>(token.node_);
    if (functionNode != nullptr) {
      functionNodes.insert(functionNode);
    }
  }

  for (auto* functionNode: functionNodes) {
    delete functionNode;
  }
  stack.clear();
}

bool reduce(
  const SNLDesign* primitive,
  Stack& stack,
  const Token& nextToken) {

  int top = int(stack.size())-1;

  if (0 <= top-1 and stack[top].type_ == 0 and stack[top-1].type_ == '!') {
    auto notNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
    notNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(0, notNode));
    return true;
  }

  if (0 <= top-1 and stack[top].type_ == '\'' and stack[top-1].type_ == 0) {
    auto notNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
    notNode->addInput(stack[top-1].node_);
    stack.pop_back(); 
    stack.pop_back();
    stack.push_back(Token(0, notNode));
    return true;
  }

  if (0 <= top and stack[top].type_ == 0) {
    if (nextToken.type_ == '\'')
      return false;
    stack[top].type_ = 1;
    return true;
  }

  if (0 <= top-2 and stack[top-2].type_ == 1 and stack[top-1].type_ == '^' and stack[top].type_ == 1) {
    auto xorNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::XOR);
    xorNode->addInput(stack[top-2].node_);
    xorNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(1, xorNode));
    return true;
  }

  if (0 <= top && stack[top].type_ == 1) {
    if (nextToken.type_ == '^')
      return false;
    stack[top].type_ = 2;
    return true;
  }

  if (0 <= top-1 and stack[top-1].type_ == 2 and stack[top].type_ == 2) {
    //Top two stack elements are of type 2
    auto andNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::AND);
    andNode->addInput(stack[top-1].node_);
    andNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(2, andNode));
    return true;
  }

  if (0 <= top-2 and stack[top-2].type_ == 2
    and (stack[top-1].type_ == '*' or stack[top-1].type_ == '&')
    and stack[top].type_ == 2) {
    auto andNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::AND);
    andNode->addInput(stack[top-2].node_);
    andNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(2, andNode));
    return true;
  }

  if (0 <= top && stack[top].type_ == 2) {
    if (nextToken.type_ == '*'
      or nextToken.type_ == '&'
      or nextToken.type_ == 0
      or nextToken.type_ == '('
      or nextToken.type_ == '!') {
      return false;
    }
    stack[top].type_ = 3;
    return true;
  }

  if (0 <= top-2
    and stack[top-2].type_ == 3
    and (stack[top-1].type_ == '+' or stack[top-1].type_ == '|') 
    and stack[top].type_ == 3) {
    auto orNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::OR);
    orNode->addInput(stack[top-2].node_);
    orNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(3, orNode));
    return true;
  }

  if (0 <= top-2
    and stack[top-2].type_ == '('
    and stack[top-1].type_ == 3
    and stack[top].type_ == ')') {
    auto t = Token(0, stack[top-1].node_);
    stack.pop_back();
    stack.pop_back();
    stack.pop_back();
    stack.push_back(t);
    return true;
  }
  return false;
}

} // namespace

namespace naja::NL {

SNLBooleanTreeFunctionNode::~SNLBooleanTreeFunctionNode() {
  for (auto input: inputs_) {
    if (not dynamic_cast<SNLBooleanTreeInputNode*>(input)) {
      delete input;
    }
  }
}

SNLBooleanTreeInputNode* SNLBooleanTree::getOrCreateInputNode(const SNLBitTerm* input) {
  auto it = inputs_.find(input);
  if (it != inputs_.end()) {
    return it->second;
  }
  auto inputNode = new SNLBooleanTreeInputNode(input);
  inputs_[input] = inputNode;
  return inputNode;
}

SNLBooleanTreeInputNode* SNLBooleanTree::getOrCreateConstantInputNode(bool constant) {
  if (constant) {
    if (constant1_ == nullptr) {
      constant1_ = new SNLBooleanTreeInputNode(SNLBooleanTreeInputNode::Type::CONSTANT1);
    }
    return constant1_;
  } else {
    if (constant0_ == nullptr) {
      constant0_ = new SNLBooleanTreeInputNode(SNLBooleanTreeInputNode::Type::CONSTANT0);
    }
    return constant0_;
  }
}

SNLBooleanTreeInputNode* SNLBooleanTree::getOrCreateStateInputNode(
    size_t state, bool inverted) {
  const StateIdentifier identifier {state, inverted};
  auto it = stateInputs_.find(identifier);
  if (it != stateInputs_.end()) {
    return it->second;
  }
  auto* inputNode = new SNLBooleanTreeInputNode(state, inverted);
  stateInputs_[identifier] = inputNode;
  return inputNode;
}

SNLBooleanTreeInputNode* SNLBooleanTree::parseInput(
  const SNLDesign* primitive,
  const std::string& function,
  size_t& pos) {
    char car = function[pos];
    int idLen = 0;
    while (('a' <= car and car <= 'z')
      or ('A' <= car and car <= 'Z')
      or ('0' <= car and car <= '9')
      or car == '.'
      or car == '_'
      or car == '['
      or car == ']') {
      ++idLen;
      car = function[pos + idLen];
    }

    if (idLen == 0) {
      std::ostringstream reason;
      reason << "Expected an identifier at character " << (pos + 1)
             << " near `" << function.substr(pos) << "`.";
      throw SNLLibertyConstructorException(reason.str());
    }

    if (idLen == 1) {
      if (function[pos] == '0') {
        pos += idLen;
        return getOrCreateConstantInputNode(false);
      }
      if (function[pos] == '1') {
        pos += idLen;
        return getOrCreateConstantInputNode(true);
      }
    }

    auto inputName = function.substr(pos, idLen);
    if (auto stateIt = stateIdentifiers_.find(inputName);
        stateIt != stateIdentifiers_.end()) {
      pos += idLen;
      return getOrCreateStateInputNode(
          stateIt->second.first, stateIt->second.second);
    }
    if (auto aliasIt = termAliases_.find(inputName);
        aliasIt != termAliases_.end()) {
      pos += idLen;
      return getOrCreateInputNode(aliasIt->second);
    }
    SNLBitTerm* input = nullptr;

    //is it a bus ?
    //auto start = inputName.find('[');
    //if (start != std::string::npos) {
    //  auto stop = inputName.find(']');
    //  if (stop == std::string::npos) {
    //    throw std::runtime_error("Expected `]' at `" + function.substr(pos) + "'.");
    //  }
    //  auto busName = inputName.substr(0, start);
    //  auto busIndex = inputName.substr(start + 1, stop - start - 1);
    //  auto bus = primitive->getBusTerm(SNLName(busName));
    //  if (bus == nullptr) {
    //    throw std::runtime_error("Bus `" + busName + "' not found.");
    //  }
    //  auto index = std::stoi(busIndex);
    //  input = bus->getBit(index);
    //  if (input == nullptr) {
    //    throw std::runtime_error("Bit `" + busIndex + "' not found in bus `" + busName + "'.");
    //  }
    //} else {
      input = primitive->getScalarTerm(NLName(inputName));
      if (input == nullptr) {
        std::ostringstream reason;
        reason << "Scalar term `" << inputName << "` referenced at character "
               << (pos + 1) << " was not found in the cell interface.";
        throw SNLLibertyConstructorException(reason.str());
      }
    //}

    auto inputNode = getOrCreateInputNode(input);

    pos += idLen;
    return inputNode;
}

void SNLBooleanTree::parse(const SNLDesign* primitive, const std::string& function) {
  parse(primitive, function, {});
}

void SNLBooleanTree::parse(
    const SNLDesign* primitive,
    const std::string& function,
    const StateIdentifiers& stateIdentifiers) {
  parse(primitive, function, stateIdentifiers, {});
}

void SNLBooleanTree::parse(
    const SNLDesign* primitive,
    const std::string& function,
    const StateIdentifiers& stateIdentifiers,
    const TermAliases& termAliases) {
  function_ = function;
  stateIdentifiers_ = stateIdentifiers;
  termAliases_ = termAliases;
  size_t pos = 0;
  Stack stack;
  try {
    while (pos < function.size()) {
      const char& car = function[pos];
      if (std::isspace(car) or car == '"') {
        pos++;
        continue;
      }
      Token nextToken(0);
      switch (car) {
        case '(':
        case ')':
        case '\'':
        case '!':
        case '^':
        case '*':
        case '+':
        case '|':
        case '&':
          nextToken = Token(car);
          ++pos;
          break;
        default:
          {
            auto input = parseInput(primitive, function, pos);
            nextToken = Token(0, input);
          }
          break;
      }
      while (reduce(primitive, stack, nextToken)) {}
      stack.push_back(nextToken);
    }
    while (reduce(primitive, stack, Token('.'))) {}

    if (stack.size() != 1 || stack.back().type_ != 3) {
      std::ostringstream reason;
      reason << "Malformed Boolean expression " << std::quoted(function)
             << ": operators and parentheses could not be reduced to one expression.";
      throw SNLLibertyConstructorException(reason.str());
    }

    auto root = stack.back().node_;
    auto inputNode = dynamic_cast<SNLBooleanTreeInputNode*>(root);
    if (inputNode) {
      root_ = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::BUFFER);
      root_->addInput(inputNode);
    } else {
      root_ = static_cast<SNLBooleanTreeFunctionNode*>(stack.back().node_);
    }
  } catch (...) {
    cleanupStack(stack);
    throw;
  }
}

SNLBooleanTreeInputNode* SNLBooleanTree::getInput(const SNLBitTerm* inputTerm) const {
  auto it = inputs_.find(inputTerm);
  if (it != inputs_.end()) {
    return it->second;
  }
  return nullptr;
}

bool SNLBooleanTreeFunctionNode::getValue() const {
  switch (type_) {
    case Type::AND:
      return std::all_of(inputs_.begin(), inputs_.end(), [](auto input) { return input->getValue(); });
    case Type::OR:
      return std::any_of(inputs_.begin(), inputs_.end(), [](auto input) { return input->getValue(); });
    case Type::XOR:
      return std::count_if(inputs_.begin(), inputs_.end(), [](auto input) { return input->getValue(); }) % 2;
    case Type::NOT:
      if (inputs_.size() != 1) {
        throw SNLLibertyConstructorException("NOT node must have exactly one input");
      }
      return not inputs_[0]->getValue();
    case Type::BUFFER:
      if (inputs_.size() != 1) {
        throw SNLLibertyConstructorException("BUFFER node must have exactly one input");
      }
      return inputs_[0]->getValue();
  }
  return false;
}

SNLTruthTable SNLBooleanTree::getTruthTable(const Terms& terms) {
  if (root_ == nullptr) {
    throw SNLLibertyConstructorException("Boolean tree not parsed");
  }
  //translate the terms to the inputs
  std::vector<SNLBooleanTreeInputNode*> inputs;
  std::vector<size_t> deps;
  for (auto term: terms) {
    auto input = getInput(term);
    //if (input == nullptr) {
    //  std::ostringstream reason;
    //  reason << "Term "
    //    << term->getName().getString()
    //    << "' not found in the inputs of the boolean tree for primitive: "
    //    << term->getDesign()->getName().getString();
    //  throw std::runtime_error(reason.str());
    //}
    if (input != nullptr) {
      inputs.push_back(input);
      deps.push_back(term->getOrderID());
    }
  }
  for (const auto& [term, _] : inputs_) {
    if (std::find(terms.begin(), terms.end(), term) == terms.end()) {
      std::ostringstream reason;
      reason << "Term `" << term->getName().getString()
             << "` referenced by Boolean expression `" << function_
             << "` is not a truth table input.";
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  int n = inputs.size();
  
  if (n > 6) {
    int rows = pow(2, n);

    //initialize the inputs
    for (auto input: inputs) {
      input->setValue(false);
    }

    std::vector<bool> mask(1u << n, false);
    for (int i = 0; i < rows; ++i) {
      // Calculate the truth values for this row
      for (int j = 0; j < n; ++j) {
        inputs[j]->setValue((i & (1 << (n - j - 1))) != 0);
      }
      bool result = root_->getValue();
      mask[i] = result;
    }
    return SNLTruthTable(n, mask, NLBitDependencies::encodeBits(deps));
  }

  int rows = pow(2, n);

  //initialize the inputs
  for (auto input: inputs) {
    input->setValue(false);
  }

  uint64_t mask = 0;
  for (int i = 0; i < rows; ++i) {
    // Calculate the truth values for this row
    for (int j = 0; j < n; ++j) {
      inputs[j]->setValue((i & (1 << (n - j - 1))) != 0);
    }
    bool result = root_->getValue();
    mask |= (result ? 1UL : 0UL) << i;
  }
  return SNLTruthTable(n, mask, NLBitDependencies::encodeBits(deps));
}

namespace {
SNLDesignModeling::BooleanExpression::NodeID appendBooleanExpressionNode(
    const SNLBooleanTreeNode* treeNode,
    SNLDesignModeling::BooleanExpression& expression) {
  using Expression = SNLDesignModeling::BooleanExpression;
  if (auto* input = dynamic_cast<const SNLBooleanTreeInputNode*>(treeNode)) {
    switch (input->getType()) {
      case SNLBooleanTreeInputNode::Type::INPUT:
        return expression.addTerm(const_cast<SNLBitTerm*>(input->getTerm()));
      case SNLBooleanTreeInputNode::Type::STATE: {
        auto state = expression.addState(input->getState());
        return input->isStateInverted()
            ? expression.addOperation(Expression::Operator::Not, {state})
            : state;
      }
      case SNLBooleanTreeInputNode::Type::CONSTANT0:
        return expression.addConstant(false);
      case SNLBooleanTreeInputNode::Type::CONSTANT1:
        return expression.addConstant(true);
    }
  }

  auto* function = dynamic_cast<const SNLBooleanTreeFunctionNode*>(treeNode);
  if (function == nullptr) {
    throw SNLLibertyConstructorException("Invalid Boolean tree node");
  }
  std::vector<Expression::NodeID> operands;
  operands.reserve(function->getInputs().size());
  for (auto* input : function->getInputs()) {
    operands.push_back(appendBooleanExpressionNode(input, expression));
  }
  switch (function->getType()) {
    case SNLBooleanTreeFunctionNode::Type::BUFFER:
      if (operands.size() != 1) {
        throw SNLLibertyConstructorException(
            "BUFFER node must have exactly one input");
      }
      return operands.front();
    case SNLBooleanTreeFunctionNode::Type::NOT:
      return expression.addOperation(Expression::Operator::Not, std::move(operands));
    case SNLBooleanTreeFunctionNode::Type::AND:
      return expression.addOperation(Expression::Operator::And, std::move(operands));
    case SNLBooleanTreeFunctionNode::Type::OR:
      return expression.addOperation(Expression::Operator::Or, std::move(operands));
    case SNLBooleanTreeFunctionNode::Type::XOR:
      return expression.addOperation(Expression::Operator::Xor, std::move(operands));
  }
  throw SNLLibertyConstructorException("Invalid Boolean tree function");
}
}

SNLDesignModeling::BooleanExpression SNLBooleanTree::getBooleanExpression() const {
  if (root_ == nullptr) {
    throw SNLLibertyConstructorException("Boolean tree not parsed");
  }
  SNLDesignModeling::BooleanExpression expression;
  expression.nodes.reserve(inputs_.size() + stateInputs_.size() + 1);
  expression.root = appendBooleanExpressionNode(root_, expression);
  return expression;
}

SNLBooleanTree::~SNLBooleanTree() {
  delete root_;
  for (auto input: inputs_) {
    delete input.second;
  }
  for (auto input: stateInputs_) {
    delete input.second;
  }
  delete constant0_;
  delete constant1_;
}

} // namespace naja::NL
