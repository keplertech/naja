// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <boost/intrusive/set.hpp>
#include <optional>

#include "NLName.h"
#include "NajaObject.h"

namespace naja::NL {

class SNLDesign;

class SNLParameter : public NajaObject {
  public:
    friend class SNLDesign;

    class Type {
      public:
        enum TypeEnum {
          Decimal, Binary, Boolean, String
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;

        operator const TypeEnum&() const { return typeEnum_; }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    SNLParameter() = delete;
    SNLParameter(const SNLParameter&) = delete;

    /**
     * \brief Create a SNLParameter.
     * \param design owner SNLDesign.
     * \param name NLName of the parameter.
     * \param type SNLParameter::Type of the parameter.
     * \param value value of the parameter.
     * \return created SNLParameter. 
     */
    static SNLParameter* create(SNLDesign* design, const NLName& name, Type type, const std::string& value);
    
    /// \brief Create a required parameter without a default value.
    static SNLParameter* create(SNLDesign* design, const NLName& name, Type type);

    /// \brief Destroy this SNLParameter.
    void destroy();
    /// \return this SNLParameter name.
    NLName getName() const { return name_; }
    /// \return whether this parameter has a default (including an empty string).
    bool hasDefaultValue() const { return value_.has_value(); }
    /// \return the default value; throws NLException if no default exists.
    std::string getValue() const;
    Type getType() const { return type_; }
    /// \return this SNLParameter owning SNLDesign.
    SNLDesign* getDesign() const { return design_; }

    const char* getTypeName() const;
    std::string getString() const;
    std::string getDescription() const;

    friend bool operator< (const SNLParameter& lp, const SNLParameter& rp) {
      return lp.getName().getString() < rp.getName().getString();
    }
    struct SNLParameterComp {
      bool operator()(const NLName& ln, const SNLParameter& rp) const {
        return ln.getString() < rp.getName().getString();
      }
      bool operator()(const SNLParameter& lp, const NLName& rn) const {
        return lp.getName().getString() < rn.getString();
      }
    };
    bool deepCompare(const SNLParameter* other, std::string& reason) const;
  private:
    SNLParameter(SNLDesign* design, const NLName& name, Type type, std::optional<std::string> value);
    static void preCreate(SNLDesign* design, const NLName& name);
    void postCreate();
    void destroyFromDesign();

    SNLDesign*                          design_                 { nullptr };
    NLName                              name_                   {};
    Type                                type_                   { Type::Decimal };
    std::optional<std::string>          value_                  {};
    boost::intrusive::set_member_hook<> designParametersHook_   {};
};

}  // namespace naja::NL