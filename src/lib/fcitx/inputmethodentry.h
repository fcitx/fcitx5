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
    InputMethodEntry(const std::string &uniqueName, const std::string &name, const std::string &languageCode,
                     const std::string &addon)
        : m_uniqueName(uniqueName), m_name(name), m_languageCode(languageCode), m_addon(addon) {}
    InputMethodEntry(const InputMethodEntry &) = delete;
    InputMethodEntry(InputMethodEntry &&) = default;
    virtual ~InputMethodEntry() {}

    InputMethodEntry &setNativeName(const std::string &nativeName) {
        m_nativeName = nativeName;
        return *this;
    }

    InputMethodEntry &setIcon(const std::string &icon) {
        m_icon = icon;
        return *this;
    }

    InputMethodEntry &setLabel(const std::string &label) {
        m_label = label;
        return *this;
    }

    void setUserData(InputMethodEntryUserData *userData) { m_userData.reset(userData); }

    InputMethodEntryUserData *userData() { return m_userData.get(); }

    const std::string &name() const { return m_name; }
    const std::string &nativeName() const { return m_nativeName; }
    const std::string &icon() const { return m_icon; }
    const std::string &uniqueName() const { return m_uniqueName; }
    const std::string &languageCode() const { return m_languageCode; }
    const std::string &addon() const { return m_addon; }

private:
    std::string m_uniqueName;
    std::string m_name;
    std::string m_nativeName;
    std::string m_icon;
    std::string m_label;
    std::string m_languageCode;
    std::string m_addon;
    std::unique_ptr<InputMethodEntryUserData> m_userData;
};
}

#endif // _FCITX_INPUTMETHODENTRY_H_
