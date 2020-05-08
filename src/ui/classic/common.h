/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_UI_CLASSIC_COMMON_H_
#define _FCITX5_UI_CLASSIC_COMMON_H_

#include <memory>
#include <glib-object.h>
#include "fcitx-utils/log.h"

namespace fcitx {
namespace classicui {

template <typename T>
using GObjectUniquePtr = std::unique_ptr<T, decltype(&g_object_unref)>;

template <typename T>
GObjectUniquePtr<T> makeGObjectUnique(T *p) {
    return {p, &g_object_unref};
}

FCITX_DECLARE_LOG_CATEGORY(classicui_logcategory);
#define CLASSICUI_DEBUG()                                                      \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Debug)

} // namespace classicui
} // namespace fcitx

#endif // _FCITX5_UI_CLASSIC_COMMON_H_
