/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTMETHODENTRY_H_
#define _FCITX_INPUTMETHODENTRY_H_

#include <memory>
#include <string>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

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

    const InputMethodEntryUserData *userData() const;

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
} // namespace fcitx

#endif // _FCITX_INPUTMETHODENTRY_H_
