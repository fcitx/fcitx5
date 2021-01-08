/*
 * SPDX-FileCopyrightText: 2020-2021 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx4frontend.h"
#include "fcitx4utils.h"
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

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext"
#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx"

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

class Fcitx4InputMethod : public dbus::ObjectVTable<Fcitx4InputMethod> {
public:
    Fcitx4InputMethod(Fcitx4FrontendModule *module, dbus::Bus *bus,
                      const char *path)
        : module_(module), instance_(module->instance()), bus_(bus),
          watcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)) {
        bus_->addObjectVTable(path, FCITX_INPUTMETHOD_DBUS_INTERFACE, *this);
    }

    std::tuple<int, bool, uint32_t, uint32_t, uint32_t, uint32_t>
    createICv3(const std::string &appname, int pid);

    dbus::ServiceWatcher &serviceWatcher() { return *watcher_; }
    dbus::Bus *bus() { return bus_; }
    Instance *instance() { return module_->instance(); }

private:
    FCITX_OBJECT_VTABLE_METHOD(createICv3, "CreateICv3", "si", "ibuuuu");

    Fcitx4FrontendModule *module_;
    Instance *instance_;
    dbus::Bus *bus_;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};

class DBusInputContext2 : public InputContext,
                          public dbus::ObjectVTable<DBusInputContext2> {
public:
    DBusInputContext2(int id, InputContextManager &icManager,
                      Fcitx4InputMethod *im, const std::string &sender,
                      const std::string &program)
        : InputContext(icManager, program),
          path_("/inputcontext_" + std::to_string(id)), im_(im),
          handler_(im_->serviceWatcher().watchService(
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

    ~DBusInputContext2() { InputContext::destroy(); }

    const char *frontend() const override { return "fcitx4frontend"; }

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

    void enableInputContext() {}

    void closeInputContext() {}

    void commitPreedit() {}

    void mouseEvent(int x) {}

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
    FCITX_OBJECT_VTABLE_METHOD(enableInputContext, "EnableIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(closeInputContext, "CloseIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusInDBus, "FocusIn", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusOutDBus, "FocusOut", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetDBus, "Reset", "", "");
    FCITX_OBJECT_VTABLE_METHOD(commitPreedit, "CommitPreedit", "", "");
    FCITX_OBJECT_VTABLE_METHOD(mouseEvent, "MouseEvent", "i", "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorRectDBus, "SetCursorRect", "iiii", "");
    FCITX_OBJECT_VTABLE_METHOD(setCapability, "SetCapacity", "u", "");
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
    Fcitx4InputMethod *im_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>> handler_;
    std::string name_;
};

std::tuple<int, bool, uint32_t, uint32_t, uint32_t, uint32_t>
Fcitx4InputMethod::createICv3(const std::string &appname, int pid) {
    auto sender = currentMessage()->sender();
    int icid = module_->nextIcIdx();
    auto *ic = new DBusInputContext2(icid, instance_->inputContextManager(),
                                     this, sender, appname);
    bus_->addObjectVTable(ic->path().path(), FCITX_INPUTCONTEXT_DBUS_INTERFACE,
                          *ic);

    return std::make_tuple(icid, false, 0, 0, 0, 0);
}

char *setStrWithLen(char *res, const char *str, size_t len) {
    if (res) {
        res = static_cast<char *>(realloc(res, len + 1));
    } else {
        res = static_cast<char *>(malloc(len + 1));
    }
    memcpy(res, str, len);
    res[len] = '\0';
    return res;
}

int getDisplayNumber() {
    const char *display = getenv("DISPLAY");
    if (!display)
        return 0;
    size_t len;
    const char *p = display + strcspn(display, ":");
    if (*p != ':')
        return 0;
    p++;
    len = strcspn(p, ".");
    char *str_disp_num = setStrWithLen(nullptr, p, len);
    int displayNumber = atoi(str_disp_num);
    free(str_disp_num);
    return displayNumber;
}

Fcitx4FrontendModule::Fcitx4FrontendModule(Instance *instance)
    : instance_(instance),
      portalBus_(std::make_unique<dbus::Bus>(dbus::BusType::Session)),
      Fcitx4InputMethod_(
          std::make_unique<Fcitx4InputMethod>(this, bus(), "/inputmethod")),
      Fcitx4InputMethodCompatible_(std::make_unique<Fcitx4InputMethod>(
          this, portalBus_.get(), "/inputmethod")) {
    Flags<dbus::RequestNameFlag> requestFlag =
        dbus::RequestNameFlag::ReplaceExisting;
    this->bus()->requestName(FCITX_DBUS_SERVICE, requestFlag);
    auto display = getDisplayNumber();
    auto dbusServiceName =
        stringutils::concat(FCITX_DBUS_SERVICE, "-", display);
    this->bus()->requestName(dbusServiceName, requestFlag);

    char *addressFile = nullptr;
    FILE *fp =
        Fcitx4XDGGetFileUserWithPrefix("dbus", addressFile, "w", nullptr);
    free(addressFile);
    fprintf(fp, "%s", this->bus()->address().c_str());
    fwrite("\0", sizeof(char), 1, fp);
    //    pid_t curPid = getpid();
    //    fwrite(&dbusmodule->daemon.pid, sizeof(pid_t), 1, fp);
    //    fwrite(&curPid, sizeof(pid_t), 1, fp);
    fclose(fp);

    event_ = instance_->watchEvent(
        EventType::InputContextInputMethodActivated, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &activated = static_cast<InputMethodActivatedEvent &>(event);
            auto *ic = activated.inputContext();
            if (strcmp(ic->frontend(), "fcitx4frontend") == 0) {
                if (const auto *entry = instance_->inputMethodManager().entry(
                        activated.name())) {
                    static_cast<DBusInputContext2 *>(ic)->updateIM(entry);
                }
            }
        });
}

Fcitx4FrontendModule::~Fcitx4FrontendModule() {}

dbus::Bus *Fcitx4FrontendModule::bus() {
    return dbus()->call<IDBusModule::bus>();
}

class Fcitx4FrontendModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new Fcitx4FrontendModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::Fcitx4FrontendModuleFactory);
