/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_TYPENAME_H_
#define _FCITX_CONFIG_TYPENAME_H_

#include <string>
#include <fcitx-utils/color.h>
#include <fcitx-utils/i18nstring.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/semver.h>

namespace fcitx {

#define FCITX_SPECIALIZE_TYPENAME(TYPE, NAME)                                  \
    static inline std::string _FCITX_UNUSED_ configTypeNameHelper(TYPE *) {    \
        return NAME;                                                           \
    }

FCITX_SPECIALIZE_TYPENAME(bool, "Boolean");
FCITX_SPECIALIZE_TYPENAME(int, "Integer");
FCITX_SPECIALIZE_TYPENAME(std::string, "String");
FCITX_SPECIALIZE_TYPENAME(SemanticVersion, "Version");
FCITX_SPECIALIZE_TYPENAME(fcitx::Key, "Key");
FCITX_SPECIALIZE_TYPENAME(fcitx::Color, "Color");
FCITX_SPECIALIZE_TYPENAME(fcitx::I18NString, "I18NString");

template <typename T, typename = void>
struct OptionTypeName {
    static std::string get() {
        using ::fcitx::configTypeNameHelper;
        return configTypeNameHelper(static_cast<T *>(nullptr));
    }
};

template <typename T>
struct OptionTypeName<std::vector<T>> {
    static std::string get() { return "List|" + OptionTypeName<T>::get(); }
};

template <typename T>
struct OptionTypeName<T,
                      typename std::enable_if<std::is_enum<T>::value>::type> {
    static std::string get() { return "Enum"; }
};
} // namespace fcitx

#endif // _FCITX_CONFIG_TYPENAME_H_
