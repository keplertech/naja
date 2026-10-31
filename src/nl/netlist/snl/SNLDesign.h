// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <map>
#include <vector>

#include "NajaCollection.h"
#include "NLDesign.h"
#include "SNLTerm.h"
#include "SNLNet.h"
#include "SNLInstance.h"
#include "SNLParameter.h"
#include "NLClock.h"

namespace naja::NL {

class NLLibrary;
class SNLScalarNet;
class SNLBusNet;
class SNLBusNetBit;
class SNLScalarTerm;
class SNLBusTerm;
class SNLBusTermBit;
class SNLBundleTerm;
class SNLRTLInfos;

/**
 * @class SNLDesign
 * @brief A SNLDesign has a dual role. It serves as a model for instances (SNLInstance::getModel) 
 * and contains instances (SNLInstance::getDesign). 
 * SNLDesign manages the terms, instances, nets, and parameters associated with an SNLDesign.
 */
class SNLDesign final: public NLObject {
  public:
    friend class NLLibrary;
    friend class SNLScalarTerm;
    friend class SNLBusTerm;
    friend class SNLBundleTerm;
    friend class SNLInstance;
    friend class SNLScalarNet;
    friend class SNLBusNet;
    friend class SNLParameter;
    friend class SNLRTLInfos;
    friend class NLClock;
    using super = NLObject;

    class Type {
      public:
        enum TypeEnum {
          Standard, UserBlackBox, AutoBlackBox, Primitive
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        bool isAutoBlackBox() const {
          return typeEnum_ == AutoBlackBox;
        }
        bool isUserBlackBox() const {
          return typeEnum_ == UserBlackBox;
        }
        std::string getString() const;
        private:
          TypeEnum typeEnum_;
    };

    SNLDesign() = delete;
    SNLDesign(const SNLDesign& design) = delete;

    /**
     * \brief Create a SNLDesign.
     * \param library owning NLLibrary.
     * \param name NLName of the SNLDesign. If empty, the SNLDesign is anonymous.
     * \return the created SNLDesign.
     */
    static SNLDesign* create(NLLibrary* library, const NLName& name=NLName());

    /**
     * \brief Create a SNLDesign with a specific Type.
     * \param library owning NLLibrary.
     * \param type the Type of the SNLDesign to create.
     * \param name NLName of the SNLDesign. If empty, the SNLDesign is anonymous.
     * \return the created SNLDesign.
     */
    static SNLDesign* create(NLLibrary* library, Type type, const NLName& name=NLName());
    
    /**
     * \brief Create a SNLDesign with a specific NLID::DesignID and a specific Type.
     * \param library owning NLLibrary.
     * \param id the NLID::DesignID of the SNLDesign to create.
     * \param type the Type of the SNLDesign to create.
     * \param name NLName of the SNLDesign. If empty, the SNLDesign is anonymous.
     * \return the created SNLDesign.
     */
    static SNLDesign* create(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name=NLName());

    struct PointerLess {
      bool operator()(const SNLDesign* ld, const SNLDesign* rd) const {
        return *ld < *rd;
      }
    };

    ///\return owning NLDB
    NLDB* getDB() const;
    /// \return owning NLLibrary.
    NLLibrary* getLibrary() const { return library_; }

    /// \return SNLTerm with NLID::DesignObjectID id or nullptr if it does not exist.
    SNLTerm* getTerm(NLID::DesignObjectID id) const;
    SNLTerm* getTermByID(NLID::DesignObjectID id) const { return getTerm(id); }
    /// \return SNLTerm with NLName name or nullptr if it does not exist
    SNLTerm* getTerm(const NLName& name) const;
    /// \return SNLScalarTerm with NLID::DesignObjectID id or nullptr if it does not exist
    SNLScalarTerm* getScalarTerm(NLID::DesignObjectID id) const;
    /// \return SNLBusTermBit with NLID::DesignObjectID id and NLID::Bit bit or nullptr if it does not exist.
    SNLBusTermBit* getBusTermBit(NLID::DesignObjectID id, NLID::Bit bit) const;
    /// \return SNLScalarTerm with NLName termName or nullptr if it does not exist
    SNLScalarTerm* getScalarTerm(const NLName& termName) const;
    /**
     * \param id NLID::DesignObjectID of the SNLBitTerm.
     * \param bit NLID::Bit of the SNLBitTerm. Relevant only if the SNLBitTerm is a SNLBusTermBit.
     * \return SNLBitTerm with NLID::DesignObjectID id and NLID::Bit bit or nullptr if it does not exist.
     */
    SNLBitTerm* getBitTerm(NLID::DesignObjectID id, NLID::Bit bit) const;
    /// \return SNLBusTerm with NLID::DesignObjectID id or nullptr if it does not exist
    SNLBusTerm* getBusTerm(NLID::DesignObjectID id) const;
    /// \return SNLBusTerm with NLName termName or nullptr if it does not exist
    SNLBusTerm* getBusTerm(const NLName& termName) const;
    /// \return SNLBundleTerm with NLID::DesignObjectID id or nullptr if it does not exist
    SNLBundleTerm* getBundleTerm(NLID::DesignObjectID id) const;
    /// \return SNLBundleTerm with NLName termName or nullptr if it does not exist
    SNLBundleTerm* getBundleTerm(const NLName& termName) const;
    /// \return the collection of SNLTerm of this SNLDesign
    NajaCollection<SNLTerm*> getTerms() const;

    /**
     * \return the collection of SNLBusTerm of this SNLDesign (SNLBusTerm subset of getTerms())
     * \sa getTerms()
     * \sa getScalarTerms()
     * \sa getBundleTerms()
     */
    NajaCollection<SNLBusTerm*> getBusTerms() const;

    /**
     * \return the collection of SNLScalarTerm of this SNLDesign (SNLScalarTerm subset of getTerms()).
     * \sa getTerms()
     * \sa getBusTerms()
     * \sa getBundleTerms()
     */
    NajaCollection<SNLScalarTerm*> getScalarTerms() const;

    /**
     * \return the collection of SNLBundleTerm of this SNLDesign (SNLBundleTerm subset of getTerms()).
     * \sa getTerms()
     * \sa getScalarTerms()
     * \sa getBusTerms()
     */
    NajaCollection<SNLBundleTerm*> getBundleTerms() const;

    /// \return the collection of SNLBitTerm of this SNLDesign (SNLScalarTerm and flattened SNLBusTerm to SNLBusTermBit).
    NajaCollection<SNLBitTerm*> getBitTerms() const;
    /// \return SNLInstance with NLID::DesignObjectID id or nullptr if it does not exist.
    SNLInstance* getInstance(NLID::DesignObjectID id) const;
    SNLInstance* getInstanceByID(NLID::DesignObjectID id) const { return getInstance(id); }
    /// \return SNLInstance with NLName name if it does not exist.
    SNLInstance* getInstance(const NLName& instanceName) const;
    /// \return the collection of SNLInstance instantiated IN this SNLDesign (instance/parent relationship).
    NajaCollection<SNLInstance*> getInstances() const;

    /// \return getInstances() excluding only instances whose model is assign glue.
    NajaCollection<SNLInstance*> getNonAssignInstances() const;

    /// \return the collection of instances whose model is assign glue.
    NajaCollection<SNLInstance*> getAssignInstances() const;

    /**
     * \return the collection of SNLInstance instantiated BY this SNLDesign (instance/model relationship).
     * \remark SNLInstance/SNLDesign model relationship is not constructed for Primitives.
     * \sa isPrimitive
     */
    NajaCollection<SNLInstance*> getSlaveInstances() const;

    /// \return the NLClock defined in this SNLDesign.
    NajaCollection<NLClock*> getClocks() const;

    /**
     * \return the collection of SNLInstance instantiated IN this SNLDesign (instance/parent relationship)
     * and instanciating a Primitive model. 
     * \sa isPrimitive getInstances getNonPrimitiveInstances
     */
    NajaCollection<SNLInstance*> getPrimitiveInstances() const;

    /**
     * \return the collection of SNLInstance instantiated IN this SNLDesign (instance/parent relationship)
     * and instanciating a non Primitive model.
     * \sa isPrimitive getInstances getPrimitiveInstances
     */
    NajaCollection<SNLInstance*> getNonPrimitiveInstances() const;

    /// \return SNLNet with NLID::DesignObjectID id or nullptr if it does not exist.
    SNLNet* getNet(NLID::DesignObjectID id) const;
    SNLNet* getNetByID(NLID::DesignObjectID id) const { return getNet(id); }
    /// \return SNLNet with NLName name or nullptr if it does not exist.
    SNLNet* getNet(const NLName& netName) const;
    /// \return SNLScalarNet with NLID::DesignObjectID id or nullptr if it does not exist.
    SNLScalarNet* getScalarNet(NLID::DesignObjectID id) const;
    /// \return SNLBusNetBit with NLID::DesignObjectID id and NLID::Bit bit or nullptr if it does not exist.
    SNLBusNetBit* getBusNetBit(NLID::DesignObjectID id, NLID::Bit bit) const;
    /// \return SNLScalarNet with NLName name or nullptr if it does not exist.
    SNLScalarNet* getScalarNet(const NLName& netName) const;
    /**
     * \param id NLID::DesignObjectID of the SNLBitNet.
     * \param bit NLID::Bit of the SNLBitNet. Relevant only if the SNLBitNet is a SNLBusNetBit.
     * \return SNLBitNet with NLID::DesignObjectID id and NLID::Bit bit or nullptr if it does not exist.
     */
    SNLBitNet* getBitNet(NLID::DesignObjectID id, NLID::Bit bit) const;
    /// \return SNLBusNet with SNLIS::DesignObjectID id or nullptr if it does not exist.
    SNLBusNet* getBusNet(NLID::DesignObjectID id) const;
    /// \return SNLBusNet with NLName name or nullptr if it does not exist.
    SNLBusNet* getBusNet(const NLName& netName) const;
    /// \return the collection of SNLNet of this SNLDesign.
    NajaCollection<SNLNet*> getNets() const;

    /**
     * \return the collection of SNLBusNet of this SNLDesign (SNLBusNet subset of getNets()).
     * \see getNets()
     * \see getScalarNets()
     */
    NajaCollection<SNLBusNet*> getBusNets() const;
    
    /**
     * \return the collection of SNLScalarNet of this SNLDesign (SNLScalarNet subset of getNets()).
     * \see getNets()
     * \see getBusNets()
     */
    NajaCollection<SNLScalarNet*> getScalarNets() const;
    
    /// \return the collection of SNLBitNet of this SNLDesign (SNLScalarNet and flattened SNLBusNet to SNLBusNetBit).
    NajaCollection<SNLBitNet*> getBitNets() const;

    /// \return SNLParameter with NLName name or nullptr if it does not exist.
    SNLParameter* getParameter(const NLName& name) const;
    /// \return the collection of SNLParameter of this SNLDesign.
    NajaCollection<SNLParameter*> getParameters() const;

    NajaCollection<SNLAttribute> getAttributes() const;
    SNLRTLInfos* getRTLInfos() const { return rtlInfos_; }
    bool hasRTLInfos() const { return rtlInfos_ != nullptr; }

    NLID::DesignID getID() const { return id_; }
    NLID getNLID() const;
    NLID::DesignReference getReference() const;
    
    /// \return NLName of this SNLDesign. 
    NLName getName() const { return name_; }
    /// \return true if this SNLDesign is unnamed.
    bool isUnnamed() const { return name_.empty(); }
    /**
     * \brief set the NLName of this SNLDesign
     * \warning this method will throw an exception if the name is already used in the design's library.
    */
    void setName(const NLName& name);
    
    /// \return this SNLDesign Type.
    Type getType() const { return type_; }
    ///Set this SNLDesign Type
    ///\warning setType cannot be called to set a design as a primitive.
    void setType(Type type);
    ///\return true if this SNLDesign is a standard design.
    bool isStandard() const { return type_ == Type::Standard; }
    ///\return true if this SNLDesign is a user blackbox.
    bool isUserBlackBox() const { return type_.isUserBlackBox(); }
    ///\return true if this SNLDesign is an auto blackbox.
    bool isAutoBlackBox() const { return type_.isAutoBlackBox(); }
    ///\return true if this SNLDesign is a blackbox.
    bool isBlackBox() const { return isUserBlackBox() or isAutoBlackBox(); }
    ///\return true if this SNLDesign is a primitive.
    bool isPrimitive() const { return type_ == Type::Primitive; }
    ///\return true if this SNLDesign is the Assign primitive (in verilog: assign net1 = net0).
    bool isAssign() const;
    ///\return true if this SNLDesign is a mux primitive.
    bool isMux() const;
    ///\return true if this SNLDesign is a hierarchy leaf (blackbox or primitive).
    bool isLeaf() const { return isBlackBox() or isPrimitive(); }
    ///\return true if this SNLDesign is a top design.
    bool isTopDesign() const;

    /**
     * \brief Cloning interface for SNLDesign.
     * \param name NLName of the the clone.
     * \return a new SNLDesign with the same interface as this SNLDesign.
     */
    SNLDesign* cloneInterface(const NLName& name=NLName()) const;
    /**
     * \brief Cloning interface for SNLDesign to a specific NLLibrary.
     * \param library NLLibrary where the clone will be created.
     * \param name NLName of the the clone.
     * \return a new SNLDesign with the same interface as this SNLDesign.
     */
    SNLDesign* cloneInterfaceToLibrary(NLLibrary* library, const NLName& name=NLName()) const;
    /**
     * \brief Cloning for SNLDesign.
     * \param name NLName of the the clone.
     * \return a new SNLDesign with the same interface and content as this SNLDesign.
     */
    SNLDesign* clone(const NLName& name=NLName()) const;
    /**
     * \brief Cloning for SNLDesign to a specific NLLibrary.
     * \param library NLLibrary where the clone will be created.
     * \param name NLName of the the clone.
     * \return a new SNLDesign with the same interface and content as this SNLDesign.
     */
    SNLDesign* cloneToLibrary(NLLibrary* library, const NLName& name=NLName()) const;

    bool deepCompare(
      const SNLDesign* other,
      std::string& reason,
      NLDesign::CompareType type=NLDesign::CompareType::Complete) const;
    void mergeAssigns();

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
    void incrementRevisionCount() { revisionCount_++; }
    int getRevisionCount() const { return revisionCount_; }
    void recursiveRevisionIncrement();

  private:
    SNLDesign(NLLibrary* library, Type type, const NLName& name);
    SNLDesign(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name);
    static void preCreate(const NLLibrary* library, Type type, const NLName& name);
    static void preCreate(const NLLibrary* library, NLID::DesignID id, Type type, const NLName& name);
    void destroyFromLibrary();
    void postCreateAndSetID();
    void postCreate() override;
    void commonPreDestroy();
    void preDestroy() override;
    void addTerm(SNLTerm* term);
    void addTermAndSetID(SNLTerm* term);
    void removeTerm(SNLTerm* term);
    void addInstance(SNLInstance* instance);
    void addInstanceAndSetID(SNLInstance* instance);
    void removeInstance(SNLInstance* instance);
    void addSlaveInstance(SNLInstance* instance);
    void removeSlaveInstance(SNLInstance* instance);
    void addNet(SNLNet* net);
    void addNetAndSetID(SNLNet* net);
    void removeNet(SNLNet* net);
    void addClock(NLClock* clock);
    void removeClock(NLClock* clock);
    void rename(SNLTerm* term, const NLName& previousName);
    void rename(SNLNet* net, const NLName& previousName);
    void rename(SNLInstance* instance, const NLName& previousName);
    void addParameter(SNLParameter* parameter);
    void removeParameter(SNLParameter* parameter);
    static bool isBetween(int n, int MSB, int LSB);
    void setOrderIDs();

    friend bool operator< (const SNLDesign& ld, const SNLDesign& rd) {
      return ld.getNLID() < rd.getNLID();
    }

    using SNLDesignTermsHook =
      boost::intrusive::member_hook<SNLTerm, boost::intrusive::set_member_hook<>, &SNLTerm::designTermsHook_>;
    using SNLDesignTerms = boost::intrusive::set<SNLTerm, SNLDesignTermsHook, boost::intrusive::compare<NLDesign::CompareByID<SNLTerm>>>;
    using SNLDesignObjectNameIDMap = std::map<NLName, NLID::DesignObjectID>;
    using SNLDesignInstancesHook =
      boost::intrusive::member_hook<SNLInstance, boost::intrusive::set_member_hook<>, &SNLInstance::designInstancesHook_>;
    using SNLDesignInstances = boost::intrusive::set<SNLInstance, SNLDesignInstancesHook, boost::intrusive::compare<NLDesign::CompareByID<SNLInstance>>>;
    using SNLDesignSlaveInstancesHook =
      boost::intrusive::member_hook<SNLInstance, boost::intrusive::set_member_hook<>, &SNLInstance::designSlaveInstancesHook_>;
    using SNLDesignSlaveInstances = boost::intrusive::set<SNLInstance, SNLDesignSlaveInstancesHook>;
    using SNLInstanceNameIDMap = std::map<NLName, NLID::DesignObjectID>;
    using SNLDesignNetsHook =
      boost::intrusive::member_hook<SNLNet, boost::intrusive::set_member_hook<>, &SNLNet::designNetsHook_>;
    using SNLDesignNets = boost::intrusive::set<SNLNet, SNLDesignNetsHook, boost::intrusive::compare<NLDesign::CompareByID<SNLNet>>>;
    using SNLDesignParametersHook =
      boost::intrusive::member_hook<SNLParameter, boost::intrusive::set_member_hook<>, &SNLParameter::designParametersHook_>;
    using SNLDesignParameters = boost::intrusive::set<SNLParameter, SNLDesignParametersHook>;

    NLID::DesignID                      id_;
    int                                 revisionCount_      {0};
    NLName                              name_               {};
    Type                                type_               { Type::Standard };
    NLLibrary*                          library_            {nullptr};
    boost::intrusive::set_member_hook<> libraryDesignsHook_ {};
    SNLDesignTerms                      terms_              {};
    SNLDesignObjectNameIDMap            termNameIDMap_      {};
    SNLDesignInstances                  instances_          {};
    SNLInstanceNameIDMap                instanceNameIDMap_  {};
    SNLDesignSlaveInstances             slaveInstances_     {};
    SNLDesignNets                       nets_               {};
    SNLDesignObjectNameIDMap            netNameIDMap_       {};
    SNLDesignParameters                 parameters_         {};
    std::vector<NLClock*>               clocks_             {};
    SNLRTLInfos*                        rtlInfos_           {nullptr};
};

}  // namespace naja::NL
