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

#include "display.h"
#include "wl_registry.h"
#include <cassert>
#include <poll.h>

namespace fcitx {
namespace wayland {

void Display::createGlobalHelper(
    GlobalsFactoryBase *factory,
    std::pair<const uint32_t, std::tuple<std::string, uint32_t, std::shared_ptr<void>>> &globalsPair) {
    std::get<std::shared_ptr<void>>(globalsPair.second) = factory->create(*registry(), globalsPair.first);
}

Display::Display(wl_display *display) : display_(display, &wl_display_disconnect) {
    wl_display_set_user_data(display, this);
    auto reg = registry();
    reg->global().connect([this](uint32_t name, const char *interface, uint32_t) {
        auto result = globals_.emplace(std::make_pair(name, std::make_tuple(interface, name, nullptr)));
        auto iter = requestedGlobals_.find(interface);
        if (iter != requestedGlobals_.end()) {
            createGlobalHelper(iter->second.get(), *result.first);
        }
    });
    reg->globalRemove().connect([this](uint32_t name) {
        auto iter = globals_.find(name);
        if (iter != globals_.end()) {
            requestedGlobals_[std::get<std::string>(iter->second)]->erase(name);
        }
    });

    wl_display_roundtrip(*this);
}

Display::~Display() {}

void Display::roundtrip() { wl_display_roundtrip(*this); }

void Display::flush() { wl_display_flush(*this); }

void Display::run() {
    pollfd pfd;

    pfd.fd = fd();
    pfd.events = POLLIN | POLLERR | POLLHUP;

    while (1) {
        wl_display_dispatch_pending(*this);
        auto ret = wl_display_flush(*this);
        if (ret < 0 && errno != EAGAIN) {
            break;
        }

        auto count = poll(&pfd, 1, -1);
        if (count < 0 && errno != EINTR) {
            break;
        }

        if (count == 1) {
            auto event = pfd.revents;
            // We can have cases where POLLIN and POLLHUP are both set for
            // example. Don't break if both flags are set.
            if ((event & POLLERR || event & POLLHUP) && !(event & POLLIN)) {
                break;
            }

            if (event & POLLIN) {
                ret = wl_display_dispatch(*this);
                if (ret == -1) {
                    break;
                }
            }
        }
    }
}

WlRegistry *Display::registry() {
    if (!registry_) {
        registry_ = std::make_unique<WlRegistry>(wl_display_get_registry(*this));
    }

    return registry_.get();
}
}
}
