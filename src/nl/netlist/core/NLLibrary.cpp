// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLLibrary.h"

#include <sstream>

#include "NajaLog.h"

#include "NLDB.h"
#include "NLException.h"
#include "SNLMacros.h"

namespace naja::NL {

NLLibrary::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string NLLibrary::Type::getString() const {
  switch (typeEnum_) {
    case Type::Standard: return "Standard";
    case Type::InDB0: return "InDB0";
    case Type::Primitives: return "Primitives";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

NLLibrary::NLLibrary(NLDB* parent, Type type, const NLName& name):
  super(),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(true)
{}

NLLibrary::NLLibrary(NLDB* parent, NLID::LibraryID id, Type type, const NLName& name):
  super(),
  id_(id),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(true)
{}

NLLibrary::NLLibrary(NLLibrary* parent, Type type, const NLName& name):
  super(),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(false)
{}

NLLibrary::NLLibrary(NLLibrary* parent, NLID::LibraryID id, Type type, const NLName& name):
  super(),
  id_(id),
  name_(name),
  type_(type),
  parent_(parent),
  isRoot_(false)
{}

NLLibrary* NLLibrary::create(NLDB* db, const NLName& name) {
  preCreate(db, Type::Standard, name);
  NLLibrary* library = new NLLibrary(db, Type::Standard, name);
  library->postCreateAndSetID();
  return library;
}

NLLibrary* NLLibrary::create(NLDB* db, Type type, const NLName& name) {
  preCreate(db, type, name);
  NLLibrary* library = new NLLibrary(db, type, name);
  library->postCreateAndSetID();
  return library;
}

NLLibrary* NLLibrary::create(NLDB* db, NLID::LibraryID libraryID, const Type type, const NLName& name) {
  preCreate(db, libraryID, type, name);
  NLLibrary* library = new NLLibrary(db, libraryID, type, name);
  library->postCreate();
  return library;
}

NLLibrary* NLLibrary::create(NLLibrary* parent, const NLName& name) {
  preCreate(parent, Type::Standard, name);
  NLLibrary* library = new NLLibrary(parent, Type::Standard, name);
  library->postCreateAndSetID();
  return library;
}

NLLibrary* NLLibrary::create(NLLibrary* parent, Type type, const NLName& name) {
  preCreate(parent, type, name);
  NLLibrary* library = new NLLibrary(parent, type, name);
  library->postCreateAndSetID();
  return library;
}

NLLibrary* NLLibrary::create(NLLibrary* parent, NLID::LibraryID id, Type type, const NLName& name) {
  preCreate(parent, id, type, name);
  NLLibrary* library = new NLLibrary(parent, id, type, name);
  library->postCreate();
  return library;
}

void NLLibrary::preCreate(NLDB* db, Type type, const NLName& name) {
  super::preCreate();
  if (not db) {
    throw NLException("malformed NLLibrary creator with NULL db argument");
  }
  if (not name.empty() and db->getLibrary(name)) {
    std::string reason = "NLDB " + db->getString() + " contains already a NLLibrary named: " + name.getString();
    throw NLException(reason);
  }
}

void NLLibrary::preCreate(NLDB* db, NLID::LibraryID libraryID, Type type, const NLName& name) {
  preCreate(db, type, name);
  if (db->getLibrary(libraryID)) {
    std::string reason = "NLDB " + db->getString() + " contains already a NLLibrary with ID: " + std::to_string(libraryID);
    throw NLException(reason);
  }
}

void NLLibrary::preCreate(NLLibrary* parentLibrary, Type type, const NLName& name) {
  super::preCreate();
  if (not parentLibrary) {
    throw NLException("malformed NLLibrary creator with NULL parent library argument");
  }
  if (type not_eq parentLibrary->getType()) {
    throw NLException("non compatible types in library constructor");
  }
  if (not name.empty() and parentLibrary->getLibrary(name)) {
    std::string reason = "NLLibrary " + parentLibrary->getString() + " contains already a NLLibrary named: " + name.getString();
    throw NLException(reason);
  }
}

void NLLibrary::preCreate(NLLibrary* parentLibrary, NLID::LibraryID id, Type type, const NLName& name) {
  preCreate(parentLibrary, type, name);
  auto db = parentLibrary->getDB();
  if (db->getLibrary(id)) {
    std::string reason = "NLDB " + db->getString() + " contains already a NLLibrary with ID: " + std::to_string(id);
    throw NLException(reason);
  }
}

void NLLibrary::postCreateAndSetID() {
  super::postCreate();
  getDB()->addLibraryAndSetID(this);
  if (not isRoot()) {
    static_cast<NLLibrary*>(parent_)->addLibrary(this);
  }
}

void NLLibrary::postCreate() {
  super::postCreate();
  getDB()->addLibrary(this);
  if (not isRoot()) {
    static_cast<NLLibrary*>(parent_)->addLibrary(this);
  }
}

void NLLibrary::commonPreDestroy() {
  NAJA_LOG_TRACE("Destroying {}", getDescription());
  struct destroySNLDesignFromLibrary {
    void operator()(SNLDesign* design) {
      design->destroyFromLibrary();
    }
  };
  struct destroyPNLDesignFromLibrary {
    void operator()(PNLDesign* design) {
      design->destroyFromLibrary();
    }
  };
  snlDesigns_.clear_and_dispose(destroySNLDesignFromLibrary());
  pnlDesigns_.clear_and_dispose(destroyPNLDesignFromLibrary());
  struct destroyLibraryFromLibrary {
    void operator()(NLLibrary* library) {
      library->destroyFromParentLibrary();
    }
  };
  libraries_.clear_and_dispose(destroyLibraryFromLibrary());
  super::preDestroy();
}

void NLLibrary::destroyFromDB() {
  commonPreDestroy();
  delete this;
}

void NLLibrary::destroyFromParentLibrary() {
  getDB()->removeLibrary(this);
  commonPreDestroy();
  delete this;
}

void NLLibrary::preDestroy() {
  getDB()->removeLibrary(this);
  if (not isRoot()) {
    static_cast<NLLibrary*>(parent_)->removeLibrary(this);
  }
  commonPreDestroy();
}

void NLLibrary::setName(const NLName& name) {
  if (name_ == name) {
    return;
  }
  if (not name.empty()) {
    if (isRoot()) {
      //check in DB
      auto db = getDB();
      if (auto collision = db->getLibrary(name)) {
        std::ostringstream reason;
        //Exclude from coverage to avoid false positive
        //LCOV_EXCL_START
        reason << "In DB " << db->getString() << ", cannot rename "
          << getString() << " to " << name.getString() << ", another library: "
          << collision->getString() << " has already this name.";
        //LCOV_EXCL_STOP
        throw NLException(reason.str());
      }
    } else {
      auto parentLibrary = getParentLibrary();
      if (auto collision = parentLibrary->getLibrary(name)) {
        std::ostringstream reason;
        //LCOV_EXCL_START
        reason << "In parent library " << parentLibrary->getString()
          << ", cannot rename " << getString() << " to " << name.getString()
          << ", another library: " << collision->getString() << " has already this name.";
        //LCOV_EXCL_STOP
        throw NLException(reason.str());
      }
    }
  }
  auto previousName = getName();
  name_ = name;
  if (isRoot()) {
    getDB()->rename(this, previousName);
  } else {
    getParentLibrary()->rename(this, previousName);
  }
}

OWNER_RENAME(NLLibrary, NLLibrary, libraryNameIDMap_)
OWNER_RENAME(NLLibrary, SNLDesign, designNameIDMap_)
OWNER_RENAME(NLLibrary, PNLDesign, pnlDesignNameIDMap_)

NLDB* NLLibrary::getDB() const {
  if (isRoot()) {
    return static_cast<NLDB*>(parent_);
  } else {
    return getParentLibrary()->getDB();
  }
}

NLLibrary* NLLibrary::getParentLibrary() const {
  if (not isRoot()) {
    return static_cast<NLLibrary*>(parent_);
  }
  return nullptr;
}

NLLibrary* NLLibrary::getLibrary(NLID::LibraryID id) const {
  auto it = libraries_.find(NLID(getDB()->getID(), id), NLIDComp<NLLibrary>());
  if (it != libraries_.end()) {
    return const_cast<NLLibrary*>(&*it);
  }
  return nullptr;
}

NLLibrary* NLLibrary::getLibrary(const NLName& name) const {
  auto lit = libraryNameIDMap_.find(name);
  if (lit != libraryNameIDMap_.end()) {
    NLID::LibraryID id = lit->second;
    return getLibrary(id);
  }
  return nullptr;
}

NajaCollection<NLLibrary*> NLLibrary::getLibraries() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&libraries_));
}

SNLDesign* NLLibrary::getSNLDesign(NLID::DesignID id) const {
  auto it = snlDesigns_.find(NLID(getDB()->getID(), getID(), id), NLIDComp<SNLDesign>());
  if (it != snlDesigns_.end()) {
    return const_cast<SNLDesign*>(&*it);
  }
  return nullptr;
}

SNLDesign* NLLibrary::getSNLDesignCaseInsensitive(const NLName& name) const {
  if (name.empty()) return nullptr;
  const auto folded = foldNameCase(name.getString());
  SNLDesign* result = nullptr;
  for (auto* candidate : getSNLDesigns()) {
    if (foldNameCase(candidate->getName().getString()) != folded) continue;
    if (result) throw NLException("ambiguous case-insensitive getSNLDesign: " + name.getString());
    result = candidate;
  }
  return result;
}

SNLDesign* NLLibrary::getSNLDesign(const NLName& name) const {
  auto dit = designNameIDMap_.find(name);
  if (dit != designNameIDMap_.end()) {
    NLID::DesignID id = dit->second;
    return getSNLDesign(id);
  }
  return nullptr;
}

NajaCollection<SNLDesign*> NLLibrary::getSNLDesigns() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&snlDesigns_));
}

NajaCollection<PNLDesign*> NLLibrary::getPNLDesigns() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&pnlDesigns_));
}

void NLLibrary::addLibrary(NLLibrary* library) {
  libraries_.insert(*library);
  if (not library->isUnnamed()) {
    libraryNameIDMap_[library->getName()] = library->id_;
  }
}

void NLLibrary::removeLibrary(NLLibrary* library) {
  if (not library->isUnnamed()) {
    libraryNameIDMap_.erase(library->getName());
  }
  libraries_.erase(*library);
}

void NLLibrary::addSNLDesignAndSetID(SNLDesign* design) {
  if (snlDesigns_.empty()) {
    design->id_ = 0;
  } else {
    auto it = snlDesigns_.rbegin();
    SNLDesign* lastDesign = &(*it);
    NLID::DesignID designID = lastDesign->id_+1;
    design->id_ = designID;
  }
  addSNLDesign(design);
}

void NLLibrary::addSNLDesign(SNLDesign* design) {
  snlDesigns_.insert(*design);
  if (not design->isUnnamed()) {
    designNameIDMap_[design->getName()] = design->id_;
  }
}

void NLLibrary::removeSNLDesign(SNLDesign* design) {
  if (not design->isUnnamed()) {
    designNameIDMap_.erase(design->getName());
  }
  snlDesigns_.erase(*design);
}

void NLLibrary::addPNLDesignAndSetID(PNLDesign* design) {
  if (pnlDesigns_.empty()) {
    design->id_ = 0;
  } else {
    auto it = pnlDesigns_.rbegin();
    PNLDesign* lastDesign = &(*it);
    NLID::DesignID designID = lastDesign->id_+1;
    design->id_ = designID;
  }
  addPNLDesign(design);
}

void NLLibrary::addPNLDesign(PNLDesign* design) {
  pnlDesigns_.insert(*design);
  if (not design->isUnnamed()) {
    pnlDesignNameIDMap_[design->getName()] = design->id_;
  }
}

void NLLibrary::removePNLDesign(PNLDesign* design) {
  if (not design->isUnnamed()) {
    pnlDesignNameIDMap_.erase(design->getName());
  }
  pnlDesigns_.erase(*design);
}

PNLDesign* NLLibrary::getPNLDesign(NLID::DesignID id) const {
  auto it = pnlDesigns_.find(NLID(getDB()->getID(), getID(), id), NLIDComp<PNLDesign>());
  if (it != pnlDesigns_.end()) {
    return const_cast<PNLDesign*>(&*it);
  }
  return nullptr;
}

PNLDesign* NLLibrary::getPNLDesign(const NLName& name) const {
  auto dit = pnlDesignNameIDMap_.find(name);
  if (dit != pnlDesignNameIDMap_.end()) {
    NLID::DesignID id = dit->second;
    printf("getPNLDesign name %s\n", name.getString().c_str());
    printf("getPNLDesign found %s\n", getPNLDesign(id)->getName().getString().c_str());
    printf("getPNLDesign id %d\n", getPNLDesign(id)->getID());
    return getPNLDesign(id);
  }
  return nullptr;
}

bool NLLibrary::deepCompare(const NLLibrary* other, std::string& reason) const {
  if (getID() not_eq other->getID()) {
    std::ostringstream oss;
    oss << "Libraries mismatch between ";
    oss << getDescription() << " and " << other->getDescription();
    oss << " (ID mismatch)";
    reason = oss.str();
    return false;
  }
  if (name_ not_eq other->getName()) {
    return false;
  }
  if (type_ not_eq other->getType()) {
    return false;
  }
  DEEP_COMPARE_MEMBER(SNLDesigns, this, other)
  DEEP_COMPARE_MEMBER(PNLDesigns, this, other)
  return true;
}

void NLLibrary::mergeAssigns() {
  if (isPrimitives()) {
    return;
  }
  for (auto design: getSNLDesigns()) {
    if (not design->isPrimitive()) {
      design->mergeAssigns();
    }
  }
}

//LCOV_EXCL_START
const char* NLLibrary::getTypeName() const {
  return "NLLibrary";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string NLLibrary::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string NLLibrary::getDescription() const {
  std::ostringstream description;
  description << "<" + std::string(getTypeName());
  if (not isUnnamed()) {
    description << " " + name_.getString();
  }
  if (isPrimitives()) {
    description << " (prim)";
  }
  description << " " << getID();
  description << ">";  
  return description.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void NLLibrary::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    for (auto design: getSNLDesigns()) {
      design->debugDump(indent+2, recursive, stream);
    }
    for (auto design: getPNLDesigns()) {
      design->debugDump(indent+2, recursive, stream);
    }
  }
}
//LCOV_EXCL_STOP

NLID NLLibrary::getNLID() const {
  return NLID(getDB()->getID(), getID());
}

}  // namespace naja::NL