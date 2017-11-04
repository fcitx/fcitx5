/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "waylandmodule.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include <wayland-client.h>

namespace fcitx {

WaylandConnection::WaylandConnection(WaylandModule *wayland, const char *name)
    : parent_(wayland), name_(name ? name : "") {
    auto display = wl_display_connect(name);
    if (!display) {
        throw std::runtime_error("Failed to open wayland connection");
    }
    display_ = std::make_unique<wayland::Display>(display);

    auto &eventLoop = parent_->instance()->eventLoop();
    ioEvent_ =
        eventLoop.addIOEvent(display_->fd(), IOEventFlag::In,
                             [this](EventSource *, int, IOEventFlags flags) {
                                 onIOEvent(flags);
                                 return true;
                             });

    group_ = std::make_unique<FocusGroup>(
        "wayland:" + name_, wayland->instance()->inputContextManager());
}

WaylandConnection::~WaylandConnection() {}

void WaylandConnection::finish() { parent_->removeDisplay(name_); }

void WaylandConnection::onIOEvent(IOEventFlags flags) {
    if ((flags & IOEventFlag::Err) || (flags & IOEventFlag::Hup)) {
        return finish();
    }

    if (wl_display_prepare_read(*display_) == 0) {
        wl_display_read_events(*display_);
    }

    if (wl_display_dispatch_pending(*display_) < 0) {
        error_ = wl_display_get_error(*display_);
        if (error_ != 0) {
            return finish();
        }
    }

    display_->flush();
}

WaylandModule::WaylandModule(fcitx::Instance *instance) : instance_(instance) {
    openDisplay("");
}

void WaylandModule::openDisplay(const std::string &name) {
    const char *displayString = nullptr;
    if (!name.empty()) {
        displayString = name.c_str();
    }

    try {
        auto iter = conns_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(name),
                                   std::forward_as_tuple(this, displayString));
        onConnectionCreated(iter.first->second);
    } catch (const std::exception &e) {
    }
}

void WaylandModule::removeDisplay(const std::string &name) {
    FCITX_LOG(Debug) << "Display removed " << name;
    auto iter = conns_.find(name);
    if (iter != conns_.end()) {
        onConnectionClosed(iter->second);
        conns_.erase(iter);
    }
    if (name.empty() && instance_->quitWhenMainDisplayDisconnected()) {
        instance_->exit();
    }
}

std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
WaylandModule::addConnectionCreatedCallback(WaylandConnectionCreated callback) {
    auto result = createdCallbacks_.add(callback);

    for (auto &p : conns_) {
        auto &conn = p.second;
        callback(conn.name(), *conn.display(), conn.focusGroup());
    }
    return result;
}

std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
WaylandModule::addConnectionClosedCallback(WaylandConnectionClosed callback) {
    return closedCallbacks_.add(callback);
}

void WaylandModule::onConnectionCreated(WaylandConnection &conn) {
    for (auto &callback : createdCallbacks_.view()) {
        callback(conn.name(), *conn.display(), conn.focusGroup());
    }
}

void WaylandModule::onConnectionClosed(WaylandConnection &conn) {
    for (auto &callback : closedCallbacks_.view()) {
        callback(conn.name(), *conn.display());
    }
}

class WaylandModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new WaylandModule(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::WaylandModuleFactory);
