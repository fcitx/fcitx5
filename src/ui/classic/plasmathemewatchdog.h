/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_UI_CLASSIC_PLASMATHEMEWATCHDOG_H_
#define _FCITX5_UI_CLASSIC_PLASMATHEMEWATCHDOG_H_

#include <functional>
#include "fcitx-utils/event.h"
#include "fcitx-utils/unixfd.h"

#define PLASMA_THEME_GENERATOR "fcitx5-plasma-theme-generator"

namespace fcitx::classicui {

class PlasmaThemeWatchdog {
public:
    PlasmaThemeWatchdog(EventLoop *event, std::function<void()> callback);

    ~PlasmaThemeWatchdog();

private:
    void cleanup();
    std::function<void()> callback_;
    UnixFD monitorFD_;
    std::unique_ptr<EventSourceIO> ioEvent_;
    std::unique_ptr<EventSourceTime> timerEvent_;
    pid_t generator_ = 0;
};

} // namespace fcitx::classicui

#endif // _FCITX5_UI_CLASSIC_PLASMATHEMEWATCHDOG_H_
