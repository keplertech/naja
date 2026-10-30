// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLDumper.h"

#include <cassert>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <utility>

#include "NajaLog.h"
#include "NajaUtils.h"
#include "NajaPerf.h"

#include "NLDB0.h"
#include "NLDB.h"
#include "NLLibrary.h"

#include "SNLDesign.h"
#include "SNLParameter.h"
#include "SNLScalarTerm.h"
#include "SNLBundleTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLAttributes.h"
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLUtils.h"

namespace {

std::vector<const naja::NL::SNLTerm*> getVerilogInterfaceTerms(const naja::NL::SNLDesign* design) {
  std::vector<const naja::NL::SNLTerm*> terms;
  for (auto term: design->getTerms()) {
    if (auto bundle = dynamic_cast<const naja::NL::SNLBundleTerm*>(term)) {
      for (auto member: bundle->getMembers()) {
        terms.push_back(member);
      }
    } else {
      terms.push_back(term);
    }
  }
  return terms;
}

// LCOV_EXCL_START
double toMilliseconds(const std::chrono::nanoseconds& duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}
// LCOV_EXCL_STOP

size_t dumpDirection(const naja::NL::SNLTerm* term, std::ostream& o) {
  switch (term->getDirection()) {
    case naja::NL::SNLTerm::Direction::Input:
      o << "input";
      return std::char_traits<char>::length("input");
    case naja::NL::SNLTerm::Direction::Output:
      o << "output";
      return std::char_traits<char>::length("output");
    case naja::NL::SNLTerm::Direction::InOut:
    case naja::NL::SNLTerm::Direction::Undefined:
      o << "inout";
      return std::char_traits<char>::length("inout");
  }
  return 0; //LCOV_EXCL_LINE
}

using ContiguousNetBits = std::vector<naja::NL::SNLBitNet*>;

char getAssignConstantBitValue(const naja::NL::SNLBitNet* bit) {
  switch (bit->getType()) {
    case naja::NL::SNLNet::Type::Assign0:
      return '0';
    case naja::NL::SNLNet::Type::Assign1:
      return '1';
    case naja::NL::SNLNet::Type::AssignX:
      return 'x';
    case naja::NL::SNLNet::Type::AssignZ:
      return 'z';
    default:
      throw naja::NL::SNLVRLDumperException("ERROR"); //LCOV_EXCL_LINE
  }
}

void dumpConstantRange(ContiguousNetBits& bits, bool& firstElement, bool& concatenation, std::string& o) {
  if (not bits.empty()) {
    if (not firstElement) {
      o += ", ";
      concatenation = true;
    } else {
      firstElement = false;
    }
    std::string constantStr;
    for (auto bit: bits) {
      constantStr += getAssignConstantBitValue(bit);
    }
    if (constantStr.size() < 4 ||
        constantStr.find_first_of("xz") != std::string::npos) {
      //binary
      o += std::to_string(bits.size()) + "'b";
      o += constantStr;
    } else {
      //hexa
      o += std::to_string(bits.size()) + "'h";
      constantStr = naja::NL::SNLVRLDumper::binStrToHexStr(constantStr);
      o += constantStr;
    }
  }
  bits.clear();
}

bool isUnsignedDecimal(const std::string& value) {
  return std::all_of(
    value.begin(),
    value.end(),
    [](unsigned char c) { return std::isdigit(c) != 0; });
}

std::string getNajaPrimitiveModelName(const naja::NL::SNLDesign* model) {
  auto* library = model->getLibrary();
  if (naja::NL::NLDB0::isDivMod(model)) {
    library = library->getParentLibrary();
  }
  return library->getName().getString();
}

std::string normalizeParameterValue(
  naja::NL::SNLParameter::Type type,
  const std::string& value) {
  if (type != naja::NL::SNLParameter::Type::Boolean) {
    return value;
  }
  if (value == "0" || value == "FALSE") {
    return "0";
  }
  if (value == "1" || value == "TRUE") {
    return "1";
  }
  return value;
}

bool isZeroVerilogIntegerLiteral(const std::string& value) {
  std::string compact;
  compact.reserve(value.size());
  for (char c : value) {
    if (c != '_') {
      compact += c;
    }
  }
  if (compact == "0") {
    return true;
  }

  const auto quote = compact.find('\'');
  if (quote == std::string::npos || quote == 0) {
    return false;
  }
  if (!std::all_of(compact.begin(), compact.begin() + quote, [](unsigned char c) {
        return std::isdigit(c);
      })) {
    return false;
  }

  auto digits = quote + 1;
  if (digits < compact.size() &&
      (compact[digits] == 's' || compact[digits] == 'S')) {
    ++digits;
  }
  if (digits == compact.size()) {
    return false;
  }
  const char base = compact[digits++];
  if (base != 'b' && base != 'B' && base != 'o' && base != 'O' &&
      base != 'd' && base != 'D' && base != 'h' && base != 'H') {
    return false;
  }
  return digits < compact.size() &&
         std::all_of(compact.begin() + digits, compact.end(), [](char c) {
           return c == '0';
         });
}

bool isNajaSequentialModel(const naja::NL::SNLDesign* model) {
  using namespace naja::NL;
  return NLDB0::isDFF(model) || NLDB0::isDLatch(model) || NLDB0::isDFFN(model) ||
      NLDB0::isDFFRN(model) || NLDB0::isDFFR(model) || NLDB0::isDFFS(model) ||
      NLDB0::isDFFE(model) || NLDB0::isDFFRE(model) ||
      NLDB0::isDFFSE(model) ||
      NLDB0::isDFFSR(model) || NLDB0::isDFFSRN(model) ||
      NLDB0::isDFFSS(model) || NLDB0::isDFFSSN(model) ||
      NLDB0::isDFFSRE(model) || NLDB0::isDFFSRNE(model) ||
      NLDB0::isDFFSSE(model) || NLDB0::isDFFSSNE(model);
}

std::string getEmittedDefaultParameterValue(
  const naja::NL::SNLInstance* instance,
  const naja::NL::SNLParameter* parameter) {
  using namespace naja::NL;
  const auto& parameterName = parameter->getName();
  if (NLDB0::isMemory(instance->getModel())) {
    if (parameterName == NLName("WIDTH") ||
        parameterName == NLName("DEPTH") ||
        parameterName == NLName("ABITS") ||
        parameterName == NLName("RD_PORTS") ||
        parameterName == NLName("WR_PORTS")) {
      return "1";
    }
    if (parameterName == NLName("MULTI_CLOCK") ||
        parameterName == NLName("RST_ENABLE") ||
        parameterName == NLName("RST_ASYNC") ||
        parameterName == NLName("RST_ACTIVE_LOW") ||
        parameterName == NLName("INIT_ENABLE")) {
      return "0";
    }
    if (parameterName == NLName("INIT")) {
      return "1'b0";
    }
  }
  if (NLDB0::isDivMod(instance->getModel())) {
    if (parameterName == NLName("WIDTH")) {
      return "1";
    }
    if (parameterName == NLName("SIGNED")) {
      return "0";
    }
  }
  if (NLDB0::isMultiply(instance->getModel())) {
    return "1";
  }
  if (NLDB0::isShift(instance->getModel())) {
    // Shift instances emit their full signature directly, bypassing dumpInstParameters.
    return parameterName == NLName("WIDTH") || parameterName == NLName("COUNT_WIDTH") ? "1" : "0"; // LCOV_EXCL_LINE
  }
  if (NLDB0::isTableSelect(instance->getModel())) {
    if (parameterName == NLName("WIDTH") ||
        parameterName == NLName("DEPTH") ||
        parameterName == NLName("ABITS")) {
      return "1";
    }
  }
  if (NLDB0::isMux2(instance->getModel()) || isNajaSequentialModel(instance->getModel())) {
    if (parameterName == NLName("WIDTH")) {
      return "1";
    }
    if (isNajaSequentialModel(instance->getModel()) && parameterName == NLName("INIT")) {
      // The emitted INIT default is all X, independent of a customized model default.
      const auto width = instance->getModel()->getTerm(NLName("Q"))->getWidth();
      return NLDB0::getUndefinedDFFInitValue(width);
    }
  }
  return parameter->getValue();
}

bool shouldDumpParameter(
  const naja::NL::SNLInstance* instance,
  const naja::NL::SNLParameter* parameter,
  const std::string& value) {
  if (not parameter->hasDefaultValue()) {
    return true;
  }
  if (naja::NL::NLDB0::isMemory(instance->getModel()) &&
      parameter->getName() == naja::NL::NLName("INIT") &&
      isZeroVerilogIntegerLiteral(value)) {
    return false;
  }
  return normalizeParameterValue(parameter->getType(), value) !=
         normalizeParameterValue(
           parameter->getType(),
           getEmittedDefaultParameterValue(instance, parameter));
}

std::string dumpName(const std::string& name) {
  // An identifier is used to give an object a unique name so it can be referenced.
  // An identifier is either a simple identifier or an escaped identifier (see 3.7.1).
  // A simple identifier shall be any sequence of letters, digits, dollar signs ($),
  // and underscore characters (_).
  // The first character of a simple identifier shall not be a digit or $; 
  // it can be a letter or an underscore. Identifiers shall be case sensitive.

  //special first character case
  bool escape = (name[0] == '$' or ('0' <= name[0] and name[0] <= '9'));
  if (not escape) {
    for (int i=0; i<name.size(); ++i) {
      if ('0' <= name[i] && name[i] <= '9') {
        continue;
      }
      if ('a' <= name[i] && name[i] <= 'z') {
        continue;
      }
      if ('A' <= name[i] && name[i] <= 'Z') {
        continue;
      }
      if (name[i] == '_') {
        continue;
      }
      if (name[i] == '$') {
        continue;
      }
      escape = true;
      break;
    }
  }

  static const std::unordered_set<std::string> keywords = {
		// IEEE 1800-2017 Annex B
	  "accept_on", "alias", "always", "always_comb", "always_ff", "always_latch", "and", "assert", "assign", "assume", "automatic", "before",
		"begin", "bind", "bins", "binsof", "bit", "break", "buf", "bufif0", "bufif1", "byte", "case", "casex", "casez", "cell", "chandle",
		"checker", "class", "clocking", "cmos", "config", "const", "constraint", "context", "continue", "cover", "covergroup", "coverpoint",
		"cross", "deassign", "default", "defparam", "design", "disable", "dist", "do", "edge", "else", "end", "endcase", "endchecker",
		"endclass", "endclocking", "endconfig", "endfunction", "endgenerate", "endgroup", "endinterface", "endmodule", "endpackage",
		"endprimitive", "endprogram", "endproperty", "endsequence", "endspecify", "endtable", "endtask", "enum", "event", "eventually",
		"expect", "export", "extends", "extern", "final", "first_match", "for", "force", "foreach", "forever", "fork", "forkjoin", "function",
		"generate", "genvar", "global", "highz0", "highz1", "if", "iff", "ifnone", "ignore_bins", "illegal_bins", "implements", "implies",
		"import", "incdir", "include", "initial", "inout", "input", "inside", "instance", "int", "integer", "interconnect", "interface",
		"intersect", "join", "join_any", "join_none", "large", "let", "liblist", "library", "local", "localparam", "logic", "longint",
		"macromodule", "matches", "medium", "modport", "module", "nand", "negedge", "nettype", "new", "nexttime", "nmos", "nor",
		"noshowcancelled", "not", "notif0", "notif1", "null", "or", "output", "package", "packed", "parameter", "pmos", "posedge", "primitive",
		"priority", "program", "property", "protected", "pull0", "pull1", "pulldown", "pullup", "pulsestyle_ondetect", "pulsestyle_onevent",
		"pure", "rand", "randc", "randcase", "randsequence", "rcmos", "real", "realtime", "ref", "reg", "reject_on", "release", "repeat",
		"restrict", "return", "rnmos", "rpmos", "rtran", "rtranif0", "rtranif1", "s_always", "s_eventually", "s_nexttime", "s_until",
		"s_until_with", "scalared", "sequence", "shortint", "shortreal", "showcancelled", "signed", "small", "soft", "solve", "specify",
		"specparam", "static", "string", "strong", "strong0", "strong1", "struct", "super", "supply0", "supply1", "sync_accept_on",
		"sync_reject_on", "table", "tagged", "task", "this", "throughout", "time", "timeprecision", "timeunit", "tran", "tranif0", "tranif1",
		"tri", "tri0", "tri1", "triand", "trior", "trireg", "type", "typedef", "union", "unique", "unique0", "unsigned", "until", "until_with",
		"untyped", "use", "uwire", "var", "vectored", "virtual", "void", "wait", "wait_order", "wand", "weak", "weak0", "weak1", "while",
		"wildcard", "wire", "with", "within", "wor", "xnor", "xor",
	};
	if (keywords.find(name) != keywords.end()) {
		escape = true;
  }

  if (escape) {
    return "\\" + name + " ";
  }
  return name;
}

std::string getBusBitConcatenationString(
  const std::vector<const naja::NL::SNLBusNetBit*>& bits,
  const auto& bitNetToString) {
  assert(not bits.empty());
  std::string concatenation = "{";
  for (size_t i = 0; i < bits.size(); ++i) {
    if (i != 0) {
      concatenation += ", ";
    }
    concatenation += bitNetToString(bits[i]);
  }
  concatenation += "}";
  return concatenation;
}

naja::NL::NLID::Bit getCanonicalRangeStep(const naja::NL::SNLBusNet* bus) {
  return (bus->getMSB() > bus->getLSB()) ? -1 : 1;
}

bool isCanonicalBusRangeOrder(const std::vector<const naja::NL::SNLBusNetBit*>& bits) {
  assert(not bits.empty());
  if (bits.size() == 1) {
    return true;
  }
  auto bus = bits.front()->getBus();
  auto expectedStep = getCanonicalRangeStep(bus);
  for (size_t i = 1; i < bits.size(); ++i) {
    if (bits[i]->getBus() != bus) {
      return false; // LCOV_EXCL_LINE: defensive, current callers only pass same-bus ranges.
    }
    auto actualStep = bits[i]->getBit() - bits[i - 1]->getBit();
    if (actualStep != expectedStep) {
      return false;
    }
  }
  return true;
}

bool isCanonicalBusRangeOrder(const ContiguousNetBits& bits) {
  assert(not bits.empty());
  if (bits.size() == 1) {
    return true;
  }
  auto firstBit = static_cast<const naja::NL::SNLBusNetBit*>(bits.front());
  auto bus = firstBit->getBus();
  auto expectedStep = getCanonicalRangeStep(bus);
  for (size_t i = 1; i < bits.size(); ++i) {
    auto previousBit = static_cast<const naja::NL::SNLBusNetBit*>(bits[i - 1]);
    auto currentBit = static_cast<const naja::NL::SNLBusNetBit*>(bits[i]);
    if (currentBit->getBus() != bus) {
      return false; // LCOV_EXCL_LINE: defensive, current callers only pass same-bus ranges.
    }
    auto actualStep = currentBit->getBit() - previousBit->getBit();
    if (actualStep != expectedStep) {
      return false;
    }
  }
  return true;
}

void normalizeAssignGroupOutputOrder(
  std::vector<const naja::NL::SNLBitNet*>& inputBits,
  std::vector<const naja::NL::SNLBusNetBit*>& outputBits) {
  assert(inputBits.size() == outputBits.size());
  assert(not outputBits.empty());
  if (outputBits.size() == 1) {
    return;
  }
  auto outputBus = outputBits.front()->getBus();
  auto expectedOutputStep = getCanonicalRangeStep(outputBus);
  auto outputStep = outputBits[1]->getBit() - outputBits[0]->getBit();
  if (outputStep != expectedOutputStep) {
    std::reverse(inputBits.begin(), inputBits.end());
    std::reverse(outputBits.begin(), outputBits.end());
  }
}

bool isContiguousBitDelta(const naja::NL::NLID::Bit delta) {
  return delta == 1 or delta == -1;
}

bool getAssignConnectivity(
  const naja::NL::SNLInstance* instance,
  const naja::NL::SNLBitNet*& inputNet,
  const naja::NL::SNLBitNet*& outputNet) {
  inputNet = nullptr;
  outputNet = nullptr;
  auto inputTerm = instance->getInstTerm(naja::NL::NLDB0::getAssignInput());
  auto outputTerm = instance->getInstTerm(naja::NL::NLDB0::getAssignOutput());
  if (inputTerm and outputTerm) {
    inputNet = inputTerm->getNet();
    outputNet = outputTerm->getNet();
  }
  return inputNet and outputNet;
}

bool dumpSingleAssign(
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet,
  std::ostream& o,
  const auto& bitNetToString) {
  std::string inputNetString;
  if (inputNet->isConstant0()) {
    inputNetString = "1'b0";
  } else if (inputNet->isConstant1()) {
    inputNetString = "1'b1";
  } else {
    inputNetString = bitNetToString(inputNet);
  }
  auto outputNetString = bitNetToString(outputNet);
  o << "assign " << outputNetString << " = " << inputNetString << ";" << '\n';
  return true;
}

std::string getBusBitRangeString(
  const std::vector<const naja::NL::SNLBusNetBit*>& bits,
  const auto& busNetToString) {
  assert(not bits.empty());
  auto firstBit = bits.front();
  auto lastBit = bits.back();
  auto bus = firstBit->getBus();
  assert(bus == lastBit->getBus());
  auto busName = busNetToString(bus);
  if (firstBit->getBit() == bus->getMSB() and lastBit->getBit() == bus->getLSB()) {
    return busName;
  }
  return busName + "[" + std::to_string(firstBit->getBit()) + ":" + std::to_string(lastBit->getBit()) + "]";
}

std::string getAssignConstantString(const std::vector<const naja::NL::SNLBitNet*>& bits) {
  assert(not bits.empty());
  std::string bitString;
  bitString.reserve(bits.size());
  for (auto bit: bits) {
    bitString += getAssignConstantBitValue(bit);
  }
  return std::to_string(bits.size()) + "'b" + bitString;
}

enum class AssignInputMode {
  Bus,
  Constant
};

struct AssignGroup {
  AssignInputMode                              inputMode_ {AssignInputMode::Bus};
  std::vector<const naja::NL::SNLBitNet*>      inputBits_ {};
  std::vector<const naja::NL::SNLBusNetBit*>   outputBits_{};
  bool                                          hasOutputStep_{false};
  naja::NL::NLID::Bit                           outputStep_{0};
  bool                                          hasInputStep_ {false};
  naja::NL::NLID::Bit                           inputStep_ {0};
};

enum class AssignChunkInputKind {
  Constant,
  Bus,
  Other
};

struct AssignBusChunk {
  const naja::NL::SNLBusNet*                    outputBus_ {nullptr};
  std::vector<const naja::NL::SNLBusNetBit*>    outputBits_ {};
  AssignChunkInputKind                          inputKind_ {AssignChunkInputKind::Other};
  const naja::NL::SNLBusNet*                    inputBus_ {nullptr};
  std::vector<const naja::NL::SNLBusNetBit*>    inputBusBits_ {};
  std::string                                   inputString_ {};
};

struct AssignEmission {
  bool            hasBusChunk_ {false};
  AssignBusChunk  busChunk_ {};
  std::string     text_ {};
};

bool initializeAssignGroup(
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet,
  AssignGroup& group) {
  auto outputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(outputNet);
  if (not outputBusBit) {
    return false;
  }
  if (inputNet->isAssignConstant()) {
    group.inputMode_ = AssignInputMode::Constant;
  } else {
    auto inputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(inputNet);
    if (not inputBusBit) {
      return false;
    }
    group.inputMode_ = AssignInputMode::Bus;
  }
  group.inputBits_.push_back(inputNet);
  group.outputBits_.push_back(outputBusBit);
  return true;
}

std::string getAssignInputString(
  const naja::NL::SNLBitNet* inputNet,
  const auto& bitNetToString) {
  if (inputNet->isConstant0()) {
    return "1'b0";
  }
  if (inputNet->isConstant1()) {
    return "1'b1";
  }
  return bitNetToString(inputNet);
}

std::string getBusBitOrRangeString(
  const std::vector<const naja::NL::SNLBusNetBit*>& bits,
  const auto& busNetToString) {
  assert(not bits.empty());
  assert(bits.size() > 1);
  return getBusBitRangeString(bits, busNetToString);
}

std::string getBusAssignInputString(
  const std::vector<const naja::NL::SNLBusNetBit*>& inputBits,
  const auto& bitNetToString,
  const auto& busNetToString) {
  assert(not inputBits.empty());
  assert(inputBits.size() > 1);
  if (isCanonicalBusRangeOrder(inputBits)) {
    return getBusBitRangeString(inputBits, busNetToString);
  }
  return getBusBitConcatenationString(inputBits, bitNetToString);
}

bool createSingleAssignBusChunk(
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet,
  const auto& bitNetToString,
  AssignBusChunk& chunk) {
  auto outputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(outputNet);
  if (not outputBusBit) {
    return false;
  }

  chunk.outputBus_ = outputBusBit->getBus();
  chunk.outputBits_.push_back(outputBusBit);
  chunk.inputString_ = getAssignInputString(inputNet, bitNetToString);

  if (inputNet->isAssignConstant()) {
    chunk.inputKind_ = AssignChunkInputKind::Constant;
    return true;
  }

  if (auto inputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(inputNet)) {
    chunk.inputKind_ = AssignChunkInputKind::Bus;
    chunk.inputBus_ = inputBusBit->getBus();
    chunk.inputBusBits_.push_back(inputBusBit);
    return true;
  }

  chunk.inputKind_ = AssignChunkInputKind::Other;
  return true;
}

AssignBusChunk createAssignGroupBusChunk(
  const AssignGroup& group,
  const auto& bitNetToString,
  const auto& busNetToString) {
  assert(not group.outputBits_.empty());
  auto inputBits = group.inputBits_;
  auto outputBits = group.outputBits_;
  normalizeAssignGroupOutputOrder(inputBits, outputBits);

  AssignBusChunk chunk;
  chunk.outputBus_ = outputBits.front()->getBus();
  chunk.outputBits_ = std::move(outputBits);

  if (group.inputMode_ == AssignInputMode::Constant) {
    chunk.inputKind_ = AssignChunkInputKind::Constant;
    chunk.inputString_ = getAssignConstantString(inputBits);
    return chunk;
  }

  chunk.inputKind_ = AssignChunkInputKind::Bus;
  chunk.inputBusBits_.reserve(inputBits.size());
  for (auto inputBit: inputBits) {
    chunk.inputBusBits_.push_back(static_cast<const naja::NL::SNLBusNetBit*>(inputBit));
  }
  chunk.inputBus_ = chunk.inputBusBits_.front()->getBus();
  chunk.inputString_ = getBusAssignInputString(
    chunk.inputBusBits_,
    bitNetToString,
    busNetToString);
  return chunk;
}

std::string getAssignBusChunkText(
  const AssignBusChunk& chunk,
  const auto& busNetToString) {
  auto outputString = getBusBitOrRangeString(
    chunk.outputBits_,
    busNetToString);
  return "assign " + outputString + " = " + chunk.inputString_ + ";";
}

bool areChunksWholeBusAssignable(
  const std::vector<const AssignBusChunk*>& chunks,
  std::vector<const AssignBusChunk*>& sortedChunks) {
  if (chunks.size() <= 1) {
    return false;
  }

  auto outputBus = chunks.front()->outputBus_;
  if (not outputBus) {
    return false; // LCOV_EXCL_LINE: bus chunks are only created for bus outputs.
  }

  const naja::NL::SNLBusNet* uniqueInputBus = nullptr;
  for (auto chunk: chunks) {
    if (chunk->outputBus_ != outputBus) {
      return false; // LCOV_EXCL_LINE: caller groups chunks by output bus before calling.
    }
    if (chunk->inputKind_ == AssignChunkInputKind::Other) {
      return false;
    }
    if (chunk->inputKind_ == AssignChunkInputKind::Bus) {
      if (not uniqueInputBus) {
        uniqueInputBus = chunk->inputBus_;
      } else if (uniqueInputBus != chunk->inputBus_) {
        return false;
      }
    }
  }

  sortedChunks = chunks;
  auto step = getCanonicalRangeStep(outputBus);
  std::sort(
    sortedChunks.begin(),
    sortedChunks.end(),
    [step](const AssignBusChunk* lhs, const AssignBusChunk* rhs) {
      auto lhsBit = lhs->outputBits_.front()->getBit();
      auto rhsBit = rhs->outputBits_.front()->getBit();
      return (step < 0) ? (lhsBit > rhsBit) : (lhsBit < rhsBit);
    });

  auto expectedBit = outputBus->getMSB();
  size_t coveredBits = 0;
  for (auto chunk: sortedChunks) {
    if (not isCanonicalBusRangeOrder(chunk->outputBits_)) {
      return false; // LCOV_EXCL_LINE: chunk output bits are normalized at creation.
    }
    if (chunk->outputBits_.front()->getBit() != expectedBit) {
      return false;
    }
    coveredBits += chunk->outputBits_.size();
    expectedBit = chunk->outputBits_.back()->getBit() + step;
  }

  return coveredBits == static_cast<size_t>(outputBus->getWidth()) &&
         expectedBit == outputBus->getLSB() + step;
}

std::string buildWholeBusAssignInputString(
  const std::vector<const AssignBusChunk*>& sortedChunks,
  const auto& bitNetToString,
  const auto& busNetToString) {
  assert(not sortedChunks.empty());

  bool sameInputBus = true;
  const naja::NL::SNLBusNet* inputBus = nullptr;
  std::vector<const naja::NL::SNLBusNetBit*> inputBits;
  for (auto chunk: sortedChunks) {
    if (chunk->inputKind_ != AssignChunkInputKind::Bus) {
      sameInputBus = false;
      break;
    }
    if (not inputBus) {
      inputBus = chunk->inputBus_;
    } else if (inputBus != chunk->inputBus_) {
      sameInputBus = false; // LCOV_EXCL_LINE: areChunksWholeBusAssignable filters mixed input buses.
      break; // LCOV_EXCL_LINE
    }
    inputBits.insert(
      inputBits.end(),
      chunk->inputBusBits_.begin(),
      chunk->inputBusBits_.end());
  }

  if (sameInputBus and isCanonicalBusRangeOrder(inputBits)) {
    return getBusAssignInputString(inputBits, bitNetToString, busNetToString);
  }

  std::string concatenation = "{";
  for (size_t i = 0; i < sortedChunks.size(); ++i) {
    if (i != 0) {
      concatenation += ", ";
    }
    concatenation += sortedChunks[i]->inputString_;
  }
  concatenation += "}";
  return concatenation;
}

std::string buildWholeBusAssignText(
  const std::vector<const AssignBusChunk*>& sortedChunks,
  const auto& bitNetToString,
  const auto& busNetToString) {
  assert(not sortedChunks.empty());
  auto outputString = busNetToString(sortedChunks.front()->outputBus_);
  auto inputString = buildWholeBusAssignInputString(
    sortedChunks,
    bitNetToString,
    busNetToString);
  return "assign " + outputString + " = " + inputString + ";";
}

bool appendAssignGroup(
  AssignGroup& group,
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet) {
  auto outputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(outputNet);
  if (not outputBusBit) {
    return false;
  }

  auto previousOutputBit = group.outputBits_.back();
  if (outputBusBit->getBus() != previousOutputBit->getBus()) {
    return false;
  }

  auto outputDelta = outputBusBit->getBit() - previousOutputBit->getBit();
  if (not isContiguousBitDelta(outputDelta)) {
    return false;
  }
  if (group.hasOutputStep_ and outputDelta != group.outputStep_) {
    return false;
  }

  if (group.inputMode_ == AssignInputMode::Constant) {
    if (not inputNet->isAssignConstant()) {
      return false;
    }
  } else {
    auto inputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(inputNet);
    if (not inputBusBit) {
      return false;
    }
    auto previousInputBusBit = static_cast<const naja::NL::SNLBusNetBit*>(group.inputBits_.back());
    if (inputBusBit->getBus() != previousInputBusBit->getBus()) {
      return false;
    }
    auto inputDelta = inputBusBit->getBit() - previousInputBusBit->getBit();
    if (not isContiguousBitDelta(inputDelta)) {
      return false;
    }
    if (group.hasInputStep_ and inputDelta != group.inputStep_) {
      return false;
    }
    if (not group.hasInputStep_) {
      group.inputStep_ = inputDelta;
      group.hasInputStep_ = true;
    }
  }

  if (not group.hasOutputStep_) {
    group.outputStep_ = outputDelta;
    group.hasOutputStep_ = true;
  }
  group.inputBits_.push_back(inputNet);
  group.outputBits_.push_back(outputBusBit);
  return true;
}

bool dumpAssignGroup(
  const AssignGroup& group,
  std::ostream& o,
  const auto& bitNetToString,
  const auto& busNetToString) {
  auto chunk = createAssignGroupBusChunk(group, bitNetToString, busNetToString);
  o << getAssignBusChunkText(chunk, busNetToString) << '\n';
  return true;
}

}

namespace naja::NL {

SNLVRLDumper::SNLVRLDumper() {
  initializeDetailedPerfConfig();
}

SNLVRLDumper::DetailedPerfScopedTimer::DetailedPerfScopedTimer(
  DetailedPerfReport& report,
  std::chrono::nanoseconds& bucket,
  size_t& calls):
  report_((report.enabled && report.sessionActive) ? &report : nullptr),
  bucket_((report.enabled && report.sessionActive) ? &bucket : nullptr) {
  if (report_) {
    // LCOV_EXCL_START
    ++calls;
    start_ = std::chrono::steady_clock::now();
    // LCOV_EXCL_STOP
  }
}

SNLVRLDumper::DetailedPerfScopedTimer::~DetailedPerfScopedTimer() {
  if (report_) {
    *bucket_ += std::chrono::steady_clock::now() - start_; // LCOV_EXCL_LINE
  }
}

SNLVRLDumper::DetailedPerfSessionGuard::DetailedPerfSessionGuard(
  SNLVRLDumper& dumper,
  const std::string& context):
  dumper_(&dumper),
  started_(dumper.beginDetailedPerfSession(context)) {}

SNLVRLDumper::DetailedPerfSessionGuard::~DetailedPerfSessionGuard() {
  if (dumper_ && started_) {
    dumper_->finalizeDetailedPerfSession(); // LCOV_EXCL_LINE
  }
}

void SNLVRLDumper::initializeDetailedPerfConfig() {
  const char* reportEnv = std::getenv("NAJA_VRL_DUMPER_REPORT");
  if (!reportEnv) {
    return;
  }
  // LCOV_EXCL_START
  std::string reportPath(reportEnv);
  if (reportPath.empty() || reportPath == "1") {
    reportPath = "vrl_dumper_perf.log";
  }
  detailedPerfReport_.enabled = true;
  detailedPerfReport_.reportPath = reportPath;
  // LCOV_EXCL_STOP
}

bool SNLVRLDumper::beginDetailedPerfSession(const std::string& context) {
  if (!detailedPerfReport_.enabled || detailedPerfReport_.sessionActive) {
    return false;
  }
  // LCOV_EXCL_START
  auto reportPath = detailedPerfReport_.reportPath;
  detailedPerfReport_ = DetailedPerfReport {};
  detailedPerfReport_.enabled = true;
  detailedPerfReport_.sessionActive = true;
  detailedPerfReport_.reportPath = std::move(reportPath);
  detailedPerfReport_.context = context;
  detailedPerfReport_.sessionStart = std::chrono::steady_clock::now();
  return true;
  // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
void SNLVRLDumper::finalizeDetailedPerfSession() {
  if (!detailedPerfReport_.enabled || !detailedPerfReport_.sessionActive) {
    return;
  }
  detailedPerfReport_.totalDuration =
    std::chrono::steady_clock::now() - detailedPerfReport_.sessionStart;
  detailedPerfReport_.sessionActive = false;

  std::ofstream output(detailedPerfReport_.reportPath, std::ios::out | std::ios::app);
  if (!output) {
    NAJA_LOG_WARN(
      "Unable to write Verilog dumper performance report: {}",
      detailedPerfReport_.reportPath.string());
    return;
  }

  output << "=== SNLVRLDumper Perf Report ===\n";
  output << "context=" << detailedPerfReport_.context << "\n";
  output << std::fixed << std::setprecision(3);
  output << "time.total_ms=" << toMilliseconds(detailedPerfReport_.totalDuration) << "\n";
  output << "time.dumpAttributes_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesDuration) << "\n";
  output << "count.dumpAttributes_calls=" << detailedPerfReport_.dumpAttributesCalls << "\n";
  output << "count.dumpAttributes_empty_calls="
         << detailedPerfReport_.dumpAttributesEmptyCalls << "\n";
  output << "count.dumpAttributes_nonempty_calls="
         << detailedPerfReport_.dumpAttributesNonEmptyCalls << "\n";
  output << "count.dumped_attributes=" << detailedPerfReport_.dumpedAttributesCount << "\n";
  output << "time.dumpAttributes_snl_attributes_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesSNLAttributesDuration) << "\n";
  output << "count.dumped_snl_attributes="
         << detailedPerfReport_.dumpedSNLAttributesCount << "\n";
  output << "time.dumpAttributes_rtl_infos_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesRTLInfosDuration) << "\n";
  output << "count.dumped_rtl_infos="
         << detailedPerfReport_.dumpedRTLInfosCount << "\n";
  if (detailedPerfReport_.dumpedAttributesCount > 0) {
    const double dumpAttributesMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesDuration).count();
    output << "derived.dumpAttributes_us_per_attribute="
           << (dumpAttributesMicros / detailedPerfReport_.dumpedAttributesCount) << "\n";
  }
  if (detailedPerfReport_.dumpedSNLAttributesCount > 0) {
    const double dumpSNLAttributesMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesSNLAttributesDuration).count();
    output << "derived.dumpAttributes_snl_attributes_us_per_attribute="
           << (dumpSNLAttributesMicros / detailedPerfReport_.dumpedSNLAttributesCount) << "\n";
  }
  if (detailedPerfReport_.dumpedRTLInfosCount > 0) {
    const double dumpRTLInfosMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesRTLInfosDuration).count();
    output << "derived.dumpAttributes_rtl_infos_us_per_attribute="
           << (dumpRTLInfosMicros / detailedPerfReport_.dumpedRTLInfosCount) << "\n";
  }
  output << "time.dumpAttributes_design_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesDesignDuration) << "\n";
  output << "count.dumpAttributes_design_calls="
         << detailedPerfReport_.dumpAttributesDesignCalls << "\n";
  output << "count.dumped_attributes_design="
         << detailedPerfReport_.dumpedAttributesDesignCount << "\n";
  if (detailedPerfReport_.dumpedAttributesDesignCount > 0) {
    const double dumpAttributesDesignMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesDesignDuration).count();
    output << "derived.dumpAttributes_design_us_per_attribute="
           << (dumpAttributesDesignMicros / detailedPerfReport_.dumpedAttributesDesignCount)
           << "\n";
  }
  output << "time.dumpAttributes_instance_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesInstanceDuration) << "\n";
  output << "count.dumpAttributes_instance_calls="
         << detailedPerfReport_.dumpAttributesInstanceCalls << "\n";
  output << "count.dumped_attributes_instance="
         << detailedPerfReport_.dumpedAttributesInstanceCount << "\n";
  if (detailedPerfReport_.dumpedAttributesInstanceCount > 0) {
    const double dumpAttributesInstanceMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesInstanceDuration).count();
    output << "derived.dumpAttributes_instance_us_per_attribute="
           << (dumpAttributesInstanceMicros / detailedPerfReport_.dumpedAttributesInstanceCount)
           << "\n";
  }
  output << "time.dumpAttributes_net_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesNetDuration) << "\n";
  output << "count.dumpAttributes_net_calls="
         << detailedPerfReport_.dumpAttributesNetCalls << "\n";
  output << "count.dumped_attributes_net="
         << detailedPerfReport_.dumpedAttributesNetCount << "\n";
  if (detailedPerfReport_.dumpedAttributesNetCount > 0) {
    const double dumpAttributesNetMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesNetDuration).count();
    output << "derived.dumpAttributes_net_us_per_attribute="
           << (dumpAttributesNetMicros / detailedPerfReport_.dumpedAttributesNetCount)
           << "\n";
  }
  output << "time.dumpInterface_ms="
         << toMilliseconds(detailedPerfReport_.dumpInterfaceDuration) << "\n";
  output << "count.dumpInterface_calls=" << detailedPerfReport_.dumpInterfaceCalls << "\n";
  output << "time.dumpNets_ms=" << toMilliseconds(detailedPerfReport_.dumpNetsDuration) << "\n";
  output << "count.dumpNets_calls=" << detailedPerfReport_.dumpNetsCalls << "\n";
  output << "time.dumpInstances_ms="
         << toMilliseconds(detailedPerfReport_.dumpInstancesDuration) << "\n";
  output << "count.dumpInstances_calls=" << detailedPerfReport_.dumpInstancesCalls << "\n";
  output << "time.dumpTermAssigns_ms="
         << toMilliseconds(detailedPerfReport_.dumpTermAssignsDuration) << "\n";
  output << "count.dumpTermAssigns_calls=" << detailedPerfReport_.dumpTermAssignsCalls << "\n";
  output << "time.dumpParameters_ms="
         << toMilliseconds(detailedPerfReport_.dumpParametersDuration) << "\n";
  output << "count.dumpParameters_calls=" << detailedPerfReport_.dumpParametersCalls << "\n";
  output << "time.dumpOneDesign_ms="
         << toMilliseconds(detailedPerfReport_.dumpOneDesignDuration) << "\n";
  output << "count.dumpOneDesign_calls=" << detailedPerfReport_.dumpOneDesignCalls << "\n";
  output << "time.dumpDesign_stream_ms="
         << toMilliseconds(detailedPerfReport_.dumpDesignStreamDuration) << "\n";
  output << "count.dumpDesign_stream_calls=" << detailedPerfReport_.dumpDesignStreamCalls << "\n";
  output << "time.dumpDesign_path_ms="
         << toMilliseconds(detailedPerfReport_.dumpDesignPathDuration) << "\n";
  output << "count.dumpDesign_path_calls=" << detailedPerfReport_.dumpDesignPathCalls << "\n";
  output << "time.dumpLibrary_stream_ms="
         << toMilliseconds(detailedPerfReport_.dumpLibraryStreamDuration) << "\n";
  output << "count.dumpLibrary_stream_calls=" << detailedPerfReport_.dumpLibraryStreamCalls
         << "\n";
  output << "time.dumpLibrary_path_ms="
         << toMilliseconds(detailedPerfReport_.dumpLibraryPathDuration) << "\n";
  output << "count.dumpLibrary_path_calls=" << detailedPerfReport_.dumpLibraryPathCalls << "\n";

  struct PhaseStat {
    const char* name;
    std::chrono::nanoseconds duration;
    size_t calls;
  };
  std::vector<PhaseStat> phases {
    {"dumpAttributes", detailedPerfReport_.dumpAttributesDuration,
      detailedPerfReport_.dumpAttributesCalls},
    {"dumpAttributes_snl_attributes", detailedPerfReport_.dumpAttributesSNLAttributesDuration,
      detailedPerfReport_.dumpAttributesCalls},
    {"dumpAttributes_rtl_infos", detailedPerfReport_.dumpAttributesRTLInfosDuration,
      detailedPerfReport_.dumpAttributesCalls},
    {"dumpInterface", detailedPerfReport_.dumpInterfaceDuration,
      detailedPerfReport_.dumpInterfaceCalls},
    {"dumpNets", detailedPerfReport_.dumpNetsDuration,
      detailedPerfReport_.dumpNetsCalls},
    {"dumpInstances", detailedPerfReport_.dumpInstancesDuration,
      detailedPerfReport_.dumpInstancesCalls},
    {"dumpTermAssigns", detailedPerfReport_.dumpTermAssignsDuration,
      detailedPerfReport_.dumpTermAssignsCalls},
    {"dumpParameters", detailedPerfReport_.dumpParametersDuration,
      detailedPerfReport_.dumpParametersCalls},
    {"dumpOneDesign", detailedPerfReport_.dumpOneDesignDuration,
      detailedPerfReport_.dumpOneDesignCalls},
    {"dumpDesign_stream", detailedPerfReport_.dumpDesignStreamDuration,
      detailedPerfReport_.dumpDesignStreamCalls},
    {"dumpDesign_path", detailedPerfReport_.dumpDesignPathDuration,
      detailedPerfReport_.dumpDesignPathCalls},
    {"dumpLibrary_stream", detailedPerfReport_.dumpLibraryStreamDuration,
      detailedPerfReport_.dumpLibraryStreamCalls},
    {"dumpLibrary_path", detailedPerfReport_.dumpLibraryPathDuration,
      detailedPerfReport_.dumpLibraryPathCalls},
  };
  std::sort(phases.begin(), phases.end(),
    [](const PhaseStat& lhs, const PhaseStat& rhs) {
      return lhs.duration > rhs.duration;
    });

  const double totalMs = toMilliseconds(detailedPerfReport_.totalDuration);
  output << "hotspots.top_by_time=\n";
  size_t rank = 1;
  for (const auto& phase: phases) {
    if (phase.duration.count() == 0 && phase.calls == 0) {
      continue;
    }
    const double phaseMs = toMilliseconds(phase.duration);
    const double pctTotal = totalMs > 0.0 ? (100.0 * phaseMs / totalMs) : 0.0;
    const double phaseMicros =
      std::chrono::duration<double, std::micro>(phase.duration).count();
    const double avgMicrosPerCall = phase.calls > 0
      ? (phaseMicros / phase.calls)
      : 0.0;
    output << "hotspot." << rank
           << "=" << phase.name
           << " time_ms=" << phaseMs
           << " pct_total=" << pctTotal
           << " calls=" << phase.calls
           << " avg_us_per_call=" << avgMicrosPerCall
           << "\n";
    ++rank;
  }
  output << "\n";
}
// LCOV_EXCL_STOP

void SNLVRLDumper::setSingleFile(bool mode) {
  configuration_.setSingleFile(mode);
}

void SNLVRLDumper::setLibraryFileName(const std::string& name) {
  configuration_.setLibraryFileName(name);
}

void SNLVRLDumper::setTopFileName(const std::string& name) {
  configuration_.setTopFileName(name);
}

void SNLVRLDumper::setDumpHierarchy(bool mode) {
  configuration_.setDumpHierarchy(mode);
}

void SNLVRLDumper::setDumpRTLInfosAsAttributes(bool mode) {
  configuration_.setDumpRTLInfosAsAttributes(mode);
}

void SNLVRLDumper::setRTLInfoDumpMode(RTLInfoDumpMode mode) {
  configuration_.setRTLInfoDumpMode(mode);
}

void SNLVRLDumper::setDumpAssignsAsInstances(bool mode) {
  configuration_.setDumpAssignsAsInstances(mode);
}

std::string SNLVRLDumper::createDesignName(const SNLDesign* design) {
  auto library = design->getLibrary();
  auto designID = design->getID();
  std::string designName = "module" + std::to_string(designID);
  int conflict = 0;
  while (library->getSNLDesign(NLName(designName))) {
    designName += "_" + std::to_string(conflict++); 
  }
  return designName;
}

std::string SNLVRLDumper::createInstanceName(const SNLInstance* instance, DesignInsideAnonymousNaming& naming) {
  auto design = instance->getDesign();
  auto instanceID = instance->getID();
  std::string instanceName = "instance_" + std::to_string(instanceID);
  int conflict = 0;
  std::string uniqueInstanceName(instanceName);
  while (design->getInstance(NLName(uniqueInstanceName))
      && naming.instanceNameSet_.find(uniqueInstanceName) != naming.instanceNameSet_.end()) {
    uniqueInstanceName = instanceName + "_" + std::to_string(conflict++); 
  }
  naming.instanceNameSet_.insert(uniqueInstanceName);
  naming.instanceNames_[instance->getID()] = uniqueInstanceName;
  return uniqueInstanceName;
}

NLName SNLVRLDumper::createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming) {
  auto design = net->getDesign();
  auto netID = net->getID();
  std::string netName = "net_" + std::to_string(netID);
  int conflict = 0;
  NLName uniqueNetName(netName);
  while (naming.netTermNameSet_.find(NLName(uniqueNetName)) != naming.netTermNameSet_.end()) {
    uniqueNetName = NLName(netName + "_" + std::to_string(conflict++)); 
  }
  naming.netTermNameSet_.insert(uniqueNetName);
  naming.netNames_[net->getID()] = uniqueNetName;
  return uniqueNetName;
}

NLName SNLVRLDumper::createUnusedWireName(
  const SNLInstTerm* instTerm,
  DesignInsideAnonymousNaming& naming) {
  auto key = std::make_pair(
    instTerm->getInstance()->getID(),
    instTerm->getBitTerm()->getFlatID());
  auto it = naming.unusedWireNames_.find(key);
  if (it != naming.unusedWireNames_.end()) {
    return it->second;
  }

  std::string wireName =
    "_naja_unused_" + std::to_string(instTerm->getInstance()->getID()) +
    "_" + std::to_string(instTerm->getBitTerm()->getFlatID());
  int conflict = 0;
  NLName uniqueWireName(wireName);
  while (naming.netTermNameSet_.find(uniqueWireName) != naming.netTermNameSet_.end()) {
    uniqueWireName = NLName(wireName + "_" + std::to_string(conflict++));
  }
  naming.netTermNameSet_.insert(uniqueWireName);
  naming.unusedWireNames_[key] = uniqueWireName;
  return uniqueWireName;
}

NLName SNLVRLDumper::getNetName(const SNLNet* net, const DesignInsideAnonymousNaming& naming) {
  if (net->isUnnamed()) {
    auto it = naming.netNames_.find(net->getID());
    assert(it != naming.netNames_.end());
    return it->second;
  } else {
    return net->getName();
  }
}

NLName SNLVRLDumper::getUnusedWireName(
  const SNLInstTerm* instTerm,
  const DesignInsideAnonymousNaming& naming) {
  auto key = std::make_pair(
    instTerm->getInstance()->getID(),
    instTerm->getBitTerm()->getFlatID());
  auto it = naming.unusedWireNames_.find(key);
  assert(it != naming.unusedWireNames_.end());
  return it->second;
}

std::string SNLVRLDumper::getBitNetString(
  const SNLBitNet* bitNet,
  const DesignInsideAnonymousNaming& naming) {
  if (!bitNet) {
    throw SNLVRLDumperException("Error while writing verilog: unexpected unconnected bit net");
  }
  if (bitNet->isAssign0()) {
    return "1'b0";
  }
  if (bitNet->isAssign1()) {
    return "1'b1";
  }
  if (bitNet->isAssignX()) {
    return "1'bx";
  }
  if (bitNet->isAssignZ()) {
    return "1'bz";
  }
  if (auto scalarNet = dynamic_cast<const SNLScalarNet*>(bitNet)) {
    auto netName = getNetName(scalarNet, naming);
    return dumpName(netName.getString());
  }
  auto busNetBit = static_cast<const SNLBusNetBit*>(bitNet);
  auto bus = busNetBit->getBus();
  auto busName = getNetName(bus, naming);
  return dumpName(busName.getString()) + "[" + std::to_string(busNetBit->getBit()) + "]";
}

void SNLVRLDumper::dumpAttributes(
  const NLObject* object,
  std::ostream& o,
  AttributeDumpSite site) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpAttributesDuration,
    detailedPerfReport_.dumpAttributesCalls);
  const bool perfActive = detailedPerfReport_.enabled && detailedPerfReport_.sessionActive;
  std::chrono::steady_clock::time_point siteStart {};
  if (perfActive) {
    // LCOV_EXCL_START
    siteStart = std::chrono::steady_clock::now();
    switch (site) {
      case AttributeDumpSite::Design:
        ++detailedPerfReport_.dumpAttributesDesignCalls;
        break;
      case AttributeDumpSite::Instance:
        ++detailedPerfReport_.dumpAttributesInstanceCalls;
        break;
      case AttributeDumpSite::Net:
        ++detailedPerfReport_.dumpAttributesNetCalls;
        break;
      // LCOV_EXCL_STOP
    }
  }
  size_t dumpedSNLAttributes = 0;
  size_t dumpedRTLInfos = 0;
  std::chrono::steady_clock::time_point snlAttributesStart {};
  if (perfActive) {
    snlAttributesStart = std::chrono::steady_clock::now(); // LCOV_EXCL_LINE
  }
  for (const auto& attribute: SNLAttributes::getAttributes(object)) {
    ++dumpedSNLAttributes;
    o << "(* ";
    o << attribute.getName().getString();
    if (attribute.hasValue()) {
      o << "=";
      if (attribute.getValue().isString()) {
        o << "\"";
      }
      o << attribute.getValue().getString();
      if (attribute.getValue().isString()) {
        o << "\"";
      }
    }
    o << " *)\n";
  }
  if (perfActive) {
    // LCOV_EXCL_START
    detailedPerfReport_.dumpAttributesSNLAttributesDuration +=
      std::chrono::steady_clock::now() - snlAttributesStart;
    // LCOV_EXCL_STOP
  }
  if (configuration_.isDumpRTLInfosAsAttributes()) {
    const SNLRTLInfos* rtlInfos = nullptr;
    if (auto design = dynamic_cast<const SNLDesign*>(object)) {
      rtlInfos = design->getRTLInfos();
    } else if (auto designObject = dynamic_cast<const SNLDesignObject*>(object)) {
      rtlInfos = designObject->getRTLInfos();
    }
    std::chrono::steady_clock::time_point rtlInfosStart {};
    if (perfActive) { // LCOV_EXCL_LINE
      rtlInfosStart = std::chrono::steady_clock::now(); // LCOV_EXCL_LINE
    }
    std::vector<std::pair<std::string, std::string>> attributes;
    if (rtlInfos) {
      switch (configuration_.getRTLInfoDumpMode()) {
        case RTLInfoDumpMode::None:
          break; // LCOV_EXCL_LINE
        case RTLInfoDumpMode::VerboseAttributes:
          attributes = rtlInfos->getDumpAttributes();
          break;
        case RTLInfoDumpMode::CompactAttribute:
          if (auto compactSourceLoc = rtlInfos->getCompactSourceLocAttribute()) {
            attributes.push_back(std::move(*compactSourceLoc));
          }
          for (const auto& info : rtlInfos->getInfos()) {
            attributes.emplace_back(info.first.getString(), info.second);
          }
          break;
      }
    }
    {
      for (const auto& info : attributes) {
        ++dumpedRTLInfos;
        o << "(* ";
        o << info.first;
        if (!info.second.empty()) {
          o << "=";
          const bool valueIsNumber = isUnsignedDecimal(info.second);
          if (!valueIsNumber) {
            o << "\"";
          }
          o << info.second;
          if (!valueIsNumber) {
            o << "\"";
          }
        }
        o << " *)\n";
      }
    }
    if (perfActive) {
      // LCOV_EXCL_START
      detailedPerfReport_.dumpAttributesRTLInfosDuration +=
        std::chrono::steady_clock::now() - rtlInfosStart;
      // LCOV_EXCL_STOP
    }
  }
  const size_t dumpedAttributes = dumpedSNLAttributes + dumpedRTLInfos;
  if (perfActive) {
    // LCOV_EXCL_START
    if (dumpedAttributes == 0) {
      ++detailedPerfReport_.dumpAttributesEmptyCalls;
    } else {
      ++detailedPerfReport_.dumpAttributesNonEmptyCalls;
    }
    detailedPerfReport_.dumpedAttributesCount += dumpedAttributes;
    detailedPerfReport_.dumpedSNLAttributesCount += dumpedSNLAttributes;
    detailedPerfReport_.dumpedRTLInfosCount += dumpedRTLInfos;
    const auto siteDuration = std::chrono::steady_clock::now() - siteStart;
    switch (site) {
      case AttributeDumpSite::Design:
        detailedPerfReport_.dumpAttributesDesignDuration += siteDuration;
        detailedPerfReport_.dumpedAttributesDesignCount += dumpedAttributes;
        break;
      case AttributeDumpSite::Instance:
        detailedPerfReport_.dumpAttributesInstanceDuration += siteDuration;
        detailedPerfReport_.dumpedAttributesInstanceCount += dumpedAttributes;
        break;
      case AttributeDumpSite::Net:
        detailedPerfReport_.dumpAttributesNetDuration += siteDuration;
        detailedPerfReport_.dumpedAttributesNetCount += dumpedAttributes;
        break;
    }
    // LCOV_EXCL_STOP
  }
}

void SNLVRLDumper::dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpInterfaceDuration,
    detailedPerfReport_.dumpInterfaceCalls);
  auto interfaceTerms = getVerilogInterfaceTerms(design);
  size_t nbChars = std::char_traits<char>::length("module  (");
  nbChars += design->getName().getString().size();
  o << "(";
  bool first = true;
  for (auto term: interfaceTerms) {
    if (not first) {
      o << ",";
      nbChars += 1;
      if (nbChars > 80) {
        o << '\n';
        nbChars = 0;
      }
      nbChars += 1;
      o << " ";
    } else {
      first = false;
    }
    nbChars += dumpDirection(term, o) + 1;
    o << " ";
    if (auto bus = dynamic_cast<const SNLBusTerm*>(term)) {
      o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
      nbChars += 3 + std::to_string(bus->getMSB()).size() + std::to_string(bus->getLSB()).size();
    }
    const auto termName = term->getName().getString();
    nbChars += termName.size();
    o << dumpName(termName);
    if (configuration_.isVerilatorSplitPackedSignals() && publicDesign_ &&
        design != publicDesign_ && term->getWidth() > 1 &&
        dynamic_cast<const SNLBusTerm*>(term)) {
      o << " /* verilator split_var */";
    }
  }
  o << ");";
}

bool SNLVRLDumper::dumpNet(const SNLNet* net, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  if (net->isAssignConstant()) {
    return false;
  }
  NLName netName;
  if (net->isUnnamed()) {
    netName = createNetName(net, naming);
  } else {
    netName = net->getName();
  }
  dumpAttributes(net, o, AttributeDumpSite::Net);
  o << "wire ";
  if (auto bus = dynamic_cast<const SNLBusNet*>(net)) {
    o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
  }
  o << dumpName(netName.getString());
  if (configuration_.isVerilatorSplitPackedSignals() && net->getWidth() > 1 &&
      dynamic_cast<const SNLBusNet*>(net)) {
    o << " /* verilator split_var */";
  }
  o << ";" << '\n';
  return true;
}

void SNLVRLDumper::dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpNetsDuration,
    detailedPerfReport_.dumpNetsCalls);
  bool atLeastOne = false;
  for (auto net: design->getNets()) {
    if (not net->isUnnamed()) {
      auto name = net->getName();
      if (design->getTerm(name)) {
        continue;
      }
    }
    bool dumped = dumpNet(net, o, naming);
    if (dumped) {
      atLeastOne = true;
    }
  }
  if (atLeastOne) {
    o << '\n';
  }
}

void SNLVRLDumper::collectUnusedWireNames(
  const SNLDesign* design,
  DesignInsideAnonymousNaming& naming) {
  for (auto instance: design->getInstances()) {
    if (NLDB0::isAssign(instance->getModel())) {
      continue;
    }
    if (NLDB0::isGate(instance->getModel())) {
      for (auto instTerm: instance->getInstTerms()) {
        if (not instTerm->getNet()) {
          createUnusedWireName(instTerm, naming);
        }
      }
      continue;
    }

    const SNLTerm* previousTerm = nullptr;
    InstTermVector instTerms;
    for (auto instTerm: instance->getInstTerms()) {
      auto bitTerm = instTerm->getBitTerm();
      const SNLTerm* currentTerm = nullptr;
      if (dynamic_cast<SNLScalarTerm*>(bitTerm)) {
        currentTerm = bitTerm;
      } else {
        auto busTermBit = static_cast<SNLBusTermBit*>(bitTerm);
        currentTerm = busTermBit->getBus();
      }
      if (currentTerm != previousTerm) {
        if (previousTerm) {
          const bool hasConnected = std::any_of(
            instTerms.begin(),
            instTerms.end(),
            [](const SNLInstTerm* term) { return term->getNet() != nullptr; });
          if (hasConnected) {
            for (auto previousInstTerm: instTerms) {
              if (not previousInstTerm->getNet()) {
                createUnusedWireName(previousInstTerm, naming);
              }
            }
          }
        }
        instTerms.clear();
      }
      instTerms.push_back(instTerm);
      previousTerm = currentTerm;
    }
    if (previousTerm) {
      const bool hasConnected = std::any_of(
        instTerms.begin(),
        instTerms.end(),
        [](const SNLInstTerm* term) { return term->getNet() != nullptr; });
      if (hasConnected) {
        for (auto instTerm: instTerms) {
          if (not instTerm->getNet()) {
            createUnusedWireName(instTerm, naming);
          }
        }
      }
    }
  }
}

void SNLVRLDumper::dumpUnusedWireDeclarations(
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  if (naming.unusedWireNames_.empty()) {
    return;
  }
  for (const auto& [_, name]: naming.unusedWireNames_) {
    o << "wire " << dumpName(name.getString()) << ";" << '\n';
  }
  o << '\n';
}

void SNLVRLDumper::dumpInsTermConnectivity(
  const SNLTerm* term,
  BitNetVector& termNets,
  const InstTermVector& instTerms,
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  assert(termNets.size() == instTerms.size());
  if (std::any_of(termNets.begin(), termNets.end(), [](const SNLBitNet* n){ return n != nullptr; })) {
    assert(not termNets.empty());
    bool concatenation = false;
    bool firstElement = true;
    std::string connectionStr;
    ContiguousNetBits contiguousBits;
    auto dumpRangeWithNaming = [&](ContiguousNetBits& bits) {
      if (bits.empty()) {
        return;
      }
      if (bits[0]->isAssignConstant()) {
        dumpConstantRange(bits, firstElement, concatenation, connectionStr);
        return;
      }
      if (not firstElement) {
        connectionStr += ", ";
        concatenation = true;
      } else {
        firstElement = false;
      }
      if (not isCanonicalBusRangeOrder(bits)) {
        concatenation = true;
        for (size_t i = 0; i < bits.size(); ++i) {
          if (i != 0) {
            connectionStr += ", ";
          }
          auto* bit = static_cast<SNLBusNetBit*>(bits[i]);
          auto bitName = getNetName(bit->getBus(), naming);
          connectionStr += dumpName(bitName.getString()) + "[";
          connectionStr += std::to_string(bit->getBit());
          connectionStr += "]";
        }
        bits.clear();
        return;
      }
      auto* rangeMSBBit = static_cast<SNLBusNetBit*>(bits[0]);
      auto rangeMSB = rangeMSBBit->getBit();
      auto* rangeLSBBit = static_cast<SNLBusNetBit*>(bits[bits.size() - 1]);
      auto rangeLSB = rangeLSBBit->getBit();
      auto* bus = rangeMSBBit->getBus();
      auto busName = getNetName(bus, naming);
      auto busMSB = bus->getMSB();
      auto busLSB = bus->getLSB();
      if (rangeMSB == busMSB && rangeLSB == busLSB) {
        connectionStr += dumpName(busName.getString());
      } else if (rangeMSB == rangeLSB) {
        connectionStr += dumpName(busName.getString()) + "[";
        connectionStr += std::to_string(rangeMSB);
        connectionStr += "]";
      } else {
        connectionStr += dumpName(busName.getString()) + "[";
        connectionStr += std::to_string(rangeMSB);
        connectionStr += ":";
        connectionStr += std::to_string(rangeLSB);
        connectionStr += "]";
      }
      bits.clear();
    };
    for (size_t i = 0; i < termNets.size(); ++i) {
      auto net = termNets[i];
      if (net) {
        if (net->isAssignConstant()) {
          if (not contiguousBits.empty()) {
            SNLBitNet* previousBit = contiguousBits.back();
            if (not previousBit->isAssignConstant()) {
              dumpRangeWithNaming(contiguousBits);
              contiguousBits = { net };
            } else {
              contiguousBits.push_back(net);
            }
          } else {
            contiguousBits.push_back(net);
          }
        } else if (dynamic_cast<SNLScalarNet*>(net)) {
          dumpRangeWithNaming(contiguousBits);
          NLName netName = getNetName(net, naming);
          if (not firstElement) {
            connectionStr += ", ";
            concatenation = true;
          } else {
            firstElement = false;
          }
          connectionStr += dumpName(netName.getString());
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto busNet = busNetBit->getBus();
          if (not contiguousBits.empty()) {
            SNLBitNet* previousBit = contiguousBits.back();
            if (previousBit->isAssignConstant()) {
              dumpRangeWithNaming(contiguousBits);
              contiguousBits = { busNetBit };
            } else {
              SNLBusNetBit* previousBusBit = static_cast<SNLBusNetBit*>(previousBit);
              if (busNet == previousBusBit->getBus()
                and ((previousBusBit->getBit() == busNetBit->getBit()+1)
                or (previousBusBit->getBit() == busNetBit->getBit()-1))) {
                contiguousBits.push_back(busNetBit);
              } else {
                dumpRangeWithNaming(contiguousBits);
                contiguousBits = { busNetBit };
              }
            }
          } else {
            contiguousBits.push_back(busNetBit);
          }
        }
      } else {
        dumpRangeWithNaming(contiguousBits);
        //dump dummy bit
        if (not firstElement) {
          connectionStr += ", ";
          concatenation = true;
        } else {
          firstElement = false;
        }
        connectionStr += dumpName(getUnusedWireName(instTerms[i], naming).getString());
      }
    }
    if (not contiguousBits.empty()) {
      dumpRangeWithNaming(contiguousBits);
    }
    o << "  ." + term->getName().getString() + "(";
    if (concatenation) {
      o << "{";
    }
    o << connectionStr;
    if (concatenation) {
      o << "}";
    }
    o << ")";
  } else {
    o << "  ." << dumpName(term->getName().getString()) << "()"; 
  }
}

void SNLVRLDumper::dumpInstanceInterface(
  const SNLInstance* instance,
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  o << " (";
  BitNetVector termNets;
  InstTermVector instTerms;
  SNLTerm* previousTerm = nullptr;
  bool first = true;
  for (auto instTerm: instance->getInstTerms()) {
    SNLTerm* currentTerm = nullptr;
    auto bitTerm = instTerm->getBitTerm();
    if (dynamic_cast<SNLScalarTerm*>(bitTerm)) {
      currentTerm = bitTerm;
    } else {
      auto busTermBit = static_cast<SNLBusTermBit*>(bitTerm);
      currentTerm = busTermBit->getBus();
    }
    if (currentTerm == previousTerm) {
      termNets.push_back(instTerm->getNet());
    } else {
      if (previousTerm) {
        if (first) {
          first = false;
        } else {
          o << ",";
        }
        o << '\n';
        dumpInsTermConnectivity(previousTerm, termNets, instTerms, o, naming);
      }
      termNets = { instTerm->getNet() };
      instTerms = { instTerm };
    }
    if (currentTerm == previousTerm) {
      instTerms.push_back(instTerm);
    }
    previousTerm = currentTerm;
  }
  if (previousTerm) {
    if (first) {
      first = false;
    } else {
      o << ",";
    }
    o << '\n';
    dumpInsTermConnectivity(previousTerm, termNets, instTerms, o, naming);
  }
  if (NLDB0::isMemory(instance->getModel())) {
    const auto signature = NLDB0::getMemorySignature(instance->getModel());
    const auto effective = NLDB0::getMemorySignature(instance);
    const auto writeSlots = std::max(size_t{1}, effective.writePorts);
    // The shared Verilog module retains controls absent from specialized models.
    // Size tie-offs explicitly: unsized fill literals trigger replication-limit
    // warnings in downstream tools for wide memories, even on unused controls.
    if (signature.writePorts == 0) {
      o << ",\n  .CLK(1'b0), .RST(1'b0), .WADDR(" << writeSlots * effective.abits
        << "'b0), .WDATA(" << writeSlots * effective.width << "'b0), .WE("
        << writeSlots << "'b0)";
    }
    if (!signature.independentWriteClocks) {
      o << ",\n  .WCLK(" << writeSlots << "'b0), .WMASK("
        << writeSlots * effective.width << "'b0)";
    }
  }
  o << '\n' << ")";
}

void SNLVRLDumper::dumpInstParameters(
  const SNLInstance* instance,
  std::ostream& o) {
  std::vector<std::pair<const SNLParameter*, std::string>> dumpedParameters;
  // Replacing a specialized primitive with a generic module can change its defaults.
  for (auto* parameter: instance->getModel()->getParameters()) {
    const auto* instParameter = instance->getInstParameter(parameter->getName());
    const auto value = instParameter ? instParameter->getValue() : parameter->getValue();
    if (shouldDumpParameter(instance, parameter, value)) {
      dumpedParameters.emplace_back(parameter, value);
    }
  }
  std::sort(
    dumpedParameters.begin(),
    dumpedParameters.end(),
    [](const auto& lhs, const auto& rhs) {
      return lhs.first->getName().getString() < rhs.first->getName().getString();
    });
  if (not dumpedParameters.empty()) {
    bool first = true;
    o << "#(" << '\n';
    for (const auto& [parameter, value]: dumpedParameters) {
      if (not first) {
        o << "," << '\n';
      }
      first = false;
      o << "  ." << parameter->getName().getString();
      o << "(";
      if (parameter->getType() == SNLParameter::Type::String) {
        o << "\"" << value << "\"";
      } else if (parameter->getType() == SNLParameter::Type::Boolean) {
        if (value=="0" or value=="FALSE") {
          o << (NLDB0::isDB0Primitive(instance->getModel()) ? "0" : "\"FALSE\"");
        } else if (value=="1" or value=="TRUE") {
          o << (NLDB0::isDB0Primitive(instance->getModel()) ? "1" : "\"TRUE\"");
        } else {
          std::ostringstream reason;
          reason << "Error while writing verilog: in design " << instance->getDesign()->getString();
          reason << ", for instance " << instance->getName().getString();
          reason << ", wrong boolean value in instance parameter " << parameter->getDescription();
          reason << ": " << value;
          throw SNLVRLDumperException(reason.str());
        }
      } else {
        o << value;
      }
      o << ")";
    }
    o << '\n' << ") ";
  }
}

// LCOV_EXCL_START
bool SNLVRLDumper::dumpAssignInstance(
  const SNLInstance* instance,
  std::ostream& o,
  DesignInsideAnonymousNaming& naming) {
  assert(NLDB0::isAssign(instance->getModel()));
  const SNLBitNet* inputNet = nullptr;
  const SNLBitNet* outputNet = nullptr;
  if (not getAssignConnectivity(instance, inputNet, outputNet)) {
    return false;
  }

  std::string instanceName;
  if (instance->isUnnamed()) {
    instanceName = createInstanceName(instance, naming);
  } else {
    instanceName = instance->getName().getString();
  }

  dumpAttributes(instance, o, AttributeDumpSite::Instance);
  o << "assign_module " << dumpName(instanceName) << " ("
    << getAssignInputString(
      inputNet,
      [&](const SNLBitNet* bitNet) { return getBitNetString(bitNet, naming); })
    << ", "
    << getBitNetString(outputNet, naming)
    << ");" << '\n';
  return true;
}
// LCOV_EXCL_STOP

bool SNLVRLDumper::dumpInstance(
  const SNLInstance* instance,
  std::ostream& o,
  DesignInsideAnonymousNaming& naming) {
  for (auto parameter: instance->getModel()->getParameters()) {
    if (not parameter->hasDefaultValue() &&
        not instance->getInstParameter(parameter->getName())) {
      throw SNLVRLDumperException(
        "Missing value for required parameter " + parameter->getName().getString() +
        " on instance " + instance->getString());
    }
  }
  if (NLDB0::isMux2(instance->getModel())) {
    emitNajaMux2Model_ = true;
    emitNajaPrimitiveModels_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << getNajaPrimitiveModelName(instance->getModel()) << " ";
    dumpInstParameters(instance, o);
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
  if (NLDB0::isMemory(instance->getModel())) {
    emitNajaMemModel_ = true;
    emitNajaPrimitiveModels_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_mem ";
    dumpInstParameters(instance, o);
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
  if (NLDB0::isMultiply(instance->getModel())) {
    const auto width = NLDB0::getMultiplyWidth(instance);
    emitNajaPrimitiveModels_ = true;
    std::string instanceName = instance->isUnnamed()
      ? createInstanceName(instance, naming) : instance->getName().getString();
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_mul #(.WIDTH(" << width << ")) " << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";\n";
    return true;
  }
  if (NLDB0::isAddSub(instance->getModel())) {
    const auto width = NLDB0::getAddSubWidth(instance);
    emitNajaPrimitiveModels_ = true;
    std::string instanceName = instance->isUnnamed()
      ? createInstanceName(instance, naming) : instance->getName().getString();
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << (NLDB0::isSubtract(instance->getModel()) ? "naja_sub" : "naja_add")
      << " #(.WIDTH(" << width << ")) " << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";\n";
    return true;
  }
  if (NLDB0::isShift(instance->getModel())) {
    const auto signature = NLDB0::getShiftSignature(instance);
    emitNajaPrimitiveModels_ = true;
    const auto instanceName = instance->isUnnamed() ? createInstanceName(instance, naming)
        : instance->getName().getString();
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    // Canonical model parameters may have no explicit instance overrides.
    o << "naja_shift #(.WIDTH(" << signature.width << "), .COUNT_WIDTH("
      << signature.countWidth << "), .RIGHT(" << signature.right
      << "), .ARITHMETIC(" << signature.arithmetic << ")) " << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";\n";
    return true;
  }
  if (NLDB0::isVectorUpdate(instance->getModel())) {
    const auto signature = NLDB0::getVectorUpdateSignature(instance);
    emitNajaPrimitiveModels_ = true;
    const auto instanceName = instance->isUnnamed() ? createInstanceName(instance, naming)
        : instance->getName().getString();
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_vector_update #(.WIDTH(" << signature.width << "), .UPDATE_WIDTH("
      << signature.updateWidth << "), .ABITS(" << signature.abits << "), .SIGNED("
      << signature.isSigned << ")) " << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";\n";
    return true;
  }
  if (NLDB0::isVectorSelect(instance->getModel())) {
    const auto signature = NLDB0::getVectorSelectSignature(instance);
    emitNajaPrimitiveModels_ = true;
    const auto instanceName = instance->isUnnamed() ? createInstanceName(instance, naming)
        : instance->getName().getString();
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_vector_select #(.WIDTH(" << signature.width << "), .SELECT_WIDTH("
      << signature.selectWidth << "), .ABITS(" << signature.abits << "), .SIGNED("
      << signature.isSigned << ")) " << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";\n";
    return true;
  }
  if (NLDB0::isTableSelect(instance->getModel())) {
    emitNajaTableSelectModel_ = true;
    emitNajaPrimitiveModels_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_table_select ";
    dumpInstParameters(instance, o);
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
  if (NLDB0::isDivMod(instance->getModel())) {
    emitNajaPrimitiveModels_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << getNajaPrimitiveModelName(instance->getModel()) << " ";
    dumpInstParameters(instance, o);
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
  if (auto* model = instance->getModel(); isNajaSequentialModel(model)) {
    emitNajaPrimitiveModels_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    std::string modelName = getNajaPrimitiveModelName(model);
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << modelName << " ";
    dumpInstParameters(instance, o);
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
  if (auto* model = instance->getModel();
      NLDB0::isFA(model)) {
    emitNajaPrimitiveModels_ = true;
  }
  if (NLDB0::isGate(instance->getModel())) {
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    auto gateName = NLDB0::getGateName(instance->getModel());
    o << gateName << " ";
    if (not instance->isUnnamed()) {
      o << instance->getName().getString();
    }
    o << "(";
    bool first = true;
    for (auto instTerm: instance->getInstTerms()) {
      if (first) {
        first = false;
      } else {
        o << ", ";
      }
      auto net = instTerm->getNet();
      if (net) {
        o << getBitNetString(net, naming);
      } else {
        o << dumpName(getUnusedWireName(instTerm, naming).getString());
      }
    }
    o << ");";
    o << '\n';
    return true;
  }
  assert(not NLDB0::isAssign(instance->getModel()));
  std::string instanceName;
  if (instance->isUnnamed()) {
    instanceName = createInstanceName(instance, naming);
  } else {
    instanceName = instance->getName().getString();
  }
  dumpAttributes(instance, o, AttributeDumpSite::Instance);
  auto model = instance->getModel();
  if (not model->isUnnamed()) { //FIXME !!
    o << dumpName(model->getName().getString()) << " ";
  }
  dumpInstParameters(instance, o);
  o << dumpName(instanceName);
  dumpInstanceInterface(instance, o, naming);
  o << ";" << '\n';
  return true;
}

void SNLVRLDumper::dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpInstancesDuration,
    detailedPerfReport_.dumpInstancesCalls);
  bool blankLine = false;
  auto bitNetToString = [&](const SNLBitNet* bitNet) {
    return getBitNetString(bitNet, naming);
  };
  auto busNetToString = [&](const SNLBusNet* busNet) {
    auto busName = getNetName(busNet, naming);
    return dumpName(busName.getString());
  };
  auto dumpAssignInstances = [&](const std::vector<const SNLInstance*>& assignInstances) {
    std::vector<AssignEmission> emissions;
    size_t i = 0;
    while (i < assignInstances.size()) {
      const SNLBitNet* inputNet = nullptr;
      const SNLBitNet* outputNet = nullptr;
      if (not getAssignConnectivity(assignInstances[i], inputNet, outputNet)) {
        ++i;
        continue;
      }
      AssignGroup group;
      if (not initializeAssignGroup(inputNet, outputNet, group)) {
        AssignEmission emission;
        emission.text_ =
          "assign " + getBitNetString(outputNet, naming) +
          " = " + getAssignInputString(inputNet, bitNetToString) + ";";
        emission.hasBusChunk_ = createSingleAssignBusChunk(
          inputNet,
          outputNet,
          bitNetToString,
          emission.busChunk_);
        emissions.push_back(std::move(emission));
        ++i;
        continue;
      }

      size_t groupSize = 1;
      while (i + groupSize < assignInstances.size()) {
        const SNLBitNet* nextInputNet = nullptr;
        const SNLBitNet* nextOutputNet = nullptr;
        if (not getAssignConnectivity(assignInstances[i + groupSize], nextInputNet, nextOutputNet)) {
          break;
        }
        if (not appendAssignGroup(group, nextInputNet, nextOutputNet)) {
          break;
        }
        ++groupSize;
      }

      AssignEmission emission;
      if (group.outputBits_.size() > 1) {
        emission.hasBusChunk_ = true;
        emission.busChunk_ = createAssignGroupBusChunk(
          group,
          bitNetToString,
          busNetToString);
        emission.text_ = getAssignBusChunkText(
          emission.busChunk_,
          busNetToString);
      } else {
        emission.text_ =
          "assign " + getBitNetString(outputNet, naming) +
          " = " + getAssignInputString(inputNet, bitNetToString) + ";";
        emission.hasBusChunk_ = createSingleAssignBusChunk(
          inputNet,
          outputNet,
          bitNetToString,
          emission.busChunk_);
      }
      emissions.push_back(std::move(emission));
      i += groupSize;
    }

    size_t emissionIndex = 0;
    while (emissionIndex < emissions.size()) {
      size_t consumed = 1;
      bool emittedWholeBusAssign = false;
      if (emissions[emissionIndex].hasBusChunk_) {
        std::vector<const AssignBusChunk*> sameBusChunks;
        auto outputBus = emissions[emissionIndex].busChunk_.outputBus_;
        while (emissionIndex + sameBusChunks.size() < emissions.size()) {
          const auto& emission = emissions[emissionIndex + sameBusChunks.size()];
          if (not emission.hasBusChunk_ or emission.busChunk_.outputBus_ != outputBus) {
            break;
          }
          sameBusChunks.push_back(&emission.busChunk_);
        }

        std::vector<const AssignBusChunk*> sortedChunks;
        if (areChunksWholeBusAssignable(sameBusChunks, sortedChunks)) {
          if (blankLine) {
            o << '\n';
          }
          o << buildWholeBusAssignText(
            sortedChunks,
            bitNetToString,
            busNetToString) << '\n';
          blankLine = true;
          consumed = sameBusChunks.size();
          emittedWholeBusAssign = true;
        }
      }

      if (not emittedWholeBusAssign) {
        if (blankLine) {
          o << '\n';
        }
        o << emissions[emissionIndex].text_ << '\n';
        blankLine = true;
      }
      emissionIndex += consumed;
    }
  };

  std::vector<const SNLInstance*> assignInstances;
  for (auto instance: design->getInstances()) {
    if (NLDB0::isAssign(instance->getModel())) {
      if (configuration_.isDumpAssignsAsInstances()) {
        // LCOV_EXCL_START
        if (not assignInstances.empty()) {
          dumpAssignInstances(assignInstances);
          assignInstances.clear();
        }
        if (blankLine) {
          o << '\n';
        }
        blankLine = dumpAssignInstance(instance, o, naming);
        // LCOV_EXCL_STOP
        continue;
      }
      assignInstances.push_back(instance);
      continue;
    }
    if (not assignInstances.empty()) {
      dumpAssignInstances(assignInstances);
      assignInstances.clear();
    }
    if (blankLine) {
      o << '\n';
    }
    blankLine = dumpInstance(instance, o, naming);
  }
  if (not assignInstances.empty()) {
    dumpAssignInstances(assignInstances);
  }
}

void SNLVRLDumper::dumpTermNetAssign(
  const SNLDesign* design,
  const SNLTerm::Direction& direction,
  const std::string& termNetName,
  const std::string& netName,
  std::ostream& o) {
    switch (direction) {
      case SNLTerm::Direction::Input:
        o << "assign " << netName << " = " << termNetName << ";" << '\n';
        break;
      case SNLTerm::Direction::Output:
        o << "assign " << termNetName << " = " << netName << ";" << '\n';
        break;
      default:
        {
          std::ostringstream reason;
          reason << "Error while writing verilog of design " << design->getString();
          reason << ", wrong direction (";
          reason << direction.getString() << ") in assign for dumping: ";
          reason << "assign " << termNetName << " = " << netName;
          throw SNLVRLDumperException(reason.str());
        }
    }
}

void SNLVRLDumper::dumpTermAssigns(
  const SNLDesign* design, std::ostream& o, const DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpTermAssignsDuration,
    detailedPerfReport_.dumpTermAssignsCalls);
  bool atLeastOne = false;
  for (auto term: design->getBitTerms()) {
    auto net = term->getNet();
    if (net) {
      if (term->getDirection() == SNLTerm::Direction::Output &&
          (net->isAssign0() || net->isAssign1() || net->isAssignX() || net->isAssignZ())) {
        // Constants substitute at input uses, but must also drive top-level
        // outputs, including bits whose net and terminal share the same name.
        auto* busBit = dynamic_cast<SNLBusTermBit*>(term);
        const auto name = busBit ?
            dumpName(busBit->getBus()->getName().getString()) + "[" + std::to_string(busBit->getBit()) + "]" :
            dumpName(term->getName().getString());
        o << "assign " << name << " = 1'b" << getAssignConstantBitValue(net) << ";\n";
        atLeastOne = true;
        continue;
      }
      if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          //same name ?
          if (scalarTerm->getName() != scalarNet->getName()) {
            //need assign
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              scalarTerm->getDirection(),
              dumpName(scalarTerm->getName().getString()),
              getBitNetString(scalarNet, naming),
              o);
          }
          //else: same name so already connected
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto bus = busNetBit->getBus();
          if (scalarTerm->getName() == bus->getName()) {
            std::ostringstream reason;
            reason << "Error while writing verilog: scalar terminal ";
            reason << scalarTerm->getString();
            reason << " and bus net ";
            reason << bus->getString();
            reason << " should not have the same name.";
            throw SNLVRLDumperException(reason.str());
          } else {
            //need assign
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              scalarTerm->getDirection(),
              dumpName(scalarTerm->getName().getString()),
              getBitNetString(busNetBit, naming),
              o);
          }
        }
      } else {
        auto busTermBit = static_cast<SNLBusTermBit*>(term);
        auto busTerm = busTermBit->getBus();
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          if (busTerm->getName() == scalarNet->getName()) {
            std::ostringstream reason;
            reason << "Error while writing verilog in design ";
            reason << design->getString() << ": ";
            reason << " bus terminal ";
            reason << busTerm->getString();
            reason << " and scalar net ";
            reason << scalarNet->getString();
            reason << " should not have the same name.";
            throw SNLVRLDumperException(reason.str());
          } else {
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              busTerm->getDirection(),
              dumpName(busTerm->getName().getString()) + "[" + std::to_string(busTermBit->getBit()) + "]",
              getBitNetString(scalarNet, naming),
              o);
          }
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto busNet = busNetBit->getBus();
          if (busTerm->getName() == busNet->getName()) {
            if (busTermBit->getBit() != busNetBit->getBit()) {
              std::ostringstream reason;
              reason << "Error while writing verilog in design ";
              reason << design->getString() << ":";
              reason << " bus terminal bit ";
              reason << busTermBit->getString();
              reason << " and bus net bit ";
              reason << busNetBit->getString();
              reason << " should have the same bit value.";
              throw SNLVRLDumperException(reason.str());
            }
          } else {
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              busTerm->getDirection(),
              dumpName(busTerm->getName().getString()) + "[" + std::to_string(busTermBit->getBit()) + "]",
              getBitNetString(busNetBit, naming),
              o
            );
          }
        }
      }
    }
  }
  if (atLeastOne) {
    o << '\n';
  }
}

void SNLVRLDumper::dumpParameter(const SNLParameter* parameter, std::ostream& o) {
  if (not parameter->hasDefaultValue()) {
    throw SNLVRLDumperException(
      "Cannot emit required parameter " + parameter->getName().getString() +
      " in design " + parameter->getDesign()->getString() +
      ": Verilog parameter declarations require a default value");
  }
  o << "parameter " << parameter->getName().getString() << " = ";
  if (parameter->getType()==SNLParameter::Type::String) {
    o << "\"" << parameter->getValue() << "\"";
  } else if (parameter->getType()==SNLParameter::Type::Boolean) {
    if (parameter->getValue()=="0") {
      o << "\"FALSE\"";
    } else if (parameter->getValue()=="1") {
      o << "\"TRUE\"";
    } else {
      std::ostringstream reason;
      reason << "Error while writing verilog: in design " << parameter->getDesign()->getString();
      reason << ", wrong boolean value in parameter " << parameter->getDescription();
      throw SNLVRLDumperException(reason.str());
    }
  } else {
    o << parameter->getValue();
  }
  o << " ;" << '\n';
}

void SNLVRLDumper::dumpParameters(const SNLDesign* design, std::ostream& o) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpParametersDuration,
    detailedPerfReport_.dumpParametersCalls);
  std::vector<const SNLParameter*> parameters;
  for (auto parameter: design->getParameters()) {
    parameters.push_back(parameter);
  }
  std::sort(
    parameters.begin(),
    parameters.end(),
    [](const SNLParameter* lhs, const SNLParameter* rhs) {
      return lhs->getName().getString() < rhs->getName().getString();
    });
  bool atLeastOne = false;
  for (auto parameter: parameters) {
    atLeastOne = true;
    dumpParameter(parameter, o);
  }
  if (atLeastOne) {
    o << '\n';
  }
}

void SNLVRLDumper::dumpOneDesign(const SNLDesign* design, std::ostream& o) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpOneDesignDuration,
    detailedPerfReport_.dumpOneDesignCalls);
  DesignInsideAnonymousNaming naming;
  for (auto term: getVerilogInterfaceTerms(design)) {
    if (not term->isUnnamed()) {
      naming.netTermNameSet_.insert(term->getName());
    }
  }
  for (auto net: design->getNets()) {
    if (not net->isUnnamed()) {
      naming.netTermNameSet_.insert(net->getName());
    }
  }
  if (design->isUnnamed()) {
    createDesignName(design);
  }
  collectUnusedWireNames(design, naming);
  dumpAttributes(design, o, AttributeDumpSite::Design);
  o << "module " << dumpName(design->getName().getString());

  dumpInterface(design, o, naming);
  o << '\n';

  dumpParameters(design, o);
  dumpNets(design, o, naming);
  dumpUnusedWireDeclarations(o, naming);
  dumpTermAssigns(design, o, naming);

  dumpInstances(design, o, naming);

  o << "endmodule //" << design->getName().getString();
  o << '\n';

}

void SNLVRLDumper::dumpNajaFAModel(std::ostream& o) {
  o << "module naja_fa(\n";
  o << "  input A,\n";
  o << "  input B,\n";
  o << "  input CI,\n";
  o << "  output S,\n";
  o << "  output CO\n";
  o << ");\n";
  o << "  assign {CO, S} = A + B + CI;\n";
  o << "endmodule //naja_fa\n";
}

void SNLVRLDumper::dumpNajaMux2Model(std::ostream& o) {
  o << "module naja_mux2 #(\n";
  o << "  parameter WIDTH = 1\n";
  o << ") (\n";
  o << "  input [WIDTH-1:0] A,\n";
  o << "  input [WIDTH-1:0] B,\n";
  o << "  input S,\n";
  o << "  output [WIDTH-1:0] Y\n";
  o << ");\n";
  o << "  assign Y = S ? B : A;\n";
  o << "endmodule //naja_mux2\n";
}

void SNLVRLDumper::dumpNajaMultiplyModel(std::ostream& o) {
  o << R"(module naja_mul #(parameter WIDTH = 1) (
  input [WIDTH-1:0] A,
  input [WIDTH-1:0] B,
  output [WIDTH-1:0] Y
);
  assign Y = A * B;
endmodule //naja_mul
)";
}

void SNLVRLDumper::dumpNajaAddSubModels(std::ostream& o) {
  o << R"(module naja_add #(parameter WIDTH = 1) (
  input [WIDTH-1:0] A,
  input [WIDTH-1:0] B,
  output [WIDTH-1:0] Y
);
  assign Y = A + B;
endmodule //naja_add
module naja_sub #(parameter WIDTH = 1) (
  input [WIDTH-1:0] A,
  input [WIDTH-1:0] B,
  output [WIDTH-1:0] Y
);
  assign Y = A - B;
endmodule //naja_sub
)";
}

void SNLVRLDumper::dumpNajaShiftModel(std::ostream& o) {
  o << R"(module naja_shift #(
  parameter WIDTH = 1,
  parameter COUNT_WIDTH = 1,
  parameter RIGHT = 0,
  parameter ARITHMETIC = 0
) (
  input [WIDTH-1:0] DATA,
  input [COUNT_WIDTH-1:0] COUNT,
  output [WIDTH-1:0] Y
);
  generate
    if (RIGHT && ARITHMETIC) begin
      assign Y = $signed(DATA) >>> COUNT;
    end else if (RIGHT) begin
      assign Y = DATA >> COUNT;
    end else begin
      assign Y = DATA << COUNT;
    end
  endgenerate
endmodule //naja_shift
)";
}

void SNLVRLDumper::dumpNajaVectorSelectModel(std::ostream& o) {
  o << R"(module naja_vector_select #(
  parameter WIDTH = 1,
  parameter SELECT_WIDTH = 1,
  parameter ABITS = 1,
  parameter SIGNED = 0
) (
  input [WIDTH-1:0] DATA,
  input [ABITS-1:0] OFFSET,
  output reg [SELECT_WIDTH-1:0] Y
);
  localparam CHECK_WIDTH = ABITS >= 32 ? ABITS+1 : 33;
  localparam TOTAL_WIDTH = WIDTH + 2*SELECT_WIDTH;
  localparam INDEX_WIDTH = $clog2(TOTAL_WIDTH);
  localparam signed [CHECK_WIDTH-1:0] UPPER = WIDTH;
  localparam signed [CHECK_WIDTH-1:0] LOWER = -SELECT_WIDTH;
  localparam signed [CHECK_WIDTH-1:0] PAD = SELECT_WIDTH;
  wire [TOTAL_WIDTH-1:0] padded = {{SELECT_WIDTH{1'bx}}, DATA, {SELECT_WIDTH{1'bx}}};
  reg signed [CHECK_WIDTH-1:0] checked_offset, position;
  always @* begin
    checked_offset = SIGNED ? {{(CHECK_WIDTH-ABITS){OFFSET[ABITS-1]}}, OFFSET}
                            : {{(CHECK_WIDTH-ABITS){1'b0}}, OFFSET};
    position = checked_offset + PAD;
    Y = {SELECT_WIDTH{1'bx}};
    if (((^OFFSET) !== 1'bx) && checked_offset < UPPER && checked_offset > LOWER)
      Y = padded[position[INDEX_WIDTH-1:0] +: SELECT_WIDTH];
  end
endmodule //naja_vector_select
)";
}

void SNLVRLDumper::dumpNajaVectorUpdateModel(std::ostream& o) {
  o << R"(module naja_vector_update #(
  parameter WIDTH = 1,
  parameter UPDATE_WIDTH = 1,
  parameter ABITS = 1,
  parameter SIGNED = 0
) (
  input [WIDTH-1:0] BASE,
  input [UPDATE_WIDTH-1:0] DATA,
  input [ABITS-1:0] OFFSET,
  input EN,
  output reg [WIDTH-1:0] Y
);
  localparam CHECK_WIDTH = ABITS >= 32 ? ABITS+1 : 33;
  localparam TOTAL_WIDTH = WIDTH + 2*UPDATE_WIDTH;
  localparam INDEX_WIDTH = $clog2(TOTAL_WIDTH);
  localparam signed [CHECK_WIDTH-1:0] UPPER = WIDTH;
  localparam signed [CHECK_WIDTH-1:0] LOWER = -UPDATE_WIDTH;
  localparam signed [CHECK_WIDTH-1:0] PAD = UPDATE_WIDTH;
  reg signed [CHECK_WIDTH-1:0] checked_offset, position;
  reg [TOTAL_WIDTH-1:0] updated;
  always @* begin
    checked_offset = SIGNED ? {{(CHECK_WIDTH-ABITS){OFFSET[ABITS-1]}}, OFFSET}
                            : {{(CHECK_WIDTH-ABITS){1'b0}}, OFFSET};
    updated = {{UPDATE_WIDTH{1'b0}}, BASE, {UPDATE_WIDTH{1'b0}}};
    position = checked_offset + PAD;
    // Padding allows native partial writes without truncating signed offsets.
    if (EN && ((^OFFSET) !== 1'bx) && checked_offset < UPPER && checked_offset > LOWER)
      updated[position[INDEX_WIDTH-1:0] +: UPDATE_WIDTH] = DATA;
    Y = updated[UPDATE_WIDTH +: WIDTH];
  end
endmodule //naja_vector_update
)";
}

void SNLVRLDumper::dumpNajaTableSelectModel(std::ostream& o) {
  o << "module naja_table_select #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter DEPTH = 1,\n";
  o << "  parameter ABITS = 1\n";
  o << ") (\n";
  o << "  input [WIDTH*DEPTH-1:0] DATA,\n";
  o << "  input [ABITS-1:0] ADDR,\n";
  o << "  output [WIDTH-1:0] Y\n";
  o << ");\n";
  o << "  localparam CHECK_WIDTH = ABITS > 32 ? ABITS : 32;\n";
  o << "  localparam INDEX_WIDTH = DEPTH > 1 ? $clog2(DEPTH) : 1;\n";
  o << "  localparam [CHECK_WIDTH-1:0] LIMIT = DEPTH;\n";
  o << "  wire [CHECK_WIDTH-1:0] checked_addr = {{(CHECK_WIDTH-ABITS){1'b0}}, ADDR};\n";
  // Each output depends on its own data column. A single packed select makes
  // event-driven scheduling infer false dependencies between unrelated bits.
  o << "  for (genvar bit_index = 0; bit_index < WIDTH; bit_index = bit_index + 1) begin: columns\n";
  o << "    wire [DEPTH-1:0] data;\n";
  o << "    for (genvar row = 0; row < DEPTH; row = row + 1) begin: rows\n";
  o << "      assign data[row] = DATA[row*WIDTH+bit_index];\n";
  o << "    end\n";
  // The full address must pass the bounds check before narrowing its index.
  // An unknown condition merges the selected bit with X, still producing X.
  o << "    assign Y[bit_index] = checked_addr < LIMIT ? data[checked_addr[INDEX_WIDTH-1:0]] : 1'bx;\n";
  o << "  end\n";
  o << "endmodule //naja_table_select\n";
}

void SNLVRLDumper::dumpNajaDFFModel(std::ostream& o) {
  o << "module naja_dff #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C) begin\n";
  o << "    Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dff\n";
}

void SNLVRLDumper::dumpNajaDLatchModel(std::ostream& o) {
  // Retention is intentional in this Verilog-compatible latch model.
  o << "/* verilator lint_off LATCH */\n";
  o << "module naja_dlatch #(\n";
  o << "  parameter WIDTH = 1\n";
  o << ") (\n";
  o << "  input E,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  always @* begin\n";
  o << "    if (E) Q = D;\n";
  o << "  end\n";
  o << "endmodule //naja_dlatch\n";
  o << "/* verilator lint_on LATCH */\n";
}

void SNLVRLDumper::dumpNajaDFFNModel(std::ostream& o) {
  o << "module naja_dffn #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(negedge C) begin\n";
  o << "    Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffn\n";
}

void SNLVRLDumper::dumpNajaDFFRNModel(std::ostream& o) {
  o << "module naja_dffrn #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  input RN,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C or negedge RN) begin\n";
  o << "    if (!RN) Q <= {WIDTH{1'b0}};\n";
  o << "    else Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffrn\n";
}

void SNLVRLDumper::dumpNajaDFFRModel(std::ostream& o) {
  o << "module naja_dffr #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  input R,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C or posedge R) begin\n";
  o << "    if (R) Q <= {WIDTH{1'b0}};\n";
  o << "    else Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffr\n";
}

void SNLVRLDumper::dumpNajaDFFSModel(std::ostream& o) {
  o << "module naja_dffs #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  input S,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C or posedge S) begin\n";
  o << "    if (S) Q <= {WIDTH{1'b1}};\n";
  o << "    else Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffs\n";
}

void SNLVRLDumper::dumpNajaDFFEModel(std::ostream& o) {
  o << "module naja_dffe #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  input E,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C) begin\n";
  o << "    if (E) Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffe\n";
}

void SNLVRLDumper::dumpNajaDFFREModel(std::ostream& o) {
  o << "module naja_dffre #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  input E,\n";
  o << "  input R,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C or posedge R) begin\n";
  o << "    if (R) Q <= {WIDTH{1'b0}};\n";
  o << "    else if (E) Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffre\n";
}

void SNLVRLDumper::dumpNajaDFFSEModel(std::ostream& o) {
  o << "module naja_dffse #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  o << "  input E,\n";
  o << "  input S,\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C or posedge S) begin\n";
  o << "    if (S) Q <= {WIDTH{1'b1}};\n";
  o << "    else if (E) Q <= D;\n";
  o << "  end\n";
  o << "endmodule //naja_dffse\n";
}

namespace {
void dumpNajaSyncDFFModel(
  std::ostream& o,
  const char* moduleName,
  const char* controlName,
  bool controlActiveLow,
  bool controlSets,
  bool enableControl) {
  o << "module " << moduleName << " #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter INIT = {WIDTH{1'bx}}\n";
  o << ") (\n";
  o << "  input C,\n";
  o << "  input [WIDTH-1:0] D,\n";
  if (enableControl) {
    o << "  input E,\n";
  }
  o << "  input " << controlName << ",\n";
  o << "  output reg [WIDTH-1:0] Q\n";
  o << ");\n";
  o << "  initial Q = INIT;\n";
  o << "  always @(posedge C) begin\n";
  o << "    if (" << (controlActiveLow ? "!" : "") << controlName << ") Q <= {WIDTH{1'b"
    << (controlSets ? "1" : "0") << "}};\n";
  if (enableControl) {
    o << "    else if (E) Q <= D;\n";
  } else {
    o << "    else Q <= D;\n";
  }
  o << "  end\n";
  o << "endmodule //" << moduleName << "\n";
}
} // namespace

void SNLVRLDumper::dumpNajaDFFSRModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffsr", "R", false, false, false);
}

void SNLVRLDumper::dumpNajaDFFSRNModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffsrn", "RN", true, false, false);
}

void SNLVRLDumper::dumpNajaDFFSSModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffss", "S", false, true, false);
}

void SNLVRLDumper::dumpNajaDFFSSNModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffssn", "SN", true, true, false);
}

void SNLVRLDumper::dumpNajaDFFSREModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffsre", "R", false, false, true);
}

void SNLVRLDumper::dumpNajaDFFSRNEModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffsrne", "RN", true, false, true);
}

void SNLVRLDumper::dumpNajaDFFSSEModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffsse", "S", false, true, true);
}

void SNLVRLDumper::dumpNajaDFFSSNEModel(std::ostream& o) {
  dumpNajaSyncDFFModel(o, "naja_dffssne", "SN", true, true, true);
}

void SNLVRLDumper::dumpNajaDivModModel(std::ostream& o) {
  o << "module naja_divmod #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter SIGNED = 0\n";
  o << ") (\n";
  o << "  input [WIDTH-1:0] A,\n";
  o << "  input [WIDTH-1:0] B,\n";
  o << "  output [WIDTH-1:0] Q,\n";
  o << "  output [WIDTH-1:0] R\n";
  o << ");\n";
  o << "  generate if (SIGNED) begin : gen_signed\n";
  o << "    assign Q = $signed(A) / $signed(B);\n";
  o << "    assign R = $signed(A) % $signed(B);\n";
  o << "  end else begin : gen_unsigned\n";
  o << "    assign Q = A / B;\n";
  o << "    assign R = A % B;\n";
  o << "  end endgenerate\n";
  o << "endmodule //naja_divmod\n";
}

void SNLVRLDumper::dumpNajaMemModel(std::ostream& o) {
  o << "module naja_mem #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter DEPTH = 1,\n";
  o << "  parameter ABITS = 1,\n";
  o << "  parameter RD_PORTS = 1,\n";
  o << "  parameter WR_PORTS = 1,\n";
  o << "  parameter MULTI_CLOCK = 0,\n";
  o << "  parameter RST_ENABLE = 0,\n";
  o << "  parameter RST_ASYNC = 0,\n";
  o << "  parameter RST_ACTIVE_LOW = 0,\n";
  o << "  parameter INIT_ENABLE = 0,\n";
  // Keep the disabled default scalar so large uninitialized memories do not
  // force simulators to elaborate a WIDTH*DEPTH packed constant. A sized
  // explicit override retains its own width when initialization is enabled.
  o << "  parameter INIT = 1'b0\n";
  o << ") (\n";
  o << "  input CLK,\n";
  o << "  input RST,\n";
  o << "  input [RD_PORTS*ABITS-1:0] RADDR,\n";
  o << "  output reg [RD_PORTS*WIDTH-1:0] RDATA,\n";
  o << "  input [(WR_PORTS > 0 ? WR_PORTS : 1)*ABITS-1:0] WADDR,\n";
  o << "  input [(WR_PORTS > 0 ? WR_PORTS : 1)*WIDTH-1:0] WDATA,\n";
  o << "  input [(WR_PORTS > 0 ? WR_PORTS : 1)-1:0] WE,\n";
  o << "  input [(WR_PORTS > 0 ? WR_PORTS : 1)-1:0] WCLK,\n";
  o << "  input [(WR_PORTS > 0 ? WR_PORTS : 1)*WIDTH-1:0] WMASK\n";
  o << ");\n\n";
  o << "  reg [WIDTH-1:0] mem [0:DEPTH-1];\n";
  o << "  integer i;\n";
  o << "  integer rp;\n";
  o << "  integer addr_index;\n";
  o << "  reg [ABITS-1:0] addr_value;\n\n";
  o << "  task automatic load_init;\n";
  o << "    integer init_idx;\n";
  o << "    begin\n";
  o << "      /* verilator lint_off SELRANGE */\n";
  o << "      for (init_idx = 0; init_idx < DEPTH; init_idx = init_idx + 1)\n";
  o << "        if (INIT === '0) mem[init_idx] = WIDTH'(0);\n";
  o << "        else mem[init_idx] = INIT[init_idx*WIDTH +: WIDTH];\n";
  o << "      /* verilator lint_on SELRANGE */\n";
  o << "    end\n";
  o << "  endtask\n\n";
  o << "  task automatic write_ports;\n";
  o << "    integer wp;\n";
  o << "    integer later;\n";
  o << "    integer addr_index;\n";
  o << "    reg allow_write;\n";
  o << "    reg [ABITS-1:0] addr_value;\n";
  o << "    begin\n";
  o << "      for (wp = 0; wp < WR_PORTS; wp = wp + 1) begin\n";
  o << "        allow_write = WE[wp];\n";
  o << "        addr_value = WADDR[wp*ABITS +: ABITS];\n";
  o << "        for (later = wp + 1; later < WR_PORTS; later = later + 1) begin\n";
  o << "          if (WE[later] && WADDR[later*ABITS +: ABITS] == addr_value)\n";
  o << "            allow_write = 1'b0;\n";
  o << "        end\n";
  o << "        addr_index = integer'(addr_value);\n";
  o << "        if (allow_write && addr_index < DEPTH)\n";
  o << "          mem[addr_index] <= WDATA[wp*WIDTH +: WIDTH];\n";
  o << "      end\n";
  o << "    end\n";
  o << "  endtask\n\n";
  o << "  wire reset_active = RST_ENABLE && (RST_ACTIVE_LOW ? ~RST : RST);\n\n";
  o << "  initial begin\n";
  o << "    if (INIT_ENABLE)\n";
  o << "      load_init();\n";
  o << "  end\n\n";
  o << "  always @* begin\n";
  o << "    for (rp = 0; rp < RD_PORTS; rp = rp + 1) begin\n";
  o << "      addr_value = RADDR[rp*ABITS +: ABITS];\n";
  o << "      addr_index = integer'(addr_value);\n";
  o << "      if (addr_index < DEPTH)\n";
  o << "        RDATA[rp*WIDTH +: WIDTH] = mem[addr_index];\n";
  o << "      else\n";
  o << "        RDATA[rp*WIDTH +: WIDTH] = WR_PORTS == 0 ? WIDTH'(1'sbx) : WIDTH'(0);\n";
  o << "    end\n";
  o << "  end\n\n";
  o << "  generate\n";
  o << "    if (WR_PORTS == 0) begin : read_only\n";
  o << "    end else if (MULTI_CLOCK) begin : independent_write_clocks\n";
  o << "      genvar port, bit_index;\n";
  o << "      for (port = 0; port < WR_PORTS; port = port + 1) begin : write_port\n";
  o << "        for (bit_index = 0; bit_index < WIDTH; bit_index = bit_index + 1) begin : write_bit\n";
  o << "          always @(posedge WCLK[port]) begin\n";
  o << "            if (WE[port] && WMASK[port*WIDTH+bit_index] &&\n";
  o << "                WADDR[port*ABITS +: ABITS] < DEPTH)\n";
  o << "              mem[WADDR[port*ABITS +: ABITS]][bit_index] <= WDATA[port*WIDTH+bit_index];\n";
  o << "          end\n";
  o << "        end\n";
  o << "      end\n";
  o << "    end else if (RST_ENABLE && RST_ASYNC && RST_ACTIVE_LOW) begin : async_low_reset\n";
  o << "      always @(posedge CLK or negedge RST) begin\n";
  o << "        if (!RST)\n";
  o << "          load_init();\n";
  o << "        else\n";
  o << "          write_ports();\n";
  o << "      end\n";
  o << "    end else if (RST_ENABLE && RST_ASYNC) begin : async_high_reset\n";
  o << "      always @(posedge CLK or posedge RST) begin\n";
  o << "        if (RST)\n";
  o << "          load_init();\n";
  o << "        else\n";
  o << "          write_ports();\n";
  o << "      end\n";
  o << "    end else if (RST_ENABLE) begin : sync_reset\n";
  o << "      always @(posedge CLK) begin\n";
  o << "        if (reset_active)\n";
  o << "          load_init();\n";
  o << "        else\n";
  o << "          write_ports();\n";
  o << "      end\n";
  o << "    end else begin : no_reset\n";
  o << "      always @(posedge CLK) begin\n";
  o << "        write_ports();\n";
  o << "      end\n";
  o << "    end\n";
  o << "  endgenerate\n";
  o << "endmodule //naja_mem\n";
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (!publicDesign_) publicDesign_ = design;
  struct PublicDesignGuard {
    const SNLDesign*& design;
    ~PublicDesignGuard() { design = nullptr; }
  } publicDesignGuard {publicDesign_};
  std::string context("dumpDesign(stream): ");
  context += design->isUnnamed() ? "anonymous_design" : design->getName().getString();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpDesignStreamDuration,
    detailedPerfReport_.dumpDesignStreamCalls);
  emitNajaMemModel_ = false;
  emitNajaMux2Model_ = false;
  emitNajaTableSelectModel_ = false;
  emitNajaPrimitiveModels_ = false;
  if (configuration_.isDumpHierarchy()) {
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    bool first = true;
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      if (not design->isPrimitive()) {
        if (not first) {
          o << '\n';
        }
        first = false;
        dumpOneDesign(design, o);
      }
    }
  } else {
    dumpOneDesign(design, o);
  }
}

void SNLVRLDumper::dumpLibrary(const NLLibrary* library, std::ostream& o) {
  std::string context("dumpLibrary(stream): ");
  context += library->isUnnamed() ? "anonymous_library" : library->getName().getString();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpLibraryStreamDuration,
    detailedPerfReport_.dumpLibraryStreamCalls);
  emitNajaMemModel_ = false;
  emitNajaMux2Model_ = false;
  emitNajaTableSelectModel_ = false;
  emitNajaPrimitiveModels_ = false;
  for (auto design: library->getSNLDesigns()) {
    dumpOneDesign(design, o);
  }
}

std::string SNLVRLDumper::getTopFileName(const SNLDesign* top) const {
  if (configuration_.hasTopFileName()) {
    return configuration_.getTopFileName();
  }
  if (not top->isUnnamed()) {
    return top->getName().getString() + ".v";
  }
  return "top.v";
} 

std::string SNLVRLDumper::getPrimitiveFileName() const {
  if (configuration_.hasLibraryFileName()) {
    return configuration_.getLibraryFileName();
  }
  return "naja_primitives.v";
}

std::string SNLVRLDumper::getLibraryFileName(const NLLibrary* library) const {
  if (configuration_.hasLibraryFileName()) {
    return configuration_.getLibraryFileName();
  }
  if (not library->isUnnamed()) {
    return library->getName().getString() + ".v";
  }
  return "library.v";
} 

void SNLVRLDumper::dumpNajaPrimitiveFile(const std::filesystem::path& path) {
  if (!emitNajaPrimitiveModels_) {
    return;
  }
  std::filesystem::path filePath = path/getPrimitiveFileName();
  std::ofstream outFile;
  outFile.open(filePath);
  NajaUtils::createBanner(
    outFile,
    "Verilog file for naja primitives",
    "//"
  );
  outFile << '\n';
  dumpNajaFAModel(outFile);
  outFile << '\n';
  dumpNajaMux2Model(outFile);
  outFile << '\n';
  dumpNajaAddSubModels(outFile);
  outFile << '\n';
  dumpNajaMultiplyModel(outFile);
  outFile << '\n';
  dumpNajaShiftModel(outFile);
  outFile << '\n';
  dumpNajaVectorUpdateModel(outFile);
  dumpNajaVectorSelectModel(outFile);
  outFile << '\n';
  dumpNajaTableSelectModel(outFile);
  outFile << '\n';
  dumpNajaDFFModel(outFile);
  outFile << '\n';
  dumpNajaDLatchModel(outFile);
  outFile << '\n';
  dumpNajaDFFNModel(outFile);
  outFile << '\n';
  dumpNajaDFFRNModel(outFile);
  outFile << '\n';
  dumpNajaDFFRModel(outFile);
  outFile << '\n';
  dumpNajaDFFSModel(outFile);
  outFile << '\n';
  dumpNajaDFFEModel(outFile);
  outFile << '\n';
  dumpNajaDFFREModel(outFile);
  outFile << '\n';
  dumpNajaDFFSEModel(outFile);
  outFile << '\n';
  dumpNajaDFFSRModel(outFile);
  outFile << '\n';
  dumpNajaDFFSRNModel(outFile);
  outFile << '\n';
  dumpNajaDFFSSModel(outFile);
  outFile << '\n';
  dumpNajaDFFSSNModel(outFile);
  outFile << '\n';
  dumpNajaDFFSREModel(outFile);
  outFile << '\n';
  dumpNajaDFFSRNEModel(outFile);
  outFile << '\n';
  dumpNajaDFFSSEModel(outFile);
  outFile << '\n';
  dumpNajaDFFSSNEModel(outFile);
  outFile << '\n';
  dumpNajaDivModModel(outFile);
  outFile << '\n';
  dumpNajaMemModel(outFile);
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, const std::filesystem::path& path) {
  publicDesign_ = design;
  struct PublicDesignGuard {
    const SNLDesign*& design;
    ~PublicDesignGuard() { design = nullptr; }
  } publicDesignGuard {publicDesign_};
  std::string context("dumpDesign(path): ");
  context += design->isUnnamed() ? "anonymous_design" : design->getName().getString();
  context += " -> ";
  context += path.string();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpDesignPathDuration,
    detailedPerfReport_.dumpDesignPathCalls);
  NajaPerf::Scope scope("SNLVRLDumper::dumpDesign");
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not design->isUnnamed()) {
      reason << design->getName().getString();
    } else {
      reason << "anonymous design";
    }
    reason << " cannot be dumped: ";
    reason << path.string() << " " << " does not exist";
    throw SNLVRLDumperException(reason.str());
  }
  if (configuration_.isSingleFile()) {
    //create file
    std::filesystem::path filePath = path/getTopFileName(design);
    std::ofstream outFile;
    outFile.open(filePath);
    NajaUtils::createBanner(
      outFile,
      "Verilog file for " + design->getName().getString(),
      "//"
    );
    outFile << '\n';
    dumpDesign(design, outFile);
    if (emitNajaPrimitiveModels_) {
      dumpNajaPrimitiveFile(path);
    }
  } else {
    SNLVRLDumper streamDumper;
    SNLVRLDumper::Configuration configuration(configuration_);
    configuration.setDumpHierarchy(false);
    streamDumper.setConfiguration(configuration);
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    bool emitNajaMemModel = false;
    bool emitNajaMux2Model = false;
    bool emitNajaPrimitiveModels = false;
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      std::filesystem::path filePath = path/getTopFileName(design);
      std::ofstream outFile;
      outFile.open(filePath);
      NajaUtils::createBanner(
        outFile,
        "Verilog file for " + design->getName().getString(),
        "//"
      );
      outFile << '\n';
      streamDumper.publicDesign_ = publicDesign_;
      streamDumper.dumpDesign(design, outFile);
      emitNajaMemModel = emitNajaMemModel or streamDumper.emitNajaMemModel_;
      emitNajaMux2Model = emitNajaMux2Model or streamDumper.emitNajaMux2Model_;
      emitNajaPrimitiveModels =
        emitNajaPrimitiveModels or streamDumper.emitNajaPrimitiveModels_;
    }
    emitNajaMemModel_ = emitNajaMemModel;
    emitNajaMux2Model_ = emitNajaMux2Model;
    emitNajaPrimitiveModels_ = emitNajaPrimitiveModels;
    if (emitNajaPrimitiveModels) {
      dumpNajaPrimitiveFile(path);
    }
  }
}

void SNLVRLDumper::dumpLibrary(const NLLibrary* library, const std::filesystem::path& path) {
  std::string context("dumpLibrary(path): ");
  context += library->isUnnamed() ? "anonymous_library" : library->getName().getString();
  context += " -> ";
  context += path.string();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpLibraryPathDuration,
    detailedPerfReport_.dumpLibraryPathCalls);
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not library->isUnnamed()) {
      reason << library->getName().getString();
    } else {
      reason << library->getDescription();
    }
    reason << " cannot be dumped: path ";
    reason << path.string() << " does not exist";
    throw SNLVRLDumperException(reason.str());
  }
  if (configuration_.isSingleFile()) {
    //create file
    std::filesystem::path filePath = path/getLibraryFileName(library);
    std::ofstream outFile;
    outFile.open(filePath);
    NajaUtils::createBanner(
      outFile,
      "Verilog file for " + library->getName().getString(),
      "//"
    );
    outFile << '\n';
    dumpLibrary(library, outFile);
    if (emitNajaPrimitiveModels_) {
      dumpNajaPrimitiveFile(path);
    }
  } else {
    for (auto design: library->getSNLDesigns()) {
      dumpDesign(design, path);
    }
  }
}

std::string SNLVRLDumper::binStrToHexStr(std::string binStr) {
  size_t missingZeros = (4-binStr.size()%4)%4;
  for (size_t i=0; i<missingZeros; i++) {
    binStr = '0' + binStr;
  }
  std::string hexStr;
  size_t it = 0;
  while (it < binStr.size()) {
    std::string hex = binStr.substr(it, 4);
    if      (hex == "0000") { hexStr += "0"; }
    else if (hex == "0001") { hexStr += "1"; }
    else if (hex == "0010") { hexStr += "2"; }
    else if (hex == "0011") { hexStr += "3"; }
    else if (hex == "0100") { hexStr += "4"; }
    else if (hex == "0101") { hexStr += "5"; }
    else if (hex == "0110") { hexStr += "6"; }
    else if (hex == "0111") { hexStr += "7"; }
    else if (hex == "1000") { hexStr += "8"; }
    else if (hex == "1001") { hexStr += "9"; }
    else if (hex == "1010") { hexStr += "A"; }
    else if (hex == "1011") { hexStr += "B"; }
    else if (hex == "1100") { hexStr += "C"; }
    else if (hex == "1101") { hexStr += "D"; }
    else if (hex == "1110") { hexStr += "E"; }
    else if (hex == "1111") { hexStr += "F"; }
    else { 
      std::ostringstream reason;
      reason << "Error in binary to hexadecimal conversion: ";
      reason << hex << " is not a convertible binary.";
      throw naja::NL::SNLVRLDumperException(reason.str());
    }
    it += 4;
  }
  return hexStr;
}

} // namespace naja::NL
