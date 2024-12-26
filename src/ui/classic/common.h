/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_UI_CLASSIC_COMMON_H_
#define _FCITX5_UI_CLASSIC_COMMON_H_

#include <glib-object.h>
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"

namespace fcitx::classicui {

template <typename T>
using GObjectUniquePtr = UniqueCPtr<T, g_object_unref>;

FCITX_DECLARE_LOG_CATEGORY(classicui_logcategory);
#define CLASSICUI_DEBUG()                                                      \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Debug)
#define CLASSICUI_ERROR()                                                      \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Error)
#define CLASSICUI_INFO()                                                       \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Info)

} // namespace fcitx::classicui

#endif // _FCITX5_UI_CLASSIC_COMMON_H_
