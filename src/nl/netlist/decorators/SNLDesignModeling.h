// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "SNLBitTerm.h"
#include "SNLTruthTable.h"
namespace naja::NL {

class SNLInstance;
class SNLInstTerm;
class SNLDesign;
class SNLParameter;

/**
 * \brief SNLDesignModeling allows to add timing informations on primitives and blackboxes.
 */
class SNLDesignModeling {
  public:
    enum class SNLTermRole {
      Clock, DataInput, DataOutput, AsyncReset, AsyncSet, SyncReset, SyncSet, Enable,
      MemoryReadAddress, MemoryReadData, MemoryWriteAddress,
      MemoryWriteData, MemoryWriteEnable, Other, ScanInput, ScanEnable
    };
    enum class SNLActiveLevel { High, Low, NA };
    struct TermRole {
      SNLTermRole role {SNLTermRole::Other};
      SNLActiveLevel activeLevel {SNLActiveLevel::NA};
    };
    using TermArcs = std::set<SNLBitTerm*, SNLBitTerm::InDesignLess>;
    using Arcs = std::map<SNLBitTerm*, TermArcs, SNLBitTerm::InDesignLess>;
    // Each shared set describes one side of a complete bipartite arc group.
    using GroupedArcs = std::map<SNLBitTerm*,
        std::vector<std::shared_ptr<const TermArcs>>, SNLBitTerm::InDesignLess>;
    struct TimingArcs {
      Arcs inputCombinatorialArcs_  {};
      Arcs outputCombinatorialArcs_ {};
      GroupedArcs inputCombinatorialGroups_ {};
      GroupedArcs outputCombinatorialGroups_ {};
      Arcs inputToClockArcs_        {};
      Arcs clockToInputArcs_        {};
      Arcs outputToClockArcs_       {};
      Arcs clockToOutputArcs_       {};
    };
    //First is parameter name, second is default value
    using Parameter = std::pair<std::string, std::string>;
    using ParameterizedArcs = std::map<std::string, TimingArcs>;
    enum Type { PARAMETERIZED, NO_PARAMETER };
    using TimingModel = std::variant<ParameterizedArcs, TimingArcs>;
    using BitTerms = std::list<SNLBitTerm*>;
    // Generic black-box memory description used by frontends and downstream
    // tools. The terms are intentionally kept as SNL bit terms instead of
    // frontend-specific names so users like SEC can model memories without
    // hardcoding fakeram/CVA6 pin naming conventions.
    enum class MemoryResetMode {
      None,
      AsyncLow,
      AsyncHigh,
      SyncLow,
      SyncHigh
    };
    struct MemoryReadPort {
      BitTerms address {};
      BitTerms data    {};
      BitTerms enables {};
    };
    struct MemoryWritePort {
      BitTerms address {};
      BitTerms data    {};
      BitTerms mask    {};
      BitTerms enables {};
      // Extra write-side dependency buses are preserved for completeness, but
      // callers may reject them if they only support address/data/mask/enable
      // memories.
      std::vector<BitTerms> extraWriteInputs {};
      // Overrides MemoryInterface.clock for this port when non-null.
      SNLBitTerm* clock {nullptr};
    };
    struct MemoryInterface {
      size_t width  {0};
      size_t depth  {0};
      size_t abits  {0};
      MemoryResetMode resetMode {MemoryResetMode::None};
      SNLBitTerm* clock {nullptr};
      SNLBitTerm* reset {nullptr};
      std::vector<MemoryReadPort> readPorts   {};
      std::vector<MemoryWritePort> writePorts {};

      bool isValid() const {
        if (width == 0 || depth == 0 || abits == 0 ||
            readPorts.empty()) return false;
        for (const auto& port : writePorts)
          if (!port.clock && !clock) return false;
        return true;
      }
    };

    struct BooleanExpression {
      using NodeID = size_t;
      static constexpr NodeID InvalidNode = std::numeric_limits<NodeID>::max();
      enum class Operator { Constant, Term, State, Not, And, Or, Xor };
      struct Node {
        Operator operation {Operator::Constant};
        bool constant {false};
        SNLBitTerm* term {nullptr};
        size_t state {0};
        std::vector<NodeID> operands {};
      };

      NodeID addConstant(bool value) {
        nodes.push_back({Operator::Constant, value});
        return nodes.size() - 1;
      }
      NodeID addTerm(SNLBitTerm* term) {
        nodes.push_back({Operator::Term, false, term});
        return nodes.size() - 1;
      }
      NodeID addState(size_t state) {
        nodes.push_back({Operator::State, false, nullptr, state});
        return nodes.size() - 1;
      }
      NodeID addOperation(Operator operation, std::vector<NodeID> operands) {
        nodes.push_back({operation, false, nullptr, 0, std::move(operands)});
        return nodes.size() - 1;
      }
      bool isValid() const { return root < nodes.size(); }

      std::vector<Node> nodes {};
      NodeID root {InvalidNode};
    };

    struct SequentialState {
      enum class ClearPresetValue { Zero, One, Hold, Toggle, Unknown };
      BooleanExpression nextState {};
      std::optional<BooleanExpression> clear {};
      std::optional<BooleanExpression> preset {};
      ClearPresetValue clearPresetValue {ClearPresetValue::Unknown};
    };
    struct SequentialOutput {
      SNLBitTerm* term {nullptr};
      BooleanExpression function {};
    };
    struct SequentialModel {
      enum class Kind { FlipFlop, Latch };
      Kind kind {Kind::FlipFlop};
      BooleanExpression clockedOn {};
      std::vector<SequentialState> states {};
      std::vector<SequentialOutput> outputs {};

      bool isValid() const {
        return clockedOn.isValid() && !states.empty() && !outputs.empty();
      }
    };

    //In case the Timing Modeling of a design depends on a parameter
    //Following method must be called prior to others
    static void setParameter(SNLDesign* design, const std::string& name, const std::string& defaultValue);
    // Large Cartesian products are retained as groups, without allocating an
    // entry for every pair. Queries enumerate the same ordered dependencies.
    static void addCombinatorialArcs(const BitTerms& inputs, const BitTerms& outputs);
    static void addCombinatorialArcs(const std::string& parameterValue, const BitTerms& inputs, const BitTerms& outputs);
    static void addInputsToClockArcs(const BitTerms& inputs, SNLBitTerm* clock);
    static void addClockToOutputsArcs(SNLBitTerm* clock, const BitTerms& outputs);
    static NajaCollection<SNLBitTerm*> getCombinatorialOutputs(SNLBitTerm* input);
    static NajaCollection<SNLBitTerm*> getCombinatorialInputs(SNLBitTerm* output);
    static NajaCollection<SNLInstTerm*> getCombinatorialOutputs(SNLInstTerm* iinput);
    static NajaCollection<SNLInstTerm*> getCombinatorialInputs(SNLInstTerm* ioutput);
    static NajaCollection<SNLBitTerm*> getOutputRelatedClocks(SNLBitTerm* output);
    static NajaCollection<SNLBitTerm*> getInputRelatedClocks(SNLBitTerm* input);
    static NajaCollection<SNLBitTerm*> getClockRelatedOutputs(SNLBitTerm* clock);
    static NajaCollection<SNLBitTerm*> getClockRelatedInputs(SNLBitTerm* clock);
    static NajaCollection<SNLInstTerm*> getOutputRelatedClocks(SNLInstTerm* ioutput);
    static NajaCollection<SNLInstTerm*> getInputRelatedClocks(SNLInstTerm* iinput);
    static NajaCollection<SNLInstTerm*> getClockRelatedOutputs(SNLInstTerm* iclock);
    static NajaCollection<SNLInstTerm*> getClockRelatedInputs(SNLInstTerm* iclock);
    static void setTermRole(SNLBitTerm* term, SNLTermRole role,
        SNLActiveLevel activeLevel = SNLActiveLevel::NA);
    // Entries override static roles for the listed terms. An empty TermRole
    // (Other, NA) explicitly removes a control role. Omitted terms keep their
    // static role. Selectors use numeric defaults/overrides, like sequential models.
    using TermRoles = std::map<SNLBitTerm*, TermRole, SNLBitTerm::InDesignLess>;
    using TermRoleTable = std::map<std::vector<uint64_t>, TermRoles>;
    static void setRolesFromParameters(SNLDesign* design,
        const std::vector<std::string>& parameters, const TermRoleTable& roles);
    static bool hasRolesFromParameters(const SNLDesign* design);
    static SNLTermRole getTermRole(const SNLBitTerm* term);
    static SNLTermRole getTermRole(const SNLInstTerm* term);
    static SNLActiveLevel getResetActiveLevel(const SNLBitTerm* term);
    static SNLActiveLevel getResetActiveLevel(const SNLInstTerm* term);
    static bool isClock(const SNLBitTerm* term);
    static bool isClock(const SNLInstTerm* term);
    static bool isAsyncReset(const SNLBitTerm* term);
    static bool isAsyncReset(const SNLInstTerm* term);
    static bool isAsyncSet(const SNLBitTerm* term);
    static bool isAsyncSet(const SNLInstTerm* term);
    static bool isSyncReset(const SNLBitTerm* term);
    static bool isSyncReset(const SNLInstTerm* term);
    static bool isSyncSet(const SNLBitTerm* term);
    static bool isSyncSet(const SNLInstTerm* term);
    static bool isReset(const SNLBitTerm* term);
    static bool isReset(const SNLInstTerm* term);
    static bool isEnable(const SNLBitTerm* term);
    static bool isEnable(const SNLInstTerm* term);
    static bool isDataInput(const SNLBitTerm* term);
    static bool isDataInput(const SNLInstTerm* term);
    static bool isDataOutput(const SNLBitTerm* term);
    static bool isDataOutput(const SNLInstTerm* term);
    static NajaCollection<SNLBitTerm*> getClockTerms(const SNLDesign* design);
    static NajaCollection<SNLBitTerm*> getAsyncResetTerms(const SNLDesign* design);
    static NajaCollection<SNLBitTerm*> getAsyncSetTerms(const SNLDesign* design);
    static NajaCollection<SNLBitTerm*> getSyncResetTerms(const SNLDesign* design);
    static NajaCollection<SNLBitTerm*> getSyncSetTerms(const SNLDesign* design);
    static NajaCollection<SNLBitTerm*> getDataInputTerms(const SNLDesign* design);
    static NajaCollection<SNLBitTerm*> getOutputTerms(const SNLDesign* design);
    // Attach or query a memory interface on a primitive design. For DB0 memory
    // primitives the interface is synthesized from the existing DB0 signature,
    // and for instances the returned interface is filtered to connected ports.
    static void setMemoryInterface(SNLDesign* design, const MemoryInterface& memInterface);
    static bool hasMemoryInterface(const SNLDesign* design);
    static MemoryInterface getMemoryInterface(const SNLDesign* design);
    static MemoryInterface getMemoryInterface(const SNLInstance* instance);
    static void setSequentialModel(SNLDesign* design, const SequentialModel& model);
    static bool hasSequentialModel(const SNLDesign* design);
    static const SequentialModel& getSequentialModel(const SNLDesign* design);
    // Keys contain unsigned parameter values in the declared parameter order.
    // Names must identify numeric parameters on the primitive. Instance
    // overrides take precedence over defaults; missing required parameters and
    // unmatched keys throw NLException. A table replaces any static model.
    using SequentialModelTable = std::map<std::vector<uint64_t>, SequentialModel>;
    static void setSequentialModelFromParameters(SNLDesign* design,
        const std::vector<std::string>& parameters, const SequentialModelTable& models);
    static bool hasSequentialModelFromParameters(const SNLDesign* design);
    // A static model is returned unchanged. For a table, return the selected
    // stored entry and cache the selection. References survive override changes
    // but not replacement/destruction of the primitive's model declaration.
    static const SequentialModel& getSequentialModel(const SNLInstance* instance);
    // Preserve the existing null-design call despite the instance overload.
    static const SequentialModel& getSequentialModel(std::nullptr_t) {
      return getSequentialModel(static_cast<const SNLDesign*>(nullptr));
    }
    static void invalidateSequentialModelCache(const SNLInstance* instance);

    static void setTruthTable(SNLDesign* design, const SNLTruthTable& truthTable);
    static void setTruthTables(SNLDesign* design, const std::vector<SNLTruthTable>& truthTable);
    static void setTruthTableFromParameter(
        SNLDesign* design,
        SNLBitTerm* output,
        const BitTerms& inputs,
        SNLParameter* parameter,
        size_t parameterBitOffset = 0);
    static SNLTruthTable getTruthTable(const SNLDesign* design);
    static SNLTruthTable getTruthTable(const SNLDesign* design, size_t flatTermID);
    static SNLTruthTable getTruthTable(const SNLInstance* instance);
    static SNLTruthTable getTruthTable(
        const SNLInstance* instance, size_t flatTermID);
    static bool hasTruthTableFromParameter(
        const SNLDesign* design, size_t flatTermID);
    static void invalidateTruthTableCache(const SNLInstance* instance);
    static bool hasModeling(const SNLDesign* design);
    static bool isSequential(const SNLDesign* design);
    static bool isConst0(const SNLDesign* design);
    static bool isConst1(const SNLDesign* design);
    static bool isConst(const SNLDesign* design);
    static bool isInv(const SNLDesign* design);
    static bool isBuf(const SNLDesign* design);
    static bool isAnd(const SNLDesign* design);
    static bool isNand(const SNLDesign* design);
    static bool isOr(const SNLDesign* design);
    static bool isNor(const SNLDesign* design);
    static bool isXor(const SNLDesign* design);
    static bool isXnor(const SNLDesign* design);
    static bool isMux(const SNLDesign* design);
    static size_t getTruthTableCount(const SNLDesign* design);
    static bool areDependenciesDefined(const SNLBitTerm* term);
    SNLDesignModeling(Type type);
    Type getType() const { return type_; }
  private:
    
    void addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output);
    void addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output, const std::string& parameterValue);
    void addCombinatorialArcs_(const BitTerms& inputs, const BitTerms& outputs,
        const std::string& parameterValue = std::string());
    void addInputToClockArc_(SNLBitTerm* input, SNLBitTerm* clock);
    void addClockToOutputArc_(SNLBitTerm* clock, SNLBitTerm* output);
    const TimingArcs* getTimingArcs(const SNLInstance* instance=nullptr) const;
    TimingArcs* getOrCreateTimingArcs(const std::string& parameterValue=std::string());
    NajaCollection<SNLBitTerm*> getCombinatorialOutputs_(SNLBitTerm* input) const;
    NajaCollection<SNLBitTerm*> getCombinatorialInputs_(SNLBitTerm* output) const;
    NajaCollection<SNLInstTerm*> getCombinatorialOutputs_(SNLInstTerm* iinput) const;
    NajaCollection<SNLInstTerm*> getCombinatorialInputs_(SNLInstTerm* ioutput) const;
    NajaCollection<SNLBitTerm*> getClockRelatedOutputs_(SNLBitTerm* clock) const;
    NajaCollection<SNLBitTerm*> getClockRelatedInputs_(SNLBitTerm* clock) const;
    NajaCollection<SNLBitTerm*> getOutputRelatedClocks_(SNLBitTerm* output) const;
    NajaCollection<SNLBitTerm*> getInputRelatedClocks_(SNLBitTerm* input) const;
    NajaCollection<SNLInstTerm*> getClockRelatedOutputs_(SNLInstTerm* iclock) const;
    NajaCollection<SNLInstTerm*> getClockRelatedInputs_(SNLInstTerm* iclock) const;
    NajaCollection<SNLInstTerm*> getOutputRelatedClocks_(SNLInstTerm* ioutput) const;
    NajaCollection<SNLInstTerm*> getInputRelatedClocks_(SNLInstTerm* iinput) const;
    void setMemoryInterface_(const MemoryInterface& memInterface) { memoryInterface_ = memInterface; }
    bool hasMemoryInterface_() const { return memoryInterface_.has_value(); }
    MemoryInterface getMemoryInterface_() const { return *memoryInterface_; }
    void setSequentialModel_(const SequentialModel& model) {
      sequentialParameters_.clear();
      sequentialModels_.clear();
      instanceSequentialModels_.clear();
      sequentialModel_ = model;
    }
    bool hasSequentialModel_() const {
      return sequentialModel_.has_value() || !sequentialModels_.empty();
    }
    const SequentialModel& getSequentialModel_() const { return *sequentialModel_; }
    void setTermRole_(SNLBitTerm* term, SNLTermRole role, SNLActiveLevel activeLevel) {
      termRoles_[term] = {role, activeLevel};
    }
    TermRole getTermRole_(const SNLBitTerm* term) const {
      auto it = termRoles_.find(const_cast<SNLBitTerm*>(term));
      return it == termRoles_.end() ? TermRole{} : it->second;
    }
    TermRole getTermRole_(const SNLInstTerm* term) const;
    std::vector<std::string> roleParameters_ {};
    TermRoleTable parameterRoles_ {};
    struct ParameterTruthTable {
      SNLParameter* parameter {nullptr};
      size_t parameterBitOffset {0};
      SNLTruthTable defaultTruthTable {};
    };
    void setTruthTableFromParameter_(
        size_t flatTermID, const ParameterTruthTable& truthTable);
    const ParameterTruthTable* getTruthTableFromParameter_(
        size_t flatTermID) const;
    SNLTruthTable getTruthTable_(
        const SNLInstance* instance, size_t flatTermID) const;
    void invalidateTruthTableCache_(const SNLInstance* instance) const;
    Type          type_       { NO_PARAMETER };
    Parameter     parameter_  {};
    TimingModel   model_      {};
    std::optional<MemoryInterface> memoryInterface_ {};
    std::optional<SequentialModel> sequentialModel_ {};
    std::vector<std::string> sequentialParameters_ {};
    SequentialModelTable sequentialModels_ {};
    struct CachedSequentialModel {
      std::vector<std::string> values {};
      const SequentialModel* model {nullptr};
    };
    mutable std::map<const SNLInstance*, CachedSequentialModel> instanceSequentialModels_ {};
    mutable std::mutex instanceSequentialModelsMutex_ {};
    std::map<SNLBitTerm*, TermRole, SNLBitTerm::InDesignLess> termRoles_ {};
    std::map<size_t, ParameterTruthTable> parameterTruthTables_ {};
    mutable std::map<const SNLInstance*, std::map<size_t, SNLTruthTable>>
        instanceTruthTables_ {};
    mutable std::mutex instanceTruthTablesMutex_ {};
};

}  // namespace naja::NL
