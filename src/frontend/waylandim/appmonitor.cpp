/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "appmonitor.h"
#include "fcitx/misc_p.h"
#include "display.h"
#include "plasmaappmonitor.h"

fcitx::AppMonitor<std::string> *
fcitx::getAppMonitor(wayland::Display *display) {
    if (getDesktopType() == DesktopType::KDE5) {
        return new PlasmaAppMonitor(display);
    }
    return nullptr;
}