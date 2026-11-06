// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLDB.h"

#include <list>
#include <vector>
#include <sstream>

#include "NajaLog.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLException.h"
#include "SNLMacros.h"

namespace naja::NL {

NLDB::NLDB(NLID::DBID id):
  id_(id)
{}

NLDB* NLDB::create(NLUniverse* universe) {
  preCreate(universe);
  NLDB* db = new NLDB();
  db->postCreateAndSetID(universe);
  return db;
}

NLDB* NLDB::create(NLUniverse* universe, NLID::DBID id) {
  preCreate(universe, id);
  NLDB* db = new NLDB(id);
  db->postCreateWithUniverse(universe);
  return db;
}

void NLDB::preCreate(NLUniverse* universe) {
  super::preCreate();
  if (not universe) {
    throw NLException("DB creation: NULL Universe");
  }
}

void NLDB::preCreate(NLUniverse* universe, NLID::DBID id) {
  preCreate(universe);
  if (NLUniverse::get()->getDB(id)) {
    std::ostringstream oss;
    oss << "DB collision: duplicate or conflicting entry detected (id="
        << static_cast<unsigned int>(id) << ")";
    throw NLException(oss.str());
  }
}

void NLDB::postCreateAndSetID(NLUniverse* universe) {
  super::postCreate();
  universe->addDBAndSetID(this);
}

void NLDB::postCreateWithUniverse(NLUniverse* universe) {
  super::postCreate();
  universe->addDB(this);
}

void NLDB::commonPreDrestroy() {
  NAJA_LOG_TRACE("Destroying {}", getDescription());
  struct destroyLibraryFromDB {
    void operator()(NL::NLLibrary* library) {
      library->destroyFromDB();
    }
  };
  //First delete standard primitives
  //collect standard libraries
  using Libraries = std::list<NLLibrary*>;
  Libraries nonRootLibraries;
  Libraries standardRootLibraries;
  for (auto it = libraries_.begin(); it!=libraries_.end(); ++it) {
    NLLibrary* library = &*it;
    //destroy first root and standard library 
    if (not library->isRoot()) {
      nonRootLibraries.push_back(library);
    } else if (library->isStandard()) {
      standardRootLibraries.push_back(library);
    } 
  }
  for (auto library: nonRootLibraries) {
    libraries_.erase(*library);
  }
  for (auto library: standardRootLibraries) {
    libraries_.erase_and_dispose(*library, destroyLibraryFromDB());
  }

  //remove all the rest
  libraries_.clear_and_dispose(destroyLibraryFromDB());
  libraryNameIDMap_.clear();
  super::preDestroy();
}

void NLDB::preDestroy() {
  commonPreDrestroy();
  NLUniverse::get()->removeDB(this);
}

void NLDB::destroyFromUniverse() {
  commonPreDrestroy();
  delete this;
}

void NLDB::addLibraryAndSetID(NLLibrary* library) {
  if (libraries_.empty()) {
    library->id_ = 0;
  } else {
    auto it = libraries_.rbegin();
    NLLibrary* lastLibrary = &(*it);
    library->id_ = lastLibrary->id_;
    ++library->id_;
  }
  addLibrary(library);
}

void NLDB::addLibrary(NLLibrary* library) {
  libraries_.insert(*library);
  libraryNameIDMap_[library->getName()] = library->id_;
}

void NLDB::removeLibrary(NLLibrary* library) {
  libraries_.erase(*library);
  libraryNameIDMap_.erase(library->getName());
}

OWNER_RENAME(NLDB, NLLibrary, libraryNameIDMap_) 

NLLibrary* NLDB::getLibrary(NLID::LibraryID id) const {
  auto it = libraries_.find(NLID(getID(), id), NLIDComp<NLLibrary>());
  if (it != libraries_.end()) {
    return const_cast<NLLibrary*>(&*it);
  }
  return nullptr;
}

NLLibrary* NLDB::getLibrary(const NLName& name) const {
  auto it = libraryNameIDMap_.find(name);
  if (it != libraryNameIDMap_.end()) {
    NLID::LibraryID id = it->second;
    return getLibrary(id);
  }
  return nullptr;
}

SNLDesign* NLDB::getSNLDesign(const NLID::DBDesignReference& designReference) const {
  auto library = getLibrary(designReference.libraryID_);
  if (library) {
    return library->getSNLDesign(designReference.designID_);
  }
  return nullptr;
}

SNLDesign* NLDB::getSNLDesign(const NLName& name) const {
  for (auto library: getLibraries()) {
    auto design = library->getSNLDesign(name);
    if (design) {
      return design;
    }
  }
  return nullptr;
}

NajaCollection<NLLibrary*> NLDB::getGlobalLibraries() const {
  return NajaCollection(new NajaIntrusiveSetCollection(&libraries_));
}

NajaCollection<NLLibrary*> NLDB::getPrimitiveLibraries() const {
  auto filter = [](const NLLibrary* l) { return l->isPrimitives(); };
  return getGlobalLibraries().getSubCollection(filter);
}

NajaCollection<NLLibrary*> NLDB::getLibraries() const {
  auto filter = [](const NLLibrary* l) { return l->isRoot(); };
  return getGlobalLibraries().getSubCollection(filter);
}

NajaCollection<NLClock*> NLDB::getClocks() const {
  //Two flattening levels: every library's designs, then every design's clocks.
  //getGlobalLibraries() is already the flat registry of every library in this
  //NLDB (root, nested, and primitive), so no library-nesting flatten is needed.
  auto designClocksFlattener = [](SNLDesign* design) { return design->getClocks(); };
  auto libraryClocksFlattener = [designClocksFlattener](NLLibrary* library) {
    return library->getSNLDesigns().getFlatCollection<SNLDesign*, NLClock*, NLClock*>(designClocksFlattener);
  }; // LCOV_EXCL_LINE Compiler-generated exception cleanup during collection construction.
  return getGlobalLibraries().getFlatCollection<NLLibrary*, NLClock*, NLClock*>(libraryClocksFlattener);
}

NajaCollection<NLReset*> NLDB::getResets() const {
  //Two flattening levels: every library's designs, then every design's resets.
  //getGlobalLibraries() is already the flat registry of every library in this
  //NLDB (root, nested, and primitive), so no library-nesting flatten is needed.
  auto designResetsFlattener = [](SNLDesign* design) { return design->getResets(); };
  auto libraryResetsFlattener = [designResetsFlattener](NLLibrary* library) {
    return library->getSNLDesigns().getFlatCollection<SNLDesign*, NLReset*, NLReset*>(designResetsFlattener);
  }; // LCOV_EXCL_LINE Compiler-generated exception cleanup during collection construction.
  return getGlobalLibraries().getFlatCollection<NLLibrary*, NLReset*, NLReset*>(libraryResetsFlattener);
}

NLID NLDB::getNLID() const {
  return NLID(id_);
}

void NLDB::setID(NLID::DBID id) {
  if (NLUniverse::get()->isDB0(this)) {
    //FIXME
    //error
  }
  NLUniverse::get()->removeDB(this);
  id_ = id;
  NLUniverse::get()->addDB(this);
}

bool NLDB::isTopDB() const {
  return NLUniverse::get()->getTopDB() == this;
}

SNLDesign* NLDB::getTopDesign() const {
  return topDesign_;
}

void NLDB::setTopDesign(SNLDesign* design) {
  if (design and design->getDB() not_eq this) {
    std::ostringstream reason;
    reason << "Impossible setTopDesign call with ";
    reason << design->getString() << " and ";
    reason << getString();
    throw NLException(reason.str());
  }
  topDesign_ = design;
}

bool NLDB::deepCompare(const NLDB* other, std::string& reason) const {
  //don't compare NLDB ID
  DEEP_COMPARE_MEMBER(Libraries, this, other)
  return true;
}

void NLDB::mergeAssigns() {
  for (auto library: getLibraries()) {
    if (not library->isPrimitives()) {
      library->mergeAssigns();
    }
  }
}

//LCOV_EXCL_START
const char* NLDB::getTypeName() const {
  return "NLDB";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string NLDB::getString() const {
  return "<" + std::string(getTypeName()) + " " + std::to_string(getID()) + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string NLDB::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + std::to_string(getID()) + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void NLDB::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    for (auto lib: getLibraries()) {
      lib->debugDump(indent+2, recursive, stream);
    }
  }
}
//LCOV_EXCL_STOP

}  // namespace naja::NL
