/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_PHYSICALKEYBOARDWATCHER_PHYSICALKEYBOARDWATCHER_H_
#define _FCITX_MODULES_PHYSICALKEYBOARDWATCHER_PHYSICALKEYBOARDWATCHER_H_

#include <memory>
#include <fcitx/addonmanager.h>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx/addoninstance.h"
#include "fcitx/instance.h"
#include "dbus_public.h"

namespace fcitx {

class PhysicalKeyboardWatcher : public AddonInstance {
public:
    explicit PhysicalKeyboardWatcher(Instance *instance);
    ~PhysicalKeyboardWatcher() override;

private:
    bool isPhysicalKeyboardAvailable();

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    Instance *instance_ = nullptr;
    dbus::Bus *bus_ = nullptr;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<dbus::ServiceWatcherEntry> entry_;
    std::unique_ptr<dbus::Slot> slot_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_PHYSICALKEYBOARDWATCHER_PHYSICALKEYBOARDWATCHER_H_
