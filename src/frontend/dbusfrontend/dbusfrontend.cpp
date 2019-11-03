//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "dbusfrontend.h"
#include "dbus_public.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod1"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext1"

namespace fcitx {

class InputMethod1 : public dbus::ObjectVTable<InputMethod1> {
public:
    InputMethod1(DBusFrontendModule *module, dbus::Bus *bus)
        : module_(module), instance_(module->instance()), bus_(bus),
          watcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)) {
        bus_->addObjectVTable("/org/freedesktop/portal/inputmethod", FCITX_INPUTMETHOD_DBUS_INTERFACE,
                              *this);
    }

    std::tuple<dbus::ObjectPath, std::vector<uint8_t>> createInputContext(
        const std::vector<dbus::DBusStruct<std::string, std::string>> &args);

    dbus::ServiceWatcher &serviceWatcher() { return *watcher_; }
    dbus::Bus *bus() { return bus_; }
    Instance *instance() { return module_->instance(); }

private:
    FCITX_OBJECT_VTABLE_METHOD(createInputContext, "CreateInputContext",
                               "a(ss)", "oay");

    DBusFrontendModule *module_;
    Instance *instance_;
    int icIdx = 0;
    dbus::Bus *bus_;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};

class DBusInputContext1 : public InputContext,
                          public dbus::ObjectVTable<DBusInputContext1> {
public:
    DBusInputContext1(int id, InputContextManager &icManager, InputMethod1 *im,
                      const std::string &sender, const std::string &program)
        : InputContext(icManager, program),
          path_("/org/freedesktop/portal/inputcontext/" + std::to_string(id)), im_(im),
          handler_(im_->serviceWatcher().watchService(
              sender,
              [this](const std::string &, const std::string &,
                     const std::string &newName) {
                  if (newName.empty()) {
                      delete this;
                  }
              })),
          name_(sender) {
        created();
    }

    ~DBusInputContext1() { InputContext::destroy(); }

    const char *frontend() const override { return "dbus"; }

    const dbus::ObjectPath path() const { return path_; }

    void updateIM(const InputMethodEntry *entry) {
        currentIMTo(name_, entry->name(), entry->uniqueName(),
                    entry->languageCode());
    }

    void commitStringImpl(const std::string &text) override {
        commitStringDBusTo(name_, text);
    }

    void updatePreeditImpl() override {
        auto preedit =
            im_->instance()->outputFilter(this, inputPanel().clientPreedit());
        std::vector<dbus::DBusStruct<std::string, int>> strs;
        for (int i = 0, e = preedit.size(); i < e; i++) {
            strs.emplace_back(std::make_tuple(
                preedit.stringAt(i), static_cast<int>(preedit.formatAt(i))));
        }
        updateFormattedPreeditTo(name_, strs, preedit.cursor());
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        deleteSurroundingTextDBusTo(name_, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        forwardKeyDBusTo(name_, static_cast<uint32_t>(key.rawKey().sym()),
                         static_cast<uint32_t>(key.rawKey().states()),
                         key.isRelease());
        bus()->flush();
    }
#define CHECK_SENDER_OR_RETURN                                                 \
    if (currentMessage()->sender() != name_)                                   \
    return

    void focusInDBus() {
        CHECK_SENDER_OR_RETURN;
        focusIn();
    }

    void focusOutDBus() {
        CHECK_SENDER_OR_RETURN;
        focusOut();
    }

    void resetDBus() {
        CHECK_SENDER_OR_RETURN;
        reset(ResetReason::Client);
    }

    void setCursorRectDBus(int x, int y, int w, int h) {
        CHECK_SENDER_OR_RETURN;
        setCursorRect(Rect{x, y, x + w, y + h});
    }

    void setCapability(uint64_t cap) {
        CHECK_SENDER_OR_RETURN;
        setCapabilityFlags(CapabilityFlags{cap});
    }

    void setSurroundingText(const std::string &str, uint32_t cursor,
                            uint32_t anchor) {
        CHECK_SENDER_OR_RETURN;
        surroundingText().setText(str, cursor, anchor);
        updateSurroundingText();
    }

    void setSurroundingTextPosition(uint32_t cursor, uint32_t anchor) {
        surroundingText().setCursor(cursor, anchor);
        CHECK_SENDER_OR_RETURN;
        updateSurroundingText();
    }

    void destroyDBus() {
        CHECK_SENDER_OR_RETURN;
        delete this;
    }

    bool processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state,
                         bool isRelease, uint32_t time) {
        CHECK_SENDER_OR_RETURN false;
        KeyEvent event(
            this, Key(static_cast<KeySym>(keyval), KeyStates(state), keycode),
            isRelease, time);
        // Force focus if there's keyevent.
        if (!hasFocus()) {
            focusIn();
        }

        return keyEvent(event);
    }

private:
    FCITX_OBJECT_VTABLE_METHOD(focusInDBus, "FocusIn", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusOutDBus, "FocusOut", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetDBus, "Reset", "", "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorRectDBus, "SetCursorRect", "iiii", "");
    FCITX_OBJECT_VTABLE_METHOD(setCapability, "SetCapability", "t", "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingText, "SetSurroundingText", "suu",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingTextPosition,
                               "SetSurroundingTextPosition", "uu", "");
    FCITX_OBJECT_VTABLE_METHOD(destroyDBus, "DestroyIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuubu",
                               "b");
    FCITX_OBJECT_VTABLE_SIGNAL(commitStringDBus, "CommitString", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(currentIM, "CurrentIM", "sss");
    FCITX_OBJECT_VTABLE_SIGNAL(updateFormattedPreedit, "UpdateFormattedPreedit",
                               "a(si)i");
    FCITX_OBJECT_VTABLE_SIGNAL(deleteSurroundingTextDBus,
                               "DeleteSurroundingText", "iu");
    // TODO UpdateClientSideUI
    FCITX_OBJECT_VTABLE_SIGNAL(forwardKeyDBus, "ForwardKey", "uub");

    dbus::ObjectPath path_;
    InputMethod1 *im_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>> handler_;
    std::string name_;
};

std::tuple<dbus::ObjectPath, std::vector<uint8_t>>
InputMethod1::createInputContext(
    const std::vector<dbus::DBusStruct<std::string, std::string>> &args) {
    std::unordered_map<std::string, std::string> strMap;
    for (auto &p : args) {
        std::string key = std::get<0>(p.data()), value = std::get<1>(p.data());
        strMap[key] = value;
    }
    std::string program;
    auto iter = strMap.find("program");
    if (iter != strMap.end()) {
        program = iter->second;
    }

    std::string *display = findValue(strMap, "display");

    auto sender = currentMessage()->sender();
    auto ic = new DBusInputContext1(icIdx++, instance_->inputContextManager(),
                                    this, sender, program);
    ic->setFocusGroup(instance_->defaultFocusGroup(display ? *display : ""));

    bus_->addObjectVTable(ic->path().path(), FCITX_INPUTCONTEXT_DBUS_INTERFACE,
                          *ic);
    return std::make_tuple(
        ic->path(), std::vector<uint8_t>(ic->uuid().begin(), ic->uuid().end()));
}

#define FCITX_PORTAL_DBUS_SERVICE "org.freedesktop.portal.Fcitx"

DBusFrontendModule::DBusFrontendModule(Instance *instance)
    : instance_(instance),
      portalBus_(std::make_unique<dbus::Bus>(dbus::BusType::Session)),
      inputMethod1_(std::make_unique<InputMethod1>(this, bus())),
      portalInputMethod1_(
          std::make_unique<InputMethod1>(this, portalBus_.get())) {

    portalBus_->attachEventLoop(&instance->eventLoop());
    if (!portalBus_->requestName(
            FCITX_PORTAL_DBUS_SERVICE,
            Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                         dbus::RequestNameFlag::Queue})) {
        FCITX_LOG(Warn) << "Can not get portal dbus name right now.";
    }

    event_ = instance_->watchEvent(
        EventType::InputContextInputMethodActivated, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &activated = static_cast<InputMethodActivatedEvent &>(event);
            auto ic = activated.inputContext();
            if (strcmp(ic->frontend(), "dbus") == 0) {
                if (auto entry = instance_->inputMethodManager().entry(
                        activated.name())) {
                    static_cast<DBusInputContext1 *>(ic)->updateIM(entry);
                }
            }
        });
}

DBusFrontendModule::~DBusFrontendModule() {
    portalBus_->releaseName(FCITX_PORTAL_DBUS_SERVICE);
}

dbus::Bus *DBusFrontendModule::bus() {
    return dbus()->call<IDBusModule::bus>();
}

class DBusFrontendModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new DBusFrontendModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::DBusFrontendModuleFactory);
