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
#include <yoga/YGNode.h>
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"

namespace fcitx::classicui {

template <typename T>
using GObjectUniquePtr = UniqueCPtr<T, g_object_unref>;

template <auto Getter>
float absolute(YGNodeRef node) {
    float offset = 0.0F;
    while (node) {
        offset += Getter(node);
        node = YGNodeGetParent(node);
    }
    return offset;
}

template <auto Getter, typename T, typename Deleter>
float absolute(const std::unique_ptr<T, Deleter> &node) {
    return absolute<Getter>(node.get());
}

FCITX_DECLARE_LOG_CATEGORY(classicui_logcategory);
#define CLASSICUI_DEBUG()                                                      \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Debug)
#define CLASSICUI_ERROR()                                                      \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Error)
#define CLASSICUI_INFO()                                                       \
    FCITX_LOGC(::fcitx::classicui::classicui_logcategory, Info)

} // namespace fcitx::classicui

#endif // _FCITX5_UI_CLASSIC_COMMON_H_
