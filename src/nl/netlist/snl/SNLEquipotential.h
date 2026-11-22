// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include <memory>
#include <vector>
#include "SNLNet.h"
#include "SNLBitTerm.h"
#include "SNLOccurrence.h"

namespace naja::NL {

class SNLNetComponent;

class SNLEquipotential {
  public:
    enum class Mode {
      Standard,
      /// Cross Assign instances and omit their inst terms from the result.
      TraverseAssigns
    };

    struct Options {
      Mode mode = Mode::Standard;
      bool keepGraph = false;
    };

    struct Graph {
      struct Node {
        SNLOccurrence net;
        std::vector<SNLOccurrence> leaves;
        std::vector<SNLBitTerm*> terms;
      };
      enum class EdgeKind { Port, Assign };
      struct Edge {
        EdgeKind kind;
        // Port: inst term in the parent path. Assign: input inst term.
        SNLOccurrence crossing;
        // Port: outer -> inner. Assign: input -> output.
        size_t from;
        size_t to;
      };
      // Nodes and crossings are retained in discovery order.
      std::vector<Node> nodes;
      std::vector<Edge> edges;
    };

    using InstTermOccurrences = std::set<SNLOccurrence>;
    using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;

    SNLEquipotential()=default;
    SNLEquipotential(const SNLEquipotential&)=default;
    SNLEquipotential(SNLNetComponent* netComponent);
    SNLEquipotential(SNLNetComponent* netComponent, Mode mode);
    SNLEquipotential(const SNLOccurrence& netComponentOccurrence);
    SNLEquipotential(
      const SNLOccurrence& netComponentOccurrence,
      Mode mode);

    SNLEquipotential(SNLNetComponent* netComponent, const Options& options);
    SNLEquipotential(const SNLOccurrence& netComponentOccurrence, const Options& options);

    const Graph* getGraph() const { return graph_.get(); }
    void dumpDotFile(const std::string& path) const;
    const Terms& getTermsSet() const { return terms_; }
    const InstTermOccurrences& getInstTermOccurrencesSet() const { return instTermOccurrences_; }
    NajaCollection<SNLBitTerm*> getTerms() const;
    NajaCollection<SNLOccurrence> getInstTermOccurrences() const;
    SNLNet::Type getType() const { return type_; }
    bool isConst0() const;
    bool isConst1() const;
    bool isConstX() const;
    bool isConstZ() const;
    std::string getString() const;
    // Identity is leaves + top terms; derived graph data must not affect map keys.
    //Comparators
    bool operator==(const SNLEquipotential& other) const {
      return instTermOccurrences_ == other.instTermOccurrences_ and terms_ == other.terms_;
    }
    bool operator!=(const SNLEquipotential& other) const {
      return not operator==(other);
    }
    bool operator<(const SNLEquipotential& other) const {
      return instTermOccurrences_ < other.instTermOccurrences_ or
        (instTermOccurrences_ == other.instTermOccurrences_ and terms_ < other.terms_);
    }
    bool operator>(const SNLEquipotential& other) const {
      return instTermOccurrences_ > other.instTermOccurrences_ or
        (instTermOccurrences_ == other.instTermOccurrences_ and terms_ > other.terms_);
    }
    bool operator<=(const SNLEquipotential& other) const {
      return not operator>(other);
    }
    bool operator>=(const SNLEquipotential& other) const {
      return not operator<(other);
    }
  private:
    // Immutable after extraction, so copies can share the graph cheaply.
    std::shared_ptr<const Graph> graph_ {};
    InstTermOccurrences instTermOccurrences_  {};
    Terms               terms_                {};
    SNLNet::Type        type_                 {SNLNet::Type::Standard};
};

}  // namespace naja::NL
