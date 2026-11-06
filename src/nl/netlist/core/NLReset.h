// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "NLObject.h"
#include "NLName.h"
#include "NajaCollection.h"
#include "SNLDesignModeling.h"

namespace naja::NL {
class SNLDesign;
/// Design-owned reset declaration. Sources are owning-design bit terminals.
/// Each occurrence of a shared model creates an independent analysis domain.
class NLReset final: public NLObject {
 public:
  using ActiveLevel = SNLDesignModeling::SNLActiveLevel;
  enum class Timing { Async, Sync };

  static NLReset* create(SNLDesign* design, const NLName& name,
    const std::vector<NLObject*>& sources, ActiveLevel level,
    Timing assertion = Timing::Async, Timing deassertion = Timing::Sync);
  NLName getName() const { return name_; }
  SNLDesign* getDesign() const { return design_; }
  NajaCollection<NLObject*> getSources() const;
  ActiveLevel getActiveLevel() const { return level_; }
  int getInactiveValue() const { return level_ == ActiveLevel::Low ? 1 : 0; }
  Timing getAssertion() const { return assertion_; }
  Timing getDeassertion() const { return deassertion_; }
  const char* getTypeName() const override { return "NLReset"; }
  std::string getString() const override { return name_.getString(); }
  std::string getDescription() const override { return "NLReset " + getString(); }
  void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
 protected:
  void preDestroy() override;
 private:
  NLReset(SNLDesign* design, const NLName& name, const std::vector<NLObject*>& sources,
    ActiveLevel level, Timing assertion, Timing deassertion);
  SNLDesign* design_;
  NLName name_;
  std::vector<NLObject*> sources_;
  ActiveLevel level_;
  Timing assertion_, deassertion_;
};
}
