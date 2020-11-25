/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandmodule.h"
#include <stdexcept>
#include <wayland-client.h>
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"

namespace fcitx {

// Return false if XDG_SESSION_TYPE is set and is not wayland.
bool isWaylandSession() {
    const char *sessionType = getenv("XDG_SESSION_TYPE");
    if (sessionType && std::string_view(sessionType) != "wayland") {
        return false;
    }
    return true;
}

WaylandConnection::WaylandConnection(WaylandModule *wayland, const char *name)
    : parent_(wayland), name_(name ? name : "") {
    auto *display = wl_display_connect(name);
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

    if (wl_display_dispatch(*display_) < 0) {
        error_ = wl_display_get_error(*display_);
        FCITX_LOG_IF(Error, error_ != 0)
            << "Wayland connection got error: " << error_;
        if (error_ != 0) {
            return finish();
        }
    }
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
    FCITX_DEBUG() << "Display removed " << name;
    auto iter = conns_.find(name);
    if (iter != conns_.end()) {
        onConnectionClosed(iter->second);
        conns_.erase(iter);
    }
    if (name.empty() && instance_->exitWhenMainDisplayDisconnected() &&
        isWaylandSession()) {
        instance_->exit();
    }
}

std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
WaylandModule::addConnectionCreatedCallback(WaylandConnectionCreated callback) {
    auto result = createdCallbacks_.add(std::move(callback));

    for (auto &p : conns_) {
        auto &conn = p.second;
        (**result->handler())(conn.name(), *conn.display(), conn.focusGroup());
    }
    return result;
}

std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
WaylandModule::addConnectionClosedCallback(WaylandConnectionClosed callback) {
    return closedCallbacks_.add(std::move(callback));
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
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::WaylandModuleFactory);
