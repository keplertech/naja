// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLContentHash.h"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "NLDB.h"

namespace naja::NL {

class SNLBusTermBit;
class PNLTechnology;

/**
 * \brief NLUniverse is a singleton class, root holder of a NL data structure.
 * NLUniverse can be created only once and is accessible through NLUniverse::get().
 * Multiple NLDB can be added to a NLUniverse.
 */
class NLUniverse final: public NLObject {
  public:
    /// Deterministic versioned SHA-256 of this object and its descendants.
    ContentHash getContentHash() const;

    friend class NLDB;
    friend class NLDB0;
    friend class NLName;
    friend class PNLTechnology;
    using super = NLObject;
    NLUniverse(const NLUniverse&) = delete;

    /// \return a created singleton NLUniverse or an error if it exists already
    static NLUniverse* create();
    /// \return the singleron NLUniverse or null if it does not exist.
    static NLUniverse* get();

    /// \return the collection of all NLDB
    NajaCollection<NLDB*> getDBs() const;

    /// \return the collection of user NLDB
    NajaCollection<NLDB*> getUserDBs() const;

    /// \return the NLDB with NLID::DBID id or null if it does not exist
    NLDB* getDB(NLID::DBID id) const;

    /// \return the NLLibrary with NLID::DBID dbID and NLID::LibraryID or null if it does not exist
    NLLibrary* getLibrary(NLID::DBID dbID, NLID::LibraryID libraryID) const;

    /// \return the SNLDesign with NLID::DesignReference reference or null if it does not exist
    SNLDesign* getSNLDesign(const NLID::DesignReference& reference) const;

    /// \return the SNLDesign named name or null if it does not exist
    /// DBs are browsed in their index ordering
    SNLDesign* getSNLDesign(const NLName& name) const;

    /// \return the SNLTerm with NLID::DesignObjectReference reference or null if it does not exist
    SNLTerm* getTerm(const NLID::DesignObjectReference& reference) const;

    /// \return the SNLNet with NLID::DesignObjectReference reference or null if it does not exist
    SNLNet* getNet(const NLID::DesignObjectReference& reference) const;

    /// \return the SNLBitNet with NLID::BitNetReference reference or null if it does not exist
    SNLBitNet* getBitNet(const NLID::BitNetReference& reference) const;

    /// \return the SNLInstance with NLID::DesignObjectReference reference or null if it does not exist
    SNLInstance* getInstance(const NLID::DesignObjectReference& reference) const;

    /// \return the SNLInstTerm with NLID id or null if it does not exist
    SNLInstTerm* getInstTerm(const NLID& id) const;

    /// \return the SNLBusTermBit with NLID id or null if it does not exist
    SNLBusTermBit* getBusTermBit(const NLID& id) const;

    /// \return the SNLBusNetBit with NLID id or null if it does not exist
    SNLBusNetBit* getBusNetBit(const NLID& id) const;

    /// \return the NLObject with NLID id or null if it does not exist
    NLObject* getObject(const NLID& id);
    
    /// \return the top NLDB or null if it has not been set.
    /// \sa setTopDB()
    static NLDB* getTopDB();

    /// \return the top SNLDesign or null if it has not been set.
    /// \sa setTopDesign()
    static SNLDesign* getTopDesign();

    /// set the top NLDB.
    void setTopDB(NLDB* db);

    /// set the top SNLDesign.
    void setTopDesign(SNLDesign* design);

    /// \return true if db is the top NLDB.
    static bool isDB0(const NLDB* db);

    /// \return the NLDB0. 
    static NLDB* getDB0();
    
    /// \brief merge all assigns and corresponding SNLBitNets in all NLDBs.
    void mergeAssigns();

    /// \return the PNLTechnology owned by this universe or null if not created.
    PNLTechnology* getTechnology() const;
    
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    NLUniverse() = default;
    static void preCreate();
    void postCreate() override;
    void preDestroy() override;

    void addDBAndSetID(NLDB* db);
    void addDB(NLDB* db);
    void removeDB(NLDB* db);

    /// \return the unique ID for name, creating it if needed.
    NLName::ID getOrCreateNameID(const std::string& name);
    /// \return the shared string for a name ID.
    const std::string& getNameString(NLName::ID id) const;
    
    using NLUniverseDBsHook =
      boost::intrusive::member_hook<NLDB, boost::intrusive::set_member_hook<>, &NLDB::universeDBsHook_>;
    using NLUniverseDBs = boost::intrusive::set<NLDB, NLUniverseDBsHook>;

    struct NameTable {
      using NameStorage = std::vector<std::unique_ptr<std::string>>;
      using NameMap = std::unordered_map<std::string_view, NLName::ID,
        std::hash<std::string_view>, std::equal_to<>>;
      NameStorage names     {};
      NameMap     nameToId  {};
    };

    static NLUniverse*  universe_;
    NameTable           nameTable_    {};
    NLUniverseDBs       dbs_          {};
    NLDB*               db0_          {nullptr};
    NLDB*               topDB_        {nullptr};
    PNLTechnology*      technology_   {nullptr};
};

}  // namespace naja::NL