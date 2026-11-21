// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLName.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLScalarNet.h"

#include "SNLSVConstructor.h"
#include "SNLSVIntent.h"

#include "slang/ast/Compilation.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/driver/Driver.h"

using namespace naja::NL;

#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

std::filesystem::path writeIntentMiniFixture(const std::string& testName) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / testName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directories(outPath);
  const auto svPath = outPath / "intent_mini.sv";
  std::ofstream svFile(svPath);
  svFile << R"(
interface mini_if;
endinterface

package mini_pkg;
  typedef logic [7:0] byte_t;
  typedef enum logic [1:0] {
    ST_IDLE = 2'b00,
    ST_RUN  = 2'b01,
    ST_DONE = 2'b11
  } state_e;
  typedef struct packed {
    logic   valid;
    byte_t  data;
    state_e state;
  } payload_t;
  typedef union packed {
    logic [7:0] raw;
    byte_t      data;
  } overlay_t;
  typedef integer integer_t;
  typedef logic scalar_t;
  typedef real floating_t;
  typedef logic unpacked_array_t[2];
  typedef logic dynamic_array_t[];
  typedef logic associative_array_t[int];
  typedef logic queue_t[$];
  typedef struct { logic value; } unpacked_struct_t;
  typedef union { logic value; bit alternate; } unpacked_union_t;
  class mini_class;
  endclass
  typedef mini_class class_t;
  typedef string string_t;
  typedef event event_t;
  typedef virtual mini_if virtual_interface_t;
  typedef chandle chandle_t;
  function void do_nothing();
  endfunction
  localparam int PLEN = (32 == 32) ? 34 : 56;
endpackage

module intent_child #(parameter int WIDTH = 1) ();
endmodule

module intent_mini #(
    parameter int DEPTH = 4
) (
    input  logic clk,
    input  logic rst_n,
    input  logic rst,
    input  logic enable,
    input  logic data
);
  import mini_pkg::*;
  localparam int IDX_W = $clog2(DEPTH);

  mini_pkg::state_e state_q;
  byte_t             byte_q;
  logic [3:0]        plain_q;
  payload_t          payload_q;
  overlay_t          overlay_q;
  logic              latch_q;
  logic              reset_enable_q;
  logic              set_enable_q;
  intent_child #(.WIDTH(DEPTH)) u_child ();

  always_latch begin
    if (enable) latch_q <= data;
  end

  always_ff @(posedge clk or posedge rst) begin
    if (rst) reset_enable_q <= 1'b0;
    else if (enable) reset_enable_q <= data;
  end

  always_ff @(posedge clk or posedge rst) begin
    if (rst) set_enable_q <= 1'b1;
    else if (enable) set_enable_q <= data;
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) state_q <= ST_IDLE;
    else        state_q <= ST_RUN;
  end
endmodule
)";
  return svPath;
}

SNLInstance* findInstanceByModel(SNLDesign* design, const std::string& modelName) {
  for (auto* instance : design->getInstances()) {
    if (instance && instance->getModel() &&
        instance->getModel()->getName().getString() == modelName) {
      return instance;
    }
  }
  return nullptr;
}

}  // namespace

class SNLSVIntentTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }

    SNLDesign* loadIntentMini() {
      SNLSVConstructor constructor(library_);
      SNLSVConstructor::ConstructOptions options;
      options.keepASTLink = true;
      constructor.construct(writeIntentMiniFixture(::testing::UnitTest::GetInstance()
                                                     ->current_test_info()
                                                     ->name()),
                            options);
      auto* top = library_->getSNLDesign(NLName("intent_mini"));
      EXPECT_NE(nullptr, top);
      return top;
    }

    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVIntentTest, returnsEnumIntentForSourceAndBitBlastedObjects) {
  EXPECT_FALSE(SNLSVIntent::available());
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);
  ASSERT_TRUE(SNLSVIntent::available());

  auto* stateQ = top->getNet(NLName("state_q"));
  ASSERT_NE(nullptr, stateQ);

  const auto type = SNLSVIntent::typeOf(stateQ);
  ASSERT_TRUE(type.valid);
  EXPECT_EQ("mini_pkg::state_e", type.typeName);
  EXPECT_EQ("enum", type.canonicalKind);
  ASSERT_TRUE(type.isEnum);
  EXPECT_EQ(2u, type.enumWidth);
  ASSERT_EQ(3u, type.members.size());
  EXPECT_EQ("ST_IDLE", type.members[0].name);
  EXPECT_EQ("2'b00", type.members[0].encoding);
  EXPECT_EQ("ST_RUN", type.members[1].name);
  EXPECT_EQ("2'b01", type.members[1].encoding);
  EXPECT_EQ("ST_DONE", type.members[2].name);
  EXPECT_EQ("2'b11", type.members[2].encoding);
  EXPECT_NE(std::string::npos, type.enumDeclLoc.file.getString().find("intent_mini.sv"));
  EXPECT_EQ(7u, type.enumDeclLoc.line);

  auto* ff = findInstanceByModel(top, "naja_dffrn__w2");
  ASSERT_NE(nullptr, ff);
  const auto ffType = SNLSVIntent::typeOf(ff);
  ASSERT_TRUE(ffType.valid);
  EXPECT_EQ(type.typeName, ffType.typeName);
  ASSERT_TRUE(ffType.isEnum);
  EXPECT_EQ(type.members[2].encoding, ffType.members[2].encoding);
}

TEST_F(SNLSVIntentTest, createsScalarSequentialPrimitivesWithLiveLinkEnabled) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  bool foundLatch = false;
  bool foundDFFRE = false;
  bool foundDFFSE = false;
  for (auto* instance : top->getInstances()) {
    const auto* model = instance->getModel();
    if (!NLDB0::isDLatch(model) &&
        !NLDB0::isDFFRE(model) &&
        !NLDB0::isDFFSE(model)) {
      continue;
    }

    const auto type = SNLSVIntent::typeOf(instance);
    ASSERT_TRUE(type.valid);
    EXPECT_EQ("logic", type.typeName);
    EXPECT_EQ("scalar", type.canonicalKind);
    foundLatch |= NLDB0::isDLatch(model);
    foundDFFRE |= NLDB0::isDFFRE(model);
    foundDFFSE |= NLDB0::isDFFSE(model);
  }

  EXPECT_TRUE(foundLatch);
  EXPECT_TRUE(foundDFFRE);
  EXPECT_TRUE(foundDFFSE);
}

TEST_F(SNLSVIntentTest, returnsScalarAliasAndPackedAggregateIntent) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  auto* clk = top->getNet(NLName("clk"));
  ASSERT_NE(nullptr, clk);
  const auto clkType = SNLSVIntent::typeOf(clk);
  ASSERT_TRUE(clkType.valid);
  EXPECT_EQ("logic", clkType.typeName);
  EXPECT_EQ("scalar", clkType.canonicalKind);
  EXPECT_FALSE(clkType.isEnum);
  EXPECT_FALSE(clkType.isStruct);

  auto* byteQ = top->getNet(NLName("byte_q"));
  ASSERT_NE(nullptr, byteQ);
  const auto byteType = SNLSVIntent::typeOf(byteQ);
  ASSERT_TRUE(byteType.valid);
  EXPECT_EQ("mini_pkg::byte_t", byteType.typeName);
  EXPECT_EQ("packed_array", byteType.canonicalKind);
  EXPECT_FALSE(byteType.isEnum);
  EXPECT_FALSE(byteType.isStruct);

  auto* plainQ = top->getNet(NLName("plain_q"));
  ASSERT_NE(nullptr, plainQ);
  const auto plainType = SNLSVIntent::typeOf(plainQ);
  ASSERT_TRUE(plainType.valid);
  EXPECT_EQ("logic[3:0]", plainType.typeName);
  EXPECT_EQ("packed_array", plainType.canonicalKind);
  EXPECT_FALSE(plainType.isEnum);
  EXPECT_FALSE(plainType.isStruct);

  auto* payloadQ = top->getNet(NLName("payload_q"));
  ASSERT_NE(nullptr, payloadQ);
  const auto payloadType = SNLSVIntent::typeOf(payloadQ);
  ASSERT_TRUE(payloadType.valid);
  EXPECT_EQ("mini_pkg::payload_t", payloadType.typeName);
  EXPECT_EQ("packed_struct", payloadType.canonicalKind);
  EXPECT_FALSE(payloadType.isEnum);
  ASSERT_TRUE(payloadType.isStruct);
  EXPECT_EQ(11u, payloadType.structWidth);
  EXPECT_EQ(12u, payloadType.structDeclLoc.line);
  ASSERT_EQ(3u, payloadType.fields.size());
  EXPECT_EQ("valid", payloadType.fields[0].name);
  EXPECT_EQ("logic", payloadType.fields[0].typeName);
  EXPECT_EQ(10u, payloadType.fields[0].msb);
  EXPECT_EQ(10u, payloadType.fields[0].lsb);
  EXPECT_EQ("data", payloadType.fields[1].name);
  EXPECT_EQ("mini_pkg::byte_t", payloadType.fields[1].typeName);
  EXPECT_EQ(9u, payloadType.fields[1].msb);
  EXPECT_EQ(2u, payloadType.fields[1].lsb);
  EXPECT_EQ("state", payloadType.fields[2].name);
  EXPECT_EQ("mini_pkg::state_e", payloadType.fields[2].typeName);
  EXPECT_EQ(1u, payloadType.fields[2].msb);
  EXPECT_EQ(0u, payloadType.fields[2].lsb);

  auto* overlayQ = top->getNet(NLName("overlay_q"));
  ASSERT_NE(nullptr, overlayQ);
  const auto overlayType = SNLSVIntent::typeOf(overlayQ);
  ASSERT_TRUE(overlayType.valid);
  EXPECT_EQ("mini_pkg::overlay_t", overlayType.typeName);
  EXPECT_EQ("packed_union", overlayType.canonicalKind);
  ASSERT_TRUE(overlayType.isStruct);
  EXPECT_EQ(8u, overlayType.structWidth);
  ASSERT_EQ(2u, overlayType.fields.size());
  EXPECT_EQ(7u, overlayType.fields[0].msb);
  EXPECT_EQ(0u, overlayType.fields[0].lsb);
  EXPECT_EQ(7u, overlayType.fields[1].msb);
  EXPECT_EQ(0u, overlayType.fields[1].lsb);

  auto* synthetic = SNLScalarNet::create(top, NLName("synthetic"));
  EXPECT_FALSE(SNLSVIntent::typeOf(synthetic).valid);
}

TEST_F(SNLSVIntentTest, returnsParametersAndPackageMembers) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  const auto params = SNLSVIntent::parametersOf(top);
  ASSERT_TRUE(params.valid);
  EXPECT_EQ("intent_mini", params.module);
  EXPECT_EQ(2u, params.params.size());

  std::map<std::string, SNLSVIntentParam> byName;
  for (const auto& param : params.params) {
    byName.emplace(param.name, param);
  }
  ASSERT_TRUE(byName.contains("DEPTH"));
  ASSERT_TRUE(byName.contains("IDX_W"));
  EXPECT_FALSE(byName["DEPTH"].localParam);
  EXPECT_EQ("4", byName["DEPTH"].value);
  EXPECT_EQ("4", byName["DEPTH"].expr);
  EXPECT_TRUE(byName["IDX_W"].localParam);
  EXPECT_EQ("2", byName["IDX_W"].value);
  EXPECT_EQ("$clog2(DEPTH)", byName["IDX_W"].expr);

  const auto plen = SNLSVIntent::packageMember("mini_pkg", "PLEN");
  EXPECT_EQ("PLEN", plen.name);
  EXPECT_EQ("34", plen.value);
  EXPECT_EQ("(32 == 32) ? 34 : 56", plen.expr);
  EXPECT_TRUE(plen.localParam);

  const auto packageType = SNLSVIntent::packageMemberType("mini_pkg", "state_e");
  ASSERT_TRUE(packageType.valid);
  EXPECT_EQ("mini_pkg::state_e", packageType.typeName);
  EXPECT_TRUE(packageType.isEnum);

  const std::map<std::string, std::string> typeKinds {
    {"integer_t", "integer"},
    {"scalar_t", "scalar"},
    {"floating_t", "floating"},
    {"byte_t", "packed_array"},
    {"unpacked_array_t", "unpacked_array"},
    {"dynamic_array_t", "dynamic_array"},
    {"associative_array_t", "associative_array"},
    {"queue_t", "queue"},
    {"payload_t", "packed_struct"},
    {"unpacked_struct_t", "unpacked_struct"},
    {"overlay_t", "packed_union"},
    {"unpacked_union_t", "unpacked_union"},
    {"class_t", "class"},
    {"string_t", "string"},
    {"event_t", "event"},
    {"virtual_interface_t", "virtual_interface"},
    {"chandle_t", "unknown"},
  };
  for (const auto& [name, expectedKind] : typeKinds) {
    const auto memberType = SNLSVIntent::packageMemberType("mini_pkg", name);
    ASSERT_TRUE(memberType.valid) << name;
    EXPECT_EQ(expectedKind, memberType.canonicalKind) << name;
  }
  const auto voidType =
    SNLSVIntent::packageMemberType("mini_pkg", "do_nothing");
  ASSERT_TRUE(voidType.valid);
  EXPECT_EQ("void", voidType.canonicalKind);

  auto* child = top->getInstance(NLName("u_child"));
  ASSERT_NE(nullptr, child);
  const auto childParams = SNLSVIntent::parametersOf(child);
  ASSERT_TRUE(childParams.valid);
  EXPECT_EQ("intent_child", childParams.module);
  ASSERT_EQ(1u, childParams.params.size());
  EXPECT_EQ("WIDTH", childParams.params[0].name);
  EXPECT_EQ("4", childParams.params[0].value);
}

TEST_F(SNLSVIntentTest, returnsInvalidForUnsupportedQueries) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  EXPECT_FALSE(SNLSVIntent::typeOf(static_cast<const NLObject*>(nullptr)).valid);
  EXPECT_FALSE(
    SNLSVIntent::parametersOf(static_cast<const NLObject*>(nullptr)).valid);
  EXPECT_FALSE(SNLSVIntent::typeOf(top).valid);

  auto* stateQ = top->getNet(NLName("state_q"));
  ASSERT_NE(nullptr, stateQ);
  EXPECT_FALSE(SNLSVIntent::parametersOf(stateQ).valid);

  auto* synthetic = SNLScalarNet::create(top, NLName("unlinked_synthetic"));
  EXPECT_FALSE(SNLSVIntent::typeOf(synthetic).valid);
  EXPECT_FALSE(SNLSVIntent::parametersOf(synthetic).valid);

  EXPECT_FALSE(SNLSVIntent::packageMemberType("missing_pkg", "state_e").valid);
  EXPECT_FALSE(SNLSVIntent::packageMemberType("mini_pkg", "missing").valid);
  EXPECT_TRUE(SNLSVIntent::packageMember("mini_pkg", "state_e").name.empty());
}

TEST_F(SNLSVIntentTest, returnsInvalidWithoutWarmLink) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);
  auto* stateQ = top->getNet(NLName("state_q"));
  ASSERT_NE(nullptr, stateQ);

  SNLSVLiveASTLinkRegistry::clear(library_->getDB());
  EXPECT_FALSE(SNLSVIntent::available());
  EXPECT_FALSE(SNLSVIntent::typeOf(stateQ).valid);
  EXPECT_FALSE(SNLSVIntent::parametersOf(top).valid);
  EXPECT_TRUE(SNLSVIntent::packageMember("mini_pkg", "PLEN").name.empty());
  EXPECT_FALSE(SNLSVIntent::packageMemberType("mini_pkg", "state_e").valid);
}

TEST_F(SNLSVIntentTest, liveASTLinkRegistryEdgeCases) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  auto* db = library_->getDB();
  const auto* retainedLink = SNLSVLiveASTLinkRegistry::get(db);
  ASSERT_NE(nullptr, retainedLink);
  ASSERT_EQ(retainedLink, SNLSVLiveASTLinkRegistry::getLatest());
  ASSERT_NE(nullptr, retainedLink->getCompilation());

  auto* liveASTLinkProperty = db->getProperty(SNLSVLiveASTLinkProperty::Name);
  ASSERT_NE(nullptr, liveASTLinkProperty);
  EXPECT_EQ(SNLSVLiveASTLinkProperty::Name, liveASTLinkProperty->getName());
  EXPECT_EQ(SNLSVLiveASTLinkProperty::Name, liveASTLinkProperty->getString());

  auto* stateQ = top->getNet(NLName("state_q"));
  auto* plainQ = top->getNet(NLName("plain_q"));
  ASSERT_NE(nullptr, stateQ);
  ASSERT_NE(nullptr, plainQ);
  const auto* stateSymbol = retainedLink->getSymbol(stateQ);
  const auto* plainSymbol = retainedLink->getSymbol(plainQ);
  ASSERT_NE(nullptr, stateSymbol);
  ASSERT_NE(nullptr, plainSymbol);
  ASSERT_NE(stateSymbol, plainSymbol);

  SNLSVLiveASTLink localLink;
  localLink.bind(nullptr, *stateSymbol);
  EXPECT_EQ(nullptr, localLink.getSymbol(nullptr));
  EXPECT_TRUE(localLink.getObjects(stateSymbol).empty());

  localLink.bind(stateQ, *stateSymbol);
  localLink.bind(plainQ, *stateSymbol);
  ASSERT_EQ(2u, localLink.getObjects(stateSymbol).size());

  // Rebinding stateQ leaves plainQ on the old symbol; rebinding plainQ then
  // removes the old symbol entry entirely.
  localLink.bind(stateQ, *plainSymbol);
  ASSERT_EQ(1u, localLink.getObjects(stateSymbol).size());
  EXPECT_EQ(plainQ, localLink.getObjects(stateSymbol).front());
  localLink.bind(plainQ, *plainSymbol);
  EXPECT_TRUE(localLink.getObjects(stateSymbol).empty());
  ASSERT_EQ(2u, localLink.getObjects(plainSymbol).size());

  // Binding an object to the same symbol twice is idempotent.
  localLink.bind(plainQ, *plainSymbol);
  EXPECT_EQ(2u, localLink.getObjects(plainSymbol).size());

  EXPECT_EQ(retainedLink, SNLSVLiveASTLinkRegistry::findForObject(db));
  EXPECT_EQ(retainedLink, SNLSVLiveASTLinkRegistry::findForObject(top));
  EXPECT_EQ(retainedLink, SNLSVLiveASTLinkRegistry::findForObject(stateQ));
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::findForObject(nullptr));
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::findForObject(library_));
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::findForObject(NLUniverse::get()));
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::findForSymbol(nullptr));
  EXPECT_EQ(retainedLink, SNLSVLiveASTLinkRegistry::findForSymbol(stateSymbol));

  const auto* package = retainedLink->getCompilation()->getPackage("mini_pkg");
  ASSERT_NE(nullptr, package);
  const auto* plenSymbol = package->find("PLEN");
  ASSERT_NE(nullptr, plenSymbol);
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::findForSymbol(plenSymbol));

  SNLSVLiveASTLinkRegistry::clear(nullptr);
  SNLSVLiveASTLinkRegistry::store(nullptr, std::make_unique<SNLSVLiveASTLink>());
  EXPECT_EQ(retainedLink, SNLSVLiveASTLinkRegistry::get(db));

  SNLSVLiveASTLinkRegistry::store(db, nullptr);
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::get(db));
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::getLatest());

  // No NLUniverse at all: clearAll()/findForSymbol() must degrade to safe
  // no-ops instead of dereferencing a null universe. stateSymbol's pointer
  // *value* stays safe to pass here (never dereferenced on this early-return
  // path) even though its pointee (the destroyed universe's retained
  // compilation) is gone by now.
  NLUniverse::get()->destroy();
  SNLSVLiveASTLinkRegistry::clearAll();
  EXPECT_EQ(nullptr, SNLSVLiveASTLinkRegistry::findForSymbol(stateSymbol));
  NLUniverse::create();  // TearDown() expects a universe to destroy
}
