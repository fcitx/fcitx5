/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_WAYLAND_CORE_DISPLAY_H_
#define _FCITX_WAYLAND_CORE_DISPLAY_H_

#include "fcitx-utils/signals.h"
#include "outputinformation.h"
#include "wl_registry.h"
#include <list>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <wayland-client.h>

namespace fcitx {
namespace wayland {

class WlOutput;

class GlobalsFactoryBase {
public:
    virtual std::shared_ptr<void> create(WlRegistry &, uint32_t name) = 0;
    void erase(uint32_t name) { globals_.erase(name); }

    const std::set<uint32_t> &globals() { return globals_; }

protected:
    std::set<uint32_t> globals_;
};

template <typename T>
class GlobalsFactory : public GlobalsFactoryBase {
public:
    virtual std::shared_ptr<void> create(WlRegistry &registry, uint32_t name) {
        std::shared_ptr<T> p;
        p.reset(registry.bind<T>(name));
        globals_.insert(name);
        return p;
    }
};

class Display {
public:
    Display(wl_display *display);
    ~Display();

    int fd() const { return wl_display_get_fd(display_.get()); }

    operator wl_display *() { return display_.get(); }

    void roundtrip();
    void flush();
    void run();

    WlRegistry *registry();

    const OutputInfomation *outputInformation(WlOutput *output) const;

    template <typename T>
    std::vector<std::shared_ptr<T>> getGlobals() {
        auto iter = requestedGlobals_.find(T::interface);
        if (iter == requestedGlobals_.end()) {
            return {};
        }
        auto &items = iter->second->globals();
        std::vector<std::shared_ptr<T>> results;
        for (uint32_t item : items) {
            auto iter = globals_.find(item);
            results.push_back(std::static_pointer_cast<T>(
                std::get<std::shared_ptr<void>>(iter->second)));
        }

        return results;
    }

    template <typename T>
    std::shared_ptr<T> getGlobal() {
        auto globals = getGlobals<T>();
        if (globals.size()) {
            return globals[0];
        } else {
            return {};
        }
    }

    template <typename T>
    std::shared_ptr<T> getGlobal(uint32_t name) {
        auto iter = globals_.find(name);
        if (iter != globals_.end() &&
            std::get<std::string>(iter->second) == T::interface) {
            return std::static_pointer_cast<T>(
                std::get<std::shared_ptr<void>>(iter->second));
        }
        return {};
    }

    template <typename T>
    void requestGlobals() {
        auto result = requestedGlobals_.emplace(std::make_pair(
            T::interface, std::make_unique<GlobalsFactory<T>>()));
        if (result.second) {
            auto iter = result.first;
            for (auto &p : globals_) {
                if (std::get<std::string>(p.second) == T::interface) {
                    createGlobalHelper(iter->second.get(), p);
                }
            }
        }
    }

    auto &globalCreated() { return globalCreatedSignal_; }
    auto &globalRemoved() { return globalRemovedSignal_; }

private:
    void createGlobalHelper(
        GlobalsFactoryBase *factory,
        std::pair<const uint32_t,
                  std::tuple<std::string, uint32_t, std::shared_ptr<void>>>
            &globalsPair);

    void addOutput(wayland::WlOutput *output);
    void removeOutput(wayland::WlOutput *output);

    fcitx::Signal<void(const std::string &, std::shared_ptr<void>)>
        globalCreatedSignal_;
    fcitx::Signal<void(const std::string &, std::shared_ptr<void>)>
        globalRemovedSignal_;
    std::unordered_map<std::string, std::unique_ptr<GlobalsFactoryBase>>
        requestedGlobals_;
    std::unique_ptr<wl_display, decltype(&wl_display_disconnect)> display_;
    std::unique_ptr<WlRegistry> registry_;
    std::unordered_map<uint32_t,
                       std::tuple<std::string, uint32_t, std::shared_ptr<void>>>
        globals_;
    std::list<fcitx::Connection> conns_;
    std::unordered_map<WlOutput *, OutputInfomation> outputInfo_;
};
}
}

#endif // _FCITX_WAYLAND_CORE_DISPLAY_H_
