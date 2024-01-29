/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_H_
#define _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "quickphrase_public.h"
#include "quickphraseprovider.h"

namespace fcitx {

enum class QuickPhraseChooseModifier { NoModifier, Alt, Control, Super };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(QuickPhraseChooseModifier, N_("None"),
                                 N_("Alt"), N_("Control"), N_("Super"));

FCITX_CONFIGURATION(
    QuickPhraseConfig,
    KeyListOption triggerKey{
        this,
        "TriggerKey",
        _("Trigger Key"),
        {Key("Super+grave"), Key("Super+semicolon")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    OptionWithAnnotation<QuickPhraseChooseModifier,
                         QuickPhraseChooseModifierI18NAnnotation>
        chooseModifier{this, "Choose Modifier", _("Choose key modifier"),
                       QuickPhraseChooseModifier::NoModifier};
    Option<bool> enableSpell{this, "Spell", _("Enable Spell check"), true};
    Option<std::string> fallbackSpellLanguage{
        this, "FallbackSpellLanguage", _("Fallback Spell check language"),
        "en"};
    ExternalOption editor{this, "Editor", _("Editor"),
                          "fcitx://config/addon/quickphrase/editor"};);

class QuickPhraseState;
class QuickPhrase final : public AddonInstance {
public:
    QuickPhrase(Instance *instance);
    ~QuickPhrase();

    const QuickPhraseConfig &config() const { return config_; }
    Instance *instance() { return instance_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/quickphrase.conf");
    }
    void setSubConfig(const std::string &path,
                      const fcitx::RawConfig &) override {
        if (path == "editor") {
            reloadConfig();
        }
    }

    void reloadConfig() override;
    void updateUI(InputContext *inputContext);
    auto &factory() { return factory_; }

    void trigger(InputContext *ic, const std::string &text,
                 const std::string &prefix, const std::string &str,
                 const std::string &alt, const Key &key);
    void setBuffer(InputContext *ic, const std::string &text);

    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
        addProvider(QuickPhraseProviderCallback);

private:
    FCITX_ADDON_EXPORT_FUNCTION(QuickPhrase, trigger);
    FCITX_ADDON_EXPORT_FUNCTION(QuickPhrase, addProvider);
    FCITX_ADDON_EXPORT_FUNCTION(QuickPhrase, setBuffer);

    void setSelectionKeys(QuickPhraseAction action);

    QuickPhraseConfig config_;
    Instance *instance_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    KeyList selectionKeys_;
    KeyStates selectionModifier_;
    CallbackQuickPhraseProvider callbackProvider_;
    BuiltInQuickPhraseProvider builtinProvider_;
    SpellQuickPhraseProvider spellProvider_;
    FactoryFor<QuickPhraseState> factory_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_H_
