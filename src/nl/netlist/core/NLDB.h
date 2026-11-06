// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLContentHash.h"
#include "NLObject.h"
#include "NLLibrary.h"
#include "NLClock.h"
#include "NLReset.h"

namespace naja::NL {

class NLUniverse;

class NLDB final: public NLObject {
  public:
    /// Deterministic versioned SHA-256 of this object and its descendants.
    ContentHash getContentHash() const;

    friend class NLUniverse;
    friend class NLLibrary;
    using super = NLObject;

    using NLDBLibrariesHook =
      boost::intrusive::member_hook<NLLibrary, boost::intrusive::set_member_hook<>, &NLLibrary::dbLibrariesHook_>;
    using NLDBLibraries = boost::intrusive::set<NLLibrary, NLDBLibrariesHook>;

    NLDB(const NLDB&) = delete;

    /**
     * \brief Create a NLDB in the NLUniverse universe.
     * \param universe the NLUniverse in which the NLDB will be created.
     * \return the created NLDB.
     */
    static NLDB* create(NLUniverse* universe);
    /**
     * \brief Create a NLDB in the NLUniverse universe with a specific NLID::DBID.
     * \param universe the NLUniverse in which the NLDB will be created.
     * \param id the NLID::DBID of the NLDB to create.
     * \return the created NLDB.
     */
    static NLDB* create(NLUniverse* universe, NLID::DBID id);

    /// \return the NLID::DBID of this NLDB.
    NLID::DBID getID() const { return id_; }
    /// \return the NLID of this NLDB. 
    NLID getNLID() const;

    /**
     * \brief Change the NLDB id. Main purpose: compare DBs after save and load.
     * \param id new DBID
     * \warning use with caution: all DB objects NLIDs will be modified, as the DB id part of NLID will
     * be modified.
     */
    void setID(NLID::DBID id);

    /// \return the unique NLLibrary in this NLDB with NLID::LibraryID id 
    NLLibrary* getLibrary(NLID::LibraryID id) const;
    /// \return the unique NLLibrary in this NLDB with NLName:name 
    NLLibrary* getLibrary(const NLName& name) const;

    /// \return the SNLDesign with NLID::DBDesignReference reference or null if it does not exist
    SNLDesign* getSNLDesign(const NLID::DBDesignReference& designReference) const;

    /// \return the SNLDesign named name or null if it does not exist
    SNLDesign* getSNLDesign(const NLName& name) const;

    bool isTopDB() const;
    SNLDesign* getTopDesign() const;
    void setTopDesign(SNLDesign* design);

    /// \return the Libraries owned by this NLDB.
    NajaCollection<NLLibrary*> getLibraries() const;

    /// \return every NLClock defined in the designs of this NLDB, across all its libraries.
    NajaCollection<NLClock*> getClocks() const;
    NajaCollection<NLReset*> getResets() const;
    /// \return the all the Libraries owned (directly or indirectly) by this NLDB.
    NajaCollection<NLLibrary*> getGlobalLibraries() const;
    /// \return the Primitive Libraries owned (directly or indirectly) by this NLDB.
    NajaCollection<NLLibrary*> getPrimitiveLibraries() const;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    void mergeAssigns();

    bool operator<(const NLDB &rdb) const {
      return getNLID() < rdb.getNLID();
    }

    bool deepCompare(const NLDB* db, std::string& reason) const;

  private:
    NLDB() = default;
    NLDB(NLID::DBID id);
    static void preCreate(NLUniverse* universe);
    static void preCreate(NLUniverse* universe, NLID::DBID id);
    void postCreateAndSetID(NLUniverse* universe);
    void postCreateWithUniverse(NLUniverse* universe);
    void commonPreDrestroy();
    void preDestroy() override;
    void destroyFromUniverse();

    void addLibrary(NLLibrary* library);
    void addLibraryAndSetID(NLLibrary* library);
    void rename(NLLibrary* library, const NLName& previousName);
    void removeLibrary(NLLibrary* library);

    NLID::DBID                          id_;
    boost::intrusive::set_member_hook<> universeDBsHook_          {};
    NLDBLibraries                       libraries_                {};
    using LibraryNameIDMap = std::map<NLName, NLID::LibraryID>;
    LibraryNameIDMap                    libraryNameIDMap_         {};
    SNLDesign*                          topDesign_                {nullptr};
};

}  // namespace naja::NL