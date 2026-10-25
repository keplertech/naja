// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NetlistGraph.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "NLLibraryTruthTables.h"
#include "SNLDesignModeling.h"
#include "ConstantPropagation.h"
#include "NLDB0.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"

using namespace naja;
using namespace naja::NL;
using namespace naja::NAJA_OPT;

namespace {

void executeCommand(const std::string& command) {
#ifdef NAJA_ENABLE_TEST_DOT
  int result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Command execution failed." << std::endl;
  }
#else
  (void) command;
#endif
}

}

class ConstantPropagationTests : public ::testing::Test {
 protected:
  void SetUp() override {
    univ = NLUniverse::create();
    db = NLDB::create(univ);
    standardLibrary = NLLibrary::create(
        db, NLLibrary::Type::Standard, NLName("standard"));
    library = NLLibrary::create(
        db, NLLibrary::Type::Primitives, NLName("nangate45"));
    top = SNLDesign::create(
        standardLibrary, SNLDesign::Type::Standard, NLName("top"));
    univ->setTopDesign(top);

    logic0 = SNLDesign::create(
        library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
    logic0Out = SNLScalarTerm::create(
        logic0, SNLTerm::Direction::Output, NLName("out"));
    SNLDesignModeling::setTruthTable(
        logic0,
        SNLTruthTable(0, 0, SNLTruthTable::fullDependencies(0)));

    logic1 = SNLDesign::create(
        library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
    logic1Out = SNLScalarTerm::create(
        logic1, SNLTerm::Direction::Output, NLName("out"));
    SNLDesignModeling::setTruthTable(
        logic1,
        SNLTruthTable(0, 1, SNLTruthTable::fullDependencies(0)));
    NLLibraryTruthTables::construct(library);
  }

  void TearDown() override {
    NLUniverse::get()->destroy();
    naja::DNL::destroy();
  }

  NLUniverse* univ {nullptr};
  NLDB* db {nullptr};
  NLLibrary* standardLibrary {nullptr};
  NLLibrary* library {nullptr};
  SNLDesign* top {nullptr};
  SNLDesign* logic0 {nullptr};
  SNLScalarTerm* logic0Out {nullptr};
  SNLDesign* logic1 {nullptr};
  SNLScalarTerm* logic1Out {nullptr};
};

// Test constat propagation
// 1. Create SNL
// 2. Create a top model with one output
// 3. create a logic_0 model
// 4. create a logic_1 model
// 5. create a logic_0 instace in top
// 6. create a logic_1 instace in top
// 7. create a and model
// 8. create a and instance in top
// 9. connect the logic_0 and logic_1 to the and instance
// 10. connect the and instance output to the top output
// 11. create DNL
// 12. create a constant propagation object
// 13. collect the constants
// 14. run the constant propagation
// 15. check the output value of the top instance
TEST_F(ConstantPropagationTests, TestConstantPropagation) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  
  
  
  
  // 5. create a logic_0 instace in top
  SNLInstance* instLogic0 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* instLogic1 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // 7. create a and model for or
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to and
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::Output,
                                      NLName("out"));  
  // 7. create a and model for xor
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to and
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nand
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to and
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nor
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to and
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for xnor
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to and
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for inv
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to and
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for buf
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to and
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for ha
  SNLDesign* haModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("HA"));

  // add 2 inputs and 2 outputs to and
  auto haIn1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto haIn2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto haOut1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out1")); 
  auto haOut2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out2"));   
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 7. create a and model for oai  
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));

  // create a and instance in top
  SNLInstance* instAnd = SNLInstance::create(top, andModel, NLName("and"));
  // create a or instance in top
  SNLInstance* instOr = SNLInstance::create(top, orModel, NLName("or"));
  // create a xor instance in top
  SNLInstance* instxOr = SNLInstance::create(top, xorModel, NLName("xor"));
  // create a nand instance in top
  SNLInstance* instnAnd = SNLInstance::create(top, nandModel, NLName("nand"));
  // create a nor instance in top
  SNLInstance* instNor = SNLInstance::create(top, norModel, NLName("nor"));
  // create a xnor instance in top
  SNLInstance* instXnor = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // create a inv instance in top
  SNLInstance* instInv = SNLInstance::create(top, invModel, NLName("inv"));
  // create a buf instance in top
  SNLInstance* instBuf = SNLInstance::create(top, bufModel, NLName("buf"));
  // create a ha instance in top
  SNLInstance* instHa = SNLInstance::create(top, haModel, NLName("ha"));
  // create a dff instance in top
  SNLInstance* instDff = SNLInstance::create(top, dffModel, NLName("dff"));
  // create a mux instance in top
  SNLInstance* instMux = SNLInstance::create(top, muxModel, NLName("mux"));
  // create a oai instance in top
  SNLInstance* instOai = SNLInstance::create(top, oaiModel, NLName("oai"));

  // 9. connect all instances inputs 
  SNLNet* netlogic0 = SNLScalarNet::create(top, NLName("logic_0_net"));
  netlogic0->setType(SNLNet::Type::Assign0);
  SNLNet* netlogic1 = SNLScalarNet::create(top, NLName("logic_1_net"));
  netlogic1->setType(SNLNet::Type::Assign1);
  SNLNet* netand = SNLScalarNet::create(top, NLName("and_output_net"));
  // connect logic0 to and
  instLogic0->getInstTerm(logic0Out)->setNet(netlogic0);
  instAnd->getInstTerm(andIn1)->setNet(netlogic0);
  instOr->getInstTerm(orIn1)->setNet(netlogic0);
  instxOr->getInstTerm(xorIn1)->setNet(netlogic0);
  instnAnd->getInstTerm(nandIn1)->setNet(netlogic0);
  instNor->getInstTerm(norIn1)->setNet(netlogic0);
  instXnor->getInstTerm(xnorIn1)->setNet(netlogic0);
  instInv->getInstTerm(invIn)->setNet(netlogic0);
  instBuf->getInstTerm(bufIn)->setNet(netlogic0);
  instHa->getInstTerm(haIn1)->setNet(netlogic0);
  instDff->getInstTerm(dffD)->setNet(netlogic0);
  instMux->getInstTerm(muxS)->setNet(netlogic0);
  instOai->getInstTerm(oaiA)->setNet(netlogic0);

  // connect logic1 to all gate inputs
  instLogic1->getInstTerm(logic1Out)->setNet(netlogic1);
  instAnd->getInstTerm(andIn2)->setNet(netlogic1);
  instOr->getInstTerm(orIn2)->setNet(netlogic1);
  instxOr->getInstTerm(xorIn2)->setNet(netlogic1);
  instnAnd->getInstTerm(nandIn2)->setNet(netlogic1);
  instNor->getInstTerm(norIn2)->setNet(netlogic1);
  instXnor->getInstTerm(xnorIn2)->setNet(netlogic1);
  instInv->getInstTerm(invIn)->setNet(netlogic1);
  instBuf->getInstTerm(bufIn)->setNet(netlogic1);
  instHa->getInstTerm(haIn2)->setNet(netlogic1);
  instDff->getInstTerm(dffCLK)->setNet(netlogic1);
  instMux->getInstTerm(muxA)->setNet(netlogic1);
  instMux->getInstTerm(muxB)->setNet(netlogic1);
  instOai->getInstTerm(oaiB1)->setNet(netlogic1);
  instOai->getInstTerm(oaiB2)->setNet(netlogic1);

  // 10. connect the and instance output to the top output
  instAnd->getInstTerm(andOut)->setNet(netand);
  topOut->setNet(netand);
  //inst4->getInstTerm(orOut)->setNet(net4);
  //topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);   
  // 13. collect the constants
  ////cp.collectConstants();
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}

TEST_F(ConstantPropagationTests, TestConstantPropagationNonBNE) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  
  
  
  
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // 7. create a and model for or
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to and
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::Output,
                                      NLName("out"));  
  // 7. create a and model for xor
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to and
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nand
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to and
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nor
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to and
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for xnor
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to and
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for inv
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to and
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for buf
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to and
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for ha
  SNLDesign* haModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("HA"));

  // add 2 inputs and 2 outputs to and
  auto haIn1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto haIn2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto haOut1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out1")); 
  auto haOut2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out2"));   
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 7. create a and model for oai  
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));

  // create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, NLName("and"));
  // create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, NLName("or"));
  // create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, NLName("xor"));
  // create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, NLName("nand"));
  // create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, NLName("nor"));
  // create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, NLName("inv"));
  // create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, NLName("buf"));
  // create a ha instance in top
  SNLInstance* inst11 = SNLInstance::create(top, haModel, NLName("ha"));
  // create a dff instance in top
  SNLInstance* inst12 = SNLInstance::create(top, dffModel, NLName("dff"));
  // create a mux instance in top
  SNLInstance* inst13 = SNLInstance::create(top, muxModel, NLName("mux"));
  // create a oai instance in top
  SNLInstance* inst14 = SNLInstance::create(top, oaiModel, NLName("oai"));

  // 9. connect all instances inputs 
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("and_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net1);
  inst4->getInstTerm(orIn1)->setNet(net1);
  inst5->getInstTerm(xorIn1)->setNet(net1);
  inst6->getInstTerm(nandIn1)->setNet(net1);
  inst7->getInstTerm(norIn1)->setNet(net1);
  inst8->getInstTerm(xnorIn1)->setNet(net1);
  inst9->getInstTerm(invIn)->setNet(net1);
  inst10->getInstTerm(bufIn)->setNet(net1);
  inst11->getInstTerm(haIn1)->setNet(net1);
  inst12->getInstTerm(dffD)->setNet(net1);
  inst13->getInstTerm(muxS)->setNet(net1);
  inst14->getInstTerm(oaiA)->setNet(net1);

  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net2);
  inst4->getInstTerm(orIn2)->setNet(net2);
  inst5->getInstTerm(xorIn2)->setNet(net2);
  inst6->getInstTerm(nandIn2)->setNet(net2);
  inst7->getInstTerm(norIn2)->setNet(net2);
  inst8->getInstTerm(xnorIn2)->setNet(net2);
  inst9->getInstTerm(invIn)->setNet(net2);
  inst10->getInstTerm(bufIn)->setNet(net2);
  inst11->getInstTerm(haIn2)->setNet(net2);
  inst12->getInstTerm(dffCLK)->setNet(net2);
  inst13->getInstTerm(muxA)->setNet(net2);
  inst13->getInstTerm(muxB)->setNet(net2);
  inst14->getInstTerm(oaiB1)->setNet(net2);
  inst14->getInstTerm(oaiB2)->setNet(net2);

  // 10. connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}

TEST_F(ConstantPropagationTests, TestAndFull) {
  auto topOut00 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out00"));

  auto topOut01 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out01"));
      
  auto topOut02 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out02"));    

  auto topOut03 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out03"));

  auto topOut04 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out04"));

  auto topOut05 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out05"));


  auto topOut06 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out06"));

  auto topOut07 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out07"));        

  auto topOut08 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out08"));

  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("in"));    

  
  
  

  
  // 5. create a logic_0 instace in top
  SNLInstance* instLogic0 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* instLogic1 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model



  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      NLName("out"));


  SNLDesignModeling::setTruthTable(andModel, SNLTruthTable(2, 8, SNLTruthTable::fullDependencies(2)));
  NLLibraryTruthTables::construct(library);
  
  // SNLDesign* andModel1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND1"));
  // // add 2 inputs and 1 output to and
  // auto and1In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("1in1"));
  // auto and1In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("1in2"));
  // auto and1Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("1out"));            


  // SNLDesign* andModel2 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND2"));
  // // add 2 inputs and 1 output to and
  // auto and2In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("2in1"));
  // auto and2In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("2in2"));
  // auto and2Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("2out"));                                                                
  // //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // // 7. create a and model for or
  // SNLDesign* andModel3 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND3"));
  // // add 2 inputs and 1 output to and
  // auto and3In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("3in1"));
  // auto and3In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("3in2"));
  // auto and3Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("3out"));    


  // SNLDesign* andModel4 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND4"));
  // // add 2 inputs and 1 output to and
  // auto and4In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("4in1"));
  // auto and4In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("4in2"));
  // auto and4Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("4out")); 

  // SNLDesign* andModel5 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND5"));
  // // add 2 inputs and 1 output to and
  // auto and5In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("5in1"));
  // auto and5In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("5in2"));
  // auto and5Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("5out"));                                           

  // SNLDesign* andModel6 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND6"));
  // // add 2 inputs and 1 output to and
  // auto and6In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("6in1"));
  // auto and6In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("6in2"));
  // auto and6Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("6out"));    

  // SNLDesign* andModel7 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND7"));
  // // add 2 inputs and 1 output to and
  // auto and7In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("7in1"));
  // auto and7In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("7in2"));
  // auto and7Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("7out"));
  
  // SNLDesign* andModel8 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND8"));
  // // add 2 inputs and 1 output to and
  // auto and8In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("8in1"));
  // auto and8In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("8in2"));
  // auto and8Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("8out"));    
  
     

  // create a and instance in top
  SNLInstance* instAnd00 = SNLInstance::create(top, andModel, NLName("and00"));

  SNLInstance* instAnd01 = SNLInstance::create(top, andModel, NLName("and01"));

  SNLInstance* instAnd02 = SNLInstance::create(top, andModel, NLName("and02"));

  SNLInstance* instAnd03 = SNLInstance::create(top, andModel, NLName("and03"));

  SNLInstance* instAnd04 = SNLInstance::create(top, andModel, NLName("and04"));

  SNLInstance* instAnd05 = SNLInstance::create(top, andModel, NLName("and05"));

  SNLInstance* instAnd06 = SNLInstance::create(top, andModel, NLName("and06"));

  SNLInstance* instAnd07 = SNLInstance::create(top, andModel, NLName("and07"));

  SNLInstance* instAnd08 = SNLInstance::create(top, andModel, NLName("and08"));





 




  
  // create a or instance in top
 

  // 9. connect all instances inputs 
  SNLNet* netlogic0 = SNLScalarNet::create(top, NLName("logic_0_net"));
  netlogic0->setType(SNLNet::Type::Assign0);
  SNLNet* netlogic1 = SNLScalarNet::create(top, NLName("logic_1_net"));
  netlogic1->setType(SNLNet::Type::Assign1);
  SNLNet* netlogicx = SNLScalarNet::create(top, NLName("logic_x_net"));
  SNLNet* netOut00 = SNLScalarNet::create(top, NLName("out00_net"));
  SNLNet* netOut01 = SNLScalarNet::create(top, NLName("out01_net"));
  SNLNet* netOut02 = SNLScalarNet::create(top, NLName("out02_net"));
  SNLNet* netOut03 = SNLScalarNet::create(top, NLName("out03_net"));
  SNLNet* netOut04 = SNLScalarNet::create(top, NLName("out04_net"));
  SNLNet* netOut05 = SNLScalarNet::create(top, NLName("out05_net"));
  SNLNet* netOut06 = SNLScalarNet::create(top, NLName("out06_net"));
  SNLNet* netOut07 = SNLScalarNet::create(top, NLName("out07_net"));
  SNLNet* netOut08 = SNLScalarNet::create(top, NLName("out08_net"));
  
  



  SNLNet* netand0 = SNLScalarNet::create(top, NLName("and_output_net"));
  SNLNet* netand1 = SNLScalarNet::create(top, NLName("and_output_net1"));
  SNLNet* netand2 = SNLScalarNet::create(top, NLName("and_output_net2"));
  SNLNet* netand3 = SNLScalarNet::create(top, NLName("and_output_net3"));
  SNLNet* netand4 = SNLScalarNet::create(top, NLName("and_output_net4"));
  SNLNet* netand5 = SNLScalarNet::create(top, NLName("and_output_net5"));
  SNLNet* netand6 = SNLScalarNet::create(top, NLName("and_output_net6"));
  SNLNet* netand7 = SNLScalarNet::create(top, NLName("and_output_net7"));
  SNLNet* netand8 = SNLScalarNet::create(top, NLName("and_output_net8"));


  // connect logic0 to and
  instLogic0->getInstTerm(logic0Out)->setNet(netlogic0);
  instAnd00->getInstTerm(andIn1)->setNet(netlogic0);
  instAnd00->getInstTerm(andIn2)->setNet(netlogic0);
  instAnd01->getInstTerm(andIn1)->setNet(netlogic0);
  instAnd02->getInstTerm(andIn1)->setNet(netlogic0);
  instAnd03->getInstTerm(andIn2)->setNet(netlogic0);
  instAnd06->getInstTerm(andIn2)->setNet(netlogic0);




  
  
  

  // connect logic1 to all gate inputs
  instLogic1->getInstTerm(logic1Out)->setNet(netlogic1);
  instAnd01->getInstTerm(andIn2)->setNet(netlogic1);
  instAnd03->getInstTerm(andIn1)->setNet(netlogic1);
  instAnd04->getInstTerm(andIn1)->setNet(netlogic1);
  instAnd04->getInstTerm(andIn2)->setNet(netlogic1);
  instAnd05->getInstTerm(andIn1)->setNet(netlogic1);
  instAnd07->getInstTerm(andIn2)->setNet(netlogic1);

  topIn->setNet(netlogicx);
  instAnd02->getInstTerm(andIn2)->setNet(netlogicx);
  instAnd05->getInstTerm(andIn2)->setNet(netlogicx);
  instAnd06->getInstTerm(andIn1)->setNet(netlogicx);
  instAnd07->getInstTerm(andIn1)->setNet(netlogicx);
  instAnd08->getInstTerm(andIn1)->setNet(netlogicx);
  instAnd08->getInstTerm(andIn2)->setNet(netlogicx);
  
  topOut00->setNet(netOut00);
  instAnd00->getInstTerm(andOut)->setNet(netOut00);
  
  topOut01->setNet(netOut01);
  instAnd01->getInstTerm(andOut)->setNet(netOut01);
  
  topOut02->setNet(netOut02);
  instAnd02->getInstTerm(andOut)->setNet(netOut02);
  
  topOut03->setNet(netOut03);
  instAnd03->getInstTerm(andOut)->setNet(netOut03);
  
  topOut04->setNet(netOut04);
  instAnd04->getInstTerm(andOut)->setNet(netOut04);
  
  topOut05->setNet(netOut05);
  instAnd05->getInstTerm(andOut)->setNet(netOut05);
  
  topOut06->setNet(netOut06);
  instAnd06->getInstTerm(andOut)->setNet(netOut06);
  
  topOut07->setNet(netOut07);
  instAnd07->getInstTerm(andOut)->setNet(netOut07);
  
  topOut08->setNet(netOut08);
  instAnd08->getInstTerm(andOut)->setNet(netOut08);


  

 

  // 10. connect the and instance output to the top output
//   instAnd00->getInstTerm(andOut)->setNet(netand);
//   topOut->setNet(netand);
  //inst4->getInstTerm(orOut)->setNet(net4);
  //topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./TestAndFullbeforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./TestAndFullbeforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);   
  // 13. collect the constants
  //cp.collectConstants();
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./TestAndFullafterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./TestAndFullafterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  EXPECT_TRUE(topOut00->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut01->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut02->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut03->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut04->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut05->getNet()->getName().getString() == std::string("out05_net"));
  EXPECT_TRUE(topOut06->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut07->getNet()->getName().getString() == std::string("out07_net"));
  EXPECT_TRUE(topOut08->getNet()->getName().getString() == std::string("out08_net"));

  
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}






//Seperate the test case above for tests for each type in the constant propagation code

// Test constat propagation for AND


TEST_F(ConstantPropagationTests, TestorFull) {
  auto topOut00 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out00"));

  auto topOut01 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out01"));
      
  auto topOut02 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out02"));    

  auto topOut03 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out03"));

  auto topOut04 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out04"));

  auto topOut05 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out05"));


  auto topOut06 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out06"));

  auto topOut07 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out07"));        

  auto topOut08 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out08"));

  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("in"));    

  
  
  

  
  // 5. create a logic_0 instace in top
  SNLInstance* instLogic0 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* instLogic1 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model



  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to and
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::Output,
                                      NLName("out"));


  SNLDesignModeling::setTruthTable(orModel, SNLTruthTable(2, 14, SNLTruthTable::fullDependencies(2)));
  NLLibraryTruthTables::construct(library);
  
  // SNLDesign* andModel1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND1"));
  // // add 2 inputs and 1 output to and
  // auto and1In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("1in1"));
  // auto and1In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("1in2"));
  // auto and1Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("1out"));            


  // SNLDesign* andModel2 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND2"));
  // // add 2 inputs and 1 output to and
  // auto and2In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("2in1"));
  // auto and2In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("2in2"));
  // auto and2Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("2out"));                                                                
  // //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // // 7. create a and model for or
  // SNLDesign* andModel3 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND3"));
  // // add 2 inputs and 1 output to and
  // auto and3In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("3in1"));
  // auto and3In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("3in2"));
  // auto and3Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("3out"));    


  // SNLDesign* andModel4 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND4"));
  // // add 2 inputs and 1 output to and
  // auto and4In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("4in1"));
  // auto and4In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("4in2"));
  // auto and4Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("4out")); 

  // SNLDesign* andModel5 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND5"));
  // // add 2 inputs and 1 output to and
  // auto and5In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("5in1"));
  // auto and5In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("5in2"));
  // auto and5Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("5out"));                                           

  // SNLDesign* andModel6 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND6"));
  // // add 2 inputs and 1 output to and
  // auto and6In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("6in1"));
  // auto and6In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("6in2"));
  // auto and6Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("6out"));    

  // SNLDesign* andModel7 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND7"));
  // // add 2 inputs and 1 output to and
  // auto and7In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("7in1"));
  // auto and7In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("7in2"));
  // auto and7Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("7out"));
  
  // SNLDesign* andModel8 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND8"));
  // // add 2 inputs and 1 output to and
  // auto and8In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("8in1"));
  // auto and8In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("8in2"));
  // auto and8Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("8out"));    
  
     

  // create a and instance in top
  SNLInstance* instOr00 = SNLInstance::create(top, orModel, NLName("or00"));

  SNLInstance* instOr01 = SNLInstance::create(top, orModel, NLName("or01"));

  SNLInstance* instOr02 = SNLInstance::create(top, orModel, NLName("or02"));

  SNLInstance* instOr03 = SNLInstance::create(top, orModel, NLName("or03"));

  SNLInstance* instOr04 = SNLInstance::create(top, orModel, NLName("or04"));

  SNLInstance* instOr05 = SNLInstance::create(top, orModel, NLName("or05"));

  SNLInstance* instOr06 = SNLInstance::create(top, orModel, NLName("or06"));

  SNLInstance* instOr07 = SNLInstance::create(top, orModel, NLName("or07"));

  SNLInstance* instOr08 = SNLInstance::create(top, orModel, NLName("or08"));





 




  
  // create a or instance in top
 

  // 9. connect all instances inputs 
  SNLNet* netlogic0 = SNLScalarNet::create(top, NLName("logic_0_net"));
  netlogic0->setType(SNLNet::Type::Assign0);
  SNLNet* netlogic1 = SNLScalarNet::create(top, NLName("logic_1_net"));
  netlogic1->setType(SNLNet::Type::Assign1);
  SNLNet* netlogicx = SNLScalarNet::create(top, NLName("logic_x_net"));
  SNLNet* netOut00 = SNLScalarNet::create(top, NLName("out00_net"));
  SNLNet* netOut01 = SNLScalarNet::create(top, NLName("out01_net"));
  SNLNet* netOut02 = SNLScalarNet::create(top, NLName("out02_net"));
  SNLNet* netOut03 = SNLScalarNet::create(top, NLName("out03_net"));
  SNLNet* netOut04 = SNLScalarNet::create(top, NLName("out04_net"));
  SNLNet* netOut05 = SNLScalarNet::create(top, NLName("out05_net"));
  SNLNet* netOut06 = SNLScalarNet::create(top, NLName("out06_net"));
  SNLNet* netOut07 = SNLScalarNet::create(top, NLName("out07_net"));
  SNLNet* netOut08 = SNLScalarNet::create(top, NLName("out08_net"));
  
  



  SNLNet* netor0 = SNLScalarNet::create(top, NLName("or_output_net"));
  SNLNet* netor1 = SNLScalarNet::create(top, NLName("or_output_net1"));
  SNLNet* netor2 = SNLScalarNet::create(top, NLName("or_output_net2"));
  SNLNet* netor3 = SNLScalarNet::create(top, NLName("or_output_net3"));
  SNLNet* netor4 = SNLScalarNet::create(top, NLName("or_output_net4"));
  SNLNet* netor5 = SNLScalarNet::create(top, NLName("or_output_net5"));
  SNLNet* netor6 = SNLScalarNet::create(top, NLName("or_output_net6"));
  SNLNet* netor7 = SNLScalarNet::create(top, NLName("or_output_net7"));
  SNLNet* netor8 = SNLScalarNet::create(top, NLName("or_output_net8"));


  // connect logic0 to and
  instLogic0->getInstTerm(logic0Out)->setNet(netlogic0);
  instOr00->getInstTerm(orIn1)->setNet(netlogic0);
  instOr00->getInstTerm(orIn2)->setNet(netlogic0);
  instOr01->getInstTerm(orIn1)->setNet(netlogic0);
  instOr02->getInstTerm(orIn1)->setNet(netlogic0);
  instOr03->getInstTerm(orIn2)->setNet(netlogic0);
  instOr06->getInstTerm(orIn2)->setNet(netlogic0);




  
  
  

  // connect logic1 to all gate inputs
  instLogic1->getInstTerm(logic1Out)->setNet(netlogic1);
  instOr01->getInstTerm(orIn2)->setNet(netlogic1);
  instOr03->getInstTerm(orIn1)->setNet(netlogic1);
  instOr04->getInstTerm(orIn1)->setNet(netlogic1);
  instOr04->getInstTerm(orIn2)->setNet(netlogic1);
  instOr05->getInstTerm(orIn1)->setNet(netlogic1);
  instOr07->getInstTerm(orIn2)->setNet(netlogic1);

  topIn->setNet(netlogicx);
  instOr02->getInstTerm(orIn2)->setNet(netlogicx);
  instOr05->getInstTerm(orIn2)->setNet(netlogicx);
  instOr06->getInstTerm(orIn1)->setNet(netlogicx);
  instOr07->getInstTerm(orIn1)->setNet(netlogicx);
  instOr08->getInstTerm(orIn1)->setNet(netlogicx);
  instOr08->getInstTerm(orIn2)->setNet(netlogicx);
  
  topOut00->setNet(netOut00);
  instOr00->getInstTerm(orOut)->setNet(netOut00);
  
  topOut01->setNet(netOut01);
  instOr01->getInstTerm(orOut)->setNet(netOut01);
  
  topOut02->setNet(netOut02);
  instOr02->getInstTerm(orOut)->setNet(netOut02);
  
  topOut03->setNet(netOut03);
  instOr03->getInstTerm(orOut)->setNet(netOut03);
  
  topOut04->setNet(netOut04);
  instOr04->getInstTerm(orOut)->setNet(netOut04);
  
  topOut05->setNet(netOut05);
  instOr05->getInstTerm(orOut)->setNet(netOut05);
  
  topOut06->setNet(netOut06);
  instOr06->getInstTerm(orOut)->setNet(netOut06);
  
  topOut07->setNet(netOut07);
  instOr07->getInstTerm(orOut)->setNet(netOut07);
  
  topOut08->setNet(netOut08);
  instOr08->getInstTerm(orOut)->setNet(netOut08);


  

 

  // 10. connect the and instance output to the top output
//   instAnd00->getInstTerm(andOut)->setNet(netand);
//   topOut->setNet(netand);
  //inst4->getInstTerm(orOut)->setNet(net4);
  //topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./TestOrFullbeforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./TestOrFullbeforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);   
  // 13. collect the constants
  //cp.collectConstants();
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./TestOrFullafterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./TestOrFullafterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  EXPECT_TRUE(topOut00->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut01->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut02->getNet()->getName().getString() == std::string("out02_net"));
  EXPECT_TRUE(topOut03->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut04->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut05->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut06->getNet()->getName().getString() == std::string("out06_net"));
  EXPECT_TRUE(topOut07->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut08->getNet()->getName().getString() == std::string("out08_net"));

  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}



TEST_F(ConstantPropagationTests, TestNorFull) {
  auto topOut00 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out00"));

  auto topOut01 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out01"));
      
  auto topOut02 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out02"));    

  auto topOut03 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out03"));

  auto topOut04 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out04"));

  auto topOut05 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out05"));


  auto topOut06 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out06"));

  auto topOut07 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out07"));        

  auto topOut08 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out08"));

  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("in"));    

  
  
  

  
  // 5. create a logic_0 instace in top
  SNLInstance* instLogic0 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* instLogic1 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model



  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to and
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output,
                                      NLName("out"));


  SNLDesignModeling::setTruthTable(norModel, SNLTruthTable(2, 1, SNLTruthTable::fullDependencies(2)));
  NLLibraryTruthTables::construct(library);
  
  // SNLDesign* andModel1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND1"));
  // // add 2 inputs and 1 output to and
  // auto and1In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("1in1"));
  // auto and1In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("1in2"));
  // auto and1Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("1out"));            


  // SNLDesign* andModel2 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND2"));
  // // add 2 inputs and 1 output to and
  // auto and2In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("2in1"));
  // auto and2In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("2in2"));
  // auto and2Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("2out"));                                                                
  // //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // // 7. create a and model for or
  // SNLDesign* andModel3 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND3"));
  // // add 2 inputs and 1 output to and
  // auto and3In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("3in1"));
  // auto and3In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("3in2"));
  // auto and3Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("3out"));    


  // SNLDesign* andModel4 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND4"));
  // // add 2 inputs and 1 output to and
  // auto and4In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("4in1"));
  // auto and4In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("4in2"));
  // auto and4Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("4out")); 

  // SNLDesign* andModel5 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND5"));
  // // add 2 inputs and 1 output to and
  // auto and5In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("5in1"));
  // auto and5In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("5in2"));
  // auto and5Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("5out"));                                           

  // SNLDesign* andModel6 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND6"));
  // // add 2 inputs and 1 output to and
  // auto and6In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("6in1"));
  // auto and6In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("6in2"));
  // auto and6Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("6out"));    

  // SNLDesign* andModel7 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND7"));
  // // add 2 inputs and 1 output to and
  // auto and7In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("7in1"));
  // auto and7In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("7in2"));
  // auto and7Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("7out"));
  
  // SNLDesign* andModel8 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND8"));
  // // add 2 inputs and 1 output to and
  // auto and8In1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("8in1"));
  // auto and8In2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
  //                                     NLName("8in2"));
  // auto and8Out = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
  //                                     NLName("8out"));    
  
     

  // create a and instance in top
  SNLInstance* instNor00 = SNLInstance::create(top, norModel, NLName("nor00"));

  SNLInstance* instNor01 = SNLInstance::create(top, norModel, NLName("nor01"));

  SNLInstance* instNor02 = SNLInstance::create(top, norModel, NLName("nor02"));

  SNLInstance* instNor03 = SNLInstance::create(top, norModel, NLName("nor03"));

  SNLInstance* instNor04 = SNLInstance::create(top, norModel, NLName("nor04"));

  SNLInstance* instNor05 = SNLInstance::create(top, norModel, NLName("nor05"));

  SNLInstance* instNor06 = SNLInstance::create(top, norModel, NLName("nor06"));

  SNLInstance* instNor07 = SNLInstance::create(top, norModel, NLName("nor07"));

  SNLInstance* instNor08 = SNLInstance::create(top, norModel, NLName("nor08"));





 




  
  // create a or instance in top
 

  // 9. connect all instances inputs 
  SNLNet* netlogic0 = SNLScalarNet::create(top, NLName("logic_0_net"));
  netlogic0->setType(SNLNet::Type::Assign0);
  SNLNet* netlogic1 = SNLScalarNet::create(top, NLName("logic_1_net"));
  netlogic1->setType(SNLNet::Type::Assign1);
  SNLNet* netlogicx = SNLScalarNet::create(top, NLName("logic_x_net"));
  SNLNet* netOut00 = SNLScalarNet::create(top, NLName("out00_net"));
  SNLNet* netOut01 = SNLScalarNet::create(top, NLName("out01_net"));
  SNLNet* netOut02 = SNLScalarNet::create(top, NLName("out02_net"));
  SNLNet* netOut03 = SNLScalarNet::create(top, NLName("out03_net"));
  SNLNet* netOut04 = SNLScalarNet::create(top, NLName("out04_net"));
  SNLNet* netOut05 = SNLScalarNet::create(top, NLName("out05_net"));
  SNLNet* netOut06 = SNLScalarNet::create(top, NLName("out06_net"));
  SNLNet* netOut07 = SNLScalarNet::create(top, NLName("out07_net"));
  SNLNet* netOut08 = SNLScalarNet::create(top, NLName("out08_net"));
  
  



  SNLNet* netnor0 = SNLScalarNet::create(top, NLName("nor_output_net"));
  SNLNet* netnor1 = SNLScalarNet::create(top, NLName("nor_output_net1"));
  SNLNet* netnor2 = SNLScalarNet::create(top, NLName("nor_output_net2"));
  SNLNet* netnor3 = SNLScalarNet::create(top, NLName("nor_output_net3"));
  SNLNet* netnor4 = SNLScalarNet::create(top, NLName("nor_output_net4"));
  SNLNet* netnor5 = SNLScalarNet::create(top, NLName("nor_output_net5"));
  SNLNet* netnor6 = SNLScalarNet::create(top, NLName("nor_output_net6"));
  SNLNet* netnor7 = SNLScalarNet::create(top, NLName("nor_output_net7"));
  SNLNet* netnor8 = SNLScalarNet::create(top, NLName("nor_output_net8"));


  // connect logic0 to and
  instLogic0->getInstTerm(logic0Out)->setNet(netlogic0);
  instNor00->getInstTerm(norIn1)->setNet(netlogic0);
  instNor00->getInstTerm(norIn2)->setNet(netlogic0);
  instNor01->getInstTerm(norIn1)->setNet(netlogic0);
  instNor02->getInstTerm(norIn1)->setNet(netlogic0);
  instNor03->getInstTerm(norIn2)->setNet(netlogic0);
  instNor06->getInstTerm(norIn2)->setNet(netlogic0);




  
  
  

  // connect logic1 to all gate inputs
  instLogic1->getInstTerm(logic1Out)->setNet(netlogic1);
  instNor01->getInstTerm(norIn2)->setNet(netlogic1);
  instNor03->getInstTerm(norIn1)->setNet(netlogic1);
  instNor04->getInstTerm(norIn1)->setNet(netlogic1);
  instNor04->getInstTerm(norIn2)->setNet(netlogic1);
  instNor05->getInstTerm(norIn1)->setNet(netlogic1);
  instNor07->getInstTerm(norIn2)->setNet(netlogic1);

  topIn->setNet(netlogicx);
  instNor02->getInstTerm(norIn2)->setNet(netlogicx);
  instNor05->getInstTerm(norIn2)->setNet(netlogicx);
  instNor06->getInstTerm(norIn1)->setNet(netlogicx);
  instNor07->getInstTerm(norIn1)->setNet(netlogicx);
  instNor08->getInstTerm(norIn1)->setNet(netlogicx);
  instNor08->getInstTerm(norIn2)->setNet(netlogicx);
  
  topOut00->setNet(netOut00);
  instNor00->getInstTerm(norOut)->setNet(netOut00);
  
  topOut01->setNet(netOut01);
  instNor01->getInstTerm(norOut)->setNet(netOut01);
  
  topOut02->setNet(netOut02);
  instNor02->getInstTerm(norOut)->setNet(netOut02);
  
  topOut03->setNet(netOut03);
  instNor03->getInstTerm(norOut)->setNet(netOut03);
  
  topOut04->setNet(netOut04);
  instNor04->getInstTerm(norOut)->setNet(netOut04);
  
  topOut05->setNet(netOut05);
  instNor05->getInstTerm(norOut)->setNet(netOut05);
  
  topOut06->setNet(netOut06);
  instNor06->getInstTerm(norOut)->setNet(netOut06);
  
  topOut07->setNet(netOut07);
  instNor07->getInstTerm(norOut)->setNet(netOut07);
  
  topOut08->setNet(netOut08);
  instNor08->getInstTerm(norOut)->setNet(netOut08);


  

 

  // 10. connect the and instance output to the top output
//   instAnd00->getInstTerm(andOut)->setNet(netand);
//   topOut->setNet(netand);
  //inst4->getInstTerm(orOut)->setNet(net4);
  //topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./TestNorFullbeforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./TestNorFullbeforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);   
  // 13. collect the constants
  //cp.collectConstants();
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./TestNorFullafterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./TestNorFullafterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }

  EXPECT_TRUE(topOut00->getNet()->getName().getString() == std::string("logic1_naja_net"));
  EXPECT_TRUE(topOut01->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut02->getNet()->getName().getString() == std::string("out02_net"));
  EXPECT_TRUE(topOut03->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut04->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut05->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut06->getNet()->getName().getString() == std::string("out06_net"));
  EXPECT_TRUE(topOut07->getNet()->getName().getString() == std::string("logic0_naja_net"));
  EXPECT_TRUE(topOut08->getNet()->getName().getString() == std::string("out08_net"));





  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}





//Seperate the test case above for tests for each type in the constant propagation code

// Test constat propagation for AND











TEST_F(ConstantPropagationTests, TestConstantPropagationAND) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(top, andModel, NLName("and2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("and2_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  topOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for OR
TEST_F(ConstantPropagationTests, TestConstantPropagationOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a or model
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to or
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, NLName("or"));
  SNLInstance* inst5 = SNLInstance::create(top, orModel, NLName("or2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("or_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("or2_output_net"));
  // connect logic0 to or
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst5->getInstTerm(orIn1)->setNet(net1);
  inst5->getInstTerm(orIn2)->setNet(net1);
  // connect logic1 to or
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst4->getInstTerm(orIn2)->setNet(net2);
  inst4->getInstTerm(orIn1)->setNet(net4);
  // connect the or instance output to the top output
  inst4->getInstTerm(orOut)->setNet(net3);
  topOut->setNet(net3);
  inst5->getInstTerm(orOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XOR
TEST_F(ConstantPropagationTests, TestConstantPropagationXOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xor model
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to xor
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, NLName("xor"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xor_output_net"));
  // connect logic0 to xor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst5->getInstTerm(xorIn1)->setNet(net1);
  // connect logic1 to xor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst5->getInstTerm(xorIn2)->setNet(net2);
  // connect the xor instance output to the top output
  inst5->getInstTerm(xorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NAND
TEST_F(ConstantPropagationTests, TestConstantPropagationNAND) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nand model
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to nand
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nand_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("nand2_output_net"));
  // 8. create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, NLName("nand"));
  SNLInstance* inst7 = SNLInstance::create(top, nandModel, NLName("nand2"));
  // connect logic0 to nand
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst6->getInstTerm(nandIn1)->setNet(net1);
  // connect logic1 to nand
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst6->getInstTerm(nandIn2)->setNet(net2);
  inst7->getInstTerm(nandIn1)->setNet(net2);
  inst7->getInstTerm(nandIn2)->setNet(net2);
  // connect the nand instance output to the top output
  inst6->getInstTerm(nandOut)->setNet(net3);
  topOut->setNet(net3);
  inst7->getInstTerm(nandOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NOR
TEST_F(ConstantPropagationTests, TestConstantPropagationNOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nor model
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to nor
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output, NLName("out"));  
  // 8. create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, NLName("nor"));
  SNLInstance* inst8 = SNLInstance::create(top, norModel, NLName("nor2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("nor2_output_net"));
  // connect logic0 to nor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst7->getInstTerm(norIn1)->setNet(net1);
  inst8->getInstTerm(norIn1)->setNet(net1);
  inst8->getInstTerm(norIn2)->setNet(net1);
  // connect logic1 to nor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst7->getInstTerm(norIn2)->setNet(net2);
  // connect the nor instance output to the top output
  inst7->getInstTerm(norOut)->setNet(net3);
  topOut->setNet(net3);
  inst8->getInstTerm(norOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XNOR
TEST_F(ConstantPropagationTests, TestConstantPropagationXNOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xnor model
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to xnor
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xnor_output_net"));
  // 8. create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // connect logic0 to xnor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst8->getInstTerm(xnorIn1)->setNet(net1);
  // connect logic1 to xnor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst8->getInstTerm(xnorIn2)->setNet(net2);
  // connect the xnor instance output to the top output
  inst8->getInstTerm(xnorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for INV
TEST_F(ConstantPropagationTests, TestConstantPropagationINV) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a inv model
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to inv
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, NLName("inv"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));    
  SNLNet* net3 = SNLScalarNet::create(top, NLName("inv_output_net"));
  // connect logic0 to inv
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst9->getInstTerm(invIn)->setNet(net1);
  // connect the inv instance output to the top output
  inst9->getInstTerm(invOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationBUF) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to buf
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, NLName("buf"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));
  SNLNet* net3 = SNLScalarNet::create(top, NLName("buf_output_net"));
  // connect logic0 to buf
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst10->getInstTerm(bufIn)->setNet(net1);
  // connect the buf instance output to the top output
  inst10->getInstTerm(bufOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationMUX) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, muxModel, NLName("mux"));
  SNLInstance* inst12 = SNLInstance::create(top, muxModel, NLName("mux2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("mux2_output_net"));
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(muxA)->setNet(net1);
  inst12->getInstTerm(muxB)->setNet(net1);
  inst12->getInstTerm(muxS)->setNet(net1);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(muxB)->setNet(net2);
  inst12->getInstTerm(muxA)->setNet(net2);
  inst11->getInstTerm(muxS)->setNet(net2);
  // connect the mux instance output to the top output
  inst11->getInstTerm(muxY)->setNet(net3);
  topOut->setNet(net3);
  inst12->getInstTerm(muxY)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationDFF) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, dffModel, NLName("dff"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(dffD)->setNet(net1);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(dffCLK)->setNet(net2);
  // connect the mux instance output to the top output
  inst11->getInstTerm(dffQ)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationOAI) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(oaiB1)->setNet(net2);
  inst11->getInstTerm(oaiB2)->setNet(net2);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

TEST_F(ConstantPropagationTests, TestConstantPropagationNonDefinedModel) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NonDeff"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(oaiB1)->setNet(net2);
  inst11->getInstTerm(oaiB2)->setNet(net2);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialAND) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(top, andModel, NLName("and2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("and_output_net2"));
  topIn->setNet(net4);
  topOut2->setNet(net5);
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst4->getInstTerm(andIn1)->setNet(net4);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net2);
  inst4->getInstTerm(andIn1)->setNet(net1);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for OR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a or model
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to or
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, NLName("or"));
  SNLInstance* inst5 = SNLInstance::create(top, orModel, NLName("or2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("or_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("or2_output_net"));
  topIn->setNet(net4);
  // connect logic0 to or
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst4->getInstTerm(orIn1)->setNet(net1);
  // connect logic1 to or
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst5->getInstTerm(orIn1)->setNet(net2);
  inst4->getInstTerm(orIn2)->setNet(net4);
  inst5->getInstTerm(orIn2)->setNet(net4);
  // connect the or instance output to the top output
  inst4->getInstTerm(orOut)->setNet(net3);
  topOut->setNet(net3);
  inst5->getInstTerm(orOut)->setNet(net5);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialXOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xor model
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to xor
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, NLName("xor"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to xor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst5->getInstTerm(xorIn1)->setNet(net1);
  // connect logic1 to xor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst5->getInstTerm(xorIn2)->setNet(net4);
  // connect the xor instance output to the top output
  inst5->getInstTerm(xorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NAND
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNAND) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nand model
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to nand
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nand_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("nand2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // 8. create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, NLName("nand"));
  SNLInstance* inst7 = SNLInstance::create(top, nandModel, NLName("nand2"));
  // connect logic0 to nand
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst6->getInstTerm(nandIn1)->setNet(net1);
  // connect logic1 to nand
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst7->getInstTerm(nandIn1)->setNet(net2);
  inst6->getInstTerm(nandIn2)->setNet(net4);
  inst7->getInstTerm(nandIn2)->setNet(net4);
  // connect the nand instance output to the top output
  inst6->getInstTerm(nandOut)->setNet(net3);
  topOut->setNet(net3);
  inst7->getInstTerm(nandOut)->setNet(net5);
  topOut->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nor model
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to nor
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output, NLName("out"));  
  // 8. create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, NLName("nor"));
  SNLInstance* inst8 = SNLInstance::create(top, norModel, NLName("nor2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("nor2_output_net"));
  topIn->setNet(net4);
  // connect logic0 to nor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst7->getInstTerm(norIn1)->setNet(net1);
  // connect logic1 to nor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst7->getInstTerm(norIn2)->setNet(net4);
  // connect the nor instance output to the top output
  inst7->getInstTerm(norOut)->setNet(net3);
  topOut->setNet(net3);

  inst8->getInstTerm(norIn1)->setNet(net2);
  inst8->getInstTerm(norIn2)->setNet(net4);
  inst8->getInstTerm(norOut)->setNet(net5);
  topOut2->setNet(net5);

  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XNOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialXNOR) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xnor model
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to xnor
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xnor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // 8. create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // connect logic0 to xnor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst8->getInstTerm(xnorIn1)->setNet(net1);
  // connect logic1 to xnor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst8->getInstTerm(xnorIn2)->setNet(net4);
  // connect the xnor instance output to the top output
  inst8->getInstTerm(xnorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for INV
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialINV) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a inv model
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to inv
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, NLName("inv"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));    
  SNLNet* net3 = SNLScalarNet::create(top, NLName("inv_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to inv
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst9->getInstTerm(invIn)->setNet(net4);
  // connect the inv instance output to the top output
  inst9->getInstTerm(invOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialBUF) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to buf
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, NLName("buf"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));
  SNLNet* net3 = SNLScalarNet::create(top, NLName("buf_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to buf
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst10->getInstTerm(bufIn)->setNet(net4);
  // connect the buf instance output to the top output
  inst10->getInstTerm(bufOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialMUX) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));    
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, muxModel, NLName("mux"));
  SNLInstance* inst12 = SNLInstance::create(top, muxModel, NLName("mux2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("mux2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(muxA)->setNet(net4);
  inst12->getInstTerm(muxA)->setNet(net1);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(muxB)->setNet(net2);
  inst12->getInstTerm(muxB)->setNet(net4);

  inst11->getInstTerm(muxS)->setNet(net2);
  inst11->getInstTerm(muxS)->setNet(net2);
  // connect the mux instance output to the top output
  inst11->getInstTerm(muxY)->setNet(net3);
  topOut->setNet(net3);
  inst12->getInstTerm(muxY)->setNet(net5);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialDFF) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, dffModel, NLName("dff1"));
  SNLInstance* inst12 = SNLInstance::create(top, dffModel, NLName("dff2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("dff_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("dff2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(dffCLK)->setNet(net4);
  inst12->getInstTerm(dffCLK)->setNet(net4);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(dffD)->setNet(net2);
  inst12->getInstTerm(dffD)->setNet(net1);
  // connect the mux instance output to the top output
  inst11->getInstTerm(dffQ)->setNet(net3);
  inst12->getInstTerm(dffQ)->setNet(net5);
  topOut->setNet(net3);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialOAI) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  SNLInstance* inst12 = SNLInstance::create(top, oaiModel, NLName("oai2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("oai2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  inst12->getInstTerm(oaiB2)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(oaiA)->setNet(net2);
  inst11->getInstTerm(oaiB1)->setNet(net2);

  inst11->getInstTerm(oaiB2)->setNet(net4);
  inst12->getInstTerm(oaiB1)->setNet(net4);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  inst12->getInstTerm(oaiY)->setNet(net5);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNonDefinedModel) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NON_DEFF"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  
  inst11->getInstTerm(oaiB1)->setNet(net2);
  inst11->getInstTerm(oaiB2)->setNet(net4);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
               
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND_Hierarchical_duplicated_nested_actions) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topOut3 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
  auto topOut4 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
  auto topOut5 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
  auto topOut6 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
  auto topOut7 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
  auto topOut8 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
  // 5. create a logic_0 instace in top
  SNLDesign* mod = SNLDesign::create(standardLibrary, SNLDesign::Type::Standard, NLName("mod"));
  SNLInstance* inst1 = SNLInstance::create(mod, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(mod, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  auto modOut =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  auto modOut2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
  auto modOut3 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
  auto modOut4 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(mod, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(mod, andModel, NLName("and2"));
  SNLInstance* inst5 = SNLInstance::create(mod, andModel, NLName("and3"));
  SNLInstance* inst6 = SNLInstance::create(mod, andModel, NLName("and4"));

  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(mod, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(mod, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(mod, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(mod, NLName("and2_output_net"));
  SNLNet* net5 = SNLScalarNet::create(mod, NLName("and3_output_net"));
  SNLNet* net6 = SNLScalarNet::create(mod, NLName("and4_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  inst6->getInstTerm(andIn1)->setNet(net2);
  inst6->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst6->getInstTerm(andIn2)->setNet(net1);
  inst6->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  modOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  modOut2->setNet(net4);
  inst5->getInstTerm(andOut)->setNet(net5);
  modOut3->setNet(net3);
  inst6->getInstTerm(andOut)->setNet(net6);
  modOut4->setNet(net4);

  //Create 2 mod instances in top and connect the outputs with nets to the top outptus

  SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
  SNLInstance* modIns2 = SNLInstance::create(top, mod, NLName("mod2"));

  //Create 8 nets on top
  SNLNet* netTopOut1 = SNLScalarNet::create(top, NLName("top_out_1"));
  SNLNet* netTopOut2 = SNLScalarNet::create(top, NLName("top_out_2"));
    SNLNet* netTopOut3 = SNLScalarNet::create(top, NLName("top_out_3"));
    SNLNet* netTopOut4 = SNLScalarNet::create(top, NLName("top_out_4"));
    SNLNet* netTopOut5 = SNLScalarNet::create(top, NLName("top_out_5"));
    SNLNet* netTopOut6 = SNLScalarNet::create(top, NLName("top_out_6"));
    SNLNet* netTopOut7 = SNLScalarNet::create(top, NLName("top_out_7"));
    SNLNet* netTopOut8 = SNLScalarNet::create(top, NLName("top_out_8"));

//connect all the outputs of mod insts to the top outputs
    modInst->getInstTerm(modOut)->setNet(netTopOut1);
    topOut->setNet(netTopOut1);
    modInst->getInstTerm(modOut2)->setNet(netTopOut2);
    topOut2->setNet(netTopOut2);
    modInst->getInstTerm(modOut3)->setNet(netTopOut3);
    topOut3->setNet(netTopOut3);
    modInst->getInstTerm(modOut4)->setNet(netTopOut4);
    topOut4->setNet(netTopOut4);
    modIns2->getInstTerm(modOut)->setNet(netTopOut5);
    topOut5->setNet(netTopOut5);
    modIns2->getInstTerm(modOut2)->setNet(netTopOut6);
    topOut6->setNet(netTopOut6);
    modIns2->getInstTerm(modOut3)->setNet(netTopOut7);
    topOut7->setNet(netTopOut7);
    modIns2->getInstTerm(modOut4)->setNet(netTopOut8);
    topOut8->setNet(netTopOut8);


  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(true);
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND_Hierarchical_duplicated_nested_actionsNonBNE) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topOut3 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
  auto topOut4 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
  auto topOut5 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
  auto topOut6 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
  auto topOut7 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
  auto topOut8 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
  // 5. create a logic_0 instace in top
  SNLDesign* mod = SNLDesign::create(standardLibrary, SNLDesign::Type::Standard, NLName("mod"));
  SNLInstance* inst1 = SNLInstance::create(mod, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(mod, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  auto modOut =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  auto modOut2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
  auto modOut3 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
  auto modOut4 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(mod, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(mod, andModel, NLName("and2"));
  SNLInstance* inst5 = SNLInstance::create(mod, andModel, NLName("and3"));
  SNLInstance* inst6 = SNLInstance::create(mod, andModel, NLName("and4"));

  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(mod, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(mod, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(mod, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(mod, NLName("and2_output_net"));
  SNLNet* net5 = SNLScalarNet::create(mod, NLName("and3_output_net"));
  SNLNet* net6 = SNLScalarNet::create(mod, NLName("and4_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  inst6->getInstTerm(andIn1)->setNet(net2);
  inst6->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst6->getInstTerm(andIn2)->setNet(net1);
  inst6->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  modOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  modOut2->setNet(net4);
  inst5->getInstTerm(andOut)->setNet(net5);
  modOut3->setNet(net3);
  inst6->getInstTerm(andOut)->setNet(net6);
  modOut4->setNet(net4);

  //Create 2 mod instances in top and connect the outputs with nets to the top outptus

  SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
  SNLInstance* modIns2 = SNLInstance::create(top, mod, NLName("mod2"));

  //Create 8 nets on top
  SNLNet* netTopOut1 = SNLScalarNet::create(top, NLName("top_out_1"));
  SNLNet* netTopOut2 = SNLScalarNet::create(top, NLName("top_out_2"));
    SNLNet* netTopOut3 = SNLScalarNet::create(top, NLName("top_out_3"));
    SNLNet* netTopOut4 = SNLScalarNet::create(top, NLName("top_out_4"));
    SNLNet* netTopOut5 = SNLScalarNet::create(top, NLName("top_out_5"));
    SNLNet* netTopOut6 = SNLScalarNet::create(top, NLName("top_out_6"));
    SNLNet* netTopOut7 = SNLScalarNet::create(top, NLName("top_out_7"));
    SNLNet* netTopOut8 = SNLScalarNet::create(top, NLName("top_out_8"));

//connect all the outputs of mod insts to the top outputs
    modInst->getInstTerm(modOut)->setNet(netTopOut1);
    topOut->setNet(netTopOut1);
    modInst->getInstTerm(modOut2)->setNet(netTopOut2);
    topOut2->setNet(netTopOut2);
    modInst->getInstTerm(modOut3)->setNet(netTopOut3);
    topOut3->setNet(netTopOut3);
    modInst->getInstTerm(modOut4)->setNet(netTopOut4);
    topOut4->setNet(netTopOut4);
    modIns2->getInstTerm(modOut)->setNet(netTopOut5);
    topOut5->setNet(netTopOut5);
    modIns2->getInstTerm(modOut2)->setNet(netTopOut6);
    topOut6->setNet(netTopOut6);
    modIns2->getInstTerm(modOut3)->setNet(netTopOut7);
    topOut7->setNet(netTopOut7);
    modIns2->getInstTerm(modOut4)->setNet(netTopOut8);
    topOut8->setNet(netTopOut8);


  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND_Hierarchical_duplicated_nested_actionsNonBNE2) {
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topOut3 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
  auto topOut4 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
  auto topOut5 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
  auto topOut6 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
  auto topOut7 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
  auto topOut8 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
  // 5. create a logic_0 instace in top
  SNLDesign* mod = SNLDesign::create(standardLibrary, SNLDesign::Type::Standard, NLName("mod"));
  SNLInstance* inst1 = SNLInstance::create(mod, logic0, NLName("logic0a"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(mod, logic0, NLName("logic0b"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  auto modOut =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  auto modOut2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
  auto modOut3 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
  auto modOut4 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(mod, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(mod, andModel, NLName("and2"));
  SNLInstance* inst5 = SNLInstance::create(mod, andModel, NLName("and3"));
  SNLInstance* inst6 = SNLInstance::create(mod, andModel, NLName("and4"));

  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(mod, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(mod, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(mod, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(mod, NLName("and2_output_net"));
  SNLNet* net5 = SNLScalarNet::create(mod, NLName("and3_output_net"));
  SNLNet* net6 = SNLScalarNet::create(mod, NLName("and4_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  inst6->getInstTerm(andIn1)->setNet(net2);
  inst6->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic0Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst6->getInstTerm(andIn2)->setNet(net1);
  inst6->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  modOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  modOut2->setNet(net4);
  inst5->getInstTerm(andOut)->setNet(net5);
  modOut3->setNet(net3);
  inst6->getInstTerm(andOut)->setNet(net6);
  modOut4->setNet(net4);

  //Create 2 mod instances in top and connect the outputs with nets to the top outptus

  SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
  SNLInstance* modIns2 = SNLInstance::create(top, mod, NLName("mod2"));

  //Create 8 nets on top
  SNLNet* netTopOut1 = SNLScalarNet::create(top, NLName("top_out_1"));
  SNLNet* netTopOut2 = SNLScalarNet::create(top, NLName("top_out_2"));
    SNLNet* netTopOut3 = SNLScalarNet::create(top, NLName("top_out_3"));
    SNLNet* netTopOut4 = SNLScalarNet::create(top, NLName("top_out_4"));
    SNLNet* netTopOut5 = SNLScalarNet::create(top, NLName("top_out_5"));
    SNLNet* netTopOut6 = SNLScalarNet::create(top, NLName("top_out_6"));
    SNLNet* netTopOut7 = SNLScalarNet::create(top, NLName("top_out_7"));
    SNLNet* netTopOut8 = SNLScalarNet::create(top, NLName("top_out_8"));

//connect all the outputs of mod insts to the top outputs
    modInst->getInstTerm(modOut)->setNet(netTopOut1);
    topOut->setNet(netTopOut1);
    modInst->getInstTerm(modOut2)->setNet(netTopOut2);
    topOut2->setNet(netTopOut2);
    modInst->getInstTerm(modOut3)->setNet(netTopOut3);
    topOut3->setNet(netTopOut3);
    modInst->getInstTerm(modOut4)->setNet(netTopOut4);
    topOut4->setNet(netTopOut4);
    modIns2->getInstTerm(modOut)->setNet(netTopOut5);
    topOut5->setNet(netTopOut5);
    modIns2->getInstTerm(modOut2)->setNet(netTopOut6);
    topOut6->setNet(netTopOut6);
    modIns2->getInstTerm(modOut3)->setNet(netTopOut7);
    topOut7->setNet(netTopOut7);
    modIns2->getInstTerm(modOut4)->setNet(netTopOut8);
    topOut8->setNet(netTopOut8);


  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  std::vector<SNLEquipotential> equipotentials;
    for (const auto& term : get()->getDNLTerms()) {
        if (term.isNull()) {
        continue;
        }
        equipotentials.push_back(term.getEquipotential());
    }
    std::string dotFileNameEquis(
        std::string(std::string("./testBusEquis") + std::string(".dot")));
    std::string svgFileNameEquis(
        std::string(std::string("./testBusEquis") + std::string(".svg")));
    SnlVisualiser snl2(top, equipotentials);
    snl2.process();
    snl2.getNetlistGraph().dumpDotFile(dotFileNameEquis.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileNameEquis +
                        std::string(" -o ") + svgFileNameEquis)
                .c_str());
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  ////cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

class VectorUpdateConstantPropagationTest : public ConstantPropagationTests,
    public ::testing::WithParamInterface<bool> {};

TEST_P(VectorUpdateConstantPropagationTest, PreservesClippingAndDisabledWrites) {
  auto* zero = SNLScalarNet::create(top); zero->setType(SNLNet::Type::Assign0);
  auto* one = SNLScalarNet::create(top); one->setType(SNLNet::Type::Assign1);
  auto* model = NLDB0::getOrCreateVectorUpdate({1, 3, 4, true});
  std::vector<std::pair<SNLScalarTerm*, bool>> outputs;
  for (int offset : {-4, -2, -1, 0, 1, 5}) for (bool enabled : {false, true}) {
    auto* instance = SNLInstance::create(top, model);
    instance->setTermNet(NLDB0::getVectorUpdateBase(model)->getBit(0), zero);
    for (unsigned bit = 0; bit < 3; ++bit)
      instance->setTermNet(NLDB0::getVectorUpdateData(model)->getBit(bit), ((5u >> bit) & 1) ? one : zero);
    for (unsigned bit = 0; bit < 4; ++bit)
      instance->setTermNet(NLDB0::getVectorUpdateOffset(model)->getBit(bit),
          ((unsigned(offset) >> bit) & 1) ? one : zero);
    instance->setTermNet(NLDB0::getVectorUpdateEnable(model), enabled ? one : zero);
    auto* net = SNLScalarNet::create(top);
    instance->setTermNet(NLDB0::getVectorUpdateOutput(model)->getBit(0), net);
    auto* term = SNLScalarTerm::create(top, SNLTerm::Direction::Output);
    term->setNet(net);
    const int bit = -offset;
    outputs.emplace_back(term, enabled && bit >= 0 && bit < 3 && ((5u >> bit) & 1));
  }
  ConstantPropagation propagation;
  propagation.setTruthTableEngine(GetParam());
  propagation.run();
  for (const auto& [output, expected] : outputs) {
    EXPECT_TRUE(output->getNet()->isConstant());
    EXPECT_EQ(expected, output->getNet()->isConstant1());
  }
}

INSTANTIATE_TEST_SUITE_P(BinaryEngines, VectorUpdateConstantPropagationTest, ::testing::Values(false, true));

class VectorSelectConstantPropagationTest : public ConstantPropagationTests,
    public ::testing::WithParamInterface<bool> {};

TEST_P(VectorSelectConstantPropagationTest, PreservesInvalidReadsAndFoldsValidReads) {
  auto* zero = SNLScalarNet::create(top); zero->setType(SNLNet::Type::Assign0);
  auto* one = SNLScalarNet::create(top); one->setType(SNLNet::Type::Assign1);
  auto* model = NLDB0::getOrCreateVectorSelect({3, 1, 4, true});
  std::vector<std::pair<SNLScalarTerm*, int>> outputs;
  for (int offset : {-1, 0, 1, 2, 3, 7}) {
    auto* instance = SNLInstance::create(top, model);
    for (unsigned bit = 0; bit < 3; ++bit)
      instance->setTermNet(NLDB0::getVectorSelectData(model)->getBit(bit), ((5u >> bit) & 1) ? one : zero);
    for (unsigned bit = 0; bit < 4; ++bit)
      instance->setTermNet(NLDB0::getVectorSelectOffset(model)->getBit(bit),
          ((unsigned(offset) >> bit) & 1) ? one : zero);
    auto* net = SNLScalarNet::create(top);
    instance->setTermNet(NLDB0::getVectorSelectOutput(model)->getBit(0), net);
    auto* term = SNLScalarTerm::create(top, SNLTerm::Direction::Output);
    term->setNet(net);
    outputs.emplace_back(term, offset >= 0 && offset < 3 ? int((5u >> offset) & 1) : -1);
  }
  ConstantPropagation propagation;
  propagation.setTruthTableEngine(GetParam());
  propagation.run();
  for (const auto& [output, expected] : outputs) {
    if (expected < 0) EXPECT_FALSE(output->getNet()->isConstant());
    else {
      EXPECT_TRUE(output->getNet()->isConstant());
      EXPECT_EQ(bool(expected), output->getNet()->isConstant1());
    }
  }
}

INSTANTIATE_TEST_SUITE_P(BinaryEngines, VectorSelectConstantPropagationTest, ::testing::Values(false, true));
