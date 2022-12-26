/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_IM_KEYBOARD_KEYBOARD_H_
#define _FCITX_IM_KEYBOARD_KEYBOARD_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/inputbuffer.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/instance.h"
#include "compose.h"
#include "isocodes.h"
#include "keyboard_public.h"
#include "longpress.h"
#include "quickphrase_public.h"
#include "xkbrules.h"

namespace fcitx {

class Instance;

enum class ChooseModifier { NoModifier, Alt, Control, Super };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(ChooseModifier, N_("None"), N_("Alt"),
                                 N_("Control"), N_("Super"));

FCITX_CONFIGURATION(
    KeyboardEngineConfig,
    Option<int, IntConstrain> pageSize{this, "PageSize", _("Page size"), 5,
                                       IntConstrain(3, 10)};
    KeyListOption prevCandidate{
        this,
        "PrevCandidate",
        _("Prev Candidate"),
        {Key("Shift+Tab")},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOption nextCandidate{
        this,
        "NextCandidate",
        _("Next Candidate"),
        {Key("Tab")},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    Option<bool> enableEmoji{this, "EnableEmoji", _("Enable emoji in hint"),
                             true};
    Option<bool> enableQuickphraseEmoji{this, "EnableQuickPhraseEmoji",
                                        _("Enable emoji in quickphrase"), true};
    OptionWithAnnotation<ChooseModifier, ChooseModifierI18NAnnotation>
        chooseModifier{this, "Choose Modifier", _("Choose key modifier"),
                       ChooseModifier::Alt};
    Option<bool> enableHintByDefault{this, "EnableHintByDefault",
                                     _("Enable hint by default"), false};
    KeyListOption hintTrigger{this,
                              "Hint Trigger",
                              _("Trigger hint mode"),
                              {Key("Control+Alt+H")},
                              KeyListConstrain()};
    KeyListOption oneTimeHintTrigger{this,
                                     "One Time Hint Trigger",
                                     _("Trigger hint mode for one time"),
                                     {Key("Control+Alt+J")},
                                     KeyListConstrain()};
    OptionWithAnnotation<bool, ToolTipAnnotation> useNewComposeBehavior{
        this,
        "UseNewComposeBehavior",
        _("Use new compose behavior"),
        true,
        {},
        {},
        {_("Show preedit when composing, and commit dead key if there is no "
           "matching sequence.")}};
    SubConfigOption spell{this, "Spell", _("Spell"),
                          "fcitx://config/addon/spell"};
    Option<bool> enableLongPress{this, "EnableLongPress",
                                 _("Type special characters with long press"),
                                 false};
    Option<std::vector<std::string>> blocklistApplicationForLongPress{
        this,
        "LongPressBlocklist",
        _("Applications disabled for long press"),
        {"konsole"}};
    SubConfigOption longPress{this, "LongPress", _("Long Press behavior"),
                              "fcitx://config/addon/keyboard/longpress"};);

class KeyboardEngine;

enum class CandidateMode { Hint, LongPress };

struct KeyboardEngineState : public InputContextProperty {
    KeyboardEngineState(KeyboardEngine *engine, InputContext *inputContext);
    KeyboardEngine *engine_;
    InputContext *inputContext_;
    bool enableWordHint_ = false;
    bool oneTimeEnableWordHint_ = false;
    InputBuffer buffer_;
    CandidateMode mode_ = CandidateMode::Hint;
    std::string origKeyString_;
    bool repeatStarted_ = false;
    ComposeState compose_;

    bool hintEnabled() const {
        return enableWordHint_ || oneTimeEnableWordHint_;
    }

    void reset(bool resetCompose = true);

    bool handleLongPress(const KeyEvent &event);
    bool handleSpellModeTrigger(const InputMethodEntry &entry,
                                const KeyEvent &event);
    bool handleCandidateSelection(const KeyEvent &event);
    std::tuple<std::string, bool> handleCompose(const KeyEvent &event);
    bool handleBackspace(const InputMethodEntry &entry);

    // Commit current buffer, also reset the state.
    // See also preeditString().
    void commitBuffer();
    std::string preeditString() const;
    std::string currentSelection() const;
    void showHintNotification(const InputMethodEntry &entry) const;

    void updateCandidate(const InputMethodEntry &entry);
    // Update preedit and send ui update.
    void setPreedit();

    // Return true if chr is pushed to buffer.
    // Return false if chr will be skipped by buffer, usually this means caller
    // need to call commit buffer and forward chr manually.
    bool updateBuffer(std::string_view chr);
};

class KeyboardEnginePrivate;

class KeyboardEngine final : public InputMethodEngineV3 {
public:
    KeyboardEngine(Instance *instance);
    ~KeyboardEngine();
    Instance *instance() { return instance_; }
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    std::vector<InputMethodEntry> listInputMethods() override;
    void reloadConfig() override;

    const KeyboardEngineConfig &config() { return config_; }
    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/keyboard.conf");
        reloadConfig();
    }

    const Configuration *getSubConfig(const std::string &path) const override;

    void setSubConfig(const std::string &, const fcitx::RawConfig &) override;

    void reset(const InputMethodEntry &entry,
               InputContextEvent &event) override;
    void deactivate(const InputMethodEntry &entry,
                    InputContextEvent &event) override;

    void invokeActionImpl(const fcitx::InputMethodEntry &entry,
                          fcitx::InvokeActionEvent &event) override;

    void resetState(InputContext *inputContext);

    FCITX_ADDON_DEPENDENCY_LOADER(spell, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(notifications, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(emoji, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(quickphrase, instance_->addonManager());

    auto factory() { return &factory_; }

    bool
    foreachLayout(const std::function<bool(
                      const std::string &layout, const std::string &description,
                      const std::vector<std::string> &languages)> &callback);
    bool foreachVariant(
        const std::string &layout,
        const std::function<
            bool(const std::string &variant, const std::string &description,
                 const std::vector<std::string> &languages)> &callback);

    bool isBlockedForLongPress(const std::string &program) const {
        return longPressBlocklistSet_.count(program) > 0;
    }
    const auto &longPressData() const { return longPressData_; }
    bool supportHint(const std::string &language);
    const auto &selectionKeys() const { return selectionKeys_; }
    auto selectionModifier() const { return selectionModifier_; }

private:
    FCITX_ADDON_EXPORT_FUNCTION(KeyboardEngine, foreachLayout);
    FCITX_ADDON_EXPORT_FUNCTION(KeyboardEngine, foreachVariant);

    void initQuickPhrase();

    Instance *instance_;
    AddonInstance *spell_ = nullptr;
    AddonInstance *notifications_ = nullptr;
    KeyboardEngineConfig config_;
    LongPressConfig longPressConfig_;
    std::unordered_map<std::string, std::vector<std::string>> longPressData_;
    XkbRules xkbRules_;
    std::string ruleName_;
    KeyStates selectionModifier_;
    KeyList selectionKeys_;
    std::unique_ptr<EventSource> deferEvent_;
    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
        quickphraseHandler_;

    FactoryFor<KeyboardEngineState> factory_{
        [this](InputContext &inputContext) {
            return new KeyboardEngineState(this, &inputContext);
        }};

    std::unordered_set<std::string> longPressBlocklistSet_;

    std::unique_ptr<EventSourceTime> cancelLastEvent_;
};

class KeyboardEngineFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new KeyboardEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX_IM_KEYBOARD_KEYBOARD_H_
