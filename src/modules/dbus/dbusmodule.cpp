/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "dbusmodule.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx/addonmanager.h"

#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx5"
#define FCITX_CONTROLLER_DBUS_INTERFACE "org.fcitx.Fcitx.Controller1"

using namespace fcitx::dbus;

namespace fcitx {

class Controller1 : public ObjectVTable {
public:
    Controller1(Instance *instance) : instance_(instance) {}

    void exit() { instance_->exit(); }

    void restart() {
        auto instance = instance_;
        instance_->eventLoop().addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
                                            [instance](EventSource *, uint64_t) {
                                                instance->restart();
                                                return false;
                                            });
    }
    void configure() { instance_->configure(); }
    void configureAddon(const std::string &addon) { instance_->configureAddon(addon); }
    void configureInputMethod(const std::string &imName) { instance_->configureInputMethod(imName); }
    std::string currentUI() { return instance_->currentUI(); }
    std::string addonForInputMethod(const std::string &imName) { return instance_->addonForInputMethod(imName); }
    void activate() { return instance_->activate(); }
    void deactivate() { return instance_->deactivate(); }
    void toggle() { return instance_->toggle(); }
    void resetInputMethodList() { return instance_->resetInputMethodList(); }
    int state() { return instance_->state(); }
    void reloadConfig() { return instance_->reloadConfig(); }
    void reloadAddonConfig(const std::string &addonName) { return instance_->reloadAddonConfig(addonName); }
    std::string currentInputMethod() { return instance_->currentInputMethod(); }
    void setCurrentInputMethod(const std::string &imName) { return instance_->setCurrentInputMethod(imName); }

private:
    Instance *instance_;

private:
    FCITX_OBJECT_VTABLE_METHOD(exit, "Exit", "", "");
    FCITX_OBJECT_VTABLE_METHOD(restart, "Restart", "", "");
    FCITX_OBJECT_VTABLE_METHOD(configure, "Configure", "", "");
    FCITX_OBJECT_VTABLE_METHOD(configureAddon, "ConfigureAddon", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(configureInputMethod, "ConfigureIM", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(currentUI, "CurrentUI", "", "s");
    FCITX_OBJECT_VTABLE_METHOD(addonForInputMethod, "AddonForIM", "s", "s");
    FCITX_OBJECT_VTABLE_METHOD(activate, "Activate", "", "");
    FCITX_OBJECT_VTABLE_METHOD(toggle, "Toggle", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetInputMethodList, "ResetIMList", "", "");
    FCITX_OBJECT_VTABLE_METHOD(state, "State", "", "i");
    FCITX_OBJECT_VTABLE_METHOD(reloadConfig, "ReloadConfig", "", "");
    FCITX_OBJECT_VTABLE_METHOD(reloadAddonConfig, "ReloadAddonConfig", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(currentInputMethod, "CurrentInputMethod", "", "s");
    FCITX_OBJECT_VTABLE_METHOD(setCurrentInputMethod, "SetCurrentIM", "s", "");
};

DBusModule::DBusModule(Instance *instance)
    : bus_(std::make_unique<dbus::Bus>(dbus::BusType::Session)),
      serviceWatcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)) {
    bus_->attachEventLoop(&instance->eventLoop());
    auto uniqueName = bus_->uniqueName();
    if (!bus_->requestName(FCITX_DBUS_SERVICE, Flags<RequestNameFlag>{RequestNameFlag::AllowReplacement,
                                                                      RequestNameFlag::ReplaceExisting})) {
        throw std::runtime_error("Unable to request dbus name");
    }

    selfWatcher_.reset(serviceWatcher_->watchService(
        FCITX_DBUS_SERVICE,
        [this, uniqueName, instance](const std::string &, const std::string &, const std::string &newName) {
            if (newName != uniqueName) {
                instance->exit();
            }
        }));

    controller_ = std::make_unique<Controller1>(instance);
    bus_->addObjectVTable("/controller", FCITX_CONTROLLER_DBUS_INTERFACE, *controller_);
}

DBusModule::~DBusModule() {}

dbus::Bus *DBusModule::bus() { return bus_.get(); }

class DBusModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override { return new DBusModule(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::DBusModuleFactory)
