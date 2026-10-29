// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLUtils.h"

#include <algorithm>
#include <list>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include "NLLibrary.h"
#include "NLDB0.h"

#include "SNLDesignModeling.h"
#include "SNLDesign.h"
#include "SNLBitNet.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLRTLInfos.h"

namespace naja::NL {

size_t SNLUtils::removeUnconsumedMuxes(SNLDesign* design) {
  if (!design) {
    return 0;
  }
  std::vector<SNLInstance*> muxes;
  std::unordered_set<SNLInstance*> candidates;
  for (auto* instance : design->getInstances()) {
    if (NLDB0::isMux2(instance->getModel())) {
      muxes.push_back(instance);
      candidates.insert(instance);
    }
  }
  std::unordered_map<SNLBitNet*, std::vector<SNLInstance*>> producers;
  for (auto* instance : muxes) {
    for (auto* term : instance->getInstTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Output && term->getNet()) {
        producers[term->getNet()].push_back(instance);
      }
    }
  }
  std::unordered_set<SNLInstance*> live;
  std::vector<SNLInstance*> pending;
  const auto markLive = [&](SNLInstance* instance) {
    if (live.insert(instance).second) {
      pending.push_back(instance);
    }
  };
  for (auto* instance : muxes) {
    for (auto* term : instance->getInstTerms()) {
      auto* net = term->getNet();
      if (term->getDirection() != SNLTerm::Direction::Output || !net) {
        continue;
      }
      // Keep all design terminal connections, including externally driven
      // inputs and inouts. Every non-mux consumer is a conservative live root.
      if (!net->getBitTerms().empty()) {
        markLive(instance);
      }
      for (auto* consumer : net->getInstTerms()) {
        if (consumer->getDirection() != SNLTerm::Direction::Output &&
            !candidates.contains(consumer->getInstance())) {
          markLive(instance);
        }
      }
    }
  }
  while (!pending.empty()) {
    auto* instance = pending.back();
    pending.pop_back();
    for (auto* input : instance->getInstTerms()) {
      auto* net = input->getNet();
      if (input->getDirection() == SNLTerm::Direction::Output || !net) {
        continue;
      }
      // Look up producers directly: scanning every connection on shared
      // constants or high-fanout inputs here would make cleanup quadratic.
      if (const auto found = producers.find(net); found != producers.end()) {
        for (auto* producer : found->second) {
          markLive(producer);
        }
      }
    }
  }
  size_t removed = 0;
  // Membership uses hashes, but deletion follows stable instance ID order.
  for (auto* instance : muxes) {
    if (!live.contains(instance)) {
      instance->destroy();
      ++removed;
    }
  }
  return removed;
}

size_t SNLUtils::mergeRepeatedMuxSelections(SNLDesign* design) {
  if (!design) {
    return 0;
  }
  std::vector<SNLInstance*> muxes;
  for (auto* instance : design->getInstances()) {
    if (NLDB0::isMux2(instance->getModel())) {
      muxes.push_back(instance);
    }
  }
  std::unordered_map<SNLBitNet*, SNLInstance*> producers;
  for (auto* instance : muxes) {
    auto* output = NLDB0::getMux2Output(instance->getModel());
    for (auto* bit : output->getBits()) {
      if (auto* net = instance->getInstTerm(bit)->getNet()) {
        const auto [found, inserted] = producers.emplace(net, instance);
        if (!inserted) found->second = nullptr;
      }
    }
  }
  std::unordered_set<SNLInstance*> removed;
  for (auto* outer : muxes) {
    if (removed.contains(outer)) {
      continue;
    }
    auto* model = outer->getModel();
    auto* a = NLDB0::getMux2InputA(model);
    auto* b = NLDB0::getMux2InputB(model);
    auto* y = NLDB0::getMux2Output(model);
    auto* select = outer->getInstTerm(NLDB0::getMux2Select(model));
    while (select->getNet()) {
      bool merged = false;
      for (bool trueBranch : {false, true}) {
        auto* dataPort = trueBranch ? b : a;
        auto* commonPort = trueBranch ? a : b;
        auto* first = outer->getInstTerm(dataPort->getBit(0))->getNet();
        const auto found = producers.find(first);
        auto* inner = found == producers.end() ? nullptr : found->second;
        if (!inner || inner == outer || removed.contains(inner) || inner->getModel() != model) {
          continue;
        }
        auto* innerSelect = inner->getInstTerm(NLDB0::getMux2Select(model))->getNet();
        if (!innerSelect) {
          continue;
        }
        std::unordered_set<SNLBitNet*> outputs;
        for (auto* bit : y->getBits()) {
          if (auto* net = outer->getInstTerm(bit)->getNet()) outputs.insert(net);
        }
        // Do not transform malformed signatures, extra drivers, permutations,
        // shared outputs, select uses of the inner result, or direct feedback.
        bool compatible = !outputs.contains(innerSelect) && !outputs.contains(select->getNet());
        std::vector<SNLBitNet*> inputs;
        for (NLID::Bit bit = 0; compatible && bit < a->getWidth(); ++bit) {
          auto* outerInput = outer->getInstTerm(dataPort->getBit(bit));
          auto* innerOutput = inner->getInstTerm(y->getBit(bit));
          auto* net = innerOutput->getNet();
          auto* data = inner->getInstTerm(dataPort->getBit(bit))->getNet();
          auto* common = outer->getInstTerm(commonPort->getBit(bit))->getNet();
          if (!net || !data || !common || outputs.contains(data) || outputs.contains(common) ||
              net != outerInput->getNet() ||
              common != inner->getInstTerm(commonPort->getBit(bit))->getNet() ||
              !net->getBitTerms().empty()) {
            compatible = false;
            break;
          }
          for (auto* connection : net->getInstTerms()) {
            if (connection != innerOutput && connection != outerInput) {
              compatible = false;
              break;
            }
          }
          inputs.push_back(data);
        }
        if (!compatible) {
          continue;
        }
        auto* outerInfo = outer->getRTLInfos();
        auto* innerInfo = inner->getRTLInfos();
        const auto annotate = [&](SNLDesignObject* object) {
          if (!outerInfo && !innerInfo) return;
          auto* info = object->getRTLInfos();
          if (!info) info = SNLRTLInfos::create(object);
          for (const auto* source : {outerInfo, innerInfo}) {
            if (!source) continue;
            if (!info->hasSourceLoc() && source->hasSourceLoc())
              info->setSourceLoc(*source->getSourceLoc());
            if (source == info) continue;
            for (const auto& origin : source->getSourceOrigins()) info->addSourceOrigin(origin);
          }
        };
        auto* combinedSelect = select->getNet();
        if (combinedSelect != innerSelect) {
          combinedSelect = SNLScalarNet::create(design);
          auto* gate = NLDB0::getOrCreateNInputGate(trueBranch ? NLDB0::GateType::And : NLDB0::GateType::Or, 2);
          auto* instance = SNLInstance::create(design, gate);
          instance->setTermNet(NLDB0::getGateNTerms(gate)->getBit(0), innerSelect);
          instance->setTermNet(NLDB0::getGateNTerms(gate)->getBit(1), select->getNet());
          instance->setTermNet(NLDB0::getGateSingleTerm(gate), combinedSelect);
          annotate(instance);
          annotate(combinedSelect);
        }
        annotate(outer);
        for (NLID::Bit bit = 0; bit < a->getWidth(); ++bit) {
          outer->getInstTerm(dataPort->getBit(bit))->setNet(inputs[bit]);
        }
        select->setNet(combinedSelect);
        removed.insert(inner);
        inner->destroy();
        merged = true;
        break;
      }
      if (!merged) break;
    }
  }
  return removed.size();
}

namespace {

using VisitedDesigns = std::set<const SNLDesign*, SNLDesign::PointerLess>;

void prepareDesignForConcurrentAccess(
    const SNLDesign* design,
    VisitedDesigns& visitedDesigns) {
  if (design == nullptr or visitedDesigns.find(design) != visitedDesigns.end()) {
    return;
  }
  visitedDesigns.insert(design);

  if (design->isPrimitive()) {
    // These DB0 primitives do not expose combinational truth tables.
    if (NLDB0::isDivMod(design) or NLDB0::isMemory(design) or
        NLDB0::isShift(design) or NLDB0::isMultiply(design) or NLDB0::isAddSub(design)) {
      return;
    }
    for (auto term: design->getBitTerms()) {
      if (term->getDirection() != SNLTerm::Direction::Input) {
        static_cast<void>(
            SNLDesignModeling::getTruthTable(design, term->getOrderID()));
      }
    }
    return;
  }

  for (auto instance: design->getInstances()) {
    prepareDesignForConcurrentAccess(instance->getModel(), visitedDesigns);
  }
}

void collectReachableInstanceCount(
    const SNLDesign* design,
    VisitedDesigns& visitedDesigns,
    VisitedDesigns& activeDesigns,
    SNLUtils::InstanceCount& count) {
  if (design == nullptr or not activeDesigns.insert(design).second) {
    return;
  }
  const bool firstVisit = visitedDesigns.insert(design).second;
  if (firstVisit) {
    ++count.reachableModels;
  }
  for (auto instance: design->getInstances()) {
    ++count.totalInstances;
    if (instance->isLeaf()) {
      ++count.leafInstances;
    }
    if (firstVisit) {
      ++count.foldedTotalInstances;
      if (instance->isLeaf()) {
        ++count.foldedLeafInstances;
      }
    }
    collectReachableInstanceCount(instance->getModel(), visitedDesigns, activeDesigns, count);
  }
  activeDesigns.erase(design);
}

}  // namespace

unsigned SNLUtils::levelize(const SNLDesign* design, DesignsLevel& designsLevel) {
  if (design->getInstances().empty()) {
    designsLevel[design] = 0;
    return 0;
  } else {
    unsigned maxLevel = 0;
    for (auto instance: design->getInstances()) {
      unsigned level = 0;
      auto model = instance->getModel();
      auto it = designsLevel.find(model);
      if (it == designsLevel.end()) {
        level = levelize(model, designsLevel);
        designsLevel[model] = level;
      } else {
        level = it->second;
      }
      if (level > maxLevel) {
        maxLevel = level;
      }
    }
    designsLevel[design] = maxLevel+1;
    return maxLevel+1;
  }
}

void SNLUtils::getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns) {
  SNLUtils::DesignsLevel designsLevel;
  SNLUtils::levelize(top, designsLevel);
  sortedDesigns = SortedDesigns(designsLevel.begin(), designsLevel.end());
  std::sort(sortedDesigns.begin(), sortedDesigns.end(),
    [](const DesignLevel& ldl, const DesignLevel& rdl) {
      if (ldl.second == rdl.second) {
        return ldl.first->getNLID() < rdl.first->getNLID();
      }
      return ldl.second < rdl.second;
    }
  );
}

SNLUtils::InstanceCount SNLUtils::countReachableInstances(const SNLDesign* top) {
  InstanceCount count;
  VisitedDesigns visitedDesigns;
  VisitedDesigns activeDesigns;
  collectReachableInstanceCount(top, visitedDesigns, activeDesigns, count);
  return count;
}

NLID::Bit SNLUtils::getWidth(NLID::Bit msb, NLID::Bit lsb) {
  return std::abs(lsb - msb) + 1;
}

SNLDesign* SNLUtils::findTop(const NLLibrary* library) {
  using Tops = std::list<SNLDesign*>;
  Tops tops;
  for (auto design: library->getSNLDesigns()) {
    if (not design->isBlackBox()
      && not design->isPrimitive()
      && design->getSlaveInstances().empty()) {
        tops.push_back(design);
    }
  }
  //Special case: if no top was found accept blackboxes and empty modules
  if (tops.empty()) {
    for (auto design: library->getSNLDesigns()) {
      if (not design->isPrimitive()) {
        tops.push_back(design);
      }
    }
  }
  if (tops.size() == 1) {
    return tops.front();
  }
  return nullptr;
}

void SNLUtils::prepareForConcurrentAccess(const SNLDesign* design) {
  VisitedDesigns visitedDesigns;
  prepareDesignForConcurrentAccess(design, visitedDesigns);
}

std::string SNLUtils::getUnnamedIDString(const char* tag, std::uint64_t id) {
  return "<" + std::string(tag) + ":" + std::to_string(id) + ">";
}

std::string SNLUtils::getNamedString(const NLName& name, const char* tag, std::uint64_t id) {
  if (not name.empty()) {
    return name.getString();
  }
  return getUnnamedIDString(tag, id);
}

std::string SNLUtils::getBusNamedString(
    const NLName& name,
    const char* tag,
    std::uint64_t id,
    NLID::Bit msb,
    NLID::Bit lsb) {
  return getNamedString(name, tag, id) + "["
    + std::to_string(msb) + ":" + std::to_string(lsb) + "]";
}

std::string SNLUtils::getBitNamedString(
    const NLName& name,
    const char* tag,
    std::uint64_t id,
    NLID::Bit bit) {
  return getNamedString(name, tag, id) + "[" + std::to_string(bit) + "]";
}

}  // namespace naja::NL
