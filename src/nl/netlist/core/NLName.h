// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace naja::NL {

// ASCII identifiers are folded without depending on the process locale.
inline std::string foldNameCase(std::string_view name) {
  std::string folded(name);
  for (auto& c : folded) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return folded;
}

class NLName {
  public:
    using ID = std::uint32_t;
    explicit NLName(const std::string& name = std::string());
    NLName(const NLName& other) = default;
    NLName& operator=(const NLName& other) = default;
    NLName(NLName&&) = default;
    NLName& operator=(NLName&&) = default;
    ~NLName() = default;

    /// \return the string representation of this NLName.
    std::string getString() const {
      return getStringRef();
    }

    /// \return true if this NLName is empty, false otherwise.
    bool empty() const {
      return getStringRef().empty();
    }
    /// \return the unique ID of this NLName.
    ID getID() const { return id_; }
    friend bool operator< (const NLName& lname, const NLName& rname) {
      return lname.id_ < rname.id_;
    }
    friend bool operator> (const NLName& lname, const NLName& rname) {
      return lname.id_ > rname.id_;
    }
    friend bool operator== (const NLName& lname, const NLName& rname) {
      return lname.id_ == rname.id_;
    }
    friend bool operator!= (const NLName& lname, const NLName& rname) {
      return lname.id_ != rname.id_;
    }
  private:
    const std::string& getStringRef() const;

    ID id_ {0};
};

template<typename T>
struct NLNameComp {
  bool operator()(const NL::NLName& name, const T& obj) const {
    return name < obj.getName();
  }
  bool operator()(const T& obj, const NL::NLName& name) const {
    return obj.getName() < name;
  }
};

}  // namespace naja::NL