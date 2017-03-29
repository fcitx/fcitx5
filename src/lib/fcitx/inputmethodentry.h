/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_INPUTMETHODENTRY_H_
#define _FCITX_INPUTMETHODENTRY_H_

#include "fcitxcore_export.h"
#include <memory>
#include <string>

namespace fcitx {

struct FCITXCORE_EXPORT InputMethodEntryUserData {
    virtual ~InputMethodEntryUserData(){};
};

class FCITXCORE_EXPORT InputMethodEntry {
public:
    InputMethodEntry(const std::string &uniqueName, const std::string &name,
                     const std::string &languageCode, const std::string &addon)
        : uniqueName_(uniqueName), name_(name), languageCode_(languageCode),
          addon_(addon) {}
    InputMethodEntry(const InputMethodEntry &) = delete;
    InputMethodEntry(InputMethodEntry &&) = default;
    virtual ~InputMethodEntry() {}

    InputMethodEntry &setNativeName(const std::string &nativeName) {
        nativeName_ = nativeName;
        return *this;
    }

    InputMethodEntry &setIcon(const std::string &icon) {
        icon_ = icon;
        return *this;
    }

    InputMethodEntry &setLabel(const std::string &label) {
        label_ = label;
        return *this;
    }

    void setUserData(InputMethodEntryUserData *userData) {
        userData_.reset(userData);
    }

    InputMethodEntryUserData *userData() { return userData_.get(); }

    const std::string &name() const { return name_; }
    const std::string &nativeName() const { return nativeName_; }
    const std::string &icon() const { return icon_; }
    const std::string &uniqueName() const { return uniqueName_; }
    const std::string &languageCode() const { return languageCode_; }
    const std::string &addon() const { return addon_; }

private:
    std::string uniqueName_;
    std::string name_;
    std::string nativeName_;
    std::string icon_;
    std::string label_;
    std::string languageCode_;
    std::string addon_;
    std::unique_ptr<InputMethodEntryUserData> userData_;
};
}

#endif // _FCITX_INPUTMETHODENTRY_H_
