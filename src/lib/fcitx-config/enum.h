/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_ENUM_H_
#define _FCITX_CONFIG_ENUM_H_

#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>

#define FCITX_ENUM_STRINGIFY(X) FCITX_STRINGIFY(X),

/**
 * Define a enum with raw string value as name.
 *
 * This is basically a simple combination for the enum class and
 * FCITX_CONFIG_ENUM_NAME.
 * It is suitable for case that enum name is a single word.
 *
 * For example:
 * \code{.cpp}
 * FCITX_CONFIG_ENUM(PropertyPropagatePolicy, All, Program, No);
 * \endcode
 *
 * @see FCITX_CONFIG_ENUM_NAME
 */
#define FCITX_CONFIG_ENUM(TYPE, ...)                                           \
    enum class TYPE { __VA_ARGS__ };                                           \
    FCITX_CONFIG_ENUM_NAME(TYPE,                                               \
                           FCITX_FOR_EACH(FCITX_ENUM_STRINGIFY, __VA_ARGS__))

/**
 * Define the string representation for a enum value to be write to RawConfig.
 *
 * This is useful when:
 * 1. Enum string representation should be multiple words.
 * 2. Or define enum name for some existing non Fcitx enum type.
 */
#define FCITX_CONFIG_ENUM_NAME(TYPE, ...)                                      \
    static constexpr const char *_##TYPE##_Names[] = {__VA_ARGS__};            \
    static inline std::string _FCITX_UNUSED_ TYPE##ToString(TYPE value) {      \
        return _##TYPE##_Names[static_cast<std::underlying_type_t<TYPE>>(      \
            value)];                                                           \
    }                                                                          \
    static inline void _FCITX_UNUSED_ marshallOption(fcitx::RawConfig &config, \
                                                     const TYPE value) {       \
        config =                                                               \
            _##TYPE##_Names[static_cast<std::underlying_type_t<TYPE>>(value)]; \
    }                                                                          \
    static inline bool _FCITX_UNUSED_ unmarshallOption(                        \
        TYPE &value, const fcitx::RawConfig &config, bool) {                   \
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
        for (decltype(size) i = 0; i < size; i++) {                            \
            config.setValueByPath("Enum/" + std::to_string(i),                 \
                                  _##TYPE##_Names[i]);                         \
        }                                                                      \
    }

/**
 * Define the string representation for a enum value to be shown in UI.
 *
 * This should be used in combine with N_ macro.
 *
 * \code{.cpp}
 * FCITX_CONFIG_ENUM_NAME_WITH_I18N(QuickPhraseChooseModifier, N_("None"),
 *                                  N_("Alt"), N_("Control"), N_("Super"));
 * \endcode
 *
 * After using this macro, the Option template need to be set with associated
 * I18NAnnotation class.
 */
#define FCITX_CONFIG_ENUM_I18N_ANNOTATION(TYPE, ...)                           \
    struct TYPE##I18NAnnotation {                                              \
        static constexpr const char *_##TYPE##_Names2[] = {__VA_ARGS__};       \
        static constexpr size_t enumLength =                                   \
            FCITX_ARRAY_SIZE(_##TYPE##_Names2);                                \
        bool skipDescription() const { return false; }                         \
        bool skipSave() const { return false; }                                \
        void dumpDescription(fcitx::RawConfig &config) const {                 \
            for (size_t i = 0; i < enumLength; i++) {                          \
                config.setValueByPath("EnumI18n/" + std::to_string(i),         \
                                      _(_##TYPE##_Names[i]));                  \
            }                                                                  \
        }                                                                      \
        static std::string toString(TYPE value) {                              \
            return _(                                                          \
                _##TYPE##_Names[static_cast<std::underlying_type_t<TYPE>>(     \
                    value)]);                                                  \
        }                                                                      \
        static_assert(FCITX_ARRAY_SIZE(_##TYPE##_Names2) ==                    \
                          FCITX_ARRAY_SIZE(_##TYPE##_Names),                   \
                      "Enum mismatch");                                        \
        static constexpr bool equal(size_t idx) {                              \
            return idx >= enumLength                                           \
                       ? true                                                  \
                       : (::fcitx::stringutils::literalEqual(                  \
                              _##TYPE##_Names[idx], _##TYPE##_Names2[idx]) &&  \
                          equal(idx + 1));                                     \
        }                                                                      \
    };                                                                         \
    static_assert(TYPE##I18NAnnotation::equal(0), "Enum mismatch");

/**
 * A short hand for combining FCITX_CONFIG_ENUM_NAME and
 * FCITX_CONFIG_ENUM_I18N_ANNOTATION.
 *
 * @see FCITX_CONFIG_ENUM_NAME
 * @see FCITX_CONFIG_ENUM_I18N_ANNOTATION
 */
#define FCITX_CONFIG_ENUM_NAME_WITH_I18N(TYPE, ...)                            \
    FCITX_CONFIG_ENUM_NAME(TYPE, __VA_ARGS__);                                 \
    FCITX_CONFIG_ENUM_I18N_ANNOTATION(TYPE, __VA_ARGS__);

#endif // _FCITX_CONFIG_ENUM_H_
