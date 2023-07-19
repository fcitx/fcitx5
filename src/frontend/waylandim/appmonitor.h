/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_

#include <optional>
#include <string>
#include <unordered_set>
#include "fcitx-utils/signals.h"

namespace fcitx {

namespace wayland {
class Display;
}

class AppMonitor {
public:
    virtual ~AppMonitor() = default;
    Signal<void(const std::unordered_map<std::string, std::string> &appState,
                const std::optional<std::string> &focus)>
        appUpdated;

    virtual bool isAvailable() const = 0;
};

class AggregatedAppMonitor : public AppMonitor {
public:
    AggregatedAppMonitor();

    void addSubMonitor(std::unique_ptr<AppMonitor> monitor);
    bool isAvailable() const override;
    AppMonitor *activeMonitor() const;

private:
    std::vector<std::unique_ptr<AppMonitor>> subMonitors_;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_
