/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_H_
#define _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "quickphrase_public.h"
#include <map>

namespace fcitx {

FCITX_CONFIG_ENUM(QuickPhraseChooseModifier, None, Alt, Control, Super);
FCITX_CONFIG_ENUM_I18N_ANNOTATION(QuickPhraseChooseModifier, N_("None"),
                                  N_("Alt"), N_("Control"), N_("Super"));

FCITX_CONFIGURATION(
    QuickPhraseConfig,
    KeyListOption triggerKey{
        this,
        "TriggerKey",
        "Trigger Key",
        {Key("Super+grave")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    fcitx::OptionWithAnnotation<QuickPhraseChooseModifier,
                                QuickPhraseChooseModifierI18NAnnotation>
        chooseModifier{this, "Choose Modifier", "Choose key modifier",
                       QuickPhraseChooseModifier::None};);

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

    void reloadConfig() override;
    void load(StandardPathFile &file);
    void updateUI(InputContext *inputContext);
    auto &factory() { return factory_; }

    void trigger(InputContext *ic, const std::string &text,
                 const std::string &prefix, const std::string &str,
                 const std::string &alt, const Key &key);

private:
    FCITX_ADDON_EXPORT_FUNCTION(QuickPhrase, trigger);

    std::multimap<std::string, std::string> map_;
    QuickPhraseConfig config_;
    Instance *instance_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    KeyList selectionKeys_;
    FactoryFor<QuickPhraseState> factory_;
};
}

#endif // _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_H_
