// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnP.h"

#ifdef _WIN32
#include <io.h>
#endif

#include <fcntl.h>
#include <iostream>
#include <sstream>
//#include <boost/asio.hpp>

#include <cassert>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "naja_nl_implementation.capnp.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLException.h"

#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLCapnPRTLInfos.h"
#include "SNLCapnPAttributes.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"

//using boost::asio::ip::tcp;

namespace {

using namespace naja::NL;

DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType SNLtoCapnPNetType(SNLNet::Type type) {
  switch (type) {
    case SNLNet::Type::Standard:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::STANDARD;
    case SNLNet::Type::Assign0:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN0;
    case SNLNet::Type::Assign1:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN1;
    case SNLNet::Type::Supply0:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::SUPPLY0;
    case SNLNet::Type::Supply1:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::SUPPLY1;
    case SNLNet::Type::AssignX:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN_X;
    case SNLNet::Type::AssignZ:
      return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN_Z;
  }
  return DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::STANDARD; //LCOV_EXCL_LINE
}

void dumpInstParameter(
  DBImplementation::LibraryImplementation::SNLDesignImplementation::Instance::InstParameter::Builder& instParameter,
  const SNLInstParameter* snlInstParameter) {
  instParameter.setName(snlInstParameter->getName().getString());
  instParameter.setValue(snlInstParameter->getValue());
}

void dumpInstance(
  DBImplementation::LibraryImplementation::SNLDesignImplementation::Instance::Builder& instance,
  const SNLInstance* snlInstance) {
  if (NLDB0::isFormalProperty(snlInstance->getModel()))
    NLDB0::getFormalProperty(snlInstance);
  instance.setId(snlInstance->getID());
  if (not snlInstance->isUnnamed()) {
    instance.setName(snlInstance->getName().getString());
  }
  auto model = snlInstance->getModel();
  auto modelReference = model->getReference();
  auto modelReferenceBuilder = instance.initModelReference();
  modelReferenceBuilder.setDbID(modelReference.dbID_);
  modelReferenceBuilder.setLibraryID(modelReference.getDBDesignReference().libraryID_);
  modelReferenceBuilder.setDesignID(modelReference.getDBDesignReference().designID_);
  size_t id = 0;
  auto instParameters = instance.initInstParameters(snlInstance->getInstParameters().size());
  for (auto instParameter: snlInstance->getInstParameters()) {
    auto instParameterBuilder = instParameters[id++];
    dumpInstParameter(instParameterBuilder, instParameter);
  }
  dumpAttributes(instance, snlInstance);
  if (snlInstance->hasRTLInfos()) {
    dumpRTLInfos(instance.initRtlInfos(), snlInstance->getRTLInfos());
  }
  size_t count = 0;
  for (auto term: snlInstance->getInstTerms()) {
    if (term->hasRTLInfos() or not SNLAttributes::getAttributes(term).empty()) ++count;
  }
  if (count) {
    auto termInfos = instance.initTermInfos(count);
    size_t id = 0;
    for (auto term: snlInstance->getInstTerms()) {
      if (term->hasRTLInfos() or not SNLAttributes::getAttributes(term).empty()) {
        auto builder = termInfos[id++];
        builder.setTermID(term->getBitTerm()->getID());
        if (auto bit = dynamic_cast<const SNLBusTermBit*>(term->getBitTerm())) {
          builder.setBit(bit->getBit());
        }
        dumpAttributes(builder, term);
        if (term->hasRTLInfos()) {
          dumpRTLInfos(builder.initRtlInfos(), term->getRTLInfos());
        }
      }
    }
  }
}

void dumpBitTermReference(DBImplementation::NetComponentReference::Builder& componentReference,
  const SNLBitTerm* term) {
  auto termRefenceBuilder = componentReference.initTermReference();
  termRefenceBuilder.setTermID(term->getID());
  if (auto busTermBit = dynamic_cast<const SNLBusTermBit*>(term)) {
    termRefenceBuilder.setBit(busTermBit->getBit());
  }
}

void dumpInstTermReference(
  DBImplementation::NetComponentReference::Builder& componentReference,
  const SNLInstTerm* instTerm) {
  auto instTermRefenceBuilder = componentReference.initInstTermReference();
  instTermRefenceBuilder.setInstanceID(instTerm->getInstance()->getID());
  auto term = instTerm->getBitTerm();
  instTermRefenceBuilder.setTermID(term->getID());
  if (auto busTermBit = dynamic_cast<const SNLBusTermBit*>(term)) {
    instTermRefenceBuilder.setBit(busTermBit->getBit());
  }
}

void dumpNetComponentReference(
  DBImplementation::NetComponentReference::Builder& componentReference,
  const SNLNetComponent* component) {
  if (auto instTerm = dynamic_cast<const SNLInstTerm*>(component)) {
    dumpInstTermReference(componentReference, instTerm);
  } else {
    auto term = dynamic_cast<const SNLBitTerm*>(component);
    dumpBitTermReference(componentReference, term);
  }
}

void dumpScalarNet(
  DBImplementation::LibraryImplementation::SNLDesignImplementation::Net::Builder& net,
  const SNLScalarNet* scalarNet) {
  auto scalarNetBuilder = net.initScalarNet();
  scalarNetBuilder.setId(scalarNet->getID());
  if (not scalarNet->isUnnamed()) {
    scalarNetBuilder.setName(scalarNet->getName().getString());
  }
  scalarNetBuilder.setType(SNLtoCapnPNetType(scalarNet->getType()));
  size_t componentsSize = scalarNet->getComponents().size();
  if (componentsSize > 0) {
    auto components = scalarNetBuilder.initComponents(componentsSize);
    size_t id = 0;
    for (auto component: scalarNet->getComponents()) {
      auto componentRefBuilder = components[id++];
      dumpNetComponentReference(componentRefBuilder, component);
    }
  }
  dumpAttributes(scalarNetBuilder, scalarNet);
  if (scalarNet->hasRTLInfos()) {
    dumpRTLInfos(scalarNetBuilder.initRtlInfos(), scalarNet->getRTLInfos());
  }
}

void dumpBusNetBit(
  DBImplementation::LibraryImplementation::SNLDesignImplementation::BusNetBit::Builder& bitBuilder,
  NLID::Bit bit,
  const SNLBusNetBit* busNetBit) {
  bitBuilder.setBit(bit);
  if (busNetBit) {
    bitBuilder.setDestroyed(false);
    bitBuilder.setType(SNLtoCapnPNetType(busNetBit->getType()));
    size_t componentsSize = busNetBit->getComponents().size();
    if (componentsSize > 0) {
      auto components = bitBuilder.initComponents(componentsSize);
      size_t id = 0;
      for (auto component: busNetBit->getComponents()) {
        auto componentRefBuilder = components[id++];
        dumpNetComponentReference(componentRefBuilder, component);
      }
    }
    dumpAttributes(bitBuilder, busNetBit);
    if (busNetBit->hasRTLInfos()) {
      dumpRTLInfos(bitBuilder.initRtlInfos(), busNetBit->getRTLInfos());
    }
  } else {
    bitBuilder.setDestroyed(true);
  }
}

void dumpBusNet(
  DBImplementation::LibraryImplementation::SNLDesignImplementation::Net::Builder& net,
  const SNLBusNet* busNet) {
  auto busNetBuilder = net.initBusNet();
  busNetBuilder.setId(busNet->getID());
  if (not busNet->isUnnamed()) {
    busNetBuilder.setName(busNet->getName().getString());
  }
  busNetBuilder.setMsb(busNet->getMSB());
  busNetBuilder.setLsb(busNet->getLSB());
  dumpAttributes(busNetBuilder, busNet);
  if (busNet->hasRTLInfos()) {
    dumpRTLInfos(busNetBuilder.initRtlInfos(), busNet->getRTLInfos());
  }
  auto bits = busNetBuilder.initBits(busNet->getWidth());
  size_t id = 0;
  for (size_t i=0; i<busNet->getWidth(); i++) {
    NLID::Bit bit = (busNet->getMSB()>busNet->getLSB())?
      busNet->getMSB()-int(i):busNet->getMSB()+int(i);
    SNLBusNetBit* busNetBit = busNet->getBit(bit);
    auto bitBuilder = bits[id++];
    dumpBusNetBit(bitBuilder, bit, busNetBit);
  }
}

void dumpDesignImplementation(
  DBImplementation::LibraryImplementation::SNLDesignImplementation::Builder& designImplementation,
  const SNLDesign* snlDesign) {
  designImplementation.setId(snlDesign->getID());

  size_t id = 0;
  auto instances = designImplementation.initInstances(snlDesign->getInstances().size());
  for (auto instance: snlDesign->getInstances()) {
    auto instanceBuilder = instances[id++];
    dumpInstance(instanceBuilder, instance);
  }

  id = 0;
  auto nets = designImplementation.initNets(snlDesign->getNets().size());
  for (auto net: snlDesign->getNets()) {
    auto netBuilder = nets[id++];
    if (auto scalarNet = dynamic_cast<const SNLScalarNet*>(net)) {
      dumpScalarNet(netBuilder, scalarNet);
    } else {
      auto busNet = static_cast<SNLBusNet*>(net);
      dumpBusNet(netBuilder, busNet);
    }
  }
}

void dumpLibraryImplementation(
  DBImplementation::LibraryImplementation::Builder& libraryImplementation,
  const NLLibrary* snlLibrary) {
  libraryImplementation.setId(snlLibrary->getID());
  auto subLibraries = libraryImplementation.initLibraryImplementations(snlLibrary->getLibraries().size());
  size_t id = 0;
  for (auto subLib: snlLibrary->getLibraries()) {
    auto subLibraryBuilder = subLibraries[id++];
    dumpLibraryImplementation(subLibraryBuilder, subLib);
  }

  auto designs = libraryImplementation.initSnlDesignImplementations(snlLibrary->getSNLDesigns().size());
  id = 0;
  for (auto snlDesign: snlLibrary->getSNLDesigns()) {
    auto designImplementationBuilder = designs[id++]; 
    dumpDesignImplementation(designImplementationBuilder, snlDesign);
  }
}

SNLNet::Type CapnPtoSNLNetType(DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType type) {
  switch (type) {
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::STANDARD:
      return SNLNet::Type::Standard;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN0:
      return SNLNet::Type::Assign0;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN1:
      return SNLNet::Type::Assign1;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::SUPPLY0:
      return SNLNet::Type::Supply0;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::SUPPLY1:
      return SNLNet::Type::Supply1;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN_X:
      return SNLNet::Type::AssignX;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType::ASSIGN_Z:
      return SNLNet::Type::AssignZ;
  }
  return SNLNet::Type::Standard; //LCOV_EXCL_LINE
}

void loadInstParameter(
  SNLInstance* instance,
  const DBImplementation::LibraryImplementation::SNLDesignImplementation::Instance::InstParameter::Reader& instParameter) {
  auto name = instParameter.getName();
  auto value = instParameter.getValue();
  auto parameter = instance->getModel()->getParameter(NLName(name));
  //LCOV_EXCL_START
  if (not parameter) {
    std::ostringstream reason;
    reason << "cannot deserialize instance parameter '" << std::string(name) << "'";
    reason << " for " << instance->getDescription();
    reason << " (model " << instance->getModel()->getDescription() << ")";
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  SNLInstParameter::create(instance, parameter, value);
}

void loadInstance(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::SNLDesignImplementation::Instance::Reader& instance) {
  auto instanceID = instance.getId();
  NLName snlName;
  if (instance.hasName()) {
    snlName = NLName(instance.getName());
  }
  auto modelReference = instance.getModelReference();
  auto snlModelReference =
    NLID::DesignReference(
      modelReference.getDbID(),
      modelReference.getLibraryID(),
      modelReference.getDesignID());
  // A snapshot restores canonical model definitions before its instances.
  // Never reinterpret an instance override as the signature of its model.
  SNLDesign* model = NLUniverse::get()->getSNLDesign(snlModelReference);
  if (not model && snlModelReference.dbID_ == NLID::DBID(0)) {
    // Compatibility for callers of the low-level interface/implementation API.
    // These callers must supply the DB0 interface first for compound models;
    // otherwise only self-describing references can be resolved faithfully.
    model = NLDB0::getOrCreatePrimitive(
      snlModelReference.libraryID_, snlModelReference.designID_);
  }
  //LCOV_EXCL_START
  if (not model) {
    std::ostringstream reason;
    reason << "cannot deserialize instance " << instanceID << ": model not found";
    reason << " (reference dbID " << snlModelReference.dbID_;
    reason << ", libraryID " << snlModelReference.libraryID_;
    reason << ", designID " << snlModelReference.designID_ << ")";
    reason << " in design " << design->getDescription();
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  auto snlInstance =
    SNLInstance::create(design, model, NLID::DesignObjectID(instanceID), snlName);
  if (instance.hasInstParameters()) {
    for (auto instParameter: instance.getInstParameters()) {
      loadInstParameter(snlInstance, instParameter);
    }
  }
  if (NLDB0::isFormalProperty(model)) NLDB0::getFormalProperty(snlInstance);
  loadAttributes(instance, snlInstance);
  if (instance.hasRtlInfos()) {
    loadRTLInfos(SNLRTLInfos::create(snlInstance), instance.getRtlInfos());
  }
  for (auto termInfo: instance.getTermInfos()) {
    auto term = model->getTerm(termInfo.getTermID());
    SNLBitTerm* bit = dynamic_cast<SNLScalarTerm*>(term);
    if (auto bus = dynamic_cast<SNLBusTerm*>(term)) {
      bit = bus->getBit(termInfo.getBit());
    }
    if (not bit) {
      throw NLException("cannot deserialize instance term metadata: term not found");
    }
    auto instTerm = snlInstance->getInstTerm(bit);
    loadAttributes(termInfo, instTerm);
    if (termInfo.hasRtlInfos()) {
      loadRTLInfos(SNLRTLInfos::create(instTerm), termInfo.getRtlInfos());
    }
  }
}

void loadTermReference(
  SNLBitNet* net,
  const DBImplementation::TermReference::Reader& termReference) {
  auto design = net->getDesign();
  auto term = design->getTerm(NLID::DesignObjectID(termReference.getTermID()));
  //LCOV_EXCL_START
  if (not term) {
    std::ostringstream reason;
    reason << "cannot deserialize term reference: no term with ID ";
    reason << termReference.getTermID();
    reason << " in design " << design->getDescription();
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
    scalarTerm->setNet(net);
  } else {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    assert(busTerm);
    auto busTermBit = busTerm->getBit(termReference.getBit());
    //LCOV_EXCL_START
    if (not busTermBit) {
      std::ostringstream reason;
      reason << "cannot deserialize term reference: no bus term bit";
      reason << " (termID " << termReference.getTermID();
      reason << ", bit " << termReference.getBit();
      reason << ", design " << design->getDescription() << ")";
      throw NLException(reason.str());
    }
    //LCOV_EXCL_STOP
    busTermBit->setNet(net);
  }
}

void loadInstTermReference(
  SNLBitNet* net,
  const DBImplementation::InstTermReference::Reader& instTermReference) {
  auto instanceID = instTermReference.getInstanceID();
  auto design = net->getDesign();
  auto instance = design->getInstance(NLID::DesignObjectID(instanceID));
  //LCOV_EXCL_START
  if (not instance) {
    std::ostringstream reason;
    reason << "cannot deserialize instance term reference: no instance with ID ";
    reason << instanceID << " in design " << design->getDescription();
    reason << " (net " << net->getDescription() << ")";
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  auto model = instance->getModel();
  auto termID = instTermReference.getTermID();
  auto term = model->getTerm(NLID::DesignObjectID(termID));
  //LCOV_EXCL_START
  if (not term) {
    std::ostringstream reason;
    reason << "cannot deserialize instance " << instance->getDescription();
    reason << " term reference: no term with ID " << termID;
    reason << " in model " << model->getDescription();
    reason << " (net " << net->getDescription() << ")";
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  SNLBitTerm* bitTerm = dynamic_cast<SNLScalarTerm*>(term);
  if (not bitTerm) {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    assert(busTerm);
    bitTerm = busTerm->getBit(instTermReference.getBit());
    //LCOV_EXCL_START
    if (not bitTerm) {
      std::ostringstream reason;
      reason << "cannot deserialize instance term reference: no bit found in bus term";
      reason << " (instance " << instance->getDescription();
      reason << ", model " << model->getDescription();
      reason << ", term " << busTerm->getDescription();
      reason << ", bit " << instTermReference.getBit();
      reason << ", net " << net->getDescription() << ")";
      throw NLException(reason.str());
    }
    //LCOV_EXCL_STOP
  }
  auto instTerm = instance->getInstTerm(bitTerm);
  assert(instTerm);
  instTerm->setNet(net);
}

void loadBusNet(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::SNLDesignImplementation::BusNet::Reader& net) {
  NLName snlName;
  if (net.hasName()) {
    snlName = NLName(net.getName());
  }
  auto busNet = SNLBusNet::create(design, NLID::DesignObjectID(net.getId()), net.getMsb(), net.getLsb(), snlName);
  if (net.hasBits()) {
    for (auto bitNet: net.getBits()) {
      auto bit = bitNet.getBit();
      auto busNetBit = busNet->getBit(bit);
      //LCOV_EXCL_START
      if (not busNetBit) {
        std::ostringstream reason;
        reason << "cannot deserialize bus net bit: no bit ";
        reason << bit << " in " << busNet->getDescription();
        throw NLException(reason.str());
      }
      //LCOV_EXCL_STOP
      if (bitNet.getDestroyed()) {
        busNetBit->destroy();
        continue;
      }
      busNetBit->setType(CapnPtoSNLNetType(bitNet.getType()));
      if (bitNet.hasComponents()) {
        for (auto componentReference: bitNet.getComponents()) {
          if (componentReference.isInstTermReference()) {
            loadInstTermReference(busNetBit, componentReference.getInstTermReference());
          } else if (componentReference.isTermReference()) {
            loadTermReference(busNetBit, componentReference.getTermReference());
          }
        }
      }
      loadAttributes(bitNet, busNetBit);
      if (bitNet.hasRtlInfos()) {
        loadRTLInfos(SNLRTLInfos::create(busNetBit), bitNet.getRtlInfos());
      }
    }
  }
  loadAttributes(net, busNet);
  if (net.hasRtlInfos()) {
    loadRTLInfos(SNLRTLInfos::create(busNet), net.getRtlInfos());
  }
}

void loadScalarNet(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::SNLDesignImplementation::ScalarNet::Reader& net) {
  NLName snlName;
  if (net.hasName()) {
    snlName = NLName(net.getName());
  }
  auto scalarNet = SNLScalarNet::create(design, NLID::DesignObjectID(net.getId()), snlName);
  scalarNet->setType(CapnPtoSNLNetType(net.getType()));
  if (net.hasComponents()) {
    for (auto componentReference: net.getComponents()) {
      if (componentReference.isInstTermReference()) {
        loadInstTermReference(scalarNet, componentReference.getInstTermReference());
      } else if (componentReference.isTermReference()) {
        loadTermReference(scalarNet, componentReference.getTermReference());
      }
    }
  }
  loadAttributes(net, scalarNet);
  if (net.hasRtlInfos()) {
    loadRTLInfos(SNLRTLInfos::create(scalarNet), net.getRtlInfos());
  }
}

void loadDesignImplementation(
  NLLibrary* library,
  const DBImplementation::LibraryImplementation::SNLDesignImplementation::Reader& designImplementation) {
  auto designID = designImplementation.getId();
  SNLDesign* snlDesign = library->getSNLDesign(NLID::DesignID(designID));
  //LCOV_EXCL_START
  if (not snlDesign) {
    std::ostringstream reason;
    reason << "cannot deserialize design: no design with ID ";
    reason << designID << " in library " << library->getDescription();
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  if (designImplementation.hasInstances()) {
    for (auto instance: designImplementation.getInstances()) {
      loadInstance(snlDesign, instance);
    }
  }
  if (designImplementation.hasNets()) {
    for (auto net: designImplementation.getNets()) {
      if (net.isScalarNet()) {
        auto scalarNet = net.getScalarNet();
        loadScalarNet(snlDesign, scalarNet);
      } else if (net.isBusNet()) {
        auto busNet = net.getBusNet();
        loadBusNet(snlDesign, busNet);
      } 
    }
  }
}

void loadLibraryImplementation(NLDB* db, const DBImplementation::LibraryImplementation::Reader& libraryImplementation) {
  auto libraryID = libraryImplementation.getId();
  NLLibrary* snlLibrary = db->getLibrary(NLID::LibraryID(libraryID));
  //LCOV_EXCL_START
  if (not snlLibrary) {
    std::ostringstream reason;
    reason << "cannot load library with id " << libraryID
      << " in db " << db->getDescription();
    if (db->getLibraries().empty()) {
      reason << ", no libraries in this db.";
    } else {
      reason << ", existing libraries are: " << std::endl;
      for (auto lib: db->getLibraries()) {
        reason << lib->getDescription() << std::endl;
      }
    }
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  if (libraryImplementation.hasSnlDesignImplementations()) {
    for (auto designImplementation: libraryImplementation.getSnlDesignImplementations()) {
      loadDesignImplementation(snlLibrary, designImplementation);
    } 
  }
}

}

namespace naja::NL {

void SNLCapnP::dumpImplementation(const NLDB* snlDB, int fileDescriptor) {
  dumpImplementation(snlDB, fileDescriptor, snlDB->getID());
}

void SNLCapnP::dumpImplementation(const NLDB* snlDB, int fileDescriptor, NLID::DBID forceDBID) {
  ::capnp::MallocMessageBuilder message;

  DBImplementation::Builder db = message.initRoot<DBImplementation>();
  db.setId(forceDBID);
  auto libraries = db.initLibraryImplementations(snlDB->getLibraries().size());
  
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryImplementationBuilder = libraries[id++];
    dumpLibraryImplementation(libraryImplementationBuilder, snlLibrary);
  }
  writePackedMessageToFd(fileDescriptor, message);
}

void SNLCapnP::dumpImplementation(const NLDB* snlDB, const std::filesystem::path& implementationPath) {
#ifdef _WIN32
  int fd = _open(
    implementationPath.string().c_str(),
    _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY,
    _S_IREAD | _S_IWRITE);
#else
  int fd = open(
    implementationPath.c_str(),
    O_CREAT | O_TRUNC | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
  dumpImplementation(snlDB, fd);
#ifdef _WIN32
  _close(fd);
#else
  close(fd);
#endif
}

//LCOV_EXCL_START
//void SNLCapnP::sendImplementation(const NLDB* db, tcp::socket& socket) {
//  sendImplementation(db, socket, db->getID());
//}
//
//void SNLCapnP::sendImplementation(const NLDB* db, tcp::socket& socket, NLID::DBID forceDBID) {
//  dumpImplementation(db, socket.native_handle(), forceDBID);
//}
//
//void SNLCapnP::sendImplementation(
//  const NLDB* db,
//  const std::string& ipAddress,
//  uint16_t port) {
//  boost::asio::io_context ioContext;
//  //socket creation
//  tcp::socket socket(ioContext);
//  socket.connect(tcp::endpoint(boost::asio::ip::make_address(ipAddress), port));
//  sendImplementation(db, socket);
//}
//LCOV_EXCL_STOP

NLDB* SNLCapnP::loadImplementation(
  int fileDescriptor,
  LoadingConfiguration loadingConfiguration) {
  ::capnp::ReaderOptions options;
  options.traversalLimitInWords = std::numeric_limits<uint64_t>::max();
  ::capnp::PackedFdMessageReader message(fileDescriptor, options);

  DBImplementation::Reader dbImplementation = message.getRoot<DBImplementation>();
  auto dbID = dbImplementation.getId();
  auto universe = NLUniverse::get();
  //LCOV_EXCL_START
  if (not universe) {
    std::ostringstream reason;
    reason << "cannot deserialize DB implementation: no existing universe";
    reason << " (call NLUniverse::create() before loadImplementation)";
    throw NLException(reason.str());
  }
  //LCOV_EXCL_STOP
  auto snldb = universe->getDB(dbID);
  if (dbImplementation.hasLibraryImplementations()) {
    for (auto libraryImplementation: dbImplementation.getLibraryImplementations()) {
      loadLibraryImplementation(snldb, libraryImplementation);
    }
  }
  return snldb;
}

NLDB* SNLCapnP::loadImplementation(
  const std::filesystem::path& implementationPath,
  LoadingConfiguration loadingConfiguration) {
#ifdef _WIN32
  int fd = _open(implementationPath.string().c_str(), _O_RDONLY | _O_BINARY);
#else
  int fd = open(implementationPath.c_str(), O_RDONLY);
#endif
  try {
    auto* db = loadImplementation(fd, loadingConfiguration);
#ifdef _WIN32
    _close(fd);
#else
    close(fd);
#endif
    return db;
  } catch (...) {
#ifdef _WIN32
    _close(fd);
#else
    close(fd);
#endif
    throw;
  }
}

//LCOV_EXCL_START
//NLDB* SNLCapnP::receiveImplementation(tcp::socket& socket) {
//  return loadImplementation(socket.native_handle());
//}
//
//NLDB* SNLCapnP::receiveImplementation(uint16_t port) {
//  boost::asio::io_context ioContext;
//  //listen for new connection
//  tcp::acceptor acceptor_(ioContext, tcp::endpoint(tcp::v4(), port));
//  //socket creation 
//  tcp::socket socket(ioContext);
//  //waiting for connection
//  acceptor_.accept(socket);
//  NLDB* db = receiveImplementation(socket);
//  return db;
//}
//LCOV_EXCL_STOP

}  // namespace naja::NL
