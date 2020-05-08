/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_MATCHRULE_H_
#define _FCITX_UTILS_DBUS_MATCHRULE_H_

#include <memory>
#include <string>
#include <vector>
#include <fcitx-utils/macros.h>

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

    bool check(Message &, const std::string &alterName = {}) const;

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
