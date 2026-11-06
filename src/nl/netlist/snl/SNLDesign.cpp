// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesign.h"

#include <algorithm>
#include <list>
#include <sstream>
#include <vector>

#include "NajaLog.h"

#include "NLException.h"
#include "NLDB.h" 
#include "NLDB0.h"
#include "NLLibrary.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBundleTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLAttributes.h"
#include "SNLMacros.h"
#include "SNLDesignModeling.h"
#include "SNLExceptions.h"
#include "SNLRTLInfos.h"
#include "SNLUtils.h"

namespace naja::NL {

SNLDesign::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string SNLDesign::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard: return "Standard";
    case Type::UserBlackBox: return "UserBlackBox";
    case Type::AutoBlackBox: return "AutoBlackBox";
    case Type::Primitive: return "Primitive";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

SNLDesign* SNLDesign::create(NLLibrary* library, const NLName& name) {
  preCreate(library, Type::Standard, name);
  SNLDesign* design = new SNLDesign(library, Type::Standard, name);
  design->postCreateAndSetID();
  return design;
}

SNLDesign* SNLDesign::create(NLLibrary* library, Type type, const NLName& name) {
  preCreate(library, type, name);
  SNLDesign* design = new SNLDesign(library, type, name);
  design->postCreateAndSetID();
  return design;
}

SNLDesign* SNLDesign::create(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name) {
  preCreate(library, id, type, name);
  SNLDesign* design = new SNLDesign(library, id, type, name);
  design->postCreate();
  return design;
}

SNLDesign::SNLDesign(NLLibrary* library, Type type, const NLName& name):
  super(),
  name_(name),
  type_(type),
  library_(library)
{}

SNLDesign::SNLDesign(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name):
  super(),
  id_(id),
  name_(name),
  type_(type),
  library_(library)
{}

void SNLDesign::preCreate(const NLLibrary* library, Type type, const NLName& name) {
  super::preCreate();
  if (not library) {
    throw NLException("malformed design creator with null library");
  }
  if (type == Type::Primitive and not library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot create a primitive design";
    if (name.empty()) {
      reason << " <anonymous>";
    } else {
      reason << " named: " << name.getString();
    }
    reason << " in a non primitives library: " << library->getString();
    throw NLException(reason.str());
  }
  if (type != Type::Primitive and library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot create a non primitive design";
    if (name.empty()) {
      reason << " <anonymous>";
    } else {
      reason << " named: " << name.getString();
    }
    reason << " in a primitives library: " << library->getString();
    throw NLException(reason.str());
  }
  //test if design with same name exists in library
  if (not name.empty() and library->getSNLDesign(name)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a SNLDesign named: " + name.getString();
    throw SNLDesignNameConflictException(reason);
  }
}

void SNLDesign::preCreate(const NLLibrary* library, NLID::DesignID id, Type type, const NLName& name) {
  SNLDesign::preCreate(library, type, name);
  //test if design with same id exists in library
  if (library->getSNLDesign(id)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a SNLDesign with ID: " + std::to_string(id);
    throw NLException(reason);
  }
}

void SNLDesign::postCreate() {
  super::postCreate();
  library_->addSNLDesign(this);
}

void SNLDesign::postCreateAndSetID() {
  super::postCreate();
  library_->addSNLDesignAndSetID(this);
}

void SNLDesign::commonPreDestroy() {
  NAJA_LOG_TRACE("Destroying {}", getDescription());
  //Clocks reference terminals of this design, so they go before those terminals.
  std::vector<NLClock*> clocks = clocks_;
  for (NLClock* clock: clocks) {
    clock->destroy();
  }
  auto resets = resets_;
  for (auto* reset : resets) {
    reset->destroy();
  }
  if (rtlInfos_) {
    rtlInfos_->destroy();
  }
  struct destroyInstanceFromDesign {
    void operator()(SNLInstance* instance) {
      instance->destroyFromDesign();
    }
  };
  instances_.clear_and_dispose(destroyInstanceFromDesign());

  if (not isPrimitive()) {
    struct destroySlaveInstanceFromModel {
      void operator()(SNLInstance* instance) {
        instance->destroyFromModel();
      }
    };
    slaveInstances_.clear_and_dispose(destroySlaveInstanceFromModel());
  }

  std::vector<SNLTerm*> topLevelTerms(getTerms().begin(), getTerms().end());
  for (auto* term: topLevelTerms) {
    term->destroyFromDesign();
  }
  terms_.clear();
  termNameIDMap_.clear();

  struct destroyNetFromDesign {
    void operator()(SNLNet* net) {
      net->destroyFromDesign();
    }
  };
  nets_.clear_and_dispose(destroyNetFromDesign());

  struct destroyParameterFromDesign {
    void operator()(SNLParameter* parameter) {
      parameter->destroyFromDesign();
    }
  };
  parameters_.clear_and_dispose(destroyParameterFromDesign());

  super::preDestroy();
}

void SNLDesign::destroyFromLibrary() {
  commonPreDestroy();
  delete this;
}

void SNLDesign::preDestroy() {
  if (isPrimitive()) {
    //FIXME: Error
  }
  library_->removeSNLDesign(this);
  commonPreDestroy();
}

NajaCollection<NLReset*> SNLDesign::getResets() const { return new NajaSTLCollection(&resets_); }
void SNLDesign::addReset(NLReset* reset) { resets_.push_back(reset); }
void SNLDesign::removeReset(NLReset* reset) {
  resets_.erase(std::remove(resets_.begin(), resets_.end(), reset), resets_.end());
}

NajaCollection<NLClock*> SNLDesign::getClocks() const {
  return new NajaSTLCollection(&clocks_);
}

void SNLDesign::addClock(NLClock* clock) {
  clocks_.push_back(clock);
}

void SNLDesign::removeClock(NLClock* clock) {
  clocks_.erase(std::remove(clocks_.begin(), clocks_.end(), clock), clocks_.end());
}

void SNLDesign::addTermAndSetID(SNLTerm* term) {
 if (terms_.empty()) {
    term->setID(0);
  } else {
    auto it = terms_.rbegin();
    SNLTerm* lastTerm = &(*it);
    NLID::DesignObjectID termID = lastTerm->getID()+1;
    term->setID(termID);
  }
  addTerm(term);
}

void SNLDesign::addTerm(SNLTerm* term) {
  assert(dynamic_cast<SNLScalarTerm*>(term) or dynamic_cast<SNLBusTerm*>(term) or dynamic_cast<SNLBundleTerm*>(term));

  if (terms_.empty()) {
    term->setFlatID(0);
  } else {
    auto it = terms_.rbegin();
    SNLTerm* lastTerm = &(*it);
    term->setFlatID(lastTerm->getFlatID() + static_cast<size_t>(lastTerm->getWidth()));
  }
  terms_.insert(*term);
  if (auto bundle = term->getBundleOwner()) {
    bundle->addMember(term);
  }
  if (not term->getName().empty()) {
    termNameIDMap_[term->getName()] = term->getID();
  }

  //Create corresponding instance terminals in slave instances
  for (auto instance: getSlaveInstances()) {
    if (SNLScalarTerm* scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
      instance->createInstTerm(scalarTerm);
    } else if (SNLBusTerm* busTerm = dynamic_cast<SNLBusTerm*>(term)) {
      for (auto bit: busTerm->getBits()) {
        instance->createInstTerm(bit);
      }
    }
  }
  setOrderIDs();
}

void SNLDesign::removeTerm(SNLTerm* term) {
  //Remove corresponding instance terminals in slave instances
  for (auto instance: getSlaveInstances()) {
    if (SNLBusTerm* bus = dynamic_cast<SNLBusTerm*>(term)) {
      for (auto bit: bus->getBits()) {
        instance->removeInstTerm(bit);
      }
    } else if (dynamic_cast<SNLScalarTerm*>(term)) {
      SNLBitTerm* bitTerm = static_cast<SNLBitTerm*>(term);
      instance->removeInstTerm(bitTerm);
    }
  }
  if (not term->getName().empty()) {
    termNameIDMap_.erase(term->getName());
  }
  terms_.erase(*term);
  if (auto bundle = term->getBundleOwner()) {
    bundle->removeMember(term);
  }
  setOrderIDs();
}

SNLTerm* SNLDesign::getTerm(NLID::DesignObjectID id) const {
  auto it = terms_.find(id, NLDesign::CompareByID<SNLTerm>());
  if (it != terms_.end()) {
    return const_cast<SNLTerm*>(&*it);
  }
  return nullptr;
}

SNLTerm* SNLDesign::getTerm(const NLName& name) const {
  auto it = termNameIDMap_.find(name);
  if (it != termNameIDMap_.end()) {
    NLID::DesignObjectID id = it->second;
    return getTerm(id);
  }
  return nullptr;
}

SNLBitTerm* SNLDesign::getBitTerm(NLID::DesignObjectID id, NLID::Bit bit) const {
  SNLBitTerm* bitTerm = getScalarTerm(id);
  if (not bitTerm) {
    bitTerm = getBusTermBit(id, bit);
  }
  return bitTerm;
}

SNLScalarTerm* SNLDesign::getScalarTerm(NLID::DesignObjectID id) const {
  return dynamic_cast<SNLScalarTerm*>(getTerm(id));
}

SNLScalarTerm* SNLDesign::getScalarTerm(const NLName& name) const {
  return dynamic_cast<SNLScalarTerm*>(getTerm(name));
}

SNLBusTermBit* SNLDesign::getBusTermBit(NLID::DesignObjectID id, NLID::Bit bit) const {
  auto bus = getBusTerm(id);
  if (bus) {
    return bus->getBit(bit);
  }
  return nullptr;
}

SNLBusTerm* SNLDesign::getBusTerm(NLID::DesignObjectID id) const {
  return dynamic_cast<SNLBusTerm*>(getTerm(id));
}

SNLBusTerm* SNLDesign::getBusTerm(const NLName& name) const {
  return dynamic_cast<SNLBusTerm*>(getTerm(name));
}

SNLBundleTerm* SNLDesign::getBundleTerm(NLID::DesignObjectID id) const {
  return dynamic_cast<SNLBundleTerm*>(getTerm(id));
}

SNLBundleTerm* SNLDesign::getBundleTerm(const NLName& name) const {
  return dynamic_cast<SNLBundleTerm*>(getTerm(name));
}

NajaCollection<SNLTerm*> SNLDesign::getTerms() const {
  auto filter = [](const SNLTerm* term) { return term->isTopLevelTerm(); };
  return NajaCollection(new NajaIntrusiveSetCollection(&terms_)).getSubCollection(filter);
}

NajaCollection<SNLBusTerm*> SNLDesign::getBusTerms() const {
  return getTerms().getSubCollection<SNLBusTerm*>();
}

NajaCollection<SNLScalarTerm*> SNLDesign::getScalarTerms() const {
  return getTerms().getSubCollection<SNLScalarTerm*>();
}

NajaCollection<SNLBundleTerm*> SNLDesign::getBundleTerms() const {
  return getTerms().getSubCollection<SNLBundleTerm*>();
}

NajaCollection<SNLBitTerm*> SNLDesign::getBitTerms() const {
  auto flattener = [](SNLTerm* term) { return term->getBits(); };
  return getTerms().getFlatCollection<SNLTerm*, SNLBitTerm*, SNLBitTerm*>(flattener);
}

void SNLDesign::addInstanceAndSetID(SNLInstance* instance) {
  if (instances_.empty()) {
    instance->id_ = 0;
  } else {
    auto it = instances_.rbegin();
    SNLInstance* lastInstance = &(*it);
    NLID::DesignObjectID instanceID = lastInstance->id_+1;
    instance->id_ = instanceID;
  }
  addInstance(instance);
}

void SNLDesign::addInstance(SNLInstance* instance) {
  instances_.insert(*instance);
  if (not instance->getName().empty()) {
    instanceNameIDMap_[instance->getName()] = instance->id_;
  }
}

void SNLDesign::removeInstance(SNLInstance* instance) {
  if (not instance->getName().empty()) {
    instanceNameIDMap_.erase(instance->getName());
  }
  instances_.erase(*instance);
}

NajaCollection<SNLInstance*> SNLDesign::getInstances() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&instances_));
}

NajaCollection<SNLInstance*> SNLDesign::getNonAssignInstances() const {
  auto filter = [](const SNLInstance* instance) { return not instance->getModel()->isAssign(); };
  return getInstances().getSubCollection(filter);
}

NajaCollection<SNLInstance*> SNLDesign::getAssignInstances() const {
  auto filter = [](const SNLInstance* instance) { return instance->getModel()->isAssign(); };
  return getInstances().getSubCollection(filter);
}

NajaCollection<SNLInstance*> SNLDesign::getSlaveInstances() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&slaveInstances_));
}

NajaCollection<SNLInstance*> SNLDesign::getPrimitiveInstances() const {
  auto filter = [](const SNLInstance* instance) { return instance->getModel()->isPrimitive(); };
  return getInstances().getSubCollection(filter);
}

NajaCollection<SNLInstance*> SNLDesign::getNonPrimitiveInstances() const {
  auto filter = [](const SNLInstance* instance) { return not instance->getModel()->isPrimitive(); };
  return getInstances().getSubCollection(filter);
}

SNLInstance* SNLDesign::getInstance(NLID::DesignObjectID id) const {
  auto it = instances_.find(id, NLDesign::CompareByID<SNLInstance>());
  if (it != instances_.end()) {
    return const_cast<SNLInstance*>(&*it);
  }
  return nullptr;
}

SNLInstance* SNLDesign::getInstance(const NLName& name) const {
  auto it = instanceNameIDMap_.find(name);
  if (it != instanceNameIDMap_.end()) {
    NLID::DesignObjectID id = it->second;
    return getInstance(id);
  }
  return nullptr;
}

void SNLDesign::addSlaveInstance(SNLInstance* instance) {
  //addSlaveInstance must be executed after addInstance.
  slaveInstances_.insert(*instance);
}

void SNLDesign::removeSlaveInstance(SNLInstance* instance) {
  slaveInstances_.erase(*instance);
}

void SNLDesign::addNetAndSetID(SNLNet* net) {
  if (nets_.empty()) {
    net->setID(0);
  } else {
    auto it = nets_.rbegin();
    SNLNet* lastNet = &(*it);
    NLID::DesignObjectID netID = lastNet->getID()+1;
    net->setID(netID);
  }
  addNet(net);
}

void SNLDesign::addNet(SNLNet* net) {
  nets_.insert(*net);
  if (not net->getName().empty()) {
    netNameIDMap_[net->getName()] = net->getID();
  }
}

void SNLDesign::removeNet(SNLNet* net) {
  assert(dynamic_cast<SNLScalarNet*>(net) or dynamic_cast<SNLBusNet*>(net));

  if (not net->getName().empty()) {
    netNameIDMap_.erase(net->getName());
  }
  nets_.erase(*net);
}

OWNER_RENAME(SNLDesign, SNLTerm, termNameIDMap_)
OWNER_RENAME(SNLDesign, SNLNet, netNameIDMap_)
OWNER_RENAME(SNLDesign, SNLInstance, instanceNameIDMap_)

SNLNet* SNLDesign::getNet(NLID::DesignObjectID id) const {
  auto it = nets_.find(id, NLDesign::CompareByID<SNLNet>());
  if (it != nets_.end()) {
    return const_cast<SNLNet*>(&*it);
  }
  return nullptr;
}

SNLNet* SNLDesign::getNet(const NLName& name) const {
  auto tit = netNameIDMap_.find(name);
  if (tit != netNameIDMap_.end()) {
    NLID::DesignObjectID id = tit->second;
    return getNet(id);
  }
  return nullptr;
}

SNLScalarNet* SNLDesign::getScalarNet(NLID::DesignObjectID id) const {
  return dynamic_cast<SNLScalarNet*>(getNet(id));
}

SNLScalarNet* SNLDesign::getScalarNet(const NLName& name) const {
  return dynamic_cast<SNLScalarNet*>(getNet(name));
}

SNLBusNet* SNLDesign::getBusNet(NLID::DesignObjectID id) const {
  return dynamic_cast<SNLBusNet*>(getNet(id));
}

SNLBusNet* SNLDesign::getBusNet(const NLName& name) const {
  return dynamic_cast<SNLBusNet*>(getNet(name));
}

SNLBusNetBit* SNLDesign::getBusNetBit(NLID::DesignObjectID id, NLID::Bit bit) const {
  auto bus = getBusNet(id);
  if (bus) {
    return bus->getBit(bit);
  }
  return nullptr;
}

SNLBitNet* SNLDesign::getBitNet(NLID::DesignObjectID id, NLID::Bit bit) const {
  SNLBitNet* bitNet = getScalarNet(id);
  if (not bitNet) {
    bitNet = getBusNetBit(id, bit);
  }
  return bitNet;
}

NajaCollection<SNLNet*> SNLDesign::getNets() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&nets_));
}

NajaCollection<SNLBusNet*> SNLDesign::getBusNets() const {
  return getNets().getSubCollection<SNLBusNet*>();
}

NajaCollection<SNLScalarNet*> SNLDesign::getScalarNets() const {
  return getNets().getSubCollection<SNLScalarNet*>();
}

NajaCollection<SNLBitNet*> SNLDesign::getBitNets() const {
  auto flattener = [](const SNLBusNet* b) { return b->getBusBits(); };
  return getNets().getFlatCollection<SNLBusNet*, SNLBusNetBit*, SNLBitNet*>(flattener);
}

NLDB* SNLDesign::getDB() const {
  return getLibrary()->getDB();
}

void SNLDesign::addParameter(SNLParameter* parameter) {
  parameters_.insert(*parameter);
}

void SNLDesign::removeParameter(SNLParameter* parameter) {
  parameters_.erase(*parameter);
}

SNLParameter* SNLDesign::getParameter(const NLName& name) const {
  auto it = parameters_.find(name, SNLParameter::SNLParameterComp());
  if (it != parameters_.end()) {
    return const_cast<SNLParameter*>(&*it);
  }
  return nullptr;
}

NajaCollection<SNLParameter*> SNLDesign::getParameters() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&parameters_));
}

NajaCollection<SNLAttribute> SNLDesign::getAttributes() const {
  return SNLAttributes::getAttributes(this);
}

void SNLDesign::setType(Type type) {
  if (type == Type::Primitive) {
    throw NLException("cannot change design type to Primitive");
  }
  type_ = type;
}

bool SNLDesign::isTopDesign() const {
  return getDB()->getTopDesign() == this; 
}

bool SNLDesign::isAssign() const {
  return NLDB0::isAssign(this);
}

bool SNLDesign::isMux() const {
  return SNLDesignModeling::isMux(this);
}

bool SNLDesign::deepCompare(
  const SNLDesign* other,
  std::string& reason,
  NLDesign::CompareType type) const {
  if (type==NLDesign::CompareType::Complete and (getID() not_eq other->getID())) {
    return false; //LCOV_EXCL_LINE
  }
  if (type!=NLDesign::CompareType::IgnoreIDAndName and (name_ not_eq other->getName())) {
    return false; //LCOV_EXCL_LINE
  }
  if (type_ not_eq other->getType()) {
    return false; //LCOV_EXCL_LINE
  }
  DEEP_COMPARE_MEMBER(Terms, this, other)
  DEEP_COMPARE_MEMBER(Nets, this, other)
  DEEP_COMPARE_MEMBER(Parameters, this, other)
  DEEP_COMPARE_MEMBER(Instances, this, other)
  return SNLAttributes::compareAttributes(this, other, reason);
}

NLID SNLDesign::getNLID() const {
  return NLID(getDB()->getID(), library_->getID(), getID());
}

NLID::DesignReference SNLDesign::getReference() const {
  return NLID::DesignReference(getDB()->getID(), library_->getID(), getID());
}

bool SNLDesign::isBetween(int n, int MSB, int LSB) {
  int min = std::min(MSB, LSB);
  int max = std::max(MSB, LSB);
  return n>=min and n<=max;
}

void SNLDesign::mergeAssigns() {
  using Instances = std::list<SNLInstance*>;
  auto filter = [](const SNLInstance* it) { return it->getModel()->isAssign(); };
  Instances assignInstances(
      getInstances().getSubCollection(filter).begin(),
      getInstances().getSubCollection(filter).end());
  auto assignInput = NLDB0::getAssignInput();
  auto assignOutput = NLDB0::getAssignOutput(); 
  for (auto assignInstance: assignInstances) {
    auto assignInstanceInput = assignInstance->getInstTerm(assignInput);
    auto assignInstanceOutput = assignInstance->getInstTerm(assignOutput);
    auto assignInputNet = assignInstanceInput->getNet();
    auto assignOutputNet = assignInstanceOutput->getNet();
    //take all components for assignOutputNet and assign them to assignInputNet
    assignOutputNet->connectAllComponentsTo(assignInputNet);
    if (dynamic_cast<SNLScalarNet*>(assignOutputNet)) {
      assignOutputNet->destroy();
    }
  }
  for (auto assignInstance: assignInstances) {
    assignInstance->destroy();
  }
}

SNLDesign* SNLDesign::cloneInterfaceToLibrary(NLLibrary* library, const NLName& name) const {
  if (isPrimitive()) {
    throw NLException("cloneToLibrary cannot be called on primitive designs");
  }
  auto newDesign = SNLDesign::create(library, Type::Standard, name);
  newDesign->terms_.clone_from(
    terms_,
    [newDesign](const SNLTerm& term){
      return term.clone(newDesign);
    },
    [](SNLTerm*){} //LCOV_EXCL_LINE
  );
  newDesign->termNameIDMap_ = termNameIDMap_;
  //clone parameters
  newDesign->parameters_.clone_from(
    parameters_,
    [newDesign](const SNLParameter& parameter){ 
      return new SNLParameter(newDesign, parameter.name_, parameter.type_, parameter.value_);
    },
    [](SNLParameter*){} //LCOV_EXCL_LINE
  );
  SNLAttributes::cloneAttributes(this, newDesign);
  if (auto* infos = getRTLInfos()) {
    SNLRTLInfos::create(newDesign)->cloneInfos(*infos);
  }
  return newDesign;
}

SNLDesign* SNLDesign::cloneInterface(const NLName& name) const {
  return cloneInterfaceToLibrary(getLibrary(), name);
}

SNLDesign* SNLDesign::cloneToLibrary(NLLibrary* library, const NLName& name) const {
  if (not name.empty() and library->getSNLDesign(name)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a SNLDesign named: " + getName().getString();
    throw NLException(reason);
  }

  //start with interface uniquification
  auto newDesign = cloneInterfaceToLibrary(library, name);
  //clone instances
  newDesign->instances_.clone_from(
    instances_,
    [newDesign](const SNLInstance& instance){ return instance.clone(newDesign); },
    [](SNLInstance*){} //LCOV_EXCL_LINE
  );
  newDesign->instanceNameIDMap_ = instanceNameIDMap_;

  //clone nets
  newDesign->nets_.clone_from(
    nets_,
    [newDesign](const SNLNet& net){ return net.clone(newDesign); },
    [](SNLNet*){} //LCOV_EXCL_LINE
  );
  newDesign->netNameIDMap_ = netNameIDMap_;

  return newDesign;
}

SNLDesign* SNLDesign::clone(const NLName& name) const {
  return cloneToLibrary(getLibrary(), name);
}

void SNLDesign::setName(const NLName& name) {
  if (name_ == name) {
    return;
  }
  if (not name.empty()) {
    /* check collision */
    if (auto collision = getLibrary()->getSNLDesign(name)) {
      std::ostringstream reason;
      reason << "In library " << getLibrary()->getString()
        << ", cannot rename " << getString() << " to "
        << name.getString() << ", another Design " << collision->getString()
        << " has already this name.";
      throw NLException(reason.str());
    }
  }
  auto previousName = getName();
  name_ = name;
  getLibrary()->rename(this, previousName);
}

//LCOV_EXCL_START
const char* SNLDesign::getTypeName() const {
  return "SNLDesign";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDesign::getString() const {
  return SNLUtils::getNamedString(getName(), "design", getID());
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLDesign::getDescription() const {
  std::ostringstream stream;
  stream << "<" + std::string(getTypeName());
  if (not isUnnamed()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (isPrimitive()) {
    stream << " (prim)";
  }
  if (not getLibrary()->isUnnamed()) {
    stream << " " << getLibrary()->getName().getString();
  }
  stream << " " << getLibrary()->getID();
  stream << ">";
  return stream.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLDesign::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    if (not getTerms().empty()) {
      stream << std::string(indent+2, ' ') << "<terms>" << std::endl;
      for (auto term: getTerms()) {
        term->debugDump(indent+4, false, stream);
      }
      stream << std::string(indent+2, ' ') << "</terms>" << std::endl;
    }
    if (not getNets().empty()) {
      stream << std::string(indent+2, ' ') << "<nets>" << std::endl;
      for (auto net: getNets()) {
        net->debugDump(indent+4, recursive, stream);
      }
      stream << std::string(indent+2, ' ') << "</nets>" << std::endl;
    }
    if (not getInstances().empty()) {
      stream << std::string(indent+2, ' ') << "<instances>" << std::endl;
      for (auto instance: getInstances()) {
        instance->debugDump(indent+4, recursive, stream);
      }
      stream << std::string(indent+2, ' ') << "</instances>" << std::endl;
    }
  }
}
//LCOV_EXCL_STOP

void SNLDesign::recursiveRevisionIncrement() {
  incrementRevisionCount();
  for (auto instance: getInstances()) {
    instance->getModel()->recursiveRevisionIncrement();
  }
}

void SNLDesign::setOrderIDs() {
  NLID::DesignObjectID bitTermId = 0;
  for (auto term: getBitTerms()) {
    term->setOrderID(bitTermId);
    bitTermId++;
  }
}


}  // namespace naja::NL
