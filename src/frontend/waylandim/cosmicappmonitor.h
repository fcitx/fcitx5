/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_COSMICAPPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_COSMICAPPMONITOR_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "fcitx-utils/signals.h"
#include "fcitx-wayland/core/display.h"
#include "appmonitor.h"
#include "zcosmic_toplevel_info_v1.h"

namespace fcitx {
namespace wayland {
class ExtForeignToplevelListV1;
class ExtForeignToplevelHandleV1;
} // namespace wayland
class CosmicWindow;

class CosmicAppMonitor : public AppMonitor {
public:
    CosmicAppMonitor(wayland::Display *display);
    ~CosmicAppMonitor() override;

    bool isAvailable() const override;

    void setExtForeignToplevelList(wayland::ExtForeignToplevelListV1 *list);
    void setCosmicToplevelInfo(wayland::ZcosmicToplevelInfoV1 *info);

    void remove(wayland::ExtForeignToplevelHandleV1 *handle);
    void refresh();

    auto *info() const { return info_; }

private:
    void setup();
    ScopedConnection globalConn_;
    ScopedConnection toplevelConn_;
    std::unordered_map<wayland::ExtForeignToplevelHandleV1 *,
                       std::unique_ptr<CosmicWindow>>
        windows_;
    std::unordered_map<std::string, uint32_t> appState_;
    wayland::ZcosmicToplevelInfoV1 *info_ = nullptr;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_COSMICAPPMONITOR_H_
