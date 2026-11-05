// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <boost/intrusive/set.hpp>
#include <vector>

#include "NLID.h"

namespace naja::NL {

class SNLInstance;
class SNLDesign;

/**
 * SNLSharedPath is a class non visible from users. Visible API is SNLPath.
 * SNLSharedPath class allows:
 * - caching SNLInstance path through a tree like structure, sharing common headPath
 * - storing Occurrence properties (in the future)
 * SNLSharedPath is a data structure composed of [SNLSharedPath, SNLInstance*] or [HeadSharedPath, tailInstance].
 * 
 * SNLSharedPath holds a key used for:
 * - comparing two Paths. 
 * - Inserting a new SNLSharedPath in an instance
 */
class SNLSharedPath {
  public:
    friend class SNLInstance;
    friend class SNLPath;
    SNLSharedPath() = delete;
    SNLSharedPath(const SNLSharedPath&) = delete;

    SNLSharedPath* getHeadSharedPath() const { return headSharedPath_; }
    SNLInstance* getTailInstance() const { return tailInstance_; }
    SNLInstance* getHeadInstance() const;
    SNLSharedPath* getTailSharedPath() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;
    //returns this SNLSharedPath key
    size_t size() const { return size_; }


    std::string getString(char separator='/');

    std::vector<NLID::DesignObjectID> getInstanceIDs() const;
    std::vector<SNLInstance*> getInstances() const;

  private:
    SNLSharedPath(SNLInstance* tailInstance, SNLSharedPath* headSharedPath=nullptr);
    ~SNLSharedPath() = default;

    void commonDestroy();
    void destroy();
    void destroyFromInstance();

    boost::intrusive::set_member_hook<> instanceSharedPathsHook_  {};
    SNLSharedPath* const                headSharedPath_;
    SNLInstance* const                  tailInstance_;
    // Shared paths are immutable; cache depth for constant-time comparisons.
    const size_t                        size_;
};

}  // namespace naja::NL
