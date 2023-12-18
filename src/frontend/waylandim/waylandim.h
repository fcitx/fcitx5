/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
#define _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_

#include <string>
#include <unordered_map>
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include "appmonitor.h"
#include "wayland_public.h"
#include "waylandim_public.h"

namespace fcitx {

FCITX_CONFIGURATION(
    WaylandIMConfig,
    Option<bool> detectApplication{
        this, "DetectApplication",
        _("Detect current running application (Need restart)"), true};
    Option<bool> preferKeyEvent{
        this, "PreferKeyEvent",
        _("Forward key event instead of commiting text if it is not handled"),
        true};);

constexpr int32_t repeatHackDelay = 1000;
class WaylandIMModule;
class WaylandIMServer;
class WaylandIMServerV2;

class WaylandIMModule : public AddonInstance {
public:
    WaylandIMModule(Instance *instance);
    ~WaylandIMModule();

    FCITX_ADDON_DEPENDENCY_LOADER(wayland, instance_->addonManager());
    Instance *instance() { return instance_; }

    wayland::ZwpInputMethodV2 *getInputMethodV2(InputContext *ic);

    bool hasKeyboardGrab(const std::string &display) const;

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/waylandim.conf");
    }
    void reloadConfig() override { readAsIni(config_, "conf/waylandim.conf"); }
    const auto &config() const { return config_; }

    FCITX_ADDON_EXPORT_FUNCTION(WaylandIMModule, getInputMethodV2);
    FCITX_ADDON_EXPORT_FUNCTION(WaylandIMModule, hasKeyboardGrab);

    AggregatedAppMonitor *appMonitor(const std::string &display);

private:
    Instance *instance_;
    WaylandIMConfig config_;
    std::unordered_map<std::string, wl_display *> displays_;
    std::unordered_map<std::string, std::unique_ptr<WaylandIMServer>> servers_;
    std::unordered_map<std::string, std::unique_ptr<WaylandIMServerV2>>
        serversV2_;
    std::unordered_map<std::string, std::unique_ptr<AggregatedAppMonitor>>
        appMonitors_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
        createdCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>> closedCallback_;
};
} // namespace fcitx

FCITX_DECLARE_LOG_CATEGORY(waylandim);

#define WAYLANDIM_DEBUG() FCITX_LOGC(::waylandim, Debug)

#endif // _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
