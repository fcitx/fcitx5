//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "display.h"
#include "wl_output.h"
#include "wl_registry.h"
#include <cassert>
#include <cstring>
#include <errno.h>
#include <poll.h>

namespace fcitx {
namespace wayland {

void Display::createGlobalHelper(
    GlobalsFactoryBase *factory,
    std::pair<const uint32_t, std::tuple<std::string, uint32_t, uint32_t,
                                         std::shared_ptr<void>>> &globalsPair) {
    std::get<std::shared_ptr<void>>(globalsPair.second) = factory->create(
        *registry(), globalsPair.first, std::get<2>(globalsPair.second));
    globalCreatedSignal_(std::get<std::string>(globalsPair.second),
                         std::get<std::shared_ptr<void>>(globalsPair.second));
}

Display::Display(wl_display *display)
    : display_(display, &wl_display_disconnect) {
    wl_display_set_user_data(display, this);
    auto reg = registry();
    reg->global().connect(
        [this](uint32_t name, const char *interface, uint32_t version) {
            auto result = globals_.emplace(std::make_pair(
                name, std::make_tuple(interface, name, version, nullptr)));
            auto iter = requestedGlobals_.find(interface);
            if (iter != requestedGlobals_.end()) {
                createGlobalHelper(iter->second.get(), *result.first);
            }
        });
    reg->globalRemove().connect([this](uint32_t name) {
        auto iter = globals_.find(name);
        if (iter != globals_.end()) {
            globalRemovedSignal_(std::get<std::string>(iter->second),
                                 std::get<std::shared_ptr<void>>(iter->second));
            requestedGlobals_[std::get<std::string>(iter->second)]->erase(name);
        }
    });

    requestGlobals<wayland::WlOutput>();
    globalCreatedSignal_.connect(
        [this](const std::string &interface, std::shared_ptr<void> data) {
            if (interface != wayland::WlOutput::interface) {
                return;
            }
            auto output = static_cast<wayland::WlOutput *>(data.get());
            addOutput(output);
        });
    globalRemovedSignal_.connect(
        [this](const std::string &interface, std::shared_ptr<void> data) {
            if (interface != wayland::WlOutput::interface) {
                return;
            }
            auto output = static_cast<wayland::WlOutput *>(data.get());
            removeOutput(output);
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
        registry_ =
            std::make_unique<WlRegistry>(wl_display_get_registry(*this));
    }

    return registry_.get();
}

const OutputInfomation *
Display::outputInformation(wayland::WlOutput *output) const {
    auto iter = outputInfo_.find(output);
    if (iter == outputInfo_.end()) {
        return nullptr;
    }
    return &iter->second;
}

void Display::addOutput(wayland::WlOutput *output) {
    outputInfo_.emplace(std::piecewise_construct, std::forward_as_tuple(output),
                        std::forward_as_tuple(output));
}

void Display::removeOutput(wayland::WlOutput *output) {
    outputInfo_.erase(output);
}
} // namespace wayland
} // namespace fcitx
