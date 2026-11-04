// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>

namespace naja::TG {

class TNodeProxy {
  public:
    enum class Role { Single, Drive, Load };
    virtual ~TNodeProxy() = default;
    Role getRole() const { return role_; }
    virtual std::string getDescription() const { return {}; }
    /// A clock registered on a sampled role starts at its paired driver.
    std::size_t getDriverNode(std::size_t single) const {
      return role_ == Role::Load ? driver_ : single;
    }
    void setRole(Role role, std::size_t driver = 0) { role_ = role; driver_ = driver; }
  private:
    Role role_ {Role::Single};
    std::size_t driver_ {0};
};

}  // namespace naja::TG
