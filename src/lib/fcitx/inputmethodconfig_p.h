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
#ifndef _FCITX_INPUTMETHODCONFIG_P_H_
#define _FCITX_INPUTMETHODCONFIG_P_H_

#include "fcitx-config/configuration.h"
#include "fcitx-utils/i18nstring.h"
#include "inputmethodentry.h"

namespace fcitx {

FCITX_CONFIGURATION(InputMethodConfig,
                    fcitx::Option<std::string> uniqueName{this, "InputMethod/UniqueName", "Unique Name"};
                    fcitx::Option<I18NString> name{this, "InputMethod/Name", "Name"};
                    fcitx::Option<std::string> icon{this, "InputMethod/Icon", "Icon"};
                    fcitx::Option<std::string> label{this, "InputMethod/Label", "Label"};
                    fcitx::Option<std::string> languageCode{this, "InputMethod/LangCode", "Language Code"};
                    fcitx::Option<std::string> addon{this, "InputMethod/Addon", "Addon"};)

InputMethodEntry toInputMethodEntry(const InputMethodConfig &config) {
    InputMethodEntry result(config.uniqueName.value(), config.name.value().match("system"),
                            config.languageCode.value());
    return result;
}
}

#endif // _FCITX_INPUTMETHODCONFIG_P_H_
