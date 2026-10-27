// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "NajaObject.h"

namespace naja::NL {

class SNLInstance;
class SNLParameter;

class SNLInstParameter : public NajaObject {
  public:
    friend class SNLInstance;
    using super = NajaObject;

    SNLInstParameter() = delete;
    SNLInstParameter(const SNLInstParameter&) = delete;

    /**
     * \brief Create a new instance parameter.
     * \param instance The instance to which the parameter belongs.
     * \param parameter The instantiated parameter.
     * \param value The value of the parameter.
     * \return The new instance parameter.
     */
    static SNLInstParameter* create(SNLInstance* instance, SNLParameter* parameter, const std::string& value);
    /// \return The name of the parameter.
    NLName getName() const;
    /// \return The instance to which this instance parameter belongs.
    SNLInstance* getInstance() const { return instance_; }
    /// \return The instantiated parameter.
    SNLParameter* getParameter() const { return parameter_; }
    /// \return The value of the parameter.
    std::string getValue() const { return value_; }
    void setValue(const std::string& value);

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    friend bool operator< (const SNLInstParameter& lp, const SNLInstParameter& rp) {
      return lp.getName().getString() < rp.getName().getString();
    }
    struct SNLInstanceParameterComp {
      bool operator()(const NLName& ln, const SNLInstParameter& rp) const {
        return ln.getString() < rp.getName().getString();
      }
      bool operator()(const SNLInstParameter& lp, const NLName& rn) const {
        return lp.getName().getString() < rn.getString();
      }
    };
    bool deepCompare(const SNLInstParameter* other, std::string& reason) const;
  private:
    SNLInstParameter(SNLInstance* instance, SNLParameter* parameter, const std::string& value);
    static void preCreate(SNLInstance* instance, SNLParameter* parameter);
    void postCreate() override;
    void preDestroy() override;
    void destroyFromInstance();

    SNLInstance*                        instance_           {nullptr};
    SNLParameter*                       parameter_          {nullptr};
    std::string                         value_              {};
    boost::intrusive::set_member_hook<> instParametersHook_ {};
};

}  // namespace naja::NL
