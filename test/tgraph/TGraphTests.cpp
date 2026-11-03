// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"

#include "TClock.h"
#include "TClockPropagator.h"
#include "TEdge.h"
#include "TEdgeProxy.h"
#include "TGraph.h"
#include "TLoopBreaker.h"
#include "TNode.h"
#include "TNodeProxy.h"

using namespace naja::NL;
using namespace naja::TG;

class TGraphTests: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto designsLib = NLLibrary::create(db);
      top_ = SNLDesign::create(designsLib, NLName("TOP"));
      auto model = SNLDesign::create(designsLib, NLName("MODEL"));
      modelClockTerm_ = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("clk"));
      topClockTerm_ = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("clk"));
      instance_ = SNLInstance::create(top_, model, NLName("instance"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    SNLDesign*      top_             {nullptr};
    SNLInstance*    instance_        {nullptr};
    SNLScalarTerm*  topClockTerm_    {nullptr};
    SNLScalarTerm*  modelClockTerm_  {nullptr};
};

TEST_F(TGraphTests, testNodesAndEdges) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  EXPECT_EQ(2u, graph.getNodesCount());
  auto e0 = graph.createEdge(n0, n1, nullptr);
  EXPECT_EQ(1u, graph.getEdgesCount());
  auto edge = graph.getEdge(e0);
  ASSERT_NE(nullptr, edge);
  EXPECT_EQ(n0, edge->getSource());
  EXPECT_EQ(n1, edge->getTarget());
}

TEST_F(TGraphTests, testInvalidAndRemovedHandles) {
  TGraph graph;
  const TGraph& view = graph;
  auto source = graph.createNode(nullptr);
  auto target = graph.createNode(nullptr);
  auto edge = graph.createEdge(source, target, nullptr);
  EXPECT_EQ(graph.getNode(source), view.getNode(source));
  EXPECT_EQ(graph.getEdge(edge), view.getEdge(edge));
  for (auto node : {TGraph::kInvalidNode, graph.getNodesStorage().size()}) {
    EXPECT_EQ(nullptr, graph.getNode(node));
    EXPECT_EQ(nullptr, view.getNode(node));
    EXPECT_FALSE(graph.removeNode(node));
    EXPECT_EQ(TGraph::kInvalidEdge, graph.createEdge(node, target, nullptr));
    EXPECT_EQ(TGraph::kInvalidEdge, graph.createEdge(source, node, nullptr));
  }
  for (auto invalid : {TGraph::kInvalidEdge, graph.getEdgesStorage().size()}) {
    EXPECT_EQ(nullptr, graph.getEdge(invalid));
    EXPECT_EQ(nullptr, view.getEdge(invalid));
    EXPECT_FALSE(graph.removeEdge(invalid));
  }
  EXPECT_EQ(2u, graph.getNodesCount());
  EXPECT_EQ(1u, graph.getEdgesCount());
  ASSERT_TRUE(graph.removeEdge(edge));
  EXPECT_FALSE(graph.removeEdge(edge));
  EXPECT_EQ(nullptr, graph.getEdge(edge));
  EXPECT_EQ(nullptr, view.getEdge(edge));
  ASSERT_TRUE(graph.removeNode(source));
  EXPECT_FALSE(graph.removeNode(source));
  EXPECT_EQ(nullptr, graph.getNode(source));
  EXPECT_EQ(nullptr, view.getNode(source));
  EXPECT_EQ(TGraph::kInvalidEdge, graph.createEdge(source, target, nullptr));
  EXPECT_EQ(TGraph::kInvalidEdge, graph.createEdge(target, source, nullptr));
  EXPECT_EQ(1u, graph.getNodesCount());
  EXPECT_EQ(0u, graph.getEdgesCount());
  EXPECT_TRUE(graph.getNode(target)->getInEdges().empty());
  EXPECT_TRUE(graph.getNode(target)->getOutEdges().empty());
}

TEST_F(TGraphTests, testEdgeSlotReuseResetsStateAndEndpoints) {
  TGraph graph;
  auto a = graph.createNode(nullptr);
  auto b = graph.createNode(nullptr);
  auto c = graph.createNode(nullptr);
  auto clock = graph.createClock("first", nullptr, a);
  auto edge = graph.createEdge(a, b, nullptr);
  graph.getEdge(edge)->addFromClock(clock);
  graph.getEdge(edge)->addToClock(clock);
  graph.getEdge(edge)->setDisabled(true);
  ASSERT_TRUE(graph.removeEdge(edge));
  EXPECT_TRUE(graph.getNode(a)->getOutEdges().empty());
  EXPECT_TRUE(graph.getNode(b)->getInEdges().empty());
  graph.createClock("second", nullptr, c);
  auto reused = graph.createEdge(c, a, nullptr);
  ASSERT_EQ(edge, reused);
  EXPECT_EQ(1u, graph.getEdgesStorage().size());
  EXPECT_EQ(1u, graph.getEdgesCount());
  const TGraph& view = graph;
  const auto* restored = view.getEdge(reused);
  ASSERT_NE(nullptr, restored);
  EXPECT_TRUE(restored->isAlive());
  EXPECT_EQ(reused, restored->getIndex());
  EXPECT_EQ(c, restored->getSource());
  EXPECT_EQ(a, restored->getTarget());
  EXPECT_FALSE(restored->isDisabled());
  EXPECT_EQ(2u, restored->getFromClocks().size());
  EXPECT_EQ(2u, restored->getToClocks().size());
  EXPECT_TRUE(restored->getFromClocks().none());
  EXPECT_TRUE(restored->getToClocks().none());
  EXPECT_EQ((std::vector<TGraph::EdgeId>{reused}), view.getNode(c)->getOutEdges());
  EXPECT_EQ((std::vector<TGraph::EdgeId>{reused}), view.getNode(a)->getInEdges());
  EXPECT_TRUE(view.getNode(b)->getInEdges().empty());
}

TEST_F(TGraphTests, testEdgeMoveAssignmentTransfersStateAndProxyOwnership) {
  struct Proxy: TEdgeProxy {
    explicit Proxy(bool& destroyed): destroyed_(destroyed) {}
    ~Proxy() override { destroyed_ = true; }
    bool& destroyed_;
  };
  bool sourceDestroyed = false;
  bool replacedDestroyed = false;
  {
    auto* proxy = new Proxy(sourceDestroyed);
    auto source = TEdge::create(7, 2, 3, proxy);
    const TEdge::ClockMask from(3, 5);
    const TEdge::ClockMask to(3, 2);
    source.setFromClocks(from);
    source.setToClocks(to);
    source.setDisabled(true);
    auto destination = TEdge::create(8, 4, 5, new Proxy(replacedDestroyed));
    destination = std::move(source);
    EXPECT_TRUE(replacedDestroyed);
    EXPECT_FALSE(sourceDestroyed);
    EXPECT_EQ(nullptr, source.getProxy());
    EXPECT_EQ(proxy, destination.getProxy());
    EXPECT_EQ(7u, destination.getIndex());
    EXPECT_EQ(2u, destination.getSource());
    EXPECT_EQ(3u, destination.getTarget());
    EXPECT_TRUE(destination.isAlive());
    EXPECT_TRUE(destination.isDisabled());
    EXPECT_EQ(from, destination.getFromClocks());
    EXPECT_EQ(to, destination.getToClocks());
  }
  EXPECT_TRUE(sourceDestroyed);
}

TEST_F(TGraphTests, testRemoveNodeDisconnectsAllSignalEdges) {
  TGraph graph;
  auto source = graph.createNode(nullptr);
  auto middle = graph.createNode(nullptr);
  auto target = graph.createNode(nullptr);
  auto incoming = graph.createEdge(source, middle, nullptr);
  auto outgoing = graph.createEdge(middle, target, nullptr);
  auto selfLoop = graph.createEdge(middle, middle, nullptr);
  auto surviving = graph.createEdge(source, target, nullptr);
  ASSERT_TRUE(graph.removeNode(middle));
  EXPECT_EQ(2u, graph.getNodesCount());
  EXPECT_EQ(1u, graph.getEdgesCount());
  for (auto edge : {incoming, outgoing, selfLoop}) {
    EXPECT_FALSE(graph.isEdgeValid(edge));
    EXPECT_EQ(nullptr, graph.getEdge(edge));
  }
  EXPECT_TRUE(graph.isEdgeValid(surviving));
  EXPECT_EQ((std::vector<TGraph::EdgeId>{surviving}), graph.getNode(source)->getOutEdges());
  EXPECT_EQ((std::vector<TGraph::EdgeId>{surviving}), graph.getNode(target)->getInEdges());
  ASSERT_EQ(middle, graph.createNode(nullptr));
  EXPECT_TRUE(graph.getNode(middle)->getInEdges().empty());
  EXPECT_TRUE(graph.getNode(middle)->getOutEdges().empty());
}

TEST_F(TGraphTests, testNodeClassification) {
  using CellKind = TNode::CellKind;
  using PinRole = TNode::PinRole;
  TGraph graph;
  auto id = graph.createNode(nullptr);
  auto node = graph.getNode(id);
  EXPECT_EQ(CellKind::Unknown, node->getCellKind());
  EXPECT_EQ(PinRole::Unknown, node->getPinRole());

  struct Query {
    CellKind kind;
    PinRole role;
    bool (TNode::*matches)() const;
  };
  const Query queries[] = {
    {CellKind::FlipFlop, PinRole::DataInput, &TNode::isFlopDataInput},
    {CellKind::FlipFlop, PinRole::DataOutput, &TNode::isFlopDataOutput},
    {CellKind::FlipFlop, PinRole::Clock, &TNode::isFlopClock},
    {CellKind::Latch, PinRole::DataInput, &TNode::isLatchDataInput},
    {CellKind::Latch, PinRole::DataOutput, &TNode::isLatchDataOutput},
    {CellKind::Latch, PinRole::Gate, &TNode::isLatchGate},
    {CellKind::Combinational, PinRole::DataInput, &TNode::isCombinationalReceiver},
    {CellKind::Combinational, PinRole::DataOutput, &TNode::isCombinationalDriver},
  };
  for (auto kind : {CellKind::Unknown, CellKind::Combinational, CellKind::FlipFlop, CellKind::Latch}) {
    node->setCellKind(kind);
    EXPECT_EQ(kind, node->getCellKind());
    EXPECT_EQ(kind == CellKind::Combinational, node->isCombinational());
    EXPECT_EQ(kind == CellKind::FlipFlop, node->isFlipFlop());
    EXPECT_EQ(kind == CellKind::Latch, node->isLatch());
    for (auto role : {PinRole::Unknown, PinRole::DataInput, PinRole::DataOutput,
                     PinRole::Clock, PinRole::Gate, PinRole::Enable,
                     PinRole::SyncReset, PinRole::SyncSet, PinRole::AsyncReset, PinRole::AsyncSet}) {
      node->setPinRole(role);
      EXPECT_EQ(role, node->getPinRole());
      //Controls must never be mistaken for data, and latch roles must not
      //match flop traversal boundaries (or vice versa).
      for (const auto& query : queries) {
        EXPECT_EQ(kind == query.kind && role == query.role, (node->*query.matches)())
          << "kind=" << static_cast<int>(kind) << " role=" << static_cast<int>(role);
      }
    }
  }
}

TEST_F(TGraphTests, testNodeClassificationSurvivesGrowthAndClearsOnReuse) {
  TGraph graph;
  auto id = graph.createNode(nullptr);
  graph.getNode(id)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(id)->setPinRole(TNode::PinRole::DataOutput);
  //Force vector growth without retaining a transient node pointer.
  for (size_t i = 0; i < 256; ++i) {
    graph.createNode(nullptr);
  }
  EXPECT_TRUE(graph.getNode(id)->isFlopDataOutput());
  ASSERT_TRUE(graph.removeNode(id));
  EXPECT_EQ(TNode::CellKind::Unknown, graph.getNodesStorage()[id].getCellKind());
  EXPECT_EQ(TNode::PinRole::Unknown, graph.getNodesStorage()[id].getPinRole());
  ASSERT_EQ(id, graph.createNode(nullptr));
  EXPECT_EQ(TNode::CellKind::Unknown, graph.getNode(id)->getCellKind());
  EXPECT_EQ(TNode::PinRole::Unknown, graph.getNode(id)->getPinRole());
  EXPECT_FALSE(graph.getNode(id)->isFlopDataOutput());
}

TEST_F(TGraphTests, testNodeMoveAssignmentTransfersStateAndProxyOwnership) {
  struct Proxy: TNodeProxy {
    explicit Proxy(bool& destroyed): destroyed_(destroyed) {}
    ~Proxy() override { destroyed_ = true; }
    bool& destroyed_;
  };
  bool sourceDestroyed = false;
  bool replacedDestroyed = false;
  {
    TGraph graph;
    auto* proxy = new Proxy(sourceDestroyed);
    auto before = graph.createNode(nullptr);
    auto source = graph.createNode(proxy);
    auto after = graph.createNode(nullptr);
    auto incoming = graph.createEdge(before, source, nullptr);
    auto outgoing = graph.createEdge(source, after, nullptr);
    auto captureIn = graph.createCaptureEdge(before, source);
    auto captureOut = graph.createCaptureEdge(source, after);
    graph.getNode(source)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(source)->setPinRole(TNode::PinRole::DataInput);
    auto destination = TNode::create(99, new Proxy(replacedDestroyed));
    destination = std::move(*graph.getNode(source));
    EXPECT_TRUE(replacedDestroyed);
    EXPECT_FALSE(sourceDestroyed);
    EXPECT_EQ(nullptr, graph.getNode(source)->getProxy());
    EXPECT_EQ(proxy, destination.getProxy());
    EXPECT_EQ(source, destination.getIndex());
    EXPECT_TRUE(destination.isAlive());
    EXPECT_EQ(TNode::CellKind::FlipFlop, destination.getCellKind());
    EXPECT_EQ(TNode::PinRole::DataInput, destination.getPinRole());
    EXPECT_TRUE(destination.isFlopDataInput());
    EXPECT_EQ((std::vector<TNode::EdgeId>{incoming}), destination.getInEdges());
    EXPECT_EQ((std::vector<TNode::EdgeId>{outgoing}), destination.getOutEdges());
    EXPECT_EQ((std::vector<TNode::CaptureEdgeId>{captureIn}), destination.getCaptureInEdges());
    EXPECT_EQ((std::vector<TNode::CaptureEdgeId>{captureOut}), destination.getCaptureOutEdges());
  }
  EXPECT_TRUE(sourceDestroyed);
}

TEST_F(TGraphTests, testCaptureEdgesAreSeparateFromSignalTraversal) {
  TGraph graph;
  auto clock = graph.createNode(nullptr);
  auto data = graph.createNode(nullptr);
  graph.getNode(clock)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(clock)->setPinRole(TNode::PinRole::Clock);
  graph.getNode(data)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(data)->setPinRole(TNode::PinRole::DataInput);
  auto capture = graph.createCaptureEdge(clock, data);
  const TGraph& view = graph;
  ASSERT_NE(nullptr, view.getCaptureEdge(capture));
  EXPECT_EQ(capture, view.getCaptureEdge(capture)->getIndex());
  EXPECT_EQ(clock, view.getCaptureEdge(capture)->getSource());
  EXPECT_EQ(data, view.getCaptureEdge(capture)->getTarget());
  EXPECT_TRUE(view.getCaptureEdge(capture)->isAlive());
  EXPECT_EQ((std::vector<TGraph::CaptureEdgeId>{capture}), graph.getNode(clock)->getCaptureOutEdges());
  EXPECT_EQ((std::vector<TGraph::CaptureEdgeId>{capture}), graph.getNode(data)->getCaptureInEdges());
  EXPECT_TRUE(graph.getNode(clock)->getCaptureInEdges().empty());
  EXPECT_TRUE(graph.getNode(data)->getCaptureOutEdges().empty());
  EXPECT_EQ(1u, graph.getCaptureEdgesCount());
  EXPECT_EQ(0u, graph.getEdgesCount());
  EXPECT_TRUE(graph.getEdgesStorage().empty());
  EXPECT_EQ(0u, graph.getNode(clock)->getOutDegree());
  EXPECT_EQ(0u, graph.getNode(data)->getInDegree());

  auto seed = graph.createClock("clk", topClockTerm_, clock);
  EXPECT_EQ(0u, TLoopBreaker::run(graph));
  EXPECT_NO_THROW(TClockPropagator::run(graph));
  //A signal edge opposite to capture creates a cycle only if capture
  //relationships accidentally participate in ordinary traversal.
  auto signal = graph.createEdge(data, clock, nullptr);
  EXPECT_EQ(0u, TLoopBreaker::run(graph));
  EXPECT_NO_THROW(TClockPropagator::run(graph));
  EXPECT_FALSE(graph.getEdge(signal)->isDisabled());
  EXPECT_FALSE(graph.getEdge(signal)->hasFromClock(seed));
  EXPECT_FALSE(graph.getEdge(signal)->hasToClock(seed));
  EXPECT_EQ(1u, graph.getEdgesCount());
  EXPECT_EQ(1u, graph.getCaptureEdgesCount());
}

TEST_F(TGraphTests, testCaptureEdgeRemovalAndSlotReuse) {
  TGraph graph;
  auto a = graph.createNode(nullptr);
  auto b = graph.createNode(nullptr);
  EXPECT_EQ(TGraph::kInvalidCaptureEdge, graph.createCaptureEdge(a, TGraph::kInvalidNode));
  EXPECT_EQ(TGraph::kInvalidCaptureEdge, graph.createCaptureEdge(TGraph::kInvalidNode, b));
  EXPECT_FALSE(graph.removeCaptureEdge(TGraph::kInvalidCaptureEdge));
  EXPECT_EQ(nullptr, graph.getCaptureEdge(0));
  EXPECT_EQ(0u, graph.getCaptureEdgesCount());
  auto first = graph.createCaptureEdge(a, b);
  auto parallel = graph.createCaptureEdge(a, b);
  ASSERT_TRUE(graph.removeCaptureEdge(first));
  EXPECT_FALSE(graph.removeCaptureEdge(first));
  EXPECT_FALSE(graph.isCaptureEdgeValid(first));
  EXPECT_EQ(nullptr, graph.getCaptureEdge(first));
  EXPECT_FALSE(graph.getCaptureEdgesStorage()[first].isAlive());
  EXPECT_EQ((std::vector<TGraph::CaptureEdgeId>{parallel}), graph.getNode(a)->getCaptureOutEdges());
  EXPECT_EQ((std::vector<TGraph::CaptureEdgeId>{parallel}), graph.getNode(b)->getCaptureInEdges());
  ASSERT_EQ(first, graph.createCaptureEdge(b, a));
  EXPECT_EQ(b, graph.getCaptureEdge(first)->getSource());
  EXPECT_EQ(a, graph.getCaptureEdge(first)->getTarget());
  EXPECT_TRUE(graph.isCaptureEdgeValid(first));
  EXPECT_EQ(2u, graph.getCaptureEdgesCount());
  //Both incoming and outgoing capture relationships are removed with a node.
  graph.createCaptureEdge(a, a);
  auto signal = graph.createEdge(a, b, nullptr);
  ASSERT_TRUE(graph.removeNode(a));
  EXPECT_EQ(0u, graph.getCaptureEdgesCount());
  EXPECT_FALSE(graph.isEdgeValid(signal));
  EXPECT_TRUE(graph.getNode(b)->getCaptureInEdges().empty());
  EXPECT_TRUE(graph.getNode(b)->getCaptureOutEdges().empty());
  EXPECT_EQ(TGraph::kInvalidCaptureEdge, graph.createCaptureEdge(a, b));
  ASSERT_EQ(a, graph.createNode(nullptr));
  EXPECT_TRUE(graph.getNode(a)->getCaptureInEdges().empty());
  EXPECT_TRUE(graph.getNode(a)->getCaptureOutEdges().empty());
  auto reused = graph.createCaptureEdge(a, b);
  EXPECT_TRUE(graph.isCaptureEdgeValid(reused));
  EXPECT_EQ(1u, graph.getCaptureEdgesCount());
}

TEST_F(TGraphTests, testClockOnTopAndInstanceTerminal) {
  TGraph graph;
  auto topNode = graph.createNode(nullptr);
  auto instNode = graph.createNode(nullptr);

  //Clock reaches the top terminal directly...
  auto topClock = graph.createClock("clk", topClockTerm_, topNode);
  //...and, through the instance, the instance terminal.
  auto instTerm = instance_->getInstTerm(modelClockTerm_);
  ASSERT_NE(nullptr, instTerm);
  auto instClock = graph.createClock("clk", instTerm, instNode);

  EXPECT_EQ(2u, graph.getClocksCount());
  EXPECT_EQ("clk", graph.getClock(topClock)->getName());
  EXPECT_EQ(topClockTerm_, graph.getClock(topClock)->getTarget());
  EXPECT_EQ(topNode, graph.getClock(topClock)->getNode());
  EXPECT_EQ(instTerm, graph.getClock(instClock)->getTarget());
  EXPECT_EQ(instNode, graph.getClock(instClock)->getNode());
  EXPECT_EQ(nullptr, graph.getClock(graph.getClocksCount()));
}

TEST_F(TGraphTests, testCreateClockRejectsInvalidNode) {
  TGraph graph;
  EXPECT_THROW(graph.createClock("clk", nullptr, 0), std::invalid_argument);
}

TEST_F(TGraphTests, testEdgeClockMasksDetectCrossing) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto edge = graph.getEdge(e0);
  //No clocks registered yet: masks are empty.
  EXPECT_EQ(0u, edge->getFromClocks().size());
  EXPECT_EQ(0u, edge->getToClocks().size());

  auto clkA = graph.createClock("clkA", topClockTerm_, n0);
  auto clkB = graph.createClock("clkB", nullptr, n1);

  //Registering clocks grows every existing edge's masks to match, all zero.
  EXPECT_EQ(2u, edge->getFromClocks().size());
  EXPECT_EQ(2u, edge->getToClocks().size());
  EXPECT_FALSE(edge->hasClockCrossing());

  //Same clock on both sides: no crossing.
  edge->addFromClock(clkA);
  edge->addToClock(clkA);
  EXPECT_TRUE(edge->hasFromClock(clkA));
  EXPECT_TRUE(edge->hasToClock(clkA));
  EXPECT_FALSE(edge->hasClockCrossing());

  //A different clock reaches the target side: crossing.
  edge->addToClock(clkB);
  EXPECT_TRUE(edge->hasClockCrossing());
  EXPECT_FALSE(edge->hasFromClock(clkB));
  EXPECT_TRUE(edge->hasToClock(clkB));
}

TEST_F(TGraphTests, testClockCountIsUnbounded) {
  TGraph graph;
  auto seedNode = graph.createNode(nullptr);
  //Register well past the old hand-rolled 64-bit mask's limit.
  constexpr std::size_t kClockCount = 200;
  for (std::size_t i = 0; i < kClockCount; ++i) {
    graph.createClock("clk" + std::to_string(i), nullptr, seedNode);
  }
  EXPECT_EQ(kClockCount, graph.getClocksCount());

  auto e0 = graph.createEdge(graph.createNode(nullptr), graph.createNode(nullptr), nullptr);
  auto edge = graph.getEdge(e0);
  TClock::ClockId highClock = kClockCount - 1;
  edge->addFromClock(highClock);
  EXPECT_GT(edge->getFromClocks().size(), 64u);
  EXPECT_TRUE(edge->hasFromClock(highClock));
}

TEST_F(TGraphTests, testAllEdgesShareTheSameClockMaskSize) {
  TGraph graph;
  auto seedNode = graph.createNode(nullptr);
  //One edge created before any clock exists...
  auto e0 = graph.createEdge(graph.createNode(nullptr), graph.createNode(nullptr), nullptr);
  EXPECT_EQ(0u, graph.getEdge(e0)->getFromClocks().size());

  graph.createClock("clkA", nullptr, seedNode);
  //...ends up sized like an edge created after that clock exists.
  //NB: re-fetch getEdge() after each mutation - createEdge()/createClock()
  //can reallocate the backing storage and invalidate previously-returned
  //TEdge* pointers; EdgeId is the stable handle, not the pointer.
  auto e1 = graph.createEdge(graph.createNode(nullptr), graph.createNode(nullptr), nullptr);
  EXPECT_EQ(1u, graph.getEdge(e0)->getFromClocks().size());
  EXPECT_EQ(1u, graph.getEdge(e1)->getFromClocks().size());
  EXPECT_EQ(1u, graph.getEdge(e0)->getToClocks().size());
  EXPECT_EQ(1u, graph.getEdge(e1)->getToClocks().size());

  //Every additional clock keeps both edges' masks in lockstep with getClocksCount().
  for (int i = 0; i < 10; ++i) {
    graph.createClock("clk" + std::to_string(i), nullptr, seedNode);
  }
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e0)->getFromClocks().size());
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e1)->getFromClocks().size());
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e0)->getToClocks().size());
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e1)->getToClocks().size());
}

TEST_F(TGraphTests, testLoopBreakerLeavesAcyclicGraphUntouched) {
  TGraph graph;
  //Diamond: n0 -> n1 -> n3, n0 -> n2 -> n3. No cycle.
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  auto n3 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n0, n2, nullptr);
  auto e2 = graph.createEdge(n1, n3, nullptr);
  auto e3 = graph.createEdge(n2, n3, nullptr);

  EXPECT_EQ(0u, TLoopBreaker::run(graph));

  EXPECT_FALSE(graph.getEdge(e0)->isDisabled());
  EXPECT_FALSE(graph.getEdge(e1)->isDisabled());
  EXPECT_FALSE(graph.getEdge(e2)->isDisabled());
  EXPECT_FALSE(graph.getEdge(e3)->isDisabled());
}

TEST_F(TGraphTests, testLoopBreakerDisablesSelfLoop) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto selfLoop = graph.createEdge(n0, n0, nullptr);

  EXPECT_EQ(1u, TLoopBreaker::run(graph));
  EXPECT_TRUE(graph.getEdge(selfLoop)->isDisabled());
}

TEST_F(TGraphTests, testLoopBreakerBreaksThreeNodeCycle) {
  TGraph graph;
  //n0 -> n1 -> n2 -> n0.
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n1, n2, nullptr);
  auto e2 = graph.createEdge(n2, n0, nullptr);

  EXPECT_EQ(1u, TLoopBreaker::run(graph));

  //Exactly one of the three edges was cut; which one depends on DFS/edge
  //order, so just check the invariant: cutting it must break the cycle.
  const std::size_t disabledCount =
    (graph.getEdge(e0)->isDisabled() ? 1 : 0) +
    (graph.getEdge(e1)->isDisabled() ? 1 : 0) +
    (graph.getEdge(e2)->isDisabled() ? 1 : 0);
  EXPECT_EQ(1u, disabledCount);
}

TEST_F(TGraphTests, testLoopBreakerIsIdempotent) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  graph.createEdge(n0, n1, nullptr);
  graph.createEdge(n1, n2, nullptr);
  graph.createEdge(n2, n0, nullptr);

  EXPECT_EQ(1u, TLoopBreaker::run(graph));
  //Already acyclic once the back edge is disabled: nothing left to cut.
  EXPECT_EQ(0u, TLoopBreaker::run(graph));
}

TEST_F(TGraphTests, testClockPropagatorLinearChainHasNoCrossing) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n1, n2, nullptr);
  auto clk = graph.createClock("clk", nullptr, n0);

  graph.createCaptureEdge(n0, n2);

  TLoopBreaker::run(graph);
  TClockPropagator::run(graph);

  EXPECT_TRUE(graph.getEdge(e0)->hasFromClock(clk));
  EXPECT_TRUE(graph.getEdge(e1)->hasFromClock(clk));
  EXPECT_TRUE(graph.getEdge(e0)->hasToClock(clk));
  EXPECT_TRUE(graph.getEdge(e1)->hasToClock(clk));
  EXPECT_FALSE(graph.getEdge(e0)->hasClockCrossing());
  EXPECT_FALSE(graph.getEdge(e1)->hasClockCrossing());
}

TEST_F(TGraphTests, testClockPropagatorFlagsMergeOfTwoClockDomains) {
  TGraph graph;
  //a --clkA--> merge <--clkB-- b, merge -> downstream.
  auto a = graph.createNode(nullptr);
  auto b = graph.createNode(nullptr);
  auto merge = graph.createNode(nullptr);
  auto downstream = graph.createNode(nullptr);
  auto eA = graph.createEdge(a, merge, nullptr);
  auto eB = graph.createEdge(b, merge, nullptr);
  auto eOut = graph.createEdge(merge, downstream, nullptr);

  auto clkA = graph.createClock("clkA", nullptr, a);
  auto clkB = graph.createClock("clkB", nullptr, b);

  graph.createCaptureEdge(merge, downstream);

  TLoopBreaker::run(graph);
  TClockPropagator::run(graph);

  //Each incoming edge only carries its own clock from the source side.
  EXPECT_TRUE(graph.getEdge(eA)->hasFromClock(clkA));
  EXPECT_FALSE(graph.getEdge(eA)->hasFromClock(clkB));
  EXPECT_TRUE(graph.getEdge(eB)->hasFromClock(clkB));
  EXPECT_FALSE(graph.getEdge(eB)->hasFromClock(clkA));

  //The capture source sees both clocks, so both propagate back from downstream.
  EXPECT_TRUE(graph.getEdge(eA)->hasToClock(clkA));
  EXPECT_TRUE(graph.getEdge(eA)->hasToClock(clkB));
  EXPECT_TRUE(graph.getEdge(eB)->hasToClock(clkA));
  EXPECT_TRUE(graph.getEdge(eB)->hasToClock(clkB));

  //Flagged: this is exactly the CDC-relevant point, two domains converging.
  EXPECT_TRUE(graph.getEdge(eA)->hasClockCrossing());
  EXPECT_TRUE(graph.getEdge(eB)->hasClockCrossing());

  //Downstream of the merge there's a single path carrying both clocks -
  //no further divergence, so this edge is not flagged even though it
  //carries multiple clock domains.
  EXPECT_TRUE(graph.getEdge(eOut)->hasFromClock(clkA));
  EXPECT_TRUE(graph.getEdge(eOut)->hasFromClock(clkB));
  EXPECT_FALSE(graph.getEdge(eOut)->hasClockCrossing());
}

TEST_F(TGraphTests, testClockPropagatorRejectsCyclesWithoutCuttingEdges) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  graph.createEdge(n0, n1, nullptr);
  graph.createEdge(n1, n0, nullptr);
  EXPECT_THROW(TClockPropagator::run(graph), std::logic_error);
  for (const auto& edge : graph.getEdgesStorage()) EXPECT_FALSE(edge.isDisabled());
  EXPECT_EQ(1u, TLoopBreaker::run(graph));
}

TEST_F(TGraphTests, testClockPropagatorSkipsRemovedNodeSlots) {
  TGraph graph;
  auto root = graph.createNode(nullptr);
  auto removed = graph.createNode(nullptr);
  auto sink = graph.createNode(nullptr);
  auto incoming = graph.createEdge(root, removed, nullptr);
  auto outgoing = graph.createEdge(removed, sink, nullptr);
  auto surviving = graph.createEdge(root, sink, nullptr);
  auto clock = graph.createClock("clk", nullptr, root);
  graph.createCaptureEdge(root, sink);
  TClockPropagator::run(graph);
  ASSERT_TRUE(graph.removeNode(removed));
  EXPECT_EQ(3u, graph.getNodesStorage().size());
  EXPECT_EQ(2u, graph.getNodesCount());
  EXPECT_NO_THROW(TClockPropagator::run(graph));
  EXPECT_FALSE(graph.isNodeValid(removed));
  EXPECT_FALSE(graph.isEdgeValid(incoming));
  EXPECT_FALSE(graph.isEdgeValid(outgoing));
  EXPECT_EQ(1u, graph.getEdgesCount());
  EXPECT_TRUE(graph.getEdge(surviving)->hasFromClock(clock));
  EXPECT_TRUE(graph.getEdge(surviving)->hasToClock(clock));
  EXPECT_FALSE(graph.getEdge(surviving)->hasClockCrossing());
}

TEST_F(TGraphTests, testClockPropagatorRegisterTransfer) {
  for (bool sameClock : {true, false}) {
    TGraph graph;
    auto launch = graph.createNode(nullptr);
    auto q = graph.createNode(nullptr);
    auto logic = graph.createNode(nullptr);
    auto d = graph.createNode(nullptr);
    auto capture = sameClock ? launch : graph.createNode(nullptr);
    graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
    auto cq = graph.createEdge(launch, q, nullptr);
    auto data0 = graph.createEdge(q, logic, nullptr);
    auto data1 = graph.createEdge(logic, d, nullptr);
    auto a = graph.createClock("a", nullptr, launch);
    auto b = sameClock ? a : graph.createClock("b", nullptr, capture);
    graph.createCaptureEdge(capture, d);
    TClockPropagator::run(graph);
    for (auto eid : {data0, data1}) {
      EXPECT_TRUE(graph.getEdge(eid)->hasFromClock(a));
      EXPECT_TRUE(graph.getEdge(eid)->hasToClock(b));
      EXPECT_EQ(!sameClock, graph.getEdge(eid)->hasClockCrossing());
    }
    //Backward propagation stops at Q and cannot enter its clock network.
    EXPECT_TRUE(graph.getEdge(cq)->getToClocks().none());
  }
}

TEST_F(TGraphTests, testCaptureSourceIncludesIncomingClocksAndOwnSeed) {
  TGraph graph;
  auto root = graph.createNode(nullptr);
  auto clock = graph.createNode(nullptr);
  auto driver = graph.createNode(nullptr);
  auto data = graph.createNode(nullptr);
  graph.createEdge(root, clock, nullptr);
  auto signal = graph.createEdge(driver, data, nullptr);
  auto a = graph.createClock("a", nullptr, root);
  auto b = graph.createClock("b", nullptr, clock);
  graph.createCaptureEdge(clock, data);
  //C has no ordinary outgoing edges, so its domain must include both
  //incoming FromClocks and its own direct seed.
  TClockPropagator::run(graph);
  EXPECT_TRUE(graph.getEdge(signal)->hasToClock(a));
  EXPECT_TRUE(graph.getEdge(signal)->hasToClock(b));
  EXPECT_EQ(2u, graph.getEdge(signal)->getToClocks().count());
}

TEST_F(TGraphTests, testCaptureFanoutAndRecomputation) {
  TGraph graph;
  auto q = graph.createNode(nullptr);
  auto logic = graph.createNode(nullptr);
  auto d0 = graph.createNode(nullptr);
  auto d1 = graph.createNode(nullptr);
  auto c0 = graph.createNode(nullptr);
  auto c1 = graph.createNode(nullptr);
  graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
  auto shared = graph.createEdge(q, logic, nullptr);
  auto branch0 = graph.createEdge(logic, d0, nullptr);
  auto branch1 = graph.createEdge(logic, d1, nullptr);
  auto a = graph.createClock("a", nullptr, c0);
  auto b = graph.createClock("b", nullptr, c1);
  auto capture0 = graph.createCaptureEdge(c0, d0);
  graph.createCaptureEdge(c1, d1);
  TClockPropagator::run(graph);
  EXPECT_TRUE(graph.getEdge(shared)->hasToClock(a));
  EXPECT_TRUE(graph.getEdge(shared)->hasToClock(b));
  EXPECT_TRUE(graph.getEdge(branch0)->hasToClock(a));
  EXPECT_FALSE(graph.getEdge(branch0)->hasToClock(b));
  EXPECT_TRUE(graph.getEdge(branch1)->hasToClock(b));
  EXPECT_FALSE(graph.getEdge(branch1)->hasToClock(a));

  graph.removeCaptureEdge(capture0);
  graph.getEdge(branch1)->setDisabled(true);
  TClockPropagator::run(graph);
  EXPECT_TRUE(graph.getEdge(shared)->getToClocks().none());
  EXPECT_TRUE(graph.getEdge(branch0)->getToClocks().none());
  EXPECT_TRUE(graph.getEdge(branch1)->getToClocks().none());
  EXPECT_TRUE(graph.getEdge(branch1)->getFromClocks().none());
  //New clocks resize masks and subsequent runs use the new seed set.
  auto c = graph.createClock("c", nullptr, c1);
  graph.getEdge(branch1)->setDisabled(false);
  TClockPropagator::run(graph);
  EXPECT_FALSE(graph.getEdge(shared)->hasToClock(a));
  EXPECT_TRUE(graph.getEdge(shared)->hasToClock(b));
  EXPECT_TRUE(graph.getEdge(shared)->hasToClock(c));
}

TEST_F(TGraphTests, testClockPropagatorRejectsUnsupportedLatchAndRemovedSeed) {
  TGraph graph;
  auto node = graph.createNode(nullptr);
  graph.getNode(node)->setCellKind(TNode::CellKind::Latch);
  EXPECT_THROW(TClockPropagator::run(graph), std::logic_error);
  graph.getNode(node)->setCellKind(TNode::CellKind::Unknown);
  graph.createClock("clk", nullptr, node);
  graph.removeNode(node);
  EXPECT_THROW(TClockPropagator::run(graph), std::logic_error);
}

TEST_F(TGraphTests, testClockPropagatorSucceedsAfterLoopBreaking) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n1, n0, nullptr);
  auto clk = graph.createClock("clk", nullptr, n0);

  TLoopBreaker::run(graph);
  EXPECT_NO_THROW(TClockPropagator::run(graph));

  //The back edge got disabled by the loop breaker; the clock should still
  //resolve on whichever edge survived enabled.
  const bool resolvedOnAnEnabledEdge =
    (!graph.getEdge(e0)->isDisabled() && graph.getEdge(e0)->hasFromClock(clk)) ||
    (!graph.getEdge(e1)->isDisabled() && graph.getEdge(e1)->hasFromClock(clk));
  EXPECT_TRUE(resolvedOnAnEnabledEdge);
}

TEST_F(TGraphTests, testMissingClockInformationIsNotACrossing) {
  TGraph graph;
  auto source = graph.createNode(nullptr);
  auto target = graph.createNode(nullptr);
  auto eid = graph.createEdge(source, target, nullptr);
  EXPECT_TRUE(graph.getEdge(eid)->hasNoLaunchClock());
  EXPECT_TRUE(graph.getEdge(eid)->hasNoCaptureClock());
  EXPECT_FALSE(graph.getEdge(eid)->hasClockCrossing());
  graph.createClock("a", nullptr, source);
  graph.createClock("b", nullptr, target);
  struct Case {
    unsigned launch;
    unsigned capture;
    bool crossing;
  };
  for (auto test : {Case{0, 0, false}, Case{1, 0, false}, Case{0, 2, false},
                    Case{1, 1, false}, Case{1, 2, true}, Case{1, 3, true}, Case{3, 3, false}}) {
    SCOPED_TRACE(::testing::Message() << "launch=" << test.launch << " capture=" << test.capture);
    auto* edge = graph.getEdge(eid);
    edge->setFromClocks(TEdge::ClockMask(2, test.launch));
    edge->setToClocks(TEdge::ClockMask(2, test.capture));
    edge->setClockGateControlClocks(TEdge::ClockMask(2, test.capture));
    EXPECT_EQ(test.launch == 0, edge->hasNoLaunchClock());
    EXPECT_EQ(test.capture == 0, edge->hasNoCaptureClock());
    EXPECT_EQ(test.crossing, edge->hasClockCrossing());
    EXPECT_EQ(test.crossing, edge->hasClockGateControlCrossing());
  }
}

TEST_F(TGraphTests, testCaptureKindSurvivesGrowthAndResetsOnReuse) {
  TGraph graph;
  auto clock = graph.createNode(nullptr);
  auto enable = graph.createNode(nullptr);
  auto control = graph.createCaptureEdge(clock, enable, TCaptureEdge::Kind::ClockGateControl);
  for (size_t i = 0; i < 100; ++i) graph.createCaptureEdge(clock, enable);
  EXPECT_EQ(TCaptureEdge::Kind::ClockGateControl, graph.getCaptureEdge(control)->getKind());
  EXPECT_EQ(TCaptureEdge::Kind::Data, graph.getCaptureEdge(control + 1)->getKind());
  EXPECT_NO_THROW(TClockPropagator::run(graph));
  ASSERT_TRUE(graph.removeCaptureEdge(control));
  ASSERT_EQ(control, graph.createCaptureEdge(clock, enable));
  EXPECT_EQ(TCaptureEdge::Kind::Data, graph.getCaptureEdge(control)->getKind());
  EXPECT_NO_THROW(TClockPropagator::run(graph));
  ASSERT_TRUE(graph.removeCaptureEdge(control));
  ASSERT_EQ(control, graph.createCaptureEdge(clock, enable, TCaptureEdge::Kind::ClockGateControl));
  EXPECT_EQ(TCaptureEdge::Kind::ClockGateControl, graph.getCaptureEdge(control)->getKind());
}

TEST_F(TGraphTests, testSeparateClockAndDataViewsAreOrderIndependent) {
  // One physical graph: gated-clock Q feeds the gate control and a data
  // endpoint. A second domain also drives that shared control/data cone.
  struct Arc { std::size_t from; std::size_t to; TEdge::Kind kind; };
  using Kind = TEdge::Kind;
  const std::array<Arc, 10> arcs {{{0, 3, Kind::Combinational},
    {3, 5, Kind::State}, {0, 6, Kind::ClockPass}, {5, 6, Kind::DataOnly},
    {6, 7, Kind::State}, {1, 2, Kind::State}, {2, 8, Kind::Combinational},
    {7, 8, Kind::Combinational}, {8, 4, Kind::Combinational}, {8, 9, Kind::Combinational}}};
  std::vector<std::array<TEdge::ClockMask, 3>> reference;
  std::mt19937 random(718);
  for (unsigned trial = 0; trial < 40; ++trial) {
    TGraph graph;
    std::array<TGraph::NodeId, 10> nodes;
    std::array<unsigned, 10> creation;
    std::iota(creation.begin(), creation.end(), 0);
    if (trial) std::shuffle(creation.begin(), creation.end(), random);
    for (auto n : creation) nodes[n] = graph.createNode(nullptr);
    auto classify = [&](unsigned n, TNode::CellKind cell, TNode::PinRole pin) {
      graph.getNode(nodes[n])->setCellKind(cell);
      graph.getNode(nodes[n])->setPinRole(pin);
    };
    classify(2, TNode::CellKind::FlipFlop, TNode::PinRole::DataOutput);
    classify(7, TNode::CellKind::FlipFlop, TNode::PinRole::DataOutput);
    classify(3, TNode::CellKind::Latch, TNode::PinRole::Gate);
    classify(4, TNode::CellKind::Latch, TNode::PinRole::DataInput);
    classify(5, TNode::CellKind::Latch, TNode::PinRole::DataOutput);
    if (trial) std::shuffle(creation.begin(), creation.end(), random);
    std::array<TGraph::EdgeId, 10> edges;
    for (auto i : creation) {
      edges[i] = graph.createEdge(nodes[arcs[i].from], nodes[arcs[i].to], nullptr);
      graph.getEdge(edges[i])->setKind(arcs[i].kind);
    }
    auto a = graph.createClock("a", nullptr, nodes[0]);
    auto b = graph.createClock("b", nullptr, nodes[1]);
    auto control = graph.createCaptureEdge(nodes[0], nodes[4], TCaptureEdge::Kind::ClockGateControl);
    graph.createCaptureEdge(nodes[0], nodes[9]);
    auto result = TClockPropagator::run(graph);
    EXPECT_EQ(1u, result.clockOrigins[nodes[6]].count());
    EXPECT_TRUE(result.clockOrigins[nodes[6]].test(a));
    for (auto n : {2, 5, 7}) EXPECT_TRUE(result.clockOrigins[nodes[n]].none());
    EXPECT_TRUE(graph.getEdge(edges[6])->hasFromClock(b));
    EXPECT_TRUE(graph.getEdge(edges[6])->hasToClock(a));
    EXPECT_TRUE(graph.getEdge(edges[6])->hasClockCrossing());
    EXPECT_TRUE(graph.getEdge(edges[6])->hasClockGateControlCrossing());
    EXPECT_TRUE(graph.getEdge(edges[7])->hasFromClock(a));
    EXPECT_FALSE(graph.getEdge(edges[7])->hasClockCrossing());
    EXPECT_FALSE(graph.getEdge(edges[7])->hasClockGateControlCrossing());
    // State boundaries stop both backward views.
    for (auto i : {1, 4, 5}) {
      EXPECT_TRUE(graph.getEdge(edges[i])->getToClocks().none());
      EXPECT_TRUE(graph.getEdge(edges[i])->getClockGateControlClocks().none());
    }
    for (unsigned i = 0; i < edges.size(); ++i) {
      const auto* e = graph.getEdge(edges[i]);
      std::array<TEdge::ClockMask, 3> masks {e->getFromClocks(), e->getToClocks(), e->getClockGateControlClocks()};
      if (!trial) reference.push_back(masks);
      else EXPECT_EQ(reference[i], masks) << "trial " << trial << " arc " << i;
      EXPECT_FALSE(e->isDisabled());
    }
    // Repeated runs clear stale control results, honor disabled edges, and
    // restore the exact masks after the caller re-enables its edge.
    graph.getEdge(edges[2])->setDisabled(true);
    result = TClockPropagator::run(graph);
    EXPECT_TRUE(result.clockOrigins[nodes[6]].none());
    EXPECT_TRUE(graph.getEdge(edges[2])->getFromClocks().none());
    EXPECT_TRUE(graph.getEdge(edges[2])->getToClocks().none());
    EXPECT_TRUE(graph.getEdge(edges[2])->getClockGateControlClocks().none());
    graph.getEdge(edges[2])->setDisabled(false);
    TClockPropagator::run(graph);
    for (unsigned i = 0; i < edges.size(); ++i) {
      const auto* e = graph.getEdge(edges[i]);
      EXPECT_EQ(reference[i][0], e->getFromClocks());
      EXPECT_EQ(reference[i][1], e->getToClocks());
      EXPECT_EQ(reference[i][2], e->getClockGateControlClocks());
    }
    graph.removeCaptureEdge(control);
    graph.getNode(nodes[4])->setCellKind(TNode::CellKind::Unknown);
    TClockPropagator::run(graph);
    for (const auto& e : graph.getEdgesStorage()) EXPECT_TRUE(e.getClockGateControlClocks().none());
  }
}

TEST_F(TGraphTests, testClockFeedbackWithQAlsoUsedAsGeneratedClock) {
  std::mt19937 random(573);
  std::vector<std::array<TEdge::ClockMask, 3>> reference;
  for (unsigned trial = 0; trial < 32; ++trial) {
    TGraph graph;
    std::array<unsigned, 5> creation {0, 1, 2, 3, 4};
    if (trial) std::shuffle(creation.begin(), creation.end(), random);
    std::array<TGraph::NodeId, 5> nodes;
    for (auto n : creation) nodes[n] = graph.createNode(nullptr);
    auto root = nodes[0];
    auto c = nodes[1];
    auto q = nodes[2];
    auto logic = nodes[3];
    auto d = nodes[4];
    graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
    const std::array<std::pair<TGraph::NodeId, TGraph::NodeId>, 5> arcs {{{root, c},
      {c, q}, {q, logic}, {logic, c}, {root, d}}};
    std::array<unsigned, 5> order {0, 1, 2, 3, 4};
    if (trial) std::shuffle(order.begin(), order.end(), random);
    std::array<TGraph::EdgeId, 5> edges;
    for (auto i : order) edges[i] = graph.createEdge(arcs[i].first, arcs[i].second, nullptr);
    auto a = graph.createClock("a", nullptr, root);
    // The physical cycle C->Q->logic->C closes through a state arc.
    // Q is also a direct capture clock, so its State arc is admitted.
    // Its feedback into C remains control because C has a root waveform.
    graph.createCaptureEdge(q, d);
    graph.createCaptureEdge(c, logic);
    auto result = TClockPropagator::run(graph);
    EXPECT_EQ(1u, result.clockOrigins[q].count());
    EXPECT_TRUE(result.clockOrigins[q].test(a));
    EXPECT_EQ(std::vector<TEdge::EdgeId>({edges[1]}), result.generatedClockStateArcs);
    EXPECT_TRUE(graph.getEdge(edges[4])->hasToClock(a));
    EXPECT_TRUE(result.clockOrigins[c].test(a));
    EXPECT_TRUE(graph.getEdge(edges[2])->hasFromClock(a));
    EXPECT_TRUE(graph.getEdge(edges[2])->hasToClock(a));
    EXPECT_TRUE(graph.getEdge(edges[3])->hasFromClock(a));
    EXPECT_TRUE(graph.getEdge(edges[1])->getToClocks().none());
    for (unsigned i = 0; i < edges.size(); ++i) {
      const auto* e = graph.getEdge(edges[i]);
      std::array<TEdge::ClockMask, 3> masks {e->getFromClocks(), e->getToClocks(), e->getClockGateControlClocks()};
      if (!trial) reference.push_back(masks);
      else EXPECT_EQ(reference[i], masks) << "trial " << trial << " arc " << i;
      EXPECT_FALSE(e->isDisabled());
    }
  }
}

TEST_F(TGraphTests, testDataViewCycleAndSelfLoopAreRejectedWithoutMutation) {
  for (bool self : {false, true}) {
    TGraph graph;
    auto a = graph.createNode(nullptr);
    auto b = self ? a : graph.createNode(nullptr);
    auto e = graph.createEdge(a, b, nullptr);
    graph.getEdge(e)->setKind(TEdge::Kind::DataOnly);
    if (!self) graph.createEdge(b, a, nullptr);
    for (unsigned run = 0; run < 2; ++run) {
      EXPECT_THROW(TClockPropagator::run(graph), std::logic_error);
      for (const auto& edge : graph.getEdgesStorage()) EXPECT_FALSE(edge.isDisabled());
    }
    graph.getEdge(e)->setDisabled(true);
    EXPECT_NO_THROW(TClockPropagator::run(graph));
  }
}

TEST_F(TGraphTests, testEdgeAnalysisMetadataResetsAndGrows) {
  TGraph graph;
  auto a = graph.createNode(nullptr);
  auto b = graph.createNode(nullptr);
  auto e = graph.createEdge(a, b, nullptr);
  auto c = graph.createClock("a", nullptr, a);
  graph.getEdge(e)->setKind(TEdge::Kind::DataOnly);
  TEdge::ClockMask mask(1);
  mask.set(c);
  graph.getEdge(e)->setClockGateControlClocks(mask);
  graph.createClock("b", nullptr, b);
  EXPECT_EQ(2u, graph.getEdge(e)->getClockGateControlClocks().size());
  EXPECT_TRUE(graph.getEdge(e)->getClockGateControlClocks().test(c));
  graph.removeEdge(e);
  EXPECT_EQ(e, graph.createEdge(a, b, nullptr));
  EXPECT_EQ(TEdge::Kind::Combinational, graph.getEdge(e)->getKind());
  EXPECT_EQ(2u, graph.getEdge(e)->getClockGateControlClocks().size());
  EXPECT_TRUE(graph.getEdge(e)->getClockGateControlClocks().none());
}

TEST_F(TGraphTests, testGeneratedClockChainsAndDataFanout) {
  for (bool foreign : {false, true}) {
    for (unsigned stages : {1u, 2u}) {
      TGraph graph;
      auto a = graph.createNode(nullptr);
      auto b = graph.createNode(nullptr);
      auto ca = graph.createClock("a", nullptr, a);
      auto cb = graph.createClock("b", nullptr, b);
      auto last = a;
      std::vector<TEdge::EdgeId> states;
      for (unsigned i = 0; i < stages; ++i) {
        auto c = graph.createNode(nullptr);
        auto q = graph.createNode(nullptr);
        graph.getNode(c)->setCellKind(TNode::CellKind::FlipFlop);
        graph.getNode(c)->setPinRole(TNode::PinRole::Clock);
        graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
        graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
        graph.createEdge(last, c, nullptr);
        auto state = graph.createEdge(c, q, nullptr);
        graph.getEdge(state)->setKind(TEdge::Kind::State);
        states.push_back(state);
        last = q;
      }
      auto c = graph.createNode(nullptr);
      graph.getNode(c)->setCellKind(TNode::CellKind::FlipFlop);
      graph.getNode(c)->setPinRole(TNode::PinRole::Clock);
      graph.createEdge(last, c, nullptr);
      auto d = graph.createNode(nullptr);
      auto data = graph.createEdge(foreign ? b : a, d, nullptr);
      graph.createCaptureEdge(c, d);
      auto fanout = graph.createNode(nullptr);
      auto fanoutEdge = graph.createEdge(last, fanout, nullptr);
      graph.createCaptureEdge(a, fanout);
      auto result = TClockPropagator::run(graph);
      EXPECT_EQ(states, result.generatedClockStateArcs);
      EXPECT_TRUE(result.unclockedFlopClockPins.empty());
      EXPECT_EQ(1u, result.clockOrigins[c].count());
      EXPECT_TRUE(result.clockOrigins[c].test(ca));
      EXPECT_TRUE(result.clockOrigins[fanout].none());
      EXPECT_TRUE(graph.getEdge(data)->hasFromClock(foreign ? cb : ca));
      EXPECT_TRUE(graph.getEdge(data)->hasToClock(ca));
      EXPECT_EQ(foreign, graph.getEdge(data)->hasClockCrossing());
      EXPECT_EQ(foreign ? 1 : 0, std::count_if(graph.getEdgesStorage().begin(),
        graph.getEdgesStorage().end(), [](const auto& edge) { return edge.hasClockCrossing(); }));
      EXPECT_TRUE(graph.getEdge(fanoutEdge)->hasFromClock(ca));
      EXPECT_TRUE(graph.getEdge(fanoutEdge)->hasToClock(ca));
      EXPECT_FALSE(graph.getEdge(fanoutEdge)->hasClockCrossing());
      graph.getEdge(states.front())->setDisabled(true);
      result = TClockPropagator::run(graph);
      EXPECT_TRUE(result.clockOrigins[c].none());
      EXPECT_NE(result.unclockedFlopClockPins.end(),
        std::find(result.unclockedFlopClockPins.begin(), result.unclockedFlopClockPins.end(), c));
    }
  }
}

TEST_F(TGraphTests, testPlainGateFeedbackAndForeignEnableAreOrderIndependent) {
  std::mt19937 random(219);
  for (bool foreign : {false, true}) {
    for (unsigned trial = 0; trial < 40; ++trial) {
      TGraph graph;
      std::array<TGraph::NodeId, 12> n;
      for (auto& node : n) node = graph.createNode(nullptr);
      auto a = graph.createClock("a", nullptr, n[0]);
      auto b = graph.createClock("b", nullptr, n[1]);
      for (unsigned i : {2u, 4u, 6u, 8u}) {
        graph.getNode(n[i])->setCellKind(TNode::CellKind::FlipFlop);
        graph.getNode(n[i])->setPinRole(TNode::PinRole::Clock);
        graph.getNode(n[i+1])->setCellKind(TNode::CellKind::FlipFlop);
        graph.getNode(n[i+1])->setPinRole(TNode::PinRole::DataOutput);
      }
      std::vector<std::pair<unsigned, unsigned>> arcs {{0,2}, {2,3}, {0,4}, {4,5},
        {foreign ? 1u : 0u,10}, {3,10}, {5,10}, {7,10}, {10,6}, {6,7}, {10,8}, {8,9}, {7,11}};
      std::vector<unsigned> order(arcs.size());
      std::iota(order.begin(), order.end(), 0);
      std::shuffle(order.begin(), order.end(), random);
      for (auto i : order) {
        auto id = graph.createEdge(n[arcs[i].first], n[arcs[i].second], nullptr);
        if (graph.getNode(n[arcs[i].second])->isFlopDataOutput()) graph.getEdge(id)->setKind(TEdge::Kind::State);
      }
      graph.createCaptureEdge(n[8], n[11]);
      auto result = TClockPropagator::run(graph);
      EXPECT_TRUE(result.generatedClockStateArcs.empty());
      EXPECT_TRUE(result.unclockedFlopClockPins.empty());
      for (unsigned i : {2u, 4u, 6u, 8u}) {
        EXPECT_EQ(1u, result.clockOrigins[n[i]].count());
        EXPECT_TRUE(result.clockOrigins[n[i]].test(foreign && i >= 6 ? b : a));
      }
      for (const auto& edge : graph.getEdgesStorage()) {
        EXPECT_FALSE(edge.isDisabled());
        EXPECT_FALSE(edge.hasClockCrossing());
      }
    }
  }
}

TEST_F(TGraphTests, testAmbiguousAndUnseededGeneratedClocksAreReported) {
  TGraph graph;
  auto a = graph.createNode(nullptr);
  graph.createClock("a", nullptr, a);
  auto merge = graph.createNode(nullptr);
  graph.getNode(merge)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(merge)->setPinRole(TNode::PinRole::Clock);
  for (unsigned i = 0; i < 2; ++i) {
    auto q = graph.createNode(nullptr);
    graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
    graph.getEdge(graph.createEdge(a, q, nullptr))->setKind(TEdge::Kind::State);
    graph.createEdge(q, merge, nullptr);
  }
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(std::vector<TEdge::NodeId>({merge}), result.unclockedFlopClockPins);
  EXPECT_TRUE(result.generatedClockStateArcs.empty());
  EXPECT_TRUE(result.clockOrigins[merge].none());
}

TEST_F(TGraphTests, testGeneratedWaveformStateCycleUsesUnionFixedPoint) {
  for (bool reverse : {false, true}) {
    TGraph graph;
    auto a = graph.createNode(nullptr);
    auto b = graph.createNode(nullptr);
    auto q = graph.createNode(nullptr);
    auto c = graph.createNode(nullptr);
    graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
    graph.getNode(c)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(c)->setPinRole(TNode::PinRole::Clock);
    auto ca = graph.createClock("a", nullptr, a);
    auto cb = graph.createClock("b", nullptr, b);
    std::vector<std::pair<TGraph::NodeId, TGraph::NodeId>> arcs {{a,q}, {b,q}, {c,q}, {q,c}};
    if (reverse) std::reverse(arcs.begin(), arcs.end());
    for (auto [from, to] : arcs) {
      auto id = graph.createEdge(from, to, nullptr);
      if (to == q) graph.getEdge(id)->setKind(TEdge::Kind::State);
    }
    auto result = TClockPropagator::run(graph);
    EXPECT_EQ(3u, result.generatedClockStateArcs.size());
    EXPECT_TRUE(result.unclockedFlopClockPins.empty());
    EXPECT_EQ(2u, result.clockOrigins[c].count());
    EXPECT_TRUE(result.clockOrigins[c].test(ca));
    EXPECT_TRUE(result.clockOrigins[c].test(cb));
    EXPECT_EQ(result.clockOrigins[c], result.clockOrigins[q]);
    for (const auto& edge : graph.getEdgesStorage()) EXPECT_FALSE(edge.isDisabled());
  }
}

TEST_F(TGraphTests, testPlainForeignEnableAndUnclockedReportLifecycle) {
  TGraph graph;
  auto a = graph.createNode(nullptr);
  auto b = graph.createNode(nullptr);
  auto enableQ = graph.createNode(nullptr);
  auto gate = graph.createNode(nullptr);
  auto middleC = graph.createNode(nullptr);
  auto middleQ = graph.createNode(nullptr);
  auto sinkD = graph.createNode(nullptr);
  graph.createClock("a", nullptr, a);
  auto cb = graph.createClock("b", nullptr, b);
  for (auto q : {enableQ, middleQ}) {
    graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
  }
  graph.getNode(middleC)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(middleC)->setPinRole(TNode::PinRole::Clock);
  auto enableState = graph.createEdge(a, enableQ, nullptr);
  graph.getEdge(enableState)->setKind(TEdge::Kind::State);
  auto clockPass = graph.createEdge(b, gate, nullptr);
  graph.createEdge(enableQ, gate, nullptr);
  graph.createEdge(gate, middleC, nullptr);
  auto state = graph.createEdge(middleC, middleQ, nullptr);
  graph.getEdge(state)->setKind(TEdge::Kind::State);
  auto data = graph.createEdge(middleQ, sinkD, nullptr);
  graph.createCaptureEdge(b, sinkD);
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(1u, result.clockOrigins[middleC].count());
  EXPECT_TRUE(result.clockOrigins[middleC].test(cb));
  EXPECT_TRUE(result.generatedClockStateArcs.empty());
  EXPECT_EQ(std::vector<TEdge::EdgeId>({enableState}), result.controlStateArcs);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_TRUE(graph.getEdge(data)->hasFromClock(cb));
  EXPECT_TRUE(graph.getEdge(data)->hasToClock(cb));
  EXPECT_FALSE(graph.getEdge(data)->hasClockCrossing());
  // With the known clock path disabled, the sole remaining Q is inferred.
  graph.getEdge(clockPass)->setDisabled(true);
  result = TClockPropagator::run(graph);
  EXPECT_EQ(1u, result.generatedClockStateArcs.size());
  EXPECT_TRUE(result.controlStateArcs.empty());
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_TRUE(result.clockOrigins[middleC].test(0));
  // Removing its seed association leaves the inferred path visible but unseeded.
  graph.removeEdge(result.generatedClockStateArcs.front());
  result = TClockPropagator::run(graph);
  EXPECT_EQ(std::vector<TEdge::NodeId>({middleC}), result.unclockedFlopClockPins);
  EXPECT_TRUE(result.generatedClockStateArcs.empty());
  auto snapshot = result;
  graph.removeNode(middleC);
  result = TClockPropagator::run(graph);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_TRUE(result.clockOrigins[middleC].none());
  EXPECT_EQ(std::vector<TEdge::NodeId>({middleC}), snapshot.unclockedFlopClockPins);
}

TEST_F(TGraphTests, testMuxGeneratedDataAndStateSelectAreOrderIndependent) {
  std::mt19937 random(467);
  for (bool stateSelect : {false, true}) {
    for (unsigned trial = 0; trial < 40; ++trial) {
      TGraph graph;
      std::array<TGraph::NodeId, 9> n;
      for (auto& node : n) node = graph.createNode(nullptr);
      auto a = graph.createClock("a", nullptr, n[0]);
      auto b = graph.createClock("b", nullptr, n[1]);
      auto c = graph.createClock("c", nullptr, n[2]);
      graph.getNode(n[3])->setCellKind(TNode::CellKind::FlipFlop);
      graph.getNode(n[3])->setPinRole(TNode::PinRole::DataOutput);
      graph.getNode(n[7])->setCellKind(TNode::CellKind::FlipFlop);
      graph.getNode(n[7])->setPinRole(TNode::PinRole::Clock);
      struct Arc { unsigned from, to; TEdge::Kind kind; };
      using K = TEdge::Kind;
      std::vector<Arc> arcs {{stateSelect ? 2u : 0u,3,K::State},
        {stateSelect ? 0u : 3u,4,K::Combinational}, {1,5,K::Combinational},
        {4,6,K::ClockPass}, {5,6,K::ClockPass},
        {stateSelect ? 3u : 2u,6,K::DataOnly}, {6,7,K::Combinational},
        {1,8,K::Combinational}};
      std::shuffle(arcs.begin(), arcs.end(), random);
      TEdge::EdgeId state = TGraph::kInvalidEdge, data = TGraph::kInvalidEdge;
      for (const auto& arc : arcs) {
        auto id = graph.createEdge(n[arc.from], n[arc.to], nullptr);
        graph.getEdge(id)->setKind(arc.kind);
        if (arc.kind == K::State) state = id;
        if (arc.to == 8) data = id;
      }
      graph.createCaptureEdge(n[7], n[8]);
      auto result = TClockPropagator::run(graph);
      EXPECT_EQ(2u, result.clockOrigins[n[7]].count());
      EXPECT_TRUE(result.clockOrigins[n[7]].test(a));
      EXPECT_TRUE(result.clockOrigins[n[7]].test(b));
      EXPECT_FALSE(result.clockOrigins[n[7]].test(c));
      EXPECT_TRUE(result.unclockedFlopClockPins.empty());
      EXPECT_EQ(stateSelect ? std::vector<TEdge::EdgeId>{} : std::vector<TEdge::EdgeId>{state},
        result.generatedClockStateArcs);
      EXPECT_EQ(stateSelect ? std::vector<TEdge::EdgeId>{state} : std::vector<TEdge::EdgeId>{},
        result.controlStateArcs);
      EXPECT_TRUE(graph.getEdge(data)->hasFromClock(b));
      EXPECT_EQ(2u, graph.getEdge(data)->getToClocks().count());
      EXPECT_TRUE(graph.getEdge(data)->hasToClock(a));
      EXPECT_TRUE(graph.getEdge(data)->hasToClock(b));
      EXPECT_TRUE(graph.getEdge(data)->hasClockCrossing());
      auto again = TClockPropagator::run(graph);
      EXPECT_EQ(result.clockOrigins, again.clockOrigins);
      EXPECT_EQ(result.controlStateArcs, again.controlStateArcs);
      graph.getEdge(state)->setDisabled(true);
      EXPECT_TRUE(TClockPropagator::run(graph).controlStateArcs.empty());
      graph.removeEdge(state);
      EXPECT_TRUE(TClockPropagator::run(graph).controlStateArcs.empty());
    }
  }
}

TEST_F(TGraphTests, testMuxAdmitsTwoDividersAndReportsSharedControlUse) {
  TGraph graph;
  auto a = graph.createNode(nullptr), b = graph.createNode(nullptr);
  auto qa = graph.createNode(nullptr), qb = graph.createNode(nullptr);
  auto mux = graph.createNode(nullptr), gate = graph.createNode(nullptr);
  auto c = graph.createNode(nullptr);
  auto ca = graph.createClock("a", nullptr, a);
  auto cb = graph.createClock("b", nullptr, b);
  for (auto q : {qa, qb}) {
    graph.getNode(q)->setCellKind(TNode::CellKind::FlipFlop);
    graph.getNode(q)->setPinRole(TNode::PinRole::DataOutput);
  }
  graph.getNode(c)->setCellKind(TNode::CellKind::FlipFlop);
  graph.getNode(c)->setPinRole(TNode::PinRole::Clock);
  auto sa = graph.createEdge(a, qa, nullptr), sb = graph.createEdge(b, qb, nullptr);
  for (auto state : {sa, sb}) graph.getEdge(state)->setKind(TEdge::Kind::State);
  for (auto q : {qa, qb}) graph.getEdge(graph.createEdge(q, mux, nullptr))->setKind(TEdge::Kind::ClockPass);
  graph.createEdge(a, gate, nullptr);
  graph.createEdge(qa, gate, nullptr);
  graph.createEdge(mux, c, nullptr);
  graph.createCaptureEdge(gate, graph.createNode(nullptr));
  auto result = TClockPropagator::run(graph);
  EXPECT_EQ(std::vector<TEdge::EdgeId>({sa, sb}), result.generatedClockStateArcs);
  EXPECT_EQ(std::vector<TEdge::EdgeId>({sa}), result.controlStateArcs);
  EXPECT_TRUE(result.unclockedFlopClockPins.empty());
  EXPECT_EQ(2u, result.clockOrigins[c].count());
  EXPECT_TRUE(result.clockOrigins[c].test(ca));
  EXPECT_TRUE(result.clockOrigins[c].test(cb));
  EXPECT_EQ(1u, result.clockOrigins[gate].count());
  EXPECT_TRUE(result.clockOrigins[gate].test(ca));
}
