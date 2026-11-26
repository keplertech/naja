// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLDB.h"
#include "NLException.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include <map>
#include <vector>

namespace naja::NL {

// A load-scoped index: never changes names or attaches state to primitive models.
class SNLHDLPrimitives {
 public:
  explicit SNLHDLPrimitives(NLDB* db) {
    for (auto* library : db->getLibraries()) {
      if (!library->isPrimitives()) continue;
      for (auto* model : library->getSNLDesigns()) {
        if (!model->isPrimitive() || model->getName().empty()) continue;
        models_.push_back(model);
        exact_[model->getName().getString()].push_back(model);
        folded_[foldNameCase(model->getName().getString())].push_back(model);
      }
    }
  }

  const std::vector<SNLDesign*>& models() const { return models_; }

  SNLDesign* find(const std::string& name, bool insensitive = false,
                  NLLibrary* library = nullptr) const {
    const auto& index = insensitive ? folded_ : exact_;
    auto it = index.find(insensitive ? foldNameCase(name) : name);
    if (it == index.end()) return nullptr;
    SNLDesign* result = nullptr;
    for (auto* model : it->second) {
      if (library && model->getLibrary() != library) continue;
      if (result) throw NLException("ambiguous primitive '" + name + "': " +
          result->getLibrary()->getName().getString() + "." + result->getName().getString() +
          " and " + model->getLibrary()->getName().getString() + "." + model->getName().getString());
      result = model;
    }
    return result;
  }

  static void validateInterface(SNLDesign* model) {
    for (auto* term : model->getTerms()) {
      if ((!dynamic_cast<SNLScalarTerm*>(term) && !dynamic_cast<SNLBusTerm*>(term)) ||
          term->getName().empty() || term->getDirection() == SNLTerm::Direction::Undefined)
        throw NLException("unsupported primitive interface: " + model->getName().getString());
    }
  }

 private:
  std::vector<SNLDesign*> models_;
  std::map<std::string, std::vector<SNLDesign*>> exact_, folded_;
};

}  // namespace naja::NL
