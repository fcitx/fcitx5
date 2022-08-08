/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_DEVICESTATUSMANAGER_DEVICEMODEWATCHER_H_
#define _FCITX_MODULES_DEVICESTATUSMANAGER_DEVICEMODEWATCHER_H_

#include <memory>
#include <fcitx/addonmanager.h>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx/instance.h"
#include "dbus_public.h"

namespace fcitx {

class DeviceStatusManager;

class DeviceModeWatcher {
public:
    DeviceModeWatcher(Instance *instance, DeviceStatusManager *parent);
    ~DeviceModeWatcher();

    bool isTabletMode() const;

private:
    void setTabletMode(bool tabletMode);

    bool isTabletModeDBus();

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    Instance *instance_ = nullptr;
    DeviceStatusManager *parent_ = nullptr;
    bool isTabletMode_ = false;
    dbus::Bus *bus_ = nullptr;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<dbus::ServiceWatcherEntry> entry_;
    std::unique_ptr<dbus::Slot> slot_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_DEVICESTATUSMANAGER_DEVICEMODEWATCHER_H_
