/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "cosmicappmonitor.h"
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <wayland-client-core.h>
#include <wayland-util.h>
#include "fcitx-utils/signals.h"
#include "display.h"
#include "ext_foreign_toplevel_handle_v1.h"
#include "ext_foreign_toplevel_list_v1.h"
#include "zcosmic_toplevel_handle_v1.h"
#include "zcosmic_toplevel_info_v1.h"

namespace fcitx {
class CosmicWindow {
public:
    CosmicWindow(CosmicAppMonitor *parent,
                 wayland::ExtForeignToplevelHandleV1 *window)
        : parent_(parent), window_(window),
          key_(std::to_string(wl_proxy_get_id(reinterpret_cast<wl_proxy *>(
              static_cast<ext_foreign_toplevel_handle_v1 *>(*window))))) {
        if (parent->info()) {
            setup(parent->info());
        }
    }

    const auto &appId() const { return appId_; }
    bool active() const { return active_; }
    const auto &key() { return key_; }

    void setup(wayland::ZcosmicToplevelInfoV1 *info) {
        if (setup_) {
            return;
        }
        cosmicWindow_.reset(info->getCosmicToplevel(window_.get()));
        conns_.emplace_back(
            cosmicWindow_->state().connect([this](wl_array *array) {
                pendingActive_ = false;
                size_t size = array->size / sizeof(uint32_t);
                for (size_t i = 0; i < size; ++i) {
                    auto entry = static_cast<uint32_t *>(array->data)[i];
                    if (entry == ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
                        pendingActive_ = true;
                    }
                }
            }));
        conns_.emplace_back(window_->done().connect([this]() {
            if (active_ != pendingActive_) {
                active_ = pendingActive_;
                parent_->refresh();
            }
        }));
        conns_.emplace_back(window_->appId().connect([this](const char *appId) {
            if (appId_ != appId) {
                appId_ = appId;
                parent_->refresh();
            }
        }));
        setup_ = true;
    }

private:
    bool setup_ = false;
    CosmicAppMonitor *parent_;
    bool pendingActive_ = false;
    bool active_ = false;
    std::string appId_;
    std::unique_ptr<wayland::ExtForeignToplevelHandleV1> window_;
    std::unique_ptr<wayland::ZcosmicToplevelHandleV1> cosmicWindow_;
    std::string key_;
    std::list<ScopedConnection> conns_;
};

CosmicAppMonitor::CosmicAppMonitor(wayland::Display *display) {
    display->requestGlobals<wayland::ExtForeignToplevelListV1>();
    display->requestGlobalsWithMinimalVersion<wayland::ZcosmicToplevelInfoV1>(
        2);

    globalConn_ = display->globalCreated().connect(
        [this](const std::string &name, const std::shared_ptr<void> &global) {
            if (name == wayland::ExtForeignToplevelListV1::interface) {
                setExtForeignToplevelList(
                    static_cast<wayland::ExtForeignToplevelListV1 *>(
                        global.get()));
            } else if (name == wayland::ZcosmicToplevelInfoV1::interface) {
                setCosmicToplevelInfo(
                    static_cast<wayland::ZcosmicToplevelInfoV1 *>(
                        global.get()));
            }
        });

    if (auto list = display->getGlobal<wayland::ExtForeignToplevelListV1>()) {
        setExtForeignToplevelList(list.get());
    }
    if (auto info = display->getGlobal<wayland::ZcosmicToplevelInfoV1>()) {
        setCosmicToplevelInfo(info.get());
    }
}

CosmicAppMonitor::~CosmicAppMonitor() = default;

bool CosmicAppMonitor::isAvailable() const {
    return toplevelConn_.connected() && info_;
}

void CosmicAppMonitor::setExtForeignToplevelList(
    wayland::ExtForeignToplevelListV1 *list) {
    toplevelConn_ = list->toplevel().connect(
        [this](wayland::ExtForeignToplevelHandleV1 *handle) {
            windows_[handle] = std::make_unique<CosmicWindow>(this, handle);
            handle->closed().connect([this, handle]() { remove(handle); });
        });
}

void CosmicAppMonitor::setCosmicToplevelInfo(
    wayland::ZcosmicToplevelInfoV1 *info) {
    info_ = info;
    for (auto &[_, window] : windows_) {
        window->setup(info_);
    }
}

void CosmicAppMonitor::remove(wayland::ExtForeignToplevelHandleV1 *handle) {
    windows_.erase(handle);
    refresh();
}

void CosmicAppMonitor::refresh() {
    std::unordered_map<std::string, std::string> state;
    std::optional<std::string> focus;
    for (const auto &[_, wlrWindow] : windows_) {
        if (!wlrWindow->appId().empty()) {
            auto iter = state.emplace(wlrWindow->key(), wlrWindow->appId());
            if (wlrWindow->active() && !focus && iter.second) {
                focus = iter.first->first;
            }
        }
    }
    appUpdated(state, focus);
}

} // namespace fcitx
