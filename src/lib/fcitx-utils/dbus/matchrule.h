//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_UTILS_DBUS_MATCHRULE_H_
#define _FCITX_UTILS_DBUS_MATCHRULE_H_

#include <fcitx-utils/macros.h>
#include <memory>
#include <string>
#include <vector>

namespace fcitx {
namespace dbus {

class MatchRulePrivate;
class Message;

struct FCITXUTILS_EXPORT MatchRule {
public:
    explicit MatchRule(std::string service, std::string path = "",
                       std::string interface = "", std::string name = "",
                       std::vector<std::string> argumentMatch = {});

    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(MatchRule)

    const std::string &rule() const noexcept;

    const std::string &service() const noexcept;
    const std::string &path() const noexcept;
    const std::string &interface() const noexcept;
    const std::string &name() const noexcept;
    const std::vector<std::string> &argumentMatch() const noexcept;

    bool check(const Message &, const std::string &alterName = {}) const;

    bool operator==(const MatchRule &other) const {
        return rule() == other.rule();
    }

    bool operator!=(const MatchRule &other) const { return !(*this == other); }

    static const std::string nullArg;

private:
    std::unique_ptr<MatchRulePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(MatchRule);
};

} // namespace dbus
} // namespace fcitx

namespace std {

template <>
struct hash<fcitx::dbus::MatchRule> {
    size_t operator()(const fcitx::dbus::MatchRule &rule) const noexcept {
        return std::hash<std::string>()(rule.rule());
    }
};
} // namespace std

#endif // _FCITX_UTILS_DBUS_MATCHRULE_H_
