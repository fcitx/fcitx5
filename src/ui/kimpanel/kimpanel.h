/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_KIMPANEL_KIMPANEL_H_
#define _FCITX_UI_KIMPANEL_KIMPANEL_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/icontheme.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"

namespace fcitx {

class KimpanelProxy;
class Action;

FCITX_CONFIGURATION(KimpanelConfig,
                    Option<bool> preferTextIcon{this, "PreferTextIcon",
                                                _("Prefer Text Icon"), false};);

class Kimpanel : public UserInterface {
public:
    Kimpanel(Instance *instance);
    ~Kimpanel();

    Instance *instance() { return instance_; }
    const Configuration *getConfig() const override;
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/kimpanel.conf");
    }
    auto &config() const { return config_; }
    void suspend() override;
    void resume() override;
    bool available() override { return available_; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void updateInputPanel(InputContext *inputContext);
    void updateCurrentInputMethod(InputContext *ic);
    void reloadConfig() override;

    void msgV1Handler(dbus::Message &msg);
    void msgV2Handler(dbus::Message &msg);

    void registerAllProperties(InputContext *ic = nullptr);
    std::string inputMethodStatus(InputContext *ic);
    std::string actionToStatus(Action *action, InputContext *ic);

private:
    void setAvailable(bool available);

    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(classicui, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());

    Instance *instance_;
    dbus::Bus *bus_;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<KimpanelProxy> proxy_;
    std::unique_ptr<dbus::ServiceWatcherEntry> entry_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    TrackableObjectReference<InputContext> lastInputContext_;
    bool auxDownIsEmpty_ = true;
    std::unique_ptr<EventSourceTime> timeEvent_;
    bool available_ = false;
    std::unique_ptr<dbus::Slot> relativeQuery_;
    bool hasRelative_ = false;
    bool hasRelativeV2_ = false;
    KimpanelConfig config_;
};
} // namespace fcitx

#endif // _FCITX_UI_KIMPANEL_KIMPANEL_H_
