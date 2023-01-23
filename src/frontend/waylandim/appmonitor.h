/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_

#include <optional>
#include <unordered_set>
#include "fcitx-utils/signals.h"

namespace fcitx {

namespace wayland {
class Display;
}

template <typename AppKey>
class AppMonitor {
public:
    Signal<void(const std::unordered_map<AppKey, std::string> &appState,
                const std::optional<AppKey> &focus)>
        appUpdated;
};

AppMonitor<std::string> *getAppMonitor(wayland::Display *display);

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_
