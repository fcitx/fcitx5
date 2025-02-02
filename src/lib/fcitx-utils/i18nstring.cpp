/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "i18nstring.h"
#include <clocale>
#include <cstddef>
#include <string>
#include "charutils.h"
#include "environ.h"
#include "misc.h"

namespace fcitx {
const std::string &I18NString::match(const std::string &locale_) const {
    std::string locale = locale_;
    if (locale == "system") {
        std::optional<std::string> lc;
        if constexpr (isAndroid() || isApple() || isWindows()) {
            // bionic doesn't recognize locale other than C or C.UTF-8
            // https://android.googlesource.com/platform/bionic/+/refs/tags/android-11.0.0_r48/libc/bionic/locale.cpp#175
            // macOS returns C for setlocale(LC_MESSAGES, nullptr)
            // Windows's locale value doesn't like the one on linux,
            // While it is also Lang_Country.Encoding, it's based on windows own
            // naming. E.g.  Chinese (Simplified)_China.936 so assume
            // FCITX_LOCALE for now, until we have code to convert it to the
            // unix standard.
            lc = getEnvironment("FCITX_LOCALE");
        } else {
#ifndef LC_MESSAGES
            auto *lcMessages = setlocale(LC_ALL, nullptr);
#else
            auto *lcMessages = setlocale(LC_MESSAGES, nullptr);
#endif
            if (lcMessages) {
                lc = lcMessages;
            }
        }
        if (lc) {
            locale = std::move(*lc);
        } else {
            locale = "";
        }
    }
    // regex
    // ^(?P<language>[^_.@[:space:]]+)
    // (_(?P<territory>[[:upper:]]+))?
    // (\\.(?P<codeset>[-_0-9a-zA-Z]+))?
    // (@(?P<modifier>[[:ascii:]]+))?$
    //
    // check locale format.
    // [language]_[country].[encoding]@modifier
    // we don't want too large locale to match.
    std::string normalizedLocale;
    size_t languageLength = 0;
    size_t territoryLength = 0;
    bool failed = false;
    auto i = locale.begin();
    auto e = locale.end();
    do {
        while (i != e && !charutils::isspace(*i) && *i != '_' && *i != '.' &&
               *i != '@') {
            normalizedLocale.push_back(*i++);
        }

        if (i == locale.begin()) {
            failed = true;
            break;
        }
        languageLength = normalizedLocale.size();

        if (i != e && *i == '_') {
            normalizedLocale.push_back('_');
            i++;
            while (i != e && charutils::isupper(*i)) {
                normalizedLocale.push_back(*i);
                i++;
            }

            territoryLength = normalizedLocale.size();
        }

        if (i != e && *i == '.') {
            // encoding is useless for us
            i++;
            while (i != e &&
                   (charutils::isupper(*i) || charutils::islower(*i) ||
                    charutils::isdigit(*i) || *i == '_' || *i == '-')) {
                i++;
            }
        }

        if (i != e && *i == '@') {
            normalizedLocale.push_back('@');
            i++;
            while (i != e) {
                normalizedLocale.push_back(*i);
                i++;
            }
        }
    } while (false);

    if (failed) {
        normalizedLocale.clear();
        territoryLength = languageLength = 0;
    }

    if (normalizedLocale.empty()) {
        return default_;
    }
    auto iter = map_.find(normalizedLocale);
    if (territoryLength && iter == map_.end()) {
        iter = map_.find(normalizedLocale.substr(0, territoryLength));
    }
    if (languageLength && iter == map_.end()) {
        iter = map_.find(normalizedLocale.substr(0, languageLength));
    }
    if (iter != map_.end()) {
        return iter->second;
    }
    return default_;
}
} // namespace fcitx
