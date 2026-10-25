// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "NLBitVecDynamic.h"
#include "NLException.h"
#include "NLBitDependencies.h"

namespace naja::NL {

class SNLTruthTable {
 public:
  // Enum for generic type for N input truth table.
  enum class GenericType {
    NONE,
    OR,
    NOR,
    AND,
    NAND,
    XOR,
    XNOR,
    TABLE_SELECT,
    DIVMOD,
    ADD_SUB,
    MULTIPLY,
    SHIFT,
    ROM,
    VECTOR_UPDATE,
    VECTOR_SELECT
  };

  enum class DivModResult {
    QUOTIENT,
    REMAINDER
  };

  // Metadata of one output bit of a word arithmetic primitive (ADD_SUB,
  // MULTIPLY, SHIFT). The dependencies list the first operand's bits then
  // the second's, most significant bit first, as the NLDB0 primitive
  // declares them; countWidth, right and arithmetic are SHIFT only.
  struct WordMetadata {
    uint32_t width{0};
    uint32_t outputBit{0};
    uint32_t countWidth{0};
    bool subtract{false};
    bool right{false};
    bool arithmetic{false};

    bool operator==(const WordMetadata&) const = default;
    bool operator<(const WordMetadata& o) const {
      return std::tie(width, outputBit, countWidth, subtract, right, arithmetic) <
             std::tie(o.width, o.outputBit, o.countWidth, o.subtract, o.right, o.arithmetic);
    }
  };
  
  // Metadata of one output bit of a read-only memory (ROM): a table select
  // whose rows are constants. The dependencies are the address bits, most
  // significant bit first; column holds this bit of every row, row 0 first.
  // Addresses at or beyond depth read 0.
  struct RomMetadata {
    uint32_t addressSize{0};
    uint32_t depth{0};
    std::vector<bool> column{};

    bool operator==(const RomMetadata&) const = default;
    bool operator<(const RomMetadata& o) const {
      return std::tie(addressSize, depth, column) <
             std::tie(o.addressSize, o.depth, o.column);
    }
  };

  // Dependencies: BASE[outputBit], DATA MSB first, OFFSET MSB first, EN.
  struct VectorUpdateMetadata {
    uint32_t width{0};
    uint32_t updateWidth{0};
    uint32_t addressSize{0};
    uint32_t outputBit{0};
    bool isSigned{false};
    bool operator==(const VectorUpdateMetadata&) const = default;
    bool operator<(const VectorUpdateMetadata& o) const {
      return std::tie(width, updateWidth, addressSize, outputBit, isSigned) <
             std::tie(o.width, o.updateWidth, o.addressSize, o.outputBit, o.isSigned);
    }
  };

  // Dependencies: DATA MSB first, OFFSET MSB first.
  struct VectorSelectMetadata {
    uint32_t width{0};
    uint32_t selectWidth{0};
    uint32_t addressSize{0};
    uint32_t outputBit{0};
    bool isSigned{false};
    bool operator==(const VectorSelectMetadata&) const = default;
    bool operator<(const VectorSelectMetadata& o) const {
      return std::tie(width, selectWidth, addressSize, outputBit, isSigned) <
             std::tie(o.width, o.selectWidth, o.addressSize, o.outputBit, o.isSigned);
    }
  };

  SNLTruthTable() : size_(0) {}

  static std::vector<uint64_t> fullDependencies(size_t size) {
    if (size == 0) {
      return {};
    }
    std::vector<size_t> deps(size);
    for (size_t i = 0; i < size; ++i) {
      deps[i] = i;
    }
    return NLBitDependencies::encodeBits(deps);
  }

  SNLTruthTable(size_t size,
                GenericType genericType,
                const std::vector<uint64_t>& dependencies)
      : size_(size), dependencies_(dependencies), genericType_(genericType) {
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
    validateGenericMetadata();
  }

  SNLTruthTable(size_t size,
                GenericType genericType,
                const std::vector<uint64_t>& dependencies,
                uint32_t tableSelectAddressSize,
                uint32_t tableSelectDepth)
      : size_(size),
        dependencies_(dependencies),
        genericType_(genericType),
        tableSelectAddressSize_(tableSelectAddressSize),
        tableSelectDepth_(tableSelectDepth) {
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
    validateGenericMetadata();
  }

  // user‐provided copy‐ctor
  SNLTruthTable(const SNLTruthTable& o)
    : size_(o.size_),
      bits_(o.bits_),
      dependencies_(o.dependencies_),
      genericType_(o.genericType_),
      tableSelectAddressSize_(o.tableSelectAddressSize_),
      tableSelectDepth_(o.tableSelectDepth_),
      divModWidth_(o.divModWidth_),
      divModSigned_(o.divModSigned_),
      divModResult_(o.divModResult_),
      divModOutputBit_(o.divModOutputBit_),
      word_(o.word_),
      rom_(o.rom_),
      vectorUpdate_(o.vectorUpdate_),
      vectorSelect_(o.vectorSelect_)
  {}

  // explicit copy‐assignment to match
  SNLTruthTable& operator=(const SNLTruthTable& o)
  {
    if (this != &o) {
      size_ = o.size_;
      bits_ = o.bits_;
      dependencies_ = o.dependencies_;
      genericType_ = o.genericType_;
      tableSelectAddressSize_ = o.tableSelectAddressSize_;
      tableSelectDepth_ = o.tableSelectDepth_;
      divModWidth_ = o.divModWidth_;
      divModSigned_ = o.divModSigned_;
      divModResult_ = o.divModResult_;
      divModOutputBit_ = o.divModOutputBit_;
      word_ = o.word_;
      rom_ = o.rom_;
      vectorUpdate_ = o.vectorUpdate_;
      vectorSelect_ = o.vectorSelect_;
    }
    return *this;
  }

  bool isNull() const {
    return size_ == std::numeric_limits<uint32_t>::max();
  }

  // Enforce size ≤ 6 BEFORE touching BitVecDynamic
  explicit SNLTruthTable(uint32_t size, uint64_t bits,
                         const std::vector<uint64_t>& dependencies)
      : dependencies_(dependencies) {
    if (size > 6) {
      std::ostringstream oss;
      oss << "Cannot create SNLTruthTable with bits_: " << bits
          << " and size: " << size << " (max=6)";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = NLBitVecDynamic(bits, 1u << size);
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
  }

  SNLTruthTable(uint32_t size, const std::vector<bool>& bits,
                const std::vector<uint64_t>& dependencies)
      : size_(size), dependencies_(dependencies) {
    if (size <= 6) {
      std::ostringstream oss;
      oss << "Should use mask constructor for 6 or less inputs";
      throw NLException(oss.str());
    }
    size_ = size;
    // now safe: 1<<size <= 64
    bits_ = NLBitVecDynamic(bits, 1u << size);
    if (size > 0 &&
        NLBitDependencies::countBitsForVector(dependencies_) > size) {
      throw NLException("Dependencies count cannot exceed truth table size");
    }
  }

  static SNLTruthTable Logic0() { return SNLTruthTable(0, 0, {}); }
  static SNLTruthTable Logic1() { return SNLTruthTable(0, 1, {}); }
  static SNLTruthTable Inv() { return SNLTruthTable(1, 0b01, fullDependencies(1)); }
  static SNLTruthTable Buf() { return SNLTruthTable(1, 0b10, fullDependencies(1)); }
  static SNLTruthTable TableSelect(
      uint32_t addressSize,
      uint32_t depth,
      const std::vector<uint64_t>& dependencies) {
    return SNLTruthTable(
        static_cast<size_t>(addressSize) + depth,
        GenericType::TABLE_SELECT,
        dependencies,
        addressSize,
        depth);
  }

  static SNLTruthTable DivMod(
      uint32_t width,
      bool isSigned,
      DivModResult result,
      uint32_t outputBit,
      const std::vector<uint64_t>& dependencies) {
    if (width == 0 ||
        width > std::numeric_limits<uint32_t>::max() / 2) {
      throw NLException("Invalid DIVMOD truth table width");
    }
    if (outputBit >= width) {
      throw NLException("DIVMOD truth table output bit is out of range");
    }
    const uint32_t inputCount = width * 2;
    if (NLBitDependencies::countBitsForVector(dependencies) != inputCount) {
      throw NLException("DIVMOD truth table must depend on every operand bit");
    }

    SNLTruthTable table;
    table.size_ = inputCount;
    table.dependencies_ = dependencies;
    table.genericType_ = GenericType::DIVMOD;
    table.divModWidth_ = width;
    table.divModSigned_ = isSigned;
    table.divModResult_ = result;
    table.divModOutputBit_ = outputBit;
    table.validateGenericMetadata();
    return table;
  }

  static SNLTruthTable AddSub(
      uint32_t width,
      bool subtract,
      uint32_t outputBit,
      const std::vector<uint64_t>& dependencies) {
    return Word(GenericType::ADD_SUB,
                {width, outputBit, 0, subtract, false, false}, dependencies);
  }

  static SNLTruthTable Multiply(
      uint32_t width,
      uint32_t outputBit,
      const std::vector<uint64_t>& dependencies) {
    return Word(GenericType::MULTIPLY,
                {width, outputBit, 0, false, false, false}, dependencies);
  }

  static SNLTruthTable Shift(
      uint32_t width,
      uint32_t countWidth,
      bool right,
      bool arithmetic,
      uint32_t outputBit,
      const std::vector<uint64_t>& dependencies) {
    return Word(GenericType::SHIFT,
                {width, outputBit, countWidth, false, right, arithmetic},
                dependencies);
  }

  static SNLTruthTable Word(
      GenericType genericType,
      const WordMetadata& word,
      const std::vector<uint64_t>& dependencies) {
    const uint64_t inputCount =
        static_cast<uint64_t>(word.width) +
        (genericType == GenericType::SHIFT ? word.countWidth : word.width);
    if (inputCount > std::numeric_limits<uint32_t>::max() ||
        NLBitDependencies::countBitsForVector(dependencies) != inputCount) {
      throw NLException("Word arithmetic truth table must depend on every operand bit");
    }
    SNLTruthTable table;
    table.size_ = static_cast<uint32_t>(inputCount);
    table.dependencies_ = dependencies;
    table.genericType_ = genericType;
    table.word_ = word;
    table.validateGenericMetadata();
    return table;
  }

  static SNLTruthTable Rom(
      const RomMetadata& rom,
      const std::vector<uint64_t>& dependencies) {
    if (NLBitDependencies::countBitsForVector(dependencies) != rom.addressSize) {
      throw NLException("ROM truth table must depend on every address bit");
    }
    SNLTruthTable table;
    table.size_ = rom.addressSize;
    table.dependencies_ = dependencies;
    table.genericType_ = GenericType::ROM;
    table.rom_ = rom;
    table.validateGenericMetadata();
    return table;
  }

  static SNLTruthTable VectorUpdate(
      const VectorUpdateMetadata& metadata,
      const std::vector<uint64_t>& dependencies) {
    const uint64_t size = uint64_t(metadata.updateWidth) + metadata.addressSize + 2;
    if (size > std::numeric_limits<uint32_t>::max() ||
        NLBitDependencies::countBitsForVector(dependencies) != size) {
      throw NLException("Invalid VECTOR_UPDATE dependency count");
    }
    SNLTruthTable table;
    table.genericType_ = GenericType::VECTOR_UPDATE;
    table.vectorUpdate_ = metadata;
    table.size_ = size;
    table.dependencies_ = dependencies;
    table.validateGenericMetadata();
    return table;
  }

  const VectorUpdateMetadata& getVectorUpdateMetadata() const {
    if (genericType_ != GenericType::VECTOR_UPDATE)
      throw NLException("Truth table is not a VECTOR_UPDATE generic");
    return vectorUpdate_;
  }

  static SNLTruthTable VectorSelect(
      const VectorSelectMetadata& metadata,
      const std::vector<uint64_t>& dependencies) {
    const uint64_t size = uint64_t(metadata.width) + metadata.addressSize;
    if (size > std::numeric_limits<uint32_t>::max() ||
        NLBitDependencies::countBitsForVector(dependencies) != size) {
      throw NLException("Invalid VECTOR_SELECT dependency count");
    }
    SNLTruthTable table;
    table.genericType_ = GenericType::VECTOR_SELECT;
    table.vectorSelect_ = metadata;
    table.size_ = size;
    table.dependencies_ = dependencies;
    table.validateGenericMetadata();
    return table;
  }

  const VectorSelectMetadata& getVectorSelectMetadata() const {
    if (genericType_ != GenericType::VECTOR_SELECT)
      throw NLException("Truth table is not a VECTOR_SELECT generic");
    return vectorSelect_;
  }

  // nullopt denotes an out-of-range bit, whose four-state value is X.
  std::optional<bool> evaluateVectorSelect(const std::vector<bool>& inputs) const {
    const auto& m = getVectorSelectMetadata();
    if (inputs.size() != size_) throw NLException("VECTOR_SELECT input count mismatch");
    for (uint32_t k = 0; k < m.width; ++k) {
      const int64_t offset = int64_t(k) - m.outputBit;
      if (!m.isSigned && offset < 0) continue;
      if (m.addressSize < 64) {
        if (m.isSigned) {
          const int64_t limit = int64_t{1} << (m.addressSize - 1);
          if (offset < -limit || offset >= limit) continue;
        } else if (uint64_t(offset) >= (uint64_t{1} << m.addressSize)) continue;
      }
      bool matches = true;
      for (uint32_t bit = 0; bit < m.addressSize; ++bit) {
        const bool expected = bit < 64 ? ((uint64_t(offset) >> bit) & 1) : offset < 0;
        if (inputs[m.width + m.addressSize - 1 - bit] != expected) {
          matches = false;
          break;
        }
      }
      if (matches) return inputs[m.width - 1 - k];
    }
    return std::nullopt;
  }

  // Evaluate binary inputs without truncating arbitrarily wide addresses.
  bool evaluateVectorUpdate(const std::vector<bool>& inputs) const {
    const auto& m = getVectorUpdateMetadata();
    if (inputs.size() != size_) throw NLException("VECTOR_UPDATE input count mismatch");
    if (!inputs.back()) return inputs.front();
    for (uint32_t k = 0; k < m.updateWidth; ++k) {
      const int64_t offset = int64_t(m.outputBit) - k;
      if (!m.isSigned && offset < 0) continue;
      if (m.addressSize < 64) {
        if (m.isSigned) {
          const int64_t limit = int64_t{1} << (m.addressSize - 1);
          if (offset < -limit || offset >= limit) continue;
        } else if (uint64_t(offset) >= (uint64_t{1} << m.addressSize)) continue;
      }
      bool matches = true;
      for (uint32_t bit = 0; bit < m.addressSize; ++bit) {
        const bool expected = bit < 64 ? ((uint64_t(offset) >> bit) & 1) : offset < 0;
        if (inputs[1 + m.updateWidth + m.addressSize - 1 - bit] != expected) {
          matches = false;
          break;
        }
      }
      if (matches) return inputs[1 + m.updateWidth - 1 - k];
    }
    return inputs.front();
  }

  bool operator==(const SNLTruthTable& o) const {
    return size_ == o.size_ && bits_ == o.bits_ && dependencies_ == o.dependencies_ &&
           genericType_ == o.genericType_ &&
           tableSelectAddressSize_ == o.tableSelectAddressSize_ &&
           tableSelectDepth_ == o.tableSelectDepth_ &&
           divModWidth_ == o.divModWidth_ &&
           divModSigned_ == o.divModSigned_ &&
           divModResult_ == o.divModResult_ &&
           divModOutputBit_ == o.divModOutputBit_ &&
           word_ == o.word_ && rom_ == o.rom_ && vectorUpdate_ == o.vectorUpdate_ && vectorSelect_ == o.vectorSelect_;
  }

  bool operator<(const SNLTruthTable& o) const {
    if (size_ != o.size_)
      return size_ < o.size_;
    if (genericType_ != o.genericType_)
      return genericType_ < o.genericType_;
    if (tableSelectAddressSize_ != o.tableSelectAddressSize_)
      return tableSelectAddressSize_ < o.tableSelectAddressSize_;
    if (tableSelectDepth_ != o.tableSelectDepth_)
      return tableSelectDepth_ < o.tableSelectDepth_;
    if (divModWidth_ != o.divModWidth_)
      return divModWidth_ < o.divModWidth_;
    if (divModSigned_ != o.divModSigned_)
      return divModSigned_ < o.divModSigned_;
    if (divModResult_ != o.divModResult_)
      return divModResult_ < o.divModResult_;
    if (divModOutputBit_ != o.divModOutputBit_)
      return divModOutputBit_ < o.divModOutputBit_;
    if (word_ != o.word_)
      return word_ < o.word_;
    if (rom_ != o.rom_)
      return rom_ < o.rom_;
    if (vectorSelect_ != o.vectorSelect_)
      return vectorSelect_ < o.vectorSelect_;
    if (vectorUpdate_ != o.vectorUpdate_)
      return vectorUpdate_ < o.vectorUpdate_;
    if (bits_ != o.bits_)
      return bits_ < o.bits_;
    return dependencies_ < o.dependencies_;
  }

  std::string getString() const {
    std::string result = "";
    result =  "<" + std::to_string(bits_.size()) + ", |"; 
    for (size_t i = 0; i < bits_.size(); i++) {
      result += std::to_string(bits_.bit(i));
    }
      
    result += "|>";
    return result;
  }

  bool isInitialized() const {
    if (size_ == 0) {
      return bits_.size() != 0;
    }
    if (dependencies_.empty()) {
      return false;
    }
    return isGeneric() || bits_.size() != 0;
  }

  bool isGeneric() const {
    return genericType_ != GenericType::NONE;
  }

  GenericType getGenericType() const {
    return genericType_;
  }

  bool isAnd() const {
    return isGenericOrEquivalent(GenericType::AND);
  }

  bool isNand() const {
    return isGenericOrEquivalent(GenericType::NAND);
  }

  bool isOr() const {
    return isGenericOrEquivalent(GenericType::OR);
  }

  bool isNor() const {
    return isGenericOrEquivalent(GenericType::NOR);
  }

  bool isXor() const {
    return isGenericOrEquivalent(GenericType::XOR);
  }

  bool isXnor() const {
    return isGenericOrEquivalent(GenericType::XNOR);
  }

  bool isMux() const {
    return genericType_ == GenericType::NONE && isInitialized() &&
           !isGeneric() && size_ == 3 && bits_.size() == 8 &&
           static_cast<uint64_t>(bits_) == 0xCA;
  }

  uint32_t getTableSelectAddressSize() const {
    if (genericType_ != GenericType::TABLE_SELECT) {
      throw NLException("Truth table is not a table select generic");
    }
    return tableSelectAddressSize_;
  }

  uint32_t getTableSelectDepth() const {
    if (genericType_ != GenericType::TABLE_SELECT) {
      throw NLException("Truth table is not a table select generic");
    }
    return tableSelectDepth_;
  }

  uint32_t getDivModWidth() const {
    requireDivMod();
    return divModWidth_;
  }

  bool isDivModSigned() const {
    requireDivMod();
    return divModSigned_;
  }

  DivModResult getDivModResult() const {
    requireDivMod();
    return divModResult_;
  }

  bool isWordArithmetic() const {
    return genericType_ == GenericType::ADD_SUB ||
           genericType_ == GenericType::MULTIPLY ||
           genericType_ == GenericType::SHIFT;
  }

  const WordMetadata& getWordMetadata() const {
    if (!isWordArithmetic()) {
      throw NLException("Truth table has no word arithmetic metadata");
    }
    return word_;
  }

  bool isRom() const { return genericType_ == GenericType::ROM; }

  const RomMetadata& getRomMetadata() const {
    if (!isRom()) {
      throw NLException("Truth table has no ROM metadata");
    }
    return rom_;
  }

  uint32_t getDivModOutputBit() const {
    requireDivMod();
    return divModOutputBit_;
  }

  using ConstantInput = std::pair<uint32_t, bool>;
  using ConstantInputs = std::vector<ConstantInput>;

  // Apply _all_ constants in one shot:
  SNLTruthTable getReducedWithConstants(ConstantInputs idxConsts) const {
    // Reduction relies on TT-local variable numbering, so sparse/non-local deps
    // must be canonicalized by the caller before reduction.
    if (!NLBitDependencies::isSimple(dependencies_)) {
      throw NLException(
          "getReducedWithConstants() does not support non full dependencies");
    }
    // trivial 0‐input table
    if (size_ == 0) {
      return *this;
    }
    if (idxConsts.empty()) {
      return *this;
    }

    // validate
    for (auto const& ic : idxConsts) {
      if (ic.first >= size_)
        throw NLException("Index out of range (max=6)");
    }

    // new size = old size minus #constants
    uint32_t k = static_cast<uint32_t>(idxConsts.size());
    uint32_t newSize = (size_ > k ? size_ - k : 0);
    size_t trueConstants = 0;
    bool hasFalseConstant = false;
    for (auto const& ic : idxConsts) {
      if (ic.second) {
        ++trueConstants;
      } else {
        hasFalseConstant = true;
      }
    }

    if (isGeneric()) {
      const auto deps = fullDependencies(newSize);
      switch (genericType_) {
        case GenericType::AND:
          if (hasFalseConstant) {
            return Logic0();
          }
          if (newSize == 0) {
            return Logic1();
          }
          if (newSize == 1) {
            return Buf();
          }
          return SNLTruthTable(newSize, GenericType::AND, deps);
        case GenericType::NAND:
          if (hasFalseConstant) {
            return Logic1();
          }
          if (newSize == 0) {
            return Logic0();
          }
          if (newSize == 1) {
            return Inv();
          }
          return SNLTruthTable(newSize, GenericType::NAND, deps);
        case GenericType::OR:
          if (trueConstants > 0) {
            return Logic1();
          }
          if (newSize == 0) {
            return Logic0();
          }
          if (newSize == 1) {
            return Buf();
          }
          return SNLTruthTable(newSize, GenericType::OR, deps);
        case GenericType::NOR:
          if (trueConstants > 0) {
            return Logic0();
          }
          if (newSize == 0) {
            return Logic1();
          }
          if (newSize == 1) {
            return Inv();
          }
          return SNLTruthTable(newSize, GenericType::NOR, deps);
        case GenericType::XOR: {
          const bool invert = (trueConstants % 2) != 0;
          if (newSize == 0) {
            return invert ? Logic1() : Logic0();
          }
          if (newSize == 1) {
            return invert ? Inv() : Buf();
          }
          return SNLTruthTable(
              newSize,
              invert ? GenericType::XNOR : GenericType::XOR,
              deps);
        }
        case GenericType::XNOR: {
          const bool invert = (trueConstants % 2) != 0;
          if (newSize == 0) {
            return invert ? Logic0() : Logic1();
          }
          if (newSize == 1) {
            return invert ? Buf() : Inv();
          }
          return SNLTruthTable(
              newSize,
              invert ? GenericType::XOR : GenericType::XNOR,
              deps);
        }
        case GenericType::VECTOR_SELECT:
          throw NLException("VECTOR_SELECT constant reduction is unsupported");
        case GenericType::TABLE_SELECT:
          throw NLException(
              "getReducedWithConstants() does not support TABLE_SELECT");
        case GenericType::DIVMOD:
          throw NLException(
              "getReducedWithConstants() does not support DIVMOD");
        case GenericType::VECTOR_UPDATE: {
          if (idxConsts.size() != size_)
            throw NLException("Partial VECTOR_UPDATE reduction is unsupported");
          std::vector<bool> inputs(size_);
          std::vector<bool> seen(size_);
          for (const auto& [index, value] : idxConsts) {
            if (seen[index]) throw NLException("Duplicate VECTOR_UPDATE constant input");
            seen[index] = true;
            inputs[index] = value;
          }
          return evaluateVectorUpdate(inputs) ? Logic1() : Logic0();
        }
        case GenericType::ROM:
          throw NLException(
              "getReducedWithConstants() does not support ROM");
        default:
          break;  // LCOV_EXCL_LINE
      }
    }

    uint32_t newN = 1u << newSize;
    uint64_t reduced = 0;
    std::vector<bool> reducedVect(newN, false);

    // for each assignment 'j' of the remaining newSize bits,
    // weave in the constants to build the original index:
    for (uint32_t j = 0; j < newN; ++j) {
      uint32_t origIdx = 0;
      uint32_t remPos = 0;

      for (uint32_t bit = 0; bit < size_; ++bit) {
        bool val = false;

        // if this 'bit' is one of the constants, use that:
        auto it = std::find_if(
            idxConsts.begin(), idxConsts.end(),
            [&](ConstantInput const& c) { return c.first == bit; });
        if (it != idxConsts.end()) {
          val = it->second;
        } else {
          // otherwise pull next bit from 'j'
          val = ((j >> remPos) & 1) != 0;
          ++remPos;
        }

        origIdx |= (uint32_t(val) << bit);
      }
      // always pull the bit via bit()
      bool inputBit = bits().bit(origIdx);

      if (newSize <= 6) {
        if (inputBit) {
          reduced |= (uint64_t{1} << j);
        }
      } else {
        reducedVect[j] = inputBit;
      }
    }

    SNLTruthTable out;
    if (newSize > 6) {
      out = SNLTruthTable(newSize, reducedVect, fullDependencies(newSize));
    } else {
      out = SNLTruthTable(newSize, reduced, fullDependencies(newSize));
    }    
    if (out.all0()) {
      return Logic0();
    }
    if (out.all1()) {
      return Logic1();
    }
    return out;
  }

  SNLTruthTable getReducedWithConstant(uint32_t idx, bool c) const {
    return getReducedWithConstants({{idx, c}});
  }

  bool hasNoInfluence(uint32_t v) const {
    auto t0 = getReducedWithConstant(v, false);
    auto t1 = getReducedWithConstant(v, true);
    return t0 == t1;
  }

  SNLTruthTable removeVariable(uint32_t v) const {
    if (v >= size_)
      throw NLException("Index out of range");
    SNLTruthTable out(size_ - 1, 0, fullDependencies(size_ - 1));
    uint32_t fullN = 1u << size_;

    for (uint32_t i = 0; i < fullN; ++i) {
      if (((i >> v) & 1) == 0) {
        uint32_t low = i & ((1u << v) - 1);
        uint32_t high = (i >> 1) & ~((1u << v) - 1);
        uint32_t ni = low | high;

        if (((bits() >> i) & 1) != 0)
          out.bits_ |= (1u << ni);
      }
    }
    return out;
    //LCOV_EXCL_START
  }
  //LCOV_EXCL_STOP

  bool all0() const {
    if (bits().size() == 0) {
      return false;
    }
    if (size() <= 6) {
      uint64_t rows = 1ull << size_;    // # of table entries = 1<<size_
      uint64_t mask = (rows < 64
               ? ((1ull << rows) - 1ull)
               : std::numeric_limits<uint64_t>::max());
      return (bits().operator uint64_t() & mask) == 0ull;
    }
    bool result = false;
    for (size_t i = 0; i < bits().size(); i++) {
      result |= bits().bit(i);
    }
    return result == false;
  }

  bool all1() const {
    if (bits().size() == 0) {
      return false;
    }
    if (size() <= 6) {
      uint64_t rows = 1ull << size_;    // # of table entries = 1<<size_
      uint64_t mask = (rows < 64
               ? ((1ull << rows) - 1ull)
               : std::numeric_limits<uint64_t>::max());
      return (bits().operator uint64_t() & mask) == mask;
    }
    bool result = true;
    for (size_t i = 0; i < bits().size(); i++) {
      result &= bits().bit(i);
    }
    return result;
  }

  uint32_t size() const { return size_; }
  const NLBitVecDynamic& bits() const { return bits_; }

  const std::vector<uint64_t>& getDependencies() const { return dependencies_; }

  std::string toString() const {
    std::ostringstream oss;
    oss << "SNLTruthTable(size=" << size_ << ", bits=" << bits_.toString()
        << ", dependencies=[";
    for (size_t i = 0; i < dependencies_.size(); ++i) {
      oss << dependencies_[i];
      if (i < dependencies_.size() - 1) {
        oss << ", ";
      }
    }
    oss << "]";
    if (genericType_ == GenericType::DIVMOD) {
      oss << ", genericType=DIVMOD"
          << ", divModWidth=" << divModWidth_
          << ", divModSigned=" << divModSigned_
          << ", divModResult=" << static_cast<int>(divModResult_)
          << ", divModOutputBit=" << divModOutputBit_;
    }
    oss << ")";
    return oss.str();
  }

 private:
  void requireDivMod() const {
    if (genericType_ != GenericType::DIVMOD) {
      throw NLException("Truth table is not a DIVMOD generic");
    }
  }

  bool isGenericOrEquivalent(GenericType type) const {
    if (genericType_ == type) {
      return true;
    }
    if (genericType_ != GenericType::NONE || !isInitialized() || size_ < 2) {
      return false;
    }
    for (size_t idx = 0; idx < bits_.size(); ++idx) {
      if (bits_.bit(idx) != expectedGenericValue(type, idx)) {
        return false;
      }
    }
    return true;
  }

  bool expectedGenericValue(GenericType type, size_t idx) const {
    bool hasZero = false;
    bool hasOne = false;
    bool oddParity = false;
    for (uint32_t bit = 0; bit < size_; ++bit) {
      const bool value =
          bit < sizeof(size_t) * 8 ? ((idx >> bit) & 1u) != 0 : false;
      hasZero |= !value;
      hasOne |= value;
      oddParity ^= value;
    }
    switch (type) {
      case GenericType::AND:
        return !hasZero;
      case GenericType::NAND:
        return hasZero;
      case GenericType::OR:
        return hasOne;
      case GenericType::NOR:
        return !hasOne;
      case GenericType::XOR:
        return oddParity;
      case GenericType::XNOR:
        return !oddParity;
      default:  // LCOV_EXCL_LINE
        return false;
    }
  }

  void validateGenericMetadata() const {
    if (genericType_ == GenericType::VECTOR_SELECT) {
      if (!vectorSelect_.width || !vectorSelect_.selectWidth || !vectorSelect_.addressSize ||
          vectorSelect_.outputBit >= vectorSelect_.selectWidth ||
          uint64_t(vectorSelect_.width) + vectorSelect_.addressSize != size_) {
        throw NLException("Invalid VECTOR_SELECT truth table metadata");
      }
      return;
    }
    // Public factories populate vector metadata only for its matching generic;
    // constructors, copies and assignment preserve that invariant.
    if (vectorSelect_ != VectorSelectMetadata{})
      throw NLException("Unexpected VECTOR_SELECT metadata on truth table"); // LCOV_EXCL_LINE
    if (genericType_ == GenericType::VECTOR_UPDATE) {
      if (!vectorUpdate_.width || !vectorUpdate_.updateWidth || !vectorUpdate_.addressSize ||
          vectorUpdate_.outputBit >= vectorUpdate_.width ||
          uint64_t(vectorUpdate_.updateWidth) + vectorUpdate_.addressSize + 2 != size_) {
        throw NLException("Invalid VECTOR_UPDATE truth table metadata");
      }
      return;
    }
    if (vectorUpdate_ != VectorUpdateMetadata{})
      throw NLException("Unexpected VECTOR_UPDATE metadata on truth table"); // LCOV_EXCL_LINE
    if (genericType_ == GenericType::TABLE_SELECT) {
      if (tableSelectAddressSize_ == 0 || tableSelectDepth_ == 0 ||
          size_ != static_cast<size_t>(tableSelectAddressSize_) +
                       tableSelectDepth_) {
        throw NLException("Invalid TABLE_SELECT truth table metadata");
      }
      return;
    }
    if (genericType_ == GenericType::DIVMOD) {
      if (tableSelectAddressSize_ != 0 || tableSelectDepth_ != 0 ||
          divModWidth_ == 0 ||
          divModWidth_ > std::numeric_limits<uint32_t>::max() / 2 ||
          size_ != divModWidth_ * 2 ||
          divModOutputBit_ >= divModWidth_) {
        throw NLException("Invalid DIVMOD truth table metadata");
      }
      return;
    }
    if (genericType_ == GenericType::ROM) {
      if (tableSelectAddressSize_ != 0 || tableSelectDepth_ != 0 ||
          divModWidth_ != 0 || word_ != WordMetadata{} ||
          rom_.addressSize == 0 || rom_.addressSize >= 64 || rom_.depth == 0 ||
          rom_.depth > (uint64_t{1} << rom_.addressSize) ||
          size_ != rom_.addressSize || rom_.column.size() != rom_.depth) {
        throw NLException("Invalid ROM truth table metadata");
      }
      return;
    }
    if (isWordArithmetic()) {
      const uint64_t inputCount =
          static_cast<uint64_t>(word_.width) +
          (genericType_ == GenericType::SHIFT ? word_.countWidth : word_.width);
      if (tableSelectAddressSize_ != 0 || tableSelectDepth_ != 0 ||
          divModWidth_ != 0 || word_.width == 0 ||
          word_.outputBit >= word_.width || size_ != inputCount ||
          (genericType_ == GenericType::SHIFT) != (word_.countWidth != 0) ||
          (genericType_ != GenericType::ADD_SUB && word_.subtract) ||
          (genericType_ != GenericType::SHIFT && (word_.right || word_.arithmetic)) ||
          (word_.arithmetic && !word_.right)) {
        throw NLException("Invalid word arithmetic truth table metadata");
      }
      return;
    }
    if (tableSelectAddressSize_ != 0 || tableSelectDepth_ != 0) {
      throw NLException("Unexpected table select metadata on truth table");
    }
    if (divModWidth_ != 0 || divModSigned_ ||
        divModResult_ != DivModResult::QUOTIENT || divModOutputBit_ != 0) {
      throw NLException("Unexpected DIVMOD metadata on truth table");
    }
    if (word_ != WordMetadata{}) {
      throw NLException("Unexpected word arithmetic metadata on truth table");
    }
    // Only the Rom factory sets ROM metadata, and it fixes the generic type.
    // LCOV_EXCL_START
    if (rom_ != RomMetadata{}) {
      throw NLException("Unexpected ROM metadata on truth table");
    }
    // LCOV_EXCL_STOP
  }

  uint32_t size_{0};
  NLBitVecDynamic bits_{0};
  std::vector<uint64_t> dependencies_{};
  GenericType genericType_{GenericType::NONE};
  uint32_t tableSelectAddressSize_{0};
  uint32_t tableSelectDepth_{0};
  uint32_t divModWidth_{0};
  bool divModSigned_{false};
  DivModResult divModResult_{DivModResult::QUOTIENT};
  uint32_t divModOutputBit_{0};
  WordMetadata word_{};
  RomMetadata rom_{};
  VectorUpdateMetadata vectorUpdate_{};
  VectorSelectMetadata vectorSelect_{};
};

}  // namespace naja::NL
