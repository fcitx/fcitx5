/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "globalconfig.h"
#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "inputcontextmanager.h"

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
    Option<bool> enumerateWithTriggerKeys{
        this, "EnumerateWithTriggerKeys",
        _("Enumerate when press trigger key repeatedly"), true};
    KeyListOption altTriggerKeys{
        this,
        "AltTriggerKeys",
        _("Temporally switch between first and current Input Method"),
        {Key("Shift_L")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption enumerateForwardKeys{
        this,
        "EnumerateForwardKeys",
        _("Enumerate Input Method Forward"),
        {Key("Control+Shift_L")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess,
                          KeyConstrainFlag::AllowModifierOnly})};
    KeyListOption enumerateBackwardKeys{
        this,
        "EnumerateBackwardKeys",
        _("Enumerate Input Method Backward"),
        {Key("Control+Shift_R")},
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
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption defaultPrevCandidate{
        this,
        "PrevCandidate",
        _("Default Previous Candidate"),
        {Key("Shift+Tab")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption defaultNextCandidate{
        this,
        "NextCandidate",
        _("Default Next Candidate"),
        {Key("Tab")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption togglePreedit{this,
                                "TogglePreedit",
                                _("Toggle embedded preedit"),
                                {Key("Control+Alt+P")},
                                KeyListConstrain()};);

FCITX_CONFIGURATION(
    BehaviorConfig, Option<bool> activeByDefault{this, "ActiveByDefault",
                                                 _("Active By Default")};
    Option<PropertyPropagatePolicy> shareState{this, "ShareInputState",
                                               _("Share Input State"),
                                               PropertyPropagatePolicy::No};
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

bool GlobalConfig::enumerateWithTriggerKeys() const {
    FCITX_D();
    return *d->hotkey->enumerateWithTriggerKeys;
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

const KeyList &GlobalConfig::togglePreeditKeys() const {
    FCITX_D();
    return *d->hotkey->togglePreedit;
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

PropertyPropagatePolicy GlobalConfig::shareInputState() const {
    FCITX_D();
    return d->behavior->shareState.value();
}

const KeyList &GlobalConfig::defaultPrevPage() const {
    FCITX_D();
    return d->hotkey->defaultPrevPage.value();
}

const KeyList &GlobalConfig::defaultNextPage() const {
    FCITX_D();
    return d->hotkey->defaultNextPage.value();
}

const KeyList &GlobalConfig::defaultPrevCandidate() const {
    FCITX_D();
    return d->hotkey->defaultPrevCandidate.value();
}

const KeyList &GlobalConfig::defaultNextCandidate() const {
    FCITX_D();
    return d->hotkey->defaultNextCandidate.value();
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
