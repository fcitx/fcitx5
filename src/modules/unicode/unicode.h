/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_UNICODE_UNICODE_H_
#define _FCITX_MODULES_UNICODE_UNICODE_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "charselectdata.h"
#include "clipboard_public.h"
#include "unicode_public.h"

namespace fcitx {

FCITX_CONFIGURATION(
    UnicodeConfig, KeyListOption triggerKey{this,
                                            "TriggerKey",
                                            _("Trigger Key"),
                                            {Key("Control+Alt+Shift+U")},
                                            KeyListConstrain()};
    KeyListOption directUnicodeKey{this,
                                   "DirectUnicodeMode",
                                   _("Type unicode in Hex number"),
                                   {Key("Control+Shift+U")},
                                   KeyListConstrain()};

);

class UnicodeState;
class Unicode : public AddonInstance {
    static constexpr char configFile[] = "conf/unicode.conf";

public:
    Unicode(Instance *instance);
    ~Unicode();

    Instance *instance() { return instance_; }

    bool trigger(InputContext *inputContext);
    bool triggerDirect(KeyEvent &keyEvent);
    void updateUI(InputContext *inputContext, bool trigger = false);
    auto &factory() { return factory_; }

    const CharSelectData &data() const { return data_; }

    void reloadConfig() override { readAsIni(config_, configFile); }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, configFile);
    }

    FCITX_ADDON_DEPENDENCY_LOADER(clipboard, instance_->addonManager());

private:
    void handleSearch(KeyEvent &keyEvent);
    void handleDirect(KeyEvent &keyEvent);

    FCITX_ADDON_EXPORT_FUNCTION(Unicode, trigger);
    Instance *instance_;
    UnicodeConfig config_;
    CharSelectData data_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    KeyList selectionKeys_;
    FactoryFor<UnicodeState> factory_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_UNICODE_UNICODE_H_
