// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>
#include "NajaCollection.h"

namespace naja {

class NajaProperty;
class NajaDumpableProperty;

class NajaObject {
  public:
    using Properties = std::map<std::string, NajaProperty*>;

    ///\return a string describing the object type
    virtual const char* getTypeName() const = 0;
    /**
     * \brief Return a short, human-readable string describing the object.
     *
     * This string is intended for display, logs, diagnostics, debug dumps,
     * scripting representations, and GUI labels. It usually matches the
     * object's name when one exists, but implementations may return a generated
     * display string for unnamed objects.
     *
     * Do not use this method for algorithmic purposes, such as map or set keys,
     * identity checks, ordering, persistence, or serialization. Use stable object
     * identifiers like IDs.
     *
     * \return a display string describing the object.
     */
    virtual std::string getString() const = 0;
    ///\return a string extensively describing the object. Useful for debug.
    virtual std::string getDescription() const = 0;

    ///\return NajaProperty with std::string name or nullptr if it does not exist
    NajaProperty* getProperty(const std::string& name) const;
    ///\return true if this NajaObject has a NajaProperty named name, false otherwise
    bool hasProperty(const std::string& name) const {
      return getProperty(name) != nullptr;
    }
    ///\return the collection of NajaProperties of this NajaObject
    NajaCollection<NajaProperty*> getProperties() const;
    bool hasProperties() const { return not properties_.empty(); }
    ///\return the collection of dumpable NajaProperties of this NajaObject
    NajaCollection<NajaDumpableProperty*> getDumpableProperties() const;

    ///destroy this object
    virtual void destroy();
    void put(NajaProperty* property);
    void remove(NajaProperty* property);
    void onDestroyed (NajaProperty* property);
    
  protected:
    NajaObject() = default;
    virtual ~NajaObject() = default;

    static void preCreate() {}
    virtual void postCreate() {}
    virtual void preDestroy();

  private:
    friend class NajaPrivateProperty;
    friend class NajaSharedProperty;
    friend class NajaRelationProperty;

    void addProperty(NajaProperty* property);
    void removeProperty(NajaProperty* property);

    Properties  properties_;
};

} // namespace naja
