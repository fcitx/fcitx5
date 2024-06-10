/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_IM_KEYBOARD_LONGPRESSDATA_H_
#define _FCITX5_IM_KEYBOARD_LONGPRESSDATA_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "fcitx-config/configuration.h"
#include "fcitx-config/option.h"
#include "fcitx-utils/i18n.h"

namespace fcitx {

FCITX_CONFIGURATION(LongPressEntryConfig,
                    Option<std::string> key{this, "Key", _("Key")};
                    Option<bool> enable{this, "Enable", _("Enable"), true};
                    Option<std::vector<std::string>> candidates{
                        this, "Candidates", _("Candidates")};)

FCITX_CONFIGURATION(LongPressConfig,
                    OptionWithAnnotation<std::vector<LongPressEntryConfig>,
                                         ListDisplayOptionAnnotation>
                        entries{this,
                                "Entries",
                                _("Entries"),
                                {},
                                {},
                                {},
                                ListDisplayOptionAnnotation("Key")};);

std::unordered_map<std::string, std::vector<std::string>>
loadLongPressData(const LongPressConfig &config);

void setupDefaultLongPressConfig(LongPressConfig &config);

} // namespace fcitx

#endif // _FCITX5_IM_KEYBOARD_LONGPRESSDATA_H_
