/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputmethodentry.h"

namespace fcitx {

class InputMethodEntryPrivate {
public:
    InputMethodEntryPrivate(const std::string &uniqueName,
                            const std::string &name,
                            const std::string &languageCode,
                            const std::string &addon)
        : uniqueName_(uniqueName), name_(name), languageCode_(languageCode),
          addon_(addon) {}

    std::string uniqueName_;
    std::string name_;
    std::string nativeName_;
    std::string icon_;
    std::string label_;
    std::string languageCode_;
    std::string addon_;
    bool configurable_ = false;
    std::unique_ptr<InputMethodEntryUserData> userData_;
};

InputMethodEntry::InputMethodEntry(const std::string &uniqueName,
                                   const std::string &name,
                                   const std::string &languageCode,
                                   const std::string &addon)
    : d_ptr(std::make_unique<InputMethodEntryPrivate>(uniqueName, name,
                                                      languageCode, addon)) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(InputMethodEntry)

InputMethodEntry &
InputMethodEntry::setNativeName(const std::string &nativeName) {
    FCITX_D();
    d->nativeName_ = nativeName;
    return *this;
}

InputMethodEntry &InputMethodEntry::setIcon(const std::string &icon) {
    FCITX_D();
    d->icon_ = icon;
    return *this;
}

InputMethodEntry &InputMethodEntry::setLabel(const std::string &label) {
    FCITX_D();
    d->label_ = label;
    return *this;
}

InputMethodEntry &InputMethodEntry::setConfigurable(bool configurable) {
    FCITX_D();
    d->configurable_ = configurable;
    return *this;
}

void InputMethodEntry::setUserData(
    std::unique_ptr<InputMethodEntryUserData> userData) {
    FCITX_D();
    d->userData_ = std::move(userData);
}

const InputMethodEntryUserData *InputMethodEntry::userData() const {
    FCITX_D();
    return d->userData_.get();
}

const std::string &InputMethodEntry::name() const {
    FCITX_D();
    return d->name_;
}
const std::string &InputMethodEntry::nativeName() const {
    FCITX_D();
    return d->nativeName_;
}
const std::string &InputMethodEntry::icon() const {
    FCITX_D();
    return d->icon_;
}
const std::string &InputMethodEntry::uniqueName() const {
    FCITX_D();
    return d->uniqueName_;
}
const std::string &InputMethodEntry::languageCode() const {
    FCITX_D();
    return d->languageCode_;
}
const std::string &InputMethodEntry::addon() const {
    FCITX_D();
    return d->addon_;
}
const std::string &InputMethodEntry::label() const {
    FCITX_D();
    return d->label_;
}
bool InputMethodEntry::isConfigurable() const {
    FCITX_D();
    return d->configurable_;
}
} // namespace fcitx
