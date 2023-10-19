/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandmodule.h"
#include <fcntl.h>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <gio/gio.h>
#include <wayland-client.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/inputcontext.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "config.h"
#include "notifications_public.h"
#include "waylandeventreader.h"
#include "wl_seat.h"

#ifdef ENABLE_DBUS
#include "dbus_public.h"
#endif

#ifdef ENABLE_X11
#include "xcb_public.h"
#endif

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(wayland_log, "wayland");

#define FCITX_WAYLAND_INFO() FCITX_LOGC(::fcitx::wayland_log, Info)
#define FCITX_WAYLAND_ERROR() FCITX_LOGC(::fcitx::wayland_log, Error)
#define FCITX_WAYLAND_DEBUG() FCITX_LOGC(::fcitx::wayland_log, Debug)

namespace {
bool isKDE5() {
    static const DesktopType desktop = getDesktopType();
    return desktop == DesktopType::KDE5;
}

class ScopedEnvvar {
public:
    explicit ScopedEnvvar(std::string name, const char *value)
        : name_(std::move(name)) {
        auto old = getenv(name_.data());
        if (old) {
            oldValue_ = std::string(old);
        }

        setenv(name_.data(), value, true);
    }

    ~ScopedEnvvar() {
        if (oldValue_) {
            setenv(name_.data(), oldValue_->data(), true);
        } else {
            unsetenv(name_.data());
        }
    }

private:
    std::string name_;
    std::optional<std::string> oldValue_ = std::nullopt;
};

} // namespace

WaylandConnection::WaylandConnection(WaylandModule *wayland, std::string name)
    : parent_(wayland), name_(std::move(name)) {
    wl_display *display = nullptr;
    {
        std::unique_ptr<ScopedEnvvar> env;
        if (wayland_log().checkLogLevel(Debug)) {
            env = std::make_unique<ScopedEnvvar>("WAYLAND_DEBUG", "1");
        }
        display = wl_display_connect(name_.empty() ? nullptr : name_.c_str());
    }
    if (!display) {
        throw std::runtime_error("Failed to open wayland connection");
    }
    init(display);
}

WaylandConnection::WaylandConnection(WaylandModule *wayland, std::string name,
                                     int fd)
    : parent_(wayland), name_(std::move(name)) {
    wl_display *display = nullptr;
    {
        std::unique_ptr<ScopedEnvvar> env;
        if (wayland_log().checkLogLevel(Debug)) {
            env = std::make_unique<ScopedEnvvar>("WAYLAND_DEBUG", "1");
        }
        display = wl_display_connect_to_fd(fd);
    }
    if (!display) {
        throw std::runtime_error("Failed to open wayland connection");
    }
    init(display);
}

WaylandConnection::~WaylandConnection() {}

void WaylandConnection::init(wl_display *display) {
    display_ = std::make_unique<wayland::Display>(display);

    group_ = std::make_unique<FocusGroup>(
        "wayland:" + name_, parent_->instance()->inputContextManager());

    panelConn_ = display_->globalCreated().connect(
        [this](const std::string &name, const std::shared_ptr<void> &seat) {
            if (name == wayland::WlSeat::interface) {
                setupKeyboard(static_cast<wayland::WlSeat *>(seat.get()));
            }
        });
    panelRemovedConn_ = display_->globalRemoved().connect(
        [this](const std::string &name, const std::shared_ptr<void> &ptr) {
            if (name == wayland::WlSeat::interface) {
                keyboards_.erase(static_cast<wayland::WlSeat *>(ptr.get()));
            }
        });
    for (auto seat : display_->getGlobals<wayland::WlSeat>()) {
        setupKeyboard(seat.get());
    }
    eventReader_ = std::make_unique<WaylandEventReader>(this);
}

void WaylandConnection::finish() { parent_->removeConnection(name_); }

void WaylandConnection::setupKeyboard(wayland::WlSeat *seat) {
    auto &kbd = (keyboards_[seat] = std::make_unique<WaylandKeyboard>(seat));
    kbd->updateKeymap().connect([this]() {
        FCITX_WAYLAND_DEBUG() << "Update keymap";
        parent_->reloadXkbOption();
    });
}

WaylandModule::WaylandModule(fcitx::Instance *instance)
    : instance_(instance), isWaylandSession_(isSessionType("wayland")) {

    delayedReloadXkbOption_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
        [this](EventSourceTime *, uint64_t) {
            reloadXkbOptionReal();
            return true;
        });
    delayedReloadXkbOption_->setEnabled(false);

    reloadConfig();
    openConnection("");
    reloadXkbOption();

#ifdef ENABLE_DBUS
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            if (!isWaylandSession_ || !*config_.allowOverrideXKB) {
                return;
            }

            auto connection = findValue(conns_, "");
            if (!connection) {
                return;
            }

            if (isKDE5()) {
                setLayoutToKDE5();
            } else if (getDesktopType() == DesktopType::GNOME) {
                setLayoutToGNOME();
            }
        }));

    deferredDiagnose_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 3000000, 0,
        [this](EventSourceTime *, uint64_t) {
            selfDiagnose();
            return true;
        });
#endif
}

void WaylandModule::reloadConfig() { readAsIni(config_, "conf/wayland.conf"); }

bool WaylandModule::openConnection(const std::string &name) {
    if (conns_.count(name)) {
        return false;
    }

    WaylandConnection *newConnection = nullptr;
    try {
        auto iter = conns_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(name),
                                   std::forward_as_tuple(this, name));
        newConnection = &iter.first->second;
    } catch (const std::exception &e) {
        FCITX_ERROR() << e.what();
    }
    if (newConnection) {
        onConnectionCreated(*newConnection);
        return true;
    }
    return false;
}

bool WaylandModule::openConnectionSocket(int fd) {
    UnixFD guard = UnixFD::own(fd);
    auto name = stringutils::concat("socket:", fd);

    if (conns_.count(name)) {
        return false;
    }

    for (const auto &[name, connection] : conns_) {
        if (connection.display()->fd() == fd) {
            return false;
        }
    }

    WaylandConnection *newConnection = nullptr;
    try {
        auto iter = conns_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(name),
                                   std::forward_as_tuple(this, name, fd));
        guard.release();
        newConnection = &iter.first->second;
    } catch (const std::exception &e) {
    }
    if (newConnection) {
        onConnectionCreated(*newConnection);
        return true;
    }
    return false;
}

void WaylandModule::removeConnection(const std::string &name) {
    FCITX_WAYLAND_INFO() << "Connection removed " << name;
    if (name.empty() && instance_->exitWhenMainDisplayDisconnected() &&
        isWaylandSession_) {
        instance_->exit();
    }
    auto iter = conns_.find(name);
    if (iter != conns_.end()) {
        onConnectionClosed(iter->second);
        conns_.erase(iter);
    }
}

std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
WaylandModule::addConnectionCreatedCallback(WaylandConnectionCreated callback) {
    auto result = createdCallbacks_.add(std::move(callback));

    for (auto &p : conns_) {
        auto &conn = p.second;
        (**result->handler())(conn.name(), *conn.display(), conn.focusGroup());
    }
    return result;
}

std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
WaylandModule::addConnectionClosedCallback(WaylandConnectionClosed callback) {
    return closedCallbacks_.add(std::move(callback));
}

void WaylandModule::onConnectionCreated(WaylandConnection &conn) {
    for (auto &callback : createdCallbacks_.view()) {
        callback(conn.name(), *conn.display(), conn.focusGroup());
    }
}

void WaylandModule::onConnectionClosed(WaylandConnection &conn) {
    for (auto &callback : closedCallbacks_.view()) {
        callback(conn.name(), *conn.display());
    }
}
void WaylandModule::reloadXkbOption() {
    delayedReloadXkbOption_->setNextInterval(30000);
    delayedReloadXkbOption_->setOneShot();
}

void WaylandModule::reloadXkbOptionReal() {
#ifdef ENABLE_DBUS
    if (!isWaylandSession_) {
        return;
    }
    auto connection = findValue(conns_, "");
    if (!connection) {
        return;
    }

    FCITX_WAYLAND_DEBUG() << "Try to reload Xkb option from desktop";
    std::optional<std::string> xkbOption = std::nullopt;
    if (isKDE5()) {

        auto dbusAddon = dbus();
        if (!dbusAddon) {
            return;
        }

        fcitx::RawConfig config;
        readAsIni(config, StandardPath::Type::Config, "kxkbrc");
        auto model = config.valueByPath("Layout/Model");
        auto options = config.valueByPath("Layout/Options");
        xkbOption = (options ? *options : "");
        instance_->setXkbParameters(connection->focusGroup()->display(),
                                    DEFAULT_XKB_RULES, model ? *model : "",
                                    *xkbOption);
        FCITX_WAYLAND_DEBUG()
            << "KDE xkb options: model=" << (model ? *model : "")
            << " options=" << *xkbOption;
    } else if (getDesktopType() == DesktopType::GNOME) {
        UniqueCPtr<GSettings, g_object_unref> settings(
            g_settings_new("org.gnome.desktop.input-sources"));
        if (settings) {

            gchar **value = g_settings_get_strv(settings.get(), "xkb-options");
            if (value) {
                auto options = g_strjoinv(",", value);
                xkbOption = (options ? options : "");
                instance_->setXkbParameters(connection->focusGroup()->display(),
                                            DEFAULT_XKB_RULES, "", *xkbOption);
                FCITX_WAYLAND_DEBUG() << "GNOME xkb options=" << *xkbOption;
                g_free(options);
                g_strfreev(value);
            }
        }
    }
#ifdef ENABLE_X11
    if (auto xcbAddon = xcb(); xcbAddon && xkbOption) {
        xcbAddon->call<IXCBModule::setXkbOption>(
            xcbAddon->call<IXCBModule::mainDisplay>(), *xkbOption);
    }
#endif
#endif
}

void WaylandModule::setLayoutToKDE5() {
#ifdef ENABLE_DBUS
    auto dbusAddon = dbus();
    if (!dbusAddon) {
        return;
    }

    auto layoutAndVariant = parseLayout(
        instance_->inputMethodManager().currentGroup().defaultLayout());

    if (layoutAndVariant.first.empty()) {
        return;
    }

    fcitx::RawConfig config;
    readAsIni(config, StandardPath::Type::Config, "kxkbrc");
    config.setValueByPath("Layout/LayoutList", layoutAndVariant.first);
    config.setValueByPath("Layout/VariantList", layoutAndVariant.second);
    config.setValueByPath("Layout/DisplayNames", "");
    config.setValueByPath("Layout/Use", "true");

    // See: https://github.com/flatpak/flatpak/issues/5370
    // The write temp file and replace does not work with mount bind,
    // if the intention is to get the file populated outside the
    // sandbox.
    if (isInFlatpak()) {
        auto file = StandardPath::global().open(StandardPath::Type::Config,
                                                "kxkbrc", O_WRONLY);
        if (file.isValid()) {
            writeAsIni(config, file.fd());
        } else {
            FCITX_WAYLAND_ERROR() << "Failed to write to kxkbrc.";
        }
    } else {
        safeSaveAsIni(config, StandardPath::Type::Config, "kxkbrc");
    }

    auto bus = dbusAddon->call<IDBusModule::bus>();
    auto message =
        bus->createSignal("/Layouts", "org.kde.keyboard", "reloadConfig");
    message.send();
#endif
}

void WaylandModule::setLayoutToGNOME() {
#ifdef ENABLE_DBUS

    auto layoutAndVariant = parseLayout(
        instance_->inputMethodManager().currentGroup().defaultLayout());

    if (layoutAndVariant.first.empty()) {
        return;
    }

    std::string xkbsource = layoutAndVariant.first;
    if (!layoutAndVariant.second.empty()) {
        xkbsource =
            stringutils::concat(xkbsource, "+", layoutAndVariant.second);
    }

    UniqueCPtr<GSettings, g_object_unref> settings(
        g_settings_new("org.gnome.desktop.input-sources"));
    if (settings) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ss)"));

        g_variant_builder_add(&builder, "(ss)", "xkb", xkbsource.data());
        UniqueCPtr<GVariant, &g_variant_unref> value(
            g_variant_ref_sink(g_variant_builder_end(&builder)));
        g_settings_set_value(settings.get(), "sources", value.get());
        g_settings_set_value(settings.get(), "mru-sources", value.get());
    }
#endif
}

void WaylandModule::selfDiagnose() {
    if (!isWaylandSession_) {
        return;
    }

    WaylandConnection *connection = findValue(conns_, "");
    if (!connection) {
        return;
    }

    if (!notifications()) {
        return;
    }

    bool isWaylandIM = false;
    connection->focusGroup()->foreach([&isWaylandIM](InputContext *ic) {
        if (stringutils::startsWith(ic->frontendName(), "wayland")) {
            isWaylandIM = true;
            return false;
        }
        return true;
    });

    auto sendMessage = [this](const std::string &category,
                              const std::string &message) {
        notifications()->call<INotifications::showTip>(
            category, _("Fcitx"), "fcitx", _("Wayland Diagnose"), message,
            60000);
    };

    auto *gtk = getenv("GTK_IM_MODULE");
    auto *qt = getenv("QT_IM_MODULE");
    std::string gtkIM = gtk ? gtk : "";
    std::string qtIM = qt ? qt : "";

    std::vector<std::string> messages;
    const auto desktop = getDesktopType();
    if (desktop == DesktopType::KDE5) {
        if (!isWaylandIM) {
            sendMessage(
                "wayland-diagnose-kde",
                _("Fcitx should be launched by KWin under KDE Wayland in order "
                  "to use Wayland input method frontend. This can improve the "
                  "experience when using Fcitx on Wayland. To "
                  "configure this, you need to go to \"System Settings\" -> "
                  "\"Virtual "
                  "keyboard\" and select \"Fcitx 5\" from it. You may also "
                  "need to disable tools that launches input method, such as "
                  "imsettings on Fedora, or im-config on Debian/Ubuntu. For "
                  "more details see "
                  "https://fcitx-im.org/wiki/"
                  "Using_Fcitx_5_on_Wayland#KDE_Plasma"));
        } else if (!gtkIM.empty() || !qtIM.empty()) {
            sendMessage("wayland-diagnose-kde",
                        _("Detect GTK_IM_MODULE and QT_IM_MODULE being set and "
                          "Wayland Input method frontend is working. It is "
                          "recommended to unset GTK_IM_MODULE and QT_IM_MODULE "
                          "and use Wayland input method frontend instead. For "
                          "more details see "
                          "https://fcitx-im.org/wiki/"
                          "Using_Fcitx_5_on_Wayland#KDE_Plasma"));
        }
    } else if (desktop == DesktopType::GNOME) {
        if (instance_->currentUI() != "kimpanel") {
            sendMessage(
                "wayland-diagnose-gnome",
                _("It is recommended to install Input Method Panel GNOME "
                  "Shell Extensions to provide the input method popup. "
                  "https://extensions.gnome.org/extension/261/kimpanel/ "
                  "Otherwise you may not be able to see input method popup "
                  "when typing in GNOME Shell's activities search box. For "
                  "more details "
                  "see "
                  "https://fcitx-im.org/wiki/Using_Fcitx_5_on_Wayland#GNOME"));
        }
    } else {
        // It is not clear whether compositor is supported, only warn if wayland
        // im is being used..
        if (isWaylandIM) {
            // Sway's input popup is not merged.
            if (!gtkIM.empty() && gtkIM != "wayland" &&
                desktop != DesktopType::Sway) {
                messages.push_back(
                    _("Detect GTK_IM_MODULE being set and "
                      "Wayland Input method frontend is working. It is "
                      "recommended to unset GTK_IM_MODULE "
                      "and use Wayland input method frontend instead."));
            }
        }

        std::unordered_set<std::string> groupLayouts;
        for (const auto &groupName : instance_->inputMethodManager().groups()) {
            if (const auto *group =
                    instance_->inputMethodManager().group(groupName)) {
                groupLayouts.insert(group->defaultLayout());
            }
            if (groupLayouts.size() >= 2) {
                messages.push_back(
                    _("Sending keyboard layout configuration to wayland "
                      "compositor from Fcitx is "
                      "not yet supported on current desktop. You may still use "
                      "Fcitx's internal layout conversion by adding layout as "
                      "input method to the input method group."));
                break;
            }
        }

        if (!messages.empty()) {
            messages.push_back(
                _("For more details see "
                  "https://fcitx-im.org/wiki/Using_Fcitx_5_on_Wayland"));

            sendMessage("wayland-diagnose-other",
                        stringutils::join(messages, "\n"));
        }
    }
}

class WaylandModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new WaylandModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::WaylandModuleFactory);
