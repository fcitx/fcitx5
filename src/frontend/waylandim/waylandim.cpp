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
#include "fcitx-utils/event.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "display.h"
#include "waylandimserver.h"
#include "waylandimserverv2.h"

FCITX_DEFINE_LOG_CATEGORY(waylandim, "waylandim")

namespace fcitx {

WaylandIMModule::WaylandIMModule(Instance *instance) : instance_(instance) {
    createdCallback_ =
        wayland()->call<IWaylandModule::addConnectionCreatedCallback>(
            [this](const std::string &name, wl_display *display,
                   FocusGroup *group) {
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
            });
}

WaylandIMModule::~WaylandIMModule() {}

wayland::ZwpInputMethodV2 *WaylandIMModule::getInputMethodV2(InputContext *ic) {
    if (ic->frontend() != std::string_view("wayland_v2")) {
        return nullptr;
    }

    auto *v2IC = static_cast<WaylandIMInputContextV2 *>(ic);
    return v2IC->inputMethodV2();
}

class WaylandIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new WaylandIMModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::WaylandIMModuleFactory);
