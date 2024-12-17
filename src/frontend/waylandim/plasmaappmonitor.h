/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_PLASMAAPPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_PLASMAAPPMONITOR_H_

#include <memory>
#include <unordered_map>
#include "fcitx-utils/signals.h"
#include "fcitx-wayland/core/display.h"
#include "appmonitor.h"

namespace fcitx {
namespace wayland {
class OrgKdePlasmaWindowManagement;
class OrgKdePlasmaWindow;
} // namespace wayland
class PlasmaWindow;

class PlasmaAppMonitor : public AppMonitor {
public:
    PlasmaAppMonitor(wayland::Display *display);
    ~PlasmaAppMonitor() override;

    void setup(wayland::OrgKdePlasmaWindowManagement *management);
    void remove(wayland::OrgKdePlasmaWindow *window);
    void refresh();
    bool isAvailable() const override;

private:
    ScopedConnection globalConn_;
    ScopedConnection windowConn_;
    std::unordered_map<wayland::OrgKdePlasmaWindow *,
                       std::unique_ptr<PlasmaWindow>>
        windows_;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_PLASMAAPPMONITOR_H_
