/*
 * SPDX-FileCopyrightText: 2020-2021 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx4frontend.h"
#include <fstream>
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "dbus_public.h"

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext"
#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx"

namespace fcitx {

namespace {

std::vector<dbus::DBusStruct<std::string, int>>
buildFormattedTextVector(const Text &text) {
    std::vector<dbus::DBusStruct<std::string, int>> vector;
    for (size_t i = 0, e = text.size(); i < e; i++) {
        // In fcitx 4, underline bit means "no underline", so we need to reverse
        // it.
        const auto flag = text.formatAt(i) ^ TextFormatFlag::Underline;
        vector.emplace_back(
            std::make_tuple(text.stringAt(i), static_cast<int>(flag)));
    }
    return vector;
}

int getDisplayNumber(const std::string &var) {
    auto pos = var.find(':');
    if (pos == std::string::npos) {
        return 0;
    }
    // skip :
    pos += 1;
    // Handle address like :0.0
    auto period = var.find(pos, '.');
    if (period != std::string::npos) {
        period -= pos;
    }

    try {
        std::string num(var.substr(pos, period));
        int displayNumber = std::stoi(num);
        return displayNumber;
    } catch (...) {
    }
    return 0;
}
} // namespace

class Fcitx4InputMethod : public dbus::ObjectVTable<Fcitx4InputMethod> {
public:
    Fcitx4InputMethod(int display, Fcitx4FrontendModule *module, dbus::Bus *bus)
        : display_(display), module_(module), instance_(module->instance()),
          bus_(std::make_unique<dbus::Bus>(bus->address())) {
        bus_->attachEventLoop(&instance_->eventLoop());
        bus_->addObjectVTable("/inputmethod", FCITX_INPUTMETHOD_DBUS_INTERFACE,
                              *this);
        Flags<dbus::RequestNameFlag> requestFlag =
            dbus::RequestNameFlag::ReplaceExisting;
        auto dbusServiceName =
            stringutils::concat(FCITX_DBUS_SERVICE, "-", display);
        bus_->requestName(dbusServiceName, requestFlag);

        auto localMachineId = getLocalMachineId(/*fallback=*/"machine-id");
        auto path = stringutils::joinPath(
            "fcitx", "dbus", stringutils::concat(localMachineId, "-", display));
        bool res = StandardPath::global().safeSave(
            StandardPath::Type::Config, path, [this](int fd) {
                auto address = bus_->address();
                fs::safeWrite(fd, address.c_str(), address.size() + 1);
                // Because fcitx5 don't launch dbus by itself, we write 0
                // on purpose to make address resolve fail, except WPS.
                pid_t pid = 0;
                fs::safeWrite(fd, &pid, sizeof(pid_t));
                fs::safeWrite(fd, &pid, sizeof(pid_t));
                return true;
            });
        if (res) {
            // Failed to write address file does not matter if we could use
            // regular dbus.
            pathWrote_ =
                stringutils::joinPath(StandardPath::global().userDirectory(
                                          StandardPath::Type::Config),
                                      path);
        }
    }

    ~Fcitx4InputMethod() {
        if (!pathWrote_.empty()) {
            unlink(pathWrote_.data());
        }
    }

    std::tuple<int, bool, uint32_t, uint32_t, uint32_t, uint32_t>
    createICv3(const std::string &appname, int pid);
    dbus::Bus *bus() { return bus_.get(); }
    auto *instance() { return module_->instance(); }
    auto *parent() { return module_; }

private:
    // V1 and V2 are too old, so we just ignore them.
    FCITX_OBJECT_VTABLE_METHOD(createICv3, "CreateICv3", "si", "ibuuuu");

    int display_;
    Fcitx4FrontendModule *module_;
    Instance *instance_;
    std::unique_ptr<dbus::Bus> bus_;
    std::string pathWrote_;
};

class Fcitx4InputContext : public InputContext,
                           public dbus::ObjectVTable<Fcitx4InputContext> {
public:
    Fcitx4InputContext(int id, InputContextManager &icManager,
                       Fcitx4InputMethod *im, const std::string &sender,
                       const std::string &program)
        : InputContext(icManager, program),
          path_(stringutils::concat("/inputcontext_", id)), im_(im),
          handler_(im_->parent()->serviceWatcher().watchService(
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

    ~Fcitx4InputContext() { InputContext::destroy(); }

    const char *frontend() const override { return "fcitx4"; }

    const dbus::ObjectPath &path() const { return path_; }

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
        std::vector<dbus::DBusStruct<std::string, int>> strs =
            buildFormattedTextVector(preedit);
        updateFormattedPreeditTo(name_, strs, preedit.cursor());
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        deleteSurroundingTextDBusTo(name_, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        forwardKeyDBusTo(name_, static_cast<uint32_t>(key.rawKey().sym()),
                         static_cast<uint32_t>(key.rawKey().states()),
                         key.isRelease() ? 1 : 0);
        bus()->flush();
    }
#define CHECK_SENDER_OR_RETURN                                                 \
    if (currentMessage()->sender() != name_)                                   \
    return

    void enableInputContext() {}

    void closeInputContext() {}

    void mouseEvent(int) {}

    void setCursorLocation(int x, int y) {
        CHECK_SENDER_OR_RETURN;
        setCursorRect(Rect{x, y, 0, 0});
    }

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
        reset();
    }

    void setCursorRectDBus(int x, int y, int w, int h) {
        CHECK_SENDER_OR_RETURN;
        setCursorRect(Rect{x, y, x + w, y + h});
    }

    void setCapability(uint32_t cap) {
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
        CHECK_SENDER_OR_RETURN;
        surroundingText().setCursor(cursor, anchor);
        updateSurroundingText();
    }

    void destroyDBus() {
        CHECK_SENDER_OR_RETURN;
        delete this;
    }

    int processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state,
                        int isRelease, uint32_t time) {
        CHECK_SENDER_OR_RETURN false;
        KeyEvent event(
            this, Key(static_cast<KeySym>(keyval), KeyStates(state), keycode),
            isRelease, time);
        // Force focus if there's keyevent.
        if (!hasFocus()) {
            focusIn();
        }

        return keyEvent(event) ? 1 : 0;
    }

private:
    // Because there is no application to use, don't impl CommitPreedit and
    // UpdateClientSideUI.
    FCITX_OBJECT_VTABLE_METHOD(enableInputContext, "EnableIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(closeInputContext, "CloseIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusInDBus, "FocusIn", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusOutDBus, "FocusOut", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetDBus, "Reset", "", "");
    FCITX_OBJECT_VTABLE_METHOD(mouseEvent, "MouseEvent", "i", "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorLocation, "SetCursorLocation", "ii",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorRectDBus, "SetCursorRect", "iiii", "");
    FCITX_OBJECT_VTABLE_METHOD(setCapability, "SetCapacity", "u", "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingText, "SetSurroundingText", "suu",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingTextPosition,
                               "SetSurroundingTextPosition", "uu", "");
    FCITX_OBJECT_VTABLE_METHOD(destroyDBus, "DestroyIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuuiu",
                               "i");

    FCITX_OBJECT_VTABLE_SIGNAL(commitStringDBus, "CommitString", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(currentIM, "CurrentIM", "sss");
    FCITX_OBJECT_VTABLE_SIGNAL(updateFormattedPreedit, "UpdateFormattedPreedit",
                               "a(si)i");
    FCITX_OBJECT_VTABLE_SIGNAL(deleteSurroundingTextDBus,
                               "DeleteSurroundingText", "iu");
    FCITX_OBJECT_VTABLE_SIGNAL(forwardKeyDBus, "ForwardKey", "uui");

    dbus::ObjectPath path_;
    Fcitx4InputMethod *im_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>> handler_;
    std::string name_;
};

std::tuple<int, bool, uint32_t, uint32_t, uint32_t, uint32_t>
Fcitx4InputMethod::createICv3(const std::string &appname, int /*pid*/) {
    auto sender = currentMessage()->sender();
    int icid = module_->nextIcIdx();
    auto *ic = new Fcitx4InputContext(icid, instance_->inputContextManager(),
                                      this, sender, appname);
    auto group =
        instance_->defaultFocusGroup(stringutils::concat("x11::", display_));
    if (!group) {
        group = instance_->defaultFocusGroup("x11:");
    }
    ic->setFocusGroup(group);
    bus_->addObjectVTable(ic->path().path(), FCITX_INPUTCONTEXT_DBUS_INTERFACE,
                          *ic);

    return std::make_tuple(icid, true, 0, 0, 0, 0);
}

Fcitx4FrontendModule::Fcitx4FrontendModule(Instance *instance)
    : instance_(instance),
      table_(
          [this](int display) {
              try {
                  fcitx4InputMethod_.emplace(
                      display, std::make_unique<Fcitx4InputMethod>(
                                   display, this, bus()));
                  return true;
              } catch (...) {
              }
              return false;
          },
          [this](int display) { fcitx4InputMethod_.erase(display); }),
      watcher_(std::make_unique<dbus::ServiceWatcher>(*bus())) {
#ifdef ENABLE_X11
    if (xcb()) {
        createdCallback_ =
            xcb()->call<IXCBModule::addConnectionCreatedCallback>(
                [this](const std::string &name, xcb_connection_t *, int,
                       FocusGroup *) { addDisplay(name); });
        closedCallback_ = xcb()->call<IXCBModule::addConnectionClosedCallback>(
            [this](const std::string &name, xcb_connection_t *) {
                removeDisplay(name);
            });
    }
#endif
    // Always create display number zero.
    addDisplay("");

    event_ = instance_->watchEvent(
        EventType::InputContextInputMethodActivated, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &activated = static_cast<InputMethodActivatedEvent &>(event);
            auto *ic = activated.inputContext();
            if (ic->frontendName() == "fcitx4") {
                if (const auto *entry = instance_->inputMethodManager().entry(
                        activated.name())) {
                    static_cast<Fcitx4InputContext *>(ic)->updateIM(entry);
                }
            }
        });
}

dbus::Bus *Fcitx4FrontendModule::bus() {
    return dbus()->call<IDBusModule::bus>();
}

void Fcitx4FrontendModule::addDisplay(const std::string &name) {
    displayToHandle_.emplace(name, table_.add(getDisplayNumber(name), name));
}

void Fcitx4FrontendModule::removeDisplay(const std::string &name) {
    displayToHandle_.erase(name);
}

class Fcitx4FrontendModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new Fcitx4FrontendModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::Fcitx4FrontendModuleFactory);
