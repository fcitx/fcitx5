/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "devicemodewatcher.h"
#include "devicestatusmanager.h"

namespace fcitx {

DeviceModeWatcher::DeviceModeWatcher(Instance *instance,
                                     DeviceStatusManager *parent)
    : instance_(instance), parent_(parent),
      bus_(dbus()->call<IDBusModule::bus>()), watcher_(*bus_) {
    entry_ = watcher_.watchService(
        "com.kylin.statusmanager.interface",
        [this](const std::string &, const std::string &,
               const std::string &newOwner) {
            FCITX_INFO() << "DeviceModeWatcher new owner: " << newOwner;
            bool tabletMode = false;
            if (!newOwner.empty()) {
                tabletMode = isTabletModeDBus();
            }

            setTabletMode(tabletMode);
        });

    slot_ =
        bus_->addMatch(dbus::MatchRule("com.kylin.statusmanager.interface", "/",
                                       "com.kylin.statusmanager.interface",
                                       "mode_change_signal"),
                       [this](dbus::Message &msg) {
                           bool tabletMode = false;
                           msg >> tabletMode;

                           setTabletMode(tabletMode);

                           return true;
                       });
}

DeviceModeWatcher::~DeviceModeWatcher() = default;

bool DeviceModeWatcher::isTabletMode() const { return isTabletMode_; }

void DeviceModeWatcher::setTabletMode(bool tabletMode) {
    if (isTabletMode_ == tabletMode) {
        return;
    }

    isTabletMode_ = tabletMode;

    parent_->updateVirtualKeyboardAutoHide();
}

bool DeviceModeWatcher::isTabletModeDBus() {
    auto msg = bus_->createMethodCall("com.kylin.statusmanager.interface", "/",
                                      "com.kylin.statusmanager.interface",
                                      "get_current_tabletmode");
    auto reply = msg.call(0);
    bool tabletMode = true;
    reply >> tabletMode;

    return tabletMode;
}

} // namespace fcitx