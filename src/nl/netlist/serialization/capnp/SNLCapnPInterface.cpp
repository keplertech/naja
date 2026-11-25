// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnP.h"

#ifdef _WIN32
#include <io.h>
#endif

#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <list>
#include <set>
//#include <boost/asio.hpp>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "naja_nl_interface.capnp.h"

#include "NajaDumpableProperty.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLCapnPRTLInfos.h"
#include "SNLCapnPAttributes.h"
#include "SNLScalarTerm.h"
#include "SNLBundleTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "NLLibraryTruthTables.h"

//using boost::asio::ip::tcp;

namespace {

using namespace naja;
using namespace naja::NL;

void dumpProperty(
  Property::Builder& property,
  const NajaDumpableProperty* najaProperty) {
  property.setName(najaProperty->getName());
  const auto& propertyValues = najaProperty->getValues();
  auto values = property.initValues(propertyValues.size());
  size_t id = 0;
  for (auto propertyValue: propertyValues) {
    auto valueBuilder = values[id++];
    if (propertyValue.index() == NajaDumpableProperty::String) {
      valueBuilder.setText(std::get<NajaDumpableProperty::String>(propertyValue));
    } else if (propertyValue.index() == NajaDumpableProperty::UInt64) {
      valueBuilder.setUint64(std::get<NajaDumpableProperty::UInt64>(propertyValue));
    }
  }
}

template<typename T> void dumpProperties(
  T& dumpObjectInterface,
  const NajaObject* object,
  auto& initProperties) {
  using NajaProperties = std::list<NajaDumpableProperty*>;
  NajaProperties najaProperties(object->getDumpableProperties().begin(), object->getDumpableProperties().end());
  auto properties = initProperties(dumpObjectInterface, najaProperties.size());
  size_t id = 0;
  for (auto najaProperty: najaProperties) {
    auto propertyBuilder = properties[id++];
    dumpProperty(propertyBuilder, najaProperty);
  }
}

Direction SNLtoCapnPDirection(SNLTerm::Direction direction) {
  switch (direction) {
    case SNLTerm::Direction::Input:
      return Direction::INPUT;
    case SNLTerm::Direction::Output:
      return Direction::OUTPUT;
    case SNLTerm::Direction::InOut:
      return Direction::INOUT;
    case SNLTerm::Direction::Undefined:
      return Direction::UNDEFINED;
  }
  return Direction::INPUT; //LCOV_EXCL_LINE
}

SNLDesignInterface::ParameterType SNLtoCapNpParameterType(SNLParameter::Type type) {
  switch (type) {
    case SNLParameter::Type::Decimal:
      return SNLDesignInterface::ParameterType::DECIMAL;
    case SNLParameter::Type::Binary:
      return SNLDesignInterface::ParameterType::BINARY;
    case SNLParameter::Type::Boolean:
      return SNLDesignInterface::ParameterType::BOOLEAN;
    case SNLParameter::Type::String:
      return SNLDesignInterface::ParameterType::STRING;
  }
  return SNLDesignInterface::ParameterType::DECIMAL; //LCOV_EXCL_LINE
}

void dumpScalarTerm(
  SNLDesignInterface::Term::Builder& term,
  const SNLScalarTerm* scalarTerm) {
  auto scalarTermBuilder = term.initScalarTerm();
  scalarTermBuilder.setId(scalarTerm->getID());
  if (not scalarTerm->isUnnamed()) {
    scalarTermBuilder.setName(scalarTerm->getName().getString());
  }
  scalarTermBuilder.setDirection(SNLtoCapnPDirection(scalarTerm->getDirection()));
  dumpAttributes(scalarTermBuilder, scalarTerm);
  if (scalarTerm->hasRTLInfos()) {
    dumpRTLInfos(scalarTermBuilder.initRtlInfos(), scalarTerm->getRTLInfos());
  }
}

void dumpBusTerm(
  SNLDesignInterface::Term::Builder& term,
  const SNLBusTerm* busTerm) {
  auto busTermBuilder = term.initBusTerm();
  busTermBuilder.setId(busTerm->getID());
  if (not busTerm->isUnnamed()) {
    busTermBuilder.setName(busTerm->getName().getString());
  }
  busTermBuilder.setMsb(busTerm->getMSB());
  busTermBuilder.setLsb(busTerm->getLSB());
  busTermBuilder.setDirection(SNLtoCapnPDirection(busTerm->getDirection()));
  dumpAttributes(busTermBuilder, busTerm);
  if (busTerm->hasRTLInfos()) {
    dumpRTLInfos(busTermBuilder.initRtlInfos(), busTerm->getRTLInfos());
  }
  size_t count = 0;
  for (auto bit: busTerm->getBits()) {
    if (bit->hasRTLInfos() or not SNLAttributes::getAttributes(bit).empty()) ++count;
  }
  if (count) {
    auto bitInfos = busTermBuilder.initBitInfos(count);
    size_t id = 0;
    for (auto bit: busTerm->getBits()) {
      if (bit->hasRTLInfos() or not SNLAttributes::getAttributes(bit).empty()) {
        auto builder = bitInfos[id++];
        builder.setBit(static_cast<const SNLBusTermBit*>(bit)->getBit());
        dumpAttributes(builder, bit);
        if (bit->hasRTLInfos()) {
          dumpRTLInfos(builder.initRtlInfos(), bit->getRTLInfos());
        }
      }
    }
  }
}

void dumpParameter(
  SNLDesignInterface::Parameter::Builder& parameter,
  const SNLParameter* snlParameter) {
  parameter.setName(snlParameter->getName().getString());
  parameter.setType(SNLtoCapNpParameterType(snlParameter->getType()));
  // A null Text pointer denotes no default; setValue("") preserves an empty default.
  if (snlParameter->hasDefaultValue()) {
    parameter.setValue(snlParameter->getValue());
  }
}

DesignType SNLtoCapNpDesignType(SNLDesign::Type type) {
  switch (type) {
    case SNLDesign::Type::Standard:
      return DesignType::STANDARD;
    case SNLDesign::Type::Primitive:
      return DesignType::PRIMITIVE;
    case SNLDesign::Type::UserBlackBox:
      return DesignType::USER_BLACKBOX;
    case SNLDesign::Type::AutoBlackBox:
      return DesignType::AUTO_BLACKBOX;
  }
  return DesignType::STANDARD; //LCOV_EXCL_LINE
}

using BooleanExpression = SNLDesignModeling::BooleanExpression;
using BooleanOperation = SNLDesignInterface::BooleanNode::Operation;

BooleanOperation dumpBooleanOperation(BooleanExpression::Operator operation) {
  using Op = BooleanExpression::Operator;
  switch (operation) {
    case Op::Constant: return BooleanOperation::CONSTANT;
    case Op::Term: return BooleanOperation::TERM;
    case Op::Not: return BooleanOperation::NOT;
    case Op::And: return BooleanOperation::AND;
    case Op::Or: return BooleanOperation::OR;
    case Op::Xor: return BooleanOperation::XOR;
    default: throw NLException("Cannot serialize a non-combinational expression");
  }
}

BooleanExpression::Operator loadBooleanOperation(BooleanOperation operation) {
  using Op = BooleanExpression::Operator;
  switch (operation) {
    case BooleanOperation::CONSTANT: return Op::Constant;
    case BooleanOperation::TERM: return Op::Term;
    case BooleanOperation::NOT: return Op::Not;
    case BooleanOperation::AND: return Op::And;
    case BooleanOperation::OR: return Op::Or;
    case BooleanOperation::XOR: return Op::Xor;
    default: throw NLException("Invalid combinational Boolean operator in snapshot");
  }
}

void dumpFunctions(SNLDesignInterface::Builder& builder, const SNLDesign* design) {
  std::vector<std::pair<SNLBitTerm*, BooleanExpression>> functions;
  for (auto* term : design->getBitTerms()) {
    if (auto function = SNLDesignModeling::getFunction(term)) functions.emplace_back(term, std::move(*function));
  }
  auto serialized = builder.initFunctions(functions.size());
  for (size_t i = 0; i < functions.size(); ++i) {
    const auto& [output, expression] = functions[i];
    auto function = serialized[i];
    function.setOutput(output->getOrderID());
    function.setRoot(expression.root);
    auto nodes = function.initNodes(expression.nodes.size());
    for (size_t j = 0; j < expression.nodes.size(); ++j) {
      const auto& node = expression.nodes[j];
      nodes[j].setOperation(dumpBooleanOperation(node.operation));
      nodes[j].setConstant(node.constant);
      if (node.operation == BooleanExpression::Operator::Term) nodes[j].setTerm(node.term->getOrderID());
      auto operands = nodes[j].initOperands(node.operands.size());
      for (size_t k = 0; k < node.operands.size(); ++k) operands.set(k, node.operands[k]);
    }
  }
}

void loadFunctions(const SNLDesignInterface::Reader& reader, SNLDesign* design) {
  std::vector<SNLBitTerm*> terms(design->getBitTerms().begin(), design->getBitTerms().end());
  std::set<uint32_t> outputs;
  for (auto function : reader.getFunctions()) {
    if (function.getOutput() >= terms.size() || !outputs.insert(function.getOutput()).second)
      throw NLException("Invalid or duplicate function output in snapshot");
    BooleanExpression expression;
    expression.root = function.getRoot();
    for (auto node : function.getNodes()) {
      BooleanExpression::Node value;
      value.operation = loadBooleanOperation(node.getOperation());
      value.constant = node.getConstant();
      if (value.operation == BooleanExpression::Operator::Term) {
        if (node.getTerm() >= terms.size()) throw NLException("Invalid function term in snapshot");
        value.term = terms[node.getTerm()];
      }
      for (auto operand : node.getOperands()) value.operands.push_back(operand);
      expression.nodes.push_back(std::move(value));
    }
    SNLDesignModeling::setFunction(terms[function.getOutput()], expression);
  }
}

void dumpDesignInterface(
  SNLDesignInterface::Builder& designInterface,
  const SNLDesign* snlDesign) {
  designInterface.setId(snlDesign->getID());
  if (not snlDesign->isUnnamed()) {
    designInterface.setName(snlDesign->getName().getString());
  }
  designInterface.setType(SNLtoCapNpDesignType(snlDesign->getType()));
  dumpAttributes(designInterface, snlDesign);
  dumpFunctions(designInterface, snlDesign);
  if (snlDesign->hasRTLInfos()) {
    dumpRTLInfos(designInterface.initRtlInfos(), snlDesign->getRTLInfos());
  }
  auto lambda = [](SNLDesignInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(designInterface, snlDesign, lambda);

  size_t id = 0;
  auto parameters = designInterface.initParameters(snlDesign->getParameters().size());
  for (auto parameter: snlDesign->getParameters()) {
    auto parameterBuilder = parameters[id++];
    dumpParameter(parameterBuilder, parameter);
  }
  
  id = 0;
  if (not snlDesign->getBundleTerms().empty()) {
    std::ostringstream reason;
    reason << "Cap'n Proto serialization does not support bundle terms in design "
           << snlDesign->getDescription();
    throw NLException(reason.str());
  }
  auto terms = designInterface.initTerms(snlDesign->getTerms().size());
  for (auto term: snlDesign->getTerms()) {
    auto termBuilder = terms[id++];
    if (auto scalarTerm = dynamic_cast<const SNLScalarTerm*>(term)) {
      dumpScalarTerm(termBuilder, scalarTerm);
    } else {
      auto busTerm = static_cast<SNLBusTerm*>(term);
      dumpBusTerm(termBuilder, busTerm);
    }
  }
}

DBInterface::LibraryType SNLtoCapnPLibraryType(NLLibrary::Type type) {
  switch (type) {
    case NLLibrary::Type::Standard:
      return DBInterface::LibraryType::STANDARD;
    case NLLibrary::Type::Primitives:
      return DBInterface::LibraryType::PRIMITIVES;
    //LCOV_EXCL_START
    case NLLibrary::Type::InDB0:
      throw NLException("Unexpected InDB0 Library type while loading Library");
    //LCOV_EXCL_STOP
  }
  return DBInterface::LibraryType::STANDARD; //LCOV_EXCL_LINE
}

NLLibrary::Type CapnPtoNLLibraryType(DBInterface::LibraryType type) {
  switch (type) {
    case DBInterface::LibraryType::STANDARD:
      return NLLibrary::Type::Standard;
    case DBInterface::LibraryType::PRIMITIVES:
      return  NLLibrary::Type::Primitives;
  }
  return NLLibrary::Type::Standard; //LCOV_EXCL_LINE
}

void dumpLibraryInterface(
  DBInterface::LibraryInterface::Builder& libraryInterface,
  const NLLibrary* snlLibrary) {
  libraryInterface.setId(snlLibrary->getID());
  auto lambda = [](DBInterface::LibraryInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(libraryInterface, snlLibrary, lambda);

  if (not snlLibrary->isUnnamed()) {
    libraryInterface.setName(snlLibrary->getName().getString());
  }
  libraryInterface.setType(SNLtoCapnPLibraryType(snlLibrary->getType()));
  auto subLibraries = libraryInterface.initLibraryInterfaces(snlLibrary->getLibraries().size());
  size_t id = 0;
  for (auto subLib: snlLibrary->getLibraries()) {
    auto subLibraryBuilder = subLibraries[id++];
    dumpLibraryInterface(subLibraryBuilder, subLib);
  }

  auto designs = libraryInterface.initSnlDesignInterfaces(snlLibrary->getSNLDesigns().size());
  id = 0;
  for (auto snlDesign: snlLibrary->getSNLDesigns()) {
    auto designInterfaceBuilder = designs[id++]; 
    dumpDesignInterface(designInterfaceBuilder, snlDesign);
  }
}

// DB0 is owned by NLUniverse. Recreate its canonical models through NLDB0,
// rather than loading ordinary primitive shells without their native modeling.
void loadDB0LibraryInterface(const DBInterface::LibraryInterface::Reader& library) {
  const auto libraryID = NLID::LibraryID(library.getId());
  for (auto definition : library.getSnlDesignInterfaces()) {
    NLDB0::PrimitiveParameters parameters;
    for (auto parameter : definition.getParameters()) {
      if (!parameter.hasValue() || !parameters.emplace(
          parameter.getName().cStr(), parameter.getValue().cStr()).second) {
        throw NLException("Cannot load DB0 model: missing default or duplicate parameter");
      }
    }
    auto* model = NLDB0::getOrCreatePrimitive(
        libraryID, NLID::DesignID(definition.getId()), parameters);
    if (!model || model->getID() != definition.getId()) {
      throw NLException("Cannot load DB0 model: canonical identity conflicts with the snapshot; use a fresh universe");
    }
    // Check the entire interface, including parameter types/defaults, terminal
    // IDs, bus bounds, directions and names. Never silently accept a mismatched
    // signature or a modified canonical model.
    ::capnp::MallocMessageBuilder generated;
    auto builder = generated.initRoot<SNLDesignInterface>();
    dumpDesignInterface(builder, model);
    auto expected = ::capnp::canonicalize(definition);
    auto actual = ::capnp::canonicalize(builder.asReader());
    if (expected.size() != actual.size() || std::memcmp(
        expected.begin(), actual.begin(), expected.size() * sizeof(::capnp::word))) {
      throw NLException("Cannot load DB0 model: canonical interface differs from the snapshot");
    }
  }
  for (auto child : library.getLibraryInterfaces()) loadDB0LibraryInterface(child);
}

SNLDesign::Type CapnPtoSNLDesignType(DesignType type) {
  switch (type) {
    case DesignType::STANDARD:
      return SNLDesign::Type::Standard;
    case DesignType::USER_BLACKBOX: 
      return SNLDesign::Type::UserBlackBox;
    case DesignType::AUTO_BLACKBOX: 
      return SNLDesign::Type::AutoBlackBox;
    case DesignType::PRIMITIVE: 
      return SNLDesign::Type::Primitive;
  }
  return SNLDesign::Type::Standard; //LCOV_EXCL_LINE
}

SNLTerm::Direction CapnPtoSNLDirection(Direction direction) {
  switch (direction) {
    case Direction::INPUT:
      return SNLTerm::Direction::Input;
    case Direction::OUTPUT:
      return SNLTerm::Direction::Output;
    case Direction::INOUT:
      return SNLTerm::Direction::InOut;
    case Direction::UNDEFINED:
      return SNLTerm::Direction::Undefined;
  }
  return SNLTerm::Direction::Undefined; //LCOV_EXCL_LINE
}

SNLParameter::Type CapnPtoSNLParameterType(SNLDesignInterface::ParameterType type) {
  switch (type) {
    case SNLDesignInterface::ParameterType::DECIMAL:
      return SNLParameter::Type::Decimal;
    case SNLDesignInterface::ParameterType::BINARY:
      return SNLParameter::Type::Binary;
    case SNLDesignInterface::ParameterType::BOOLEAN:
      return SNLParameter::Type::Boolean;
    case SNLDesignInterface::ParameterType::STRING:
      return SNLParameter::Type::String;
  }
  return SNLParameter::Type::Decimal; //LCOV_EXCL_LINE
}

template<typename T> void loadProperties(
  const T& dumpObjectReader,
  NajaObject* object,
  auto& propertiesGetter) {
  for (auto property: propertiesGetter(dumpObjectReader)) {
    auto najaProperty = NajaDumpableProperty::create(object, property.getName());
    for (auto value: property.getValues()) {
      if (value.isText()) {
        najaProperty->addStringValue(value.getText());
      } else if (value.isUint64()) {
        najaProperty->addUInt64Value(value.getUint64());
      }
    }
  }
}

void loadScalarTerm(
  SNLDesign* design,
  const SNLDesignInterface::ScalarTerm::Reader& term) {
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  NLName snlName;
  if (term.hasName()) {
    snlName = NLName(term.getName());
  }
  auto scalarTerm = SNLScalarTerm::create(design, NLID::DesignObjectID(termID), CapnPtoSNLDirection(termDirection), snlName);
  loadAttributes(term, scalarTerm);
  if (term.hasRtlInfos()) {
    loadRTLInfos(SNLRTLInfos::create(scalarTerm), term.getRtlInfos());
  }
}

void loadBusTerm(
  SNLDesign* design,
  const SNLDesignInterface::BusTerm::Reader& term) {
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  auto msb = term.getMsb();
  auto lsb = term.getLsb();
  NLName snlName;
  if (term.hasName()) {
    snlName = NLName(term.getName());
  }
  auto busTerm = SNLBusTerm::create(design, NLID::DesignObjectID(termID), CapnPtoSNLDirection(termDirection), msb, lsb, snlName);
  loadAttributes(term, busTerm);
  if (term.hasRtlInfos()) {
    loadRTLInfos(SNLRTLInfos::create(busTerm), term.getRtlInfos());
  }
  for (auto bitInfo: term.getBitInfos()) {
    auto bit = busTerm->getBit(bitInfo.getBit());
    if (not bit) {
      // Malformed input only: the serializer emits metadata for existing bits.
      throw NLException("cannot deserialize bus term metadata: bit not found"); // LCOV_EXCL_LINE
    }
    loadAttributes(bitInfo, bit);
    if (bitInfo.hasRtlInfos()) {
      loadRTLInfos(SNLRTLInfos::create(bit), bitInfo.getRtlInfos());
    }
  }
}

void loadDesignParameter(
  SNLDesign* design,
  const SNLDesignInterface::Parameter::Reader& parameter) {
  auto name = parameter.getName();
  auto type = parameter.getType();
  if (parameter.hasValue()) {
    SNLParameter::create(design, NLName(name), CapnPtoSNLParameterType(type), parameter.getValue());
  } else {
    SNLParameter::create(design, NLName(name), CapnPtoSNLParameterType(type));
  }
}

void loadDesignInterface(
  NLLibrary* library,
  const SNLDesignInterface::Reader& designInterface) {
  auto designID = designInterface.getId();
  auto designType = designInterface.getType();
  NLName snlName;
  if (designInterface.hasName()) {
    snlName = NLName(designInterface.getName());
  }
  SNLDesign* snlDesign = SNLDesign::create(library, NLID::DesignID(designID), CapnPtoSNLDesignType(designType), snlName);
  loadAttributes(designInterface, snlDesign);
  if (designInterface.hasRtlInfos()) {
    loadRTLInfos(SNLRTLInfos::create(snlDesign), designInterface.getRtlInfos());
  }
   if (designInterface.hasProperties()) {
    auto lambda = [](const SNLDesignInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(designInterface, snlDesign, lambda);
  }
  if (designInterface.hasParameters()) {
    for (auto parameter: designInterface.getParameters()) {
      loadDesignParameter(snlDesign, parameter);
    }
  }
  if (designInterface.hasTerms()) {
    for (auto term: designInterface.getTerms()) {
      if (term.isScalarTerm()) {
        auto scalarTerm = term.getScalarTerm();
        loadScalarTerm(snlDesign, scalarTerm);
      } else if (term.isBusTerm()) {
        auto busTerm = term.getBusTerm();
        loadBusTerm(snlDesign, busTerm);
      }
    }
  }
  loadFunctions(designInterface, snlDesign);
}

void loadLibraryInterface(
  NajaObject* parent,
  const DBInterface::LibraryInterface::Reader& libraryInterface,
  SNLCapnP::LoadingConfiguration loadingConfiguration, naja::NajaProgressReporter& progress) {
  NLLibrary* parentLibrary = nullptr;
  NLDB* parentDB = dynamic_cast<NLDB*>(parent);
  if (not parentDB) {
    parentLibrary = static_cast<NLLibrary*>(parent);
  }
  auto libraryID = libraryInterface.getId();
  auto libraryType = libraryInterface.getType();
  if (libraryType == DBInterface::LibraryType::PRIMITIVES) {
    auto universe = NLUniverse::get();
    // LCOV_EXCL_START
    if (not universe) {
      std::ostringstream reason;
      reason << "Cannot load library interface: no existing universe";
      throw NLException(reason.str());
    }
    // LCOV_EXCL_STOP
    NLLibrary* existingPrimitives = nullptr;
    if (parentDB) {
      existingPrimitives = universe->getLibrary(parentDB->getID(), libraryInterface.getId());
    // LCOV_EXCL_START
    } else {
      existingPrimitives = universe->getLibrary(parentLibrary->getDB()->getID(), libraryInterface.getId());
    }
    // LCOV_EXCL_STOP
    if (existingPrimitives) {
      if (loadingConfiguration.primitiveConflictPolicy_ ==
          SNLCapnP::LoadingConfiguration::PrimitiveConflictPolicy::PreferExisting) {
        return;
      }
      if (loadingConfiguration.primitiveConflictPolicy_ ==
          SNLCapnP::LoadingConfiguration::PrimitiveConflictPolicy::ForbidConflicts) {
        std::ostringstream reason;
        reason << "Cannot load library interface: primitives library already loaded";
        throw NLException(reason.str());
      }
    }
  }
  NLName snlName;
  if (libraryInterface.hasName()) {
    snlName = NLName(libraryInterface.getName());
  }
  NLLibrary* snlLibrary = nullptr;
  if (parentDB) {
    snlLibrary = NLLibrary::create(parentDB, NLID::LibraryID(libraryID), CapnPtoNLLibraryType(libraryType), snlName);
  } else {
    snlLibrary = NLLibrary::create(parentLibrary, NLID::LibraryID(libraryID), CapnPtoNLLibraryType(libraryType), snlName);
  }
  if (libraryInterface.hasProperties()) {
    auto lambda = [](const DBInterface::LibraryInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(libraryInterface, snlLibrary, lambda);
  }
  if (libraryInterface.hasSnlDesignInterfaces()) {
    for (auto designInterface: libraryInterface.getSnlDesignInterfaces()) {
      loadDesignInterface(snlLibrary, designInterface);
      if (progress) progress.advance(designInterface.getName().cStr());
    } 
  }
  if (libraryInterface.hasLibraryInterfaces()) {
    for (auto subLibraryInterface: libraryInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snlLibrary, subLibraryInterface, loadingConfiguration, progress);
    }
  }
  if (snlLibrary->isPrimitives()) {
    NLLibraryTruthTables::construct(snlLibrary);
  }
}

}

namespace naja::NL {

void SNLCapnP::dumpInterface(const NLDB* snlDB, int fileDescriptor) {
  dumpInterface(snlDB, fileDescriptor, snlDB->getID());
}

void SNLCapnP::dumpInterface(const NLDB* snlDB, int fileDescriptor, NLID::DBID forceDBID) {
  ::capnp::MallocMessageBuilder message;

  DBInterface::Builder db = message.initRoot<DBInterface>();
  db.setId(forceDBID);
  auto lambda = [](DBInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(db, snlDB, lambda);

  auto libraries = db.initLibraryInterfaces(snlDB->getLibraries().size());
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryInterfaceBuilder = libraries[id++];
    dumpLibraryInterface(libraryInterfaceBuilder, snlLibrary);
  }

  if (auto topDesign = snlDB->getTopDesign()) {
    auto designReference = topDesign->getReference();
    auto designReferenceBuilder = db.initTopDesignReference();
    designReferenceBuilder.setDbID(designReference.dbID_);
    designReferenceBuilder.setLibraryID(designReference.getDBDesignReference().libraryID_);
    designReferenceBuilder.setDesignID(designReference.getDBDesignReference().designID_);
  }

  writePackedMessageToFd(fileDescriptor, message);
}

void SNLCapnP::dumpInterface(const NLDB* snlDB, const std::filesystem::path& interfacePath) {
#ifdef _WIN32
  int fd = _open(
    interfacePath.string().c_str(),
    _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY,
    _S_IREAD | _S_IWRITE);
#else
  int fd = open(
    interfacePath.c_str(),
    O_CREAT | O_TRUNC | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif

  dumpInterface(snlDB, fd);

#ifdef _WIN32
  _close(fd);
#else
  close(fd);
#endif
}

//Need to find a proper way to test serialization on the wire
//LCOV_EXCL_START
//void SNLCapnP::sendInterface(const NLDB* db, tcp::socket& socket, NLID::DBID forceDBID) {
//  dumpInterface(db, socket.native_handle(), forceDBID);
//}
//
//void SNLCapnP::sendInterface(const NLDB* db, tcp::socket& socket) {
//  sendInterface(db, socket, db->getID());
//}
//
//void SNLCapnP::sendInterface(
//  const NLDB* db,
//  const std::string& ipAddress,
//  uint16_t port) {
//  boost::asio::io_context ioContext;
//  //socket creation
//  tcp::socket socket(ioContext);
//  socket.connect(tcp::endpoint(boost::asio::ip::make_address(ipAddress), port));
//  sendInterface(db, socket);
//}
//LCOV_EXCL_STOP

NLDB* SNLCapnP::loadInterface(
  int fileDescriptor,
  LoadingConfiguration loadingConfiguration) {
  naja::NajaProgressReporter progress;
  progress.setCallback(loadingConfiguration.progress);
  progress.begin("construct", "snapshot interfaces");
  ::capnp::PackedFdMessageReader message(fileDescriptor);

  DBInterface::Reader dbInterface = message.getRoot<DBInterface>();
  auto dbID = dbInterface.getId();
  auto universe = NLUniverse::get();
  if (not universe) {
    universe = NLUniverse::create();
  }
  if (dbID == 0) {
    for (auto library : dbInterface.getLibraryInterfaces()) loadDB0LibraryInterface(library);
    progress.finish();
    if (!loadingConfiguration.nested_) progress.begin("done");
    return NLDB0::getDB0();
  }
  NLDB* snldb = universe->getDB(dbID);
  if (not snldb) {
    snldb = NLDB::create(universe, dbID);
  }
  
  if (dbInterface.hasProperties()) {
    auto lambda = [](const DBInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(dbInterface, snldb, lambda);
  }
  
  if (dbInterface.hasLibraryInterfaces()) {
    for (auto libraryInterface: dbInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snldb, libraryInterface, loadingConfiguration, progress);
    }
  }
  if (dbInterface.hasTopDesignReference()) {
    auto designReference = dbInterface.getTopDesignReference();
    auto snlDesignReference =
      NLID::DesignReference(
        designReference.getDbID(),
        designReference.getLibraryID(),
        designReference.getDesignID());
    auto topDesign = NLUniverse::get()->getSNLDesign(snlDesignReference);
    if (not topDesign) {
      //LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot deserialize top design: no design found with provided reference";
      throw NLException(reason.str());
      //LCOV_EXCL_STOP
    }
    snldb->setTopDesign(topDesign);
  }
  progress.finish();
  if (!loadingConfiguration.nested_) progress.begin("done");
  return snldb;
}

NLDB* SNLCapnP::loadInterface(
  const std::filesystem::path& interfacePath,
  LoadingConfiguration loadingConfiguration) {
  //FIXME: verify if file can be opened
  int fd = 0;
#ifdef _WIN32
  fd = _open(interfacePath.string().c_str(), _O_RDONLY | _O_BINARY);
#else
  fd = open(interfacePath.c_str(), O_RDONLY);
#endif
  try {
    auto* db = loadInterface(fd, loadingConfiguration);
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
//NLDB* SNLCapnP::receiveInterface(tcp::socket& socket) {
//  return loadInterface(socket.native_handle());
//}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
//NLDB* SNLCapnP::receiveInterface(uint16_t port) {
//  boost::asio::io_context ioContext;
//  //listen for new connection
//  tcp::acceptor acceptor_(ioContext, tcp::endpoint(tcp::v4(), port));
//  //socket creation 
//  tcp::socket socket(ioContext);
//  //waiting for connection
//  acceptor_.accept(socket);
//  NLDB* db = receiveInterface(socket);
//  return db;
//}
//LCOV_EXCL_STOP

}  // namespace naja::NL
