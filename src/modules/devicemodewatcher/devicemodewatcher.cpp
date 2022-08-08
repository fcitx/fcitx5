/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx/addonfactory.h"
#include "devicemodewatcher.h"

namespace fcitx {

DeviceModeWatcher::DeviceModeWatcher(Instance *instance)
    : instance_(instance), bus_(dbus()->call<IDBusModule::bus>()),
      watcher_(*bus_) {
    entry_ = watcher_.watchService(
        "com.kylin.statusmanager.interface",
        [this](const std::string &, const std::string &,
               const std::string &newOwner) {
            FCITX_INFO() << "DeviceModeWatcher new owner: " << newOwner;
            bool tabletMode = false;
            if (!newOwner.empty()) {
                tabletMode = isTabletMode();
            }

            auto deviceMode = tabletMode ? DeviceMode::Tablet : DeviceMode::PC;
            instance_->setDeviceMode(deviceMode);
        });

    slot_ =
        bus_->addMatch(dbus::MatchRule("com.kylin.statusmanager.interface", "/",
                                       "com.kylin.statusmanager.interface",
                                       "mode_change_signal"),
                       [this](dbus::Message &msg) {
                           bool tabletMode = false;
                           msg >> tabletMode;

                           auto deviceMode =
                               tabletMode ? DeviceMode::Tablet : DeviceMode::PC;
                           instance_->setDeviceMode(deviceMode);

                           return true;
                       });
}

DeviceModeWatcher::~DeviceModeWatcher() = default;

bool DeviceModeWatcher::isTabletMode() {
    auto msg = bus_->createMethodCall("com.kylin.statusmanager.interface", "/",
                                      "com.kylin.statusmanager.interface",
                                      "get_current_tabletmode");
    auto reply = msg.call(0);
    bool tabletMode = true;
    reply >> tabletMode;

    return tabletMode;
}

class DeviceModeWatcherFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new DeviceModeWatcher(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::DeviceModeWatcherFactory)
