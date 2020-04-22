//
// Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_INPUTMETHODCONFIG_P_H_
#define _FCITX_INPUTMETHODCONFIG_P_H_

#include <vector>
#include "fcitx-config/configuration.h"
#include "fcitx-utils/i18nstring.h"
#include "inputmethodentry.h"

namespace fcitx {
FCITX_CONFIGURATION(InputMethodGroupItemConfig,
                    Option<std::string> name{this, "Name", "Name"};
                    Option<std::string> layout{this, "Layout", "Layout"};);

FCITX_CONFIGURATION(
    InputMethodGroupConfig,
    Option<std::string> name{this, "Name", "Group Name"};
    Option<std::vector<InputMethodGroupItemConfig>> items{this, "Items",
                                                          "Items"};
    Option<std::string> defaultLayout{this, "Default Layout", "Layout"};
    Option<std::string> defaultInputMethod{this, "DefaultIM",
                                           "Default Input Method"};);

FCITX_CONFIGURATION(InputMethodConfig,
                    Option<std::vector<InputMethodGroupConfig>> groups{
                        this, "Groups", "Groups"};
                    Option<std::vector<std::string>> groupOrder{
                        this, "GroupOrder", "Group Order"};);

FCITX_CONFIGURATION(
    InputMethodInfoBase, Option<I18NString> name{this, "Name", "Name"};
    Option<std::string> icon{this, "Icon", "Icon"};
    Option<std::string> label{this, "Label", "Label"};
    Option<std::string> languageCode{this, "LangCode", "Language Code"};
    Option<std::string> addon{this, "Addon", "Addon"};
    Option<bool> configurable{this, "Configurable", "Configurable", false};)

FCITX_CONFIGURATION(InputMethodInfo, Option<InputMethodInfoBase> im{
                                         this, "InputMethod", "Input Method"};)

InputMethodEntry toInputMethodEntry(const std::string &uniqueName,
                                    const InputMethodInfo &config) {
    const auto &langCode = config.im->languageCode.value();
    const auto &name = config.im->name.value();
    InputMethodEntry result(uniqueName, name.match("system"), langCode,
                            config.im->addon.value());
    if (!langCode.empty() && langCode != "*") {
        const auto &nativeName = name.match(langCode);
        if (nativeName != name.defaultString()) {
            result.setNativeName(nativeName);
        }
    }
    result.setIcon(*config.im->icon)
        .setLabel(*config.im->label)
        .setConfigurable(*config.im->configurable);
    return result;
}
} // namespace fcitx

#endif // _FCITX_INPUTMETHODCONFIG_P_H_
