/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "dbusfrontend.h"
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
#include "dbus_public.h"

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod1"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext1"

namespace fcitx {

namespace {

std::vector<dbus::DBusStruct<std::string, int>>
buildFormattedTextVector(const Text &text) {
    std::vector<dbus::DBusStruct<std::string, int>> vector;
    for (int i = 0, e = text.size(); i < e; i++) {
        vector.emplace_back(std::make_tuple(
            text.stringAt(i), static_cast<int>(text.formatAt(i))));
    }
    return vector;
}
} // namespace

class InputMethod1 : public dbus::ObjectVTable<InputMethod1> {
public:
    InputMethod1(DBusFrontendModule *module, dbus::Bus *bus, const char *path)
        : module_(module), instance_(module->instance()), bus_(bus),
          watcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)) {
        bus_->addObjectVTable(path, FCITX_INPUTMETHOD_DBUS_INTERFACE, *this);
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
    dbus::Bus *bus_;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};

class DBusInputContext1 : public InputContext,
                          public dbus::ObjectVTable<DBusInputContext1> {
public:
    DBusInputContext1(int id, InputContextManager &icManager, InputMethod1 *im,
                      const std::string &sender, const std::string &program)
        : InputContext(icManager, program),
          path_("/org/freedesktop/portal/inputcontext/" + std::to_string(id)),
          im_(im), handler_(im_->serviceWatcher().watchService(
                       sender,
                       [this](const std::string &, const std::string &,
                              const std::string &newName) {
                           if (newName.empty()) {
                               delete this;
                           }
                       })),
          name_(sender) {
        processKeyEventMethod.setClosureFunction(
            [this](dbus::Message message, const dbus::ObjectMethod &method) {
                if (capabilityFlags().test(CapabilityFlag::KeyEventOrderFix)) {
                    InputContextEventBlocker blocker(this);
                    return method(std::move(message));
                }
                return method(std::move(message));
            });
        created();
    }

    ~DBusInputContext1() { InputContext::destroy(); }

    const char *frontend() const override { return "dbus"; }

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

    void updateClientSideUIImpl() override {
        auto preedit =
            im_->instance()->outputFilter(this, inputPanel().preedit());
        auto auxUp = im_->instance()->outputFilter(this, inputPanel().auxUp());
        auto auxDown =
            im_->instance()->outputFilter(this, inputPanel().auxDown());
        auto candidateList = inputPanel().candidateList();
        int cursorIndex = 0;

        std::vector<dbus::DBusStruct<std::string, int>> preeditStrings,
            auxUpStrings, auxDownStrings;
        std::vector<dbus::DBusStruct<std::string, std::string>> candidates;

        preeditStrings = buildFormattedTextVector(preedit);
        auxUpStrings = buildFormattedTextVector(auxUp);
        auxDownStrings = buildFormattedTextVector(auxDown);
        if (candidateList) {
            for (int i = 0, e = candidateList->size(); i < e; i++) {
                auto &candidate = candidateList->candidate(i);
                if (candidate.isPlaceHolder()) {
                    continue;
                }
                Text labelText = candidate.hasCustomLabel()
                                     ? candidate.customLabel()
                                     : candidateList->label(i);
                labelText = im_->instance()->outputFilter(this, labelText);
                Text candidateText =
                    im_->instance()->outputFilter(this, candidate.text());
                candidates.emplace_back(std::make_tuple(
                    labelText.toString(), candidateText.toString()));
            }
            cursorIndex = candidateList->cursorIndex();
        }
        updateClientSideUITo(name_, preeditStrings, preedit.cursor(),
                             auxUpStrings, auxDownStrings, candidates,
                             cursorIndex);
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

    void setCursorRectV2DBus(int x, int y, int w, int h, double scale) {
        CHECK_SENDER_OR_RETURN;
        setCursorRect(Rect{x, y, x + w, y + h}, scale);
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
    FCITX_OBJECT_VTABLE_METHOD(setCursorRectV2DBus, "SetCursorRectV2", "iiiid",
                               "");
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
    FCITX_OBJECT_VTABLE_SIGNAL(updateClientSideUI, "UpdateClientSideUI",
                               "a(si)ia(si)a(si)a(ss)i");
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
    for (const auto &p : args) {
        const auto &[key, value] = p.data();
        strMap[key] = value;
    }
    std::string program;
    auto iter = strMap.find("program");
    if (iter != strMap.end()) {
        program = iter->second;
    }

    std::string *display = findValue(strMap, "display");

    auto sender = currentMessage()->sender();
    auto *ic = new DBusInputContext1(module_->nextIcIdx(),
                                     instance_->inputContextManager(), this,
                                     sender, program);
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
      inputMethod1_(std::make_unique<InputMethod1>(
          this, bus(), "/org/freedesktop/portal/inputmethod")),
      inputMethod1Compatible_(std::make_unique<InputMethod1>(
          this, portalBus_.get(), "/inputmethod")),
      portalInputMethod1_(std::make_unique<InputMethod1>(
          this, portalBus_.get(), "/org/freedesktop/portal/inputmethod")) {

    portalBus_->attachEventLoop(&instance->eventLoop());
    if (!portalBus_->requestName(
            FCITX_PORTAL_DBUS_SERVICE,
            Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                         dbus::RequestNameFlag::Queue})) {
        FCITX_WARN() << "Can not get portal dbus name right now.";
    }

    event_ = instance_->watchEvent(
        EventType::InputContextInputMethodActivated, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &activated = static_cast<InputMethodActivatedEvent &>(event);
            auto *ic = activated.inputContext();
            if (strcmp(ic->frontend(), "dbus") == 0) {
                if (const auto *entry = instance_->inputMethodManager().entry(
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
