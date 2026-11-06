// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "TConstantPropagator.h"
#include <deque>
#include <map>
#include <set>
#include <tuple>
#include <numeric>
#include <limits>
#include "TSNLGraphConstructor.h"
#include "NLDB0.h"
#include "SNLBitNet.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLBusTerm.h"
#include "SNLDesignModeling.h"

namespace naja::TG {
using namespace naja::NL;
using Value = TConstantPropagator::Value;
namespace {
// Each input position is local to this output's explicit bit dependencies.
// Exhaustive cofactors are bounded by the existing finite truth table itself.
struct Evaluation {
  Value value{Value::Unknown};
  std::vector<bool> live;
  bool transparent{false};
};
// Reduced Boolean cofactors for compact generic truth-table families. Variables
// represent unknown input bits; known values become terminals. This avoids
// expanding a wide arithmetic/table-select model into an exponential LUT.
class Cofactors {
public:
  using Bit = size_t;
  using Word = std::vector<Bit>; // least significant bit first
  struct Node {
    size_t variable;
    Bit low, high;
  };
  std::vector<Node> nodes{{size_t(-1), 0, 0}, {size_t(-1), 1, 1}};
  Bit variable(size_t index) { return make(index, 0, 1); }
  Bit make(size_t variable, Bit low, Bit high) {
    if (low == high) {
      return low;
    }
    auto key = std::make_tuple(variable, low, high);
    auto [it, added] = unique.emplace(key, nodes.size());
    if (added) {
      nodes.push_back({variable, low, high});
    }
    return it->second;
  }
  Bit apply(unsigned function, Bit a, Bit b) {
    if (a < 2 && b < 2) {
      return (function >> (a + 2 * b)) & 1;
    }
    auto key = std::make_tuple(function, a, b);
    auto found = cache.find(key);
    if (found != cache.end()) {
      return found->second;
    }
    auto variable = std::min(nodes[a].variable, nodes[b].variable);
    auto branch = [&](Bit id, bool high) { return nodes[id].variable == variable ? (high ? nodes[id].high : nodes[id].low) : id; };
    auto low = apply(function, branch(a, false), branch(b, false));
    auto high = apply(function, branch(a, true), branch(b, true));
    auto value = make(variable, low, high);
    cache.emplace(key, value);
    return value;
  }
  Bit And(Bit a, Bit b) { return apply(8, a, b); }
  Bit Or(Bit a, Bit b) { return apply(14, a, b); }
  Bit Xor(Bit a, Bit b) { return apply(6, a, b); }
  Bit Not(Bit a) { return Xor(a, 1); }
  Bit mux(Bit s, Bit a, Bit b) { return Or(And(Not(s), a), And(s, b)); }
  Word mux(Bit s, const Word& a, const Word& b) {
    Word result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
      result[i] = mux(s, a[i], b[i]);
    }
    return result;
  }
  Word add(const Word& a, const Word& b, bool subtract = false) {
    Word result(a.size());
    Bit carry = subtract;
    for (size_t i = 0; i < a.size(); ++i) {
      auto right = subtract ? Not(b[i]) : b[i];
      auto both = Xor(a[i], right);
      result[i] = Xor(both, carry);
      carry = Or(And(a[i], right), And(both, carry));
    }
    return result;
  }
  Bit less(const Word& a, const Word& b) {
    Bit result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
      result = Or(And(Not(a[i]), b[i]), And(Not(Xor(a[i], b[i])), result));
    }
    return result;
  }
  Bit equal(const Word& word, int64_t value, bool sign = false) {
    if (word.size() < 64) {
      if (sign) {
        auto limit = int64_t{1} << (word.size() - 1);
        if (value < -limit || value >= limit) {
          return 0;
        }
      } else if (value < 0 || uint64_t(value) >= (uint64_t{1} << word.size())) {
        return 0;
      }
    } else if (!sign && value < 0) {
      return 0;
    }
    Bit result = 1;
    for (size_t i = 0; i < word.size(); ++i) {
      bool expected = i < 64 ? bool((uint64_t(value) >> i) & 1) : value < 0;
      result = And(result, expected ? word[i] : Not(word[i]));
    }
    return result;
  }

private:
  std::map<std::tuple<size_t, Bit, Bit>, Bit> unique;
  std::map<std::tuple<unsigned, Bit, Bit>, Bit> cache;
};

Evaluation evaluateGeneric(const SNLTruthTable& table, const std::vector<Value>& inputs) {
  using G = SNLTruthTable::GenericType;
  using Word = Cofactors::Word;
  Cofactors f;
  // Interleave operand bits, least significant first, for arithmetic cofactors.
  // Variable identities are local input positions; ranks affect storage only.
  std::vector<size_t> rank(inputs.size());
  std::iota(rank.begin(), rank.end(), 0);
  auto kind = table.getGenericType();
  size_t width = table.isWordArithmetic() ? table.getWordMetadata().width : kind == G::DIVMOD ? table.getDivModWidth()
                                                                                              : 0;
  if (width && kind != G::SHIFT) {
    for (size_t i = 0; i < width; ++i) {
      rank[width - 1 - i] = 2 * i;
      rank[2 * width - 1 - i] = 2 * i + 1;
    }
  }
  std::vector<size_t> bits;
  for (size_t i = 0; i < inputs.size(); ++i) {
    bits.push_back(inputs[i] == Value::Unknown ? f.variable(rank[i]) : size_t(inputs[i] == Value::One));
  }
  auto word = [&](size_t start, size_t count) {
    Word result;
    for (size_t i = count; i > 0; --i) {
      result.push_back(bits.at(start + i - 1));
    }
    return result;
  };
  auto unknown = [&]() { return f.variable(inputs.size()); };
  size_t output = 0;
  if (kind == G::TABLE_SELECT || kind == G::ROM) {
    auto count = kind == G::ROM ? table.getRomMetadata().addressSize : table.getTableSelectAddressSize();
    auto depth = kind == G::ROM ? table.getRomMetadata().depth : table.getTableSelectDepth();
    auto address = word(0, count);
    for (size_t row = 0; row < depth; ++row) {
      auto value = kind == G::ROM ? size_t(table.getRomMetadata().column[row]) : bits[count + row];
      output = f.Or(output, f.And(f.equal(address, row), value));
    }
  } else if (kind == G::VECTOR_SELECT) {
    const auto& m = table.getVectorSelectMetadata();
    auto address = word(m.width, m.addressSize);
    output = unknown();
    for (size_t k = 0; k < m.width; ++k) {
      output = f.mux(f.equal(address, int64_t(k) - m.outputBit, m.isSigned), output, bits[m.width - 1 - k]);
    }
  } else if (kind == G::VECTOR_UPDATE) {
    const auto& m = table.getVectorUpdateMetadata();
    auto address = word(1 + m.updateWidth, m.addressSize);
    output = bits.front();
    for (size_t k = 0; k < m.updateWidth; ++k) {
      output = f.mux(f.And(bits.back(), f.equal(address, int64_t(m.outputBit) - k, m.isSigned)), output, bits[m.updateWidth - k]);
    }
  } else if (table.isWordArithmetic()) {
    const auto& m = table.getWordMetadata();
    auto a = word(0, m.width);
    auto b = word(m.width, kind == G::SHIFT ? m.countWidth : m.width);
    Word result(m.width, 0);
    if (kind == G::ADD_SUB) {
      result = f.add(a, b, m.subtract);
    } else if (kind == G::MULTIPLY) {
      for (size_t i = 0; i < m.width; ++i) {
        Word partial(m.width, 0);
        for (size_t j = 0; j + i < m.width; ++j) {
          partial[j + i] = f.And(a[j], b[i]);
        }
        result = f.add(result, partial);
      }
    } else {
      result = a;
      for (size_t i = 0; i < b.size(); ++i) {
        Word shifted(m.width, m.right && m.arithmetic ? a.back() : 0);
        size_t distance = i < std::numeric_limits<size_t>::digits ? size_t{1} << i : m.width;
        if (distance < m.width) {
          for (size_t j = 0; j < m.width - distance; ++j) {
            if (m.right) {
              shifted[j] = result[j + distance];
            } else {
              shifted[j + distance] = result[j];
            }
          }
        }
        result = f.mux(b[i], result, shifted);
      }
    }
    output = result.at(m.outputBit);
  } else if (kind == G::DIVMOD) {
    auto a = word(0, width), b = word(width, width);
    auto signA = table.isDivModSigned() ? a.back() : 0;
    auto signB = table.isDivModSigned() ? b.back() : 0;
    Word zero(width, 0);
    a = f.mux(signA, a, f.add(zero, a, true));
    b = f.mux(signB, b, f.add(zero, b, true));
    Word remainder(width + 1, 0), divisor = b, quotient(width, 0);
    divisor.push_back(0);
    for (size_t i = width; i > 0; --i) {
      for (size_t j = width; j > 0; --j) {
        remainder[j] = remainder[j - 1];
      }
      remainder[0] = a[i - 1];
      auto take = f.Not(f.less(remainder, divisor));
      quotient[i - 1] = take;
      remainder = f.mux(take, remainder, f.add(remainder, divisor, true));
    }
    remainder.resize(width);
    bool quotientResult = table.getDivModResult() == SNLTruthTable::DivModResult::QUOTIENT;
    auto result = quotientResult ? quotient : remainder;
    auto sign = quotientResult ? f.Xor(signA, signB) : signA;
    result = f.mux(sign, result, f.add(zero, result, true));
    output = f.mux(f.equal(b, 0), result.at(table.getDivModOutputBit()), unknown());
  } else {
    throw NLException("Unsupported generic truth table in timing constant propagation"); // LCOV_EXCL_LINE All supported GenericType values are handled above.
  }
  Evaluation result{output < 2 ? (output ? Value::One : Value::Zero) : Value::Unknown,
                    std::vector<bool>(inputs.size(), false), false};
  std::vector<size_t> pending{output};
  std::set<size_t> seen, support;
  while (!pending.empty()) {
    auto id = pending.back();
    pending.pop_back();
    if (id < 2 || !seen.insert(id).second) {
      continue;
    }
    support.insert(f.nodes[id].variable);
    pending.push_back(f.nodes[id].low);
    pending.push_back(f.nodes[id].high);
  }
  for (size_t i = 0; i < inputs.size(); ++i) {
    result.live[i] = support.count(rank[i]);
  }
  result.transparent = support.size() == 1 && !support.count(inputs.size());
  return result;
}

Evaluation evaluate(const SNLTruthTable& table, const std::vector<Value>& inputs) {
  Evaluation result{Value::Unknown, std::vector<bool>(inputs.size(), true), false};
  for (size_t i = 0; i < inputs.size(); ++i) {
    result.live[i] = inputs[i] == Value::Unknown;
  }
  if (!table.isInitialized() || table.size() != inputs.size()) {
    return result;
  }
  SNLTruthTable::ConstantInputs constants;
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (inputs[i] != Value::Unknown) {
      constants.emplace_back(i, inputs[i] == Value::One);
      result.live[i] = false;
    }
  }
  if (table.isGeneric()) {
    // Associative tables have a linear fast path; compact word/table families
    // use reduced Boolean cofactors rather than materializing their truth table.
    using G = SNLTruthTable::GenericType;
    auto kind = table.getGenericType();
    if (kind != G::AND && kind != G::NAND && kind != G::OR && kind != G::NOR && kind != G::XOR && kind != G::XNOR) {
      return evaluateGeneric(table, inputs);
    }
    auto canonical = SNLTruthTable(table.size(), kind, SNLTruthTable::fullDependencies(table.size()));
    auto reduced = canonical.getReducedWithConstants(constants);
    if (reduced.size() == 0) {
      result.value = reduced.all1() ? Value::One : Value::Zero;
      std::fill(result.live.begin(), result.live.end(), false);
    } else {
      result.transparent = reduced.size() == 1;
    }
    return result;
  }
  bool any0 = false, any1 = false;
  size_t liveCount = 0;
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (inputs[i] != Value::Unknown) {
      continue;
    }
    bool depends = false;
    for (size_t assignment = 0; assignment < table.bits().size(); ++assignment) {
      bool allowed = true;
      for (const auto& [bit, value] : constants) {
        if (bool((assignment >> bit) & 1) != value) {
          allowed = false;
          break;
        }
      }
      if (!allowed) {
        continue;
      }
      if (table.bits().bit(assignment) != table.bits().bit(assignment ^ (size_t{1} << i))) {
        depends = true;
        break;
      }
    }
    result.live[i] = depends;
    liveCount += depends;
  }
  for (size_t assignment = 0; assignment < table.bits().size(); ++assignment) {
    bool allowed = true;
    for (const auto& [bit, value] : constants) {
      if (bool((assignment >> bit) & 1) != value) {
        allowed = false;
        break;
      }
    }
    if (allowed) {
      if (table.bits().bit(assignment)) {
        any1 = true;
      } else {
        any0 = true;
      }
    }
  }
  if (!any0 || !any1) {
    result.value = any1 ? Value::One : Value::Zero;
  }
  result.transparent = liveCount == 1;
  return result;
}
} // namespace
std::vector<Value> TConstantPropagator::run(TSNLGraphConstructor& constructor, const std::vector<Seed>& seeds) {
  auto& graph = constructor.getGraph();
  std::vector<Value> values(graph.getNodesStorage().size(), Value::Unknown);
  std::deque<size_t> pending;
  std::vector<bool> queued(values.size(), false), forced(values.size(), false);
  auto schedule = [&](size_t id) { if (!queued[id]) { pending.push_back(id); queued[id] = true; } };
  auto set = [&](size_t id, Value value) {
    if (value == Value::Unknown || forced[id] || values[id] == value) {
      return;
    }
    if (values[id] != Value::Unknown) {
      throw NLException("Conflicting constants in timing data view");
    }
    values[id] = value;
    schedule(id);
  };
  for (const auto& node : graph.getNodesStorage()) {
    const auto* proxy = static_cast<const TSNLNodeProxy*>(node.getProxy());
    auto* net = proxy->getOccurrence().getComponentBitNet();
    if (net && (net->getType().isAssign0() || net->getType().isAssign1() || net->getType().isSupply0() || net->getType().isSupply1())) {
      set(node.getIndex(), net->getType().isAssign1() || net->getType().isSupply1() ? Value::One : Value::Zero);
    }
  }
  for (const auto& [id, value] : seeds) {
    if (forced[id] && values[id] != value) {
      throw NLException("Conflicting reset inactive values");
    }
    values[id] = value;
    forced[id] = true;
    schedule(id);
  }
  for (const auto& edge : graph.getEdgesStorage()) {
    graph.getEdge(edge.getIndex())->setDataDisabled(false);
    graph.getEdge(edge.getIndex())->setDataTransparent(false);
  }
  // Occurrences without a constant input are never evaluated. Truth tables
  // are fetched lazily only when the first constant reaches an output.
  std::map<std::pair<const SNLDesign*, size_t>, SNLTruthTable> tables;
  auto output = [&](size_t id) {
    const auto* proxy = static_cast<const TSNLNodeProxy*>(graph.getNode(id)->getProxy());
    auto* pin = proxy->getOccurrence().getInstTerm();
    if (!pin) {
      return; // LCOV_EXCL_LINE Builder emits combinational drivers/arcs only on instance pins.
    }
    auto* model = pin->getInstance()->getModel();
    const bool db0 = NLDB0::isDB0Primitive(model);
    const bool rom = db0 && NLDB0::isMemory(model) && NLDB0::getMemorySignature(pin->getInstance()).writePorts == 0;
    if (!graph.getNode(id)->isCombinationalDriver() && !rom) {
      return;
    }
    std::vector<size_t> arcs;
    std::vector<Value> inputs;
    for (auto edgeId : graph.getNode(id)->getInEdges()) {
      auto* edge = graph.getEdge(edgeId);
      auto* arc = static_cast<const TSNLEdgeProxy*>(edge->getProxy());
      if (!edge->isAlive() || edge->isDisabled() || edge->getKind() == TEdge::Kind::State ||
          arc->getKind() == TSNLEdgeProxy::Kind::Connectivity) {
        continue;
      }
      arcs.push_back(edgeId);
      edge->setDataDisabled(values[edge->getSource()] != Value::Unknown);
    }
    if (!db0) {
      return; // Constant pins are pruned even when the function is uncovered.
    }
    auto key = std::make_pair(model, pin->getBitTerm()->getOrderID());
    auto found = tables.find(key);
    if (found == tables.end()) {
      found = tables.emplace(key, SNLDesignModeling::getTruthTable(key.first, key.second)).first;
    }
    auto table = rom ? SNLDesignModeling::getTruthTable(pin->getInstance(), key.second) : found->second;
    if (table.isGeneric()) {
      // Timing arc sets sort by (term ID, bit), whereas generic metadata uses
      // declaration order (MSB first). TABLE_SELECT puts address bits first,
      // followed by row 0, row 1, ... for this output bit.
      auto term = [&](size_t arc) {
        auto* source = static_cast<const TSNLNodeProxy*>(graph.getNode(graph.getEdge(arc)->getSource())->getProxy());
        return source->getOccurrence().getInstTerm()->getBitTerm();
      };
      std::sort(arcs.begin(), arcs.end(), [&](size_t a, size_t b) {
        auto* left = term(a);
        auto* right = term(b);
        if (table.getGenericType() == SNLTruthTable::GenericType::TABLE_SELECT) {
          auto address = NLDB0::getTableSelectAddress(model)->getID();
          bool la = left->getID() == address, ra = right->getID() == address;
          if (la != ra) {
            return la;
          }
          if (!la) {
            return left->getOrderID() > right->getOrderID();
          }
        }
        return left->getOrderID() < right->getOrderID();
      });
    }
    for (auto arc : arcs) {
      inputs.push_back(values[graph.getEdge(arc)->getSource()]);
    }
    auto evaluation = evaluate(table, inputs);
    for (size_t i = 0; i < arcs.size(); ++i) {
      // Canonical assign glue represents a wire in structural queries.
      graph.getEdge(arcs[i])->setDataDisabled(!NLDB0::isAssign(model) && !evaluation.live[i]);
      graph.getEdge(arcs[i])->setDataTransparent(evaluation.live[i] && evaluation.transparent);
    }
    set(id, evaluation.value);
  };
  for (const auto& node : graph.getNodesStorage()) {
    if (node.isCombinationalDriver() && node.getInEdges().empty()) {
      output(node.getIndex());
    }
  }
  while (!pending.empty()) {
    auto id = pending.front();
    pending.pop_front();
    queued[id] = false;
    for (auto edgeId : graph.getNode(id)->getOutEdges()) {
      auto* edge = graph.getEdge(edgeId);
      if (!edge->isAlive() || edge->isDisabled() || edge->getKind() == TEdge::Kind::State) {
        continue;
      }
      auto* arc = static_cast<const TSNLEdgeProxy*>(edge->getProxy());
      if (arc->getKind() == TSNLEdgeProxy::Kind::Connectivity) {
        // A multiply driven wire is constant only when every physical driver
        // agrees. One constant driver must not hide another unknown driver.
        auto target = edge->getTarget();
        auto value = values[id];
        for (auto incoming : graph.getNode(target)->getInEdges()) {
          auto* driver = graph.getEdge(incoming);
          auto* dependency = static_cast<const TSNLEdgeProxy*>(driver->getProxy());
          if (driver->isAlive() && !driver->isDisabled() && dependency->getKind() == TSNLEdgeProxy::Kind::Connectivity &&
              values[driver->getSource()] != value) {
            value = Value::Unknown;
            break;
          }
        }
        set(target, value);
      } else {
        output(edge->getTarget());
      }
    }
  }
  return values;
}
} // namespace naja::TG
