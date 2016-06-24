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

#include "fcitx/inputcontext.h"
#include "dbusfrontend.h"
#include "fcitx/instance.h"
#include "fcitx-utils/dbus-object-vtable.h"
#include "fcitx-utils/dbus-message.h"
#include "fcitx-utils/metastring.h"
#include "modules/dbus/dbus_public.h"

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod1"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext1"

namespace fcitx {

class DBusInputContext1 : public InputContext, public dbus::ObjectVTable {
public:
    DBusInputContext1(int id, InputContextManager &icManager, const std::string &program) : InputContext(icManager, program),
        m_path("/inputcontext/" + std::to_string(id)) {
    }

    const dbus::ObjectPath path() const {
        return m_path;
    }

    using InputContext::focusIn;
    using InputContext::focusOut;
    using InputContext::reset;

    void setCursorRectDBus(int x, int y, int w, int h) { setCursorRect({x, y, x + w, y + h}); }

    void setCapability(uint64_t cap) { setCapabilityFlags(CapabilityFlags{cap}); }

    void setSurroundingText(const std::string &str, uint32_t cursor, uint32_t anchor) {
        surroundingText().setText(str, cursor, anchor);
        updateSurroundingText();
    }

    void setSurroundingTextPosition(uint32_t cursor, uint32_t anchor) {
        surroundingText().setCursor(cursor, anchor);
        updateSurroundingText();
    }

    void destroy() { delete this; }

    bool processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state, bool isRelease, uint32_t time) {
        KeyEvent event(this, Key(static_cast<KeySym>(keyval), KeyStates(state)), isRelease, keycode, time);
        return keyEvent(event);
    }

    void commitStringImpl(const std::string &text) override { commitStringDBus(text); }

    void updatePreeditImpl() override {
        auto &preedit = this->preedit();
        std::vector<dbus::DBusStruct<std::string, int>> strs;
        for (int i = 0, e = preedit.size(); i < e; i++) {
            strs.push_back(std::make_tuple(preedit.stringAt(i), static_cast<int>(preedit.formatAt(i))));
        }
        updateFormattedPreedit(strs, preedit.cursor());
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        deleteSurroundingTextDBus(offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent & key) override {
        forwardKeyDBus(static_cast<uint32_t>(key.rawKey().sym()),
                       static_cast<uint32_t>(key.rawKey().states()),
                       key.isRelease());
    }
private:
    FCITX_OBJECT_VTABLE_METHOD(focusIn, "focusIn", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusOut, "focusOut", "", "");
    FCITX_OBJECT_VTABLE_METHOD(reset, "Reset", "", "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorRectDBus, "SetCursorRect", "iiii", "");
    FCITX_OBJECT_VTABLE_METHOD(setCapability, "SetCapability", "t", "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingText, "SetSurroundingText", "suu", "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingTextPosition, "SetSurroundingTextPosition", "uu", "");
    FCITX_OBJECT_VTABLE_METHOD(destroy, "DestroyIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuuiu", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(commitStringDBus, "CommitString", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(currentIM, "CurrentIM", "sss");
    FCITX_OBJECT_VTABLE_SIGNAL(updateFormattedPreedit, "UpdateFormattedPreedit", "a(si)i");
    FCITX_OBJECT_VTABLE_SIGNAL(deleteSurroundingTextDBus, "DeleteSurroundingText", "iu");
    // TODO UpdateClientSideUI
    FCITX_OBJECT_VTABLE_SIGNAL(forwardKeyDBus, "forwardKey", "uub");

    dbus::ObjectPath m_path;
};

class InputMethod1 : public dbus::ObjectVTable {
public:
    InputMethod1(DBusFrontendModule *module) : m_module(module), m_instance(module->instance()) {
    }

    std::tuple<dbus::ObjectPath, std::vector<uint8_t>> createInputContext(const std::vector<dbus::DBusStruct<std::string, std::string>> &args) {
        std::unordered_map<std::string, std::string> strMap;
        for (auto &p : args) {
            std::string key, value;
            std::tie(key, value) = p;
            strMap[key] = value;
        }
        std::string program;
        auto iter = strMap.find("program");
        if (iter != strMap.end()) {
            program = iter->second;
        }

        // TODO: monitor sender dbus name
        // auto msg = currentMessage();
        auto ic = new DBusInputContext1(icIdx++, m_instance->inputContextManager(), program);
        auto bus = m_module->dbus()->call<IDBusModule::bus>();
        bus->addObjectVTable(ic->path().path(), FCITX_INPUTCONTEXT_DBUS_INTERFACE, *ic);
        return std::make_tuple(ic->path(), std::vector<uint8_t>(ic->uuid().begin(), ic->uuid().end()));
    }

    dbus::ObjectPath createInputContext2() {
        return std::get<0>(createInputContext({}));
    }
private:
    FCITX_OBJECT_VTABLE_METHOD(createInputContext, "CreateIC", "a(ss)", "oay");
    // debug purpose for now
    // TODO remove me
    FCITX_OBJECT_VTABLE_METHOD(createInputContext2, "CreateIC2", "", "o");

    DBusFrontendModule *m_module;
    Instance *m_instance;
    int icIdx = 0;
};

DBusFrontendModule::DBusFrontendModule(Instance *instance) : m_instance(instance),
    m_inputMethod1(std::make_unique<InputMethod1>(this)) {
    auto bus = this->dbus()->call<IDBusModule::bus>();
    bus->addObjectVTable("/inputmethod", FCITX_INPUTMETHOD_DBUS_INTERFACE, *m_inputMethod1.get());
}

DBusFrontendModule::~DBusFrontendModule() {}

AddonInstance *DBusFrontendModule::dbus() {
    auto &addonManager = m_instance->addonManager();
    return addonManager.addon("dbus");
}
}
