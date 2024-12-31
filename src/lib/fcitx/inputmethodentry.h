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
#include <fcitx/fcitxcore_export.h>

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
    /**
     * A compact label that intented to be shown in a compact space.
     *
     * usually some latin-character, or a single character of the input method
     * language.
     *
     * UI may choose to strip it to a shorter version if the content is too
     * long. For example, classicui will take the first section of text to make
     * it fit with in 3 character width. custom -> cus fr-tg -> fr mon-a1 -> mon
     * us (intl) -> us
     *
     * @return label text
     */
    const std::string &label() const;
    bool isConfigurable() const;

    /**
     * Helper function to check if this is a keyboard input method.
     *
     * @return is keyboard or not.
     */
    bool isKeyboard() const;

private:
    std::unique_ptr<InputMethodEntryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodEntry);
};
} // namespace fcitx

#endif // _FCITX_INPUTMETHODENTRY_H_
