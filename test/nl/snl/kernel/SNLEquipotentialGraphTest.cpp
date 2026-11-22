// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLPath.h"
#include "SNLEquipotential.h"
#include "NLException.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <tuple>

using namespace naja::NL;

class SNLEquipotentialGraphTest: public ::testing::Test {
  protected:
    using Equi = SNLEquipotential;
    using Graph = Equi::Graph;
    using Kind = Graph::EdgeKind;
    using EdgeKey = std::tuple<Kind, SNLOccurrence, SNLOccurrence, SNLOccurrence>;
    NLLibrary* lib_;
    SNLDesign* top_;
    SNLDesign* leaf_;
    SNLScalarTerm* pin_;

    void SetUp() override {
      auto db = NLDB::create(NLUniverse::create());
      lib_ = NLLibrary::create(db, NLName("lib"));
      top_ = design("top");
      auto primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
      leaf_ = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("leaf"));
      pin_ = port(leaf_, "i");
    }
    void TearDown() override { NLUniverse::get()->destroy(); }
    SNLDesign* design(const char* name) { return SNLDesign::create(lib_, NLName(name)); }
    SNLScalarTerm* port(SNLDesign* model, const char* name) {
      return SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName(name));
    }
    SNLScalarNet* net(SNLDesign* model, const char* name) {
      return SNLScalarNet::create(model, NLName(name));
    }
    SNLInstance* instance(SNLDesign* parent, SNLDesign* model, const char* name) {
      return SNLInstance::create(parent, model, NLName(name));
    }
    std::set<EdgeKey> edges(const Graph* graph) {
      std::set<EdgeKey> result;
      for (const auto& e: graph->edges) {
        result.emplace(e.kind, e.crossing, graph->nodes[e.from].net, graph->nodes[e.to].net);
      }
      return result;
    }
    Equi check(const SNLOccurrence& start, size_t nodes, size_t crossings,
        Equi::Mode mode = Equi::Mode::Standard) {
      Equi equi(start, Equi::Options{mode, true});
      Equi plain(start, mode);
      EXPECT_EQ(nullptr, plain.getGraph());
      EXPECT_EQ(equi, plain);
      EXPECT_FALSE(equi < plain);
      EXPECT_FALSE(equi > plain);
      EXPECT_TRUE(equi <= plain);
      EXPECT_TRUE(equi >= plain);
      EXPECT_EQ(equi.getType(), plain.getType());
      auto graph = equi.getGraph();
      EXPECT_EQ(nodes, graph->nodes.size());
      EXPECT_EQ(crossings, graph->edges.size());
      if (nodes) { EXPECT_EQ(start.getComponentBitNetOccurrence(), graph->nodes.front().net); }
      Equi::InstTermOccurrences leaves;
      Equi::Terms terms;
      std::set<SNLOccurrence> nets;
      for (const auto& node: graph->nodes) {
        EXPECT_TRUE(nets.insert(node.net).second);
        EXPECT_TRUE(std::is_sorted(node.leaves.begin(), node.leaves.end()));
        EXPECT_TRUE(std::is_sorted(node.terms.begin(), node.terms.end(), SNLDesignObject::PointerLess{}));
        for (const auto& leaf: node.leaves) {
          EXPECT_TRUE(leaves.insert(leaf).second);
          EXPECT_EQ(node.net, leaf.getComponentBitNetOccurrence());
        }
        for (auto term: node.terms) {
          EXPECT_TRUE(terms.insert(term).second);
          EXPECT_TRUE(node.net.getPath().empty());
          EXPECT_EQ(node.net, SNLOccurrence(term).getComponentBitNetOccurrence());
        }
      }
      EXPECT_EQ(equi.getInstTermOccurrencesSet(), leaves);
      EXPECT_EQ(equi.getTermsSet(), terms);
      std::set<SNLOccurrence> seen;
      for (const auto& edge: graph->edges) {
        EXPECT_LT(edge.from, graph->nodes.size());
        EXPECT_LT(edge.to, graph->nodes.size());
        EXPECT_TRUE(seen.insert(edge.crossing).second);
        auto instTerm = dynamic_cast<SNLInstTerm*>(edge.crossing.getObject());
        EXPECT_NE(nullptr, instTerm);
        if (edge.kind == Kind::Port) {
          EXPECT_FALSE(instTerm->getInstance()->isLeaf());
          EXPECT_EQ(edge.crossing.getComponentBitNetOccurrence(), graph->nodes[edge.from].net);
          auto inside = SNLOccurrence(SNLPath(edge.crossing.getPath(), instTerm->getInstance()),
            instTerm->getBitTerm()).getComponentBitNetOccurrence();
          EXPECT_EQ(inside, graph->nodes[edge.to].net);
        } else {
          EXPECT_EQ(NLDB0::getAssignInput(), instTerm->getBitTerm());
          EXPECT_EQ(edge.crossing.getComponentBitNetOccurrence(), graph->nodes[edge.from].net);
          auto output = instTerm->getInstance()->getInstTerm(NLDB0::getAssignOutput());
          EXPECT_EQ(SNLOccurrence(edge.crossing.getPath(), output).getComponentBitNetOccurrence(),
            graph->nodes[edge.to].net);
          EXPECT_EQ(graph->nodes[edge.from].net.getPath(), graph->nodes[edge.to].net.getPath());
        }
      }
      Equi copy(equi);
      EXPECT_EQ(graph, copy.getGraph());
      EXPECT_EQ(equi.getString(), copy.getString());
      Equi repeated(start, Equi::Options{mode, true});
      EXPECT_EQ(equi.getString(), repeated.getString());
      return equi;
    }
};

TEST_F(SNLEquipotentialGraphTest, TwoLevelsAndStartPoints) {
  auto middle = design("middle");
  auto inner = design("inner");
  auto topTerm = port(top_, "i");
  auto midTerm = port(middle, "i");
  auto innerTerm = port(inner, "i");
  auto a = instance(top_, middle, "a");
  auto b = instance(middle, inner, "b");
  auto c = instance(inner, leaf_, "c");
  auto outerNet = net(top_, "n\"\\\r\n");
  auto middleNet = net(middle, "n");
  auto innerNet = net(inner, "n");
  topTerm->setNet(outerNet);
  port(top_, "second")->setNet(outerNet);
  a->getInstTerm(midTerm)->setNet(outerNet);
  midTerm->setNet(middleNet);
  b->getInstTerm(innerTerm)->setNet(middleNet);
  innerTerm->setNet(innerNet);
  c->getInstTerm(pin_)->setNet(innerNet);
  auto equi = check(SNLOccurrence(topTerm), 3, 2);
  auto graph = equi.getGraph();
  EXPECT_EQ(SNLOccurrence(outerNet), graph->nodes[0].net);
  EXPECT_EQ(SNLOccurrence(SNLPath(a), middleNet), graph->nodes[1].net);
  EXPECT_EQ(SNLOccurrence(SNLPath(SNLPath(a), b), innerNet), graph->nodes[2].net);
  EXPECT_EQ(0u, graph->edges[0].from);
  EXPECT_EQ(1u, graph->edges[0].to);
  EXPECT_EQ(1u, graph->edges[1].from);
  EXPECT_EQ(2u, graph->edges[1].to);
  auto fromLeaf = check(SNLOccurrence(SNLPath(SNLPath(a), b), c->getInstTerm(pin_)), 3, 2);
  auto fromPort = check(SNLOccurrence(a->getInstTerm(midTerm)), 3, 2);
  EXPECT_EQ(edges(graph), edges(fromLeaf.getGraph()));
  EXPECT_EQ(edges(graph), edges(fromPort.getGraph()));
  EXPECT_EQ(equi, fromLeaf);
  EXPECT_EQ(equi, fromPort);
  EXPECT_EQ(equi, Equi(topTerm, Equi::Options{Equi::Mode::Standard, true}));
  auto path = std::filesystem::temp_directory_path() / "naja-equipotential-graph.dot";
  equi.dumpDotFile(path.string());
  std::ifstream file(path);
  std::string dot((std::istreambuf_iterator<char>(file)), {});
  EXPECT_NE(std::string::npos, dot.find("n0 -> n1"));
  EXPECT_NE(std::string::npos, dot.find("Port:"));
  EXPECT_NE(std::string::npos, dot.find("Leaf:"));
  EXPECT_NE(std::string::npos, dot.find("Term:"));
  EXPECT_NE(std::string::npos, dot.find("\\\""));
  EXPECT_NE(std::string::npos, dot.find("\\\\"));
  EXPECT_NE(std::string::npos, dot.find("\\n"));
  std::filesystem::remove(path);
  EXPECT_THROW(equi.dumpDotFile((path / "missing" / "file.dot").string()), NLException);
}

TEST_F(SNLEquipotentialGraphTest, SiblingsOfSameModel) {
  auto model = design("model");
  auto term = port(model, "i");
  auto child = instance(model, leaf_, "child");
  auto inside = net(model, "inside");
  term->setNet(inside);
  child->getInstTerm(pin_)->setNet(inside);
  auto a = instance(top_, model, "a");
  auto b = instance(top_, model, "b");
  auto outside = net(top_, "outside");
  a->getInstTerm(term)->setNet(outside);
  b->getInstTerm(term)->setNet(outside);
  auto equi = check(SNLOccurrence(SNLPath(a), child->getInstTerm(pin_)), 3, 2);
  EXPECT_EQ(2u, equi.getInstTermOccurrencesSet().size());
  EXPECT_EQ(SNLOccurrence(SNLPath(a), inside), equi.getGraph()->nodes[0].net);
  EXPECT_EQ(SNLOccurrence(outside), equi.getGraph()->nodes[1].net);
  EXPECT_EQ(SNLOccurrence(SNLPath(b), inside), equi.getGraph()->nodes[2].net);
}

TEST_F(SNLEquipotentialGraphTest, FeedthroughAndCycle) {
  auto model = design("model");
  auto a = port(model, "a");
  auto b = port(model, "b");
  auto inside = net(model, "inside");
  a->setNet(inside);
  b->setNet(inside);
  auto inst = instance(top_, model, "u");
  auto left = net(top_, "left");
  auto right = net(top_, "right");
  inst->getInstTerm(a)->setNet(left);
  inst->getInstTerm(b)->setNet(right);
  auto feed = check(SNLOccurrence(inst->getInstTerm(a)), 3, 2);
  EXPECT_EQ(SNLOccurrence(left), feed.getGraph()->nodes[0].net);
  EXPECT_EQ(SNLOccurrence(SNLPath(inst), inside), feed.getGraph()->nodes[1].net);
  EXPECT_EQ(SNLOccurrence(right), feed.getGraph()->nodes[2].net);
  inst->getInstTerm(b)->setNet(left);
  auto cycle = check(SNLOccurrence(inst->getInstTerm(a)), 2, 2);
  EXPECT_EQ(cycle.getGraph()->edges[0].from, cycle.getGraph()->edges[1].from);
  EXPECT_EQ(cycle.getGraph()->edges[0].to, cycle.getGraph()->edges[1].to);
}

TEST_F(SNLEquipotentialGraphTest, TwoPortsSeparateNetsAndBusBit) {
  auto model = design("model");
  auto bus = SNLBusTerm::create(model, SNLTerm::Direction::Input, 5, 3, NLName("p"));
  auto a = bus->getBit(5);
  auto b = bus->getBit(3);
  auto inst = instance(top_, model, "u");
  auto outside = net(top_, "outside");
  auto na = net(model, "na");
  auto nb = net(model, "nb");
  a->setNet(na);
  b->setNet(nb);
  auto la = instance(model, leaf_, "la");
  auto lb = instance(model, leaf_, "lb");
  la->getInstTerm(pin_)->setNet(na);
  lb->getInstTerm(pin_)->setNet(nb);
  inst->getInstTerm(a)->setNet(outside);
  inst->getInstTerm(b)->setNet(outside);
  auto equi = check(SNLOccurrence(inst->getInstTerm(a)), 3, 2);
  auto graph = equi.getGraph();
  for (const auto& edge: graph->edges) {
    auto crossed = static_cast<SNLInstTerm*>(edge.crossing.getObject())->getBitTerm();
    EXPECT_TRUE(crossed == a or crossed == b);
    const auto& node = graph->nodes[edge.to];
    ASSERT_EQ(1u, node.leaves.size());
    EXPECT_EQ(SNLOccurrence(SNLPath(inst), crossed == a ? na : nb), node.net);
    EXPECT_EQ(crossed == a ? la->getInstTerm(pin_) : lb->getInstTerm(pin_), node.leaves[0].getObject());
  }
}

TEST_F(SNLEquipotentialGraphTest, InternalAndDisconnectedNets) {
  auto model = design("model");
  auto inst = instance(top_, model, "u");
  auto child = instance(model, leaf_, "child");
  auto inside = net(model, "inside");
  child->getInstTerm(pin_)->setNet(inside);
  check(SNLOccurrence(SNLPath(inst), child->getInstTerm(pin_)), 1, 0);
  auto floating = port(top_, "floating");
  check(SNLOccurrence(floating), 0, 0);
  auto term = port(model, "p");
  term->setNet(inside);
  // Disconnected outer side is not a crossing between two visited nets.
  check(SNLOccurrence(SNLPath(inst), child->getInstTerm(pin_)), 1, 0);
  auto other = port(model, "other");
  auto outer = net(top_, "outside");
  inst->getInstTerm(other)->setNet(outer);
  // Disconnected inner side likewise has no edge.
  check(SNLOccurrence(inst->getInstTerm(other)), 1, 0);
}

TEST_F(SNLEquipotentialGraphTest, AssignsInHierarchyAndReverseDirection) {
  auto model = design("model");
  auto input = port(model, "i");
  auto output = port(model, "o");
  auto inNet = net(model, "in");
  auto outNet = net(model, "out");
  input->setNet(inNet);
  output->setNet(outNet);
  auto assign = instance(model, NLDB0::getAssign(), "link");
  auto ai = assign->getInstTerm(NLDB0::getAssignInput());
  auto ao = assign->getInstTerm(NLDB0::getAssignOutput());
  ai->setNet(inNet);
  ao->setNet(outNet);
  auto inst = instance(top_, model, "u");
  auto topTerm = port(top_, "i");
  auto outer = net(top_, "outside");
  topTerm->setNet(outer);
  inst->getInstTerm(input)->setNet(outer);
  auto standard = check(SNLOccurrence(topTerm), 2, 1);
  EXPECT_EQ(Equi::InstTermOccurrences({SNLOccurrence(SNLPath(inst), ai)}), standard.getInstTermOccurrencesSet());
  auto forward = check(SNLOccurrence(topTerm), 3, 2, Equi::Mode::TraverseAssigns);
  auto reverse = check(SNLOccurrence(SNLPath(inst), ao), 3, 2, Equi::Mode::TraverseAssigns);
  EXPECT_TRUE(forward.getInstTermOccurrencesSet().empty());
  EXPECT_EQ(edges(forward.getGraph()), edges(reverse.getGraph()));
  EXPECT_NE(std::string::npos, forward.getString().find("Assign"));
  auto path = std::filesystem::temp_directory_path() / "naja-equipotential-assign.dot";
  forward.dumpDotFile(path.string());
  std::ifstream file(path);
  std::string dot((std::istreambuf_iterator<char>(file)), {});
  EXPECT_NE(std::string::npos, dot.find("Assign:"));
  std::filesystem::remove(path);
  ao->setNet(nullptr);
  check(SNLOccurrence(topTerm), 2, 1, Equi::Mode::TraverseAssigns);
}

TEST_F(SNLEquipotentialGraphTest, EmptyAndSharedGraphCopy) {
  Equi empty;
  EXPECT_EQ(nullptr, empty.getGraph());
  auto term = port(top_, "i");
  Equi copy;
  {
    Equi original(term, Equi::Options{Equi::Mode::Standard, true});
    ASSERT_NE(nullptr, original.getGraph());
    EXPECT_TRUE(original.getGraph()->nodes.empty());
    copy = original;
    EXPECT_EQ(original.getGraph(), copy.getGraph());
  }
  EXPECT_TRUE(copy.getGraph()->nodes.empty());
  EXPECT_TRUE(copy.getGraph()->edges.empty());
  EXPECT_EQ(Equi(term), copy);
  EXPECT_NE(std::string::npos, copy.getString().find("Graph: Nodes: []"));
  auto path = std::filesystem::temp_directory_path() / "naja-equipotential-empty.dot";
  copy.dumpDotFile(path.string());
  std::ifstream file(path);
  std::string dot((std::istreambuf_iterator<char>(file)), {});
  EXPECT_EQ("digraph equipotential {\n}\n", dot);
  empty.dumpDotFile(path.string());
  std::filesystem::remove(path);
}
