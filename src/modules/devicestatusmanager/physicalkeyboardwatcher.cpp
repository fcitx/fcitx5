/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "physicalkeyboardwatcher.h"
#include "devicestatusmanager.h"

namespace fcitx {

PhysicalKeyboardWatcher::PhysicalKeyboardWatcher(Instance *instance,
                                                 DeviceStatusManager *parent)
    : instance_(instance), parent_(parent),
      bus_(dbus()->call<IDBusModule::bus>()), watcher_(*bus_) {
    entry_ = watcher_.watchService(
        "com.kylin.statusmanager.interface",
        [this](const std::string &, const std::string &,
               const std::string &newOwner) {
            FCITX_INFO() << "PhysicalKeyboardWatcher new owner: " << newOwner;
            bool available = true;
            if (!newOwner.empty()) {
                available = isPhysicalKeyboardAvailableDBus();
            }

            setPhysicalKeyboardAvailable(available);
        });

    slot_ =
        bus_->addMatch(dbus::MatchRule("com.kylin.statusmanager.interface", "/",
                                       "com.kylin.statusmanager.interface",
                                       "inputmethod_change_signal"),
                       [this](dbus::Message &msg) {
                           bool available = true;
                           msg >> available;

                           setPhysicalKeyboardAvailable(available);

                           return true;
                       });
}

PhysicalKeyboardWatcher::~PhysicalKeyboardWatcher() = default;

bool PhysicalKeyboardWatcher::isPhysicalKeyboardAvailable() const {
    return isPhysicalKeyboardAvailable_;
}

void PhysicalKeyboardWatcher::setPhysicalKeyboardAvailable(bool available) {
    if (isPhysicalKeyboardAvailable_ == available) {
        return;
    }

    isPhysicalKeyboardAvailable_ = available;
    if (isPhysicalKeyboardAvailable_) {
        instance_->setInputMethodMode(InputMethodMode::PhysicalKeyboard);
    } else {
        instance_->setInputMethodMode(InputMethodMode::OnScreenKeyboard);
    }

    instance_->setVirtualKeyboardAutoShow(!isPhysicalKeyboardAvailable_);

    parent_->updateVirtualKeyboardAutoHide();
}

bool PhysicalKeyboardWatcher::isPhysicalKeyboardAvailableDBus() {
    auto msg = bus_->createMethodCall("com.kylin.statusmanager.interface", "/",
                                      "com.kylin.statusmanager.interface",
                                      "get_inputmethod_mode");

    auto reply = msg.call(0);
    bool available = true;
    reply >> available;

    return available;
}

} // namespace fcitx