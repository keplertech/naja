// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include <tbb/global_control.h>

#include "DNL.h"
#include "ConstantPropagation.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLLibraryTruthTables.h"
#include "NLUniverse.h"
#include "NLName.h"
#include "SNLCapnP.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLEquipotential.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLTruthTable.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "RemoveLoadlessLogic.h"

#include "SNLNetlist0.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "."
#endif

static size_t getDumpSize(const std::filesystem::path& dump_path) {
  std::error_code ec;
  size_t total = 0;
  auto interface_path = dump_path / std::string(SNLCapnP::InterfaceName);
  auto implementation_path = dump_path / std::string(SNLCapnP::ImplementationName);
  auto interface_size = std::filesystem::file_size(interface_path, ec);
  if (!ec) {
    total += interface_size;
  }
  ec.clear();
  auto implementation_size = std::filesystem::file_size(implementation_path, ec);
  if (!ec) {
    total += implementation_size;
  }
  return total;
}

static void BM_CreateNetlist0(benchmark::State& state) {
  for (auto _ : state) {
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    SNLNetlist0::create(db);
    benchmark::DoNotOptimize(SNLNetlist0::getTop());
    NLUniverse::get()->destroy();
  }
}
BENCHMARK(BM_CreateNetlist0);

static void BM_CreateInstances(benchmark::State& state) {
  const int instance_count = static_cast<int>(state.range(0));
  for (auto _ : state) {
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    auto lib = NLLibrary::create(db, NLName("BENCH"));
    auto model = SNLDesign::create(lib, NLName("MODEL"));
    SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
    SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
    auto top = SNLDesign::create(lib, NLName("TOP"));

    for (int i = 0; i < instance_count; ++i) {
      std::string name = "u" + std::to_string(i);
      SNLInstance::create(top, model, NLName(name));
    }

    benchmark::DoNotOptimize(top);
    NLUniverse::get()->destroy();
  }
  state.SetItemsProcessed(state.iterations() * instance_count);
}
BENCHMARK(BM_CreateInstances)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_TraversalInstances(benchmark::State& state) {
  const int instance_count = static_cast<int>(state.range(0));
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto lib = NLLibrary::create(db, NLName("BENCH_TRAVERSAL"));
  auto model = SNLDesign::create(lib, NLName("MODEL"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
  auto top = SNLDesign::create(lib, NLName("TOP"));

  for (int i = 0; i < instance_count; ++i) {
    std::string name = "u" + std::to_string(i);
    SNLInstance::create(top, model, NLName(name));
  }

  for (auto _ : state) {
    size_t count = 0;
    for (auto instance: top->getInstances()) {
      benchmark::DoNotOptimize(instance);
      ++count;
    }
    benchmark::DoNotOptimize(count);
  }

  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * instance_count);
}
BENCHMARK(BM_TraversalInstances)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_LoadVerilogFile(benchmark::State& state, const char* filename) {
  const std::filesystem::path benchmark_path =
    std::filesystem::path(SNL_VRL_BENCHMARKS_PATH) / filename;
  const auto file_size = std::filesystem::file_size(benchmark_path);

  for (auto _ : state) {
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    auto lib = NLLibrary::create(db, NLName("VRL"));
    SNLVRLConstructor constructor(lib);
    constructor.parse(benchmark_path);
    constructor.setFirstPass(false);
    constructor.parse(benchmark_path);
    benchmark::DoNotOptimize(lib);
    NLUniverse::get()->destroy();
  }

  state.SetBytesProcessed(state.iterations() * file_size);
}
BENCHMARK_CAPTURE(BM_LoadVerilogFile, Gates0, "test_gates0.v");
BENCHMARK_CAPTURE(BM_LoadVerilogFile, FullAdder, "fulladder.v");
BENCHMARK_CAPTURE(BM_LoadVerilogFile, Gates2, "test_gates2.v");
BENCHMARK_CAPTURE(BM_LoadVerilogFile, LargeHierGates, "large_hier_gates.v");

static void BM_HierarchyTraversal(benchmark::State& state) {
  const std::filesystem::path benchmark_path =
    std::filesystem::path(SNL_VRL_BENCHMARKS_PATH) / "large_hier_gates.v";
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto lib = NLLibrary::create(db, NLName("VRL_HIER"));
  SNLVRLConstructor constructor(lib);
  constructor.parse(benchmark_path);
  constructor.setFirstPass(false);
  constructor.parse(benchmark_path);
  auto top = SNLUtils::findTop(lib);

  for (auto _ : state) {
    SNLUtils::SortedDesigns sorted;
    SNLUtils::getDesignsSortedByHierarchicalLevel(top, sorted);
    benchmark::DoNotOptimize(sorted);
  }

  NLUniverse::get()->destroy();
}
BENCHMARK(BM_HierarchyTraversal);

// Reuse each model across many occurrences to exercise hierarchy expansion,
// rather than just increasing the number of distinct source designs.
static SNLScalarTerm* createFanoutHierarchy(int groups, int depth, bool independent=false) {
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto lib = NLLibrary::create(db, NLName("BENCH_CONNECTIVITY"));
  auto primitives = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMITIVES"));
  auto model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("LEAF"));
  auto input = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
  for (int level = 0; level < depth; ++level) {
    auto parent = SNLDesign::create(lib, NLName("LEVEL_" + std::to_string(level)));
    auto port = SNLScalarTerm::create(parent, SNLTerm::Direction::Input, NLName("i"));
    auto net = SNLScalarNet::create(parent, NLName("fanout"));
    port->setNet(net);
    const int children = level == 0 ? 8 : 1;
    for (int child = 0; child < children; ++child) {
      auto instance = SNLInstance::create(parent, model, NLName("u" + std::to_string(child)));
      instance->getInstTerm(input)->setNet(net);
    }
    model = parent;
    input = port;
  }
  auto top = SNLDesign::create(lib, NLName("TOP"));
  auto port = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("i"));
  auto net = SNLScalarNet::create(top, NLName("fanout"));
  port->setNet(net);
  for (int group = 0; group < groups; ++group) {
    if (independent and group != 0) {
      auto groupPort = SNLScalarTerm::create(
        top, SNLTerm::Direction::Input, NLName("i" + std::to_string(group)));
      net = SNLScalarNet::create(top, NLName("fanout" + std::to_string(group)));
      groupPort->setNet(net);
    }
    auto instance = SNLInstance::create(top, model, NLName("u" + std::to_string(group)));
    instance->getInstTerm(input)->setNet(net);
  }
  universe->setTopDesign(top);
  return port;
}

static void measureDNLConstruction(benchmark::State& state, bool independent) {
  const int groups = static_cast<int>(state.range(0));
  const int depth = static_cast<int>(state.range(1));
  // CPU simulation monitors serial work, not scheduler-dependent scaling.
  tbb::global_control serial(tbb::global_control::max_allowed_parallelism, 1);
  auto port = createFanoutHierarchy(groups, depth, independent);
  {
    auto check = naja::DNL::get();
    const auto isoID = check->getTop().getTerminalFromBitTerm(port).getIsoID();
    const auto& iso = check->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
    if (check->getLeaves().size() != static_cast<size_t>(groups * 8) or
        check->getNBterms() != static_cast<size_t>((independent ? groups : 1) + groups * (depth + 8)) or
        check->getDNLIsoDB().getNumNonEmptyIsos() != static_cast<size_t>(independent ? groups : 1) or
        iso.getDrivers().size() != 1 or
        iso.getReaders().size() != static_cast<size_t>(independent ? 8 : groups * 8)) {
      throw std::runtime_error("Unexpected expanded hierarchy size");
    }
    naja::DNL::destroy();
  }
  for (auto _ : state) {
    auto dnl = naja::DNL::get();
    benchmark::DoNotOptimize(dnl);
    naja::DNL::destroy();
  }
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * groups * 8);
}
static void BM_DNLConstruction(benchmark::State& state) {
  measureDNLConstruction(state, false);
}
BENCHMARK(BM_DNLConstruction)
  ->Args({16, 1})->Args({128, 1})->Args({1024, 1})
  ->Args({16, 4})->Args({128, 4})->Args({1024, 4});

static void BM_DNLIndependentNets(benchmark::State& state) {
  measureDNLConstruction(state, true);
}
BENCHMARK(BM_DNLIndependentNets)
  ->Args({16, 1})->Args({128, 1})->Args({1024, 1});

static void BM_Equipotential(benchmark::State& state) {
  const int groups = static_cast<int>(state.range(0));
  const int depth = static_cast<int>(state.range(1));
  auto port = createFanoutHierarchy(groups, depth);
  {
    SNLEquipotential check(port);
    if (check.getInstTermOccurrencesSet().size() != static_cast<size_t>(groups * 8) or
        check.getTermsSet().size() != 1) {
      throw std::runtime_error("Unexpected equipotential size");
    }
  }
  for (auto _ : state) {
    SNLEquipotential equipotential(port);
    benchmark::DoNotOptimize(equipotential);
  }
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * groups * 8);
}
BENCHMARK(BM_Equipotential)
  ->Args({16, 1})->Args({128, 1})->Args({1024, 1})
  ->Args({16, 4})->Args({128, 4})->Args({1024, 4});

static void BM_AssignChain(benchmark::State& state) {
  const int length = static_cast<int>(state.range(0));
  const bool reverse = state.range(1) != 0;
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto lib = NLLibrary::create(db, NLName("BENCH_ASSIGNS"));
  auto top = SNLDesign::create(lib, NLName("TOP"));
  universe->setTopDesign(top);
  auto input = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("i"));
  auto output = SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("o"));
  auto net = SNLScalarNet::create(top, NLName("n0"));
  input->setNet(net);
  for (int stage = 0; stage < length; ++stage) {
    auto next = SNLScalarNet::create(top, NLName("n" + std::to_string(stage + 1)));
    auto instance = SNLInstance::create(top, NLDB0::getAssign(), NLName("u" + std::to_string(stage)));
    instance->getInstTerm(NLDB0::getAssignInput())->setNet(net);
    instance->getInstTerm(NLDB0::getAssignOutput())->setNet(next);
    net = next;
  }
  output->setNet(net);
  auto start = reverse ? output : input;
  {
    SNLEquipotential check(start, SNLEquipotential::Mode::TraverseAssigns);
    if (check.getTermsSet().size() != 2 or
        not check.getTermsSet().count(input) or not check.getTermsSet().count(output) or
        not check.getInstTermOccurrencesSet().empty()) {
      throw std::runtime_error("Assign chain did not reach both endpoints");
    }
  }
  for (auto _ : state) {
    SNLEquipotential equipotential(start, SNLEquipotential::Mode::TraverseAssigns);
    benchmark::DoNotOptimize(equipotential);
  }
  universe->destroy();
  state.SetItemsProcessed(state.iterations() * length);
}
BENCHMARK(BM_AssignChain)
  ->Args({64, 0})->Args({256, 0})->Args({1024, 0})
  ->Args({64, 1})->Args({256, 1})->Args({1024, 1});

// Two inverter chains share an input; only the first chain drives a top output.
static SNLScalarTerm* createLogicChains(int length, bool constant) {
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto primitives = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMITIVES"));
  auto model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("INV"));
  auto input = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
  auto output = SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
  SNLDesignModeling::setTruthTable(
    model, SNLTruthTable(1, 1, SNLTruthTable::fullDependencies(1)));
  if (constant) {
    for (int value = 0; value < 2; ++value) {
      auto driver = SNLDesign::create(
        primitives, SNLDesign::Type::Primitive, NLName("LOGIC" + std::to_string(value)));
      SNLScalarTerm::create(driver, SNLTerm::Direction::Output, NLName("o"));
      SNLDesignModeling::setTruthTable(
        driver, SNLTruthTable(0, value, SNLTruthTable::fullDependencies(0)));
    }
  }
  NLLibraryTruthTables::construct(primitives);
  auto lib = NLLibrary::create(db, NLName("BENCH_OPTIMIZATION"));
  auto top = SNLDesign::create(lib, NLName("TOP"));
  universe->setTopDesign(top);
  auto source = SNLScalarNet::create(top, NLName("source"));
  if (constant) {
    source->setType(SNLNet::Type::Assign0);
  } else {
    auto port = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("i"));
    port->setNet(source);
  }
  auto result = SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("o"));
  for (int chain = 0; chain < 2; ++chain) {
    auto net = source;
    for (int stage = 0; stage < length; ++stage) {
      const auto suffix = std::to_string(chain) + "_" + std::to_string(stage);
      auto next = SNLScalarNet::create(top, NLName("n" + suffix));
      auto instance = SNLInstance::create(top, model, NLName("u" + suffix));
      instance->getInstTerm(input)->setNet(net);
      instance->getInstTerm(output)->setNet(next);
      net = next;
    }
    if (chain == 0) {
      result->setNet(net);
    }
  }
  return result;
}

static void BM_ConstantPropagation(benchmark::State& state) {
  const int length = static_cast<int>(state.range(0));
  tbb::global_control serial(tbb::global_control::max_allowed_parallelism, 1);
  auto output = createLogicChains(length, true);
  for (auto _ : state) {
    naja::NAJA_OPT::ConstantPropagation propagation;
    propagation.setTruthTableEngine(true);
    propagation.run();
  }
  if (not (length % 2 ? output->getNet()->isConstant1() : output->getNet()->isConstant0())) {
    throw std::runtime_error("Constant propagation did not resolve the chain output");
  }
  naja::DNL::destroy();
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * length * 2);
}
// Mutating passes run once per fresh fixture, never repeatedly on optimized input.
BENCHMARK(BM_ConstantPropagation)->Arg(63)->Arg(256)->Arg(1023)->Iterations(1);

static void BM_DeadLogicElimination(benchmark::State& state) {
  const int length = static_cast<int>(state.range(0));
  tbb::global_control serial(tbb::global_control::max_allowed_parallelism, 1);
  auto output = createLogicChains(length, false);
  auto top = NLUniverse::get()->getTopDesign();
  for (auto _ : state) {
    naja::NAJA_OPT::LoadlessLogicRemover remover;
    remover.process();
  }
  if (top->getInstances().size() != static_cast<size_t>(length) or
      output->getNet()->getInstTerms().size() != 1) {
    throw std::runtime_error("Dead logic elimination did not preserve the live chain");
  }
  for (int stage = 0; stage < length; ++stage) {
    if (not top->getInstance(NLName("u0_" + std::to_string(stage))) or
        top->getInstance(NLName("u1_" + std::to_string(stage)))) {
      throw std::runtime_error("Unexpected surviving logic instance");
    }
  }
  naja::DNL::destroy();
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * length * 2);
}
BENCHMARK(BM_DeadLogicElimination)->Arg(64)->Arg(256)->Arg(1024)->Iterations(1);

static void BM_DesignClone(benchmark::State& state) {
  const int length = static_cast<int>(state.range(0));
  createLogicChains(length, false);
  auto top = NLUniverse::get()->getTopDesign();
  {
    auto copy = top->clone();
    std::string reason;
    if (not top->deepCompare(copy, reason, NLDesign::CompareType::IgnoreIDAndName)) {
      throw std::runtime_error("Design clone differs from source: " + reason);
    }
    copy->destroy();
  }
  for (auto _ : state) {
    auto copy = top->clone();
    benchmark::DoNotOptimize(copy);
    copy->destroy();
  }
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * length * 2);
}
BENCHMARK(BM_DesignClone)->Arg(64)->Arg(256)->Arg(1024);

struct InstanceLookup {
  NLName name;
  NLID::DesignObjectID id;
  SNLInstance* expected;
};

static void measureInstanceLookup(benchmark::State& state, bool byName) {
  const int count = static_cast<int>(state.range(0));
  const bool missing = state.range(1) != 0;
  createLogicChains(count / 2, false);
  auto top = NLUniverse::get()->getTopDesign();
  std::vector<SNLInstance*> instances;
  for (auto instance: top->getInstances()) {
    instances.push_back(instance);
  }
  if (instances.size() != static_cast<size_t>(count)) {
    throw std::runtime_error("Unexpected lookup fixture size");
  }
  std::vector<InstanceLookup> queries;
  queries.reserve(instances.size());
  // Odd stride permutes the power-of-two sizes, avoiding declaration-order access.
  for (size_t i = 0; i < instances.size(); ++i) {
    auto index = (i * 17 + 11) % instances.size();
    auto instance = instances[index];
    auto name = missing ? NLName("missing_" + std::to_string(index)) : instance->getName();
    auto id = missing ? static_cast<NLID::DesignObjectID>(count + index) : instance->getID();
    auto expected = missing ? nullptr : instance;
    if (top->getInstance(name) != expected or top->getInstance(id) != expected) {
      throw std::runtime_error("Instance lookup returned an unexpected result");
    }
    queries.push_back({name, id, expected});
  }
  if (byName) {
    for (auto _ : state) {
      for (const auto& query: queries) {
        benchmark::DoNotOptimize(top->getInstance(query.name));
      }
    }
  } else {
    for (auto _ : state) {
      for (const auto& query: queries) {
        benchmark::DoNotOptimize(top->getInstance(query.id));
      }
    }
  }
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_InstanceLookupByName(benchmark::State& state) {
  measureInstanceLookup(state, true);
}
BENCHMARK(BM_InstanceLookupByName)
  ->Args({128, 0})->Args({1024, 0})->Args({8192, 0})
  ->Args({128, 1})->Args({1024, 1})->Args({8192, 1});

static void BM_InstanceLookupByID(benchmark::State& state) {
  measureInstanceLookup(state, false);
}
BENCHMARK(BM_InstanceLookupByID)
  ->Args({128, 0})->Args({1024, 0})->Args({8192, 0})
  ->Args({128, 1})->Args({1024, 1})->Args({8192, 1});

struct TerminalReconnect {
  SNLInstTerm* term;
  SNLBitNet* original;
  SNLScalarNet* destination;
  size_t originalSize;
};

static void BM_TerminalReconnect(benchmark::State& state) {
  const int count = static_cast<int>(state.range(0));
  const bool shared = state.range(1) != 0;
  createLogicChains(count / 2, false);
  auto top = NLUniverse::get()->getTopDesign();
  auto hub = SNLScalarNet::create(top, NLName("hub"));
  std::vector<TerminalReconnect> connections;
  connections.reserve(static_cast<size_t>(count));
  for (auto instance: top->getInstances()) {
    auto term = instance->getInstTerm(instance->getModel()->getScalarTerm(NLName("i")));
    auto destination = shared ? hub : SNLScalarNet::create(
      top, NLName("destination_" + std::to_string(connections.size())));
    connections.push_back({term, term->getNet(), destination, term->getNet()->getInstTerms().size()});
  }
  if (connections.size() != static_cast<size_t>(count)) {
    throw std::runtime_error("Unexpected reconnection fixture size");
  }
  // Validate the connected state before timing, including the high-fanout case.
  for (const auto& connection: connections) {
    connection.term->setNet(connection.destination);
  }
  for (const auto& connection: connections) {
    if (connection.term->getNet() != connection.destination or
        connection.destination->getInstTerms().size() != static_cast<size_t>(shared ? count : 1)) {
      throw std::runtime_error("Terminal reconnection produced incorrect fanout");
    }
  }
  for (const auto& connection: connections) {
    connection.term->setNet(connection.original);
  }
  for (auto _ : state) {
    for (const auto& connection: connections) {
      connection.term->setNet(nullptr);
    }
    for (const auto& connection: connections) {
      connection.term->setNet(connection.destination);
    }
    for (const auto& connection: connections) {
      connection.term->setNet(connection.original);
    }
  }
  for (const auto& connection: connections) {
    if (connection.term->getNet() != connection.original or
        connection.original->getInstTerms().size() != connection.originalSize or
        not connection.destination->getInstTerms().empty()) {
      throw std::runtime_error("Terminal reconnection did not restore original connectivity");
    }
  }
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * count * 3);
}
BENCHMARK(BM_TerminalReconnect)
  ->Args({128, 0})->Args({1024, 0})->Args({8192, 0})
  ->Args({128, 1})->Args({1024, 1})->Args({8192, 1});

static void BM_CapnPSerialize(benchmark::State& state) {
  const auto base_path = std::filesystem::temp_directory_path() / "snl_bench_dump";
  size_t run_id = 0;
  size_t bytes = 0;

  for (auto _ : state) {
    state.PauseTiming();
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    SNLNetlist0::create(db);
    auto out_path = base_path / ("run_" + std::to_string(run_id++));
    std::error_code ec;
    std::filesystem::remove_all(out_path, ec);
    std::filesystem::create_directories(out_path, ec);
    state.ResumeTiming();

    SNLCapnP::dump(db, out_path);
    state.PauseTiming();
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
    state.ResumeTiming();

    auto loaded = SNLCapnP::load(out_path);
    benchmark::DoNotOptimize(loaded);

    state.PauseTiming();
    bytes = getDumpSize(out_path);
    std::filesystem::remove_all(out_path, ec);
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
    state.ResumeTiming();
  }
  if (bytes > 0) {
    state.SetBytesProcessed(state.iterations() * bytes);
  }
}
BENCHMARK(BM_CapnPSerialize);

BENCHMARK_MAIN();
