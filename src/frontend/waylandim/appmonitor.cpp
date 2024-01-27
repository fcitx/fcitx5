/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "appmonitor.h"
#include <algorithm>

namespace fcitx {

AggregatedAppMonitor::AggregatedAppMonitor() = default;

bool AggregatedAppMonitor::isAvailable() const {
    return std::any_of(
        subMonitors_.begin(), subMonitors_.end(),
        [](const auto &monitor) { return monitor->isAvailable(); });
}

AppMonitor *AggregatedAppMonitor::activeMonitor() const {
    auto iter = std::find_if(
        subMonitors_.begin(), subMonitors_.end(),
        [](const auto &subMonitor) { return subMonitor->isAvailable(); });
    return iter == subMonitors_.end() ? nullptr : iter->get();
}

void AggregatedAppMonitor::addSubMonitor(std::unique_ptr<AppMonitor> monitor) {
    subMonitors_.emplace_back(std::move(monitor));

    subMonitors_.back()->appUpdated.connect(
        [this, monitor = subMonitors_.back().get()](
            const std::unordered_map<std::string, std::string> &appState,
            const std::optional<std::string> &focus) {
            if (activeMonitor() == monitor) {
                appUpdated(appState, focus);
            }
        });
}

} // namespace fcitx