// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignModeling.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

#include "NLBitDependencies.h"
#include "NLException.h"
#include "NLName.h"
#include "NajaDumpableProperty.h"
#include "NajaPrivateProperty.h"

#include "NLDB0.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLParameter.h"
#include "SNLScalarTerm.h"

// Common macros to unify repeated calculations without changing behavior
#define TT_NCHUNKS_FROM_BITS(nBits) \
  ((nBits) / 64 + (((nBits) % 64) > 0 ? 1 : 0))

#define TT_FILL_BITS(bitsVec, nBits, property, bitsIdx)          \
  do {                                                           \
    size_t _tt_nchunks = TT_NCHUNKS_FROM_BITS(nBits);            \
    for (size_t c = 0; c < _tt_nchunks; ++c) {                   \
      uint64_t mask = (property)->getUInt64Value((bitsIdx) + c); \
      for (size_t b = 0; b < 64; ++b) {                          \
        size_t pos = c * 64 + b;                                 \
        if (pos >= (nBits))                                      \
          break;                                                 \
        if ((mask >> b) & 1)                                     \
          (bitsVec)[pos] = true;                                 \
      }                                                          \
    }                                                            \
  } while (0)

namespace {

uint64_t widthMask(size_t width) {
  return width >= 64 ? std::numeric_limits<uint64_t>::max()
                     : ((uint64_t{1} << width) - 1);
}

uint64_t parseUnsignedDigits(
    const std::string& digits, unsigned base, const std::string& literal) {
  if (digits.empty()) {
    throw naja::NL::NLException(
        "Empty truth-table parameter value <" + literal + ">");
  }
  uint64_t value = 0;
  for (char character : digits) {
    const char lower = static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
    if (lower == 'x' || lower == 'z' || lower == '?') {
      throw naja::NL::NLException(
          "Unknown bits are not supported in truth-table parameter value <" +
          literal + ">");
    }
    unsigned digit = 0;
    if (lower >= '0' && lower <= '9') {
      digit = static_cast<unsigned>(lower - '0');
    } else if (lower >= 'a' && lower <= 'f') {
      digit = static_cast<unsigned>(lower - 'a' + 10);
    } else {
      throw naja::NL::NLException(
          "Invalid truth-table parameter value <" + literal + ">");
    }
    if (digit >= base ||
        value > (std::numeric_limits<uint64_t>::max() - digit) / base) {
      throw naja::NL::NLException(
          "Truth-table parameter value does not fit in 64 bits <" + literal +
          ">");
    }
    value = value * base + digit;
  }
  return value;
}

uint64_t parseTruthTableParameterValue(
    const std::string& literal, size_t contextualWidth) {
  std::string normalized;
  normalized.reserve(literal.size());
  for (char character : literal) {
    if (character != '_' &&
        !std::isspace(static_cast<unsigned char>(character))) {
      normalized.push_back(character);
    }
  }
  if (normalized.empty()) {
    throw naja::NL::NLException("Empty truth-table parameter value");
  }

  size_t declaredWidth = 0;
  bool hasDeclaredWidth = false;
  unsigned base = 10;
  std::string digits;
  const auto quote = normalized.find('\'');
  if (quote != std::string::npos) {
    if (quote > 0) {
      declaredWidth = static_cast<size_t>(parseUnsignedDigits(
          normalized.substr(0, quote), 10, literal));
      hasDeclaredWidth = true;
    }
    size_t position = quote + 1;
    if (position < normalized.size() &&
        (normalized[position] == 's' || normalized[position] == 'S')) {
      ++position;
    }
    if (position + 1 == normalized.size() &&
        (normalized[position] == '0' || normalized[position] == '1' ||
         normalized[position] == 'x' || normalized[position] == 'X' ||
         normalized[position] == 'z' || normalized[position] == 'Z' ||
         normalized[position] == '?')) {
      if (normalized[position] != '0' && normalized[position] != '1') {
        throw naja::NL::NLException(
            "Unknown bits are not supported in truth-table parameter value <" +
            literal + ">");
      }
      return normalized[position] == '1' ? widthMask(contextualWidth) : 0;
    }
    if (position >= normalized.size()) {
      throw naja::NL::NLException(
          "Invalid truth-table parameter value <" + literal + ">");
    }
    switch (static_cast<char>(std::tolower(
        static_cast<unsigned char>(normalized[position])))) {
      case 'b': base = 2; break;
      case 'o': base = 8; break;
      case 'd': base = 10; break;
      case 'h': base = 16; break;
      default:
        throw naja::NL::NLException(
            "Invalid truth-table parameter radix in <" + literal + ">");
    }
    digits = normalized.substr(position + 1);
  } else {
    if (normalized.size() > 2 && normalized[0] == '0' &&
        (normalized[1] == 'x' || normalized[1] == 'X')) {
      base = 16;
      digits = normalized.substr(2);
    } else {
      digits = normalized;
    }
  }

  uint64_t value = parseUnsignedDigits(digits, base, literal);
  if (hasDeclaredWidth && declaredWidth < 64) {
    value &= widthMask(declaredWidth);
  }
  return value;
}

uint64_t getParameterTruthTableBits(
    const std::string& value, size_t inputCount, size_t parameterBitOffset) {
  // LCOV_EXCL_START
  // Public callers reject more than six inputs before reaching this helper.
  if (inputCount > 6) {
    throw naja::NL::NLException(
        "Parameter-derived truth tables support at most 6 inputs");
  }
  // LCOV_EXCL_STOP
  const size_t tableBitCount = size_t{1} << inputCount;
  if (parameterBitOffset > 64 - tableBitCount) {
    throw naja::NL::NLException(
        "Parameter-derived truth table exceeds the 64-bit parameter range");
  }
  const uint64_t parameterValue = parseTruthTableParameterValue(
      value, parameterBitOffset + tableBitCount);
  if (tableBitCount == 64) {
    return parameterValue;
  }
  return (parameterValue >> parameterBitOffset) & widthMask(tableBitCount);
}

bool isDB0SequentialPrimitive(const naja::NL::SNLDesign* design) {
  return design &&
         (naja::NL::NLDB0::isDLatch(design) ||
          naja::NL::NLDB0::isDFF(design) ||
          naja::NL::NLDB0::isDFFN(design) ||
          naja::NL::NLDB0::isDFFRN(design) ||
          naja::NL::NLDB0::isDFFR(design) ||
          naja::NL::NLDB0::isDFFS(design) ||
          naja::NL::NLDB0::isDFFE(design) ||
          naja::NL::NLDB0::isDFFRE(design) ||
          naja::NL::NLDB0::isDFFSE(design) ||
          naja::NL::NLDB0::isDFFSR(design) ||
          naja::NL::NLDB0::isDFFSRN(design) ||
          naja::NL::NLDB0::isDFFSS(design) ||
          naja::NL::NLDB0::isDFFSSN(design) ||
          naja::NL::NLDB0::isDFFSRE(design) ||
          naja::NL::NLDB0::isDFFSRNE(design) ||
          naja::NL::NLDB0::isDFFSSE(design) ||
          naja::NL::NLDB0::isDFFSSNE(design));
}

naja::NL::SNLScalarTerm* getDB0SequentialClockTerm(const naja::NL::SNLDesign* design) {
  if (naja::NL::NLDB0::isDLatch(design)) {
    return design->getScalarTerm(naja::NL::NLName("E"));
  }
  if (naja::NL::NLDB0::isDFF(design) ||
      naja::NL::NLDB0::isDFFN(design) ||
      naja::NL::NLDB0::isDFFRN(design) ||
      naja::NL::NLDB0::isDFFR(design) ||
      naja::NL::NLDB0::isDFFS(design) ||
      naja::NL::NLDB0::isDFFE(design) ||
      naja::NL::NLDB0::isDFFRE(design) ||
      naja::NL::NLDB0::isDFFSE(design) ||
      naja::NL::NLDB0::isDFFSR(design) ||
      naja::NL::NLDB0::isDFFSRN(design) ||
      naja::NL::NLDB0::isDFFSS(design) ||
      naja::NL::NLDB0::isDFFSSN(design) ||
      naja::NL::NLDB0::isDFFSRE(design) ||
      naja::NL::NLDB0::isDFFSRNE(design) ||
      naja::NL::NLDB0::isDFFSSE(design) ||
      naja::NL::NLDB0::isDFFSSNE(design)) {
    return design->getScalarTerm(naja::NL::NLName("C"));
  }
  return nullptr;  // LCOV_EXCL_LINE
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0ClockRelatedInputs(
    naja::NL::SNLBitTerm* clock) {
  auto design = clock->getDesign();
  auto db0Clock = getDB0SequentialClockTerm(design);
  if (clock != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return design->getBitTerms().getSubCollection(
      [db0Clock](const naja::NL::SNLBitTerm* term) {
        return term != db0Clock &&
               term->getDirection() != naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0ClockRelatedOutputs(
    naja::NL::SNLBitTerm* clock) {
  auto design = clock->getDesign();
  auto db0Clock = getDB0SequentialClockTerm(design);
  if (clock != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return design->getBitTerms().getSubCollection(
      [](const naja::NL::SNLBitTerm* term) {
        return term->getDirection() == naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0InputRelatedClocks(
    naja::NL::SNLBitTerm* input) {
  auto design = input->getDesign();
  auto db0Clock = getDB0SequentialClockTerm(design);
  if (!db0Clock || input == db0Clock ||
      input->getDirection() == naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return naja::NajaCollection(new naja::NajaSingletonCollection(db0Clock))
      .getParentTypeCollection<naja::NL::SNLBitTerm*>();
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0OutputRelatedClocks(
    naja::NL::SNLBitTerm* output) {
  auto db0Clock = getDB0SequentialClockTerm(output->getDesign());
  if (!db0Clock ||
      output->getDirection() != naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return naja::NajaCollection(new naja::NajaSingletonCollection(db0Clock))
      .getParentTypeCollection<naja::NL::SNLBitTerm*>();
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0ClockRelatedInputs(
    naja::NL::SNLInstTerm* clock) {
  auto instance = clock->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock || clock->getBitTerm() != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return instance->getInstTerms().getSubCollection(
      [db0Clock](const naja::NL::SNLInstTerm* term) {
        return term->getBitTerm() != db0Clock &&
               term->getBitTerm()->getDirection() != naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0ClockRelatedOutputs(
    naja::NL::SNLInstTerm* clock) {
  auto instance = clock->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock || clock->getBitTerm() != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return instance->getInstTerms().getSubCollection(
      [](const naja::NL::SNLInstTerm* term) {
        return term->getBitTerm()->getDirection() == naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0InputRelatedClocks(
    naja::NL::SNLInstTerm* input) {
  auto instance = input->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock || input->getBitTerm() == db0Clock ||
      input->getBitTerm()->getDirection() == naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return naja::NajaCollection<naja::NL::SNLInstTerm*>(
      new naja::NajaSingletonCollection(instance->getInstTerm(db0Clock)));
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0OutputRelatedClocks(
    naja::NL::SNLInstTerm* output) {
  auto instance = output->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock ||
      output->getBitTerm()->getDirection() != naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return naja::NajaCollection<naja::NL::SNLInstTerm*>(
      new naja::NajaSingletonCollection(instance->getInstTerm(db0Clock)));
}

naja::NL::SNLDesignModeling::MemoryResetMode convertMemoryResetMode(
    naja::NL::NLDB0::MemoryResetMode mode) {
  switch (mode) {
    case naja::NL::NLDB0::MemoryResetMode::None:
      return naja::NL::SNLDesignModeling::MemoryResetMode::None;
    case naja::NL::NLDB0::MemoryResetMode::AsyncLow:
      return naja::NL::SNLDesignModeling::MemoryResetMode::AsyncLow;
    case naja::NL::NLDB0::MemoryResetMode::AsyncHigh:
      return naja::NL::SNLDesignModeling::MemoryResetMode::AsyncHigh;
    case naja::NL::NLDB0::MemoryResetMode::SyncLow:
      return naja::NL::SNLDesignModeling::MemoryResetMode::SyncLow;
    case naja::NL::NLDB0::MemoryResetMode::SyncHigh:
      return naja::NL::SNLDesignModeling::MemoryResetMode::SyncHigh;
  }
  return naja::NL::SNLDesignModeling::MemoryResetMode::None;  // LCOV_EXCL_LINE
}

naja::NL::SNLDesignModeling::MemoryInterface buildDB0MemoryInterface(
    const naja::NL::SNLDesign* design) {
  // DB0 memories already carry a compact signature. Expand that signature into
  // the same generic MemoryInterface shape used for Liberty/SV-inferred
  // memories so downstream clients do not need separate DB0-specific code.
  auto signature = naja::NL::NLDB0::getMemorySignature(design);
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  memInterface.width = signature.width;
  memInterface.depth = signature.depth;
  memInterface.abits = signature.abits;
  memInterface.resetMode = convertMemoryResetMode(signature.resetMode);
  memInterface.clock = signature.independentWriteClocks
    ? nullptr : naja::NL::NLDB0::getMemoryClock(design);
  memInterface.reset = signature.independentWriteClocks
    ? nullptr : naja::NL::NLDB0::getMemoryReset(design);

  auto* raddr = naja::NL::NLDB0::getMemoryReadAddress(design);
  auto* rdata = naja::NL::NLDB0::getMemoryReadData(design);
  auto* waddr = naja::NL::NLDB0::getMemoryWriteAddress(design);
  auto* wdata = naja::NL::NLDB0::getMemoryWriteData(design);
  auto* we = naja::NL::NLDB0::getMemoryWriteEnable(design);

  memInterface.readPorts.reserve(signature.readPorts);
  for (size_t port = 0; port < signature.readPorts; ++port) {
    naja::NL::SNLDesignModeling::MemoryReadPort readPort;
    for (size_t bit = 0; bit < signature.abits; ++bit) {
      readPort.address.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              raddr->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.abits + bit))));
    }
    for (size_t bit = 0; bit < signature.width; ++bit) {
      readPort.data.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              rdata->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.width + bit))));
    }
    memInterface.readPorts.push_back(std::move(readPort));
  }

  memInterface.writePorts.reserve(signature.writePorts);
  for (size_t port = 0; port < signature.writePorts; ++port) {
    naja::NL::SNLDesignModeling::MemoryWritePort writePort;
    if (signature.independentWriteClocks) {
      writePort.clock = design->getBusTerm(naja::NL::NLName("WCLK"))->getBit(
        static_cast<naja::NL::NLID::Bit>(port));
      auto* masks = design->getBusTerm(naja::NL::NLName("WMASK"));
      for (size_t bit = 0; bit < signature.width; ++bit)
        writePort.mask.push_back(masks->getBit(
          static_cast<naja::NL::NLID::Bit>(port * signature.width + bit)));
    }
    for (size_t bit = 0; bit < signature.abits; ++bit) {
      writePort.address.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              waddr->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.abits + bit))));
    }
    for (size_t bit = 0; bit < signature.width; ++bit) {
      writePort.data.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              wdata->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.width + bit))));
    }
    // Port i's enable is WE[i], the same index layout as its WADDR/WDATA slices.
    writePort.enables.push_back(
        static_cast<naja::NL::SNLBitTerm*>(
            we->getBit(static_cast<naja::NL::NLID::Bit>(port))));
    memInterface.writePorts.push_back(std::move(writePort));
  }

  return memInterface;
}

bool containsBitTerm(
    const naja::NL::SNLDesignModeling::BitTerms& terms,
    const naja::NL::SNLBitTerm* candidate) {
  return std::find(terms.begin(), terms.end(), candidate) != terms.end();
}

bool isMemoryClockTerm(
    const naja::NL::SNLDesignModeling::MemoryInterface& memory,
    const naja::NL::SNLBitTerm* term) {
  if (!term) return false;
  if (term == memory.clock) return true;
  for (const auto& port : memory.writePorts)
    if (port.clock == term) return true;
  return false;
}

bool hasMemoryClock(const naja::NL::SNLDesignModeling::MemoryInterface& memory) {
  if (memory.clock) return true;
  for (const auto& port : memory.writePorts)
    if (port.clock) return true;
  return false;
}

bool isMemoryClockRelatedInputTerm(
    const naja::NL::SNLDesignModeling::MemoryInterface& memInterface,
    const naja::NL::SNLBitTerm* term,
    const naja::NL::SNLBitTerm* clock = nullptr) {
  if (term == nullptr || isMemoryClockTerm(memInterface, term)) {
    return false;
  }
  if (memInterface.reset != nullptr && term == memInterface.reset) {
    return !clock || clock == memInterface.clock;
  }
  // Read ports describe the combinational read surface of the memory. They do
  // not update stored state on the clock edge, so keep them out of the
  // clock-input relation. Write controls/data and reset are the sequential
  // next-state dependencies.
  for (const auto& writePort : memInterface.writePorts) {
    if (clock && clock != (writePort.clock ? writePort.clock : memInterface.clock)) continue;
    if (containsBitTerm(writePort.address, term) ||
        containsBitTerm(writePort.data, term) ||
        containsBitTerm(writePort.mask, term) ||
        containsBitTerm(writePort.enables, term)) {
      return true;
    }
    for (const auto& extraWriteInputs : writePort.extraWriteInputs) {
      if (containsBitTerm(extraWriteInputs, term)) {
        return true;
      }
    }
  }
  return false;
}

bool isMemoryClockRelatedOutputTerm(
    const naja::NL::SNLDesignModeling::MemoryInterface& memInterface,
    const naja::NL::SNLBitTerm* term) {
  if (term == nullptr) {
    return false; // LCOV_EXCL_LINE
  }
  for (const auto& readPort : memInterface.readPorts) {
    if (containsBitTerm(readPort.data, term)) {
      return true; 
    }
  }
  return false;
}

bool tryGetMemoryInterface(
    const naja::NL::SNLDesign* design,
    naja::NL::SNLDesignModeling::MemoryInterface& memInterface) {
  if (!naja::NL::SNLDesignModeling::hasMemoryInterface(design)) {
    return false; // LCOV_EXCL_LINE
  }
  memInterface = naja::NL::SNLDesignModeling::getMemoryInterface(design);
  return hasMemoryClock(memInterface);
}

bool tryGetMemoryInterface(
    const naja::NL::SNLInstance* instance,
    naja::NL::SNLDesignModeling::MemoryInterface& memInterface) {
  if (instance == nullptr ||
      !naja::NL::SNLDesignModeling::hasMemoryInterface(instance->getModel())) {
    return false; // LCOV_EXCL_LINE
  }
  memInterface = naja::NL::SNLDesignModeling::getMemoryInterface(instance);
  return hasMemoryClock(memInterface);
}

template <typename Predicate>
naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryBitTerms(
    naja::NL::SNLDesign* design,
    Predicate predicate) {
  return design->getBitTerms().getSubCollection(
      [predicate](const naja::NL::SNLBitTerm* term) {
        return predicate(term);
      }); // LCOV_EXCL_LINE
} // LCOV_EXCL_LINE

template <typename Predicate>
naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryInstTerms(
    naja::NL::SNLInstance* instance,
    Predicate predicate) {
  return instance->getInstTerms().getSubCollection(
      [predicate](const naja::NL::SNLInstTerm* term) {
        return term != nullptr && predicate(term->getBitTerm());
      }); // LCOV_EXCL_LINE
} // LCOV_EXCL_LINE

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryClockRelatedInputs(
    naja::NL::SNLBitTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getDesign(), memInterface) ||
      !isMemoryClockTerm(memInterface, clock)) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return getMemoryBitTerms(
      clock->getDesign(), [memInterface, clock](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedInputTerm(memInterface, term, clock);
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryClockRelatedOutputs(
    naja::NL::SNLBitTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getDesign(), memInterface) ||
      !isMemoryClockTerm(memInterface, clock)) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return getMemoryBitTerms(
      clock->getDesign(), [memInterface](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedOutputTerm(memInterface, term);
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryInputRelatedClocks(
    naja::NL::SNLBitTerm* input) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!input || !tryGetMemoryInterface(input->getDesign(), memInterface) ||
      !isMemoryClockRelatedInputTerm(memInterface, input)) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return getMemoryBitTerms(input->getDesign(),
    [memInterface, input](const naja::NL::SNLBitTerm* clock) {
      return isMemoryClockTerm(memInterface, clock) &&
        isMemoryClockRelatedInputTerm(memInterface, input, clock);
    });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryOutputRelatedClocks(
    naja::NL::SNLBitTerm* output) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!output || !tryGetMemoryInterface(output->getDesign(), memInterface) ||
      !isMemoryClockRelatedOutputTerm(memInterface, output)) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return getMemoryBitTerms(output->getDesign(),
    [memInterface](const naja::NL::SNLBitTerm* clock) {
      return isMemoryClockTerm(memInterface, clock);
    });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryClockRelatedInputs(
    naja::NL::SNLInstTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getInstance(), memInterface) ||
      !isMemoryClockTerm(memInterface, clock->getBitTerm())) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return getMemoryInstTerms(
      clock->getInstance(), [memInterface, clock](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedInputTerm(memInterface, term, clock->getBitTerm());
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryClockRelatedOutputs(
    naja::NL::SNLInstTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getInstance(), memInterface) ||
      !isMemoryClockTerm(memInterface, clock->getBitTerm())) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return getMemoryInstTerms(
      clock->getInstance(), [memInterface](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedOutputTerm(memInterface, term);
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryInputRelatedClocks(
    naja::NL::SNLInstTerm* input) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!input || !tryGetMemoryInterface(input->getInstance(), memInterface) ||
      !isMemoryClockRelatedInputTerm(memInterface, input->getBitTerm())) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return getMemoryInstTerms(input->getInstance(),
    [memInterface, input](const naja::NL::SNLBitTerm* clock) {
      return isMemoryClockTerm(memInterface, clock) &&
        isMemoryClockRelatedInputTerm(memInterface, input->getBitTerm(), clock);
    });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryOutputRelatedClocks(
    naja::NL::SNLInstTerm* output) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!output || !tryGetMemoryInterface(output->getInstance(), memInterface) ||
      !isMemoryClockRelatedOutputTerm(memInterface, output->getBitTerm())) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return getMemoryInstTerms(output->getInstance(),
    [memInterface](const naja::NL::SNLBitTerm* clock) {
      return isMemoryClockTerm(memInterface, clock);
    });
}

void validateMemoryInterfaceForDesign(
    naja::NL::SNLDesign* design,
    const naja::NL::SNLDesignModeling::MemoryInterface& memInterface) {
  if (!memInterface.isValid()) {
    throw naja::NL::NLException(
        "SNLDesignModeling::setMemoryInterface: invalid memory memInterface");
  }
  auto validateBitTerms = [design](const naja::NL::SNLDesignModeling::BitTerms& terms,
                                   const std::string& name) {
    for (auto* term : terms) {
      if (!term || term->getDesign() != design) {
        throw naja::NL::NLException(
            "SNLDesignModeling::setMemoryInterface: invalid " + name +
            " term ownership");
      }
    }
  };

  if (memInterface.clock && memInterface.clock->getDesign() != design) {
    throw naja::NL::NLException(
        "SNLDesignModeling::setMemoryInterface: invalid clock term");
  }
  if (memInterface.reset && memInterface.reset->getDesign() != design) {
    throw naja::NL::NLException(
        "SNLDesignModeling::setMemoryInterface: invalid reset term");
  }

  for (const auto& port : memInterface.readPorts) {
    validateBitTerms(port.address, "read-address");
    validateBitTerms(port.data, "read-data");
    validateBitTerms(port.enables, "read-enable");
  }
  for (const auto& port : memInterface.writePorts) {
    if (port.clock && port.clock->getDesign() != design)
      throw naja::NL::NLException("SNLDesignModeling::setMemoryInterface: invalid write clock ownership");
    validateBitTerms(port.address, "write-address");
    validateBitTerms(port.data, "write-data");
    validateBitTerms(port.mask, "write-mask");
    validateBitTerms(port.enables, "write-enable");
    for (const auto& extra : port.extraWriteInputs) {
      validateBitTerms(extra, "extra-write");
    }
  }
}

bool isConnectedInstanceBitTerm(const naja::NL::SNLInstance* instance,
                                const naja::NL::SNLBitTerm* term) {
  if (!instance || !term) {
    return false; // LCOV_EXCL_LINE
  }
  auto* instTerm = instance->getInstTerm(term);
  return instTerm != nullptr && instTerm->getNet() != nullptr;
}

bool areConnectedInstanceBitTerms(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesignModeling::BitTerms& terms) {
  for (auto* term : terms) {
    if (!isConnectedInstanceBitTerm(instance, term)) {
      return false;
    }
  }
  return true;
}

bool isConnectedReadPort(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesignModeling::MemoryReadPort& port) {
  return areConnectedInstanceBitTerms(instance, port.address) &&
         areConnectedInstanceBitTerms(instance, port.data) &&
         areConnectedInstanceBitTerms(instance, port.enables);
}

bool isConnectedWritePort(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesignModeling::MemoryWritePort& port) {
  if ((port.clock && !isConnectedInstanceBitTerm(instance, port.clock)) ||
      !areConnectedInstanceBitTerms(instance, port.address) ||
      !areConnectedInstanceBitTerms(instance, port.data) ||
      !areConnectedInstanceBitTerms(instance, port.mask) ||
      !areConnectedInstanceBitTerms(instance, port.enables)) {
    return false;
  }
  for (const auto& extraInputs : port.extraWriteInputs) {
    if (!areConnectedInstanceBitTerms(instance, extraInputs)) {
      return false;
    }
  }
  return true;
}

class SNLDesignModelingProperty : public naja::NajaPrivateProperty {
 public:
  using Inherit = naja::NajaPrivateProperty;
  static const inline std::string Name = "SNLDesignModelingProperty";
  static SNLDesignModelingProperty* create(
      naja::NL::SNLDesign* design,
      naja::NL::SNLDesignModeling::Type type) {
    preCreate(design, Name);
    SNLDesignModelingProperty* property = new SNLDesignModelingProperty();
    property->modeling_ = new naja::NL::SNLDesignModeling(type);
    property->postCreate(design);
    return property;
  }
  static void preCreate(naja::NL::SNLDesign* design, const std::string& name) {
    Inherit::preCreate(design, name);
    if (not(design->isLeaf())) {
      std::ostringstream reason;
      reason << "Impossible to add Timing Modeling on a non leaf design <"
             << design->getName().getString() << ">";
      throw naja::NL::NLException(reason.str());
    }
  }
  void preDestroy() override {
    if (modeling_) {
      delete modeling_;
    }
    Inherit::preDestroy();
  }
  naja::NL::SNLDesignModeling::Type getModelingType() const {
    return getModeling()->getType();
  }
  std::string getName() const override { return Name; }
  // LCOV_EXCL_START
  std::string getString() const override { return Name; }
  // LCOV_EXCL_STOP
  naja::NL::SNLDesignModeling* getModeling() const { return modeling_; }

 private:
  naja::NL::SNLDesignModeling* modeling_{nullptr};
};

SNLDesignModelingProperty* getProperty(const naja::NL::SNLDesign* design) {
  auto property = static_cast<SNLDesignModelingProperty*>(
      design->getProperty(SNLDesignModelingProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

// type will used only if the property is created
SNLDesignModelingProperty* getOrCreateProperty(
    naja::NL::SNLDesign* design,
    naja::NL::SNLDesignModeling::Type type) {
  auto property = getProperty(design);
  if (property) {
    return property;
  }
  return SNLDesignModelingProperty::create(design, type);
}

void insertInArcs(naja::NL::SNLDesignModeling::Arcs& arcs,
                  naja::NL::SNLBitTerm* term0,
                  naja::NL::SNLBitTerm* term1) {
  auto iit = arcs.find(term0);
  if (iit == arcs.end()) {
    auto result = arcs.insert({term0, naja::NL::SNLDesignModeling::TermArcs()});
    if (not result.second) {
      throw naja::NL::NLException("Error while inserting in timing arcs");
    }
    iit = result.first;
  }
  naja::NL::SNLDesignModeling::TermArcs& termArcs = iit->second;
  auto oit = termArcs.find(term1);
  if (oit != termArcs.end()) {
    throw naja::NL::NLException("Error while inserting in timing arcs");
  }
  termArcs.insert(term1);
}

naja::NL::SNLDesign* verifyInputs(
    const naja::NL::SNLDesignModeling::BitTerms& terms0,
    const std::string& terms0Naming,
    const naja::NL::SNLDesignModeling::BitTerms& terms1,
    const std::string& terms1Naming,
    const std::string& method) {
  if (terms0.empty()) {
    throw naja::NL::NLException("Error in " + method + ": empty " +
                                terms0Naming);
  }
  if (terms1.empty()) {
    throw naja::NL::NLException("Error in " + method + ": empty " +
                                terms1Naming);
  }
  naja::NL::SNLDesign* design = nullptr;
  for (auto term : terms0) {
    if (not design) {
      design = term->getDesign();
    } else if (design not_eq term->getDesign()) {
      throw naja::NL::NLException("Error in " + method +
                                  ": incompatible designs");
    }
  }
  for (auto term : terms1) {
    if (design not_eq term->getDesign()) {
      throw naja::NL::NLException("Error in " + method +
                                  ": incompatible designs");
    }
  }
  return design;
}

#define GET_RELATED_TERMS_IN_ARCS(ARCS)        \
  const auto* timingArcs = getTimingArcs();    \
  const auto it = timingArcs->ARCS.find(term); \
  if (it == timingArcs->ARCS.end()) {          \
    return NajaCollection<SNLBitTerm*>();      \
  }                                            \
  return NajaCollection(new NajaSTLCollection(&(it->second)));

#define GET_RELATED_INSTTERMS_IN_ARCS(ARCS)                   \
  auto instance = iterm->getInstance();                       \
  const TimingArcs* timingArcs = getTimingArcs(instance);     \
  auto it = timingArcs->ARCS.find(iterm->getBitTerm());       \
  if (it == timingArcs->ARCS.end()) {                         \
    return NajaCollection<SNLInstTerm*>();                    \
  }                                                           \
  auto transformer = [=](const SNLBitTerm* term) {            \
    return instance->getInstTerm(term);                       \
  };                                                          \
  return NajaCollection(new NajaSTLCollection(&(it->second))) \
      .getTransformerCollection<SNLInstTerm*>(transformer);

#define GET_RELATED_OBJECTS(TYPE, INPUT, DESIGN_GETTER, GETTER) \
  auto property = getProperty(INPUT->DESIGN_GETTER);            \
  if (property) {                                               \
    auto modeling = property->getModeling();                    \
    return modeling->GETTER(INPUT);                             \
  }                                                             \
  return NajaCollection<TYPE*>();

static const std::string SNLDesignTruthTablePropertyName =
    "SNLDesignTruthTableProperty";

naja::NajaDumpableProperty* getTruthTableProperty(
    const naja::NL::SNLDesign* design) {
  return dynamic_cast<naja::NajaDumpableProperty*>(
      design->getProperty(SNLDesignTruthTablePropertyName));
}

size_t getDependencyChunkCount(const naja::NL::SNLDesign* design) {
  return TT_NCHUNKS_FROM_BITS(design->getBitTerms().size()); }

std::vector<size_t> getInputFlatPositions(const naja::NL::SNLDesign* design) {
  std::vector<size_t> inputFlatPositions;
  size_t flatPos = 0;
  for (auto term : design->getBitTerms()) {
    if (term->getDirection() != naja::NL::SNLTerm::Direction::Output) {
      inputFlatPositions.push_back(flatPos);
    }
    ++flatPos;
  }
  return inputFlatPositions;
}

std::vector<uint64_t> validateAndPadDependenciesForDesign(
    const naja::NL::SNLDesign* design,
    const naja::NL::SNLTruthTable& truthTable) {
  const auto depChunkCount = getDependencyChunkCount(design);
  if (truthTable.size() == 0) {
    return std::vector<uint64_t>(depChunkCount, 0);
  }
  const auto& deps = truthTable.getDependencies();
  if (deps.empty()) {
    throw naja::NL::NLException("Truth table dependencies are required");
  }

  const auto decodedDeps = naja::NL::NLBitDependencies::decodeBits(deps);
  if (decodedDeps.empty()) {
    throw naja::NL::NLException("Truth table dependencies are required");
  }
  // SNLTruthTable constructors already reject oversized dependency sets.
  // LCOV_EXCL_START
  if (decodedDeps.size() > truthTable.size()) {
    throw naja::NL::NLException(
        "Truth table dependencies count cannot exceed truth table size");
  }
  // LCOV_EXCL_STOP

  const auto inputFlatPositions = getInputFlatPositions(design);
  if (inputFlatPositions.empty()) {
    std::vector<uint64_t> paddedDeps(depChunkCount, 0);
    std::copy(deps.begin(), deps.end(), paddedDeps.begin());
    return paddedDeps;
  }
  for (size_t dep : decodedDeps) {
    if (std::find(inputFlatPositions.begin(), inputFlatPositions.end(), dep) ==
        inputFlatPositions.end()) {
      std::ostringstream reason;
      reason << "Truth table dependency " << dep
             << " is not an input flat-term position in design <"
             << design->getName().getString() << ">";
      throw naja::NL::NLException(reason.str());
    }
  }

  if (deps.size() > depChunkCount) {
    std::ostringstream reason;
    reason << "Truth table dependency storage for design <"
           << design->getName().getString() << "> exceeds flat-term span";
    throw naja::NL::NLException(reason.str());
  }

  std::vector<uint64_t> paddedDeps(depChunkCount, 0);
  std::copy(deps.begin(), deps.end(), paddedDeps.begin());
  return paddedDeps;
}

void advanceTruthTableValueIndex(const naja::NL::SNLDesign* design,
                                 size_t& valIdx,
                                 uint32_t nInputs) {
  size_t ttNBits = static_cast<size_t>(1u) << nInputs;
  valIdx += 1;
  valIdx += TT_NCHUNKS_FROM_BITS(ttNBits);
  valIdx += getDependencyChunkCount(design);
}

void createTruthTableProperty(naja::NL::SNLDesign* design,
                              const naja::NL::SNLTruthTable& truthTable) {
  // LCOV_EXCL_START
  if (getTruthTableProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  // LCOV_EXCL_STOP
  const auto deps = validateAndPadDependenciesForDesign(design, truthTable);
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  property->addUInt64Value(truthTable.size());
  for (auto mask : truthTable.bits().getChunks()) {
    property->addUInt64Value(mask);
  }
  for (auto dep : deps) {
    property->addUInt64Value(dep);
  }
}

void createTruthTableProperty(
    naja::NL::SNLDesign* design,
    const std::vector<naja::NL::SNLTruthTable>& truthTables) {
  if (truthTables.empty()) {
    throw naja::NL::NLException("Cannot set empty truth table");
  }
  // LCOV_EXCL_START
  if (getTruthTableProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  // LCOV_EXCL_STOP
  std::vector<std::vector<uint64_t>> depsPerTable;
  depsPerTable.reserve(truthTables.size());
  for (const auto& truthTable : truthTables) {
    depsPerTable.push_back(
        validateAndPadDependenciesForDesign(design, truthTable));
  }
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  for (size_t index = 0; index < truthTables.size(); ++index) {
    const auto& truthTable = truthTables[index];
    property->addUInt64Value(truthTable.size());
    for (auto mask : truthTable.bits().getChunks()) {
      property->addUInt64Value(mask);
    }
    for (auto dep : depsPerTable[index]) {
      property->addUInt64Value(dep);
    }
  }
}

naja::NajaCollection<naja::NL::SNLBitTerm*>
getCombinatorialOutputsDepsFromTruthTable(naja::NL::SNLBitTerm* term) {
  size_t flatID = term->getOrderID();

  // Find the flatID of the term
  auto flatTerms = term->getDesign()->getBitTerms();

  // build map: output‐index → flat‐index
  std::map<size_t, size_t> outputFlatID2FlatID;
  size_t outputID = 0, flatIdx = 0;
  for (auto const& ft : flatTerms) {
    if (ft->getDirection() != naja::NL::SNLTerm::Direction::Input) {
      outputFlatID2FlatID[outputID++] = flatIdx;
    }
    ++flatIdx;
  }

  // turn to vector for std::find_if
  std::vector<naja::NL::SNLBitTerm*> flatTermsVec(flatTerms.begin(),
                                                  flatTerms.end());

  // LCOV_EXCL_START
  // ensure there's at least one truth table
  if (naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign()) == 0) {
    std::ostringstream reason;
    reason << "Design <" << term->getDesign()->getName().getString()
           << "> has no truth table";
    throw naja::NL::NLException(reason.str());
  }

  // LCOV_EXCL_STOP
  auto design = term->getDesign();
  auto property = getTruthTableProperty(design);
  size_t tableIdx = 0;
  std::vector<size_t> flatDepIDs;

  if (property) {
    size_t valIdx = 0;
    size_t total = property->getValues().size();

    while (valIdx < total) {
      uint32_t nInputs =
          static_cast<uint32_t>(property->getUInt64Value(valIdx));
      size_t nBits = static_cast<size_t>(1u) << nInputs;
      size_t nchunks = TT_NCHUNKS_FROM_BITS(nBits);
      size_t ndeps = getDependencyChunkCount(design);

      // collect dependency masks
      std::vector<uint64_t> deps;
      for (size_t i = valIdx + 1 + nchunks; i < valIdx + 1 + nchunks + ndeps;
           ++i) {
        deps.push_back(property->getUInt64Value(i));
      }

      auto decodedDeps = naja::NL::NLBitDependencies::decodeBits(deps);
      if (decodedDeps.empty() ||
          std::find(decodedDeps.begin(), decodedDeps.end(), flatID) !=
              decodedDeps.end()) {
        auto mapIt = outputFlatID2FlatID.find(tableIdx);
        // LCOV_EXCL_START
        if (mapIt == outputFlatID2FlatID.end()) {
          std::ostringstream reason;
          reason << "Output flat ID " << tableIdx << " not found in map";
          throw naja::NL::NLException(reason.str());
        }
        // LCOV_EXCL_STOP
        flatDepIDs.push_back(mapIt->second);
      }

      // advance valIdx past this table's values, masks, and count entry
      advanceTruthTableValueIndex(design, valIdx, nInputs);

      // LCOV_EXCL_START: malformed‐table guard
      if (valIdx >= total + 1) {
        std::ostringstream reason;
        std::string all;
        for (size_t i = 0; i < total; ++i) {
          all += std::to_string(property->getUInt64Value(i)) + " ";
        }
        reason << "Malformed truth table for design <"
               << design->getName().getString() << "> " << all
               << "\nWith valIdx " << valIdx << " and total " << total;
        throw naja::NL::NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      ++tableIdx;
    }
  }

  // build result collection from unique (ID,bit) pairs
  std::set<
      std::pair<naja::NL::NLID::DesignObjectID, naja::NL::NLID::DesignObjectID>>
      depTermsIds;
  for (auto id : flatDepIDs) {
    auto bt = flatTermsVec[id];
    // LCOV_EXCL_START
    if (!bt) {
      std::ostringstream reason;
      reason << "Bit term with ID " << id << " not found in design "
             << term->getDesign()->getName().getString();
      throw naja::NL::NLException(reason.str());
    }
    // LCOV_EXCL_STOP
    depTermsIds.insert({bt->getID(), bt->getBit()});
  }

  auto filter = [=](auto const* bterm) {
    return depTermsIds.count({bterm->getID(), bterm->getBit()}) > 0;
  };
  return flatTerms.getSubCollection(filter);
}

naja::NajaCollection<naja::NL::SNLBitTerm*>
getCombinatorialInputDepsFromTruthTable(naja::NL::SNLBitTerm* term) {
  size_t flatID = term->getOrderID();
  // Find the flatID of the term
  auto flatTerms = term->getDesign()->getBitTerms();
  // create a map for output index and flatID
  std::map<size_t, size_t> outputFlatID2FlatID;

  // turn to vector to use the STL algorithm
  std::vector<naja::NL::SNLBitTerm*> flatTermsVec(flatTerms.begin(),
                                                  flatTerms.end());
  size_t tableCount =
      naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign());
  if (tableCount == 0) {
    return {};
  }
  naja::NL::SNLTruthTable truthTable;
  if (tableCount == 1) {
    truthTable = naja::NL::SNLDesignModeling::getTruthTable(term->getDesign());
  } else {
    truthTable =
        naja::NL::SNLDesignModeling::getTruthTable(term->getDesign(), flatID);
  }
  const auto& deps = truthTable.getDependencies();
  if (naja::NL::NLBitDependencies::countBitsForVector(deps) == 0) {
    // In the absence of any deps, we assume—by default—that everything depends
    // on everything
    return flatTerms.getSubCollection([](const naja::NL::SNLBitTerm* bterm) {
      return bterm->getDirection() != naja::NL::SNLBitTerm::Direction::Output;
    });
  }
  // auto flatTermsIDs = naja::NL::NLBitDependencies::decodeBits(deps);
  // auto flatTerms = term->getDesign()->getBitTerms();
  //  turn to vector to use the STL algorithm
  // std::vector<naja::NL::SNLBitTerm*> flatTermsVec(flatTerms.begin(),
  //                                                 flatTerms.end());

  std::set<
      std::pair<naja::NL::NLID::DesignObjectID, naja::NL::NLID::DesignObjectID>>
      depTermsIds;
  // extract bit values for depsß
  size_t depsIdx = 0;
  for (auto chunk : deps) {
    for (size_t b = 0; b < 64; ++b) {
      size_t pos = depsIdx * 64 + b;
      if (pos >= flatTerms.size())
        break;
      if ((chunk >> b) & 1) {
        auto bt = flatTermsVec[pos];
        // LCOV_EXCL_START
        if (!bt) {
          std::ostringstream reason;
          reason << "Bit term with ID " << pos << " not found in design "
                 << term->getDesign()->getName().getString();
          throw naja::NL::NLException(reason.str());
        }
        // LCOV_EXCL_STOP
        depTermsIds.insert({bt->getID(), bt->getBit()});
      }
    }
    depsIdx++;
  }
  auto filter = [=](const naja::NL::SNLBitTerm* bterm) {
    return depTermsIds.find({bterm->getID(), bterm->getBit()}) !=
           depTermsIds.end();
  };
  return flatTerms.getSubCollection(filter);
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getCombinatorialDepsFromTruthTable(
    naja::NL::SNLBitTerm* term) {
  if (term->getDirection() != naja::NL::SNLTerm::Direction::Input) {
    return getCombinatorialInputDepsFromTruthTable(term);
  }
  return getCombinatorialOutputsDepsFromTruthTable(term);
}

}  // namespace

namespace naja::NL {

SNLDesignModeling::SNLDesignModeling(Type type) : type_(type) {
  if (type_ == NO_PARAMETER) {
    model_ = TimingArcs();
  } else {
    model_ = ParameterizedArcs();
  }
}

void SNLDesignModeling::setTruthTableFromParameter_(
    size_t flatTermID, const ParameterTruthTable& truthTable) {
  std::lock_guard<std::mutex> lock(instanceTruthTablesMutex_);
  if (!parameterTruthTables_.emplace(flatTermID, truthTable).second) {
    throw NLException("Design output already has a parameter-derived truth table");
  }
}

const SNLDesignModeling::ParameterTruthTable*
SNLDesignModeling::getTruthTableFromParameter_(size_t flatTermID) const {
  auto it = parameterTruthTables_.find(flatTermID);
  return it == parameterTruthTables_.end() ? nullptr : &it->second;
}

SNLTruthTable SNLDesignModeling::getTruthTable_(
    const SNLInstance* instance, size_t flatTermID) const {
  std::lock_guard<std::mutex> lock(instanceTruthTablesMutex_);
  auto& instanceTables = instanceTruthTables_[instance];
  if (auto cached = instanceTables.find(flatTermID);
      cached != instanceTables.end()) {
    return cached->second;
  }

  const auto* parameterTruthTable = getTruthTableFromParameter_(flatTermID);
  // LCOV_EXCL_START
  // Public callers only enter this helper after finding the same metadata.
  if (!parameterTruthTable) {
    return SNLTruthTable();
  }
  // LCOV_EXCL_STOP
  std::string value = parameterTruthTable->parameter->getValue();
  if (auto* instanceParameter =
          instance->getInstParameter(parameterTruthTable->parameter->getName())) {
    value = instanceParameter->getValue();
  }
  const auto& defaultTable = parameterTruthTable->defaultTruthTable;
  const uint64_t bits = getParameterTruthTableBits(
      value, defaultTable.size(), parameterTruthTable->parameterBitOffset);
  SNLTruthTable resolvedTable(
      defaultTable.size(), bits, defaultTable.getDependencies());
  return instanceTables.emplace(flatTermID, std::move(resolvedTable))
      .first->second;
}

void SNLDesignModeling::invalidateTruthTableCache_(
    const SNLInstance* instance) const {
  std::lock_guard<std::mutex> lock(instanceTruthTablesMutex_);
  instanceTruthTables_.erase(instance);
}

void SNLDesignModeling::addCombinatorialArc_(SNLBitTerm* input,
                                             SNLBitTerm* output) {
  TimingArcs* arcs = getOrCreateTimingArcs();
  insertInArcs(arcs->inputCombinatorialArcs_, input, output);
  insertInArcs(arcs->outputCombinatorialArcs_, output, input);
}

void SNLDesignModeling::addCombinatorialArc_(
    SNLBitTerm* input,
    SNLBitTerm* output,
    const std::string& parameterValue) {
  TimingArcs* arcs = getOrCreateTimingArcs(parameterValue);
  insertInArcs(arcs->inputCombinatorialArcs_, input, output);
  insertInArcs(arcs->outputCombinatorialArcs_, output, input);
}

void SNLDesignModeling::addInputToClockArc_(SNLBitTerm* input,
                                            SNLBitTerm* clock) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw NLException("Wrong SNLDesignModeling type for addInputToClockArc");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.inputToClockArcs_, input, clock);
  insertInArcs(arcs.clockToInputArcs_, clock, input);
}

void SNLDesignModeling::addClockToOutputArc_(SNLBitTerm* clock,
                                             SNLBitTerm* output) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw NLException("Wrong SNLDesignModeling type for addClockToOutputArc");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.outputToClockArcs_, output, clock);
  insertInArcs(arcs.clockToOutputArcs_, clock, output);
}

SNLDesignModeling::TimingArcs* SNLDesignModeling::getOrCreateTimingArcs(
    const std::string& parameterValue) {
  if (type_ == Type::NO_PARAMETER) {
    if (not parameterValue.empty()) {
      throw NLException("Contradictory type in SNLDesignModeling");
    }
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    std::string paramValue = parameter_.second;
    if (not parameterValue.empty()) {
      paramValue = parameterValue;
    }
    ParameterizedArcs& parameterizedArcs =
        std::get<Type::PARAMETERIZED>(model_);
    auto ait = parameterizedArcs.find(paramValue);
    if (ait == parameterizedArcs.end()) {
      // create it
      auto result = parameterizedArcs.insert({paramValue, TimingArcs()});
      if (not result.second) {
        throw naja::NL::NLException("Error in Timing arcs insertion");
      }
      ait = result.first;
    }
    return &(ait->second);
  }
}

const SNLDesignModeling::TimingArcs* SNLDesignModeling::getTimingArcs(
    const SNLInstance* instance) const {
  if (type_ == Type::NO_PARAMETER) {
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    const ParameterizedArcs& parameterizedArcs =
        std::get<Type::PARAMETERIZED>(model_);
    // find the parameter value
    if (instance != nullptr) {
      // //get Arcs from parameter
      auto parameter = parameter_.first;
      // find parameter in instance
      auto instParameter = instance->getInstParameter(NLName(parameter));
      if (instParameter) {
        auto value = instParameter->getValue();
        auto pit = parameterizedArcs.find(value);
        if (pit == parameterizedArcs.end()) {
          std::ostringstream reason;
          reason << "Parameter value <" << value << "> for Parameter <"
                 << parameter << "> cannot be found in design <"
                 << instance->getModel()->getName().getString()
                 << "> modeling. Existing values are: ";
          bool first = true;
          for (auto parcs : parameterizedArcs) {
            if (!first) {
              reason << ", ";
            }
            reason << parcs.first;
            first = false;
          }
          throw NLException(reason.str());
        }
        return &(pit->second);
      }
      // if not found then switch to default parameter
    }
    auto defaultParameterValue = parameter_.second;
    if (defaultParameterValue.empty()) {
      // LCOV_EXCL_START
      throw NLException("No Default parameter value while getting Timing Arcs");
      // LCOV_EXCL_STOP
    }
    auto ait = parameterizedArcs.find(defaultParameterValue);
    if (ait != parameterizedArcs.end()) {
      return &(ait->second);
    } else {
      // LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot find " << defaultParameterValue
             << " in parameterized arcs.";
      throw NLException(reason.str());
      // LCOV_EXCL_STOP
    }
  }
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(inputCombinatorialArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs_(
    SNLInstTerm* iterm) const {
    GET_RELATED_INSTTERMS_IN_ARCS(inputCombinatorialArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs_(
    SNLBitTerm* term) const {
    GET_RELATED_TERMS_IN_ARCS(outputCombinatorialArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs_(
    SNLInstTerm* iterm) const {
    GET_RELATED_INSTTERMS_IN_ARCS(outputCombinatorialArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedInputs_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(clockToInputArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedOutputs_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(clockToOutputArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getInputRelatedClocks_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(inputToClockArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputRelatedClocks_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(outputToClockArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedInputs_(
    SNLInstTerm* iterm) const {GET_RELATED_INSTTERMS_IN_ARCS(clockToInputArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedOutputs_(
    SNLInstTerm* iterm) const {
    GET_RELATED_INSTTERMS_IN_ARCS(clockToOutputArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getInputRelatedClocks_(
    SNLInstTerm* iterm) const {GET_RELATED_INSTTERMS_IN_ARCS(inputToClockArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getOutputRelatedClocks_(
    SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(outputToClockArcs_)
}

void SNLDesignModeling::addCombinatorialArcs(const BitTerms& inputs,
                                             const BitTerms& outputs) {
  auto design = verifyInputs(inputs, "inputs", outputs, "outputs",
                             "addCombinatorialArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input : inputs) {
    for (auto output : outputs) {
      modeling->addCombinatorialArc_(input, output);
    }
  }
}

void SNLDesignModeling::addCombinatorialArcs(const std::string& parameterValue,
                                             const BitTerms& inputs,
                                             const BitTerms& outputs) {
  auto design = verifyInputs(inputs, "inputs", outputs, "outputs",
                             "addCombinatorialArcs");
  auto property = getOrCreateProperty(design, Type::PARAMETERIZED);
  auto modeling = property->getModeling();
  for (auto input : inputs) {
    for (auto output : outputs) {
      modeling->addCombinatorialArc_(input, output, parameterValue);
    }
  }
}

void SNLDesignModeling::addInputsToClockArcs(const BitTerms& inputs,
                                             SNLBitTerm* clock) {
  auto design =
      verifyInputs(inputs, "inputs", {clock}, "clock", "addInputsToClockArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input : inputs) {
    modeling->addInputToClockArc_(input, clock);
  }
}

void SNLDesignModeling::addClockToOutputsArcs(SNLBitTerm* clock,
                                              const BitTerms& outputs) {
  auto design = verifyInputs({clock}, "clock", outputs, "outputs",
                             "addClockToOutputsArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto output : outputs) {
    modeling->addClockToOutputArc_(clock, output);
  }
}

void SNLDesignModeling::setParameter(SNLDesign* design,
                                     const std::string& name,
                                     const std::string& defaultValue) {
  auto parameter = design->getParameter(NLName(name));
  if (not parameter) {
    std::ostringstream reason;
    reason << "Parameter " << name << " is unknown in "
           << design->getName().getString();
    throw NLException(reason.str());
  }
  auto property = getOrCreateProperty(design, Type::PARAMETERIZED);
  auto modeling = property->getModeling();
  modeling->parameter_ = std::make_pair(name, defaultValue);
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs(
    SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialOutputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign()) > 0) {
      if (naja::NL::SNLDesignModeling::areDependenciesDefined(term)) {
        return getCombinatorialDepsFromTruthTable(term);
      }
    }
  }
  // return all outputs of the design
  return term->getDesign()->getBitTerms().getSubCollection(
      [](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Input;
      });
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs(
    SNLInstTerm* iterm) {
  auto property = getProperty(iterm->getInstance()->getModel());
  if (property) {
    GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(),
                        getCombinatorialOutputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(
            iterm->getBitTerm()->getDesign()) > 0) {
      if (naja::NL::SNLDesignModeling::areDependenciesDefined(
              iterm->getBitTerm())) {
        return getCombinatorialDepsFromTruthTable(iterm->getBitTerm())
              .getTransformerCollection<SNLInstTerm*>(
                  [=](const SNLBitTerm* term) {
                    return iterm->getInstance()->getInstTerm(term);
                  });
      }
    }
  }
  // return all outputs of the instance
  return iterm->getInstance()
      ->getModel()
      ->getBitTerms()
      .getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Input;
      })
      .getTransformerCollection<SNLInstTerm*>([=](const SNLBitTerm* term) {
        return iterm->getInstance()->getInstTerm(term);
      });
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs(
    SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialInputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign()) >
        0) {
      auto snltt = naja::NL::SNLDesignModeling::getTruthTable(
          term->getDesign(), term->getOrderID());
      if (NLBitDependencies::countBitsForVector(snltt.getDependencies()) > 0) {
        return getCombinatorialDepsFromTruthTable(term);
      }
    }
  }
  // return all inputs of the design
  return term->getDesign()->getBitTerms().getSubCollection(
      [](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Output;
      });
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs(
    SNLInstTerm* iterm) {
  auto property = getProperty(iterm->getInstance()->getModel());
  if (property) {
    GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(),
                        getCombinatorialInputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(
            iterm->getInstance()->getModel()) > 0) {
      auto snltt = naja::NL::SNLDesignModeling::getTruthTable(
          iterm->getInstance()->getModel(), iterm->getBitTerm()->getOrderID());
      if (NLBitDependencies::countBitsForVector(snltt.getDependencies()) > 0) {
        return getCombinatorialDepsFromTruthTable(iterm->getBitTerm())
            .getTransformerCollection<SNLInstTerm*>(
                [=](const SNLBitTerm* term) {
                  return iterm->getInstance()->getInstTerm(term);
                });
      }
    }
    // return all inputs of the instance
  }
  return iterm->getInstance()
      ->getModel()
      ->getBitTerms()
      .getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Output;
      })
      .getTransformerCollection<SNLInstTerm*>([=](const SNLBitTerm* term) {
        return iterm->getInstance()->getInstTerm(term);
      });
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedInputs(
    SNLBitTerm* clock){
    if (isDB0SequentialPrimitive(clock->getDesign())) {
      return getDB0ClockRelatedInputs(clock);
    }
    if (hasMemoryInterface(clock->getDesign())) {
      return getMemoryClockRelatedInputs(clock);
    }
    GET_RELATED_OBJECTS(SNLBitTerm, clock, getDesign(), getClockRelatedInputs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedOutputs(
    SNLBitTerm* clock){
                                           if (isDB0SequentialPrimitive(clock->getDesign())) {
                                             return getDB0ClockRelatedOutputs(clock);
                                           }
                                           if (hasMemoryInterface(clock->getDesign())) {
                                             return getMemoryClockRelatedOutputs(clock);
                                           }
                                           GET_RELATED_OBJECTS(SNLBitTerm,
                                           clock,
                                           getDesign(),
                                           getClockRelatedOutputs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getInputRelatedClocks(
    SNLBitTerm* input){
    if (isDB0SequentialPrimitive(input->getDesign())) {
      return getDB0InputRelatedClocks(input);
    }
    if (hasMemoryInterface(input->getDesign())) {
      return getMemoryInputRelatedClocks(input);
    }
    GET_RELATED_OBJECTS(SNLBitTerm, input, getDesign(), getInputRelatedClocks_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputRelatedClocks(
    SNLBitTerm* output){
                                            if (isDB0SequentialPrimitive(output->getDesign())) {
                                              return getDB0OutputRelatedClocks(output);
                                            }
                                            if (hasMemoryInterface(output->getDesign())) {
                                              return getMemoryOutputRelatedClocks(output);
                                            }
                                            GET_RELATED_OBJECTS(SNLBitTerm,
                                            output,
                                            getDesign(),
                                            getOutputRelatedClocks_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedInputs(
    SNLInstTerm* clock){
                                            if (isDB0SequentialPrimitive(clock->getInstance()->getModel())) {
                                              return getDB0ClockRelatedInputs(clock);
                                            }
                                            if (hasMemoryInterface(clock->getInstance()->getModel())) {
                                              return getMemoryClockRelatedInputs(clock);
                                            }
                                            GET_RELATED_OBJECTS(SNLInstTerm,
                                            clock,
                                            getInstance() -> getModel(),
                                            getClockRelatedInputs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedOutputs(
    SNLInstTerm* clock){
                                            if (isDB0SequentialPrimitive(clock->getInstance()->getModel())) {
                                              return getDB0ClockRelatedOutputs(clock);
                                            }
                                            if (hasMemoryInterface(clock->getInstance()->getModel())) {
                                              return getMemoryClockRelatedOutputs(clock);
                                            }
                                            GET_RELATED_OBJECTS(SNLInstTerm,
                                            clock,
                                            getInstance()->getModel(),
                                            getClockRelatedOutputs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getInputRelatedClocks(
    SNLInstTerm* input){
                                            if (isDB0SequentialPrimitive(input->getInstance()->getModel())) {
                                              return getDB0InputRelatedClocks(input);
                                            }
                                            if (hasMemoryInterface(input->getInstance()->getModel())) {
                                              return getMemoryInputRelatedClocks(input);
                                            }
                                            GET_RELATED_OBJECTS(SNLInstTerm,
                                            input,
                                            getInstance()->getModel(),
                                            getInputRelatedClocks_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getOutputRelatedClocks(
    SNLInstTerm* output) {
  if (isDB0SequentialPrimitive(output->getInstance()->getModel())) {
    return getDB0OutputRelatedClocks(output);
  }
  if (hasMemoryInterface(output->getInstance()->getModel())) {
    return getMemoryOutputRelatedClocks(output);
  }
  GET_RELATED_OBJECTS(SNLInstTerm, output, getInstance()->getModel(),
                      getOutputRelatedClocks_)
}

void SNLDesignModeling::setMemoryInterface(
    SNLDesign* design,
    const MemoryInterface& memInterface) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add memory memInterface on non-primitive design");
  }
  validateMemoryInterfaceForDesign(design, memInterface);
  auto property = getOrCreateProperty(design, NO_PARAMETER);
  property->getModeling()->setMemoryInterface_(memInterface);
}

void SNLDesignModeling::setSequentialModel(
    SNLDesign* design,
    const SequentialModel& model) {
  if (!design || !design->isPrimitive()) {
    throw NLException("Cannot add sequential model on non-primitive design");
  }
  if (!model.isValid()) {
    throw NLException("Cannot add invalid sequential model");
  }
  auto property = getOrCreateProperty(design, NO_PARAMETER);
  std::lock_guard<std::mutex> lock(property->getModeling()->instanceSequentialModelsMutex_);
  property->getModeling()->setSequentialModel_(model);
}

bool SNLDesignModeling::hasSequentialModel(const SNLDesign* design) {
  auto property = design ? getProperty(design) : nullptr;
  return property && property->getModeling()->hasSequentialModel_();
}

const SNLDesignModeling::SequentialModel&
SNLDesignModeling::getSequentialModel(const SNLDesign* design) {
  auto property = design ? getProperty(design) : nullptr;
  if (!property || !property->getModeling()->hasSequentialModel_()) {
    throw NLException("Design has no sequential model");
  }
  if (!property->getModeling()->sequentialModels_.empty()) {
    throw NLException("Parameterized sequential model requires an instance: " +
        design->getDescription() + "; use getSequentialModel(instance) to resolve instance overrides and defaults");
  }
  return property->getModeling()->getSequentialModel_();
}

void SNLDesignModeling::setSequentialModelFromParameters(
    SNLDesign* design, const std::vector<std::string>& parameters,
    const SequentialModelTable& models) {
  const std::string context = "SNLDesignModeling::setSequentialModelFromParameters: " +
      (design ? design->getDescription() : std::string("<null design>"));
  auto fail = [&context](const std::string& detail) {
    throw NLException(context + ": " + detail);
  };
  if (!design) fail("cannot attach a model table to a null design");
  if (!design->isPrimitive()) fail("destination must be a primitive design");
  if (parameters.empty()) fail("parameter list is empty; declare at least one numeric selector");
  if (models.empty()) fail("model table is empty; declare at least one parameter-value entry");
  std::set<std::string> names;
  for (size_t i = 0; i < parameters.size(); ++i) {
    const auto& name = parameters[i];
    const auto field = "parameters[" + std::to_string(i) + "] <" + name + ">: ";
    auto* parameter = design->getParameter(NLName(name));
    if (!parameter) fail(field + "parameter is not declared on this primitive");
    if (parameter->getType() == SNLParameter::Type::String) {
      fail(field + "type String is unsupported; expected Binary, Boolean, or Decimal");
    }
    if (!names.insert(name).second) fail(field + "duplicate selector name; each parameter must appear once");
  }
  size_t entryIndex = 0;
  for (const auto& [values, model] : models) {
    std::ostringstream key;
    key << "models[" << entryIndex++ << "] values=[";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i) key << ", ";
      key << (i < parameters.size() ? parameters[i] : "<extra>") << "=" << values[i];
    }
    key << "]: ";
    const auto entry = key.str();
    if (values.size() != parameters.size()) {
      fail(entry + "key has " + std::to_string(values.size()) + " values; expected " +
          std::to_string(parameters.size()) + " in declared parameter order");
    }
    if (model.kind != SequentialModel::Kind::FlipFlop && model.kind != SequentialModel::Kind::Latch) {
      fail(entry + "invalid kind enum value " + std::to_string(static_cast<int>(model.kind)) +
          "; expected FlipFlop or Latch");
    }
    if (model.states.empty()) fail(entry + "states is empty; at least one state is required");
    if (model.outputs.empty()) fail(entry + "outputs is empty; at least one output is required");
    auto validateExpression = [&](const BooleanExpression& expression, const std::string& field) {
      const auto prefix = entry + field + ": ";
      if (!expression.isValid()) {
        fail(prefix + "root node index " + std::to_string(expression.root) +
            " is outside node array of size " + std::to_string(expression.nodes.size()));
      }
      for (size_t i = 0; i < expression.nodes.size(); ++i) {
        const auto& node = expression.nodes[i];
        const auto nodeContext = prefix + "nodes[" + std::to_string(i) + "]: ";
        using Op = BooleanExpression::Operator;
        if (node.operation < Op::Constant || node.operation > Op::Xor) {
          fail(nodeContext + "invalid operator enum value " + std::to_string(static_cast<int>(node.operation)) +
              "; expected Constant, Term, State, Not, And, Or, or Xor");
        }
        const char* operatorNames[] = {"Constant", "Term", "State", "Not", "And", "Or", "Xor"};
        const bool leaf = node.operation == Op::Constant || node.operation == Op::Term || node.operation == Op::State;
        const size_t expectedOperands = leaf ? 0 : node.operation == Op::Not ? 1 : 2;
        if (node.operands.size() != expectedOperands) {
          fail(nodeContext + "operator " + operatorNames[static_cast<size_t>(node.operation)] +
              " has " + std::to_string(node.operands.size()) + " operands; expected " + std::to_string(expectedOperands));
        }
        if (node.operation == Op::Term && !node.term) fail(nodeContext + "Term node has a null term");
        if (node.operation == Op::Term && node.term->getDesign() != design) {
          fail(nodeContext + "term " + node.term->getDescription() + " belongs to " +
              node.term->getDesign()->getDescription() + "; expected the destination primitive");
        }
        if (node.operation == Op::State && node.state >= model.states.size()) {
          fail(nodeContext + "state index " + std::to_string(node.state) + " is outside states array of size " +
              std::to_string(model.states.size()));
        }
        for (size_t j = 0; j < node.operands.size(); ++j) {
          if (node.operands[j] >= i) {
            fail(nodeContext + "operands[" + std::to_string(j) + "]=" + std::to_string(node.operands[j]) +
                " must refer to an earlier node (index < " + std::to_string(i) + "); cyclic/forward references are unsupported");
          }
        }
      }
    };
    validateExpression(model.clockedOn, "clocked_on");
    for (size_t i = 0; i < model.states.size(); ++i) {
      const auto& state = model.states[i];
      const auto field = "states[" + std::to_string(i) + "]";
      if (state.clearPresetValue < SequentialState::ClearPresetValue::Zero ||
          state.clearPresetValue > SequentialState::ClearPresetValue::Unknown) {
        fail(entry + field + ".clear_preset_value: invalid enum value " +
            std::to_string(static_cast<int>(state.clearPresetValue)) + "; expected Zero, One, Hold, Toggle, or Unknown");
      } // LCOV_EXCL_LINE: unreachable fallthrough after the throwing diagnostic.
      validateExpression(state.nextState, field + ".next_state"); // LCOV_EXCL_LINE: Clang attributes the preceding diagnostic's unreachable fallthrough here; validation is tested.
      if (state.clear) validateExpression(*state.clear, field + ".clear");
      if (state.preset) validateExpression(*state.preset, field + ".preset");
    }
    std::set<SNLBitTerm*> outputs;
    for (size_t i = 0; i < model.outputs.size(); ++i) {
      const auto& output = model.outputs[i];
      const auto field = "outputs[" + std::to_string(i) + "]";
      if (!output.term) fail(entry + field + ": output term is null");
      if (output.term->getDesign() != design) {
        fail(entry + field + ": term " + output.term->getDescription() + " belongs to " +
            output.term->getDesign()->getDescription() + "; expected the destination primitive");
      }
      if (output.term->getDirection() != SNLTerm::Direction::Output) {
        fail(entry + field + ": term " + output.term->getDescription() + "; expected direction Output");
      }
      if (!outputs.insert(output.term).second) fail(entry + field + ": duplicate output term " + output.term->getDescription());
      validateExpression(output.function, field + ".function");
    }
  }
  auto* modeling = getOrCreateProperty(design, NO_PARAMETER)->getModeling();
  std::lock_guard<std::mutex> lock(modeling->instanceSequentialModelsMutex_);
  modeling->instanceSequentialModels_.clear();
  modeling->sequentialModel_.reset();
  modeling->sequentialParameters_ = parameters;
  modeling->sequentialModels_ = models;
}

bool SNLDesignModeling::hasSequentialModelFromParameters(const SNLDesign* design) {
  auto* property = design ? getProperty(design) : nullptr;
  return property && !property->getModeling()->sequentialModels_.empty();
}

const SNLDesignModeling::SequentialModel&
SNLDesignModeling::getSequentialModel(const SNLInstance* instance) {
  if (!instance) throw NLException("SNLDesignModeling::getSequentialModel(instance): null instance; expected a live SNLInstance");
  auto* design = instance->getModel();
  if (!hasSequentialModelFromParameters(design)) {
    if (!hasSequentialModel(design)) {
      throw NLException("SNLDesignModeling::getSequentialModel: " + instance->getDescription() +
          "; primitive " + design->getDescription() + " has no sequential model; attach a static model or parameter table first");
    }
    return getSequentialModel(design);
  }
  auto* modeling = getProperty(design)->getModeling();
  std::lock_guard<std::mutex> lock(modeling->instanceSequentialModelsMutex_);
  std::vector<std::string> literals;
  for (const auto& name : modeling->sequentialParameters_) {
    auto* parameter = design->getParameter(NLName(name));
    auto* override = instance->getInstParameter(NLName(name));
    if (!parameter || (!override && !parameter->hasDefaultValue())) {
      throw NLException("Missing required sequential model parameter <" + name +
          "> on instance <" + instance->getName().getString() + ">; " + instance->getDescription() +
          "; primitive " + design->getDescription() + "; " +
          (!parameter ? "selector declaration was removed from the primitive" :
           "no instance override and no parameter default; create an SNLInstParameter override or declare a default"));
    }
    literals.push_back(override ? override->getValue() : parameter->getValue());
  }
  auto cached = modeling->instanceSequentialModels_.find(instance);
  if (cached != modeling->instanceSequentialModels_.end() &&
      cached->second.values == literals) return *cached->second.model;
  std::vector<uint64_t> values;
  for (size_t i = 0; i < literals.size(); ++i) {
    try {
      values.push_back(parseTruthTableParameterValue(literals[i], 1));
    } catch (const NLException& error) {
      std::string cause = error.what();
      for (const std::string label : {"truth-table", "Truth-table"}) {
        size_t position = 0;
        while ((position = cause.find(label, position)) != std::string::npos) {
          cause.replace(position, label.size(), "numeric");
          position += 7;
        }
      }
      throw NLException("Sequential model parameter <" + modeling->sequentialParameters_[i] + "> on instance <" +
          instance->getName().getString() + ">; " + instance->getDescription() +
          "; primitive " + design->getDescription() + "; raw value <" + literals[i] +
          "> from " + (instance->getInstParameter(NLName(modeling->sequentialParameters_[i])) ?
          "instance override" : "parameter default") + "; expected an unsigned value representable in 64 bits: " + cause);
    }
  }
  auto selected = modeling->sequentialModels_.find(values);
  if (selected == modeling->sequentialModels_.end()) {
    std::ostringstream reason;
    reason << "No sequential model matches parameters on instance <" << instance->getName().getString()
        << ">; " << instance->getDescription() << "; primitive " << design->getDescription() << "; resolved values=[";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i) reason << ", ";
      reason << modeling->sequentialParameters_[i] << "=" << values[i] << " (raw <" << literals[i] << ">, "
          << (instance->getInstParameter(NLName(modeling->sequentialParameters_[i])) ? "override" : "default") << ")";
    }
    reason << "]; available keys in declared parameter order=[";
    bool first = true;
    for (const auto& [key, model] : modeling->sequentialModels_) {
      if (!first) reason << ", ";
      first = false;
      reason << "[";
      for (size_t i = 0; i < key.size(); ++i) {
        if (i) reason << ", ";
        reason << modeling->sequentialParameters_[i] << "=" << key[i];
      }
      reason << "]";
    }
    reason << "]; declare a matching table entry or correct the instance parameter values";
    throw NLException(reason.str());
  }
  modeling->instanceSequentialModels_[instance] = {std::move(literals), &selected->second};
  return selected->second;
}

void SNLDesignModeling::invalidateSequentialModelCache(const SNLInstance* instance) {
  if (!instance) return;
  if (auto* property = getProperty(instance->getModel())) {
    auto* modeling = property->getModeling();
    std::lock_guard<std::mutex> lock(modeling->instanceSequentialModelsMutex_);
    modeling->instanceSequentialModels_.erase(instance);
  }
}

void SNLDesignModeling::setRolesFromParameters(
    SNLDesign* design, const std::vector<std::string>& parameters,
    const TermRoleTable& roles) {
  const std::string context = "SNLDesignModeling::setRolesFromParameters: " +
      (design ? design->getDescription() : "<null design>");
  auto fail = [&context](const std::string& detail) {
    throw NLException(context + ": " + detail);
  };
  if (!design || !design->isPrimitive()) fail("destination must be a primitive design");
  if (parameters.empty() || roles.empty()) fail("parameter list and role table must be nonempty");
  std::set<std::string> names;
  for (const auto& name : parameters) {
    auto* parameter = design->getParameter(NLName(name));
    if (!parameter) fail("unknown selector parameter <" + name + ">");
    if (parameter->getType() == SNLParameter::Type::String) {
      fail("selector <" + name + "> must be Binary, Boolean, or Decimal");
    }
    if (!names.insert(name).second) fail("duplicate selector parameter <" + name + ">");
  }
  for (const auto& [values, entry] : roles) {
    if (values.size() != parameters.size()) fail("values must match the declared parameter order and count");
    for (const auto& [term, role] : entry) {
      if (!term || term->getDesign() != design) fail("role term must belong to the destination primitive");
      if (role.role < SNLTermRole::Clock || role.role > SNLTermRole::ScanEnable) fail("invalid SNLTermRole");
      if (role.activeLevel < SNLActiveLevel::High || role.activeLevel > SNLActiveLevel::NA) fail("invalid SNLActiveLevel");
    }
  }
  auto* modeling = getOrCreateProperty(design, NO_PARAMETER)->getModeling();
  modeling->roleParameters_ = parameters;
  modeling->parameterRoles_ = roles;
}

bool SNLDesignModeling::hasRolesFromParameters(const SNLDesign* design) {
  auto* property = design ? getProperty(design) : nullptr;
  return property && !property->getModeling()->parameterRoles_.empty();
}

SNLDesignModeling::TermRole SNLDesignModeling::getTermRole_(const SNLInstTerm* term) const {
  auto* instance = term->getInstance();
  auto* design = instance->getModel();
  const std::string context = "Term roles on " + instance->getDescription() +
      "; primitive " + design->getDescription();
  std::vector<uint64_t> values;
  for (const auto& name : roleParameters_) {
    auto* parameter = design->getParameter(NLName(name));
    auto* override = instance->getInstParameter(NLName(name));
    if (!parameter || (!override && !parameter->hasDefaultValue())) {
      throw NLException(context + ": missing required role parameter <" + name + ">");
    }
    const auto& literal = override ? override->getValue() : parameter->getValue();
    try {
      values.push_back(parseTruthTableParameterValue(literal, 1));
    } catch (const NLException& error) {
      throw NLException(context + ": invalid role parameter <" + name + "> raw value <" + literal +
          "> from " + (override ? "instance override" : "parameter default") + ": " + error.what());
    }
  }
  auto selected = parameterRoles_.find(values);
  if (selected == parameterRoles_.end()) {
    std::ostringstream reason;
    reason << context << ": no term roles match resolved parameters [";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i) reason << ", ";
      reason << roleParameters_[i] << "=" << values[i];
    }
    reason << "]; declare a matching role table entry";
    throw NLException(reason.str());
  }
  auto role = selected->second.find(term->getBitTerm());
  if (role != selected->second.end()) return role->second;
  return {getTermRole(term->getBitTerm()), getResetActiveLevel(term->getBitTerm())};
}

void SNLDesignModeling::setTermRole(
    SNLBitTerm* term, SNLTermRole role, SNLActiveLevel activeLevel) {
  if (!term || !term->getDesign()->isLeaf()) return;
  auto property = getOrCreateProperty(term->getDesign(), NO_PARAMETER);
  property->getModeling()->setTermRole_(term, role, activeLevel);
}

namespace {
SNLDesignModeling::SNLTermRole getMemoryTermRole(
    const SNLBitTerm* term,
    const SNLDesignModeling::MemoryInterface& memory) {
  using Role = SNLDesignModeling::SNLTermRole;
  using ResetMode = SNLDesignModeling::MemoryResetMode;
  if (isMemoryClockTerm(memory, term)) return Role::Clock;
  if (term == memory.reset &&
      (memory.resetMode == ResetMode::AsyncLow ||
       memory.resetMode == ResetMode::AsyncHigh)) {
    return Role::AsyncReset;
  }
  if (term == memory.reset &&
      (memory.resetMode == ResetMode::SyncLow ||
       memory.resetMode == ResetMode::SyncHigh)) {
    return Role::SyncReset;
  }
  auto contains = [term](const SNLDesignModeling::BitTerms& terms) {
    return std::find(terms.begin(), terms.end(), term) != terms.end();
  };
  for (const auto& port : memory.readPorts) {
    if (contains(port.address)) return Role::MemoryReadAddress;
    if (contains(port.data)) return Role::MemoryReadData;
    if (contains(port.enables)) return Role::Enable;
  }
  for (const auto& port : memory.writePorts) {
    if (contains(port.address)) return Role::MemoryWriteAddress;
    if (contains(port.data)) return Role::MemoryWriteData;
    if (contains(port.mask) || contains(port.enables)) {
      return Role::MemoryWriteEnable;
    }
  }
  return Role::Other;
}
}

SNLDesignModeling::SNLTermRole SNLDesignModeling::getTermRole(
    const SNLBitTerm* term) {
  if (!term) return SNLTermRole::Other;
  auto* design = term->getDesign();
  if (hasMemoryInterface(design)) {
    return getMemoryTermRole(term, getMemoryInterface(design));
  }
  if (auto property = getProperty(design)) {
    return property->getModeling()->getTermRole_(term).role;
  }
  return SNLTermRole::Other;
}

SNLDesignModeling::SNLTermRole SNLDesignModeling::getTermRole(
    const SNLInstTerm* term) {
  if (!term) return SNLTermRole::Other;
  if (hasRolesFromParameters(term->getBitTerm()->getDesign())) {
    return getProperty(term->getBitTerm()->getDesign())->getModeling()->getTermRole_(term).role;
  }
  return getTermRole(term->getBitTerm());
}

SNLDesignModeling::SNLActiveLevel SNLDesignModeling::getResetActiveLevel(
    const SNLBitTerm* term) {
  auto role = getTermRole(term);
  if (role != SNLTermRole::AsyncReset && role != SNLTermRole::AsyncSet &&
      role != SNLTermRole::SyncReset && role != SNLTermRole::SyncSet) {
    return SNLActiveLevel::NA;
  }
  if (term && hasMemoryInterface(term->getDesign())) {
    auto memory = getMemoryInterface(term->getDesign());
    if (term == memory.reset) {
      return (memory.resetMode == MemoryResetMode::AsyncLow ||
              memory.resetMode == MemoryResetMode::SyncLow)
          ? SNLActiveLevel::Low : SNLActiveLevel::High;
    }
  }
  if (term) {
    if (auto property = getProperty(term->getDesign())) {
      return property->getModeling()->getTermRole_(term).activeLevel;
    }
  }
  return SNLActiveLevel::NA; // LCOV_EXCL_LINE defensive inconsistent role fallback
}

SNLDesignModeling::SNLActiveLevel SNLDesignModeling::getResetActiveLevel(
    const SNLInstTerm* term) {
  if (!term) return SNLActiveLevel::NA;
  if (hasRolesFromParameters(term->getBitTerm()->getDesign())) {
    const auto role = getProperty(term->getBitTerm()->getDesign())->getModeling()->getTermRole_(term);
    return role.role == SNLTermRole::AsyncReset || role.role == SNLTermRole::AsyncSet ||
        role.role == SNLTermRole::SyncReset || role.role == SNLTermRole::SyncSet
        ? role.activeLevel : SNLActiveLevel::NA;
  }
  return getResetActiveLevel(term->getBitTerm());
}

#define DEFINE_TERM_ROLE_PREDICATE(NAME, ROLE)                    \
  bool SNLDesignModeling::NAME(const SNLBitTerm* term) {          \
    return getTermRole(term) == SNLTermRole::ROLE;                \
  }                                                               \
  bool SNLDesignModeling::NAME(const SNLInstTerm* term) {         \
    return getTermRole(term) == SNLTermRole::ROLE;                \
  }

DEFINE_TERM_ROLE_PREDICATE(isClock, Clock)
DEFINE_TERM_ROLE_PREDICATE(isAsyncReset, AsyncReset)
DEFINE_TERM_ROLE_PREDICATE(isAsyncSet, AsyncSet)
DEFINE_TERM_ROLE_PREDICATE(isSyncReset, SyncReset)
DEFINE_TERM_ROLE_PREDICATE(isSyncSet, SyncSet)
DEFINE_TERM_ROLE_PREDICATE(isEnable, Enable)
#undef DEFINE_TERM_ROLE_PREDICATE

bool SNLDesignModeling::isReset(const SNLBitTerm* term) {
  return isAsyncReset(term) || isSyncReset(term);
}
bool SNLDesignModeling::isReset(const SNLInstTerm* term) {
  return isAsyncReset(term) || isSyncReset(term);
}
bool SNLDesignModeling::isDataInput(const SNLBitTerm* term) {
  auto role = getTermRole(term);
  return role == SNLTermRole::DataInput || role == SNLTermRole::MemoryWriteData;
}
bool SNLDesignModeling::isDataInput(const SNLInstTerm* term) {
  auto role = getTermRole(term);
  return role == SNLTermRole::DataInput || role == SNLTermRole::MemoryWriteData;
}
bool SNLDesignModeling::isDataOutput(const SNLBitTerm* term) {
  auto role = getTermRole(term);
  return role == SNLTermRole::DataOutput || role == SNLTermRole::MemoryReadData;
}
bool SNLDesignModeling::isDataOutput(const SNLInstTerm* term) {
  auto role = getTermRole(term);
  return role == SNLTermRole::DataOutput || role == SNLTermRole::MemoryReadData;
}

namespace {
NajaCollection<SNLBitTerm*> getTermsWithRole(
    const SNLDesign* design, SNLDesignModeling::SNLTermRole role) {
  if (!design) return NajaCollection<SNLBitTerm*>();
  if (SNLDesignModeling::hasMemoryInterface(design)) {
    auto memory = SNLDesignModeling::getMemoryInterface(design);
    return design->getBitTerms().getSubCollection(
        [role, memory = std::move(memory)](const SNLBitTerm* term) {
          return getMemoryTermRole(term, memory) == role;
        });
  }
  return design->getBitTerms().getSubCollection([role](const SNLBitTerm* term) {
    return SNLDesignModeling::getTermRole(term) == role;
  });
}
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::Clock);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getAsyncResetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::AsyncReset);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getAsyncSetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::AsyncSet);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getSyncResetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::SyncReset);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getSyncSetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::SyncSet);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getDataInputTerms(const SNLDesign* design) {
  if (!design) return NajaCollection<SNLBitTerm*>();
  if (hasMemoryInterface(design)) {
    auto memory = getMemoryInterface(design);
    return design->getBitTerms().getSubCollection(
        [memory = std::move(memory)](const SNLBitTerm* term) {
          return getMemoryTermRole(term, memory) == SNLTermRole::MemoryWriteData;
        });
  }
  return design->getBitTerms().getSubCollection([](const SNLBitTerm* term) {
    return SNLDesignModeling::isDataInput(term);
  });
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputTerms(const SNLDesign* design) {
  if (!design) return NajaCollection<SNLBitTerm*>();
  if (hasMemoryInterface(design)) {
    auto memory = getMemoryInterface(design);
    return design->getBitTerms().getSubCollection(
        [memory = std::move(memory)](const SNLBitTerm* term) {
          return getMemoryTermRole(term, memory) == SNLTermRole::MemoryReadData;
        });
  }
  return design->getBitTerms().getSubCollection([](const SNLBitTerm* term) {
    return SNLDesignModeling::isDataOutput(term);
  });
}

bool SNLDesignModeling::hasMemoryInterface(const SNLDesign* design) {
  if (!design) {
    return false;
  }
  if (auto property = getProperty(design)) {
    if (property->getModeling()->hasMemoryInterface_()) {
      return true;
    }
  }
  return NLDB0::isMemory(design);
}

SNLDesignModeling::MemoryInterface SNLDesignModeling::getMemoryInterface(
    const SNLDesign* design) {
  if (!design) {
    throw NLException("SNLDesignModeling::getMemoryInterface: null design");
  }
  if (auto property = getProperty(design)) {
    if (property->getModeling()->hasMemoryInterface_()) {
      return property->getModeling()->getMemoryInterface_();
    }
  }
  if (NLDB0::isMemory(design)) {
    return buildDB0MemoryInterface(design);
  }
  throw NLException(
      "SNLDesignModeling::getMemoryInterface: design has no memory memInterface");
}

SNLDesignModeling::MemoryInterface SNLDesignModeling::getMemoryInterface(
    const SNLInstance* instance) {
  if (!instance) {
    throw NLException("SNLDesignModeling::getMemoryInterface: null instance");
  }
  auto memInterface = getMemoryInterface(instance->getModel());
  if (NLDB0::isMemory(instance->getModel())) {
    const auto signature = NLDB0::getMemorySignature(instance);
    memInterface.width = signature.width;
    memInterface.depth = signature.depth;
    memInterface.abits = signature.abits;
    memInterface.resetMode = convertMemoryResetMode(signature.resetMode);
  }
  // Elaboration may instantiate a generic memory primitive with only a subset
  // of ports connected. Return the effective instance memInterface, not the full
  // primitive declaration, so clients only model observable/driveable ports.
  if (!isConnectedInstanceBitTerm(instance, memInterface.clock)) {
    memInterface.clock = nullptr;
  }
  if (memInterface.reset && !isConnectedInstanceBitTerm(instance, memInterface.reset)) {
    memInterface.reset = nullptr;
  }
  std::vector<MemoryReadPort> connectedReadPorts;
  connectedReadPorts.reserve(memInterface.readPorts.size());
  for (const auto& readPort : memInterface.readPorts) {
    if (isConnectedReadPort(instance, readPort)) {
      connectedReadPorts.push_back(readPort);
    }
  }
  memInterface.readPorts = std::move(connectedReadPorts);
  std::vector<MemoryWritePort> connectedWritePorts;
  connectedWritePorts.reserve(memInterface.writePorts.size());
  for (const auto& writePort : memInterface.writePorts) {
    if (isConnectedWritePort(instance, writePort)) {
      connectedWritePorts.push_back(writePort);
    }
  }
  memInterface.writePorts = std::move(connectedWritePorts);
  return memInterface;
}

void SNLDesignModeling::setTruthTableFromParameter(
    SNLDesign* design,
    SNLBitTerm* output,
    const BitTerms& inputs,
    SNLParameter* parameter,
    size_t parameterBitOffset) {
  if (!design || !output || !parameter) {
    throw NLException(
        "SNLDesignModeling::setTruthTableFromParameter: null argument");
  }
  if (!design->isPrimitive()) {
    throw NLException(
        "Cannot add parameter-derived truth table on non-primitive design");
  }
  if (output->getDesign() != design ||
      output->getDirection() == SNLTerm::Direction::Input) {
    throw NLException(
        "Parameter-derived truth table output is not an output of the design");
  }
  if (parameter->getDesign() != design) {
    throw NLException(
        "Parameter-derived truth table parameter belongs to another design");
  }
  if (inputs.empty() || inputs.size() > 6) {
    throw NLException(
        "Parameter-derived truth tables require between 1 and 6 inputs");
  }
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }

  std::vector<size_t> dependencies;
  dependencies.reserve(inputs.size());
  for (auto* input : inputs) {
    if (!input || input->getDesign() != design ||
        input->getDirection() == SNLTerm::Direction::Output) {
      throw NLException(
          "Parameter-derived truth table input is not an input of the design");
    }
    if (!dependencies.empty() &&
        input->getOrderID() <= dependencies.back()) {
      throw NLException(
          "Parameter-derived truth table inputs must follow design order");
    }
    dependencies.push_back(input->getOrderID());
  }

  const uint64_t defaultBits = getParameterTruthTableBits(
      parameter->getValue(), inputs.size(), parameterBitOffset);
  ParameterTruthTable parameterTruthTable{
      parameter,
      parameterBitOffset,
      SNLTruthTable(
          inputs.size(), defaultBits,
          NLBitDependencies::encodeBits(dependencies))};
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  if (property->getModelingType() != Type::NO_PARAMETER) {
    throw NLException(
        "Parameter-derived truth tables require non-parameterized timing arcs");
  }
  auto* modeling = property->getModeling();
  modeling->setTruthTableFromParameter_(
      output->getOrderID(), parameterTruthTable);
  for (auto* input : inputs) {
    modeling->addCombinatorialArc_(input, output);
  }
}

void SNLDesignModeling::setTruthTable(SNLDesign* design,
                                      const SNLTruthTable& truthTable) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  if (auto property = getProperty(design);
      property &&
      !property->getModeling()->parameterTruthTables_.empty()) {
    throw NLException("Design already has a parameter-derived Truth Table");
  }
  const auto& outputs =
      design->getBitTerms().getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() == SNLTerm::Direction::Output;
      });
  if (outputs.size() != 1) {
    std::ostringstream reason;
    reason << "cannot add truth table on Design <"
           << design->getName().getString() << "> that has <" << outputs.size()
           << "> outputs";
    throw NLException(reason.str());
  }
  createTruthTableProperty(design, truthTable);
}

void SNLDesignModeling::setTruthTables(
    SNLDesign* design,
    const std::vector<SNLTruthTable>& truthTables) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  if (auto property = getProperty(design);
      property &&
      !property->getModeling()->parameterTruthTables_.empty()) {
    throw NLException("Design already has a parameter-derived Truth Table");
  }
  const auto& outputs =
      design->getBitTerms().getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Input;
      });
  if (outputs.size() != truthTables.size()) {
    std::ostringstream reason;
    reason << "cannot add truth tables on Design <"
           << design->getName().getString() << "> that has <" << outputs.size()
           << "> outputs, but provided <" << truthTables.size()
           << "> truth tables";
    throw NLException(reason.str());
  }
  createTruthTableProperty(design, truthTables);
}

bool SNLDesignModeling::areDependenciesDefined(const SNLBitTerm* term) {
  auto design = term->getDesign();
  if (SNLDesignModeling::getTruthTableCount(design) == 0) {
    return false;
  }

  if (term->getDirection() != SNLTerm::Direction::Input) {
    SNLTruthTable tt =
        SNLDesignModeling::getTruthTable(design, term->getOrderID());
    return true;
  }

  size_t flatID = term->getOrderID();
  bool foundExplicitDependencies = false;
  for (const auto& candidate : design->getBitTerms()) {
    if (candidate->getDirection() == SNLTerm::Direction::Input) {
      continue;
    }
    SNLTruthTable tt =
        SNLDesignModeling::getTruthTable(design, candidate->getOrderID());
    const auto& deps = tt.getDependencies();
    if (naja::NL::NLBitDependencies::countBitsForVector(deps) == 0) {
      return false;
    }
    foundExplicitDependencies = true;
    auto decodedDeps = naja::NL::NLBitDependencies::decodeBits(deps);
    if (std::find(decodedDeps.begin(), decodedDeps.end(), flatID) !=
        decodedDeps.end()) {
      return true;
    }
  }
  return foundExplicitDependencies;
}

size_t SNLDesignModeling::getTruthTableCount(const SNLDesign* design) {
  if (NLDB0::isDB0Primitive(design)) {
    if (isDB0SequentialPrimitive(design)) {
      return 0;
    }
    // A read-only memory is combinational: one table per read-data bit.
    const bool readOnlyMemory = NLDB0::isMemory(design) &&
        NLDB0::getMemorySignature(design).writePorts == 0;
    if (NLDB0::isMemory(design) && !readOnlyMemory) {
      return 0;
    }
    if (readOnlyMemory ||
        NLDB0::isVectorSelect(design) || NLDB0::isVectorUpdate(design) || NLDB0::isMux2(design) || NLDB0::isTableSelect(design) ||
        NLDB0::isDivMod(design) || NLDB0::isShift(design) ||
        NLDB0::isMultiply(design) || NLDB0::isAddSub(design)) {
      size_t tableCount = 0;
      for (const auto* term : design->getBitTerms()) {
        if (term->getDirection() != SNLTerm::Direction::Input) {
          ++tableCount;
        }
      }
      return tableCount;
    }
    size_t tableCount = 0;
    for (const auto* term : design->getBitTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Input) {
        continue;
      }
      auto tt = getTruthTable(design, term->getOrderID());
      if (!tt.isNull()) {
        ++tableCount;
      }
    }
    return tableCount;
  }
  if (auto modelingProperty = getProperty(design)) {
    const auto parameterTableCount =
        modelingProperty->getModeling()->parameterTruthTables_.size();
    if (parameterTableCount != 0) {
      return parameterTableCount;
    }
  }
  auto property = getTruthTableProperty(design);
  size_t tableIdx = 0;
  if (property != nullptr) {
    size_t valIdx = 0;
    size_t total = property->getValues().size();

    if (total == 0) {
      return 0;
    }

    while (valIdx < total) {
      uint32_t nInputs =
          static_cast<uint32_t>(property->getUInt64Value(valIdx));
      advanceTruthTableValueIndex(design, valIdx, nInputs);

      // LCOV_EXCL_START
      if (valIdx >= total + 1) {
        std::ostringstream reason;
        std::string result;
        for (size_t i = 0; i < total; ++i)
          result += std::to_string(property->getUInt64Value(i)) + " ";
        reason << "Maldformed truth table for design <"
               << design->getName().getString() << "> " << result
               << "\nWith valIdx " << valIdx << " and total " << total;
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      ++tableIdx;
    }
  }
  return tableIdx;
}

SNLTruthTable SNLDesignModeling::getTruthTable(const SNLDesign* design) {
  if (NLDB0::isDB0Primitive(design)) {
    if (isDB0SequentialPrimitive(design) || NLDB0::isMemory(design) ||
        NLDB0::isShift(design) || NLDB0::isMultiply(design) || NLDB0::isAddSub(design) ||
        NLDB0::isDivMod(design) || NLDB0::isVectorSelect(design) || NLDB0::isVectorUpdate(design)) {
      return SNLTruthTable();
    }
    if (NLDB0::isTableSelect(design)) {
      const SNLBitTerm* outputTerm = nullptr;
      for (auto* term : design->getBitTerms()) {
        if (term->getDirection() != SNLTerm::Direction::Input) {
          if (outputTerm != nullptr) {
            throw NLException(
                "SNLDesignModeling::getTruthTable: table select has per-output truth tables");
          }
          outputTerm = term;
        }
      }
      return outputTerm ? NLDB0::getTableSelectTruthTable(
                              design, outputTerm->getOrderID())
                        : SNLTruthTable();
    }
    return NLDB0::getPrimitiveTruthTable(design);
  }
  if (auto modelingProperty = getProperty(design)) {
    const auto& parameterTruthTables =
        modelingProperty->getModeling()->parameterTruthTables_;
    if (parameterTruthTables.size() == 1) {
      return parameterTruthTables.begin()->second.defaultTruthTable;
    }
    if (parameterTruthTables.size() > 1) {
      throw NLException(
          "SNLDesignModeling::getTruthTable: design has per-output truth tables");
    }
  }
  auto property = getTruthTableProperty(design);
  if (property) {
    size_t tableSize = property->getValues().size() - 1;
    uint64_t declaredInputs = property->getUInt64Value(0);
    uint64_t num_bits = 1u << declaredInputs;
    size_t expectedChunks =
        TT_NCHUNKS_FROM_BITS(num_bits) + getDependencyChunkCount(design);
    if (expectedChunks != tableSize) {
      std::ostringstream reason;
      reason << "Truth table size " << tableSize
             << " does not match number of chunks " << expectedChunks
             << " which suggests per output functionality";
      throw NLException(reason.str());
    }

    // multi‐chunk (>64‐bit) table?
    if (property->getValues().size() > 3) {
      uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
      uint32_t nBits = 1u << numInputs;
      // LCOV_EXCL_START
      if (nBits <= 64) {
        std::ostringstream reason;
        reason << "Truth table size " << nBits << " is not larger than 64 bits";
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      size_t nChunks = TT_NCHUNKS_FROM_BITS(nBits);
      size_t bitsIdx = 1;  // skip the size entry
      std::vector<bool> bits(nBits, false);
      TT_FILL_BITS(bits, nBits, property, bitsIdx);

      size_t depsIdx = bitsIdx + nChunks;
      std::vector<uint64_t> deps;
      size_t nDeps = getDependencyChunkCount(design);
      for (size_t i = depsIdx; i < depsIdx + nDeps; ++i) {
        if (i < property->getValues().size()) {
          deps.push_back(property->getUInt64Value(i));
        } else {
          // LCOV_EXCL_START
          std::ostringstream reason;
          reason << "Truth table size " << property->getValues().size()
                 << " is smaller then requested index " << i;
          throw NLException(reason.str());
          // LCOV_EXCL_STOP
        }
      }
      return SNLTruthTable(numInputs, bits,
                           numInputs == 0 ? std::vector<uint64_t>{} : deps);
    }

    // single‐chunk
    if (declaredInputs <= 6) {
      std::vector<uint64_t> deps;
      size_t nDeps = getDependencyChunkCount(design);
      for (size_t i = 2; i < 2 + nDeps; ++i) {
        if (i < property->getValues().size()) {
          deps.push_back(property->getUInt64Value(i));
        }
      }
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(1),
                           declaredInputs == 0 ? std::vector<uint64_t>{} : deps);
    } else {
      // LCOV_EXCL_START
      std::ostringstream reason;
      reason << "Truth table size " << declaredInputs
             << " is larger than 64 bits";
      throw NLException(reason.str());
      // LCOV_EXCL_STOP
    }
  }
  return SNLTruthTable();
}

SNLTruthTable SNLDesignModeling::getTruthTable(const SNLDesign* design,
                                               size_t flatTermID) {
  auto resolveOutputIndex = [&](size_t orderID,
                                NLID::DesignObjectID& outputID,
                                NLID::DesignObjectID& outputCount) {
    outputID = 0;
    outputCount = 0;
    bool found = false;
    size_t bitTermIdx = 0;
    for (const auto& term : design->getBitTerms()) {
      if (term->getDirection() != SNLTerm::Direction::Input) {
        if (bitTermIdx == orderID) {
          outputID = outputCount;
          found = true;
        }
        ++outputCount;
      }
      ++bitTermIdx;
    }
    return found; };

  if (NLDB0::isDB0Primitive(design) &&
      (isDB0SequentialPrimitive(design) || NLDB0::isMemory(design))) {
    return SNLTruthTable();
  }
  auto property = getTruthTableProperty(design);
  NLID::DesignObjectID outputID = 0;
  NLID::DesignObjectID outputCount = 0;
  const bool isOutputTerm = resolveOutputIndex(flatTermID, outputID, outputCount);
  if (NLDB0::isDB0Primitive(design)) {
    // throw an error in case outputIndex is not 1
    // check if FA design
    if (NLDB0::isFA(design)) {
      if (NLDB0::getFAOutputS()->getID() == flatTermID) {
        return NLDB0::getFASumTruthTable();
      } else if (NLDB0::getFAOutputCO()->getID() == flatTermID) {
        return NLDB0::getFACoutTruthTable();
      } else {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID << " is not an output in FA design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
    }
    if (NLDB0::isMux2(design)) {
      if (!isOutputTerm) {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID
               << " is not an output in mux2 design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      return NLDB0::getPrimitiveTruthTable(design);
    } else if (NLDB0::isVectorSelect(design)) {
      return NLDB0::getVectorSelectTruthTable(design, flatTermID);
    } else if (NLDB0::isVectorUpdate(design)) {
      return NLDB0::getVectorUpdateTruthTable(design, flatTermID);
    } else if (NLDB0::isTableSelect(design)) {
      if (!isOutputTerm) {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID
               << " is not an output in table select design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      return NLDB0::getTableSelectTruthTable(design, flatTermID);
    } else if (NLDB0::isDivMod(design)) {
      if (!isOutputTerm) {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID
               << " is not an output in divmod design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      return NLDB0::getDivModTruthTable(design, flatTermID);
    } else if (NLDB0::isAddSub(design) || NLDB0::isMultiply(design) ||
               NLDB0::isShift(design)) {
      if (!isOutputTerm) {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID
               << " is not an output in word arithmetic design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      return NLDB0::getWordTruthTable(design, flatTermID);
    } else {
      if (outputCount != 1) {
        std::ostringstream reason;
        reason << "Design <" << design->getName().getString()
               << "> is a DB0 primitive but has " << outputCount
               << " outputs instead of 1";
        throw NLException(reason.str());
      }
      return NLDB0::getPrimitiveTruthTable(design);
    }
  }
  if (!isOutputTerm) {
    std::ostringstream reason;
    reason << "Term ID " << flatTermID << " is not an output in design <"
           << design->getName().getString() << ">";
    throw NLException(reason.str());
  }

  if (auto modelingProperty = getProperty(design)) {
    if (const auto* parameterTruthTable = modelingProperty->getModeling()
            ->getTruthTableFromParameter_(flatTermID)) {
      return parameterTruthTable->defaultTruthTable;
    }
  }

  if (property) {
    if (!NLDB0::isDB0Primitive(design) && getTruthTableCount(design) == 1) {
      return getTruthTable(design);
    }

    size_t tableIdx = 0;
    size_t valIdx = 0;
    size_t total = property->getValues().size();

    while (true) {
      // LCOV_EXCL_START
      if (valIdx >= total + 1) {
        std::ostringstream reason;
        reason << "Output ID " << outputID << " is out of range for design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      if (tableIdx == outputID) {
        break;
      }
      uint32_t nInputs =
          static_cast<uint32_t>(property->getUInt64Value(valIdx));
      advanceTruthTableValueIndex(design, valIdx, nInputs);
      ++tableIdx;
    }

    uint64_t declaredInputs = property->getUInt64Value(valIdx);

    // single‐chunk fast‐path?
    if (declaredInputs <= 6) {
      std::vector<uint64_t> deps;
      size_t nDeps = getDependencyChunkCount(design);
      for (size_t i = 0; i < nDeps; ++i) {
        deps.push_back(property->getUInt64Value(valIdx + 2 + i));
      }
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(valIdx + 1),
                           declaredInputs == 0 ? std::vector<uint64_t>{} : deps);
    }

    // multi‐chunk
    uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
    uint32_t nBits = 1u << numInputs;
    size_t nChunks = TT_NCHUNKS_FROM_BITS(nBits);
    size_t bitsIdx = valIdx + 1;
    std::vector<bool> bits(nBits, false);
    TT_FILL_BITS(bits, nBits, property, bitsIdx);

    std::vector<uint64_t> deps;
    size_t nDeps = getDependencyChunkCount(design);
    for (size_t i = bitsIdx + nChunks; i < bitsIdx + nChunks + nDeps; ++i) {
      if (i < total) {
        deps.push_back(property->getUInt64Value(i));
      } else {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "Truth table size " << total
               << " is smaller then requested index " << i;
        throw NLException(reason.str());
        // LCOV_EXCL_STOP
      }
    }
    return SNLTruthTable(numInputs, bits,
                         numInputs == 0 ? std::vector<uint64_t>{} : deps);
  }

  return SNLTruthTable();
}

SNLTruthTable SNLDesignModeling::getTruthTable(
    const SNLInstance* instance) {
  if (!instance) {
    throw NLException("SNLDesignModeling::getTruthTable: null instance");
  }
  const auto* model = instance->getModel();
  if (getTruthTableCount(model) != 1) {
    return getTruthTable(model);
  }
  for (auto* term : model->getBitTerms()) {
    if (term->getDirection() != SNLTerm::Direction::Input) {
      return getTruthTable(instance, term->getOrderID());
    }
  }
  return SNLTruthTable();  // LCOV_EXCL_LINE: a valid single table has an output
}

SNLTruthTable SNLDesignModeling::getTruthTable(
    const SNLInstance* instance, size_t flatTermID) {
  if (!instance) {
    throw NLException("SNLDesignModeling::getTruthTable: null instance");
  }
  const auto* model = instance->getModel();
  if (NLDB0::isMemory(model) &&
      NLDB0::getMemorySignature(instance).writePorts == 0) {
    return NLDB0::getRomTruthTable(instance, flatTermID);
  }
  if (auto property = getProperty(model)) {
    auto* modeling = property->getModeling();
    if (modeling->getTruthTableFromParameter_(flatTermID)) {
      return modeling->getTruthTable_(instance, flatTermID);
    }
  }
  return getTruthTable(model, flatTermID);
}

bool SNLDesignModeling::hasTruthTableFromParameter(
    const SNLDesign* design, size_t flatTermID) {
  // A read-only memory's contents come from its INIT instance parameter.
  if (NLDB0::isMemory(design) &&
      NLDB0::getMemorySignature(design).writePorts == 0) {
    return true;
  }
  if (auto property = getProperty(design)) {
    return property->getModeling()->getTruthTableFromParameter_(flatTermID) !=
           nullptr;
  }
  return false;
}

void SNLDesignModeling::invalidateTruthTableCache(
    const SNLInstance* instance) {
  if (!instance) {
    return;
  }
  if (auto property = getProperty(instance->getModel())) {
    property->getModeling()->invalidateTruthTableCache_(instance);
  }
}

bool SNLDesignModeling::hasModeling(const SNLDesign* design) {
  auto property = getProperty(design);
  if (property) {
    return true;
  } else {
    return getTruthTableProperty(design) || hasMemoryInterface(design);
  }
}

bool SNLDesignModeling::isSequential(const SNLDesign* design) {
  if (isDB0SequentialPrimitive(design)) {
    return true;
  }
  if (hasMemoryInterface(design)) {
    const auto memory = getMemoryInterface(design);
    if (!memory.writePorts.empty() || memory.clock || memory.reset) return true;
  }
  auto property = getProperty(design);
  if (property) {
    auto modeling = property->getModeling();
    if (modeling->hasSequentialModel_()) {
      return true;
    }
    const auto arcs = modeling->getTimingArcs();
    return not arcs->inputToClockArcs_.empty() or
           not arcs->clockToInputArcs_.empty();
  }
  return false;
}

namespace {
template<typename Predicate>
bool isTruthTablePredicate(const SNLDesign* design, Predicate predicate) {
  try {
    auto truthTable = SNLDesignModeling::getTruthTable(design);
    return truthTable.isInitialized() && predicate(truthTable);
  } catch (const NLException&) {
    return false;
  }
}

bool isTruthTableFamily(
    const SNLDesign* design,
    bool (SNLTruthTable::*predicate)() const) {
  return isTruthTablePredicate(design, [predicate](const SNLTruthTable& truthTable) {
    return (truthTable.*predicate)();
  });
}
}  // namespace

bool SNLDesignModeling::isConst0(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 3) {
    return false;
  }
  return isTruthTablePredicate(design, [](const SNLTruthTable& truthTable) {
    return truthTable == SNLTruthTable::Logic0();
  }); // LCOV_EXCL_LINE: compiler-generated lambda closure coverage artifact
}

bool SNLDesignModeling::isConst1(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  return isTruthTablePredicate(design, [](const SNLTruthTable& truthTable) {
    return truthTable == SNLTruthTable::Logic1();
  }); // LCOV_EXCL_LINE: compiler-generated lambda closure coverage artifact
}

bool SNLDesignModeling::isConst(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  return isTruthTablePredicate(design, [](const SNLTruthTable& truthTable) {
    return truthTable == SNLTruthTable::Logic0() ||
           truthTable == SNLTruthTable::Logic1();
  }); // LCOV_EXCL_LINE: compiler-generated lambda closure coverage artifact
}

bool SNLDesignModeling::isInv(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  return isTruthTablePredicate(design, [](const SNLTruthTable& truthTable) {
    return !truthTable.isGeneric() &&
           truthTable.size() == 1 && truthTable.bits().size() == 2 &&
           static_cast<uint64_t>(truthTable.bits()) == 0b01;
  });
}

bool SNLDesignModeling::isBuf(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  return isTruthTablePredicate(design, [](const SNLTruthTable& truthTable) {
    return !truthTable.isGeneric() &&
           truthTable.size() == 1 && truthTable.bits().size() == 2 &&
           static_cast<uint64_t>(truthTable.bits()) == 0b10;
  });
}

bool SNLDesignModeling::isAnd(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isAnd);
}

bool SNLDesignModeling::isNand(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isNand);
}

bool SNLDesignModeling::isOr(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isOr);
}

bool SNLDesignModeling::isNor(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isNor);
}

bool SNLDesignModeling::isXor(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isXor);
}

bool SNLDesignModeling::isXnor(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isXnor);
}

bool SNLDesignModeling::isMux(const SNLDesign* design) {
  return NLDB0::isMux2(design) ||
         isTruthTableFamily(design, &SNLTruthTable::isMux);
}

}  // namespace naja::NL
