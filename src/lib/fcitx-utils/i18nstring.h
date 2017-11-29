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
#ifndef _FCITX_UTILS_I18NSTRING_H_
#define _FCITX_UTILS_I18NSTRING_H_

#include "fcitxutils_export.h"
#include <locale>
#include <string>
#include <unordered_map>

namespace fcitx {
class FCITXUTILS_EXPORT I18NString {
public:
    I18NString() {}
    virtual ~I18NString() {}

    void set(const std::string &str, const std::string &locale = "") {
        if (locale.size()) {
            map_[locale] = str;
        } else {
            default_ = str;
        }
    }

    void clear() {
        default_.clear();
        map_.clear();
    }

    const std::string &match(const std::string &locale = "system") const;

    bool operator==(const I18NString &other) const {
        return other.default_ == default_ && other.map_ == map_;
    }

    bool operator!=(const I18NString &other) const {
        return !operator==(other);
    }

    const std::string &defaultString() const { return default_; }
    const std::unordered_map<std::string, std::string> &
    localizedStrings() const {
        return map_;
    }

protected:
    std::string default_;
    std::unordered_map<std::string, std::string> map_;
};
}

#endif // _FCITX_UTILS_I18NSTRING_H_
