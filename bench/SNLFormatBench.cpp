// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLCapnP.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLEquipotential.h"
#include "SNLLibertyConstructor.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLSVConstructor.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"
#include "VHDLConstructor.h"

using namespace naja::NL;

namespace {

class BenchmarkDirectory {
 public:
  BenchmarkDirectory() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (size_t attempt = 0; ; ++attempt) {
      path = std::filesystem::temp_directory_path() /
        ("naja-format-bench-" + std::to_string(stamp) + "-" + std::to_string(attempt));
      if (std::filesystem::create_directory(path)) {
        break;
      }
    }
  }
  ~BenchmarkDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
  BenchmarkDirectory(const BenchmarkDirectory&) = delete;
  BenchmarkDirectory& operator=(const BenchmarkDirectory&) = delete;
  void write(const std::string& name, const std::string& source) const {
    std::ofstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output.open(path / name, std::ios::binary);
    output << source;
    output.close();
  }
  std::filesystem::path path;
};

NLLibrary* createLibrary(bool primitives=false) {
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  return NLLibrary::create(db, primitives ? NLLibrary::Type::Primitives : NLLibrary::Type::Standard,
    NLName("BENCH"));
}

void validateOutputs(SNLDesign* top, size_t width, int kind) {
  if (not top or top->getBitTerms().size() != width * 3 + (kind == 3 ? 1 : 0) or
      top->getInstances().empty()) {
    throw std::runtime_error("Unexpected elaborated design shape");
  }
  size_t blocks = 0;
  bool hasAdder = false;
  bool hasRegister = false;
  for (auto instance: top->getInstances()) {
    blocks += not instance->getModel()->isPrimitive();
    hasAdder |= NLDB0::isAddSub(instance->getModel()) or NLDB0::isFA(instance->getModel());
    hasRegister |= NLDB0::isDFF(instance->getModel());
  }
  if ((kind == 0 and blocks != width / 8) or
      (kind == 2 and not hasAdder) or (kind == 3 and not hasRegister)) {
    throw std::runtime_error("Elaboration omitted hierarchy, arithmetic, or registers");
  }
  auto output = top->getBusTerm(NLName("y"));
  if (not output or output->getBits().size() != width) {
    throw std::runtime_error("Unexpected elaborated output width");
  }
  for (auto bit: output->getBits()) {
    if (not bit->getNet()) {
      throw std::runtime_error("Elaborated output is disconnected");
    }
    bool driven = false;
    for (auto term: bit->getNet()->getInstTerms()) {
      driven |= term->getDirection() == SNLTerm::Direction::Output;
    }
    if (not driven) {
      throw std::runtime_error("Elaborated output has no driver");
    }
  }
}

// Kinds: repeated hierarchy, unpacked arrays, arithmetic, clocked registers.
std::string systemVerilogSource(int groups, int kind) {
  const int width = groups * 8;
  std::ostringstream source;
  if (kind == 0) {
    source << "module stage(input logic [7:0] a,b, output logic [7:0] y);"
      " assign y = a ^ b; endmodule\n";
  }
  source << "module top(input logic [" << width - 1 << ":0] a,b, "
    << (kind == 3 ? "input logic clk, " : "")
    << "output logic [" << width - 1 << ":0] y);\n";
  if (kind == 1) {
    source << "logic [7:0] words [0:" << groups - 1 << "];\n";
  }
  if (kind == 3) {
    source << "always_ff @(posedge clk) y <= a ^ b;\n";
  } else {
    source << "for (genvar i=0; i<" << groups << "; i++) begin: lanes\n";
    if (kind == 0) {
      source << "stage u(a[8*i+:8], b[8*i+:8], y[8*i+:8]);\n";
    } else if (kind == 1) {
      source << "assign words[i] = a[8*i+:8] ^ b[8*i+:8];"
        " assign y[8*i+:8] = words[i];\n";
    } else {
      source << "assign y[8*i+:8] = (a[8*i+:8] + b[8*i+:8]) ^ a[8*i+:8];\n";
    }
    source << "end\n";
  }
  source << "endmodule\n";
  return source.str();
}

std::string vhdlSource(int groups, int kind) {
  const std::string context = "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n";
  std::ostringstream source;
  if (kind == 0) {
    source << context << "entity stage is port(a,b: in std_logic_vector(7 downto 0);"
      " y: out std_logic_vector(7 downto 0)); end;"
      " architecture rtl of stage is begin y <= a xor b; end;\n";
  }
  source << context << "entity top is port(a,b: in std_logic_vector(" << groups * 8 - 1
    << " downto 0); " << (kind == 3 ? "clk: in std_logic; " : "")
    << "y: out std_logic_vector(" << groups * 8 - 1 << " downto 0)); end;\n"
    << "architecture rtl of top is\n";
  if (kind == 1) {
    source << "type words_t is array(0 to " << groups - 1
      << ") of std_logic_vector(7 downto 0); signal words: words_t;\n";
  }
  source << "begin\n";
  if (kind == 3) {
    source << "process(clk) begin if rising_edge(clk) then y <= a xor b; end if; end process;\n";
  } else {
    source << "lanes: for i in 0 to " << groups - 1 << " generate\n";
    if (kind == 0) {
      source << "u: entity work.stage port map(a(8*i+7 downto 8*i),"
        " b(8*i+7 downto 8*i), y(8*i+7 downto 8*i));\n";
    } else if (kind == 1) {
      source << "words(i) <= a(8*i+7 downto 8*i) xor b(8*i+7 downto 8*i);"
        " y(8*i+7 downto 8*i) <= words(i);\n";
    } else {
      source << "y(8*i+7 downto 8*i) <= std_logic_vector(unsigned(a(8*i+7 downto 8*i))"
        " + unsigned(b(8*i+7 downto 8*i))) xor a(8*i+7 downto 8*i);\n";
    }
    source << "end generate;\n";
  }
  source << "end;\n";
  return source.str();
}

std::string libertySource(int cells, int tableSize) {
  std::ostringstream source;
  source << "library(bench) { time_unit: \"1ns\"; capacitive_load_unit(1,pf);"
    " lu_table_template(delay) { variable_1: input_net_transition;"
    " variable_2: total_output_net_capacitance; }\n";
  for (int cell = 0; cell < cells; ++cell) {
    source << "cell(cell_" << cell << ") { pin(A) { direction: input; }"
      " pin(Y) { direction: output; function: \"!A\"; timing() { related_pin: \"A\";"
      " timing_sense: negative_unate; timing_type: combinational;\n";
    for (const auto* table: {"cell_rise", "cell_fall", "rise_transition", "fall_transition"}) {
      source << table << "(delay) { ";
      for (const auto* index: {"index_1", "index_2"}) {
        source << index << "(\"";
        for (int i = 1; i <= tableSize; ++i) {
          source << (i == 1 ? "" : ",") << "0." << i;
        }
        source << "\"); ";
      }
      source << "values(";
      for (int row = 0; row < tableSize; ++row) {
        source << (row ? "," : "") << "\"";
        for (int col = 0; col < tableSize; ++col) {
          source << (col ? "," : "") << "0.02";
        }
        source << "\"";
      }
      source << "); }\n";
    }
    source << "} } }\n";
  }
  source << "}\n";
  return source.str();
}

SNLDesign* createSnapshotDesign(int count) {
  auto library = createLibrary();
  auto primitives = NLLibrary::create(library->getDB(), NLLibrary::Type::Primitives, NLName("PRIMITIVES"));
  auto model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("buffer_cell"));
  auto a = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("a"));
  auto y = SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("y"));
  auto top = SNLDesign::create(library, NLName("top"));
  auto input = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("a"));
  auto output = SNLBusTerm::create(top, SNLTerm::Direction::Output, count - 1, 0, NLName("y"));
  auto inputNet = SNLScalarNet::create(top, NLName("input_net"));
  auto outputNet = SNLBusNet::create(top, count - 1, 0, NLName("output_net"));
  input->setNet(inputNet);
  output->setNet(outputNet);
  for (int i = 0; i < count; ++i) {
    auto instance = SNLInstance::create(top, model, NLName("u" + std::to_string(i)));
    instance->getInstTerm(a)->setNet(inputNet);
    instance->getInstTerm(y)->setNet(outputNet->getBit(i));
  }
  NLUniverse::get()->setTopDesign(top);
  return top;
}

void validateSnapshotDesign(NLDB* db, int count, bool traverseAssigns=false) {
  auto library = db->getLibrary(NLName("BENCH"));
  auto top = library ? library->getSNLDesign(NLName("top")) : nullptr;
  // Verilog port-to-wire assignments reload as canonical Assign instances.
  const int expectedInstances = traverseAssigns ? count * 2 + 1 : count;
  if (not top or top->getInstances().size() != static_cast<size_t>(expectedInstances) or
      top->getBitTerms().size() != static_cast<size_t>(count + 1)) {
    throw std::runtime_error("Unexpected serialized design shape");
  }
  auto input = top->getScalarTerm(NLName("a"));
  auto output = top->getBusTerm(NLName("y"));
  if (not input or not output) {
    throw std::runtime_error("Serialized design lost its ports");
  }
  SNLEquipotential inputConnectivity;
  if (traverseAssigns) {
    inputConnectivity = SNLEquipotential(input, SNLEquipotential::Mode::TraverseAssigns);
    if (inputConnectivity.getInstTermOccurrencesSet().size() != static_cast<size_t>(count)) {
      throw std::runtime_error("Verilog round trip changed input fanout");
    }
  }
  for (int i = 0; i < count; ++i) {
    auto instance = top->getInstance(NLName("u" + std::to_string(i)));
    if (not instance or not input->getNet() or not output->getBit(i)->getNet()) {
      throw std::runtime_error("Serialized connectivity differs from fixture");
    }
    auto instanceInput = instance->getInstTerm(instance->getModel()->getScalarTerm(NLName("a")));
    auto instanceOutput = instance->getInstTerm(instance->getModel()->getScalarTerm(NLName("y")));
    if (not instanceInput or not instanceOutput) {
      throw std::runtime_error("Serialized instance lost its ports");
    }
    if (traverseAssigns) {
      SNLEquipotential connectivity(output->getBit(i), SNLEquipotential::Mode::TraverseAssigns);
      if (not inputConnectivity.getInstTermOccurrencesSet().count(SNLOccurrence(instanceInput)) or
          connectivity.getInstTermOccurrencesSet().size() != 1 or
          not connectivity.getInstTermOccurrencesSet().count(SNLOccurrence(instanceOutput))) {
        throw std::runtime_error("Verilog round trip changed connectivity through assigns");
      }
    } else if (instanceInput->getNet() != input->getNet() or
        instanceOutput->getNet() != output->getBit(i)->getNet()) {
      throw std::runtime_error("Snapshot round trip changed connectivity");
    }
  }
}

}  // namespace

static void BM_SystemVerilogElaboration(benchmark::State& state) {
  const int groups = static_cast<int>(state.range(0));
  const int kind = static_cast<int>(state.range(1));
  BenchmarkDirectory directory;
  directory.write("design.sv", systemVerilogSource(groups, kind));
  auto library = createLibrary();
  SNLSVConstructor constructor(library);
  SNLSVConstructor::ConstructOptions options;
  options.diagnosticsReportPath.reset();
  for (auto _ : state) {
    constructor.construct(directory.path / "design.sv", options);
  }
  validateOutputs(library->getSNLDesign(NLName("top")), static_cast<size_t>(groups * 8), kind);
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * groups * 8);
}
BENCHMARK(BM_SystemVerilogElaboration)
  ->Args({8, 0})->Args({32, 0})->Args({128, 0})
  ->Args({8, 1})->Args({32, 1})->Args({128, 1})
  ->Args({8, 2})->Args({32, 2})->Args({128, 2})
  ->Args({8, 3})->Args({32, 3})->Args({128, 3})->Iterations(1);

static void BM_VHDLElaboration(benchmark::State& state) {
  const int groups = static_cast<int>(state.range(0));
  const int kind = static_cast<int>(state.range(1));
  BenchmarkDirectory directory;
  directory.write("design.vhd", vhdlSource(groups, kind));
  auto library = createLibrary();
  VHDLConstructor::ConstructOptions options;
  options.diagnosticsReportPath.reset();
  VHDLConstructor constructor(library, options);
  SNLDesign* top = nullptr;
  for (auto _ : state) {
    top = constructor.constructFile(directory.path / "design.vhd", "top");
  }
  validateOutputs(top, static_cast<size_t>(groups * 8), kind);
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * groups * 8);
}
BENCHMARK(BM_VHDLElaboration)
  ->Args({8, 0})->Args({32, 0})->Args({128, 0})
  ->Args({8, 1})->Args({32, 1})->Args({128, 1})
  ->Args({8, 2})->Args({32, 2})->Args({128, 2})
  ->Args({8, 3})->Args({32, 3})->Args({128, 3})->Iterations(1);

static void BM_LibertyLoading(benchmark::State& state) {
  const int cells = static_cast<int>(state.range(0));
  const int tableSize = static_cast<int>(state.range(1));
  BenchmarkDirectory directory;
  directory.write("cells.lib", libertySource(cells, tableSize));
  auto library = createLibrary(true);
  SNLLibertyConstructor constructor(library);
  for (auto _ : state) {
    constructor.construct(directory.path / "cells.lib");
  }
  if (library->getSNLDesigns().size() != static_cast<size_t>(cells)) {
    throw std::runtime_error("Liberty load omitted cells");
  }
  for (int i = 0; i < cells; ++i) {
    auto cell = library->getSNLDesign(NLName("cell_" + std::to_string(i)));
    if (not cell or cell->getBitTerms().size() != 2 or
        SNLDesignModeling::getCombinatorialInputs(cell->getScalarTerm(NLName("Y"))).size() != 1) {
      throw std::runtime_error("Liberty load omitted ports or timing dependencies");
    }
  }
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * cells);
}
BENCHMARK(BM_LibertyLoading)
  ->Args({16, 2})->Args({128, 2})->Args({512, 2})
  ->Args({16, 8})->Args({128, 8})->Args({512, 8})->Iterations(1);

static void BM_SnapshotEncode(benchmark::State& state) {
  const int count = static_cast<int>(state.range(0));
  BenchmarkDirectory directory;
  auto top = createSnapshotDesign(count);
  for (auto _ : state) {
    SNLCapnP::dump(top->getDB(), directory.path);
  }
  NLUniverse::get()->destroy();
  auto loaded = SNLCapnP::load(directory.path);
  validateSnapshotDesign(loaded, count);
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_SnapshotEncode)->Arg(64)->Arg(512)->Arg(4096);

static void BM_SnapshotDecode(benchmark::State& state) {
  const int count = static_cast<int>(state.range(0));
  BenchmarkDirectory directory;
  auto top = createSnapshotDesign(count);
  SNLCapnP::dump(top->getDB(), directory.path);
  NLUniverse::get()->destroy();
  NLDB* loaded = nullptr;
  for (auto _ : state) {
    loaded = SNLCapnP::load(directory.path);
  }
  validateSnapshotDesign(loaded, count);
  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_SnapshotDecode)->Arg(64)->Arg(512)->Arg(4096)->Iterations(1);

static void BM_VerilogDump(benchmark::State& state) {
  const int count = static_cast<int>(state.range(0));
  BenchmarkDirectory directory;
  auto top = createSnapshotDesign(count);
  auto primitives = top->getDB()->getLibrary(NLName("PRIMITIVES"));
  std::string result;
  for (auto _ : state) {
    std::ostringstream output;
    SNLVRLDumper dumper;
    dumper.dumpLibrary(primitives, output);
    dumper.dumpDesign(top, output);
    result = output.str();
    benchmark::DoNotOptimize(result);
  }
  directory.write("roundtrip.v", result);
  NLUniverse::get()->destroy();
  auto library = createLibrary();
  SNLVRLConstructor constructor(library);
  constructor.parse(directory.path / "roundtrip.v");
  constructor.setFirstPass(false);
  constructor.parse(directory.path / "roundtrip.v");
  validateSnapshotDesign(library->getDB(), count, true);
  NLUniverse::get()->destroy();
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(result.size()));
}
BENCHMARK(BM_VerilogDump)->Arg(64)->Arg(512)->Arg(4096);

BENCHMARK_MAIN();
