/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "display.h"
#include <errno.h>
#include <poll.h>
#include <cassert>
#include <cstring>
#include <fcitx-utils/log.h>
#include "wl_callback.h"
#include "wl_output.h"
#include "wl_registry.h"

namespace fcitx::wayland {

void Display::createGlobalHelper(
    GlobalsFactoryBase *factory,
    std::pair<const uint32_t, std::tuple<std::string, uint32_t, uint32_t,
                                         std::shared_ptr<void>>> &globalsPair) {
    std::get<std::shared_ptr<void>>(globalsPair.second) = factory->create(
        *registry(), globalsPair.first, std::get<2>(globalsPair.second));

    globalCreatedSignal_(std::get<std::string>(globalsPair.second),
                         std::get<std::shared_ptr<void>>(globalsPair.second));
    sync();
    flush();
}

Display::Display(wl_display *display) : display_(display) {
    wl_display_set_user_data(display, this);
    auto *reg = registry();
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
            const auto &interface = std::get<std::string>(iter->second);
            auto localGlobalIter = requestedGlobals_.find(interface);
            if (localGlobalIter != requestedGlobals_.end()) {
                localGlobalIter->second->erase(name);
            }
            globals_.erase(iter);
        }
    });

    requestGlobals<wayland::WlOutput>();
    globalCreatedSignal_.connect([this](const std::string &interface,
                                        const std::shared_ptr<void> &data) {
        if (interface != wayland::WlOutput::interface) {
            return;
        }
        auto *output = static_cast<wayland::WlOutput *>(data.get());
        addOutput(output);
    });
    globalRemovedSignal_.connect([this](const std::string &interface,
                                        const std::shared_ptr<void> &data) {
        if (interface != wayland::WlOutput::interface) {
            return;
        }
        auto *output = static_cast<wayland::WlOutput *>(data.get());
        removeOutput(output);
    });

    sync();
    flush();
}

Display::~Display() {}

void Display::roundtrip() { wl_display_roundtrip(*this); }

void Display::sync() {
    callbacks_.emplace_back(
        std::make_unique<WlCallback>(wl_display_sync(*this)));
    callbacks_.back()->done().connect(
        [this, iter = std::prev(callbacks_.end())](uint32_t) {
            callbacks_.erase(iter);
        });
}

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
} // namespace fcitx::wayland
