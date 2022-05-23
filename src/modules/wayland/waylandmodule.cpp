/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandmodule.h"
#include <stdexcept>
#include <gio/gio.h>
#include <wayland-client.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "config.h"
#include "wl_seat.h"

#ifdef ENABLE_DBUS
#include "dbus_public.h"
#endif

#ifdef ENABLE_X11
#include "xcb_public.h"
#endif

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(wayland_log, "wayland");

#define FCITX_WAYLAND_DEBUG() FCITX_LOGC(::fcitx::wayland_log, Debug)

namespace {
bool isKDE5() {
    static const DesktopType desktop = getDesktopType();
    return desktop == DesktopType::KDE5;
}

class ScopedEnvvar {
public:
    explicit ScopedEnvvar(std::string name, const char* value) : name_(std::move(name)) {
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

    auto &eventLoop = parent_->instance()->eventLoop();
    ioEvent_ =
        eventLoop.addIOEvent(display_->fd(), IOEventFlag::In,
                             [this](EventSource *, int, IOEventFlags flags) {
                                 onIOEvent(flags);
                                 return true;
                             });

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
}

void WaylandConnection::finish() { parent_->removeConnection(name_); }

void WaylandConnection::setupKeyboard(wayland::WlSeat *seat) {
    auto &kbd = (keyboards_[seat] = std::make_unique<WaylandKeyboard>(seat));
    kbd->updateKeymap().connect([this]() {
        FCITX_WAYLAND_DEBUG() << "Update keymap";
        parent_->reloadXkbOption();
    });
}

void WaylandConnection::onIOEvent(IOEventFlags flags) {
    if ((flags & IOEventFlag::Err) || (flags & IOEventFlag::Hup)) {
        return finish();
    }

    if (wl_display_prepare_read(*display_) == 0) {
        if (flags & IOEventFlag::In) {
            wl_display_read_events(*display_);
        } else {
            wl_display_cancel_read(*display_);
        }
    }

    if (wl_display_dispatch(*display_) < 0) {
        error_ = wl_display_get_error(*display_);
        FCITX_LOGC_IF(wayland_log, Error, error_ != 0)
            << "Wayland connection got error: " << error_;
        if (error_ != 0) {
            return finish();
        }
    }

    wl_display_flush(*display_);
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
            if (!isKDE5() || !isWaylandSession_ || !*config_.allowOverrideXKB) {
                return;
            }

            auto connection = findValue(conns_, "");
            if (!connection) {
                return;
            }

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
            config.setValueByPath("Layout/VariantList",
                                  layoutAndVariant.second);
            config.setValueByPath("Layout/DisplayNames", "");
            config.setValueByPath("Layout/Use", "true");

            safeSaveAsIni(config, StandardPath::Type::Config, "kxkbrc");

            auto bus = dbusAddon->call<IDBusModule::bus>();
            auto message = bus->createSignal("/Layouts", "org.kde.keyboard",
                                             "reloadConfig");
            message.send();
        }));
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
    FCITX_WAYLAND_DEBUG() << "Connection removed " << name;
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
        auto settings = g_settings_new("org.gnome.desktop.input-sources");
        if (settings) {

            gchar **value = g_settings_get_strv(settings, "xkb-options");
            if (value) {
                auto options = g_strjoinv(",", value);
                xkbOption = (options ? options : "");
                instance_->setXkbParameters(connection->focusGroup()->display(),
                                            DEFAULT_XKB_RULES, "", *xkbOption);
                FCITX_WAYLAND_DEBUG() << "GNOME xkb options=" << *xkbOption;
                g_free(options);
            }
            g_object_unref(settings);
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

class WaylandModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new WaylandModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::WaylandModuleFactory);
