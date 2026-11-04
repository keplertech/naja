// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>
#include <variant>

#include "NLObject.h"
#include "NLName.h"
#include "NajaCollection.h"

namespace naja::NL {

class SNLDesign;

/**
 * \brief NLClock is an SDC-style clock definition owned by the design it belongs to.
 *
 * A primary clock (create_clock) is defined by its period and waveform on its
 * sources. A generated clock (create_generated_clock) derives its period from a
 * master clock: period = master period * divideBy / multiplyBy, and it is
 * synchronous with that master. Generated clocks store only their declaration;
 * period, unit and waveform are evaluated from the current master on each read.
 * For master (P,R,F), division D keeps rise R. Its high time is D*P/2 for
 * even D, and D*(F-R) for odd D. Multiplication M divides the period and high
 * time by M about R. Inversion changes (R,F) into (F,R+P) after generation.
 * Edge times are chronological, not reduced modulo the period. Chains compose
 * these rules per declaration. No explicit edge, duty, phase or shift controls
 * are supported. The master must outlive every referencing generated clock.
 *
 * First-pass restrictions:
 * - sources and master source must be top-level terminals (SNLBitTerm) of the owning design;
 *   instance terminals are rejected until instance-terminal occurrences are supported.
 * - the master clock must belong to the same design.
 * - the master source and the master clock must share a time unit.
 */
class NLClock final: public NLObject {
  public:
    using super = NLObject;

    enum class Kind { Primary, Generated };

    /**
     * \brief Create a primary clock (SDC create_clock).
     * \param design the SNLDesign owning the clock; sources must be its top terminals.
     * \param name the name of the clock.
     * \param period clock period, strictly positive, expressed in timeUnit.
     * \param timeUnit time unit, for example "ns" or "ps".
     * \param sources top-level terminals where the clock is defined; at least one.
     * \return the created NLClock.
     * \throws NLException on invalid arguments.
     */
    static NLClock* createPrimary(
      SNLDesign* design,
      const NLName& name,
      double period,
      const std::string& timeUnit,
      const std::vector<NLObject*>& sources);

    /**
     * \brief Create a generated clock (SDC create_generated_clock).
     * \param design the SNLDesign owning the clock.
     * \param name the name of the clock.
     * \param master the master clock; must belong to design.
     * \param masterSource the top-level terminal on the master's path where the generated clock originates.
     * \param divideBy strictly positive divide factor.
     * \param multiplyBy strictly positive multiply factor.
     * \param invert whether the generated clock is inverted with respect to the master.
     * \param sources top-level terminals where the generated clock is defined; at least one.
     * \return the created NLClock.
     * \throws NLException on invalid arguments.
     */
    static NLClock* createGenerated(
      SNLDesign* design,
      const NLName& name,
      NLClock* master,
      NLObject* masterSource,
      unsigned divideBy,
      unsigned multiplyBy,
      bool invert,
      const std::vector<NLObject*>& sources);

    NLName getName() const { return name_; }
    Kind getKind() const { return std::holds_alternative<Primary>(definition_) ? Kind::Primary : Kind::Generated; }
    SNLDesign* getDesign() const { return design_; }

    /// \return the clock period in timeUnit.
    double getPeriod() const;
    /// \return the frequency in 1/timeUnit, derived from the period.
    double getFrequency() const { return 1.0 / getPeriod(); }
    const std::string& getTimeUnit() const;

    /// \return the first rising edge of the derived waveform. Generated edge
    /// times are relative to the master's first rise, not normalized modulo period.
    double getRiseAt() const;
    /// \return the falling edge following getRiseAt(); for generated clocks
    /// it may exceed the period, notably after inversion.
    double getFallAt() const;
    /// \brief Set a primary waveform; requires 0 <= riseAt < fallAt <= period.
    /// Generated clocks reject overrides with NLException.
    void setWaveform(double riseAt, double fallAt);

    NajaCollection<NLObject*> getSources() const;

    /// \return the master clock, or nullptr for a primary clock.
    NLClock* getMaster() const;
    /// \return the master source terminal, or nullptr for a primary clock.
    NLObject* getMasterSource() const;
    unsigned getDivideBy() const;
    unsigned getMultiplyBy() const;
    bool isInverted() const;

    /// \return the primary clock at the root of the master chain (this clock if primary).
    NLClock* getRootClock();
    /// \return true if other and this clock share the same root clock.
    bool isSynchronousWith(NLClock* other);

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

  protected:

    static void preCreate(const SNLDesign* design, const NLName& name);
    void postCreate() override;
    void preDestroy() override;

  private:
    static void checkSource(const SNLDesign* design, NLObject* source);
    static void checkPeriod(double period, const std::string& timeUnit);

    struct Primary {
      double period;
      std::string timeUnit;
      double riseAt;
      double fallAt;
    };
    struct Generated {
      NLClock* master;
      NLObject* masterSource;
      unsigned divideBy;
      unsigned multiplyBy;
      bool invert;
    };
    using Definition = std::variant<Primary, Generated>;
    struct Timing { double period; double riseAt; double fallAt; };
    NLClock(SNLDesign* design, const NLName& name, Definition definition);
    Timing getTiming() const;

    NLName                 name_;
    SNLDesign*             design_ {nullptr};
    std::vector<NLObject*>  sources_ {};
    Definition             definition_;

};

}  // namespace naja::NL
