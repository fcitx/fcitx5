/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "wlrappmonitor.h"
#include "fcitx/addoninstance.h"
#include "wayland_public.h"
#include "zwlr_foreign_toplevel_handle_v1.h"
#include "zwlr_foreign_toplevel_manager_v1.h"

namespace fcitx {
class WlrWindow {
public:
    WlrWindow(WlrAppMonitor *parent,
              wayland::ZwlrForeignToplevelHandleV1 *window)
        : parent_(parent), window_(window) {
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

private:
    WlrAppMonitor *parent_;
    bool pendingActive_ = false;
    bool active_ = false;
    std::string appId_;
    std::unique_ptr<wayland::ZwlrForeignToplevelHandleV1> window_;
    std::list<fcitx::ScopedConnection> conns_;
};

} // namespace fcitx

fcitx::WlrAppMonitor::WlrAppMonitor(wayland::Display *display) {
    FCITX_INFO() << "WLR";
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

void fcitx::WlrAppMonitor::setup(
    wayland::ZwlrForeignToplevelManagerV1 *management) {
    FCITX_INFO() << "SETUP";
    management->toplevel().connect(
        [this](fcitx::wayland::ZwlrForeignToplevelHandleV1 *handle) {
            windows_[handle] = std::make_unique<WlrWindow>(this, handle);
            handle->closed().connect([this, handle]() { remove(handle); });
        });
}

void fcitx::WlrAppMonitor::remove(
    wayland::ZwlrForeignToplevelHandleV1 *handle) {
    windows_.erase(handle);
}

void fcitx::WlrAppMonitor::refresh() {
    appState_.clear();
    for (const auto &[_, wlrWindow] : windows_) {

        if (!wlrWindow->appId().empty()) {
            auto &state = appState_[wlrWindow->appId()];
            if (state == 2) {
                continue;
            }
            state = wlrWindow->active() ? 2 : 1;
        }
    }
    FCITX_INFO() << appState_;
}
