/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_MATCHRULE_H_
#define _FCITX_UTILS_DBUS_MATCHRULE_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <fcitx-utils/dbus/message.h>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/macros.h>

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief API for DBus matching rule.

namespace fcitx::dbus {

class MatchRulePrivate;
class Message;

/**
 * A dbus matching rule to be used with add match.
 *
 * Usually it is used to monitor certain signals on the bus.
 *
 * @see dbus::Bus::addMatch
 */
struct FCITXUTILS_EXPORT MatchRule {
public:
    explicit MatchRule(std::string service, std::string path = "",
                       std::string interface = "", std::string name = "",
                       std::vector<std::string> argumentMatch = {});
    explicit MatchRule(MessageType type, std::string service,
                       std::string destination = "", std::string path = "",
                       std::string interface = "", std::string name = "",
                       std::vector<std::string> argumentMatch = {},
                       bool eavesdrop = false);

    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(MatchRule)

    MessageType messageType() const noexcept;

    const std::string &rule() const noexcept;

    const std::string &service() const noexcept;
    const std::string &destination() const noexcept;
    const std::string &path() const noexcept;
    const std::string &interface() const noexcept;
    const std::string &name() const noexcept;
    const std::vector<std::string> &argumentMatch() const noexcept;
    bool eavesdrop() const noexcept;

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

} // namespace fcitx::dbus

namespace std {

template <>
struct hash<fcitx::dbus::MatchRule> {
    size_t operator()(const fcitx::dbus::MatchRule &rule) const noexcept {
        return std::hash<std::string>()(rule.rule());
    }
};
} // namespace std

#endif // _FCITX_UTILS_DBUS_MATCHRULE_H_
