// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "NLReset.h"
#include <memory>
#include <set>
#include "SNLDesign.h"
#include "SNLBitTerm.h"
#include "NLException.h"

namespace naja::NL {
NLReset::NLReset(SNLDesign* design, const NLName& name,
    const std::vector<NLObject*>& sources, ActiveLevel level,
    Timing assertion, Timing deassertion):
  design_(design), name_(name), sources_(sources), level_(level),
  assertion_(assertion), deassertion_(deassertion) {}

NLReset* NLReset::create(SNLDesign* design, const NLName& name,
    const std::vector<NLObject*>& sources, ActiveLevel level,
    Timing assertion, Timing deassertion) {
  if (!design) {
    throw NLException("NLReset: design is null");
  }
  if (name.getString().empty()) {
    throw NLException("NLReset: a reset needs a name");
  }
  for (auto* reset : design->getResets()) {
    if (reset->getName() == name) {
      throw NLException("NLReset: duplicate name " + name.getString());
    }
  }
  if (level != ActiveLevel::Low && level != ActiveLevel::High) {
    throw NLException("NLReset: active level must be Low or High");
  }
  for (auto timing : {assertion, deassertion}) {
    if (timing != Timing::Async && timing != Timing::Sync) {
      throw NLException("NLReset: assertion and deassertion must be Timing::Async or Timing::Sync");
    }
  }
  if (sources.empty()) {
    throw NLException("NLReset: at least one source is required");
  }
  std::set<NLObject*> seen;
  for (auto* source : sources) {
    auto* term = dynamic_cast<SNLBitTerm*>(source);
    if (!term) {
      throw NLException("NLReset: sources must be owning-design bit terminals");
    }
    if (term->getDesign() != design) {
      throw NLException("NLReset: source belongs to another design");
    }
    for (auto* clock : design->getClocks()) {
      for (auto* clockSource : clock->getSources()) {
        if (clockSource == source) {
          throw NLException("NLReset: a clock source cannot also be a reset source");
        }
      }
    }
    if (!seen.insert(source).second) {
      throw NLException("NLReset: duplicate source");
    }
  }
  auto reset = std::unique_ptr<NLReset>(new NLReset(design, name, sources, level, assertion, deassertion));
  reset->postCreate();
  design->addReset(reset.get());
  return reset.release();
}
void NLReset::preDestroy() {
  design_->removeReset(this);
  NLObject::preDestroy();
}
NajaCollection<NLObject*> NLReset::getSources() const { return new NajaSTLCollection(&sources_); }
void NLReset::debugDump(size_t indent, bool, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
} // namespace naja::NL
