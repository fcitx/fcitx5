/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandim.h"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include "fcitx-utils/event.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/misc_p.h"
#include "appmonitor.h"
#include "display.h"
#include "plasmaappmonitor.h"
#include "virtualinputcontext.h"
#include "wayland_public.h"
#include "waylandimserver.h"
#include "waylandimserverv2.h"
#include "wlrappmonitor.h"

FCITX_DEFINE_LOG_CATEGORY(waylandim, "waylandim")

namespace fcitx {

WaylandIMModule::WaylandIMModule(Instance *instance) : instance_(instance) {
    reloadConfig();
    createdCallback_ =
        wayland()->call<IWaylandModule::addConnectionCreatedCallback>(
            [this](const std::string &name, wl_display *display,
                   FocusGroup *group) {
                displays_[name] = display;
                appMonitor(name);
                WaylandIMServer *server =
                    new WaylandIMServer(display, group, name, this);
                servers_[name].reset(server);
                WaylandIMServerV2 *serverv2 =
                    new WaylandIMServerV2(display, group, name, this);
                serversV2_[name].reset(serverv2);
            });
    closedCallback_ =
        wayland()->call<IWaylandModule::addConnectionClosedCallback>(
            [this](const std::string &name, wl_display *) {
                servers_.erase(name);
                serversV2_.erase(name);
                appMonitors_.erase(name);
            });
}

WaylandIMModule::~WaylandIMModule() {}

wayland::ZwpInputMethodV2 *WaylandIMModule::getInputMethodV2(InputContext *ic) {
    if (ic->frontend() != std::string_view("wayland_v2")) {
        return nullptr;
    }

    if (auto *v2IC = dynamic_cast<WaylandIMInputContextV2 *>(ic)) {
        return v2IC->inputMethodV2();
    }
    auto *vic = static_cast<VirtualInputContext *>(ic);
    return static_cast<WaylandIMInputContextV2 *>(vic->parent())
        ->inputMethodV2();
}

AggregatedAppMonitor *WaylandIMModule::appMonitor(const std::string &display) {
    if (!*config_.detectApplication) {
        return nullptr;
    }

    auto displayIter = displays_.find(display);
    if (displayIter == displays_.end()) {
        return nullptr;
    }

    auto &appMonitorPtr = appMonitors_[display];
    if (!appMonitorPtr) {
        auto *display = static_cast<wayland::Display *>(
            wl_display_get_user_data(displayIter->second));

        auto plasmaMonitor = std::make_unique<PlasmaAppMonitor>(display);
        auto wlrMonitor = std::make_unique<WlrAppMonitor>(display);
        appMonitorPtr = std::make_unique<AggregatedAppMonitor>();
        if (getDesktopType() == DesktopType::KDE5) {
            appMonitorPtr->addSubMonitor(std::move(plasmaMonitor));
            appMonitorPtr->addSubMonitor(std::move(wlrMonitor));
        } else {
            appMonitorPtr->addSubMonitor(std::move(wlrMonitor));
            appMonitorPtr->addSubMonitor(std::move(plasmaMonitor));
        }
    }
    return appMonitorPtr.get();
}

class WaylandIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new WaylandIMModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::WaylandIMModuleFactory);
