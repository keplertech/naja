
// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "SNLBitTerm.h"
#include "SNLDesignModeling.h"
#include "SNLTruthTable.h"

namespace naja::NL {

class SNLDesign;
class SNLBitTerm;

class SNLBooleanTreeNode {
  public:
    SNLBooleanTreeNode(const SNLBooleanTreeNode&) = delete;
    virtual bool getValue() const = 0;
    virtual ~SNLBooleanTreeNode() = default;
  protected:
    SNLBooleanTreeNode() = default;
};

class SNLBooleanTreeInputNode: public SNLBooleanTreeNode {
  public:
    enum class Type { INPUT, STATE, CONSTANT0, CONSTANT1 };
    SNLBooleanTreeInputNode(const SNLBitTerm* input):
      type_(Type::INPUT),
      term_(input)
    {}

    SNLBooleanTreeInputNode(Type type):
      type_(type),
      term_(nullptr)
    {}

    SNLBooleanTreeInputNode(size_t state, bool inverted):
      type_(Type::STATE),
      term_(nullptr),
      state_(state),
      stateInverted_(inverted)
    {}

    void setValue(bool value) { value_ = value; }

    bool getValue() const override {
      switch (type_) {
        case Type::INPUT:
        case Type::STATE:
          return value_;
        case Type::CONSTANT0:
          return false;
        case Type::CONSTANT1:
          return true;
      }
      return false; //LCOV_EXCL_LINE
    }

    const SNLBitTerm* getTerm() const {
      return term_;
    }
    Type getType() const { return type_; }
    size_t getState() const { return state_; }
    bool isStateInverted() const { return stateInverted_; }

  private:
    Type              type_     {Type::INPUT};
    const SNLBitTerm* term_     {nullptr};
    size_t            state_    {0};
    bool              stateInverted_ {false};
    bool              value_    {false};
};

class SNLBooleanTreeFunctionNode: public SNLBooleanTreeNode {
  public:
    enum class Type { AND, OR, XOR, NOT, BUFFER };
    using Inputs = std::vector<SNLBooleanTreeNode*>;

    SNLBooleanTreeFunctionNode(Type type):
      type_(type)
    {}

    ~SNLBooleanTreeFunctionNode();

    void addInput(SNLBooleanTreeNode* input) {
      inputs_.push_back(input);
    }

    Type getType() const { return type_; }
    const Inputs& getInputs() const { return inputs_; }

    bool getValue() const override;

  private:
    Type    type_;
    Inputs  inputs_;
};

class SNLBooleanTree {
  public:
    using Inputs = std::map<const SNLBitTerm*, SNLBooleanTreeInputNode*, SNLBitTerm::PointerLess>;
    using StateIdentifier = std::pair<size_t, bool>;
    using StateIdentifiers = std::map<std::string, StateIdentifier>;
    using TermAliases = std::map<std::string, SNLBitTerm*>;

    SNLBooleanTree() = default;
    ~SNLBooleanTree();

    SNLBooleanTreeInputNode* parseInput(
      const SNLDesign* primitive,
      const std::string& function,
      size_t& pos);
    void parse(const SNLDesign* primitive, const std::string& function);
    void parse(
      const SNLDesign* primitive,
      const std::string& function,
      const StateIdentifiers& stateIdentifiers);
    void parse(
      const SNLDesign* primitive,
      const std::string& function,
      const StateIdentifiers& stateIdentifiers,
      const TermAliases& termAliases);
    SNLBooleanTreeFunctionNode* getRoot() const { return root_; }
    const Inputs& getInputs() const { return inputs_; }
    SNLBooleanTreeInputNode* getInput(const SNLBitTerm* inputTerm) const;
    SNLBooleanTreeInputNode* getOrCreateInputNode(const SNLBitTerm* input);
    SNLBooleanTreeInputNode* getOrCreateConstantInputNode(bool constant);
    SNLBooleanTreeInputNode* getOrCreateStateInputNode(size_t state, bool inverted);
    void setRoot(SNLBooleanTreeFunctionNode* root) { root_ = root; }

    using Terms = std::vector<SNLBitTerm*>;
    SNLTruthTable getTruthTable(const Terms& terms);
    SNLDesignModeling::BooleanExpression getBooleanExpression() const;
    /// Literal AND/OR shape, with optional output and input negations.
    std::optional<SNLDesignModeling::GatePolarity> getInvertedInputGate() const;
  private:
    std::string                 function_   {};
    Inputs                      inputs_     {};
    StateIdentifiers            stateIdentifiers_ {};
    TermAliases                 termAliases_ {};
    std::map<StateIdentifier, SNLBooleanTreeInputNode*> stateInputs_ {};
    SNLBooleanTreeInputNode*    constant0_  {nullptr};
    SNLBooleanTreeInputNode*    constant1_  {nullptr};
    SNLBooleanTreeFunctionNode* root_       {nullptr};  
};

}  // namespace naja::NL
