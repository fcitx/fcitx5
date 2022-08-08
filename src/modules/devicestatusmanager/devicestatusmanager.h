/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_DEVICESTATUSMANAGER_DEVICESTATUSMANAGER_H_
#define _FCITX_MODULES_DEVICESTATUSMANAGER_DEVICESTATUSMANAGER_H_

#include "fcitx/addoninstance.h"
#include "fcitx/instance.h"

#include "devicemodewatcher.h"
#include "physicalkeyboardwatcher.h"

namespace fcitx {

class DeviceStatusManager : public AddonInstance {
public:
    explicit DeviceStatusManager(Instance *instance);
    ~DeviceStatusManager() override;

    void updateVirtualKeyboardAutoHide();

private:
    Instance *instance_;

    DeviceModeWatcher deviceModeWatcher_;

    PhysicalKeyboardWatcher physicalKeyboardWatcher_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_DEVICESTATUSMANAGER_DEVICESTATUSMANAGER_H_
