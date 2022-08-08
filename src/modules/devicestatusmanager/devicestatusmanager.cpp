/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "devicestatusmanager.h"
#include "fcitx/addonfactory.h"

namespace fcitx {

DeviceStatusManager::DeviceStatusManager(Instance *instance)
    : instance_(instance), deviceModeWatcher_(instance, this),
      physicalKeyboardWatcher_(instance, this) {}

DeviceStatusManager::~DeviceStatusManager() = default;

void DeviceStatusManager::updateVirtualKeyboardAutoHide() {
    instance_->setVirtualKeyboardAutoHide(
        deviceModeWatcher_.isTabletMode() ||
        !physicalKeyboardWatcher_.isPhysicalKeyboardAvailable());
}

class DeviceStatusManagerFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new DeviceStatusManager(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::DeviceStatusManagerFactory)
