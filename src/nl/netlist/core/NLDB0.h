// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include "NLID.h"
#include "SNLTruthTable.h"

namespace naja::NL {

class NLName;
class NLUniverse;
class NLDB;
class NLLibrary;
class SNLDesign;
class SNLInstance;
class SNLTerm;
class SNLScalarTerm;
class SNLBusTerm;

/**
 * \brief NLDB0 is a NLDB automatically managed by NLUniverse.
 *
 * NLDB0 has the following characteristics:
 *   - it is the first NLDB of NLUniverse (NLID::DBID=0)
 *   - it is created automatically and only with NLUniverse creation
 *   - it is deleted automatically and only with NLUniverse deletion
 *   - it holds special NLLibrary with special primitives such as assign,....
 *   - Those special components are accesssible through static methods available in NLUniverse 
 */
class NLDB0 {
  friend class NLUniverse;
  public:
    // Snapshot compatibility version for canonical primitive library identities.
    static constexpr unsigned LibraryIDVersion = 3;

    enum class MemoryResetMode {
      None,
      AsyncLow,
      AsyncHigh,
      SyncLow,
      SyncHigh
    };

    /**
     * \brief Canonical storage form for resolved bit-vector parameter values.
     *
     * A canonical bit-vector value is stored as a Verilog-style sized literal:
     *   "<width>'b<msb...lsb>"           (unsigned)
     *   "<width>'sb<msb...lsb>"          (signed)
     * where the digits are exactly <width> lowercase characters in
     * {0, 1, x, z}, ordered MSB to LSB, with no separators.
     *
     * The stored value represents the literal itself — its own bits, sized to
     * <width> — and NOT the result of a destination-type conversion, which
     * frontends generally cannot know. A consumer applying the value at a
     * different width W must therefore size it the way a Verilog expression
     * context would:
     *   - W < width: keep the rightmost W digits (truncate on the left),
     *   - W > width, unsigned: pad on the left with '0',
     *   - W > width, signed: replicate the leftmost digit (which may be x/z).
     *
     * Not every SNLParameter::Type::Binary value is canonical: frontends only
     * produce this form for values they can resolve faithfully (e.g. sized,
     * non-negative Verilog based literals, or fully evaluated flop INIT
     * bits). Parameter defaults, plain integers, unsized literals ('hF),
     * negated literals (-4'b1, whose value depends on the evaluation context
     * width), concatenations, identifiers, and values built through direct
     * API calls keep their source or caller-provided form. Consumers must
     * therefore treat canonical form as a convention to detect, not a
     * guarantee.
     *
     * formatBitVectorValue validates its input and throws NLException if the
     * digits do not form exactly <width> 0/1/x/z characters.
     */
    static std::string formatBitVectorValue(
      size_t width,
      std::string_view msbToLsbDigits,
      bool isSigned=false);
    /**
     * \brief Format a canonical DFF INIT literal.
     *
     * Equivalent to formatBitVectorValue with isSigned=false; kept under this
     * name for the DB0 sequential primitives whose INIT parameter follows the
     * canonical bit-vector contract.
     */
    static std::string formatDFFInitValue(size_t width, std::string_view msbToLsbDigits);
    static std::string getUndefinedDFFInitValue(size_t width);

    struct MemorySignature {
      size_t          width      {0};
      size_t          depth      {0};
      size_t          abits      {0};
      size_t          readPorts  {0};
      /// Zero creates a read-only memory, without clock/reset/write terminals.
      size_t          writePorts {0};
      MemoryResetMode resetMode  {MemoryResetMode::None};
      /// Independent write clocks and per-bit masks; reads remain asynchronous.
      bool independentWriteClocks {false};

      bool operator==(const MemorySignature& other) const {
        return width == other.width &&
               depth == other.depth &&
               abits == other.abits &&
               readPorts == other.readPorts &&
               writePorts == other.writePorts &&
               resetMode == other.resetMode &&
               independentWriteClocks == other.independentWriteClocks;
      }
    };

    struct DivModSignature {
      size_t width {0};
      bool   isSigned {false};

      bool operator==(const DivModSignature& other) const {
        return width == other.width && isSigned == other.isSigned;
      }
    };

    /// Count is an unsigned COUNT_WIDTH-bit value. Oversized counts produce
    /// zero, or the original sign bit for arithmetic right shifts.
    struct ShiftSignature {
      size_t width {0};
      size_t countWidth {0};
      bool right {false};
      bool arithmetic {false};

      bool operator==(const ShiftSignature& other) const {
        return width == other.width && countWidth == other.countWidth &&
               right == other.right && arithmetic == other.arithmetic;
      }
    };

    static SNLDesign* getOrCreateShift(const ShiftSignature& signature);
    static bool isShift(const SNLDesign* design);
    static ShiftSignature getShiftSignature(const SNLDesign* design);
    static ShiftSignature getShiftSignature(const SNLInstance* instance);
    static SNLBusTerm* getShiftData(const SNLDesign* design);
    static SNLBusTerm* getShiftCount(const SNLDesign* design);
    static SNLBusTerm* getShiftOutput(const SNLDesign* design);

    /// Multiply two WIDTH-bit vectors modulo 2**WIDTH. Frontends must first
    /// apply language-specific operand sizing and sign extension; the retained
    /// low product bits then have the same signed and unsigned interpretation.
    static SNLDesign* getOrCreateMultiply(size_t width);
    static bool isMultiply(const SNLDesign* design);
    static size_t getMultiplyWidth(const SNLDesign* design);
    static size_t getMultiplyWidth(const SNLInstance* instance);
    static SNLBusTerm* getMultiplyInputA(const SNLDesign* design);
    static SNLBusTerm* getMultiplyInputB(const SNLDesign* design);
    static SNLBusTerm* getMultiplyOutput(const SNLDesign* design);

    /// Add or subtract WIDTH-bit operands modulo 2**WIDTH. The operation is
    /// fixed by the canonical model; frontends apply operand sizing first.
    static SNLDesign* getOrCreateAddSub(size_t width, bool subtract);
    static bool isAddSub(const SNLDesign* design);
    static bool isSubtract(const SNLDesign* design);
    static size_t getAddSubWidth(const SNLDesign* design);
    static size_t getAddSubWidth(const SNLInstance* instance);
    static SNLBusTerm* getAddSubInputA(const SNLDesign* design);
    static SNLBusTerm* getAddSubInputB(const SNLDesign* design);
    static SNLBusTerm* getAddSubOutput(const SNLDesign* design);

    struct TableSelectSignature {
      size_t width {0};
      size_t depth {0};
      size_t abits {0};

      bool operator==(const TableSelectSignature& other) const {
        return width == other.width &&
               depth == other.depth &&
               abits == other.abits;
      }
    };

    struct VectorUpdateSignature {
      size_t width{0};
      size_t updateWidth{0};
      size_t abits{0};
      bool isSigned{false};
      bool operator==(const VectorUpdateSignature&) const = default;
    };
    /// Combinational clipped bit-vector update; no frontend or state ownership.
    static SNLDesign* getOrCreateVectorUpdate(const VectorUpdateSignature& signature);
    static bool isVectorUpdate(const SNLDesign* design);
    static VectorUpdateSignature getVectorUpdateSignature(const SNLDesign* design);
    static VectorUpdateSignature getVectorUpdateSignature(const SNLInstance* instance);
    static SNLBusTerm* getVectorUpdateBase(const SNLDesign* design);
    static SNLBusTerm* getVectorUpdateData(const SNLDesign* design);
    static SNLBusTerm* getVectorUpdateOffset(const SNLDesign* design);
    static SNLScalarTerm* getVectorUpdateEnable(const SNLDesign* design);
    static SNLBusTerm* getVectorUpdateOutput(const SNLDesign* design);
    static SNLTruthTable getVectorUpdateTruthTable(const SNLDesign* design, size_t flatTermID);

    struct VectorSelectSignature {
      size_t width{0};
      size_t selectWidth{0};
      size_t abits{0};
      bool isSigned{false};
      bool operator==(const VectorSelectSignature&) const = default;
    };
    static SNLDesign* getOrCreateVectorSelect(const VectorSelectSignature& signature);
    static bool isVectorSelect(const SNLDesign* design);
    static VectorSelectSignature getVectorSelectSignature(const SNLDesign* design);
    static VectorSelectSignature getVectorSelectSignature(const SNLInstance* instance);
    static SNLBusTerm* getVectorSelectData(const SNLDesign* design);
    static SNLBusTerm* getVectorSelectOffset(const SNLDesign* design);
    static SNLBusTerm* getVectorSelectOutput(const SNLDesign* design);
    static SNLTruthTable getVectorSelectTruthTable(const SNLDesign* design, size_t flatTermID);

    class GateType {
      public:
        enum GateTypeEnum {
          And, Nand, Or, Nor, Xor, Xnor, Buf, Not, Unknown
        };
        GateType() = default;
        GateType(const std::string& name);
        GateType(const GateTypeEnum& gateTypeEnum);
        GateType(const GateType&) = default;
        GateType(GateType&&) = default;
        GateType& operator=(const GateType&) = default;
        operator const GateTypeEnum&() const {return gateTypeEnum_;}
        std::string getString() const;
        bool isNInput() const;
        bool isNOutput() const;
      private:
        GateTypeEnum  gateTypeEnum_;
    };

    static NLDB* getDB0();
    static bool isDB0(const NLDB* db);
    static NLLibrary* getDB0RootLibrary();
    static bool isDB0Library(const NLLibrary* library);
    static bool isDB0Primitive(const SNLDesign* design);
    /// \brief True only for fully managed memory primitives created by NLDB0.
    ///
    /// Liberty/Python clients can still attach a generic memory interface to
    /// their own primitives with SNLDesignModeling; those designs deliberately
    /// remain outside NLDB0 ownership and must not be reported here.
    static bool isMemory(const SNLDesign* design);
    static MemorySignature getMemorySignature(const SNLDesign* design);
    static MemorySignature getMemorySignature(const SNLInstance* instance);
    static SNLScalarTerm* getMemoryClock(const SNLDesign* design);
    static SNLScalarTerm* getMemoryReset(const SNLDesign* design);
    static SNLBusTerm* getMemoryReadAddress(const SNLDesign* design);
    static SNLBusTerm* getMemoryReadData(const SNLDesign* design);
    static SNLBusTerm* getMemoryWriteAddress(const SNLDesign* design);
    static SNLBusTerm* getMemoryWriteData(const SNLDesign* design);
    static SNLBusTerm* getMemoryWriteEnable(const SNLDesign* design);
    static bool isDivMod(const SNLDesign* design);
    static DivModSignature getDivModSignature(const SNLDesign* design);
    static DivModSignature getDivModSignature(const SNLInstance* instance);
    static SNLBusTerm* getDivModDividend(const SNLDesign* design);
    static SNLBusTerm* getDivModDivisor(const SNLDesign* design);
    static SNLBusTerm* getDivModQuotient(const SNLDesign* design);
    static SNLBusTerm* getDivModRemainder(const SNLDesign* design);
    static SNLTruthTable getDivModTruthTable(
      const SNLDesign* design,
      size_t flatTermID);
    /// Generic truth table of one output bit of an add/subtract, multiply
    /// or shift primitive; flatTermID is that bit's order ID.
    static SNLTruthTable getWordTruthTable(
      const SNLDesign* design,
      size_t flatTermID);
    /// Generic ROM truth table of one read-data bit of a read-only memory
    /// (no write ports); the instance INIT parameter supplies the contents.
    /// Uninitialized when INIT is disabled or holds x/z digits.
    static SNLTruthTable getRomTruthTable(
      const SNLInstance* instance,
      size_t flatTermID);
    static bool isTableSelect(const SNLDesign* design);
    static TableSelectSignature getTableSelectSignature(const SNLDesign* design);
    static TableSelectSignature getTableSelectSignature(const SNLInstance* instance);
    static SNLBusTerm* getTableSelectData(const SNLDesign* design);
    static SNLBusTerm* getTableSelectAddress(const SNLDesign* design);
    static SNLBusTerm* getTableSelectOutput(const SNLDesign* design);
    static SNLTruthTable getTableSelectTruthTable(const SNLDesign* design, size_t flatTermID);

    static SNLTruthTable getPrimitiveTruthTable(const SNLDesign* design);

    /// \brief Canonical model parameters (name -> default value), used to
    /// rebuild multi-parameter primitives from serialized DB0 interfaces.
    using PrimitiveParameters = std::map<std::string, std::string>;
    /// \brief Resolve, lazily (re)creating if needed, the DB0 primitive a
    /// serialized model reference points to.
    ///
    /// DB0 (NLID::DBID 0) primitive IDs are canonical: \p libraryID selects the
    /// primitive family and \p designID encodes the single size parameter
    /// (gate fan-in/out, mux/dff/divmod/multiply/add/subtract width).
    /// Multi-parameter families (memory, table select, shift, vector update) are rebuilt from
    /// \p parameters, which must carry the full model signature independently
    /// of instance overrides. Returns nullptr if the
    /// reference does not designate a known DB0 primitive family.
    static SNLDesign* getOrCreatePrimitive(
      NLID::LibraryID libraryID,
      NLID::DesignID designID,
      const PrimitiveParameters& parameters = {});

    static SNLDesign* getAssign();
    static bool isAssign(const SNLDesign* design);
    static SNLScalarTerm* getAssignInput();
    static SNLScalarTerm* getAssignOutput();
    static SNLDesign* getFA();
    static bool isFA(const SNLDesign* design);
    static SNLScalarTerm* getFAInputA();
    static SNLScalarTerm* getFAInputB();
    static SNLScalarTerm* getFAInputCI();
    static SNLScalarTerm* getFAOutputS();
    static SNLScalarTerm* getFAOutputCO();
    ///\return truth table for Sum output: A XOR B XOR CI (3 inputs, bits=0x96)
    static SNLTruthTable getFASumTruthTable();
    ///\return truth table for Carry-out output: majority(A,B,CI) (3 inputs, bits=0xE8)
    static SNLTruthTable getFACoutTruthTable();
    static SNLDesign* getOrCreateMux2(size_t width);
    static SNLDesign* getMux2();
    static bool isMux2(const SNLDesign* design);
    static SNLBusTerm* getMux2InputA(const SNLDesign* mux2);
    static SNLBusTerm* getMux2InputA();
    static SNLBusTerm* getMux2InputB(const SNLDesign* mux2);
    static SNLBusTerm* getMux2InputB();
    static SNLScalarTerm* getMux2Select(const SNLDesign* mux2);
    static SNLScalarTerm* getMux2Select();
    static SNLBusTerm* getMux2Output(const SNLDesign* mux2);
    static SNLBusTerm* getMux2Output();
    static SNLDesign* getDFF();
    static SNLDesign* getOrCreateDFF(size_t width);
    static bool isDFF(const SNLDesign* design);
    /// \brief Plain edge-triggered D flip-flop.
    /// Pins: C (clock), D (data), Q (output).
    static SNLScalarTerm* getDFFClock();
    static SNLScalarTerm* getDFFData();
    static SNLScalarTerm* getDFFOutput();
    /// \brief Plain transparent-high D latch.
    /// Pins: E (enable), D (data), Q (output).
    static SNLDesign* getDLatch();
    static SNLDesign* getOrCreateDLatch(size_t width);
    static bool isDLatch(const SNLDesign* design);
    static SNLScalarTerm* getDLatchEnable();
    static SNLScalarTerm* getDLatchData();
    static SNLScalarTerm* getDLatchOutput();
    /// \brief Plain negative-edge-triggered D flip-flop.
    /// Pins: C (clock), D (data), Q (output).
    static SNLDesign* getDFFN();
    static SNLDesign* getOrCreateDFFN(size_t width);
    static bool isDFFN(const SNLDesign* design);
    static SNLScalarTerm* getDFFNClock();
    static SNLScalarTerm* getDFFNData();
    static SNLScalarTerm* getDFFNOutput();
    /// \brief Edge-triggered D flip-flop with active-low asynchronous reset.
    /// Pins: C (clock), D (data), RN (reset low active), Q (output).
    static SNLDesign* getDFFRN();
    static SNLDesign* getOrCreateDFFRN(size_t width);
    static bool isDFFRN(const SNLDesign* design);
    static SNLScalarTerm* getDFFRNClock();
    static SNLScalarTerm* getDFFRNData();
    static SNLScalarTerm* getDFFRNResetN();
    static SNLScalarTerm* getDFFRNOutput();
    /// \brief Edge-triggered D flip-flop with active-high asynchronous reset.
    /// Pins: C (clock), D (data), R (reset high active), Q (output).
    static SNLDesign* getDFFR();
    static SNLDesign* getOrCreateDFFR(size_t width);
    static bool isDFFR(const SNLDesign* design);
    static SNLScalarTerm* getDFFRClock();
    static SNLScalarTerm* getDFFRData();
    static SNLScalarTerm* getDFFRReset();
    static SNLScalarTerm* getDFFROutput();
    /// \brief Edge-triggered D flip-flop with clock enable.
    /// Pins: C (clock), D (data), E (enable), Q (output).
    static SNLDesign* getDFFE();
    static SNLDesign* getOrCreateDFFE(size_t width);
    static bool isDFFE(const SNLDesign* design);
    static SNLScalarTerm* getDFFEClock();
    static SNLScalarTerm* getDFFEData();
    static SNLScalarTerm* getDFFEEnable();
    static SNLScalarTerm* getDFFEOutput();
    /// \brief Edge-triggered D flip-flop with clock enable and asynchronous reset.
    /// Pins: C (clock), D (data), E (enable), R (reset), Q (output).
    static SNLDesign* getDFFRE();
    static SNLDesign* getOrCreateDFFRE(size_t width);
    static bool isDFFRE(const SNLDesign* design);
    static SNLScalarTerm* getDFFREClock();
    static SNLScalarTerm* getDFFREData();
    static SNLScalarTerm* getDFFREEnable();
    static SNLScalarTerm* getDFFREReset();
    static SNLScalarTerm* getDFFREOutput();
    /// \brief Edge-triggered D flip-flop with clock enable and asynchronous set.
    /// Pins: C (clock), D (data), E (enable), S (set), Q (output).
    static SNLDesign* getDFFSE();
    static SNLDesign* getOrCreateDFFSE(size_t width);
    static bool isDFFSE(const SNLDesign* design);
    static SNLScalarTerm* getDFFSEClock();
    static SNLScalarTerm* getDFFSEData();
    static SNLScalarTerm* getDFFSEEnable();
    static SNLScalarTerm* getDFFSESet();
    static SNLScalarTerm* getDFFSEOutput();
    /// \brief Edge-triggered D flip-flop with active-high asynchronous set.
    /// Pins: C (clock), D (data), S (set high active), Q (output).
    static SNLDesign* getDFFS();
    static SNLDesign* getOrCreateDFFS(size_t width);
    static bool isDFFS(const SNLDesign* design);
    static SNLScalarTerm* getDFFSClock();
    static SNLScalarTerm* getDFFSData();
    static SNLScalarTerm* getDFFSSet();
    static SNLScalarTerm* getDFFSOutput();
    /// \brief Positive-edge D flip-flop with active-high synchronous reset.
    /// Pins: C (clock), D (data), R (sync reset high active), Q (output).
    static SNLDesign* getDFFSR();
    static SNLDesign* getOrCreateDFFSR(size_t width);
    static bool isDFFSR(const SNLDesign* design);
    static SNLScalarTerm* getDFFSRClock();
    static SNLScalarTerm* getDFFSRData();
    static SNLScalarTerm* getDFFSRReset();
    static SNLScalarTerm* getDFFSROutput();
    /// \brief Positive-edge D flip-flop with active-low synchronous reset.
    /// Pins: C (clock), D (data), RN (sync reset low active), Q (output).
    static SNLDesign* getDFFSRN();
    static SNLDesign* getOrCreateDFFSRN(size_t width);
    static bool isDFFSRN(const SNLDesign* design);
    static SNLScalarTerm* getDFFSRNClock();
    static SNLScalarTerm* getDFFSRNData();
    static SNLScalarTerm* getDFFSRNResetN();
    static SNLScalarTerm* getDFFSRNOutput();
    /// \brief Positive-edge D flip-flop with active-high synchronous set.
    /// Pins: C (clock), D (data), S (sync set high active), Q (output).
    static SNLDesign* getDFFSS();
    static SNLDesign* getOrCreateDFFSS(size_t width);
    static bool isDFFSS(const SNLDesign* design);
    static SNLScalarTerm* getDFFSSClock();
    static SNLScalarTerm* getDFFSSData();
    static SNLScalarTerm* getDFFSSSet();
    static SNLScalarTerm* getDFFSSOutput();
    /// \brief Positive-edge D flip-flop with active-low synchronous set.
    /// Pins: C (clock), D (data), SN (sync set low active), Q (output).
    static SNLDesign* getDFFSSN();
    static SNLDesign* getOrCreateDFFSSN(size_t width);
    static bool isDFFSSN(const SNLDesign* design);
    static SNLScalarTerm* getDFFSSNClock();
    static SNLScalarTerm* getDFFSSNData();
    static SNLScalarTerm* getDFFSSNSetN();
    static SNLScalarTerm* getDFFSSNOutput();
    static SNLDesign* getDFFSRE();
    static SNLDesign* getOrCreateDFFSRE(size_t width);
    static bool isDFFSRE(const SNLDesign* design);
    static SNLDesign* getDFFSRNE();
    static SNLDesign* getOrCreateDFFSRNE(size_t width);
    static bool isDFFSRNE(const SNLDesign* design);
    static SNLDesign* getDFFSSE();
    static SNLDesign* getOrCreateDFFSSE(size_t width);
    static bool isDFFSSE(const SNLDesign* design);
    static SNLDesign* getDFFSSNE();
    static SNLDesign* getOrCreateDFFSSNE(size_t width);
    static bool isDFFSSNE(const SNLDesign* design);
    static NLLibrary* getGateLibrary(const GateType& type);
    static bool isGateLibrary(const NLLibrary* lib);
    static NLLibrary* getOrCreateGateLibrary(const GateType& type);
    static SNLDesign* getOrCreateNOutputGate(const GateType& type, size_t nbOutputs);
    static SNLDesign* getOrCreateNInputGate(const GateType& type, size_t nbInputs);
    static bool isGate(const SNLDesign* design);
    static std::string getGateName(const SNLDesign* design);
    static bool isNInputGate(const SNLDesign* design);
    static bool isNOutputGate(const SNLDesign* design);
    ///\return the single terminal of a N-Gate: output if N-input, input if N-output.
    static SNLScalarTerm* getGateSingleTerm(const SNLDesign* gate);
    ///\return the bus term of size N of a N-Gate: output if N-output, input if N-input.
    static SNLBusTerm* getGateNTerms(const SNLDesign* gate);
    /// \brief Create or reuse a DB0 divider/remainder primitive.
    static SNLDesign* getOrCreateDivMod(const DivModSignature& signature);
    /// \brief Create or reuse a fully managed DB0 memory primitive.
    static SNLDesign* getOrCreateMemory(const MemorySignature& signature);
    /// \brief Create or reuse a compact combinational table-select primitive.
    static SNLDesign* getOrCreateTableSelect(const TableSelectSignature& signature);
  private:
    static NLDB* create(NLUniverse* universe);
    static constexpr char RootLibraryName[] { "PRIMITIVES" };
};

}  // namespace naja::NL
