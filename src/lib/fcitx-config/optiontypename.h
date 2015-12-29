/*
 * Copyright (C) 2015~2015 by CSSlayer
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
#ifndef _FCITX_CONFIG_TYPENAME_H_
#define _FCITX_CONFIG_TYPENAME_H_

#include <string>
#include <fcitx-utils/key.h>
#include <fcitx-utils/color.h>

namespace fcitx {

template <typename T>
struct OptionTypeName;

#define FCITX_SPECIALIZE_TYPENAME(TYPE, NAME)                                  \
    namespace fcitx {                                                          \
    template <>                                                                \
    struct OptionTypeName<TYPE> {                                              \
        static std::string get() { return NAME; }                              \
    };                                                                         \
    }

template <typename T>
struct OptionTypeName<std::vector<T>> {
    static std::string get() { return "List|" + OptionTypeName<T>::get(); }
};
}

FCITX_SPECIALIZE_TYPENAME(int, "Integer");
FCITX_SPECIALIZE_TYPENAME(std::string, "String");
FCITX_SPECIALIZE_TYPENAME(fcitx::Key, "Key");
FCITX_SPECIALIZE_TYPENAME(fcitx::Color, "Color");

#endif // _FCITX_CONFIG_TYPENAME_H_
