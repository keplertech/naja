// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
#include "ConstantPropagation.h"

#include <iostream>
#include <limits>
#include <ranges>
#include <set>
#include <stack>
#include <vector>

#include "NajaPerf.h"

#include "SNLDesignModeling.h"
#include "NLLibraryTruthTables.h"
#include "SNLScalarNet.h"
#include "SNLTruthTable.h"
#include "NLDB0.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLUniquifier.h"

#include "Reduction.h"
using namespace naja::DNL;
using namespace naja::NAJA_OPT;
using namespace naja::NL;
using namespace naja::BNE;

namespace {

constexpr NLID::DesignObjectID InvalidPathToDrive =
    std::numeric_limits<NLID::DesignObjectID>::max();
constexpr size_t InvalidFlatTermToDrive = std::numeric_limits<size_t>::max();

}

// #define DEBUG_PRINTS

void ConstantPropagation::initializeTypesID() {
  for (DNLID leaf : dnl_->getLeaves()) {
    DNLInstanceFull instance = dnl_->getDNLInstanceFromID(leaf);
    std::string name = instance.getSNLModel()->getName().getString();
    if (name.find("NAND") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> NAND\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::NAND;
    } else if (name.find("XNOR") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> XNOR\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::XNOR;
    } else if (name.find("NOR") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> NOR\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::NOR;
    } else if (name.find("XOR") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> XOR\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::XOR;
    } else if (name.find("AND") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> AND\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::AND;
    } else if (name.find("OR") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> OR\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::OR;
    } else if (name.find("INV") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> INV\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::INV;
    } else if (name.find("BUF") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> BUF\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::BUF;
    } else if (name.find("HA") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> HA\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::HA;
    } else if (name.find("DFF") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> DFF\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::DFF;
    } else if (name.find("MUX") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> MUX\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::MUX;
    } else if (name.find("OAI") != std::string::npos) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> OAI\n", name.c_str());
// LCOV_EXCL_STOP
#endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::OAI;
    }
  }
}

void ConstantPropagation::collectConstants() {
  auto logic0 = NLLibraryTruthTables::getDesignForTruthTable(
                    *(dnl_->getTop()
                          .getSNLModel()
                          ->getDB()
                          ->getPrimitiveLibraries()
                          .begin()),
                    SNLTruthTable::Logic0())
                    .first;
  auto logic1 = NLLibraryTruthTables::getDesignForTruthTable(
                    *(dnl_->getTop()
                          .getSNLModel()
                          ->getDB()
                          ->getPrimitiveLibraries()
                          .begin()),
                    SNLTruthTable::Logic1())
                    .first;
  for (DNLID leaf : dnl_->getLeaves()) {
    DNLInstanceFull instance = dnl_->getDNLInstanceFromID(leaf);
    if (instance.getSNLModel() == logic0) {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() ==
            SNLBitTerm::Direction::Output) {
          initialConstants0_.insert(term.getIsoID());
          constants0_.insert(term.getIsoID());
        }
      }
    } else if (instance.getSNLModel() == logic1) {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() ==
            SNLBitTerm::Direction::Output) {
          initialConstants1_.insert(term.getIsoID());
          constants1_.insert(term.getIsoID());
        }
      }
    }
  }
  initialConstants0_.insert(dnl_->getDNLIsoDB().getConstant0Isos().begin(),
                            dnl_->getDNLIsoDB().getConstant0Isos().end());
  initialConstants1_.insert(dnl_->getDNLIsoDB().getConstant1Isos().begin(),
                            dnl_->getDNLIsoDB().getConstant1Isos().end());
  constants0_.insert(dnl_->getDNLIsoDB().getConstant0Isos().begin(),
                     dnl_->getDNLIsoDB().getConstant0Isos().end());
  constants1_.insert(dnl_->getDNLIsoDB().getConstant1Isos().begin(),
                     dnl_->getDNLIsoDB().getConstant1Isos().end());
}

unsigned ConstantPropagation::computeOutputValue(DNLID instanceID) {
  DNLInstanceFull instance = dnl_->getDNLInstanceFromID(instanceID);
  const auto* model = instance.getSNLInstance()->getModel();
  if (NLDB0::isVectorSelect(model)) {
    if (NLDB0::getVectorSelectSignature(model).selectWidth != 1) return (unsigned)-1;
    std::vector<bool> inputs;
    for (DNLID id = instance.getTermIndexes().first; id <= instance.getTermIndexes().second; ++id) {
      const auto& term = dnl_->getDNLTerminalFromID(id);
      if (term.getSnlBitTerm()->getDirection() != SNLTerm::Direction::Input) continue;
      if (constants0_.contains(term.getIsoID())) inputs.push_back(false);
      else if (constants1_.contains(term.getIsoID())) inputs.push_back(true);
      else return (unsigned)-1;
    }
    const auto value = NLDB0::getVectorSelectTruthTable(model,
        NLDB0::getVectorSelectOutput(model)->getBit(0)->getOrderID()).evaluateVectorSelect(inputs);
    return value ? unsigned(*value) : (unsigned)-1;
  }
  if (NLDB0::isVectorUpdate(model)) {
    if (NLDB0::getVectorUpdateSignature(model).width != 1) return (unsigned)-1;
    std::vector<bool> inputs;
    for (DNLID id = instance.getTermIndexes().first; id <= instance.getTermIndexes().second; ++id) {
      const auto& term = dnl_->getDNLTerminalFromID(id);
      if (term.getSnlBitTerm()->getDirection() != SNLTerm::Direction::Input) continue;
      if (constants0_.contains(term.getIsoID())) inputs.push_back(false);
      else if (constants1_.contains(term.getIsoID())) inputs.push_back(true);
      else return (unsigned)-1;
    }
    const auto table = NLDB0::getVectorUpdateTruthTable(model,
        NLDB0::getVectorUpdateOutput(model)->getBit(0)->getOrderID());
    return table.evaluateVectorUpdate(inputs);
  }
  const SNLTruthTable& truthTable =
      SNLDesignModeling::getTruthTable(instance.getSNLInstance());
  if (not truthTable.isInitialized()) {
    // LCOV_EXCL_START
    return (unsigned)-1;
    // LCOV_EXCL_STOP
  }
  std::vector<std::pair<NLID::DesignObjectID, int>> constTerms;
  for (DNLID termId = instance.getTermIndexes().first;
       termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
       termId++) {
    const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
    if (term.getSnlBitTerm()->getDirection() != SNLBitTerm::Direction::Input) {
      continue;
    }
    if (constants0_.find(term.getIsoID()) != constants0_.end()) {
      constTerms.push_back({term.getSnlTerm()->getBitTerm()->getID(), 0});
    } else if (constants1_.find(term.getIsoID()) != constants1_.end()) {
      constTerms.push_back({term.getSnlTerm()->getBitTerm()->getID(), 1});
    }
  }
  // Canonical vector updates return through the dedicated branch above;
  // stored non-DB0 truth tables do not preserve vector-update metadata.
  if (truthTable.getGenericType() == SNLTruthTable::GenericType::VECTOR_UPDATE &&
      constTerms.size() != truthTable.size()) return (unsigned)-1; // LCOV_EXCL_LINE
  SNLTruthTable reducedTruthTable = ReductionOptimization::reduceTruthTable(
      instance.getSNLInstance(), truthTable, constTerms);
  if (reducedTruthTable.all0()) {
    return 0;
  } else if (reducedTruthTable.all1()) {
    return 1;
  }
  return (unsigned)-1;
}

void ConstantPropagation::performConstantPropagationAnalysis() {
  std::set<DNLID> constants;
  constants.insert(initialConstants0_.begin(), initialConstants0_.end());
  constants.insert(initialConstants1_.begin(), initialConstants1_.end());
#ifdef DEBUG_PRINTS
  //  LCOV_EXCL_START
  printf("Constant Propagation : Number of constants before: %lu\n",
         constants.size());
  size_t loop = 0;
// LCOV_EXCL_STOP
#endif
  while (!constants.empty()) {
#ifdef DEBUG_PRINTS
    //  LCOV_EXCL_START
    printf("loop: %lu\n", loop);
    loop++;
    // LCOV_EXCL_STOP
#endif
    std::set<DNLID> constantsNew;
    for (DNLID constant : constants) {
      DNLIso iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(constant);
      for (DNLID readerTerm : iso.getReaders()) {
        DNLTerminalFull reader = dnl_->getDNLTerminalFromID(readerTerm);
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        if (!reader.getDNLInstance().isTop()) {
          printf("%lu reader: %s\n", constant,
                 reader.getDNLInstance()
                     .getSNLInstance()
                     ->getName()
                     .getString()
                     .c_str());
        } else {
          printf("%lu reader: %s\n", constant,
                 dnl_->getTopDesign()->getName().getString().c_str());
        }
// LCOV_EXCL_STOP
#endif
        if (reader.getDNLInstance().isTop()) {
          continue;
        }
        bool isConst = true;
        std::vector<DNLTerminalFull> output;
        DNLTerminalFull q;
        for (DNLID termId = reader.getDNLInstance().getTermIndexes().first;
             termId <= reader.getDNLInstance().getTermIndexes().second;
             termId++) {
          const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
          if (term.getSnlBitTerm()->getDirection() ==
              SNLBitTerm::Direction::Output) {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("output: %s\n",
                   term.getSnlBitTerm()->getName().getString().c_str());
// LCOV_EXCL_STOP
#endif
            output.push_back(term);
            if (term.getSnlBitTerm()->getName().getString() ==
                std::string("Q")) {
              q = term;
            }
            continue;
          }
          if (constants0_.find(term.getIsoID()) == constants0_.end() &&
              constants1_.find(term.getIsoID()) == constants1_.end()) {
            isConst = false;
          }
        }
        if (output.size() == 1 || !q.isNull()) {
          if (isConst) {
            partialConstantInstances_.erase(reader.getDNLInstance().getID());
            // Analyze the contants in ouptus and propagate them
            unsigned newConst = (unsigned)-1;
            if (truthTableEngine_ && q.isNull()) {
              newConst = computeOutputValue(reader.getDNLInstance().getID());
            } else {
              newConst = computeOutputValueForConstantInstance(
                  reader.getDNLInstance().getID());
            }
            if (newConst == 1) {
              constantsNew.insert(output[0].getIsoID());
              constants1_.insert(output[0].getIsoID());
              partialConstantInstances_.erase(reader.getDNLInstance().getID());
            } else if (newConst == 0) {
              constantsNew.insert(output[0].getIsoID());
              constants0_.insert(output[0].getIsoID());
              partialConstantInstances_.erase(reader.getDNLInstance().getID());
            }
          } else {
            unsigned newConst = (unsigned)-1;
            if (truthTableEngine_ && q.isNull()) {
              newConst = computeOutputValue(reader.getDNLInstance().getID());
            } else {
              newConst = computeOutputValueForPartiallyConstantInstance(
                  reader.getDNLInstance().getID());
            }
            if (newConst == 1) {
#ifdef DEBUG_PRINTS
              // LCOV_EXCL_START
              printf("%lu\n", output[0].getIsoID());
// LCOV_EXCL_STOP
#endif
              DNLID iso = DNLID_MAX;
              if (q.isNull()) {
                iso = output[0].getIsoID();
              } else {
                iso = q.getIsoID();
              }
              constantsNew.insert(iso);
              constants1_.insert(iso);
              partialConstantInstances_.erase(reader.getDNLInstance().getID());
            } else if (newConst == 0) {
              DNLID iso = DNLID_MAX;
              if (q.isNull()) {
                iso = output[0].getIsoID();
              } else {
                iso = q.getIsoID();
              }
              constantsNew.insert(iso);
              constants0_.insert(iso);
              partialConstantInstances_.erase(reader.getDNLInstance().getID());

            } else {
              if (!reader.getDNLInstance().isTop()) {
                partialConstantInstances_.insert(
                    reader.getDNLInstance().getID());
              }
            }
          }
        }
      }
    }
    constants = constantsNew;
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("Constant Propagation : Number of constants after: %lu\n",
         constants0_.size() + constants1_.size());
// LCOV_EXCL_STOP
#endif
}

unsigned ConstantPropagation::computeOutputValueForConstantInstance(
    DNLID instanceID) {
  DNLInstanceFull instance = dnl_->getDNLInstanceFromID(instanceID);
  if (NLDB0::isVectorSelect(instance.getSNLInstance()->getModel())) return computeOutputValue(instanceID);
  if (NLDB0::isVectorUpdate(instance.getSNLInstance()->getModel()))
    return computeOutputValue(instanceID);
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("f instance: %s\n",
         instance.getSNLInstance()->getName().getString().c_str());
// LCOV_EXCL_STOP
#endif
  switch (
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()]) {
    case Type::AND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f AND 0\n");
// LCOV_EXCL_STOP
#endif
          return 0;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f AND 1\n");
// LCOV_EXCL_STOP
#endif
      return 1;
    }
    case Type::OR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f OR 1\n");
// LCOV_EXCL_STOP
#endif
          return 1;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f OR 0\n");
// LCOV_EXCL_STOP
#endif
      return 0;
    }
    case Type::XOR: {
      unsigned count = 0;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          count++;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f XOR %d\n", count % 2);
// LCOV_EXCL_STOP
#endif
      return count % 2;
    }
    case Type::NAND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f NAND 1\n");
// LCOV_EXCL_STOP
#endif
          return 1;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f NAND 0\n");
// LCOV_EXCL_STOP
#endif
      return 0;
    }
    case Type::NOR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f NOR 0\n");
// LCOV_EXCL_STOP
#endif
          return 0;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f NOR 1\n");
// LCOV_EXCL_STOP
#endif
      return 1;
    }
    case Type::XNOR: {
      unsigned count = 0;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          count++;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f XNOR %d\n", (count % 2) == 0);
// LCOV_EXCL_STOP
#endif
      return (count % 2) == 0;
    }
    case Type::INV: {
      DNLID iso = DNLID_MAX;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        iso = term.getIsoID();
      }
      if (constants1_.find(iso) != constants1_.end()) {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f INV 0\n");
// LCOV_EXCL_STOP
#endif
        return 0;
      } else {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f INV 1\n");
// LCOV_EXCL_STOP
#endif
        return 1;
      }
    }
    case Type::BUF: {
      DNLID termId = instance.getTermIndexes().first;
      const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
      if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f BUF 1\n");
// LCOV_EXCL_STOP
#endif
        return 1;
      } else {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f BUF 0\n");
// LCOV_EXCL_STOP
#endif
        return 0;
      }
    }
    case Type::DFF: {
      unsigned d = (unsigned)-1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getName().getString() == std::string("D")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("f DFF 1\n");
// LCOV_EXCL_STOP
#endif
            d = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("f DFF 0\n");
// LCOV_EXCL_STOP
#endif
            d = 0;
          }
          break;
        }
      }
      return d;
    }
    case Type::MUX: {
      unsigned s = (unsigned)-1;
      unsigned d = (unsigned)-1;
      unsigned q = (unsigned)-1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getName().getString() == std::string("S")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            s = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            s = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("A")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            d = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            d = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("B")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            q = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            q = 0;
          }
        }
      }
      if (s == 0) {
        return d;
      } else {
        return q;
      }
    }
    case Type::OAI: {
      unsigned result = (unsigned)-1;
      unsigned a = (unsigned)-1;
      unsigned b1 = (unsigned)-1;
      unsigned b2 = (unsigned)-1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getName().getString() == std::string("A")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            a = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            a = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("B1")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            b1 = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            b1 = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("B2")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            b2 = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            b2 = 0;
          }
        }
      }
      if (a == 1 && (b1 == 1 || b2 == 1)) {
        result = 0;
      } else if (a == 0 || (b1 == 0 && b2 == 0)) {
        result = 1;
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p OAI %d\n", result);
// LCOV_EXCL_STOP
#endif
      return result;
    }
    default:
      break;
  }
  return (unsigned)-1;
}

unsigned ConstantPropagation::computeOutputValueForPartiallyConstantInstance(
    DNLID instanceID) {
  DNLInstanceFull instance = dnl_->getDNLInstanceFromID(instanceID);
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("p instance: %s\n",
         instance.getSNLInstance()->getName().getString().c_str());
// LCOV_EXCL_STOP
#endif
  switch (
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()]) {
    case Type::AND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p AND 0\n");
// LCOV_EXCL_STOP
#endif
          return 0;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p AND nan\n");
// LCOV_EXCL_STOP
#endif
      return (unsigned)-1;
    }
    case Type::OR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p OR 1\n");
// LCOV_EXCL_STOP
#endif
          return 1;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p OR nan\n");
// LCOV_EXCL_STOP
#endif
      return (unsigned)-1;
    }
    case Type::NAND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p NAND 1\n");
// LCOV_EXCL_STOP
#endif
          return 1;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p NAND nan\n");
// LCOV_EXCL_STOP
#endif
      return (unsigned)-1;
    }
    case Type::NOR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p NOR 0\n");
// LCOV_EXCL_STOP
#endif
          return 0;
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p NOR nan\n");
// LCOV_EXCL_STOP
#endif
      return (unsigned)-1;
    }
    case Type::DFF: {
      unsigned d = (unsigned)-1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getName().getString() == std::string("D")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("p DFF 1\n");
// LCOV_EXCL_STOP
#endif
            d = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("p DFF 0\n");
// LCOV_EXCL_STOP
#endif
            d = 0;
          }
          break;
        }
      }
      return d;
    }
    case Type::MUX: {
      unsigned s = (unsigned)-1;
      unsigned d = (unsigned)-1;
      unsigned q = (unsigned)-1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getName().getString() == std::string("S")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            s = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            s = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("A")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            d = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            d = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("B")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            q = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            q = 0;
          }
        }
      }
      if (s == 0) {
        return d;
      } else if (s == 1) {
        return q;
      }
      return (unsigned)-1;
    }
    case Type::OAI: {
      unsigned result = (unsigned)-1;
      unsigned a = (unsigned)-1;
      unsigned b1 = (unsigned)-1;
      unsigned b2 = (unsigned)-1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId != DNLID_MAX and termId <= instance.getTermIndexes().second;
           termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() !=
            SNLBitTerm::Direction::Input) {
          continue;
        }
        if (term.getSnlBitTerm()->getName().getString() == std::string("A")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            a = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            a = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("B1")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            b1 = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            b1 = 0;
          }
        } else if (term.getSnlBitTerm()->getName().getString() ==
                   std::string("B2")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            b2 = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            b2 = 0;
          }
        }
      }
      if (a == 1 && (b1 == 1 || b2 == 1)) {
        result = 0;
      } else if (a == 0 || (b1 == 0 && b2 == 0)) {
        result = 1;
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p OAI %d\n", result);
// LCOV_EXCL_STOP
#endif
      return result;
    }
    default:
      break;
  }
  return (unsigned)-1;
}

void ConstantPropagation::changeDriverToLocal0(SNLInstTerm* term, DNLID id) {
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja_") +
                   term->getDesign()->getName().getString());
  auto netName = NLName(name + "_net");
  SNLNet* assign0 = term->getDesign()->getNet(netName);
  if (nullptr == assign0) {
    assign0 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign0->setType(naja::NL::SNLNet::Type::Supply0);
  term->setNet(assign0);
  SNLTruthTable tt(0, 0, SNLTruthTable::fullDependencies(0));
  // find primitives library
  if (term->getDB()->getPrimitiveLibraries().size() != 1) {
    // LCOV_EXCL_START
    throw NLException("There should be only one primitive library");
    // LCOV_EXCL_STOP
  }
  auto primitives = *term->getDB()->getPrimitiveLibraries().begin();
  auto logic0 =
      NLLibraryTruthTables::getDesignForTruthTable(primitives, tt).first;

  SNLInstance* logic0Inst = term->getDesign()->getInstance(NLName(name));
  if (nullptr == logic0Inst) {
    if (logic0 == nullptr) {
      // LCOV_EXCL_START
      throw NLException("No logic0 design found");
      // LCOV_EXCL_STOP
    }
    logic0Inst = SNLInstance::create(term->getDesign(), logic0, NLName(name));
  }
  (*logic0Inst->getInstTerms().begin())->setNet(assign0);
}

void ConstantPropagation::changeDriverToLocal1(SNLInstTerm* term, DNLID id) {
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja_") +
                   term->getDesign()->getName().getString());
  auto netName = NLName(name + "_net");
  SNLNet* assign1 = term->getDesign()->getNet(netName);
  if (nullptr == assign1) {
    assign1 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign1->setType(naja::NL::SNLNet::Type::Supply1);
  term->setNet(assign1);
  SNLTruthTable tt(0, 1, SNLTruthTable::fullDependencies(0));

  // find primitives library
  if (term->getDB()->getPrimitiveLibraries().size() != 1) {
    // LCOV_EXCL_START
    throw NLException("There should be only one primitive library");
    // LCOV_EXCL_STOP
  }
  auto primitives = *term->getDB()->getPrimitiveLibraries().begin();
  auto logic1 =
      NLLibraryTruthTables::getDesignForTruthTable(primitives, tt).first;
  SNLInstance* logic1Inst = term->getDesign()->getInstance(NLName(name));
  if (nullptr == logic1Inst) {
    if (logic1 == nullptr) {
      // LCOV_EXCL_START
      throw NLException("No logic1 design found");
      // LCOV_EXCL_STOP
    }
    logic1Inst = SNLInstance::create(term->getDesign(), logic1, NLName(name));
  }
  (*logic1Inst->getInstTerms().begin())->setNet(assign1);
}

void ConstantPropagation::propagateConstants() {
  for (DNLID iso : constants0_) {
    if (initialConstants0_.find(iso) != initialConstants0_.end()) {
      continue;
    }
    DNLIso const0iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(iso);
    if (const0iso.getDrivers().size() > 1) {
      continue;
    }
    for (DNLID reader : const0iso.getReaders()) {
      DNLTerminalFull readerTerm = dnl_->getDNLTerminalFromID(reader);
      DNLInstanceFull readerInst = readerTerm.getDNLInstance();
      if (readerInst.isTop()) {
        constant0TopReaders_.push_back(readerTerm.getSnlBitTerm());
        continue;
      }
      std::vector<NLID::DesignObjectID> path;
      DNLInstanceFull currentInstance = readerInst;
      while (currentInstance.isTop() == false) {
        path.push_back(currentInstance.getSNLInstance()->getID());
        currentInstance = currentInstance.getParentInstance();
      }
      std::reverse(path.begin(), path.end());
      constant0Readers_.push_back(std::tuple<std::vector<NLID::DesignObjectID>,
                                             size_t, DNLID>(
          path, readerTerm.getSnlTerm()->getBitTerm()->getFlatID(),
          readerInst.getID()));
    }
  }
  for (DNLID iso : constants1_) {
    if (initialConstants1_.find(iso) != initialConstants1_.end()) {
      continue;
    }
    DNLIso const1iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(iso);
    if (const1iso.getDrivers().size() > 1) {
      continue;
    }
    for (DNLID reader : const1iso.getReaders()) {
      DNLTerminalFull readerTerm = dnl_->getDNLTerminalFromID(reader);
      DNLInstanceFull readerInst = readerTerm.getDNLInstance();
      if (readerInst.isTop()) {
        constant1TopReaders_.push_back(readerTerm.getSnlBitTerm());
        continue;
      }
      std::vector<NLID::DesignObjectID> path;
      DNLInstanceFull currentInstance = readerInst;
      while (currentInstance.isTop() == false) {
        path.push_back(currentInstance.getSNLInstance()->getID());
        currentInstance = currentInstance.getParentInstance();
      }
      std::reverse(path.begin(), path.end());
      constant1Readers_.push_back(std::tuple<std::vector<NLID::DesignObjectID>,
                                             size_t, DNLID>(
          path, readerTerm.getSnlTerm()->getBitTerm()->getFlatID(),
          readerInst.getID()));
    }
  }
  for (DNLID instId : partialConstantInstances_) {
    DNLInstanceFull inst = dnl_->getDNLInstanceFromID(instId);
    std::vector<NLID::DesignObjectID> path;
    DNLInstanceFull currentInstance = inst;
    while (currentInstance.isTop() == false) {
      path.push_back(currentInstance.getSNLInstance()->getID());
      currentInstance = currentInstance.getParentInstance();
    }
    std::reverse(path.begin(), path.end());
    std::vector<std::pair<NLID::DesignObjectID, int>> instTerms;
    // size_t numInputs = 0;
    for (DNLID termId = inst.getTermIndexes().first;
         termId <= inst.getTermIndexes().second; termId++) {
      const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
      if (term.getSnlBitTerm()->getDirection() ==
          SNLBitTerm::Direction::Input) {
        // numInputs++;
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
          instTerms.push_back(std::pair<NLID::DesignObjectID, int>(
              term.getSnlTerm()->getBitTerm()->getID(), 0));
        } else if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          instTerms.push_back(std::pair<NLID::DesignObjectID, int>(
              term.getSnlTerm()->getBitTerm()->getID(), 1));
        }
      }
    }
    // assert(numInputs > instTerms.size());
    partialConstantReaders_.push_back(
        std::tuple<std::vector<NLID::DesignObjectID>,
                   std::vector<std::pair<NLID::DesignObjectID, int>>, DNLID>(
            path, instTerms, inst.getID()));
  }
  if (!normalizedUniquification_) {
    for (auto& path : constant0Readers_) {
      SNLUniquifier uniquifier(std::get<0>(path), std::get<2>(path));
      uniquifier.process();
      SNLInstTerm* constTerm =
          uniquifier.getPathUniq().back()->getInstTermByFlatID(std::get<1>(path));
      changeDriverToLocal0(constTerm, std::get<2>(path));
    }
    for (SNLBitTerm* term : constant0TopReaders_) {
      term->setNet(nullptr);
      std::string name(std::string("logic0_naja_") +
                       term->getDesign()->getName().getString());
      auto netName = NLName(name + "_net");
      SNLNet* assign0 = term->getDesign()->getNet(netName);
      if (nullptr == assign0) {
        assign0 = SNLScalarNet::create(term->getDesign(), netName);
      }
      assign0->setType(naja::NL::SNLNet::Type::Supply0);
      term->setNet(assign0);
      SNLTruthTable tt(0, 0, SNLTruthTable::fullDependencies(0));
      auto logic0 = NLLibraryTruthTables::getDesignForTruthTable(
                        *(term->getDB()->getPrimitiveLibraries().begin()), tt)
                        .first;
      SNLInstance* logic0Inst = term->getDesign()->getInstance(NLName(name));
      if (nullptr == logic0Inst) {
        logic0Inst =
            SNLInstance::create(term->getDesign(), logic0, NLName(name));
      }
      (*logic0Inst->getInstTerms().begin())->setNet(assign0);
    }
    for (auto& path : constant1Readers_) {
      SNLUniquifier uniquifier(std::get<0>(path), std::get<2>(path));
      uniquifier.process();
      SNLInstTerm* constTerm =
          uniquifier.getPathUniq().back()->getInstTermByFlatID(std::get<1>(path));
      changeDriverToLocal1(constTerm, std::get<2>(path));
    }
    for (SNLBitTerm* term : constant1TopReaders_) {
      term->setNet(nullptr);
      std::string name(std::string("logic1_naja_") +
                       term->getDesign()->getName().getString());
      auto netName = NLName(name + "_net");
      SNLNet* assign1 = term->getDesign()->getNet(netName);
      if (nullptr == assign1) {
        assign1 = SNLScalarNet::create(term->getDesign(), netName);
      }
      assign1->setType(naja::NL::SNLNet::Type::Supply1);
      term->setNet(assign1);
      SNLTruthTable tt(0, 1, SNLTruthTable::fullDependencies(0));
      auto logic1 = NLLibraryTruthTables::getDesignForTruthTable(
                        *(term->getDB()->getPrimitiveLibraries().begin()), tt)
                        .first;
      SNLInstance* logic1Inst = term->getDesign()->getInstance(NLName(name));
      if (nullptr == logic1Inst) {
        logic1Inst =
            SNLInstance::create(term->getDesign(), logic1, NLName(name));
      }
      (*logic1Inst->getInstTerms().begin())->setNet(assign1);
    }
  } else {
    BNE::BNE bne;
    for (auto& path : constant0Readers_) {
      auto context = std::get<0>(path);
      context.pop_back();
      bne.addDriveWithConstantAction(context, std::get<0>(path).back(),
                                     std::get<1>(path), 0);
    }
    for (SNLBitTerm* term : constant0TopReaders_) {
      bne.addDriveWithConstantAction(std::vector<NLID::DesignObjectID>(),
                                     InvalidPathToDrive,
                                     InvalidFlatTermToDrive,
                                     0, term);
    }
    for (auto& path : constant1Readers_) {
      auto context = std::get<0>(path);
      context.pop_back();
      bne.addDriveWithConstantAction(context, std::get<0>(path).back(),
                                     std::get<1>(path), 1);
    }
    for (SNLBitTerm* term : constant1TopReaders_) {
      bne.addDriveWithConstantAction(std::vector<NLID::DesignObjectID>(),
                                     InvalidPathToDrive,
                                     InvalidFlatTermToDrive,
                                     1, term);
    }
    bne.process();
  }
}

void ConstantPropagation::run() {
  NajaPerf::Scope scope("ConstantPropagation");
  destroy();
  dnl_ = get();
  initializeTypesID();
  collectConstants();
  performConstantPropagationAnalysis();
  propagateConstants();
  destroy();
}
