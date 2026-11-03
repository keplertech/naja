// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLContentHash.h"
#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "SNLNetComponent.h"

namespace naja::NL {

class SNLBitTerm;
class SNLBundleTerm;

class SNLTerm: public SNLNetComponent {
  public:
    /// Deterministic versioned SHA-256 of this object and its descendants.
    ContentHash getContentHash() const;

    friend class SNLDesign;
    using super = SNLNetComponent;
    
    NLID::DesignObjectReference getReference() const;
    
    /// \return the unique NLID::DesignObjectID of this SNLTerm in the containing SNLDesign.
    virtual NLID::DesignObjectID getID() const = 0;

    /**
     * \brief Get the flat id of this SNLTerm in the containing SNLDesign flat SNLBitTerms.
     * 
     * With three Terms created in following order:
     *   - SNLScalarTerm term1
     *   - SNLBusTerm term2[1:0]
     *   - SNLScalarTerm term3
     * 
     * Positions will be the following:
     * |0      | 1        | 2        | 3     |
     * |-------|----------|----------|-------|
     * |term0  | term2    |          | term3 |
     * |       | term2[1] | term2[0] |       |  
     * \return this SNLTerm flat id in the containing SNLDesign flat SNLBitTerms.
     **/
    virtual size_t getFlatID() const = 0;
    /// \return term NLName.
    virtual NLName getName() const = 0;
    /// \return term width, 1 for SNLScalarTerm and SNLBusNetBit.
    virtual NLID::Bit getWidth() const = 0;

    /// \return the Collection of SNLBitTerm composing this SNLTerm. 
    virtual NajaCollection<SNLBitTerm*> getBits() const = 0;

    /// \return the owning SNLBundleTerm when this term is bundled, nullptr otherwise.
    virtual SNLBundleTerm* getBundleOwner() const { return nullptr; }

    /// \return true when this term belongs to the top-level design interface.
    virtual bool isTopLevelTerm() const { return getBundleOwner() == nullptr; }

  protected:
    SNLTerm() = default;

    static void preCreate();
    void postCreate() override;
    void preDestroy() override;

  private:
    //following used in BusTerm and ScalarTerm
    virtual void setID(NLID::DesignObjectID id) = 0;
    virtual void setFlatID(size_t flatID) = 0;
    boost::intrusive::set_member_hook<> designTermsHook_  {};
    virtual void destroyFromDesign() = 0;
    virtual SNLTerm* clone(SNLDesign* design) const = 0;
};

}  // namespace naja::NL
