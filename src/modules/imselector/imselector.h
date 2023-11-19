/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_IMSELECTOR_IMSELECTOR_H_
#define _FCITX5_MODULES_IMSELECTOR_IMSELECTOR_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-config/option.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"

namespace fcitx {
using KeyListOptionWithToolTip =
    Option<KeyList, ListConstrain<KeyConstrain>, DefaultMarshaller<KeyList>,
           ToolTipAnnotation>;

FCITX_CONFIGURATION(
    IMSelectorConfig,
    KeyListOption triggerKey{
        this,
        "TriggerKey",
        _("Trigger Key"),
        {},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOption triggerKeyLocal{
        this,
        "TriggerKeyLocal",
        _("Trigger Key for only current input context"),
        {},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOptionWithToolTip switchKey{
        this,
        "SwitchKey",
        _("Hotkey for switching to the N-th input method"),
        {},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess),
        {},
        ToolTipAnnotation(
            _("The n-th hotkey in the list selects the n-th input method."))};
    KeyListOptionWithToolTip switchKeyLocal{
        this,
        "SwitchKeyLocal",
        _("Hotkey for switching to the N-th input "
          "method for only current input context"),
        {},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess),
        {},
        ToolTipAnnotation(
            _("The n-th hotkey in the list selects the n-th input method."))};);

class IMSelectorState : public InputContextProperty {
public:
    bool enabled_ = false;

    void reset(InputContext *ic) {
        enabled_ = false;
        ic->inputPanel().reset();
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
};

class IMSelector final : public AddonInstance {
public:
    IMSelector(Instance *instance);

    const IMSelectorConfig &config() const { return config_; }
    Instance *instance() { return instance_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/imselector.conf");
    }
    auto &factory() { return factory_; }

    void reloadConfig() override;
    bool trigger(InputContext *inputContext, bool local);

private:
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    Instance *instance_;
    IMSelectorConfig config_;
    KeyList selectionKeys_;
    FactoryFor<IMSelectorState> factory_;
};
} // namespace fcitx

#endif // _FCITX5_MODULES_IMSELECTOR_IMSELECTOR_H_
