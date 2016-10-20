/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include <fcitx/instance.h>
#include <wayland-client.h>

namespace fcitx {

WaylandConnection::WaylandConnection(WaylandModule *wayland, const char *name)
    : parent_(wayland), name_(name ? name : ""), display_(nullptr, wl_display_disconnect) {
    display_.reset(wl_display_connect(name));
    if (!display_) {
        throw std::runtime_error("Failed to open wayland connection");
    }

    auto &eventLoop = parent_->instance()->eventLoop();
    ioEvent_.reset(eventLoop.addIOEvent(wl_display_get_fd(display_.get()), IOEventFlag::In,
                                        [this](EventSource *, int, IOEventFlags flags) {
                                            onIOEvent(flags);
                                            return true;
                                        }));

    group_ = new FocusGroup(wayland->instance()->inputContextManager());
}

WaylandConnection::~WaylandConnection() { delete group_; }

void WaylandConnection::finish() {
    display_.reset();
    if (name_.empty()) {
        parent_->instance()->exit();
    }
    parent_->removeDisplay(name_);
}

void WaylandConnection::onIOEvent(IOEventFlags flags) {
    if ((flags & IOEventFlag::Err) || (flags & IOEventFlag::Hup)) {
        return finish();
    }

    if (wl_display_prepare_read(display_.get()) == 0) {
        wl_display_read_events(display_.get());
    }

    if (wl_display_dispatch_pending(display_.get()) < 0) {
        error_ = wl_display_get_error(display_.get());
        if (error_ != 0) {
            return finish();
        }
    }

    wl_display_flush(display_.get());
}

WaylandModule::WaylandModule(fcitx::Instance *instance) : instance_(instance) { openDisplay(""); }

void WaylandModule::openDisplay(const std::string &name) {
    const char *displayString = nullptr;
    if (!name.empty()) {
        displayString = name.c_str();
    }

    try {
        auto iter = conns_.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                   std::forward_as_tuple(this, displayString));
        onConnectionCreated(iter.first->second);
    } catch (const std::exception &e) {
    }
}

void WaylandModule::removeDisplay(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter != conns_.end()) {
        conns_.erase(iter);
    }
}

HandlerTableEntry<WaylandConnectionCreated> *
WaylandModule::addConnectionCreatedCallback(WaylandConnectionCreated callback) {
    auto result = createdCallbacks_.add(callback);

    for (auto &p : conns_) {
        auto &conn = p.second;
        callback(conn.name(), conn.display(), conn.focusGroup());
    }
    return result;
}

HandlerTableEntry<WaylandConnectionClosed> *
WaylandModule::addConnectionClosedCallback(WaylandConnectionClosed callback) {
    return closedCallbacks_.add(callback);
}

void WaylandModule::onConnectionCreated(WaylandConnection &conn) {
    for (auto &callback : createdCallbacks_.view()) {
        callback(conn.name(), conn.display(), conn.focusGroup());
    }
}

void WaylandModule::onConnectionClosed(WaylandConnection &conn) {
    for (auto &callback : closedCallbacks_.view()) {
        callback(conn.name(), conn.display());
    }
}
}
