/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_WLRAPPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WLRAPPMONITOR_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "fcitx-utils/signals.h"
#include "fcitx-wayland/core/display.h"
#include "appmonitor.h"

namespace fcitx {
namespace wayland {
class ZwlrForeignToplevelManagerV1;
class ZwlrForeignToplevelHandleV1;
} // namespace wayland
class WlrWindow;

class WlrAppMonitor : public AppMonitor {
public:
    WlrAppMonitor(wayland::Display *display);
    ~WlrAppMonitor() override;

    bool isAvailable() const override;

    void setup(wayland::ZwlrForeignToplevelManagerV1 *management);
    void remove(wayland::ZwlrForeignToplevelHandleV1 *handle);
    void refresh();

private:
    ScopedConnection globalConn_;
    ScopedConnection toplevelConn_;
    std::unordered_map<wayland::ZwlrForeignToplevelHandleV1 *,
                       std::unique_ptr<WlrWindow>>
        windows_;
    std::unordered_map<std::string, uint32_t> appState_;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_WLRAPPMONITOR_H_
