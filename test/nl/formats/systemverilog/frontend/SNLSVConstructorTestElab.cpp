// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>

#include "NLUniverse.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

class SNLSVConstructorTestElab: public ::testing::Test {
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
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestElab, elaborateParameterizedPorts) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"param_inst"/"param_inst.sv");

  auto leaf = library_->getSNLDesign(NLName("leaf"));
  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(leaf, nullptr);
  ASSERT_NE(top, nullptr);

  auto leafA = leaf->getBusTerm(NLName("a"));
  auto leafY = leaf->getBusTerm(NLName("y"));
  ASSERT_NE(leafA, nullptr);
  ASSERT_NE(leafY, nullptr);
  EXPECT_EQ(4, leafA->getWidth());
  EXPECT_EQ(4, leafY->getWidth());

  auto topA = top->getBusNet(NLName("a"));
  auto topY = top->getBusNet(NLName("y"));
  ASSERT_NE(topA, nullptr);
  ASSERT_NE(topY, nullptr);
  EXPECT_EQ(4, topA->getWidth());
  EXPECT_EQ(4, topY->getWidth());

  auto inst = top->getInstance(NLName("u0"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(leaf, inst->getModel());

  auto leafABit0 = leafA->getBit(0);
  auto leafYBit0 = leafY->getBit(0);
  ASSERT_NE(leafABit0, nullptr);
  ASSERT_NE(leafYBit0, nullptr);

  auto instABit0 = inst->getInstTerm(leafABit0);
  auto instYBit0 = inst->getInstTerm(leafYBit0);
  ASSERT_NE(instABit0, nullptr);
  ASSERT_NE(instYBit0, nullptr);
  EXPECT_EQ(topA->getBit(0), instABit0->getNet());
  EXPECT_EQ(topY->getBit(0), instYBit0->getNet());

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "elaborateParameterizedPorts";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
}

TEST_F(SNLSVConstructorTestElab, elaborateInstanceArrays) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "instance_arrays" / "instance_arrays.sv");

  auto leaf = library_->getSNLDesign(NLName("instance_array_leaf"));
  auto interfaceModel =
    library_->getSNLDesign(NLName("instance_array_if"));
  auto top = library_->getSNLDesign(NLName("instance_arrays_top"));
  ASSERT_NE(top, nullptr);
  auto* sinkInstance = top->getInstance(NLName("instance_arrays_top_gen_interfaces_0_sink"));
  ASSERT_NE(sinkInstance, nullptr);
  auto* sink = sinkInstance->getModel();
  ASSERT_NE(leaf, nullptr);
  ASSERT_NE(interfaceModel, nullptr);
  ASSERT_NE(sink, nullptr);
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(16, top->getNonAssignInstances().size());

  auto leafA = leaf->getScalarTerm(NLName("a"));
  auto leafY = leaf->getScalarTerm(NLName("y"));
  auto moduleI = top->getBusNet(NLName("module_i"));
  auto moduleO = top->getBusNet(NLName("module_o"));
  auto matrixI = top->getBusNet(NLName("matrix_i"));
  auto matrixO = top->getBusNet(NLName("matrix_o"));
  ASSERT_NE(leafA, nullptr);
  ASSERT_NE(leafY, nullptr);
  ASSERT_NE(moduleI, nullptr);
  ASSERT_NE(moduleO, nullptr);
  ASSERT_NE(matrixI, nullptr);
  ASSERT_NE(matrixO, nullptr);

  for (NLID::Bit i = 0; i < 4; ++i) {
    auto* inst = top->getInstance(
      NLName("instance_arrays_top_u_" + std::to_string(i)));
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(leaf, inst->getModel());
    const auto bit = static_cast<NLID::Bit>(3 - i);
    EXPECT_EQ(moduleI->getBit(bit), inst->getInstTerm(leafA)->getNet());
    EXPECT_EQ(moduleO->getBit(bit), inst->getInstTerm(leafY)->getNet());
  }

  for (NLID::Bit i = 0; i < 2; ++i) {
    for (NLID::Bit j = 0; j < 3; ++j) {
      auto* inst = top->getInstance(
        NLName(
          "instance_arrays_top_matrix_" + std::to_string(i) + "_" +
          std::to_string(j)));
      ASSERT_NE(inst, nullptr);
      EXPECT_EQ(leaf, inst->getModel());
      auto* instANet = inst->getInstTerm(leafA)->getNet();
      auto* instYNet = inst->getInstTerm(leafY)->getNet();
      bool inputConnected = false;
      bool outputConnected = false;
      for (auto* bit : matrixI->getBits()) {
        inputConnected = inputConnected || bit == instANet;
      }
      for (auto* bit : matrixO->getBits()) {
        outputConnected = outputConnected || bit == instYNet;
      }
      EXPECT_TRUE(inputConnected) << "matrix[" << i << "][" << j << "]";
      EXPECT_TRUE(outputConnected) << "matrix[" << i << "][" << j << "]";
    }
  }

  auto sinkBusSig = sink->getScalarTerm(NLName("bus__sig"));
  auto sinkY = sink->getScalarTerm(NLName("y"));
  auto interfaceO = top->getBusNet(NLName("interface_o"));
  ASSERT_NE(sinkBusSig, nullptr);
  ASSERT_NE(sinkY, nullptr);
  ASSERT_NE(interfaceO, nullptr);
  for (NLID::Bit i = 0; i < 2; ++i) {
    for (NLID::Bit j = 0; j < 2; ++j) {
      auto* interfaceInst = top->getInstance(
        NLName(
          "instance_arrays_top_gen_interfaces_" + std::to_string(i) +
          "_buses_" + std::to_string(j)));
      ASSERT_NE(interfaceInst, nullptr);
      EXPECT_EQ(interfaceModel, interfaceInst->getModel());
    }

    auto* sinkInst = top->getInstance(
      NLName(
        "instance_arrays_top_gen_interfaces_" + std::to_string(i) +
        "_sink"));
    ASSERT_NE(sinkInst, nullptr);
    EXPECT_EQ(sink, sinkInst->getModel());
    EXPECT_NE(nullptr, sinkInst->getInstTerm(sinkBusSig)->getNet());
    EXPECT_EQ(interfaceO->getBit(i), sinkInst->getInstTerm(sinkY)->getNet());
  }
}
