// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLPath.h"

#include <sstream>

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLSharedPath.h"

namespace naja::NL {

SNLPath::SNLPath(SNLSharedPath* sharedPath) : sharedPath_(sharedPath) {}

SNLSharedPath* SNLPath::createInstanceSharedPath(SNLInstance* instance) {
  auto sharedPath = instance->getSharedPath(nullptr);
  if (not sharedPath) {
    sharedPath = new SNLSharedPath(instance);
  }
  return sharedPath;
}

SNLPath::SNLPath(SNLInstance* instance) : SNLPath() {
  if (instance) {
    sharedPath_ = createInstanceSharedPath(instance);
  }
}

SNLPath::SNLPath(SNLInstance* headInstance, const SNLPath& tailPath)
    : SNLPath() {
  if (not headInstance) {
    throw NLException("cannot create SNLPath with null head instance");
  }

  if (not tailPath.sharedPath_) {
    sharedPath_ = createInstanceSharedPath(headInstance);
  } else {
    SNLInstance* tailInstance = tailPath.getTailInstance();
    SNLSharedPath* headSharedPath =
        SNLPath(headInstance, tailPath.getHeadPath()).sharedPath_;
    sharedPath_ = tailInstance->getSharedPath(headSharedPath);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(tailInstance, headSharedPath);
    }
  }
}

SNLPath::SNLPath(const SNLPath& headPath, SNLInstance* tailInstance)
    : SNLPath() {
  if (not tailInstance) {
    throw NLException("Cannot create SNLPath with null tailInstance");
  }

  if (not headPath.sharedPath_) {
    sharedPath_ = createInstanceSharedPath(tailInstance);
  } else {
    SNLSharedPath* headSharedPath = headPath.sharedPath_;
    if (headSharedPath->getModel() not_eq tailInstance->getDesign()) {
      std::ostringstream error;
      error << "Cannot create SNLPath with incompatible headPath: ";
      error << headSharedPath->getString() << " with model ";
      error << headSharedPath->getModel()->getString();
      error << " and tail instance: " << tailInstance->getDescription();
      error << " with design: " << tailInstance->getDesign()->getString();
      throw NLException(error.str());
    }

    sharedPath_ = tailInstance->getSharedPath(headSharedPath);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(tailInstance, headSharedPath);
    }
  }
}

SNLPath::SNLPath(const SNLDesign* top, const PathStringDescriptor& descriptor)
    : SNLPath() {
  if (top and not descriptor.empty()) {
    using Instances = std::vector<SNLInstance*>;
    Instances instances;
    auto design = top;
    for (auto instanceName : descriptor) {
      if (instanceName.empty()) {
        throw NLException("Anonymous instance in SNLPath constructor.");
      }
      auto instance = design->getInstance(NLName(instanceName));
      if (not instance) {
        throw NLException("Unfound instance in SNLPath constructor.");
      }
      instances.push_back(instance);
      design = instance->getModel();
    }
    SNLPath path;
    for (auto instance : instances) {
      path = SNLPath(path, instance);
    }
    sharedPath_ = path.sharedPath_;
  }
}

SNLPath ::SNLPath(const SNLDesign* top, const PathIDDescriptor& descriptor)
    : SNLPath() {
  if (top and not descriptor.empty()) {
    using Instances = std::vector<SNLInstance*>;
    Instances instances;
    auto design = top;
    for (auto instanceID : descriptor) {
      auto instance = design->getInstance(instanceID);
      if (not instance) {
        throw NLException("Unfound instance in SNLPath constructor.");
      }
      instances.push_back(instance);
      design = instance->getModel();
    }
    SNLPath path;
    for (auto instance : instances) {
      path = SNLPath(path, instance);
    }
    sharedPath_ = path.sharedPath_;
  }
}

SNLPath::PathIDDescriptor SNLPath::getIDDescriptor() const {
  SNLPath::PathIDDescriptor descriptor;
  if (sharedPath_) {
    auto sharedPath = sharedPath_;
    while (sharedPath) {
      descriptor.push_back(sharedPath->getHeadInstance()->getID());
      sharedPath = sharedPath->getTailSharedPath();
    }
  }
  return descriptor;
  // LCOV_EXCL_START
}
// LCOV_EXCL_STOP

SNLInstance* SNLPath::getHeadInstance() const {
  return sharedPath_ ? sharedPath_->getHeadInstance() : nullptr;
}

SNLPath SNLPath::getTailPath() const {
  return SNLPath(sharedPath_ ? sharedPath_->getTailSharedPath() : nullptr);
}

SNLPath SNLPath::getHeadPath() const {
  return SNLPath(sharedPath_ ? sharedPath_->getHeadSharedPath() : nullptr);
}

SNLInstance* SNLPath::getTailInstance() const {
  return sharedPath_ ? sharedPath_->getTailInstance() : nullptr;
}

bool SNLPath::empty() const {
  return not sharedPath_;
}

size_t SNLPath::size() const {
  if (sharedPath_) {
    return sharedPath_->size();
  }
  return 0;
}

SNLDesign* SNLPath::getDesign() const {
  return sharedPath_ ? sharedPath_->getDesign() : nullptr;
}

SNLDesign* SNLPath::getModel() const {
  return sharedPath_ ? sharedPath_->getModel() : nullptr;
}

bool SNLPath::operator==(const SNLPath& path) const {
  return sharedPath_ == path.sharedPath_;
}

bool SNLPath::operator!=(const SNLPath& path) const {
  return sharedPath_ != path.sharedPath_;
}

bool SNLPath::operator<(const SNLPath& path) const {
  if (sharedPath_) {
    if (path.sharedPath_) {
      // both non null
      // start by comparing sizes
      auto thisSize = size();
      auto otherSize = path.size();
      if (thisSize not_eq otherSize) {
        return thisSize < otherSize;
      } else {
        // same size... compare instances one by one
        auto thisSharedPath = sharedPath_;
        auto otherSharedPath = path.sharedPath_;
        while (thisSharedPath) {
          auto thisTailInstance = thisSharedPath->getTailInstance();
          auto otherTailInstance = otherSharedPath->getTailInstance();
          if (thisTailInstance->getNLID() < otherTailInstance->getNLID()) {
            return true;
          }
          if (otherTailInstance->getNLID() < thisTailInstance->getNLID()) {
            return false;
          }
          thisSharedPath = thisSharedPath->getHeadSharedPath();
          otherSharedPath = otherSharedPath->getHeadSharedPath();
        }
        return false;
      }
    } else {
      return false;
    }
  } else {
    // this is empty path
    // if other path is non null => true
    // if other path is null => false
    return path.sharedPath_ != nullptr;
  }
  return false;
}

bool SNLPath::operator<=(const SNLPath& path) const {
  return *this < path or *this == path;
}

bool SNLPath::operator>(const SNLPath& path) const {
  return not (*this <= path);
}

bool SNLPath::operator>=(const SNLPath& path) const {
  return not (*this < path);
}

// LCOV_EXCL_START
std::string SNLPath::getString(const char separator) const {
  if (sharedPath_) {
    return sharedPath_->getString(separator);
  }
  return std::string();
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string SNLPath::getDescription(const char separator) const {
  if (sharedPath_) {
    return "<" + sharedPath_->getString(separator) + ">";
  }
  return "<>";
}
// LCOV_EXCL_STOP

std::vector<NLID::DesignObjectID> SNLPath::getInstanceIDs() const {
  if (not sharedPath_) {
    return {};
  }
  return sharedPath_->getInstanceIDs();
}

std::vector<SNLInstance*> SNLPath::getInstances() const {
  std::vector<SNLInstance*> instances;
  if (not sharedPath_) {
    return instances;
  }
  return sharedPath_->getInstances();
}

SNLPath::PathStringDescriptor SNLPath::getPathDescriptor() const {
  PathStringDescriptor descriptor;
  if (sharedPath_) {
    auto sharedPath = sharedPath_;
    while (sharedPath) {
      descriptor.push_back(
          sharedPath->getHeadInstance()->getName().getString());
      sharedPath = sharedPath->getTailSharedPath();
    }
  }
  return descriptor;
}

std::vector<NLName> SNLPath::getPathNames() const {
  std::vector<NLName> names;
  if (sharedPath_) {
    auto sharedPath = sharedPath_;
    while (sharedPath) {
      if (!sharedPath->getHeadInstance()->getName().empty()) {
        names.push_back(sharedPath->getHeadInstance()->getName());
      } else {
        names.push_back(NLName(std::to_string(sharedPath->getHeadInstance()->getID()))); 
      }
      sharedPath = sharedPath->getTailSharedPath();
    }
  }
  return names; 
}

}  // namespace NL // namespace naja
