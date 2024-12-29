/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_I18NSTRING_H_
#define _FCITX_UTILS_I18NSTRING_H_

#include <string>
#include <unordered_map>
#include <fcitx-utils/fcitxutils_export.h>

namespace fcitx {
class FCITXUTILS_EXPORT I18NString {
public:
    I18NString() = default;
    virtual ~I18NString() = default;

    void set(const std::string &str, const std::string &locale = "") {
        if (!locale.empty()) {
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
} // namespace fcitx

#endif // _FCITX_UTILS_I18NSTRING_H_
