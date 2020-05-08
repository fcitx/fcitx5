/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_PUBLIC_H_
#define _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_PUBLIC_H_

#include <functional>
#include <string>
#include <vector>
#include <fcitx/addoninstance.h>

namespace fcitx {

enum class NotificationsCapability {
    Actions = (1 << 0),
    Markup = (1 << 1),
    Link = (1 << 2),
    Body = (1 << 3)
};

typedef std::function<void(const std::string &)> NotificationActionCallback;
typedef std::function<void(uint32_t reason)> NotificationClosedCallback;
} // namespace fcitx

FCITX_ADDON_DECLARE_FUNCTION(
    Notifications, sendNotification,
    uint32_t(const std::string &appName, uint32_t replaceId,
             const std::string &appIcon, const std::string &summary,
             const std::string &body, const std::vector<std::string> &actions,
             int32_t timeout, NotificationActionCallback actionCallback,
             NotificationClosedCallback closedCallback));
FCITX_ADDON_DECLARE_FUNCTION(Notifications, showTip,
                             void(const std::string &tipId,
                                  const std::string &appName,
                                  const std::string &appIcon,
                                  const std::string &summary,
                                  const std::string &body, int32_t timeout));
FCITX_ADDON_DECLARE_FUNCTION(Notifications, closeNotification,
                             void(uint64_t internalId));

#endif // _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_PUBLIC_H_
