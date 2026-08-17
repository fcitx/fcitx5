/*
 * SPDX-FileCopyrightText: 2026~2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "gnomeappmonitor.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "fcitx-utils/dbus/matchrule.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/log.h"
#include "common.h"

namespace fcitx {

GnomeAppMonitor::GnomeAppMonitor(dbus::Bus *bus)
    : bus_(bus), serviceWatcher_(std::make_unique<dbus::ServiceWatcher>(*bus)) {
    FCITX_IBUS_DEBUG() << "GnomeAppMonitor created";
    handleShell_ = serviceWatcher_->watchService(
        std::string(introspectName),
        [this](const std::string & /*service*/,
               const std::string & /*oldOwner*/,
               const std::string &newOwner) { setShell(newOwner); });
    handlePortal_ = serviceWatcher_->watchService(
        std::string(gnomePortalName),
        [this](const std::string & /*service*/,
               const std::string & /*oldOwner*/,
               const std::string &newOwner) { setPortal(newOwner); });
}

GnomeAppMonitor::~GnomeAppMonitor() = default;

void GnomeAppMonitor::setShell(const std::string &name) {
    if (shellName_ == name) {
        return;
    }
    shellPid_.reset();
    shellName_ = name;
    if (shellName_.empty()) {
        refreshState();
        return;
    }

    auto msg = bus_->createMethodCall(
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
        "GetConnectionUnixProcessID");

    msg << shellName_;
    shellPidSlot_ = msg.callAsync(0, [this, name](dbus::Message &message) {
        if (message.isError()) {
            return true;
        }
        if (shellName_ != name || message.signature() != "u") {
            return true;
        }
        uint32_t pid;
        message >> pid;

        setShellPid(pid);
        return true;
    });
}

void GnomeAppMonitor::setShellPid(uint32_t pid) {
    shellPid_ = pid;
    FCITX_IBUS_DEBUG() << "GNOME Shell pid set to " << pid;

    shellNameChanged();
    apps_.clear();
    focus_.clear();
    overviewActive_ = false;
    if (!shellName_.empty()) {
        getOverviewActive();
    }
    refreshState();
}

void GnomeAppMonitor::setPortal(const std::string &name) {
    if (portalName_ != name) {
        portalName_ = name;
        refreshState();
    }
}

void GnomeAppMonitor::refreshState() {
    if (shellName_.empty() || portalName_.empty() || !shellPid_.has_value()) {
        monitorBus_.reset();
        filter_.reset();
    } else {
        monitorBus_ = std::make_unique<dbus::Bus>(bus_->address());
        monitorBus_->attachEventLoop(bus_->eventLoop());

        propertyChangedSlot_ = bus_->addMatch(
            shellPropertyChangedRule_, [this](dbus::Message &message) {
                std::string interface;
                std::vector<dbus::DictEntry<std::string, dbus::Variant>>
                    changedProperties;
                std::vector<std::string> invalidatedProperties;
                message >> interface >> changedProperties >>
                    invalidatedProperties;
                for (const auto &entry : changedProperties) {
                    if (entry.key() == "OverviewActive") {
                        if (entry.value().signature() == "b") {
                            overviewActive_ = entry.value().dataAs<bool>();
                            updateState();
                        }
                        return true;
                    }
                }
                for (const auto &property : invalidatedProperties) {
                    if (property == "OverviewActive") {
                        getOverviewActive();
                        break;
                    }
                }
                return true;
            });
        filter_ = monitorBus_->addFilter([this](dbus::Message &message) {
            if (message.type() == dbus::MessageType::MethodCall &&
                getRunningAppRule_.check(message, portalName_) &&
                message.destination() == shellName_) {
                lastSerial_ = message.serial();
                return true;
            }
            if (message.type() == dbus::MessageType::Reply &&
                replyRule_.check(message, shellName_) &&
                message.destination() == portalName_) {
                if (message.replySerial() == lastSerial_) {
                    lastSerial_ = 0;
                    if (message.signature() == "a{sa{sv}}") {
                        std::vector<dbus::DictEntry<
                            std::string, std::vector<dbus::DictEntry<
                                             std::string, dbus::Variant>>>>
                            result;
                        message >> result;

                        std::string focus;
                        std::unordered_set<std::string> apps;
                        for (const auto &entry : result) {
                            apps.insert(entry.key());
                            for (const auto &property : entry.value()) {
                                if (property.key() == "active-on-seats") {
                                    focus = entry.key();
                                }
                            }
                        }
                        if (apps_ != apps || focus_ != focus) {
                            apps_ = std::move(apps);
                            focus_ = std::move(focus);
                            updateState();
                        }
                    }
                    refreshState();
                }
                return true;
            }
            return false;
        });

        // This needs to be after addFilter, otherwise message might mess the
        // connection state.
        auto call = monitorBus_->createMethodCall(
            "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus.Monitoring", "BecomeMonitor");
        call << std::vector<std::string>{replyRule_.rule(),
                                         getRunningAppRule_.rule()}
             << static_cast<uint32_t>(0U);

        auto slotHolder = std::make_shared<std::unique_ptr<dbus::Slot>>();
        *slotHolder = call.callAsync(
            0, [slotHolder](dbus::Message &message) { return true; });
    }
}

bool GnomeAppMonitor::isAvailable() const { return !monitorBus_; }

void GnomeAppMonitor::getOverviewActive() {
    if (!monitorBus_) {
        return;
    }
    auto call =
        monitorBus_->createMethodCall(shellDBusName, "/org/gnome/Shell",
                                      "org.freedesktop.DBus.Properties", "Get");
    call << "org.gnome.Shell" << "OverviewActive";
    getSlot_ = call.callAsync(0, [this](dbus::Message &message) {
        if (message.type() == dbus::MessageType::Reply &&
            message.signature() == "v") {
            dbus::Variant value;
            message >> value;
            if (value.signature() == "b") {
                bool overviewActive = value.dataAs<bool>();
                if (overviewActive_ != overviewActive) {
                    overviewActive_ = overviewActive;
                    updateState();
                }
            }
        }
        getSlot_.reset();
        return true;
    });
}

void GnomeAppMonitor::updateState() {
    std::unordered_map<std::string, std::string> state;
    std::optional<std::string> focus;
    for (const auto &appId : apps_) {
        state[appId] = appId;
    }
    // Always add a dummy gnome-shell overview state.
    state["gnome-shell-overview"] = "gnome-shell";
    if (overviewActive_) {
        focus = "gnome-shell-overview";
    } else if (!focus_.empty()) {
        focus = focus_;
    }
    FCITX_IBUS_DEBUG() << "GnomeAppMonitor state updated, focus: " << focus
                       << ", apps: " << state;
    appUpdated(state, focus);
}

} // namespace fcitx
