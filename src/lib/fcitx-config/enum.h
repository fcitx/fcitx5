/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_CONFIG_ENUM_H_
#define _FCITX_CONFIG_ENUM_H_

#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/macros.h>

#define FCITX_ENUM_STRINGIFY(X) FCITX_STRINGIFY(X),

#define FCITX_CONFIG_ENUM(TYPE, ...)                                           \
    enum class TYPE { __VA_ARGS__ };                                           \
    static const char *_##TYPE##_Names[] = {                                   \
        FCITX_FOR_EACH(FCITX_ENUM_STRINGIFY, __VA_ARGS__)};                    \
    static inline std::string to_string(TYPE value) {                          \
        return _##TYPE##_Names[static_cast<std::underlying_type_t<TYPE>>(      \
            value)];                                                           \
    }                                                                          \
    static inline void marshallOption(fcitx::RawConfig &config,                \
                                      const TYPE value) {                      \
        config =                                                               \
            _##TYPE##_Names[static_cast<std::underlying_type_t<TYPE>>(value)]; \
    }                                                                          \
    static inline bool unmarshallOption(TYPE &value,                           \
                                        const fcitx::RawConfig &config) {      \
        auto size = FCITX_ARRAY_SIZE(_##TYPE##_Names);                         \
        for (decltype(size) i = 0; i < size; i++) {                            \
            if (config.value() == _##TYPE##_Names[i]) {                        \
                value = static_cast<TYPE>(i);                                  \
                return true;                                                   \
            }                                                                  \
        }                                                                      \
        return false;                                                          \
    }                                                                          \
    _FCITX_UNUSED_                                                             \
    static void dumpDescriptionHelper(fcitx::RawConfig &config, TYPE *) {      \
        auto size = FCITX_ARRAY_SIZE(_##TYPE##_Names);                         \
        config.setValueByPath("Enum/Length", std::to_string(size));            \
        for (decltype(size) i = 0; i < size; i++) {                            \
            config.setValueByPath("Enum/" + std::to_string(i),                 \
                                  _##TYPE##_Names[i]);                         \
        }                                                                      \
    }

#endif // _FCITX_CONFIG_ENUM_H_
