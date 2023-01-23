/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/dbus/variant.h"
#include "fcitx/misc_p.h"
#include "appmonitor.h"

namespace fcitx {

constexpr const char GNOME_PORTAL[] =
    "org.freedesktop.impl.portal.desktop.gnome";
constexpr const char GTK_PORTAL[] = "org.freedesktop.impl.portal.desktop.gtk";
constexpr const char GNOME_SHELL_NAME[] = "org.gnome.Shell";

GnomeAppMonitor::GnomeAppMonitor(dbus::Bus *bus) : bus_(bus), watcher_(*bus) {
    auto serviceChanged = [this](const std::string &serviceName,
                                 const std::string &,
                                 const std::string &newOwner) {
        bool changed = false;
        if (serviceName == GTK_PORTAL) {
            if (newOwner != gtkPortalOwner_) {
                gtkPortalOwner_ = newOwner;
                changed = true;
            }
        } else if (serviceName == GNOME_PORTAL) {
            if (newOwner != gnomePortalOwner_) {
                gnomePortalOwner_ = newOwner;
                changed = true;
            }
        } else if (serviceName == GNOME_SHELL_NAME) {
            if (newOwner != gnomeShellOwner_) {
                gnomeShellOwner_ = newOwner;
                changed = true;
            }
        }
        if (changed) {
            recreateMonitorBus();
        }
    };

    entries_.push_back(watcher_.watchService(GNOME_PORTAL, serviceChanged));
    entries_.push_back(watcher_.watchService(GTK_PORTAL, serviceChanged));
    entries_.push_back(watcher_.watchService(GNOME_SHELL_NAME, serviceChanged));
}

void GnomeAppMonitor::refreshAppState(dbus::Message &msg) {
    if (msg.signature() != "a{sa{sv}}") {
        return;
    }
    std::vector<dbus::DictEntry<
        std::string, std::vector<dbus::DictEntry<std::string, dbus::Variant>>>>
        result;
    msg >> result;
    appState_.clear();
    for (const auto &item : result) {
        bool active = std::any_of(
            item.value().begin(), item.value().end(),
            [](const dbus::DictEntry<std::string, dbus::Variant> &prop) {
                return prop.key() == "active-on-seats";
            });
        if (active) {
            appState_[item.key()] = 2;
        } else {
            appState_[item.key()] = 1;
        }
    }
    FCITX_INFO() << appState_;
}

void GnomeAppMonitor::recreateMonitorBus() {
    reply_.reset();
    monitorBus_.reset();
    std::vector<std::string> rules;
    bool gtkPortalReady = !gtkPortalOwner_.empty() && !gnomeShellOwner_.empty();
    bool gnomePortalReady =
        !gnomePortalOwner_.empty() && !gnomeShellOwner_.empty();

    if (gtkPortalReady) {
        rules.push_back(dbus::MatchRule(dbus::MessageType::Reply,
                                        gnomeShellOwner_, gtkPortalOwner_)
                            .rule());
    }

    if (gnomePortalReady) {
        rules.push_back(dbus::MatchRule(dbus::MessageType::Reply,
                                        gnomeShellOwner_, gnomePortalOwner_)
                            .rule());
    }

    if (rules.empty()) {
        return;
    }

    monitorBus_ = std::make_unique<dbus::Bus>(bus_->address());
    monitorBus_->attachEventLoop(bus_->eventLoop());
    auto msg = monitorBus_->createMethodCall(
        "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus.Monitoring", "BecomeMonitor");
    msg << rules;
    msg << static_cast<uint32_t>(0u);
    reply_ = msg.callAsync(0, [this](dbus::Message &msg) {
        if (!msg.isError()) {
            filter_ = monitorBus_->addFilter([this](dbus::Message &msg) {
                FCITX_INFO() << msg.signature();
                FCITX_INFO() << msg.sender();
                FCITX_INFO() << msg.destination();
                FCITX_INFO() << static_cast<int>(msg.type());
                refreshAppState(msg);
                return true;
            });
        } else {
            monitorBus_.reset();
        }
        reply_.reset();
        return true;
    });
}

} // namespace fcitx
