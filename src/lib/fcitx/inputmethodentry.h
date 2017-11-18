/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_INPUTMETHODENTRY_H_
#define _FCITX_INPUTMETHODENTRY_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/macros.h>
#include <memory>
#include <string>

namespace fcitx {

struct FCITXCORE_EXPORT InputMethodEntryUserData {
    virtual ~InputMethodEntryUserData() = default;
};

class InputMethodEntryPrivate;

class FCITXCORE_EXPORT InputMethodEntry {
public:
    InputMethodEntry(const std::string &uniqueName, const std::string &name,
                     const std::string &languageCode, const std::string &addon);
    InputMethodEntry(const InputMethodEntry &) = delete;
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(InputMethodEntry)

    InputMethodEntry &setNativeName(const std::string &nativeName);
    InputMethodEntry &setIcon(const std::string &icon);
    InputMethodEntry &setLabel(const std::string &label);
    InputMethodEntry &setConfigurable(bool configurable);
    void setUserData(std::unique_ptr<InputMethodEntryUserData> userData);

    InputMethodEntryUserData *userData();

    const std::string &name() const;
    const std::string &nativeName() const;
    const std::string &icon() const;
    const std::string &uniqueName() const;
    const std::string &languageCode() const;
    const std::string &addon() const;
    const std::string &label() const;
    bool isConfigurable() const;

private:
    std::unique_ptr<InputMethodEntryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodEntry);
};
}

#endif // _FCITX_INPUTMETHODENTRY_H_
