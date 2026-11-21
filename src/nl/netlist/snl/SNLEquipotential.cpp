// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLEquipotential.h"

#include "SNLPath.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "NLDB0.h"
#include <map>
#include <fstream>
#include <sstream>  

namespace {

using namespace naja::NL;

// Allocated only when graph retention is requested. Lookup order never drives output.
struct EquipotentialGraphBuilder {
  using Graph = SNLEquipotential::Graph;
  std::shared_ptr<Graph> graph = std::make_shared<Graph>();
  std::map<SNLOccurrence, size_t> nodes;
  std::set<SNLOccurrence> crossings;
  std::vector<std::pair<SNLOccurrence, SNLOccurrence>> endpoints;

  void visit(const SNLOccurrence& net) {
    nodes.emplace(net, graph->nodes.size());
    graph->nodes.push_back({net, {}, {}});
  }

  void cross(Graph::EdgeKind kind, const SNLOccurrence& crossing,
      const SNLOccurrence& from, const SNLOccurrence& to) {
    if (not from.isValid() or not to.isValid() or not crossings.insert(crossing).second) {
      return;
    }
    graph->edges.push_back({kind, crossing, 0, 0});
    endpoints.emplace_back(from, to);
  }

  void finish(const SNLEquipotential::InstTermOccurrences& leaves,
      const SNLEquipotential::Terms& terms) {
    for (size_t i = 0; i < endpoints.size(); ++i) {
      graph->edges[i].from = nodes.at(endpoints[i].first);
      graph->edges[i].to = nodes.at(endpoints[i].second);
    }
    // Iterate the existing ordered sets, preserving their order and exact membership.
    for (const auto& leaf: leaves) {
      graph->nodes[nodes.at(leaf.getComponentBitNetOccurrence())].leaves.push_back(leaf);
    }
    for (auto term: terms) {
      auto net = SNLOccurrence(term).getComponentBitNetOccurrence();
      graph->nodes[nodes.at(net)].terms.push_back(term);
    }
  }
};

struct SNLEquipotentialExtractor {
  explicit SNLEquipotentialExtractor(
    SNLEquipotential::InstTermOccurrences& instTermOccurrences,
    SNLEquipotential::Terms& terms,
    SNLNet::Type& type,
    SNLEquipotential::Mode mode,
    EquipotentialGraphBuilder* graph
  ): instTermOccurrences_(instTermOccurrences),
    terms_(terms),
    type_(type),
    mode_(mode),
    graph_(graph)
  {}

  void extractNetComponentsFromNetOccurrence(
    const naja::NL::SNLOccurrence& netOccurrence,
    const SNLOccurrence& comingFromComponent,
    bool start
  ) {
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      //already visited... report loop ??
      return;
    }
    visitedNetOccurrences_.insert(netOccurrence);
    if (graph_) {
      graph_->visit(netOccurrence);
    }
    auto net = dynamic_cast<const SNLBitNet*>(netOccurrence.getObject());
    if (net->getType() != SNLNet::Type::Standard) {
      if (not hasDrivingType_) {
        type_ = net->getType();
        hasDrivingType_ = true;
      } else if (type_ != net->getType()) {
        type_ = SNLNet::Type::Standard;
      }
    }
    auto path = netOccurrence.getPath();
    for (auto component: net->getComponents()) {
      if (not start and component == comingFromComponent.getObject()) {
        continue;
      }
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        auto instance = instTerm->getInstance();
        if (instance->isLeaf()) {
          if (mode_ == SNLEquipotential::Mode::TraverseAssigns and
              NLDB0::isAssign(instance->getModel())) {
            auto assignInput = NLDB0::getAssignInput();
            auto assignOutput = NLDB0::getAssignOutput();
            auto oppositeTerm = instTerm->getBitTerm() == assignInput
              ? assignOutput
              : assignInput;
            auto oppositeInstTerm = instance->getInstTerm(oppositeTerm);
            if (graph_) {
              auto input = SNLOccurrence(path, instance->getInstTerm(assignInput));
              auto output = SNLOccurrence(path, instance->getInstTerm(assignOutput));
              graph_->cross(SNLEquipotential::Graph::EdgeKind::Assign, input,
                input.getComponentBitNetOccurrence(), output.getComponentBitNetOccurrence());
            }
            extractNetFromNetComponentOccurrence(
              naja::NL::SNLOccurrence(path, oppositeInstTerm), false);
            continue;
          }
          //construct InstTerm Occurrences
          instTermOccurrences_.emplace(naja::NL::SNLOccurrence(path, instTerm));
        } else {
          //get inside instance by exploring from term occurrence
          auto term = instTerm->getBitTerm();
          auto instancePath = naja::NL::SNLPath(path, instance);
          if (graph_) {
            graph_->cross(SNLEquipotential::Graph::EdgeKind::Port,
              SNLOccurrence(path, instTerm), netOccurrence,
              SNLOccurrence(instancePath, term).getComponentBitNetOccurrence());
          }
          extractNetFromNetComponentOccurrence(naja::NL::SNLOccurrence(instancePath, term), false);
        }
      } else {
        auto term = static_cast<SNLBitTerm*>(component);
        if (path.empty()) {
          //top term: end exploration here
          terms_.insert(term);      
        } else {
          for (auto term: net->getBitTerms()) {
            //go up: extract from upper instance term occurrence
            auto parentPath = path.getHeadPath();
            auto instance = path.getTailInstance();
            auto instTerm = instance->getInstTerm(term);
            if (graph_) {
              auto crossing = SNLOccurrence(parentPath, instTerm);
              graph_->cross(SNLEquipotential::Graph::EdgeKind::Port, crossing,
                crossing.getComponentBitNetOccurrence(), netOccurrence);
            }
            extractNetFromNetComponentOccurrence(naja::NL::SNLOccurrence(parentPath, instTerm), false);
          }
        }
      }
    }
  }
  
  //start extraction from net component
  //construct net and start exploration
  void extractNetFromNetComponentOccurrence(
    const naja::NL::SNLOccurrence& netComponentOccurrence,
    bool start
  ) {
    auto netOccurrence = netComponentOccurrence.getComponentBitNetOccurrence();
    if (netOccurrence.isValid()) {
      extractNetComponentsFromNetOccurrence(netOccurrence, netComponentOccurrence, start);
    }
  } 

#if 0
    auto netOccurrence = netComponentOccurrence.getNetOccurrence();
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      auto net = netOccurrence.getNet();
      auto path = netOccurrence.getPath();
      for (auto component: net->getComponents()) {
        auto instTerm = dynamic_cast<SNLInstTerm*>(component);
        if (instTerm) {

        } else {
          auto term = static_cast<SNLBitTerm*>(component);
          if (path.empty()) {
            
          } else {
            //construct term occurrence and go up
            auto termOccurrence = SNLBitTermOccurrence(path.getHeadPath(), term);
            extractFromNetComponentOccurrence(termOccurrence);
          }
        }
      }
    }
    naja::SNL::SNLPath path = netComponentOccurrence.getPath();
    naja::SNL::SNLBitNet* net = netComponentOccurrence.getNet();

    if (net) {
      //explore NetOccurrence
      naja::SNL::SNLBitNetOccurrence netOccurrence(path, net);
      extractFromNetOccurrence(netOccurrence);
    }
  }
#endif

  private:
    SNLEquipotential::InstTermOccurrences&  instTermOccurrences_;
    SNLEquipotential::Terms&                terms_;
    SNLNet::Type&                           type_;
    SNLEquipotential::Mode                  mode_;
    EquipotentialGraphBuilder*              graph_;
    bool                                    hasDrivingType_    {false};
    using NetOccurrences = std::set<SNLOccurrence>;
    NetOccurrences                          visitedNetOccurrences_  {};
};

}

namespace naja::NL {

SNLEquipotential::SNLEquipotential(
  SNLNetComponent* netComponent):
  SNLEquipotential(netComponent, Mode::Standard)
{}

SNLEquipotential::SNLEquipotential(
  SNLNetComponent* netComponent,
  Mode mode):
  SNLEquipotential(netComponent, Options{mode, false})
{}

SNLEquipotential::SNLEquipotential(
  const SNLOccurrence& netComponentOccurrence):
  SNLEquipotential(netComponentOccurrence, Mode::Standard)
{}

SNLEquipotential::SNLEquipotential(
  const SNLOccurrence& netComponentOccurrence,
  Mode mode):
  SNLEquipotential(netComponentOccurrence, Options{mode, false})
{}

SNLEquipotential::SNLEquipotential(
  SNLNetComponent* netComponent, const Options& options):
  SNLEquipotential(SNLOccurrence(netComponent), options)
{}

SNLEquipotential::SNLEquipotential(
  const SNLOccurrence& netComponentOccurrence, const Options& options) {
  auto graph = options.keepGraph ? std::make_unique<EquipotentialGraphBuilder>() : nullptr;
  SNLEquipotentialExtractor extractor(
    instTermOccurrences_, terms_, type_, options.mode, graph.get());
  extractor.extractNetFromNetComponentOccurrence(netComponentOccurrence, true);
  if (graph) {
    graph->finish(instTermOccurrences_, terms_);
    graph_ = graph->graph;
  }
}

bool SNLEquipotential::isConst0() const {
  return type_.isConst0();
}

bool SNLEquipotential::isConst1() const {
  return type_.isConst1();
}

bool SNLEquipotential::isConstX() const {
  return type_.isConstX();
}

bool SNLEquipotential::isConstZ() const {
  return type_.isConstZ();
}

std::string SNLEquipotential::getString() const {
  std::ostringstream stream;
  stream << "SNLEquipotential: " << type_.getString() << ", ";
  stream << "InstTermOccurrences: [";
  bool first = true;
  for (const auto& instTermOccurrence: instTermOccurrences_) {
    if (not first) {
      stream << ", ";
    }
    first = false;
    stream << instTermOccurrence.getString();
  }
  stream << "], Terms: [";
  first = true;
  for (const auto& term: terms_) {
    if (not first) {
      stream << ", ";
    }
    first = false;
    stream << term->getString();
  }
  stream << "]";
  if (graph_) {
    stream << ", Graph: Nodes: [";
    for (size_t i = 0; i < graph_->nodes.size(); ++i) {
      const auto& node = graph_->nodes[i];
      stream << "\n" << i << ": " << node.net.getString() << " Leaves: [";
      for (const auto& leaf: node.leaves) { stream << leaf.getString() << "; "; }
      stream << "] Terms: [";
      for (auto term: node.terms) { stream << term->getString() << "; "; }
      stream << "]";
    }
    stream << "], Edges: [";
    for (const auto& edge: graph_->edges) {
      stream << "\n" << (edge.kind == Graph::EdgeKind::Port ? "Port" : "Assign")
        << " " << edge.crossing.getString() << " " << edge.from << " -> " << edge.to;
    }
    stream << "]";
  }
  return stream.str();
}

namespace {
std::string dotQuote(const std::string& value) {
  std::string result = "\"";
  for (char c: value) {
    if (c == '\\' or c == '\"') { result += '\\'; }
    if (c == '\n') { result += "\\n"; }
    else if (c != '\r') { result += c; }
  }
  return result + "\"";
}
}

void SNLEquipotential::dumpDotFile(const std::string& path) const {
  std::ofstream stream(path);
  if (not stream) { throw NLException("Cannot open equipotential DOT file: " + path); }
  stream << "digraph equipotential {\n";
  if (graph_) {
    for (size_t i = 0; i < graph_->nodes.size(); ++i) {
      const auto& node = graph_->nodes[i];
      auto label = node.net.getString();
      for (const auto& leaf: node.leaves) { label += "\nLeaf: " + leaf.getString(); }
      for (auto term: node.terms) { label += "\nTerm: " + term->getString(); }
      stream << "  n" << i << " [label=" << dotQuote(label) << "];\n";
    }
    for (const auto& edge: graph_->edges) {
      auto label = std::string(edge.kind == Graph::EdgeKind::Port ? "Port: " : "Assign: ")
        + edge.crossing.getString();
      stream << "  n" << edge.from << " -> n" << edge.to
        << " [label=" << dotQuote(label) << "];\n";
    }
  }
  stream << "}\n";
}

NajaCollection<SNLBitTerm*> SNLEquipotential::getTerms() const {
  return NajaCollection(new NajaSTLCollection(&terms_));
}

NajaCollection<SNLOccurrence> SNLEquipotential::getInstTermOccurrences() const {
  return NajaCollection(new NajaSTLCollection(&instTermOccurrences_));
}

}  // namespace naja::NL
