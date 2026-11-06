// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLClock.h"

#include <cmath>
#include <utility>
#include <memory>

#include "NLException.h"
#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"

namespace naja::NL {

namespace {

std::string sourceDescription(const NLObject* source) {
  return source ? source->getDescription() : std::string("null");
}

}  // namespace

NLClock* NLClock::createPrimary(
  SNLDesign* design,
  const NLName& name,
  double period,
  const std::string& timeUnit,
  const std::vector<NLObject*>& sources)
{
  preCreate(design, name);
  checkPeriod(period, timeUnit);
  if (sources.empty()) {
    throw NLException("NLClock " + name.getString() + ": a primary clock needs at least one source");
  }
  for (NLObject* source: sources) {
    checkSource(design, source);
  }

  auto clock = std::unique_ptr<NLClock>(new NLClock(design, name, Primary{period, timeUnit, 0.0, period / 2.0}));
  clock->sources_ = sources;
  clock->setWaveform(0.0, period / 2.0);
  clock->postCreate();
  return clock.release();
}

NLClock* NLClock::createGenerated(
  SNLDesign* design,
  const NLName& name,
  NLClock* master,
  NLObject* masterSource,
  unsigned divideBy,
  unsigned multiplyBy,
  bool invert,
  const std::vector<NLObject*>& sources)
{
  preCreate(design, name);
  if (!master) {
    throw NLException("NLClock " + name.getString() + ": a generated clock needs a master clock");
  }
  if (master->getDesign() != design) {
    throw NLException("NLClock " + name.getString() + ": master clock " + master->getName().getString()
      + " belongs to another design");
  }
  checkSource(design, masterSource);
  if (divideBy == 0 || multiplyBy == 0) {
    throw NLException("NLClock " + name.getString() + ": divide and multiply factors must be strictly positive");
  }
  if (sources.empty()) {
    throw NLException("NLClock " + name.getString() + ": a generated clock needs at least one source");
  }
  for (NLObject* source: sources) {
    checkSource(design, source);
  }

  // Validate derived values before publishing the declaration to the design.
  auto clock = std::unique_ptr<NLClock>(new NLClock(design, name,
    Generated{master, masterSource, divideBy, multiplyBy, invert}));
  clock->getTiming();
  clock->sources_ = sources;
  clock->postCreate();
  return clock.release();
}

NLClock::NLClock(SNLDesign* design, const NLName& name, Definition definition):
  name_(name),
  design_(design),
  definition_(std::move(definition))
{}

void NLClock::preCreate(const SNLDesign* design, const NLName& name) {
  if (!design) {
    throw NLException("NLClock: design is null");
  }
  if (name.getString().empty()) {
    throw NLException("NLClock: a clock needs a name");
  }
  for (const NLClock* clock: design->getClocks()) {
    if (clock->getName() == name) {
      throw NLException("NLClock " + name.getString() + " already exists in design " + design->getString());
    }
  }
}

void NLClock::postCreate() {
  super::postCreate();
  design_->addClock(this);
}

void NLClock::preDestroy() {
  design_->removeClock(this);
  super::preDestroy();
}

void NLClock::checkSource(const SNLDesign* design, NLObject* source) {
  if (!source) {
    throw NLException("NLClock: source is null");
  }
  if (dynamic_cast<SNLInstTerm*>(source)) {
    throw NLException("NLClock: instance terminal " + sourceDescription(source)
      + " is not supported yet, only top-level terminals");
  }
  auto* term = dynamic_cast<SNLBitTerm*>(source);
  if (!term) {
    throw NLException("NLClock: " + sourceDescription(source) + " is not a top-level terminal");
  }
  for (auto* reset : design->getResets()) {
    for (auto* resetSource : reset->getSources()) {
      if (resetSource == source) {
        throw NLException("NLClock: a reset source cannot also be a clock source");
      }
    }
  }
  if (term->getDesign() != design) {
    throw NLException("NLClock: terminal " + sourceDescription(source) + " belongs to another design");
  }
}

void NLClock::checkPeriod(double period, const std::string& timeUnit) {
  if (!std::isfinite(period) || period <= 0.0) {
    throw NLException("NLClock: period must be a finite, strictly positive value");
  }
  if (timeUnit.empty()) {
    throw NLException("NLClock: time unit is required");
  }
}

void NLClock::setWaveform(double riseAt, double fallAt) {
  auto* primary = std::get_if<Primary>(&definition_);
  if (!primary) {
    throw NLException("NLClock " + name_.getString()
      + ": setWaveform is only supported on primary clocks; generated waveforms are derived from the master");
  }
  if (!std::isfinite(riseAt) || !std::isfinite(fallAt)
      || riseAt < 0.0 || riseAt >= fallAt || fallAt > primary->period) {
    throw NLException("NLClock " + name_.getString()
      + ": waveform requires 0 <= riseAt < fallAt <= period");
  }
  primary->riseAt = riseAt;
  primary->fallAt = fallAt;
}

NLClock::Timing NLClock::getTiming() const {
  if (const auto* primary = std::get_if<Primary>(&definition_)) {
    return {primary->period, primary->riseAt, primary->fallAt};
  }
  const auto& generated = std::get<Generated>(definition_);
  const auto master = generated.master->getTiming();
  // Form D/M before multiplying the period to avoid needless intermediate
  // overflow. Long double retains extra precision on platforms that provide it.
  const long double scale = static_cast<long double>(generated.divideBy) / generated.multiplyBy;
  const long double period = master.period * scale;
  const long double high = generated.divideBy % 2 == 0
    ? period / 2 : (static_cast<long double>(master.fallAt) - master.riseAt) * scale;
  const long double rise = generated.invert ? master.riseAt + high : master.riseAt;
  const long double fall = generated.invert ? master.riseAt + period : master.riseAt + high;
  Timing timing {static_cast<double>(period), static_cast<double>(rise), static_cast<double>(fall)};
  if (!std::isfinite(timing.period) || timing.period <= 0.0 ||
      !std::isfinite(timing.riseAt) || !std::isfinite(timing.fallAt)) {
    throw NLException("NLClock " + name_.getString()
      + ": derived period and waveform must be finite, with a strictly positive period");
  }
  return timing;
}

double NLClock::getPeriod() const { return getTiming().period; }
double NLClock::getRiseAt() const { return getTiming().riseAt; }
double NLClock::getFallAt() const { return getTiming().fallAt; }
const std::string& NLClock::getTimeUnit() const {
  if (const auto* primary = std::get_if<Primary>(&definition_)) return primary->timeUnit;
  return std::get<Generated>(definition_).master->getTimeUnit();
}
NLClock* NLClock::getMaster() const {
  const auto* generated = std::get_if<Generated>(&definition_);
  return generated ? generated->master : nullptr;
}
NLObject* NLClock::getMasterSource() const {
  const auto* generated = std::get_if<Generated>(&definition_);
  return generated ? generated->masterSource : nullptr;
}
unsigned NLClock::getDivideBy() const {
  const auto* generated = std::get_if<Generated>(&definition_);
  return generated ? generated->divideBy : 1;
}
unsigned NLClock::getMultiplyBy() const {
  const auto* generated = std::get_if<Generated>(&definition_);
  return generated ? generated->multiplyBy : 1;
}
bool NLClock::isInverted() const {
  const auto* generated = std::get_if<Generated>(&definition_);
  return generated && generated->invert;
}

NajaCollection<NLObject*> NLClock::getSources() const {
  return new NajaSTLCollection(&sources_);
}

NLClock* NLClock::getRootClock() {
  NLClock* root = this;
  while (root->getMaster()) {
    root = root->getMaster();
  }
  return root;
}

bool NLClock::isSynchronousWith(NLClock* other) {
  return other && getRootClock() == other->getRootClock();
}

const char* NLClock::getTypeName() const {
  return "NLClock";
}

std::string NLClock::getString() const {
  return name_.getString();
}

std::string NLClock::getDescription() const {
  return "NLClock " + name_.getString() + " (period " + std::to_string(getPeriod()) + " " + getTimeUnit() + ")";
} // LCOV_EXCL_LINE Compiler-generated cleanup if string allocation throws.

void NLClock::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  std::string pad(indent, ' ');
  stream << pad << getDescription() << std::endl;
  stream << pad << "  kind: " << (getKind() == Kind::Primary ? "primary" : "generated") << std::endl;
  stream << pad << "  waveform: " << getRiseAt() << " " << getFallAt() << std::endl;
  for (const NLObject* source: sources_) {
    stream << pad << "  source: " << sourceDescription(source) << std::endl;
  }
  if (getMaster()) {
    stream << pad << "  master: " << getMaster()->getString() << " divide " << getDivideBy()
      << " multiply " << getMultiplyBy() << (isInverted() ? " inverted" : "") << std::endl;
  }
}

}  // namespace naja::NL
