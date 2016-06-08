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
#include "fcitx-utils/dbus.h"
#include "fcitx/addonmanager.h"

#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx"
#define FCITX_CONTROLLER_DBUS_INTERFACE "org.fcitx.Fcitx.Controller1"

using namespace fcitx::dbus;

namespace fcitx
{

class Controller1 : public ObjectVTable {
public:
    Controller1(Instance *instance) : m_instance(instance) {
    }

    void exit() {
        m_instance->exit();
    }

    void restart() { m_instance->restart(); }
    void configure() { m_instance->configure(); }
    void configureAddon(const std::string &addon) { m_instance->configureAddon(addon); }
    void configureInputMethod(const std::string &imName) { m_instance->configureInputMethod(imName); }
    std::string currentUI() { return m_instance->currentUI(); }
    std::string addonForInputMethod(const std::string &imName) { return m_instance->addonForInputMethod(imName); }
    void activate() { return m_instance->activate(); }
    void deactivate() { return m_instance->deactivate(); }
    void toggle() { return m_instance->toggle(); }
    void resetInputMethodList() { return m_instance->resetInputMethodList(); }
    int state() { return m_instance->state(); }
    void reloadConfig() { return m_instance->reloadConfig(); }
    void reloadAddonConfig(const std::string &addonName) { return m_instance->reloadAddonConfig(addonName); }
    std::string currentInputMethod() { return m_instance->currentInputMethod(); }
    void setCurrentInputMethod(const std::string &imName) { return m_instance->setCurrentInputMethod(imName); }

private:
    Instance *m_instance;

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

DBusModule::DBusModule(Instance *instance) : m_bus(std::make_unique<dbus::Bus>(dbus::BusType::Session)) {
    m_bus->attachEventLoop(&instance->eventLoop());
    if (!m_bus->requestName(FCITX_DBUS_SERVICE, Flags<RequestNameFlag>{RequestNameFlag::AllowReplacement, RequestNameFlag::ReplaceExisting})) {
        throw std::runtime_error("Unable to request dbus name");
    }
    m_controller = std::make_unique<Controller1>(instance);
    m_bus->addObjectVTable("/controller", FCITX_CONTROLLER_DBUS_INTERFACE, *m_controller);
}

DBusModule::~DBusModule() {
}

AddonInstance *DBusModuleFactory::create(AddonManager *manager)
{
    return new DBusModule(manager->instance());
}


}
