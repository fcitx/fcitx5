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

#include "i18nstring.h"
#include "charutils.h"

namespace fcitx {
const std::string &I18NString::match(std::string locale) const {
    if (locale == "system") {
        char *lc = setlocale(LC_MESSAGES, nullptr);
        if (lc) {
            locale = lc;
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
    auto i = locale.begin(), e = locale.end();
    do {
        while (i != e && !charutils::isspace(*i) && *i != '_' && *i != '.' && *i != '@') {
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
            while (i != e && (charutils::isupper(*i) || charutils::islower(*i) || charutils::isdigit(*i) || *i == '_' ||
                              *i == '-')) {
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
    } while (0);

    if (failed) {
        normalizedLocale.clear();
        territoryLength = languageLength = 0;
    }

    if (normalizedLocale.size() == 0) {
        return m_default;
    }
    auto iter = m_map.find(normalizedLocale);
    if (territoryLength && iter == m_map.end()) {
        iter = m_map.find(normalizedLocale.substr(0, territoryLength));
    }
    if (languageLength && iter == m_map.end()) {
        iter = m_map.find(normalizedLocale.substr(0, languageLength));
    }
    if (iter != m_map.end()) {
        return iter->second;
    }
    return m_default;
}
}
