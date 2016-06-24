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

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod1"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext1"

namespace fcitx {

class DBusInputContext1 : public InputContext, public dbus::ObjectVTable {
public:
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
    // TODO UpdateClientSideUI
    FCITX_OBJECT_VTABLE_SIGNAL(forwardKey, "forwardKey", "uui");
};

class InputMethod1 : public dbus::ObjectVTable {
public:
};

DBusFrontendModule::DBusFrontendModule(Instance *instance) : m_instance(instance) {}

DBusFrontendModule::~DBusFrontendModule() {}

AddonInstance *DBusFrontendModule::dbus() {
    auto &addonManager = m_instance->addonManager();
    return addonManager.addon("dbus");
}
}
