/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_PUBLIC_H_
#define _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_PUBLIC_H_

#include <fcitx/addoninstance.h>
#include <functional>
#include <string>
#include <vector>

namespace fcitx {

enum class NotificationsCapability { Actions = (1 << 0), Markup = (1 << 1), Link = (1 << 2), Body = (1 << 3) };

typedef std::function<void(const std::string &)> NotificationActionCallback;
typedef std::function<void(uint32_t reason)> NotificationClosedCallback;
}

FCITX_ADDON_DECLARE_FUNCTION(Notifications, sendNotification,
                             uint32_t(const std::string &appName, uint32_t replaceId, const std::string &appIcon,
                                      const std::string &summary, const std::string &body,
                                      const std::vector<std::string> &actions, int32_t timeout,
                                      NotificationActionCallback actionCallback,
                                      NotificationClosedCallback closedCallback));
FCITX_ADDON_DECLARE_FUNCTION(Notifications, showTip,
                             void(const std::string &tipId, const std::string &appName, const std::string &appIcon,
                                  const std::string &summary, const std::string &body, int32_t timeout));
FCITX_ADDON_DECLARE_FUNCTION(Notifications, closeNotification, void(uint64_t internalId));

#endif // _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_PUBLIC_H_
