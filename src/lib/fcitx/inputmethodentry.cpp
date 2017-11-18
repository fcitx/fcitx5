//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "inputmethodentry.h"

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

InputMethodEntryUserData *InputMethodEntry::userData() {
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
}
