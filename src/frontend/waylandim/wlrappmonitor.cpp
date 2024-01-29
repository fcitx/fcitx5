/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "wlrappmonitor.h"
#include <wayland-client-core.h>
#include "zwlr_foreign_toplevel_handle_v1.h"
#include "zwlr_foreign_toplevel_manager_v1.h"

namespace fcitx {
class WlrWindow {
public:
    WlrWindow(WlrAppMonitor *parent,
              wayland::ZwlrForeignToplevelHandleV1 *window)
        : parent_(parent), window_(window),
          key_(std::to_string(wl_proxy_get_id(reinterpret_cast<wl_proxy *>(
              static_cast<zwlr_foreign_toplevel_handle_v1 *>(*window))))) {
        conns_.emplace_back(window->state().connect([this](wl_array *array) {
            pendingActive_ = false;
            size_t size = array->size / sizeof(uint32_t);
            for (size_t i = 0; i < size; ++i) {
                auto entry = static_cast<uint32_t *>(array->data)[i];
                if (entry == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
                    pendingActive_ = true;
                }
            }
        }));
        conns_.emplace_back(window->done().connect([this]() {
            active_ = pendingActive_;
            parent_->refresh();
        }));
        conns_.emplace_back(window->appId().connect([this](const char *appId) {
            appId_ = appId;
            parent_->refresh();
        }));
    }

    const auto &appId() const { return appId_; }
    bool active() const { return active_; }
    const auto &key() { return key_; }

private:
    WlrAppMonitor *parent_;
    bool pendingActive_ = false;
    bool active_ = false;
    std::string appId_;
    std::unique_ptr<wayland::ZwlrForeignToplevelHandleV1> window_;
    std::string key_;
    std::list<ScopedConnection> conns_;
};

WlrAppMonitor::WlrAppMonitor(wayland::Display *display) {
    display->requestGlobals<wayland::ZwlrForeignToplevelManagerV1>();

    globalConn_ = display->globalCreated().connect(
        [this](const std::string &name,
               const std::shared_ptr<void> &management) {
            if (name == wayland::ZwlrForeignToplevelManagerV1::interface) {
                setup(static_cast<wayland::ZwlrForeignToplevelManagerV1 *>(
                    management.get()));
            }
        });

    if (auto management =
            display->getGlobal<wayland::ZwlrForeignToplevelManagerV1>()) {
        setup(management.get());
    }
}

WlrAppMonitor::~WlrAppMonitor() = default;

bool WlrAppMonitor::isAvailable() const { return toplevelConn_.connected(); }

void WlrAppMonitor::setup(wayland::ZwlrForeignToplevelManagerV1 *management) {
    toplevelConn_ = management->toplevel().connect(
        [this](wayland::ZwlrForeignToplevelHandleV1 *handle) {
            windows_[handle] = std::make_unique<WlrWindow>(this, handle);
            handle->closed().connect([this, handle]() { remove(handle); });
        });
}

void WlrAppMonitor::remove(wayland::ZwlrForeignToplevelHandleV1 *handle) {
    windows_.erase(handle);
    refresh();
}

void WlrAppMonitor::refresh() {
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
