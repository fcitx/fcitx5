/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "physicalkeyboardwatcher.h"
#include "fcitx/addonfactory.h"

namespace fcitx {

PhysicalKeyboardWatcher::PhysicalKeyboardWatcher(Instance *instance)
    : instance_(instance), bus_(dbus()->call<IDBusModule::bus>()),
      watcher_(*bus_) {
    entry_ = watcher_.watchService(
        "com.kylin.statusmanager.interface",
        [this](const std::string &, const std::string &,
               const std::string &newOwner) {
            FCITX_INFO() << "PhysicalKeyboardWatcher new owner: " << newOwner;
            bool available = true;
            if (!newOwner.empty()) {
                available = isPhysicalKeyboardAvailable();
            }

            instance_->setPhysicalKeyboardAvailable(available);
        });

    slot_ =
        bus_->addMatch(dbus::MatchRule("com.kylin.statusmanager.interface", "/",
                                       "com.kylin.statusmanager.interface",
                                       "inputmethod_change_signal"),
                       [this](dbus::Message &msg) {
                           bool available = true;
                           msg >> available;

                           instance_->setPhysicalKeyboardAvailable(available);

                           return true;
                       });
}

PhysicalKeyboardWatcher::~PhysicalKeyboardWatcher() = default;

bool PhysicalKeyboardWatcher::isPhysicalKeyboardAvailable() {
    auto msg = bus_->createMethodCall("com.kylin.statusmanager.interface", "/",
                                      "com.kylin.statusmanager.interface",
                                      "get_inputmethod_mode");

    auto reply = msg.call(0);
    bool available = true;
    reply >> available;

    return available;
}

class PhysicalKeyboardWatcherFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new PhysicalKeyboardWatcher(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::PhysicalKeyboardWatcherFactory)
