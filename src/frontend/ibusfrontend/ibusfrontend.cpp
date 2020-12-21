/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "ibusfrontend.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <mutex>
#include <fmt/format.h>
#include <uuid/uuid.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "dbus_public.h"

FCITX_DEFINE_LOG_CATEGORY(ibus, "ibus")
#define FCITX_IBUS_DEBUG() FCITX_LOGC(::ibus, Debug)
#define FCITX_IBUS_WARN() FCITX_LOGC(::ibus, Warn)

#define IBUS_INPUTMETHOD_DBUS_INTERFACE "org.freedesktop.IBus"
#define IBUS_INPUTCONTEXT_DBUS_INTERFACE "org.freedesktop.IBus.InputContext"
#define IBUS_SERVICE_DBUS_INTERFACE "org.freedesktop.IBus.Service"
#define IBUS_PANEL_SERVICE_NAME "org.freedesktop.IBus.Panel"

namespace fcitx {

namespace {

bool isInFlatpak() {
    static bool inFlatpak = fs::isreg("/.flatpak-info");
    return inFlatpak;
}

std::string readFileContent(const std::string &file) {
    std::ifstream fin(file, std::ios::binary | std::ios::in);
    std::vector<char> buffer;
    constexpr auto chunkSize = 4096;
    do {
        auto curSize = buffer.size();
        buffer.resize(curSize + chunkSize);
        if (!fin.read(buffer.data() + curSize, chunkSize)) {
            buffer.resize(curSize + fin.gcount());
            break;
        }
    } while (0);
    std::string str{buffer.begin(), buffer.end()};
    return stringutils::trim(str);
}

std::string getLocalMachineId(void) {
    auto content = readFileContent("/var/lib/dbus/machine-id");
    if (content.empty()) {
        content = readFileContent("/etc/machine-id");
    }

    if (content.empty()) {
        content = "machine-id";
    }

    return content;
}

std::string getSocketPath(bool isWayland) {
    auto *path = getenv("IBUS_ADDRESS_FILE");
    if (path) {
        return path;
    }
    std::string hostname = "unix";
    std::string displaynumber = "0";
    if (isWayland) {
        displaynumber = "wayland-0";
        if (auto *display = getenv("WAYLAND_DISPLAY")) {
            displaynumber = display;
        }
    } else if (auto *display = getenv("DISPLAY")) {
        auto *p = display;
        for (; *p != ':' && *p != '\0'; p++) {
            ;
        }

        char *displaynumberStart = nullptr;
        if (*p == ':') {
            hostname = std::string(display, p);
            displaynumberStart = p + 1;

            for (; *p != '.' && *p != '\0'; p++) {
                ;
            }

            displaynumber = std::string(displaynumberStart, p);
        } else {
            hostname = display;
        }
    }

    if (hostname[0] == '\0') {
        hostname = "unix";
    }

    return stringutils::joinPath(
        "ibus/bus", stringutils::concat(getLocalMachineId(), "-", hostname, "-",
                                        displaynumber));
}

std::string getFullSocketPath(bool isWayland) {
    return stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::Config),
        getSocketPath(isWayland));
}

std::pair<std::string, pid_t> getAddress(const std::string &socketPath) {
    pid_t pid = -1;

    /* get address from env variable */
    auto *address = getenv("IBUS_ADDRESS");
    if (address) {
        return {address, -1};
    }

    /* read address from ~/.config/ibus/bus/soketfile */
    UniqueFilePtr file(fopen(socketPath.c_str(), "rb"));
    if (!file) {
        return {};
    }
    RawConfig config;
    readFromIni(config, file.get());
    const auto *value = config.valueByPath("IBUS_ADDRESS");
    const auto *pidValue = config.valueByPath("IBUS_DAEMON_PID");

    if (value && pidValue) {
        try {
            pid = std::stoi(*pidValue);
            // Check if we are in flatpak, or pid is same with ourselves, or
            // another running process.
            if (isInFlatpak() || pid == getpid() || kill(pid, 0) == 0) {
                return {*value, pid};
            }
        } catch (...) {
        }
    }

    return {};
}

pid_t runIBusExit() {
    pid_t child_pid;
    if ((child_pid = fork()) == -1) {
        perror("fork");
        return -1;
    }

    /* child process */
    if (child_pid == 0) {
        char arg0[] = "ibus";
        char arg1[] = "exit";
        char *args[] = {arg0, arg1, nullptr};
        setpgid(
            child_pid,
            child_pid); // Needed so negative PIDs can kill children of /bin/sh
        execvp(args[0], args);
        perror("execl");
        _exit(1);
    }

    return child_pid;
}
} // namespace

using AttachmentsType = FCITX_STRING_TO_DBUS_TYPE("a{sv}");
using IBusText = FCITX_STRING_TO_DBUS_TYPE("(sa{sv}sv)");
using IBusAttrList = FCITX_STRING_TO_DBUS_TYPE("(sa{sv}av)");
using IBusAttribute = FCITX_STRING_TO_DBUS_TYPE("(sa{sv}uuuu)");

class IBusFrontend : public dbus::ObjectVTable<IBusFrontend> {
public:
    IBusFrontend(IBusFrontendModule *module, dbus::Bus *bus,
                 const std::string &interface)
        : module_(module), instance_(module->instance()), bus_(bus),
          watcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)) {
        if (watcher_) {
            bus_->addObjectVTable("/org/freedesktop/IBus", interface, *this);
        }
    }

    dbus::ObjectPath createInputContext(const std::string &args);

    dbus::ServiceWatcher &serviceWatcher() { return *watcher_; }
    dbus::Bus *bus() { return bus_; }
    Instance *instance() { return module_->instance(); }

private:
    FCITX_OBJECT_VTABLE_METHOD(createInputContext, "CreateInputContext", "s",
                               "o");

    IBusFrontendModule *module_;
    Instance *instance_;
    int icIdx = 0;
    dbus::Bus *bus_;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};

constexpr uint32_t releaseMask = (1 << 30);

IBusAttribute makeIBusAttr(uint32_t type, uint32_t value, uint32_t start,
                           uint32_t end) {
    IBusAttribute attr;
    std::get<0>(attr) = "IBusAttribute";
    std::get<2>(attr) = type;
    std::get<3>(attr) = value;
    std::get<4>(attr) = start;
    std::get<5>(attr) = end;
    return attr;
}

IBusAttrList makeIBusAttrList() {
    IBusAttrList attrList;
    std::get<0>(attrList) = "IBusAttrList";
    return attrList;
}

IBusText makeSimpleIBusText(const std::string &str) {
    IBusText text;
    std::get<0>(text) = "IBusText";
    std::get<2>(text) = str;
    std::get<3>(text).setData(makeIBusAttrList());
    return text;
}

class IBusInputContext;
class IBusService : public dbus::ObjectVTable<IBusService> {
public:
    IBusService(IBusInputContext *ic) : ic_(ic) {}

private:
    void destroyDBus();

    FCITX_OBJECT_VTABLE_METHOD(destroyDBus, "Destroy", "", "");
    IBusInputContext *ic_;
};

class IBusInputContext : public InputContext,
                         public dbus::ObjectVTable<IBusInputContext> {
public:
    IBusInputContext(int id, InputContextManager &icManager, IBusFrontend *im,
                     const std::string &sender, const std::string &program)
        : InputContext(icManager, program),
          path_("/org/freedesktop/IBus/InputContext_" + std::to_string(id)),
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
                InputContextEventBlocker blocker(this);
                return method(std::move(message));
            });
        im->bus()->addObjectVTable(path().path(),
                                   IBUS_INPUTCONTEXT_DBUS_INTERFACE, *this);
        im->bus()->addObjectVTable(path().path(), IBUS_SERVICE_DBUS_INTERFACE,
                                   service_);
        created();
    }

    ~IBusInputContext() { InputContext::destroy(); }

    const char *frontend() const override { return "ibus"; }

    const std::string &name() const { return name_; }

    dbus::ObjectPath path() const { return path_; }

    void commitStringImpl(const std::string &str) override {
        IBusText text = makeSimpleIBusText(str);
        commitTextTo(name_, dbus::Variant(std::move(text)));
    }

    void updatePreeditImpl() override {
        auto preedit =
            im_->instance()->outputFilter(this, inputPanel().clientPreedit());
        // variant : -> s, a{sv} sv
        // v -> s a? av
        dbus::Variant v;
        const auto preeditString = preedit.toString();
        IBusText text = makeSimpleIBusText(preedit.toString());

        IBusAttrList attrList = makeIBusAttrList();

        size_t offset = 0;
        for (int i = 0, e = preedit.size(); i < e; i++) {
            auto len = utf8::length(preedit.stringAt(i));
            if (preedit.formatAt(i) & TextFormatFlag::Underline) {
                std::get<2>(attrList).emplace_back(
                    makeIBusAttr(1, 1, offset, offset + len));
            }
            if (preedit.formatAt(i) & TextFormatFlag::HighLight) {
                std::get<2>(attrList).emplace_back(
                    makeIBusAttr(2, 16777215, offset, offset + len));
                std::get<2>(attrList).emplace_back(
                    makeIBusAttr(3, 0, offset, offset + len));
            }

            offset += len;
        }

        std::get<3>(text).setData(std::move(attrList));

        v.setData(std::move(text));
        uint32_t cursor = preedit.cursor() >= 0
                              ? utf8::length(preeditString, 0, preedit.cursor())
                              : 0;
        if (clientCommitPreedit_) {
            updatePreeditTextWithModeTo(name_, v, cursor, offset != 0, 0);
        } else {
            updatePreeditTextTo(name_, v, cursor, offset != 0);
        }
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        deleteSurroundingTextDBusTo(name_, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        uint32_t state = static_cast<uint32_t>(key.rawKey().states());
        if (key.isRelease()) {
            state |= releaseMask;
        }

        auto code = key.rawKey().code();
        if (code) {
            code -= 8;
        }

        forwardKeyEventTo(name_, static_cast<uint32_t>(key.rawKey().sym()),
                          static_cast<uint32_t>(code), state);
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

    void setCursorLocation(int x, int y, int w, int h) {
        CHECK_SENDER_OR_RETURN;
        auto flags = capabilityFlags().unset(CapabilityFlag::RelativeRect);
        setCapabilityFlags(flags);
        setCursorRect(Rect{x, y, x + w, y + h});
    }

    void setCursorLocationRelative(int x, int y, int w, int h) {
        CHECK_SENDER_OR_RETURN;
        auto flags = capabilityFlags();
        flags |= CapabilityFlag::RelativeRect;
        setCapabilityFlags(flags);
        setCursorRect(Rect{x, y, x + w, y + h});
    }

    void setCapability(uint32_t cap) {
        CHECK_SENDER_OR_RETURN;

        enum Capabilities {
            IBUS_CAP_PREEDIT_TEXT = 1 << 0,
            IBUS_CAP_AUXILIARY_TEXT = 1 << 1,
            IBUS_CAP_LOOKUP_TABLE = 1 << 2,
            IBUS_CAP_FOCUS = 1 << 3,
            IBUS_CAP_PROPERTY = 1 << 4,
            IBUS_CAP_SURROUNDING_TEXT = 1 << 5
        };
        auto flags = capabilityFlags()
                         .unset(CapabilityFlag::FormattedPreedit)
                         .unset(CapabilityFlag::SurroundingText);
        if (cap & IBUS_CAP_PREEDIT_TEXT) {
            flags |= CapabilityFlag::Preedit;
            flags |= CapabilityFlag::FormattedPreedit;
        }
        if (cap & IBUS_CAP_SURROUNDING_TEXT) {
            flags |= CapabilityFlag::SurroundingText;
            if (!capabilityFlags().test(CapabilityFlag::SurroundingText)) {
                requireSurroundingTextTo(name_);
            }
        }

        setCapabilityFlags(flags);
    }

    void setSurroundingText(const std::string &str, uint32_t cursor,
                            uint32_t anchor) {
        CHECK_SENDER_OR_RETURN;
        surroundingText().setText(str, cursor, anchor);
        updateSurroundingText();
    }

    bool processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state) {
        CHECK_SENDER_OR_RETURN false;
        KeyEvent event(this,
                       Key(static_cast<KeySym>(keyval),
                           KeyStates(state & (~releaseMask)), keycode + 8),
                       state & releaseMask, 0);
        // Force focus if there's keyevent.
        if (!hasFocus()) {
            focusIn();
        }

        return keyEvent(event);
    }

    void enable() {}
    void disable() {}
    static bool isEnabled() { return true; }
    void propertyActivate(const std::string &, int32_t) {}
    void setEngine(const std::string &) {}
    static dbus::Variant getEngine() { return dbus::Variant(0); }
    void setSurroundingText(const dbus::Variant &text, uint32_t cursor,
                            uint32_t anchor) {
        if (text.signature() != "(sa{sv}sv)") {
            return;
        }
        const auto &s = text.dataAs<IBusText>();
        surroundingText().setText(std::get<2>(s), cursor, anchor);
        updateSurroundingText();
    }
    IBusService &service() { return service_; }

private:
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuu", "b");
    FCITX_OBJECT_VTABLE_METHOD(setCursorLocation, "SetCursorLocation", "iiii",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorLocationRelative,
                               "SetCursorLocationRelative", "iiii", "");
    FCITX_OBJECT_VTABLE_METHOD(focusInDBus, "FocusIn", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusOutDBus, "FocusOut", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetDBus, "Reset", "", "");
    FCITX_OBJECT_VTABLE_METHOD(enable, "Enable", "", "");
    FCITX_OBJECT_VTABLE_METHOD(disable, "Disable", "", "");
    FCITX_OBJECT_VTABLE_METHOD(isEnabled, "IsEnabled", "", "b");
    FCITX_OBJECT_VTABLE_METHOD(setCapability, "SetCapabilities", "u", "");
    FCITX_OBJECT_VTABLE_METHOD(propertyActivate, "PropertyActivate", "si", "");
    FCITX_OBJECT_VTABLE_METHOD(setEngine, "SetEngine", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(getEngine, "GetEngine", "", "v");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingText, "SetSurroundingText", "vuu",
                               "");

    FCITX_OBJECT_VTABLE_SIGNAL(commitText, "CommitText", "v");
    FCITX_OBJECT_VTABLE_SIGNAL(enabled, "Enabled", "");
    FCITX_OBJECT_VTABLE_SIGNAL(disabled, "Disabled", "");
    FCITX_OBJECT_VTABLE_SIGNAL(forwardKeyEvent, "ForwardKeyEvent", "uuu");
    FCITX_OBJECT_VTABLE_SIGNAL(updatePreeditText, "UpdatePreeditText", "vub");
    FCITX_OBJECT_VTABLE_SIGNAL(updatePreeditTextWithMode,
                               "UpdatePreeditTextWithMode", "vubu");
    FCITX_OBJECT_VTABLE_SIGNAL(deleteSurroundingTextDBus,
                               "DeleteSurroundingText", "iu");
    FCITX_OBJECT_VTABLE_SIGNAL(requireSurroundingText, "RequireSurroundingText",
                               "");

    // We don't use following
    FCITX_OBJECT_VTABLE_SIGNAL(showPreeditText, "ShowPreeditText", "");
    FCITX_OBJECT_VTABLE_SIGNAL(hidePreeditText, "HidePreeditText", "");
    FCITX_OBJECT_VTABLE_SIGNAL(updateAuxiliaryText, "UpdateAuxiliaryText",
                               "vb");
    FCITX_OBJECT_VTABLE_SIGNAL(showAuxiliaryText, "ShowAuxiliaryText", "");
    FCITX_OBJECT_VTABLE_SIGNAL(hideAuxiliaryText, "hideAuxiliaryText", "");
    FCITX_OBJECT_VTABLE_SIGNAL(updateLookupTable, "UpdateLookupTable", "vb");
    FCITX_OBJECT_VTABLE_SIGNAL(showLookupTable, "ShowLookupTable", "");
    FCITX_OBJECT_VTABLE_SIGNAL(hideLookupTable, "HideLookupTable", "");
    FCITX_OBJECT_VTABLE_SIGNAL(pageUpLookupTable, "PageUpLookupTable", "");
    FCITX_OBJECT_VTABLE_SIGNAL(pageDownLookupTable, "PageDownLookupTable", "");
    FCITX_OBJECT_VTABLE_SIGNAL(cursorUpLookupTable, "CursorUpLookupTable", "");
    FCITX_OBJECT_VTABLE_SIGNAL(cursorDownLookupTable, "CursorDownLookupTable",
                               "");
    FCITX_OBJECT_VTABLE_SIGNAL(registerProperties, "RegisterProperties", "v");
    FCITX_OBJECT_VTABLE_SIGNAL(updateProperty, "UpdateProperty", "v");

    // We dont tell others anything.
    static std::tuple<uint32_t, uint32_t> contentType() { return {0, 0}; }
    void setContentType(uint32_t hints, uint32_t purpose) {

        static const CapabilityFlags purpose_related_capability = {
            CapabilityFlag::Alpha,   CapabilityFlag::Digit,
            CapabilityFlag::Number,  CapabilityFlag::Dialable,
            CapabilityFlag::Url,     CapabilityFlag::Email,
            CapabilityFlag::Password};

        static const CapabilityFlags hints_related_capability = {
            CapabilityFlag::SpellCheck,
            CapabilityFlag::NoSpellCheck,
            CapabilityFlag::WordCompletion,
            CapabilityFlag::Lowercase,
            CapabilityFlag::Uppercase,
            CapabilityFlag::UppercaseWords,
            CapabilityFlag::UppwercaseSentences,
            CapabilityFlag::NoOnScreenKeyboard};

        enum {
            GTK_INPUT_PURPOSE_FREE_FORM,
            GTK_INPUT_PURPOSE_ALPHA,
            GTK_INPUT_PURPOSE_DIGITS,
            GTK_INPUT_PURPOSE_NUMBER,
            GTK_INPUT_PURPOSE_PHONE,
            GTK_INPUT_PURPOSE_URL,
            GTK_INPUT_PURPOSE_EMAIL,
            GTK_INPUT_PURPOSE_NAME,
            GTK_INPUT_PURPOSE_PASSWORD,
            GTK_INPUT_PURPOSE_PIN
        };

        auto flag = capabilityFlags()
                        .unset(purpose_related_capability)
                        .unset(hints_related_capability);

#define CASE_PURPOSE(_PURPOSE, _CAPABILITY)                                    \
    case _PURPOSE:                                                             \
        flag |= _CAPABILITY;                                                   \
        break;

        switch (purpose) {
            CASE_PURPOSE(GTK_INPUT_PURPOSE_ALPHA, CapabilityFlag::Alpha)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_DIGITS, CapabilityFlag::Digit);
            CASE_PURPOSE(GTK_INPUT_PURPOSE_NUMBER, CapabilityFlag::Number)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_PHONE, CapabilityFlag::Dialable)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_URL, CapabilityFlag::Url)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_EMAIL, CapabilityFlag::Email)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_NAME, CapabilityFlag::Name)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_PASSWORD, CapabilityFlag::Password)
            CASE_PURPOSE(GTK_INPUT_PURPOSE_PIN,
                         (CapabilityFlags{CapabilityFlag::Password,
                                          CapabilityFlag::Digit}))
        case GTK_INPUT_PURPOSE_FREE_FORM:
        default:
            break;
        }

        enum {
            GTK_INPUT_HINT_NONE = 0,
            GTK_INPUT_HINT_SPELLCHECK = 1 << 0,
            GTK_INPUT_HINT_NO_SPELLCHECK = 1 << 1,
            GTK_INPUT_HINT_WORD_COMPLETION = 1 << 2,
            GTK_INPUT_HINT_LOWERCASE = 1 << 3,
            GTK_INPUT_HINT_UPPERCASE_CHARS = 1 << 4,
            GTK_INPUT_HINT_UPPERCASE_WORDS = 1 << 5,
            GTK_INPUT_HINT_UPPERCASE_SENTENCES = 1 << 6,
            GTK_INPUT_HINT_INHIBIT_OSK = 1 << 7,
            GTK_INPUT_HINT_VERTICAL_WRITING = 1 << 8,
            GTK_INPUT_HINT_EMOJI = 1 << 9,
            GTK_INPUT_HINT_NO_EMOJI = 1 << 10
        };

#define CHECK_HINTS(_HINTS, _CAPABILITY)                                       \
    if (hints & _HINTS) {                                                      \
        flag |= _CAPABILITY;                                                   \
    }

        CHECK_HINTS(GTK_INPUT_HINT_SPELLCHECK,
                    fcitx::CapabilityFlag::SpellCheck)
        CHECK_HINTS(GTK_INPUT_HINT_NO_SPELLCHECK,
                    fcitx::CapabilityFlag::NoSpellCheck);
        CHECK_HINTS(GTK_INPUT_HINT_WORD_COMPLETION,
                    fcitx::CapabilityFlag::WordCompletion)
        CHECK_HINTS(GTK_INPUT_HINT_LOWERCASE, fcitx::CapabilityFlag::Lowercase)
        CHECK_HINTS(GTK_INPUT_HINT_UPPERCASE_CHARS,
                    fcitx::CapabilityFlag::Uppercase)
        CHECK_HINTS(GTK_INPUT_HINT_UPPERCASE_WORDS,
                    fcitx::CapabilityFlag::UppercaseWords)
        CHECK_HINTS(GTK_INPUT_HINT_UPPERCASE_SENTENCES,
                    fcitx::CapabilityFlag::UppwercaseSentences)
        CHECK_HINTS(GTK_INPUT_HINT_INHIBIT_OSK,
                    fcitx::CapabilityFlag::NoOnScreenKeyboard)
        setCapability(flag);
    }
    FCITX_OBJECT_VTABLE_WRITABLE_PROPERTY(
        contentType, "ContentType", "(uu)",
        ([]() -> dbus::DBusStruct<uint32_t, uint32_t> {
            return {0, 0};
        }),
        ([this](dbus::DBusStruct<uint32_t, uint32_t> type) {
            setContentType(std::get<0>(type), std::get<1>(type));
        }),
        dbus::PropertyOption::Hidden);
    FCITX_OBJECT_VTABLE_WRITABLE_PROPERTY(
        clientCommitPreedit, "ClientCommitPreedit", "(b)",
        ([this]() -> dbus::DBusStruct<bool> { return {clientCommitPreedit_}; }),
        ([this](dbus::DBusStruct<bool> value) {
            clientCommitPreedit_ = std::get<0>(value);
        }),
        dbus::PropertyOption::Hidden);

    dbus::ObjectPath path_;
    IBusFrontend *im_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>> handler_;
    std::string name_;
    bool clientCommitPreedit_ = false;

    IBusService service_{this};
};

void IBusService::destroyDBus() {
    if (currentMessage()->sender() != ic_->name()) {
        return;
    }
    delete ic_;
}

dbus::ObjectPath
IBusFrontend::createInputContext(const std::string & /* unused */) {
    auto sender = currentMessage()->sender();
    auto *ic = new IBusInputContext(icIdx++, instance_->inputContextManager(),
                                    this, sender, "");
    ic->setFocusGroup(instance_->defaultFocusGroup());
    return ic->path();
}

#define IBUS_PORTAL_DBUS_SERVICE "org.freedesktop.portal.IBus"
#define IBUS_PORTAL_DBUS_INTERFACE "org.freedesktop.IBus.Portal"

std::set<std::string> allSocketPaths() {
    std::set<std::string> paths;
    if (isInFlatpak()) {
        // Flatpak always use DISPLAY=:99, which means we will need to guess
        // what files are available.
        auto map = StandardPath::global().multiOpenFilter(
            StandardPath::Type::Config, "ibus/bus", O_RDONLY,
            [](const std::string &path, const std::string &, bool user) {
                if (!user) {
                    return false;
                }
                return stringutils::startsWith(path, getLocalMachineId());
            });

        for (const auto &item : map) {
            paths.insert(item.second.path());
        }

        // Make the guess that display is 0, it is the most common value that
        // people would have.
        if (paths.empty()) {
            paths.insert(stringutils::joinPath(
                StandardPath::global().userDirectory(
                    StandardPath::Type::Config),
                "ibus/bus",
                stringutils::concat(getLocalMachineId(), "-unix-", 0)));
        }
    } else {
        paths.insert(getFullSocketPath(false));
    }

    // Also add wayland.
    paths.insert(getFullSocketPath(true));
    return paths;
}

IBusFrontendModule::IBusFrontendModule(Instance *instance)
    : instance_(instance), socketPaths_(allSocketPaths()) {
    dbus::VariantTypeRegistry::defaultRegistry().registerType<IBusText>();
    dbus::VariantTypeRegistry::defaultRegistry().registerType<IBusAttribute>();
    dbus::VariantTypeRegistry::defaultRegistry().registerType<IBusAttrList>();
    replaceIBus();
}

IBusFrontendModule::~IBusFrontendModule() {
    if (portalBus_) {
        portalBus_->releaseName(IBUS_PORTAL_DBUS_SERVICE);
    }

    if (addressWrote_.empty()) {
        return;
    }
    for (const auto &path : socketPaths_) {
        auto address = getAddress(path);
        if (address.first == addressWrote_ && address.second == pidWrote_) {
            // Writeback an empty invalid address file.
            RawConfig config;
            config.setValueByPath("IBUS_ADDRESS", "");
            config.setValueByPath("IBUS_DAEMON_PID", "");
            StandardPath::global().safeSave(
                StandardPath::Type::Config, path,
                [&config](int fd) { return writeAsIni(config, fd); });
        }
    }
}

dbus::Bus *IBusFrontendModule::bus() {
    return dbus()->call<IDBusModule::bus>();
}

void IBusFrontendModule::replaceIBus() {
    FCITX_IBUS_DEBUG() << "Found ibus socket files: " << socketPaths_;
    std::pair<std::string, pid_t> address;
    for (const auto &path : socketPaths_) {
        address = getAddress(path);

        FCITX_IBUS_DEBUG() << "Found ibus address from file " << path << ": "
                           << address;

        if (isInFlatpak()) {
            // Check the in flatpak special pid value.
            if (address.second == 0) {
                continue;
            }
        } else {
            // It's not meaningful to compare pid from different pid namespace.
            if (address.second == getpid()) {
                continue;
            }
        }
        if (!address.first.empty() &&
            address.first.find("fcitx_random_string") == std::string::npos) {
            break;
        }
    }
    const auto &oldAddress = address.first;
    FCITX_IBUS_DEBUG() << "Old ibus address is: " << oldAddress;
    if (!oldAddress.empty()) {
        if (isInFlatpak()) {
            // When running inside flatpak, ibus command won't be available.
            // sd-bus does not connect to IBus's dbus, probably due to different
            // implemenation on dbus protocol. sd-bus has bug on xdg-dbus-proxy
            // anyway, so luckily, in the flatpak we are forced to used libdbus
            // which works for bus of ibus.
            FCITX_IBUS_DEBUG() << "Connecting to ibus address: " << oldAddress;
            dbus::Bus bus(oldAddress);
            if (bus.isOpen()) {
                auto call = bus.createMethodCall(
                    "org.freedesktop.IBus", "/org/freedesktop/IBus",
                    "org.freedesktop.IBus", "Exit");
                call << false;
                call.call(1000000);
                // Wait 1 second to become ibus.
                timeEvent_ = instance()->eventLoop().addTimeEvent(
                    CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
                    [this](EventSourceTime *, uint64_t) {
                        becomeIBus();
                        return true;
                    });
            }
        } else {
            // If ibus command is not available, then ibus is probably not
            // installed anyway, so there is nothing to worry about.
            auto pid = runIBusExit();
            if (pid > 0) {
                FCITX_IBUS_DEBUG() << "Running ibus exit.";
                timeEvent_ = instance()->eventLoop().addTimeEvent(
                    CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
                    [this, pid, address](EventSourceTime *, uint64_t) {
                        int stat = -1;
                        pid_t ret;
                        while ((ret = waitpid(pid, &stat, WNOHANG)) <= 0) {
                            if (ret == 0) {
                                FCITX_IBUS_DEBUG()
                                    << "ibus exit haven't ended yet, kill it.";
                                kill(pid, SIGKILL);
                                waitpid(pid, &stat, WNOHANG);
                                break;
                            }

                            if (errno != EINTR) {
                                stat = -1;
                                break;
                            }
                        }

                        FCITX_IBUS_DEBUG() << "ibus exit returns with " << stat;
                        if (stat != 0) {
                            auto cmd = readFileContent(stringutils::joinPath(
                                "/proc", address.second, "cmdline"));
                            if (cmd.find("ibus-daemon") != std::string::npos) {
                                FCITX_IBUS_DEBUG()
                                    << "try to kill ibus-daemon.";
                                // Well we can't kill it so better not to
                                // replace it.
                                if (kill(address.second, SIGKILL) != 0) {
                                    return true;
                                }
                            }
                        }
                        becomeIBus();
                        return true;
                    });
            }
        }
    }

    if (!timeEvent_) {
        becomeIBus();
    }
}

void IBusFrontendModule::becomeIBus() {
    // ibusBus_.reset();
    FCITX_IBUS_DEBUG() << "Requesting IBus service name.";
    if (!bus()->requestName(
            "org.freedesktop.IBus",
            Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                         dbus::RequestNameFlag::Queue})) {
        FCITX_IBUS_DEBUG() << "Failed to request IBus service name.";
        return;
    }

    inputMethod1_ = std::make_unique<IBusFrontend>(
        this, bus(), IBUS_INPUTMETHOD_DBUS_INTERFACE);
    RawConfig config;
    auto address = bus()->address();
    // This is a small hack to make ibus think that address is changed.
    // Otherwise it won't retry connection since we always use session bus
    // instead of start our own one.
    // https://dbus.freedesktop.org/doc/dbus-specification.html#addresses
    // We just try to append a custom argument. And dbus will simply ignore
    // unrecognized such field.
    while (stringutils::endsWith(address, ";")) {
        address.pop_back();
    }
    address.append(",fcitx_random_string=");
    ICUUID uuid;
    uuid_generate(uuid.data());
    for (auto v : uuid) {
        address.append(fmt::format("{:02x}", static_cast<int>(v)));
    }
    FCITX_IBUS_DEBUG() << "IBus address is written with: " << address;
    config.setValueByPath("IBUS_ADDRESS", address);
    // im module use kill(pid, 0) to check, since we're using different pid
    // namespace, write with a pid make this call return 0.
    pid_t pidToWrite = isInFlatpak() ? 0 : getpid();
    config.setValueByPath("IBUS_DAEMON_PID", std::to_string(pidToWrite));

    FCITX_IBUS_DEBUG() << "Writing ibus daemon info.";
    for (const auto &path : socketPaths_) {
        if (!StandardPath::global().safeSave(
                StandardPath::Type::Config, path,
                [&config](int fd) { return writeAsIni(config, fd); })) {
            return;
        }
    }

    addressWrote_ = address;
    pidWrote_ = pidToWrite;

    bus()->requestName(
        IBUS_PANEL_SERVICE_NAME,
        Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                     dbus::RequestNameFlag::Queue});

    portalBus_ = std::make_unique<dbus::Bus>(bus()->address());
    portalIBusFrontend_ = std::make_unique<IBusFrontend>(
        this, portalBus_.get(), IBUS_PORTAL_DBUS_INTERFACE);
    portalBus_->attachEventLoop(&instance()->eventLoop());
    if (!portalBus_->requestName(
            IBUS_PORTAL_DBUS_SERVICE,
            Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                         dbus::RequestNameFlag::Queue})) {
        FCITX_IBUS_WARN() << "Can not get portal ibus name right now.";
    }
}

class IBusFrontendModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new IBusFrontendModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::IBusFrontendModuleFactory);
