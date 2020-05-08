/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_PUBLIC_H_
#define _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_PUBLIC_H_

#include <fcitx/addoninstance.h>

namespace fcitx {

using NotificationItemCallback = std::function<void(bool)>;

} // namespace fcitx

FCITX_ADDON_DECLARE_FUNCTION(NotificationItem, enable, void());
FCITX_ADDON_DECLARE_FUNCTION(
    NotificationItem, watch,
    std::unique_ptr<HandlerTableEntry<NotificationItemCallback>>(
        NotificationItemCallback));
FCITX_ADDON_DECLARE_FUNCTION(NotificationItem, disable, void());
FCITX_ADDON_DECLARE_FUNCTION(NotificationItem, registered, bool());

#endif // _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_PUBLIC_H_
