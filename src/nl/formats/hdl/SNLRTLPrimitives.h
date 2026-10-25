// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

namespace naja::NL {

class SNLBitNet;
class SNLDesign;
class SNLInstance;
class SNLNet;

/// Hardware construction after language-specific sizing and process analysis.
/// Inputs must belong to design. Invalid inputs throw NLException before an
/// instance is created. Source links and initialization remain caller-owned.
class SNLRTLPrimitives {
  public:
    using Bits = std::vector<SNLBitNet*>;
    enum class GateKind { And, Nand, Or, Nor, Xor, Xnor, Buf, Not };

    /// Clipped combinational update. Bits are LSB first; unknown offset or
    /// enable other than 1 leaves BASE unchanged. Caller normalizes language
    /// bounds and scheduling. Signed offsets may partially overlap bit zero.
    static SNLInstance* createVectorUpdate(
      SNLDesign* design, const Bits& base, const Bits& data, const Bits& offset,
      bool isSigned, SNLBitNet* enable, SNLNet* output);

    /// Read a bit window at a full signed/unsigned offset. Each out-of-range
    /// output bit is X; unknown offset makes the entire result X. Bits are LSB first.
    static SNLInstance* createVectorSelect(
      SNLDesign* design, const Bits& data, const Bits& offset,
      bool isSigned, SNLNet* output);

    /// Read-only memory with instance-owned INIT contents. Data is word-major,
    /// address zero first, with each word and address least-significant-bit first.
    /// Data bits must be constant 0/1/X/Z; output width determines word width.
    static SNLInstance* createROM(
      SNLDesign* design, const Bits& data, const Bits& address, SNLNet* output);

    /// Input vectors are least-significant-bit first. Select 0 chooses a;
    /// select 1 chooses b. Output is connected by its declared bit order.
    static SNLInstance* createMux(
      SNLDesign* design, SNLBitNet* select,
      const Bits& a, const Bits& b, SNLNet* output);

    /// One positive-edge register without reset or enable. All nets have width 1.
    static SNLInstance* createDFF(
      SNLDesign* design, SNLNet* clock, SNLNet* data, SNLNet* output);

    /// One positive-edge register with an active-high clock enable. All nets
    /// have width 1.
    static SNLInstance* createDFFE(
      SNLDesign* design, SNLNet* clock, SNLNet* data,
      SNLNet* enable, SNLNet* output);

    /// One positive-edge register with an active-high synchronous reset to zero.
    /// All nets have width 1.
    static SNLInstance* createDFFSR(
      SNLDesign* design, SNLNet* clock, SNLNet* data,
      SNLNet* reset, SNLNet* output);

    /// One positive-edge register with active-high clock enable and active-high
    /// synchronous reset to zero. Reset has priority over enable. All nets have
    /// width 1.
    static SNLInstance* createDFFSRE(
      SNLDesign* design, SNLNet* clock, SNLNet* data,
      SNLNet* enable, SNLNet* reset, SNLNet* output);

    /// Create a canonical scalar logic gate. Buf and Not take exactly one input;
    /// all other kinds take at least one. Every net must have width one and belong
    /// to design. Invalid inputs throw before an instance is created.
    static SNLInstance* createGate(
      SNLDesign* design, GateKind kind,
      const std::vector<SNLNet*>& inputs, SNLNet* output);

    /// Connect matching hardware bit positions through canonical DB0 Assigns.
    /// Input and output must belong to design and have the same nonzero width.
    static std::vector<SNLInstance*> createAssign(
      SNLDesign* design, SNLNet* input, SNLNet* output);

    /// Apply a canonical gate independently at every hardware bit position.
    /// Vector position zero is the rightmost declared source bit. Inputs and
    /// output must have the same nonzero width.
    static std::vector<SNLInstance*> createBitwiseGate(
      SNLDesign* design, GateKind kind,
      const std::vector<SNLNet*>& inputs, SNLNet* output);
};

}  // namespace naja::NL
