// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <fstream>
#include <iostream>

#include <argparse/argparse.hpp>

#include "NajaVersion.h"
#include "NajaPerf.h"
#include "NajaUtils.h"
#include "NajaLog.h"

#include "NLUniverse.h"
#include "NLException.h"
#include "SNLPyEdit.h"
#include "SNLPyLoader.h"
#include "SNLUtils.h"
#include "SNLLibertyConstructor.h"
#include "SNLSVConstructor.h"
#include "VHDLConstructor.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include "ConstantPropagation.h"
#include "RemoveLoadlessLogic.h"
#include "SNLCapnP.h"
#include "Reduction.h"
#include "Utils.h"
#include "NetlistGraph.h"

using namespace naja::NL;
using namespace naja::NAJA_OPT;

namespace {

enum class FormatType { NOT_PROVIDED, UNKNOWN, VERILOG, SYSTEMVERILOG, VHDL, SNL, DOT, SVG};
FormatType argToFormatType(const std::string& inputFormat) {
  if (inputFormat.empty()) {
    return FormatType::NOT_PROVIDED;
  } else if (inputFormat == "gate-level-verilog" || inputFormat == "verilog") {
    return FormatType::VERILOG;
  } else if (inputFormat == "systemverilog" || inputFormat == "sv") {
    return FormatType::SYSTEMVERILOG;
  } else if (inputFormat == "vhdl") {
    return FormatType::VHDL;
  } else if (inputFormat == "snl") {
    return FormatType::SNL;
  } else if (inputFormat == "dot") {
    return FormatType::DOT;
  /*} else if (inputFormat == "svg") {
    return FormatType::SVG;*/
  } else {
    return FormatType::UNKNOWN;
  }
}

enum class OptimizationType { NOT_PROVIDED, UNKNOWN, DLE, ALL };
OptimizationType argToOptimizationType(const std::string& optimization) {
  if (optimization.empty()) {
    return OptimizationType::NOT_PROVIDED;
  } else if (optimization == "dle") {
    return OptimizationType::DLE;
  } else if (optimization == "all") {
    return OptimizationType::ALL;
  } else {
    return OptimizationType::UNKNOWN;
  }
}

using Paths = std::vector<std::filesystem::path>;

long long getCountDelta(size_t before, size_t after) {
  return static_cast<long long>(after) - static_cast<long long>(before);
}

void logInstanceCountDelta(
    const std::string& label,
    const SNLUtils::InstanceCount& before,
    const SNLUtils::InstanceCount& after) {
  NAJA_LOG_INFO(
      "{} instance count:\n"
      "  unfolded: total {} -> {} (delta {}), leaf {} -> {} (delta {})\n"
      "  folded: total {} -> {} (delta {}), leaf {} -> {} (delta {})\n"
      "  reachable models: {} -> {}",
      label,
      before.totalInstances,
      after.totalInstances,
      getCountDelta(before.totalInstances, after.totalInstances),
      before.leafInstances,
      after.leafInstances,
      getCountDelta(before.leafInstances, after.leafInstances),
      before.foldedTotalInstances,
      after.foldedTotalInstances,
      getCountDelta(before.foldedTotalInstances, after.foldedTotalInstances),
      before.foldedLeafInstances,
      after.foldedLeafInstances,
      getCountDelta(before.foldedLeafInstances, after.foldedLeafInstances),
      before.reachableModels,
      after.reachableModels);
}

bool validateExistingPath(
    const std::filesystem::path& path,
    std::string_view option,
    std::string_view purpose) {
  if (path.empty()) {
    NAJA_LOG_CRITICAL(
      "{} contains an empty path; provide a path to {}",
      option,
      purpose);
    return false;
  }
  std::error_code ec;
  const bool exists = std::filesystem::exists(path, ec);
  if (ec) {
    NAJA_LOG_CRITICAL(
      "Cannot inspect {} path '{}': {}",
      purpose,
      path.string(),
      ec.message());
    return false;
  }
  if (!exists) {
    NAJA_LOG_CRITICAL(
      "{} path '{}' does not exist (resolved to '{}')",
      purpose,
      path.string(),
      std::filesystem::absolute(path).string());
    return false;
  }
  return true;
}

bool validateOutputPath(
    const std::filesystem::path& path,
    std::string_view option,
    std::string_view purpose) {
  if (path.empty()) {
    NAJA_LOG_CRITICAL(
      "{} requires a non-empty path for {}",
      option,
      purpose);
    return false;
  }
  const auto parent = path.parent_path().empty()
    ? std::filesystem::current_path()
    : path.parent_path();
  std::error_code ec;
  const bool parentExists = std::filesystem::is_directory(parent, ec);
  if (ec || !parentExists) {
    NAJA_LOG_CRITICAL(
      "Cannot write {} to '{}': parent directory '{}' {}. "
      "Create the directory or choose another path",
      purpose,
      path.string(),
      parent.string(),
      ec ? "cannot be inspected" : "does not exist");
    return false;
  }
  return true;
}

}

int main(int argc, char* argv[]) {
  const auto najaEditStart{std::chrono::steady_clock::now()};
  argparse::ArgumentParser program("naja_edit", naja::NAJA_VERSION);
  program.add_description(
      "Edit gate level netlists using python script and apply optimizations");
  program.add_argument("-f", "--from_format")
    .help("from/input format: gate-level-verilog, "
          "systemverilog|sv (Verilog/SystemVerilog RTL subset), vhdl, snl; "
          "verilog is a compatibility alias and will be deprecated soon");
  program.add_argument("-t", "--to_format")
    .help("to/output format: gate-level-verilog, snl, dot; "
          "verilog is a compatibility alias and will be deprecated soon");
  program.add_argument("-i", "--input").append().help("input netlist paths");
  program.add_argument("-o", "--output").help("output netlist");
  program.add_argument("-p", "--primitives")
    .nargs(argparse::nargs_pattern::at_least_one)
    .help("input primitives: Liberty or Naja Python files, loaded before HDL or SNL input");
  program.add_argument("-d", "--dump_primitives")
    .help("dump primitives library in verilog");
  program.add_argument("-e", "--pre_edit")
    .help(
      "edit netlist using python script after loading netlist and before "
      "applying optimizations");
  program.add_argument("-z", "--post_edit")
    .help(
      "edit netlist using python script after optimizations and before "
      "dumping netlist");
  program.add_argument("-a", "--apply")
    .help(
      "apply optimization: dle (remove loadless logic), all (all "
      "optimizations)");
  program.add_argument("-l", "--log")
    .default_value(std::string("naja_edit.log"))
    .help("Dump log file (default name: naja_edit.log)");
  program.add_argument("-s", "--stats")
    .default_value(std::string("naja_perf.log"))
    .help("Dump performance log file (default: naja_perf.log, env override: NAJA_PERF)");
  program.add_argument("--sv_flist")
    .help("SystemVerilog slang command file path (passed as -f <file>)");
  program.add_argument("--vhdl_top").help("VHDL top entity name");
  program.add_argument("--vhdl_diagnostics_report")
    .help("VHDL diagnostics report path (empty disables file output)");
  program.add_argument("--sv_top")
    .help("SystemVerilog top module name (injected as --top)");
  program.add_argument("--sv_elaborated_ast_json_path")
    .help("Dump Slang elaborated AST JSON for SystemVerilog parsing");
  program.add_argument("--sv_diagnostics_report_path")
    .help(
      "Incremental SystemVerilog diagnostics report path "
      "(default: naja_sv_diagnostics.log)");
  program.add_argument("--sv_no_pretty_print_elaborated_ast_json")
    .default_value(false)
    .implicit_value(true)
    .help("Disable pretty-print formatting for SystemVerilog elaborated AST JSON dump");
  program.add_argument("--sv_no_source_info_in_elaborated_ast_json")
    .default_value(false)
    .implicit_value(true)
    .help("Disable source info in SystemVerilog elaborated AST JSON dump");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  auto statsPath = naja::NajaPerf::getLogPathFromEnv("NAJA_PERF", "naja_perf.log");
  if (!statsPath) {
    statsPath = "naja_perf.log";
  }
  if (program.is_used("--stats")) {
    statsPath = program.get<std::string>("--stats");
  }
  naja::NajaPerf::create(statsPath, "naja_edit");

  naja::log::init();
  naja::log::setPattern(
    "%Y-%m-%d %H:%M:%S,%e [naja_edit] [%^%l%$] %v");
  naja::log::setLevel(spdlog::level::trace);

  if (program.is_used("--log")) {
    auto logName = program.get<std::string>("--log");
    {
      std::ofstream logFile(logName, std::ios::out);
      if (logFile.is_open()) {
        std::string bannerTitle = "naja_edit";
        std::ostringstream bannerStream;
        naja::NajaUtils::createBanner(logFile, bannerTitle, "#");
        logFile << std::endl;
        logFile.close();
      }
    }
    naja::log::addFileSink(logName, spdlog::level::trace);
  }

  naja::log::flushEvery(std::chrono::seconds(3));

  bool argError = false;

  std::string inputFormat;
  if (auto inputFormatArg = program.present("-f")) {
    inputFormat = *inputFormatArg;
  }
  if (inputFormat == "verilog") {
    NAJA_LOG_WARN(
      "--from_format (-f) 'verilog' will be deprecated soon; "
      "use 'gate-level-verilog' instead");
  }
  FormatType inputFormatType = argToFormatType(inputFormat);
  if (inputFormatType == FormatType::UNKNOWN ||
      inputFormatType == FormatType::DOT ||
      inputFormatType == FormatType::SVG) {
    NAJA_LOG_CRITICAL(
      "Invalid value '{}' for --from_format (-f). "
      "Expected one of: gate-level-verilog (or verilog), systemverilog (or sv), vhdl, snl",
      inputFormat);
    argError = true;
  }

  if (program.present("-i")) {
    if (inputFormatType == FormatType::NOT_PROVIDED) {
      NAJA_LOG_CRITICAL(
        "--from_format (-f) is required when --input (-i) is provided");
      argError = true;
    }
  }

  std::string outputFormat;
  if (auto outputFormatArg = program.present("-t")) {
    outputFormat = *outputFormatArg;
    if (outputFormat == "verilog") {
      NAJA_LOG_WARN(
        "--to_format (-t) 'verilog' will be deprecated soon; "
        "use 'gate-level-verilog' instead");
    }
  } else {
    if (program.is_used("-o")) {
      // in case output format is not provided and output path is provided
      // output format is same as input format
      if (inputFormatType == FormatType::NOT_PROVIDED) {
        NAJA_LOG_CRITICAL(
          "--to_format (-t) is required when --output (-o) is provided "
          "without an input format to infer from");
        argError = true;
      } else if (inputFormatType == FormatType::SYSTEMVERILOG || inputFormatType == FormatType::VHDL) {
        NAJA_LOG_CRITICAL(
          "--to_format (-t) is required for HDL input. "
          "Expected one of: gate-level-verilog (or verilog), snl, dot");
        argError = true;
      } else {
        outputFormat = inputFormat;
      }
    }
  }
  FormatType outputFormatType = argToFormatType(outputFormat);
  
  if (outputFormatType == FormatType::UNKNOWN ||
      outputFormatType == FormatType::SYSTEMVERILOG ||
      outputFormatType == FormatType::VHDL ||
      outputFormatType == FormatType::SVG) {
    NAJA_LOG_CRITICAL(
      "Invalid value '{}' for --to_format (-t). "
      "Expected one of: gate-level-verilog (or verilog), snl, dot",
      outputFormat);
    argError = true;
  }

  Paths primitivesPaths;
  if (auto primitives = program.present("-p")) {
    auto primitivesPathsString = program.get<std::vector<std::string>>("-p");
    primitivesPaths.resize(primitivesPathsString.size());
    std::transform(primitivesPathsString.begin(),
      primitivesPathsString.end(),
      primitivesPaths.begin(),
      [](const std::string& str) {
        return std::filesystem::path(str);
      }
    );
  }

  const auto vhdlTop = program.present("--vhdl_top");
  const auto vhdlReport = program.present("--vhdl_diagnostics_report");
  if ((vhdlTop || vhdlReport) && inputFormatType != FormatType::VHDL) {
    NAJA_LOG_CRITICAL("VHDL options (--vhdl_*) require --from_format vhdl");
    argError = true;
  }
  if (vhdlTop && vhdlTop->empty()) {
    NAJA_LOG_CRITICAL("VHDL top name (--vhdl_top) cannot be empty");
    argError = true;
  }
  bool hasSvFlistPath = false;
  std::filesystem::path svFlistPath;
  if (auto svFlist = program.present("--sv_flist")) {
    hasSvFlistPath = true;
    svFlistPath = std::filesystem::path(*svFlist);
  }

  bool hasSvTop = false;
  std::string svTop;
  if (auto svTopArg = program.present("--sv_top")) {
    hasSvTop = true;
    svTop = *svTopArg;
    if (svTop.empty()) {
      NAJA_LOG_CRITICAL("SystemVerilog top name (--sv_top) cannot be empty");
      argError = true;
    }
  }

  bool hasSvElaboratedASTJsonPath = false;
  std::filesystem::path svElaboratedASTJsonPath;
  if (auto svAstPath = program.present("--sv_elaborated_ast_json_path")) {
    hasSvElaboratedASTJsonPath = true;
    svElaboratedASTJsonPath = std::filesystem::path(*svAstPath);
  }

  bool hasSvDiagnosticsReportPath = false;
  std::filesystem::path svDiagnosticsReportPath;
  if (auto svDiagPath = program.present("--sv_diagnostics_report_path")) {
    hasSvDiagnosticsReportPath = true;
    svDiagnosticsReportPath = std::filesystem::path(*svDiagPath);
  }

  const bool svPrettyPrintElaboratedASTJson =
    !program.get<bool>("--sv_no_pretty_print_elaborated_ast_json");
  const bool svIncludeSourceInfoInElaboratedASTJson =
    !program.get<bool>("--sv_no_source_info_in_elaborated_ast_json");

  const bool hasSystemVerilogSpecificOption =
    hasSvFlistPath || hasSvTop || hasSvElaboratedASTJsonPath || hasSvDiagnosticsReportPath ||
    !svPrettyPrintElaboratedASTJson || !svIncludeSourceInfoInElaboratedASTJson;
  if (hasSystemVerilogSpecificOption && inputFormatType != FormatType::SYSTEMVERILOG) {
    NAJA_LOG_CRITICAL(
      "SystemVerilog parsing options (--sv_*) are only valid with input format "
      "'systemverilog' (or 'sv')");
    argError = true;
  }
  if (inputFormatType == FormatType::SYSTEMVERILOG &&
      !program.present("-i") && !hasSvFlistPath) {
    NAJA_LOG_CRITICAL(
      "For SystemVerilog input, provide at least one --input (-i) file or --sv_flist");
    argError = true;
  }
  if (inputFormatType != FormatType::NOT_PROVIDED &&
      inputFormatType != FormatType::UNKNOWN &&
      inputFormatType != FormatType::SYSTEMVERILOG &&
      !program.present("-i")) {
    NAJA_LOG_CRITICAL(
      "--input (-i) is required with --from_format '{}'",
      inputFormat);
    argError = true;
  }

  OptimizationType optimizationType = OptimizationType::NOT_PROVIDED;
  if (auto optimizationArg = program.present("-a")) {
    std::string optimization = *optimizationArg;
    optimizationType = argToOptimizationType(optimization);
    if (optimizationType == OptimizationType::UNKNOWN) {
      NAJA_LOG_CRITICAL(
        "Invalid value '{}' for --apply (-a). Expected one of: dle, all",
        optimization);
      argError = true;
    }
  }

  using StringPaths = std::vector<std::string>;
  StringPaths inputStringPaths = program.get<StringPaths>("-i");

  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  std::transform(inputStringPaths.begin(), inputStringPaths.end(),
                 std::back_inserter(inputPaths),
                 [](const std::string& sp) -> std::filesystem::path {
                   return std::filesystem::path(sp);
                 });

  if (inputFormatType == FormatType::SNL && inputPaths.size() > 1) {
    NAJA_LOG_CRITICAL(
      "--from_format snl accepts exactly one --input path, but {} were provided",
      inputPaths.size());
    argError = true;
  }
  for (const auto& inputPath : inputPaths) {
    argError =
      !validateExistingPath(inputPath, "--input (-i)", "input") || argError;
  }
  for (const auto& primitivesPath : primitivesPaths) {
    argError =
      !validateExistingPath(
        primitivesPath, "--primitives (-p)", "primitives input") || argError;
  }
  if (hasSvFlistPath) {
    argError =
      !validateExistingPath(
        svFlistPath, "--sv_flist", "SystemVerilog command file") || argError;
  }
  if (auto preEdit = program.present("-e")) {
    argError =
      !validateExistingPath(*preEdit, "--pre_edit (-e)", "pre-edit script") || argError;
  }
  if (auto postEdit = program.present("-z")) {
    argError =
      !validateExistingPath(*postEdit, "--post_edit (-z)", "post-edit script") || argError;
  }

  std::filesystem::path outputPath;
  if (auto output = program.present("-o")) {
    outputPath = std::filesystem::path(*output);
    argError =
      !validateOutputPath(outputPath, "--output (-o)", "netlist output") || argError;
  } else {
    if (outputFormatType != FormatType::NOT_PROVIDED) {
      NAJA_LOG_CRITICAL(
        "--output (-o) is required when --to_format (-t) is provided");
      argError = true;
    }
  }

  if (argError) {
    NAJA_LOG_CRITICAL(
      "Invalid command-line configuration; run 'naja_edit --help' for usage");
    return EXIT_FAILURE;
  }

  try {
    NLDB* db = nullptr;
    NLLibrary* primitivesLibrary = nullptr;
    auto loadPrimitivesLibrary = [&](NLDB* currentDB) -> NLLibrary* {
      if (primitivesPaths.empty()) {
        return nullptr;
      }
      auto* currentPrimitivesLibrary =
        NLLibrary::create(currentDB, NLLibrary::Type::Primitives, NLName("PRIMS"));
      SNLLibertyConstructor libertyConstructor(currentPrimitivesLibrary);
      Paths libertyPrimitivesPaths;
      libertyPrimitivesPaths.reserve(primitivesPaths.size());
      auto constructLibertyPrimitives = [&]() {
        if (not libertyPrimitivesPaths.empty()) {
          libertyConstructor.construct(libertyPrimitivesPaths);
          libertyPrimitivesPaths.clear();
        }
      };
      for (const auto& path : primitivesPaths) {
        NAJA_LOG_INFO("Parsing primitives file: {}", path.string());
        auto extension = path.extension();
        if (extension == ".py") {
          constructLibertyPrimitives();
          SNLPyLoader::loadPrimitives(currentPrimitivesLibrary, path);
        } else if (SNLLibertyConstructor::isLibertyPath(path)) {
          libertyPrimitivesPaths.push_back(path);
        } else {
          NAJA_LOG_CRITICAL(
            "Cannot determine primitives format for '{}'. Expected a .py file, "
            "a .lib* Liberty file, or a gzip/zip file containing Liberty data",
            path.string());
          std::exit(EXIT_FAILURE);
        }
      }
      constructLibertyPrimitives();
      return currentPrimitivesLibrary;
    };
    {
      naja::NajaPerf::Scope scope("SNL Creation");
      NLUniverse::create();

      if (inputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Parsing SNL format");
        const auto start{std::chrono::steady_clock::now()};
        auto inputPath = inputPaths[0];
        if (!primitivesPaths.empty()) {
          db = NLDB::create(NLUniverse::get());
          primitivesLibrary = loadPrimitivesLibrary(db);
        }
        db = SNLCapnP::load(inputPath);
        NLUniverse::get()->setTopDesign(db->getTopDesign());
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          NAJA_LOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Parsing verilog");
        db = NLDB::create(NLUniverse::get());
        primitivesLibrary = loadPrimitivesLibrary(db);

        auto designLibrary = NLLibrary::create(db, NLName("DESIGN"));
        SNLVRLConstructor constructor(designLibrary);
        const auto start{std::chrono::steady_clock::now()};
        for (const auto& path : inputPaths) {
          NAJA_LOG_INFO("Parsing verilog file: {}", path.string());
        }
        constructor.construct(inputPaths);
        auto top = SNLUtils::findTop(designLibrary);
        if (top) {
          NLUniverse::get()->setTopDesign(top);
          NAJA_LOG_INFO("Found top design: " + top->getString());
        } else {
          throw NLException(
            "No top design was found after parsing Verilog. Ensure the input "
            "has exactly one uninstantiated root module.");
        }
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          NAJA_LOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::SYSTEMVERILOG) {
        naja::NajaPerf::Scope scope("Parsing systemverilog");
        db = NLDB::create(NLUniverse::get());
        primitivesLibrary = loadPrimitivesLibrary(db);

        auto designLibrary = NLLibrary::create(db, NLName("DESIGN"));
        SNLSVConstructor constructor(designLibrary);
        SNLSVConstructor::ConstructOptions options;
        options.prettyPrintElaboratedASTJson = svPrettyPrintElaboratedASTJson;
        options.includeSourceInfoInElaboratedASTJson =
          svIncludeSourceInfoInElaboratedASTJson;
        if (hasSvElaboratedASTJsonPath) {
          options.elaboratedASTJsonPath = svElaboratedASTJsonPath;
        }
        if (hasSvDiagnosticsReportPath) {
          options.diagnosticsReportPath = svDiagnosticsReportPath;
        }

        Paths svInputPaths = inputPaths;
        bool removeTemporaryTopFlist = false;
        std::filesystem::path temporaryTopFlistPath;
        auto removeTemporaryTopFlistIfNeeded = [&]() {
          if (!removeTemporaryTopFlist) {
            return;
          }
          std::error_code ec;
          std::filesystem::remove(temporaryTopFlistPath, ec);
        };

        const auto quotePathForSlangCommandFile = [](const std::filesystem::path& path) {
          std::string quoted;
          quoted.reserve(path.string().size() + 2);
          quoted.push_back('"');
          for (const auto c : path.string()) {
            if (c == '\\' || c == '"') {
              quoted.push_back('\\');
            }
            quoted.push_back(c);
          }
          quoted.push_back('"');
          return quoted;
        };

        if (hasSvTop) {
          temporaryTopFlistPath =
            std::filesystem::temp_directory_path() /
            std::filesystem::path(
              "naja_edit_sv_top_" +
              std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
              ".f");
          std::ofstream topFlist(temporaryTopFlistPath, std::ios::out | std::ios::trunc);
          if (!topFlist) {
            NAJA_LOG_CRITICAL(
              "Failed to create temporary SystemVerilog command file: {}",
              temporaryTopFlistPath.string());
            std::exit(EXIT_FAILURE);
          }
          topFlist << "--top " << svTop << "\n";
          if (hasSvFlistPath) {
            topFlist << "-f " << quotePathForSlangCommandFile(svFlistPath) << "\n";
          }
          for (const auto& svInputPath : inputPaths) {
            topFlist << quotePathForSlangCommandFile(svInputPath) << "\n";
          }
          topFlist.close();
          svInputPaths.clear();
          svInputPaths.emplace_back(std::filesystem::path("-f"));
          svInputPaths.emplace_back(temporaryTopFlistPath);
          removeTemporaryTopFlist = true;
        } else if (hasSvFlistPath) {
          svInputPaths.insert(svInputPaths.begin(), svFlistPath);
          svInputPaths.insert(svInputPaths.begin(), std::filesystem::path("-f"));
        }

        const auto start{std::chrono::steady_clock::now()};
        {
          std::ostringstream oss;
          oss << "Parsing SystemVerilog input(s): ";
          size_t i = 0;
          for (const auto& path : svInputPaths) {
            if (i++ >= 4) {
              oss << std::endl;
              i = 0;
            }
            oss << path << " ";
          }
          NAJA_LOG_INFO(oss.str());
        }
        try {
          constructor.construct(svInputPaths, options);
        } catch (...) {
          removeTemporaryTopFlistIfNeeded();
          throw;
        }
        removeTemporaryTopFlistIfNeeded();

        auto top = SNLUtils::findTop(designLibrary);
        if (top) {
          NLUniverse::get()->setTopDesign(top);
          NAJA_LOG_INFO("Found top design: " + top->getString());
        } else {
          throw NLException(
            "No top design was found after parsing SystemVerilog. Ensure the "
            "input has exactly one uninstantiated root module, or select one "
            "with --sv_top <module>.");
        }
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          NAJA_LOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::VHDL) {
        db = NLDB::create(NLUniverse::get());
        primitivesLibrary = loadPrimitivesLibrary(db);
        auto* designLibrary = NLLibrary::create(db, NLName("DESIGN"));
        VHDLConstructor::ConstructOptions options;
        if (vhdlReport) {
          if (vhdlReport->empty()) options.diagnosticsReportPath.reset();
          else options.diagnosticsReportPath = *vhdlReport;
        }
        VHDLConstructor constructor(designLibrary, options);
        auto* top = constructor.constructFiles(inputPaths, vhdlTop.value_or(""));
        if (!top) throw NLException("No top entity was found after parsing VHDL");
        NLUniverse::get()->setTopDesign(top);
        NAJA_LOG_INFO("Found top design: " + top->getString());
    } else if (inputFormatType == FormatType::NOT_PROVIDED) {
      db = NLDB::create(NLUniverse::get());
      NLUniverse::get()->setTopDB(db);
      } else {
        NAJA_LOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
        std::exit(EXIT_FAILURE);
      }
    }

    if (program.is_used("-e")) {
      naja::NajaPerf::Scope scope("Python Pre Editing");
      const auto start{std::chrono::steady_clock::now()};
      auto editPath = std::filesystem::path(program.get<std::string>("-e"));
      NAJA_LOG_INFO("Editing netlist using python script (post netlist loading): {}", editPath.string());
      SNLPyEdit::edit(editPath);
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Editing done in: " << elapsed_seconds.count() << "s";
        NAJA_LOG_INFO(oss.str());
      }
    }
    bool useBNE = true;
    if (std::getenv("NAJA_DISABLE_BNE")) {
      useBNE = false;
    }
    if (optimizationType == OptimizationType::DLE) {
      naja::NajaPerf::Scope scope("Optimization_DLE");
      const auto beforeInstanceCount =
          SNLUtils::countReachableInstances(NLUniverse::get()->getTopDesign());
      const auto start{std::chrono::steady_clock::now()};
      NAJA_LOG_INFO("Starting removal of loadless logic");
      LoadlessLogicRemover remover;
      remover.setNormalizedUniquification(useBNE);
      remover.process();
      const auto afterInstanceCount =
          SNLUtils::countReachableInstances(NLUniverse::get()->getTopDesign());
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Removal of loadless logic done in: " << elapsed_seconds.count() << "s";
        NAJA_LOG_INFO(oss.str());
      } 
      logInstanceCountDelta(
          "DLE optimization", beforeInstanceCount, afterInstanceCount);
      //NetlistStatistics stats(*get());
      //stats.process();
      //spdlog::info(stats.getReport());
    } else if (optimizationType == OptimizationType::ALL) {
      naja::NajaPerf::Scope scope("Optimization_ALL");
      const auto beforeInstanceCount =
          SNLUtils::countReachableInstances(NLUniverse::get()->getTopDesign());
      const auto start{std::chrono::steady_clock::now()};
      NAJA_LOG_INFO("Starting full optimization (constant propagation and removal of loadless logic)");
      ConstantPropagation cp;
      cp.setTruthTableEngine(true);
      cp.setNormalizedUniquification(useBNE);
      cp.run();
      ReductionOptimization reductionOptimization(cp.getPartialConstantReaders());
      reductionOptimization.setNormalizedUniquification(useBNE);
      reductionOptimization.run();
      LoadlessLogicRemover remover;
      remover.setNormalizedUniquification(useBNE);
      remover.process();
      const auto afterInstanceCount =
          SNLUtils::countReachableInstances(NLUniverse::get()->getTopDesign());
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Full optimization done in: " << elapsed_seconds.count()
            << "s";
        NAJA_LOG_INFO(oss.str());
      }
      logInstanceCountDelta(
          "Full optimization", beforeInstanceCount, afterInstanceCount);
      //NetlistStatistics stats(*get());
      //stats.process();
      //spdlog::info(stats.getReport());
    }

    if (program.is_used("-z")) {
      naja::NajaPerf::Scope scope("Python Post Editing");
      const auto start{std::chrono::steady_clock::now()};
      auto editPath = std::filesystem::path(program.get<std::string>("-z"));
      NAJA_LOG_INFO("Post editing netlist using python script: {}", editPath.string());
      SNLPyEdit::edit(editPath);
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Post editing done in: " << elapsed_seconds.count() << "s";
        NAJA_LOG_INFO(oss.str());
      }
    }

    {
      naja::NajaPerf::Scope scope("Dumping Netlist");
      if (outputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Dumping SNL format");
        NAJA_LOG_INFO("Dumping netlist in SNL format to {}", outputPath.string());
        SNLCapnP::dump(db, outputPath);
      } else if (outputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Dumping verilog");
        if (db->getTopDesign()) {
          std::ofstream output(outputPath);
          SNLVRLDumper dumper;
          dumper.setSingleFile(true);
          NAJA_LOG_INFO("Dumping netlist in verilog format to {}", outputPath.string());
          dumper.dumpDesign(db->getTopDesign(), output);
        } else {
          db->debugDump(0);
        }
      } else if (outputFormatType == FormatType::DOT) {
        naja::NajaPerf::Scope scope("Dumping DOT format");
        std::string dotFileName(outputPath.string());
        naja::SnlVisualiser snl(db->getTopDesign());
        snl.process();
        snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
      } /*else if (outputFormatType == FormatType::SVG) {
          std::string dotFileName(outputPath.string());
          std::string svgFileName(
              outputPath.string() + std::string(".svg"));
          naja::SnlVisualiser snl(db->getTopDesign());
          snl.process();
          snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
          system(std::string(std::string("dot -Tsvg ") + dotFileName +
                            std::string(" -o ") + svgFileName)
                    .c_str());
      }*/
    }

    if (program.is_used("-d")) {
      if (not primitivesLibrary and inputFormatType==FormatType::SNL) {
        auto primitiveLibraries = db->getPrimitiveLibraries();
        if (not primitiveLibraries.empty()) {}
          primitivesLibrary = *(db->getPrimitiveLibraries().begin());
      }
      if (primitivesLibrary) {
        auto outputPrimitivesPath =
            std::filesystem::path(program.get<std::string>("-d"));
        std::ofstream output(outputPrimitivesPath);
        SNLVRLDumper dumper;
        dumper.setSingleFile(true);
        dumper.dumpLibrary(primitivesLibrary, output);
      }
    }
  } catch (const NLException& e) {
    NAJA_LOG_CRITICAL("naja_edit failed: {}", e.what());
    return EXIT_FAILURE;
  } catch (const std::exception& e) {
    NAJA_LOG_CRITICAL("naja_edit failed with an unexpected error: {}", e.what());
    return EXIT_FAILURE;
  }
  const auto najaEditEnd{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> najaElapsedSeconds{najaEditEnd - najaEditStart};
  auto memInfo = naja::NajaPerf::getMemoryUsage();
  auto vmRSS = memInfo.first;
  auto vmPeak = memInfo.second;
  NAJA_LOG_INFO("########################################################");
  {
    std::ostringstream oss;
    oss << "naja_edit done in: " << najaElapsedSeconds.count() << "s";
    if (vmRSS != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(RSS): " << vmRSS / 1024.0 << "Mb";
    }
    if (vmPeak != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(Peak): " << vmPeak / 1024.0 << "Mb";
    }
    NAJA_LOG_INFO(oss.str());
  }
  NAJA_LOG_INFO("naja version: {}", naja::NAJA_VERSION);
  NAJA_LOG_INFO("Git hash: {}", naja::NAJA_GIT_HASH);
  NAJA_LOG_INFO("########################################################");
  return EXIT_SUCCESS;
}
