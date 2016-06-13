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

#include "globalconfig.h"
#include "fcitx-config/configuration.h"

namespace fcitx {

namespace impl {
FCITX_CONFIGURATION(
    GlobalConfig, fcitx::Option<std::vector<Key>> triggerKeys{this,
                                                              "Hotkey/TriggerKeys",
                                                              "Trigger Input Method",
                                                              {Key("Ctrl+Space"), Key("Super+Space"),
                                                               Key("Zenkaku_Hankaku"), Key("Hangul")}};
    fcitx::Option<std::vector<Key>> altTriggerKeys{
        this, "Hotkey/AltTriggerKeys", "Trigger Input Method Only after using it to deactivate", {Key("L_Shift")}};
    fcitx::Option<std::vector<Key>> activateKeys{this,
                                                 "Hotkey/ActivateKeys",
                                                 "ActivateKeys",
                                                 {
                                                     Key("Hangul_Hanja"),
                                                 }};
    fcitx::Option<std::vector<Key>> deactivateKeys{
        this, "Hotkey/DeactivateKeys", "DeactivateKeys", {Key("Hangul_Romaja")}};);
}

class GlobalConfigPrivate : public impl::GlobalConfig {};

GlobalConfig::GlobalConfig() : d_ptr(std::make_unique<GlobalConfigPrivate>()) {}

GlobalConfig::~GlobalConfig() {}

const std::vector<Key> &GlobalConfig::triggerKeys() const {
    FCITX_D();
    return d->triggerKeys.value();
}
}
