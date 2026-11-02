// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NLContentHash.h"
#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "SNLDesignObject.h"

namespace naja::NL {

class SNLBitNet;

class SNLNet: public SNLDesignObject {
  public:
    /// Deterministic versioned SHA-256 of this object and its descendants.
    ContentHash getContentHash() const;

    friend class SNLDesign;
    using super = SNLDesignObject;

    class Type {
      public:
        enum TypeEnum {
          Standard, Assign0, Assign1, Supply0, Supply1, AssignX, AssignZ
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;
        bool operator==(const Type& other) const {
          return typeEnum_ == other.typeEnum_;
        }
        bool operator==(const TypeEnum& otherEnum) const {
          return typeEnum_ == otherEnum;
        }

        operator const TypeEnum&() const { return typeEnum_; }
        constexpr bool isAssign0() const { return typeEnum_ == Assign0;  }
        constexpr bool isAssign1() const { return typeEnum_ == Assign1;  }
        constexpr bool isAssignX() const { return typeEnum_ == AssignX;  }
        constexpr bool isAssignZ() const { return typeEnum_ == AssignZ;  }
        constexpr bool isAssign() const {
          return isAssign0() or isAssign1() or isAssignX() or isAssignZ();
        }
        constexpr bool isSupply0() const { return typeEnum_ == Supply0; }
        constexpr bool isSupply1() const { return typeEnum_ == Supply1; }
        constexpr bool isSupply() const { return isSupply0() or isSupply1(); }
        constexpr bool isConst0() const { return typeEnum_ == Assign0 or typeEnum_ == Supply0; }
        constexpr bool isConst1() const { return typeEnum_ == Assign1 or typeEnum_ == Supply1; }
        constexpr bool isConstX() const { return typeEnum_ == AssignX; }
        constexpr bool isConstZ() const { return typeEnum_ == AssignZ; }
        constexpr bool isDriving() const { return isAssign() or isSupply(); }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    NLID::DesignObjectReference getReference() const;
    /// \return this SNLNet unique ID in parent SNLDesign.
    virtual NLID::DesignObjectID getID() const = 0;

    /// \return net NLName.
    virtual NLName getName() const = 0;
    
    /// \return net width, 1 for SNLScalarNet and SNLBusNetBit.
    virtual NLID::Bit getWidth() const = 0;

    /**
     * \return this SNLNet SNLBitNet collection.
     * 
     * Will return itself for SNLScalarNet and SNLBusNetBit and a collection
     * of SNLBusNetBit for SNLBusNet.
     */ 
    virtual NajaCollection<SNLBitNet*> getBits() const = 0;

    /// \brief Change this SNLNet type. 
    virtual void setType(const Type& type) = 0;

    /// \return true if all bits of this net are assigned to 1'b0.
    virtual bool isAssign0() const = 0;
    /// \return true if all bits of this net are assigned to 1'b1.
    virtual bool isAssign1() const = 0;
    /// \return true if all bits of this net are assigned to 1'bx.
    virtual bool isAssignX() const = 0;
    /// \return true if all bits of this net are assigned to 1'bz.
    virtual bool isAssignZ() const = 0;
    /// \return true if all bits of this net are assign constants.
    bool isAssignConstant() const {
      return isAssign0() or isAssign1() or isAssignX() or isAssignZ();
    }
    /// \return true if all bits of this net are of type Supply0
    virtual bool isSupply0() const = 0;
    /// \return true if all bits of this net are of type Supply1
    virtual bool isSupply1() const = 0;
    /// \return true if all bits of this net are of type Supply0 or Supply1
    bool isSupply() const { return isSupply0() or isSupply1(); }
    /// \return true if all bits of this net are constants 0
    bool isConstant0() const { return isAssign0() or isSupply0(); }
    /// \return true if all bits of this net are constants 1
    bool isConstant1() const { return isAssign1() or isSupply1(); }
    /// \return true if all bits of this net are constants X
    bool isConstantX() const { return isAssignX(); }
    /// \return true if all bits of this net are constants Z
    bool isConstantZ() const { return isAssignZ(); }
    /// \return true if all bits of this net are constants
    bool isConstant() const {
      return isConstant0() or isConstant1() or isConstantX() or isConstantZ();
    }


    virtual bool deepCompare(const SNLNet* other, std::string& reason) const = 0;

  protected:
    SNLNet() = default;

    static void preCreate();
    void postCreate() override;
    void preDestroy() override;

  private:
    virtual void destroyFromDesign() = 0;
    virtual SNLNet* clone(SNLDesign* design) const = 0;

    //following used in BusNet and ScalarNet
    virtual void setID(NLID::DesignObjectID id) = 0;
    boost::intrusive::set_member_hook<> designNetsHook_ {};
};

}  // namespace naja::NL
