/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDCURSORTHEME_H_
#define _FCITX_UI_CLASSIC_WAYLANDCURSORTHEME_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <wayland-cursor.h>
#include "fcitx-utils/signals.h"
#include "config.h"
#include "portalsettingmonitor.h"
#include "wl_shm.h"

namespace fcitx::classicui {
class WaylandUI;

struct WaylandCursorInfo {
    std::shared_ptr<wl_cursor_theme> theme;
    wl_cursor *cursor = nullptr;
};

class WaylandCursorTheme {
public:
    WaylandCursorTheme(WaylandUI *ui);
    void setTheme(const std::string &theme);

    auto &themeChanged() const { return themeChangedSignal_; }

    WaylandCursorInfo loadCursorTheme(int scale);
    auto cursorSize() const { return cursorSize_; }

private:
    void setCursorSize(int cursorSize);
    void timerCallback();
    void frameCallback();

    fcitx::Signal<void()> themeChangedSignal_;

    std::shared_ptr<wayland::WlShm> shm_;
    // Size to theme map;
    std::unordered_map<int, WaylandCursorInfo> themes_;
    int cursorSize_ = 24;
    std::string themeName_;
#ifdef ENABLE_DBUS
    std::unique_ptr<PortalSettingMonitor> settingMonitor_;
    std::unique_ptr<PortalSettingEntry> cursorSizeWatcher_, cursorThemeWatcher_;
#endif
};

} // namespace fcitx::classicui

#endif