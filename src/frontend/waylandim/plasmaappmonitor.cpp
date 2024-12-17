/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "plasmaappmonitor.h"
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include "fcitx-utils/signals.h"
#include "display.h"
#include "plasma-window-management/org_kde_plasma_window.h"
#include "plasma-window-management/org_kde_plasma_window_management.h"

namespace fcitx {
class PlasmaWindow {
public:
    PlasmaWindow(PlasmaAppMonitor *parent, wayland::OrgKdePlasmaWindow *window,
                 const char *uuid)
        : parent_(parent), window_(window), uuid_(uuid) {
        conns_.emplace_back(
            window->stateChanged().connect([this](uint32_t state) {
                active_ =
                    (state & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
                parent_->refresh();
            }));
        conns_.emplace_back(
            window->appIdChanged().connect([this](const char *appId) {
                appId_ = appId;
                parent_->refresh();
            }));
    }

    const auto &appId() const { return appId_; }
    bool active() const { return active_; }
    const auto &key() { return uuid_; }

private:
    PlasmaAppMonitor *parent_;
    std::unique_ptr<wayland::OrgKdePlasmaWindow> window_;
    std::string uuid_;
    bool active_ = false;
    std::string appId_;
    std::list<ScopedConnection> conns_;
};

PlasmaAppMonitor::PlasmaAppMonitor(wayland::Display *display) {
    display->requestGlobals<wayland::OrgKdePlasmaWindowManagement>();

    globalConn_ = display->globalCreated().connect(
        [this](const std::string &name,
               const std::shared_ptr<void> &management) {
            if (name == wayland::OrgKdePlasmaWindowManagement::interface) {
                setup(static_cast<wayland::OrgKdePlasmaWindowManagement *>(
                    management.get()));
            }
        });

    if (auto management =
            display->getGlobal<wayland::OrgKdePlasmaWindowManagement>()) {
        setup(management.get());
    }
}

PlasmaAppMonitor::~PlasmaAppMonitor() = default;

bool PlasmaAppMonitor::isAvailable() const { return windowConn_.connected(); }

void PlasmaAppMonitor::setup(
    wayland::OrgKdePlasmaWindowManagement *management) {
    windowConn_ = management->windowWithUuid().connect(
        [this, management](uint32_t, const char *uuid) {
            auto *window = management->getWindowByUuid(uuid);
            windows_[window] =
                std::make_unique<PlasmaWindow>(this, window, uuid);
            window->unmapped().connect([this, window]() { remove(window); });
        });
}

void PlasmaAppMonitor::remove(wayland::OrgKdePlasmaWindow *window) {
    windows_.erase(window);
    refresh();
}

void PlasmaAppMonitor::refresh() {
    std::unordered_map<std::string, std::string> state;
    std::optional<std::string> focus;
    for (const auto &[_, plasmaWindow] : windows_) {
        if (!plasmaWindow->appId().empty()) {
            auto iter =
                state.emplace(plasmaWindow->key(), plasmaWindow->appId());
            if (plasmaWindow->active() && !focus && iter.second) {
                focus = iter.first->first;
            }
        }
    }
    appUpdated(state, focus);
}

} // namespace fcitx