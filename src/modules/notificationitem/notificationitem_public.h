/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_PUBLIC_H_
#define _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_PUBLIC_H_

#include <fcitx-utils/handlertable.h>
#include <fcitx/addoninstance.h>

namespace fcitx {

using NotificationItemCallback = std::function<void(bool)>;

} // namespace fcitx

// When enable is called
FCITX_ADDON_DECLARE_FUNCTION(NotificationItem, enable, void());
// Callback will be called when registered changed.
FCITX_ADDON_DECLARE_FUNCTION(
    NotificationItem, watch,
    std::unique_ptr<HandlerTableEntry<NotificationItemCallback>>(
        NotificationItemCallback));
FCITX_ADDON_DECLARE_FUNCTION(NotificationItem, disable, void());
// Can be used to query current state.
FCITX_ADDON_DECLARE_FUNCTION(NotificationItem, registered, bool());

#endif // _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_PUBLIC_H_
