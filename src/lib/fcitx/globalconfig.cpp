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

#include "globalconfig.h"
#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"

namespace fcitx {

namespace impl {

FCITX_CONFIGURATION(
    HotkeyConfig,
    KeyListOption triggerKeys{
        this,
        "TriggerKeys",
        _("Trigger Input Method"),
        {Key("Control+space"), Key("Zenkaku_Hankaku"), Key("Hangul")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption altTriggerKeys{
        this,
        "AltTriggerKeys",
        _("Alternative Trigger Input Method"),
        {Key("Shift_L")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption enumerateForwardKeys{
        this,
        "EnumerateForwardKeys",
        _("Enumerate Input Method Forward"),
        {Key("Control+Shift_R")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption enumerateBackwardKeys{
        this,
        "EnumerateBackwardKeys",
        _("Enumerate Input Method Backward"),
        {Key("Control+Shift_L")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption enumerateGroupForwardKeys{
        this,
        "EnumerateGroupForwardKeys",
        _("Enumerate Input Method Group Forward"),
        {Key("Super+space")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption enumerateGroupBackwardKeys{
        this,
        "EnumerateGroupBackwardKeys",
        _("Enumerate Input Method Group Backward"),
        {Key("Super+Shift+space")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption activateKeys{
        this,
        "ActivateKeys",
        _("Activate Input Method"),
        {
            Key("Hangul_Hanja"),
        },
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption deactivateKeys{
        this,
        "DeactivateKeys",
        _("Deactivate Input Method"),
        {Key("Hangul_Romaja")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption defaultPrevPage{
        this,
        "PrevPage",
        _("Default Previous page"),
        {Key("Up")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption defaultNextPage{
        this,
        "NextPage",
        _("Default Next page"),
        {Key("Down")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};);

FCITX_CONFIGURATION(
    BehaviorConfig, Option<bool> activeByDefault{this, "ActiveByDefault",
                                                 _("Active By Default")};
    Option<bool> showInputMethodInformation{
        this, "ShowInputMethodInformation",
        _("Show Input Method Information when switch input method"), true};
    Option<bool> showInputMethodInformationWhenFocusIn{
        this, "showInputMethodInformationWhenFocusIn",
        _("Show Input Method Information when changing focus"), false};
    Option<int, IntConstrain> defaultPageSize{this, "DefaultPageSize",
                                              _("Default page size"), 5,
                                              IntConstrain(1, 10)};
    HiddenOption<std::vector<std::string>> enabledAddons{
        this, "EnabledAddons", "Force Enabled Addons"};
    HiddenOption<std::vector<std::string>> disabledAddons{
        this, "DisabledAddons", "Force Disabled Addons"};);

FCITX_CONFIGURATION(GlobalConfig,
                    Option<HotkeyConfig> hotkey{this, "Hotkey", _("Hotkey")};
                    Option<BehaviorConfig> behavior{this, "Behavior",
                                                    _("Behavior")};);
} // namespace impl

class GlobalConfigPrivate : public impl::GlobalConfig {};

GlobalConfig::GlobalConfig() : d_ptr(std::make_unique<GlobalConfigPrivate>()) {}

GlobalConfig::~GlobalConfig() {}

void GlobalConfig::load(const RawConfig &rawConfig, bool partial) {
    FCITX_D();
    d->load(rawConfig, partial);
}

void GlobalConfig::save(RawConfig &config) const {
    FCITX_D();
    d->save(config);
}

bool GlobalConfig::safeSave(const std::string &path) const {
    FCITX_D();
    return safeSaveAsIni(*d, path);
}

const KeyList &GlobalConfig::triggerKeys() const {
    FCITX_D();
    return *d->hotkey->triggerKeys;
}

const KeyList &GlobalConfig::altTriggerKeys() const {
    FCITX_D();
    return *d->hotkey->altTriggerKeys;
}

const KeyList &GlobalConfig::activateKeys() const {
    FCITX_D();
    return *d->hotkey->activateKeys;
}

const KeyList &GlobalConfig::deactivateKeys() const {
    FCITX_D();
    return d->hotkey->deactivateKeys.value();
}

const KeyList &GlobalConfig::enumerateForwardKeys() const {
    FCITX_D();
    return d->hotkey->enumerateForwardKeys.value();
}

const KeyList &GlobalConfig::enumerateBackwardKeys() const {
    FCITX_D();
    return d->hotkey->enumerateBackwardKeys.value();
}

const KeyList &GlobalConfig::enumerateGroupForwardKeys() const {
    FCITX_D();
    return *d->hotkey->enumerateGroupForwardKeys;
}

const KeyList &GlobalConfig::enumerateGroupBackwardKeys() const {
    FCITX_D();
    return *d->hotkey->enumerateGroupBackwardKeys;
}

bool GlobalConfig::activeByDefault() const {
    FCITX_D();
    return d->behavior->activeByDefault.value();
}

bool GlobalConfig::showInputMethodInformation() const {
    FCITX_D();
    return d->behavior->showInputMethodInformation.value();
}

bool GlobalConfig::showInputMethodInformationWhenFocusIn() const {
    FCITX_D();
    return d->behavior->showInputMethodInformationWhenFocusIn.value();
}

const KeyList &GlobalConfig::defaultPrevPage() const {
    FCITX_D();
    return d->hotkey->defaultPrevPage.value();
}

const KeyList &GlobalConfig::defaultNextPage() const {
    FCITX_D();
    return d->hotkey->defaultNextPage.value();
}

int GlobalConfig::defaultPageSize() const {
    FCITX_D();
    return d->behavior->defaultPageSize.value();
}

const std::vector<std::string> &GlobalConfig::enabledAddons() const {
    FCITX_D();
    return *d->behavior->enabledAddons;
}

const std::vector<std::string> &GlobalConfig::disabledAddons() const {
    FCITX_D();
    return *d->behavior->disabledAddons;
}

void GlobalConfig::setEnabledAddons(const std::vector<std::string> &addons) {
    FCITX_D();
    d->behavior.mutableValue()->enabledAddons.setValue(addons);
}

void GlobalConfig::setDisabledAddons(const std::vector<std::string> &addons) {
    FCITX_D();
    d->behavior.mutableValue()->disabledAddons.setValue(addons);
}

const Configuration &GlobalConfig::config() const {
    FCITX_D();
    return *d;
}
} // namespace fcitx
