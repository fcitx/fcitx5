#include "waylandcursortheme.h"
#include <charconv>
#include <cstdlib>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <wayland-cursor.h>
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/environ.h"
#include "fcitx-utils/misc_p.h"
#include "dbus_public.h"
#include "portalsettingmonitor.h"
#include "waylandui.h"
#include "wl_shm.h"

namespace fcitx::classicui {

WaylandCursorTheme::WaylandCursorTheme(WaylandUI *ui)
    : shm_(ui->display()->getGlobal<wayland::WlShm>()) {

    if (auto size = getEnvironment("XCURSOR_SIZE")) {
        unsigned int cursorSize = 0;
        if (std::from_chars(size->data(), size->data() + size->size(),
                            cursorSize)
                .ec == std::errc()) {
            setCursorSize(cursorSize);
        }
    }

    if (auto theme = getEnvironment("XCURSOR_THEME")) {
        setTheme(*theme);
    } else {
        setTheme({});
    }

#ifdef ENABLE_DBUS
    if (auto *dbusAddon = ui->parent()->dbus()) {
        settingMonitor_ = std::make_unique<PortalSettingMonitor>(
            *dbusAddon->call<IDBusModule::bus>());
        cursorSizeWatcher_ =
            settingMonitor_->watch("org.gnome.desktop.interface", "cursor-size",
                                   [this](const dbus::Variant &value) {
                                       if (value.signature() == "i") {
                                           setCursorSize(value.dataAs<int>());
                                       }
                                   });
        cursorThemeWatcher_ = settingMonitor_->watch(
            "org.gnome.desktop.interface", "cursor-theme",
            [this](const dbus::Variant &value) {
                if (value.signature() == "s") {
                    setTheme(value.dataAs<std::string>());
                }
            });
    }
#endif
}

void WaylandCursorTheme::setCursorSize(int cursorSize) {
    // Add some simple validation.
    int newCursorSize = 24;
    if (cursorSize > 0 && cursorSize < 2048) {
        newCursorSize = cursorSize;
    }

    if (newCursorSize == cursorSize_) {
        return;
    }
    cursorSize_ = newCursorSize;
    themes_.clear();
    themeChangedSignal_();
}

void WaylandCursorTheme::setTheme(const std::string &theme) {
    themes_.clear();
    themeName_ = theme;
    themeChangedSignal_();
}

WaylandCursorInfo WaylandCursorTheme::loadCursorTheme(int scale) {
    auto size = cursorSize_ * scale;
    if (auto *theme = findValue(themes_, size)) {
        return *theme;
    }
    WaylandCursorInfo info;
    info.theme = std::shared_ptr<wl_cursor_theme>(
        wl_cursor_theme_load(themeName_.empty() ? nullptr : themeName_.data(),
                             size, *shm_),
        wl_cursor_theme_destroy);
    if (info.theme) {
        info.cursor = wl_cursor_theme_get_cursor(info.theme.get(), "default");
        if (!info.cursor) {
            info.cursor =
                wl_cursor_theme_get_cursor(info.theme.get(), "left_ptr");
        }
    }

    auto &themePtr = themes_[size] = std::move(info);
    return themePtr;
}

} // namespace fcitx::classicui
