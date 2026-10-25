// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "SNLRTLPrimitives.h"

#include <algorithm>
#include "NLDB0.h"
#include "NLException.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLScalarTerm.h"

namespace naja::NL {

namespace {

bool validNet(SNLDesign* design, SNLNet* net, size_t width) {
  return design && net && net->getDesign() == design &&
    static_cast<size_t>(net->getWidth()) == width;
}

void connectBits(
  SNLInstance* instance, SNLBusTerm* term, const SNLRTLPrimitives::Bits& bits) {
  SNLInstance::Terms terms;
  terms.reserve(bits.size());
  // Canonical mux terminals are [width-1:0].
  for (size_t bit = 0; bit < bits.size(); ++bit) {
    terms.push_back(term->getBit(static_cast<NLID::Bit>(bit)));
  }
  instance->setTermsNets(terms, bits);
}

NLDB0::GateType gateType(SNLRTLPrimitives::GateKind kind) {
  switch (kind) {
    case SNLRTLPrimitives::GateKind::And: return NLDB0::GateType::And;
    case SNLRTLPrimitives::GateKind::Nand: return NLDB0::GateType::Nand;
    case SNLRTLPrimitives::GateKind::Or: return NLDB0::GateType::Or;
    case SNLRTLPrimitives::GateKind::Nor: return NLDB0::GateType::Nor;
    case SNLRTLPrimitives::GateKind::Xor: return NLDB0::GateType::Xor;
    case SNLRTLPrimitives::GateKind::Xnor: return NLDB0::GateType::Xnor;
    case SNLRTLPrimitives::GateKind::Buf: return NLDB0::GateType::Buf;
    case SNLRTLPrimitives::GateKind::Not: return NLDB0::GateType::Not;
  }
  throw NLException("SNLRTLPrimitives::createGate: invalid gate kind");
}

SNLBitNet* bitAtHardwarePosition(SNLNet* net, size_t position) {
  if (auto* bit = dynamic_cast<SNLBitNet*>(net))
    return position == 0 ? bit : nullptr;
  auto* bus = dynamic_cast<SNLBusNet*>(net);
  if (!bus || position >= static_cast<size_t>(bus->getWidth()))
    return nullptr;  // LCOV_EXCL_LINE: caller validates matching net widths.
  return bus->getBitAtPosition(static_cast<size_t>(bus->getWidth()) - 1 - position);
}

}  // namespace

SNLInstance* SNLRTLPrimitives::createVectorSelect(
  SNLDesign* design, const Bits& data, const Bits& offset, bool isSigned, SNLNet* output) {
  const auto validBits = [design](const Bits& bits) {
    return !bits.empty() && std::all_of(bits.begin(), bits.end(),
        [design](auto* bit) { return validNet(design, bit, 1); });
  };
  if (!output || !validBits(data) || !validBits(offset) ||
      !validNet(design, output, output->getWidth()))
    throw NLException("SNLRTLPrimitives::createVectorSelect: invalid nets or widths");
  auto* model = NLDB0::getOrCreateVectorSelect(
      {data.size(), size_t(output->getWidth()), offset.size(), isSigned});
  auto* instance = SNLInstance::create(design, model);
  connectBits(instance, NLDB0::getVectorSelectData(model), data);
  connectBits(instance, NLDB0::getVectorSelectOffset(model), offset);
  instance->setTermNet(NLDB0::getVectorSelectOutput(model), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createVectorUpdate(
  SNLDesign* design, const Bits& base, const Bits& data, const Bits& offset,
  bool isSigned, SNLBitNet* enable, SNLNet* output) {
  const auto validBits = [design](const Bits& bits) {
    return !bits.empty() && std::all_of(bits.begin(), bits.end(),
        [design](auto* bit) { return validNet(design, bit, 1); });
  };
  if (!validBits(base) || !validBits(data) || !validBits(offset) ||
      !validNet(design, enable, 1) || !validNet(design, output, base.size()))
    throw NLException("SNLRTLPrimitives::createVectorUpdate: invalid nets or widths");
  auto* model = NLDB0::getOrCreateVectorUpdate({base.size(), data.size(), offset.size(), isSigned});
  auto* instance = SNLInstance::create(design, model);
  connectBits(instance, NLDB0::getVectorUpdateBase(model), base);
  connectBits(instance, NLDB0::getVectorUpdateData(model), data);
  connectBits(instance, NLDB0::getVectorUpdateOffset(model), offset);
  instance->setTermNet(NLDB0::getVectorUpdateEnable(model), enable);
  instance->setTermNet(NLDB0::getVectorUpdateOutput(model), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createROM(
  SNLDesign* design, const Bits& data, const Bits& address, SNLNet* output) {
  const auto width = output ? static_cast<size_t>(output->getWidth()) : 0;
  const auto validBits = [design](const Bits& bits) {
    return std::all_of(bits.begin(), bits.end(), [design](auto* bit) { return validNet(design, bit, 1); });
  };
  if (!width || !validNet(design, output, width) || data.empty() || address.empty() ||
      data.size() % width || !validBits(data) || !validBits(address) ||
      !std::all_of(data.begin(), data.end(), [](auto* bit) { return bit->isConstant(); }))
    throw NLException("SNLRTLPrimitives::createROM: invalid nets, contents or widths");
  std::string contents;
  contents.reserve(data.size());
  for (auto it = data.rbegin(); it != data.rend(); ++it)
    contents.push_back((*it)->isConstant0() ? '0' : (*it)->isConstant1() ? '1' : (*it)->isConstantX() ? 'x' : 'z');
  // Hex is compact for binary images; retain exact X/Z bits in binary images.
  const bool binary = contents.find_first_of("xz") == std::string::npos;
  if (binary) {
    contents.insert(0, (4 - contents.size() % 4) % 4, '0');
    std::string hex;
    for (size_t i = 0; i < contents.size(); i += 4) {
      unsigned digit = 0;
      for (size_t j = 0; j < 4; ++j) digit = 2 * digit + (contents[i+j] == '1');
      hex.push_back("0123456789abcdef"[digit]);
    }
    contents = std::move(hex);
  }
  auto* model = NLDB0::getOrCreateMemory({width, data.size()/width, address.size(), 1, 0});
  auto* instance = SNLInstance::create(design, model);
  for (const auto& name : {"WIDTH", "DEPTH", "ABITS", "RD_PORTS", "WR_PORTS", "RST_ENABLE", "RST_ASYNC", "RST_ACTIVE_LOW"}) {
    auto* parameter = model->getParameter(NLName(name));
    SNLInstParameter::create(instance, parameter, parameter->getValue());
  }
  SNLInstParameter::create(instance, model->getParameter(NLName("INIT_ENABLE")), "1");
  SNLInstParameter::create(instance, model->getParameter(NLName("INIT")),
      std::to_string(data.size()) + (binary ? "'h" : "'b") + contents);
  connectBits(instance, NLDB0::getMemoryReadAddress(model), address);
  instance->setTermNet(NLDB0::getMemoryReadData(model), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createMux(
  SNLDesign* design, SNLBitNet* select,
  const Bits& a, const Bits& b, SNLNet* output) {
  const auto validBits = [design](const Bits& bits) {
    return std::all_of(bits.begin(), bits.end(), [design](auto* bit) {
      return validNet(design, bit, 1);
    });
  };
  if (a.empty() || a.size() != b.size() ||
      !validNet(design, select, 1) || !validNet(design, output, a.size()) ||
      !validBits(a) || !validBits(b)) {
    throw NLException("SNLRTLPrimitives::createMux: invalid nets or widths");
  }
  auto* model = NLDB0::getOrCreateMux2(a.size());
  auto* instance = SNLInstance::create(design, model);
  connectBits(instance, NLDB0::getMux2InputA(model), a);
  connectBits(instance, NLDB0::getMux2InputB(model), b);
  instance->setTermNet(NLDB0::getMux2Select(model), select);
  instance->setTermNet(NLDB0::getMux2Output(model), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createDFF(
  SNLDesign* design, SNLNet* clock, SNLNet* data, SNLNet* output) {
  if (!validNet(design, clock, 1) || !validNet(design, data, 1) ||
      !validNet(design, output, 1)) {
    throw NLException("SNLRTLPrimitives::createDFF: invalid nets or widths");
  }
  auto* instance = SNLInstance::create(design, NLDB0::getDFF());
  instance->setTermNet(NLDB0::getDFFClock(), clock);
  instance->setTermNet(NLDB0::getDFFData(), data);
  instance->setTermNet(NLDB0::getDFFOutput(), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createDFFE(
  SNLDesign* design, SNLNet* clock, SNLNet* data,
  SNLNet* enable, SNLNet* output) {
  if (!validNet(design, clock, 1) || !validNet(design, data, 1) ||
      !validNet(design, enable, 1) || !validNet(design, output, 1)) {
    throw NLException("SNLRTLPrimitives::createDFFE: invalid nets or widths");
  }
  auto* instance = SNLInstance::create(design, NLDB0::getDFFE());
  instance->setTermNet(NLDB0::getDFFEClock(), clock);
  instance->setTermNet(NLDB0::getDFFEData(), data);
  instance->setTermNet(NLDB0::getDFFEEnable(), enable);
  instance->setTermNet(NLDB0::getDFFEOutput(), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createDFFSR(
  SNLDesign* design, SNLNet* clock, SNLNet* data,
  SNLNet* reset, SNLNet* output) {
  if (!validNet(design, clock, 1) || !validNet(design, data, 1) ||
      !validNet(design, reset, 1) || !validNet(design, output, 1)) {
    throw NLException("SNLRTLPrimitives::createDFFSR: invalid nets or widths");
  }
  auto* instance = SNLInstance::create(design, NLDB0::getDFFSR());
  instance->setTermNet(NLDB0::getDFFSRClock(), clock);
  instance->setTermNet(NLDB0::getDFFSRData(), data);
  instance->setTermNet(NLDB0::getDFFSRReset(), reset);
  instance->setTermNet(NLDB0::getDFFSROutput(), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createDFFSRE(
  SNLDesign* design, SNLNet* clock, SNLNet* data,
  SNLNet* enable, SNLNet* reset, SNLNet* output) {
  if (!validNet(design, clock, 1) || !validNet(design, data, 1) ||
      !validNet(design, enable, 1) || !validNet(design, reset, 1) ||
      !validNet(design, output, 1)) {
    throw NLException("SNLRTLPrimitives::createDFFSRE: invalid nets or widths");
  }
  auto* model = NLDB0::getDFFSRE();
  auto* instance = SNLInstance::create(design, model);
  instance->setTermNet(model->getScalarTerm(NLName("C")), clock);
  instance->setTermNet(model->getScalarTerm(NLName("D")), data);
  instance->setTermNet(model->getScalarTerm(NLName("E")), enable);
  instance->setTermNet(model->getScalarTerm(NLName("R")), reset);
  instance->setTermNet(model->getScalarTerm(NLName("Q")), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createGate(
  SNLDesign* design, GateKind kind,
  const std::vector<SNLNet*>& inputs, SNLNet* output) {
  if (!validNet(design, output, 1) || inputs.empty() ||
      ((kind == GateKind::Buf || kind == GateKind::Not) && inputs.size() != 1) ||
      std::any_of(inputs.begin(), inputs.end(), [design](auto* input) {
        return !validNet(design, input, 1);
      })) {
    throw NLException("SNLRTLPrimitives::createGate: invalid nets, widths, or fan-in");
  }
  const auto type = gateType(kind);
  const bool nOutput = kind == GateKind::Buf || kind == GateKind::Not;
  auto* model = nOutput
      ? NLDB0::getOrCreateNOutputGate(type, 1)
      : NLDB0::getOrCreateNInputGate(type, inputs.size());
  auto* instance = SNLInstance::create(design, model);
  if (nOutput) {
    instance->setTermNet(NLDB0::getGateSingleTerm(model), inputs.front());
    instance->setTermNet(NLDB0::getGateNTerms(model)->getBitAtPosition(0), output);
  } else {
    auto* inputTerms = NLDB0::getGateNTerms(model);
    for (size_t position = 0; position < inputs.size(); ++position)
      instance->setTermNet(inputTerms->getBitAtPosition(position), inputs[position]);
    instance->setTermNet(NLDB0::getGateSingleTerm(model), output);
  }
  return instance;
}

std::vector<SNLInstance*> SNLRTLPrimitives::createAssign(
  SNLDesign* design, SNLNet* input, SNLNet* output) {
  const auto width = output ? static_cast<size_t>(output->getWidth()) : 0;
  if (width == 0 || !validNet(design, input, width) ||
      !validNet(design, output, width)) {
    throw NLException("SNLRTLPrimitives::createAssign: invalid nets or widths");
  }
  std::vector<SNLInstance*> instances;
  instances.reserve(width);
  for (size_t position = 0; position < width; ++position) {
    auto* instance = SNLInstance::create(design, NLDB0::getAssign());
    instance->setTermNet(NLDB0::getAssignInput(), bitAtHardwarePosition(input, position));
    instance->setTermNet(NLDB0::getAssignOutput(), bitAtHardwarePosition(output, position));
    instances.push_back(instance);
  }
  return instances;
}

std::vector<SNLInstance*> SNLRTLPrimitives::createBitwiseGate(
  SNLDesign* design, GateKind kind,
  const std::vector<SNLNet*>& inputs, SNLNet* output) {
  const auto width = output ? static_cast<size_t>(output->getWidth()) : 0;
  if (!validNet(design, output, width) || width == 0 || inputs.empty() ||
      ((kind == GateKind::Buf || kind == GateKind::Not) && inputs.size() != 1) ||
      std::any_of(inputs.begin(), inputs.end(), [design, width](auto* input) {
        return !validNet(design, input, width);
      })) {
    throw NLException(
      "SNLRTLPrimitives::createBitwiseGate: invalid nets, widths, or fan-in");
  }
  gateType(kind);
  std::vector<SNLInstance*> instances;
  instances.reserve(width);
  for (size_t position = 0; position < width; ++position) {
    std::vector<SNLNet*> inputBits;
    inputBits.reserve(inputs.size());
    for (auto* input : inputs)
      inputBits.push_back(bitAtHardwarePosition(input, position));
    instances.push_back(createGate(
      design, kind, inputBits, bitAtHardwarePosition(output, position)));
  }
  return instances;
}

}  // namespace naja::NL
