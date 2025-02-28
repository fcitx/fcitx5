/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
#define _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <wayland-client-core.h>
#include <wayland-client.h>
#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-config/option.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
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
        true};
    OptionWithAnnotation<bool, ToolTipAnnotation> persistentVirtualKeyboard{
        this,
        "PersistentVirtualKeyboard",
        _("Keep virtual keyboard object for V2 Protocol (Need restart)"),
        false,
        {},
        {},
        {_("If enabled, when using zwp_input_method_v2, do not create and "
           "destroy zwp_virtual_keyboard_v1 on activate and deactivate. This "
           "may workaround some bug in certain Compositor, including "
           "Sway<=1.9, RiverWM<=0.3.0.")}};);

constexpr int32_t repeatHackDelay = 3000;
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

    bool persistentVirtualKeyboard() const {
        return persistentVirtualKeyboard_;
    }

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
    bool persistentVirtualKeyboard_ = false;
};
} // namespace fcitx

FCITX_DECLARE_LOG_CATEGORY(waylandim);

#define WAYLANDIM_DEBUG() FCITX_LOGC(::waylandim, Debug)

#endif // _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
