//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX5_UI_CLASSIC_COMMON_H_
#define _FCITX5_UI_CLASSIC_COMMON_H_

#include "fcitx-utils/log.h"
#include <glib-object.h>
#include <memory>

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
