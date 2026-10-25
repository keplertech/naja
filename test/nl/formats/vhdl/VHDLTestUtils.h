// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLDB0.h"
#include "SNLBitNet.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLInstParameter.h"
#include "SNLScalarTerm.h"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace naja::NL::test {
struct DFFBit {
  SNLBitNet* clock;
  SNLBitNet* data;
  SNLBitNet* output;
};

inline std::vector<DFFBit> dffBits(SNLDesign* design) {
  std::vector<DFFBit> bits;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (!NLDB0::isDFF(model)) continue;
    auto* clock = instance->getInstTerm(model->getScalarTerm(NLName("C")))->getNet();
    for (auto* term : instance->getInstTerms()) {
      if (term->getDirection() != SNLTerm::Direction::Output) continue;
      auto* output = dynamic_cast<SNLBusTermBit*>(term->getBitTerm());
      SNLBitTerm* data = output ? static_cast<SNLBitTerm*>(model->getBusTerm(NLName("D"))->getBit(output->getBit()))
          : model->getScalarTerm(NLName("D"));
      bits.push_back({clock, instance->getInstTerm(data)->getNet(), term->getNet()});
    }
  }
  return bits;
}

using MemoryState = std::unordered_map<SNLInstance*, std::vector<uint64_t>>;

inline MemoryState zeroMemoryState(SNLDesign* design) {
  MemoryState state;
  for (auto* instance : design->getInstances()) if (NLDB0::isMemory(instance->getModel()) &&
      NLDB0::getMemorySignature(instance).writePorts) {
    const auto signature = NLDB0::getMemorySignature(instance);
    if (signature.width > 64) throw std::runtime_error("test memory word exceeds 64 bits");
    state[instance].resize(signature.depth, 0);
  }
  return state;
}

// Evaluate the actual canonical primitive connectivity, with flop outputs
// supplied as the current state. This also catches undriven data and cycles.
inline bool evaluateRTL(SNLBitNet* net, std::unordered_map<SNLBitNet*, bool>& values,
                 std::unordered_set<SNLBitNet*>& visiting, const MemoryState* memories = nullptr) {
  if (net->isConstant0()) return false;
  if (net->isConstant1()) return true;
  if (const auto found = values.find(net); found != values.end()) return found->second;
  if (!visiting.insert(net).second) throw std::runtime_error("combinational cycle");
  SNLInstance* driver = nullptr;
  SNLBitTerm* output = nullptr;
  for (auto* term : net->getInstTerms())
    if (term->getDirection() == SNLTerm::Direction::Output) {
      if (driver) throw std::runtime_error("multiple drivers");
      driver = term->getInstance();
      output = term->getBitTerm();
    }
  if (!driver || NLDB0::isDFF(driver->getModel())) throw std::runtime_error("missing data/state");
  auto* model = driver->getModel();
  const auto read = [&](SNLBitTerm* term) {
    return evaluateRTL(driver->getInstTerm(term)->getNet(), values, visiting, memories);
  };
  bool value;
  if (NLDB0::isAssign(model)) {
    value = read(NLDB0::getAssignInput());
  } else if (NLDB0::isVectorSelect(model)) {
    std::vector<bool> inputs;
    for (auto* port : {NLDB0::getVectorSelectData(model), NLDB0::getVectorSelectOffset(model)})
      for (auto* bit : port->getBits()) inputs.push_back(read(bit));
    const auto result = NLDB0::getVectorSelectTruthTable(model,
        output->getOrderID()).evaluateVectorSelect(inputs);
    if (!result) throw std::runtime_error("Out-of-range vector select in binary evaluator");
    value = *result;
  } else if (NLDB0::isVectorUpdate(model)) {
    const auto outputBit = output->getBit();
    std::vector<bool> inputs{read(NLDB0::getVectorUpdateBase(model)->getBit(outputBit))};
    for (auto* port : {NLDB0::getVectorUpdateData(model), NLDB0::getVectorUpdateOffset(model)})
      for (auto* bit : port->getBits()) inputs.push_back(read(bit));
    inputs.push_back(read(NLDB0::getVectorUpdateEnable(model)));
    value = NLDB0::getVectorUpdateTruthTable(model,
        output->getOrderID()).evaluateVectorUpdate(inputs);
  } else if (NLDB0::isFA(model)) {
    const auto a = read(NLDB0::getFAInputA());
    const auto b = read(NLDB0::getFAInputB());
    const auto carry = read(NLDB0::getFAInputCI());
    value = output == NLDB0::getFAOutputS() ? (a ^ b ^ carry)
        : ((a && b) || (a && carry) || (b && carry));
  } else if (NLDB0::isAddSub(model) || NLDB0::isMultiply(model)) {
    const bool multiply = NLDB0::isMultiply(model);
    const auto width = multiply ? NLDB0::getMultiplyWidth(driver) : NLDB0::getAddSubWidth(driver);
    if (width > 64) throw std::runtime_error("test arithmetic word exceeds 64 bits");
    const auto readWord = [&](SNLBusTerm* term) {
      uint64_t word = 0;
      for (size_t bit = 0; bit < width; ++bit)
        if (read(term->getBit(bit))) word |= uint64_t(1) << bit;
      return word;
    };
    const auto a = readWord(multiply ? NLDB0::getMultiplyInputA(model) : NLDB0::getAddSubInputA(model));
    const auto b = readWord(multiply ? NLDB0::getMultiplyInputB(model) : NLDB0::getAddSubInputB(model));
    const auto result = multiply ? a * b : NLDB0::isSubtract(model) ? a - b : a + b;
    value = (result >> static_cast<SNLBusTermBit*>(output)->getBit()) & 1;
  } else if (NLDB0::isShift(model)) {
    const auto signature = NLDB0::getShiftSignature(driver);
    size_t count = 0;
    // Saturate instead of truncating high bits or overflowing the host count.
    for (size_t i = signature.countWidth; i; --i)
      count = count >= signature.width ? signature.width :
          std::min(signature.width, count * 2 + read(NLDB0::getShiftCount(model)->getBit(i-1)));
    const auto bit = size_t(static_cast<SNLBusTermBit*>(output)->getBit());
    const bool valid = signature.right ? count < signature.width-bit : count <= bit;
    if (valid) value = read(NLDB0::getShiftData(model)->getBit(signature.right ? bit+count : bit-count));
    else value = signature.arithmetic && read(NLDB0::getShiftData(model)->getBit(signature.width-1));
  } else if (NLDB0::isTableSelect(model)) {
    const auto signature = NLDB0::getTableSelectSignature(driver);
    size_t address = 0;
    for (size_t i = signature.abits; i; --i)
      address = address * 2 + read(NLDB0::getTableSelectAddress(model)->getBit(i - 1));
    if (address >= signature.depth) throw std::runtime_error("unknown table select result");
    const auto bit = static_cast<SNLBusTermBit*>(output)->getBit();
    value = read(NLDB0::getTableSelectData(model)->getBit(address * signature.width + bit));
  } else if (NLDB0::isMemory(model)) {
    const auto signature = NLDB0::getMemorySignature(driver);
    const auto bit = static_cast<SNLBusTermBit*>(output)->getBit();
    const auto port = bit / signature.width;
    size_t address = 0;
    for (size_t i = 0; i < signature.abits; ++i)
      if (read(NLDB0::getMemoryReadAddress(model)->getBit(port * signature.abits + i)))
        address |= size_t(1) << i;
    if (!signature.writePorts) {
      if (address >= signature.depth) throw std::runtime_error("unknown ROM read result");
      value = false;
      if (address < signature.depth) {
        const auto init = driver->getInstParameter(NLName("INIT"))->getValue();
        const auto offset = address * signature.width + bit % signature.width;
        const auto base = init.find('\'') + 1;
        if (init[base] == 'h') {
          const auto digit = init.at(init.size() - 1 - offset/4);
          const auto number = digit <= '9' ? digit-'0' : digit-'a'+10;
          value = (number >> (offset%4)) & 1;
        } else {
          const auto digit = init.at(init.size() - 1 - offset);
          if (digit != '0' && digit != '1') throw std::runtime_error("nonbinary ROM output");
          value = digit == '1';
        }
      }
    } else {
      if (!memories) throw std::runtime_error("missing memory state");
      value = address < signature.depth &&
          ((memories->at(driver).at(address) >> (bit % signature.width)) & 1);
    }
  } else if (NLDB0::isMux2(model)) {
    const auto bit = static_cast<SNLBusTermBit*>(output)->getBit();
    value = read(NLDB0::getMux2Select(model))
        ? read(NLDB0::getMux2InputB(model)->getBit(bit))
        : read(NLDB0::getMux2InputA(model)->getBit(bit));
  } else {
    if (!NLDB0::isGate(model)) throw std::runtime_error("unexpected primitive");
    SNLTruthTable::ConstantInputs inputs;
    for (auto* term : driver->getInstTerms())
      if (term->getDirection() == SNLTerm::Direction::Input)
        inputs.emplace_back(inputs.size(), evaluateRTL(term->getNet(), values, visiting, memories));
    const auto table = NLDB0::getPrimitiveTruthTable(model);
    const auto dependencies = SNLTruthTable::fullDependencies(inputs.size());
    const auto normalized = table.getGenericType() == SNLTruthTable::GenericType::NONE
        ? SNLTruthTable(inputs.size(), static_cast<uint64_t>(table.bits()), dependencies)
        : SNLTruthTable(inputs.size(), table.getGenericType(), dependencies);
    value = normalized.getReducedWithConstants(inputs).all1();
  }
  visiting.erase(net);
  values[net] = value;
  return value;
}

// Sample all memory inputs using pre-edge state. Call before replacing DFF
// state to preserve VHDL signal scheduling and read-before-write collisions.
inline MemoryState nextMemoryState(const MemoryState& current,
                                   std::unordered_map<SNLBitNet*, bool>& values) {
  auto next = current;
  std::unordered_set<SNLBitNet*> visiting;
  for (const auto& [instance, words] : current) {
    auto* model = instance->getModel();
    const auto signature = NLDB0::getMemorySignature(instance);
    const auto read = [&](SNLBusTerm* term, size_t offset, size_t width) {
      uint64_t value = 0;
      for (size_t i = 0; i < width; ++i)
        if (evaluateRTL(instance->getInstTerm(term->getBit(offset+i))->getNet(), values, visiting, &current))
          value |= uint64_t(1) << i;
      return value;
    };
    for (size_t port = 0; port < signature.writePorts; ++port) {
      if (!read(NLDB0::getMemoryWriteEnable(model), signature.writePorts - 1 - port, 1)) continue;
      const auto address = read(NLDB0::getMemoryWriteAddress(model), port * signature.abits, signature.abits);
      if (address < signature.depth) {
        const auto data = read(NLDB0::getMemoryWriteData(model), port * signature.width, signature.width);
        if (signature.independentWriteClocks) {
          const auto mask = read(model->getBusTerm(NLName("WMASK")), port * signature.width, signature.width);
          next.at(instance)[address] = (next.at(instance)[address] & ~mask) | (data & mask);
        } else next.at(instance)[address] = data;
      }
    }
  }
  return next;
}

inline std::vector<unsigned> firInputs(unsigned lanes) {
  std::vector<unsigned> inputs;
  const auto mask = (1u << lanes) - 1;
  for (unsigned cycle = 0; cycle < 128; ++cycle)
    inputs.push_back(cycle < 4 ? 0 : cycle < 4 + lanes ? 1u << (cycle - 4) :
        ((cycle * 1777u + 91u) ^ (cycle % 5 ? 0 : mask)) & mask);
  return inputs;
}

// Simulate the actual leaf flop/gate connectivity, then propagate each leaf's
// output through the top-level adder network. Each instance owns its state,
// even when several instances share a specialized model.
inline std::vector<unsigned> simulateFIR(SNLDesign* top, const std::vector<unsigned>& inputs) {
  using Values = std::unordered_map<SNLBitNet*, bool>;
  std::unordered_map<SNLInstance*, Values> states;
  auto* input = top->getBusTerm(NLName("regx_in"));
  auto* output = top->getBusTerm(NLName("y_out"));
  if (!input || !output) throw std::runtime_error("missing FIR ports");
  for (auto* instance : top->getInstances()) {
    if (NLDB0::isAssign(instance->getModel()) ||
        NLDB0::isGate(instance->getModel()) || NLDB0::isFA(instance->getModel()) ||
        NLDB0::isMux2(instance->getModel())) continue;
    auto& state = states[instance];
    for (auto* flop : instance->getModel()->getInstances()) if (NLDB0::isDFF(flop->getModel()))
      state[flop->getInstTerm(NLDB0::getDFFOutput())->getNet()] = false;
    if (state.size() != 3) throw std::runtime_error("expected three registers per FIR leaf");
    auto* clock = instance->getModel()->getScalarTerm(NLName("ck"));
    if (!clock || instance->getInstTerm(clock)->getNet() != top->getScalarTerm(NLName("ck"))->getNet())
      throw std::runtime_error("incorrect FIR clock binding");
  }
  if (states.size() != input->getWidth()) throw std::runtime_error("incorrect generated FIR instance count");
  std::vector<unsigned> outputs;
  for (const auto pattern : inputs) {
    Values topValues;
    for (unsigned bit = 0; bit < input->getWidth(); ++bit)
      topValues[input->getBit(bit)->getNet()] = (pattern >> bit) & 1;
    for (auto& [instance, state] : states) {
      auto* model = instance->getModel();
      auto* data = model->getScalarTerm(NLName("reg_in"));
      if (!data) throw std::runtime_error("missing FIR leaf input");
      auto values = state;
      values[data->getNet()] = topValues.at(instance->getInstTerm(data)->getNet());
      std::unordered_set<SNLBitNet*> visiting;
      for (auto* flop : model->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
        if (flop->getInstTerm(NLDB0::getDFFClock())->getNet() != model->getScalarTerm(NLName("ck"))->getNet())
          throw std::runtime_error("incorrect leaf flop clock");
        state[flop->getInstTerm(NLDB0::getDFFOutput())->getNet()] =
            evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting);
      }
      values = state;
      values[data->getNet()] = topValues.at(instance->getInstTerm(data)->getNet());
      auto* result = model->getBusTerm(NLName("y_out"));
      if (!result) throw std::runtime_error("missing FIR leaf output");
      for (auto* bit : result->getBits())
        topValues[instance->getInstTerm(bit)->getNet()] = evaluateRTL(bit->getNet(), values, visiting);
    }
    std::unordered_set<SNLBitNet*> visiting;
    unsigned result = 0;
    for (unsigned bit = 0; bit < output->getWidth(); ++bit)
      result |= unsigned(evaluateRTL(output->getBit(bit)->getNet(), topValues, visiting)) << bit;
    outputs.push_back(result);
  }
  return outputs;
}
} // namespace naja::NL::test
