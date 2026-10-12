/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_NOTIFICATIONITEM_QTKEYNAME_H_
#define _FCITX_MODULES_NOTIFICATIONITEM_QTKEYNAME_H_

#include <string_view>
#include <fcitx-utils/key.h>

namespace fcitx {

std::string_view qtKeyName(KeySym key);

} // namespace fcitx

#endif // _FCITX_MODULES_NOTIFICATIONITEM_QTKEYNAME_H_
