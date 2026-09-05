/*
 * SPDX-FileCopyrightText: 2026~2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "gnomeappmonitor.h"
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "fcitx-utils/coroutine.h"
#include "fcitx-utils/dbus/coroutine.h"
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
    shellName_ = name;
    init();
}

void GnomeAppMonitor::setPortal(const std::string &name) {
    if (portalName_ == name) {
        return;
    }
    portalName_ = name;
    init();
}

void GnomeAppMonitor::init() {
    reset();

    if (portalName_.empty() || shellName_.empty()) {
        return;
    }
    FCITX_IBUS_DEBUG() << "Init GNOME app monitor, shell DBus Name: "
                       << shellName_
                       << " GNOME XDG portal dbus name: " << portalName_;

    // Fire the init task.
    initShellTask_ = initShell();
    initShellTask_->resume();
}

Coroutine<void> GnomeAppMonitor::initShell() {
    try {
        auto pid = co_await dbus::AsyncReturn<uint32_t>(
            std::move(bus_->createMethodCall(
                          "org.freedesktop.DBus", "/org/freedesktop/DBus",
                          "org.freedesktop.DBus", "GetConnectionUnixProcessID")
                      << shellName_));

        FCITX_IBUS_DEBUG() << "GNOME Shell pid set to " << pid;
        initMonitorBus();

        // This needs to be after init monitor bus, otherwise message might mess
        // the connection state.
        co_await dbus::AsyncCall(
            std::move(monitorBus_->createMethodCall(
                          "org.freedesktop.DBus", "/org/freedesktop/DBus",
                          "org.freedesktop.DBus.Monitoring", "BecomeMonitor")
                      << std::vector<std::string>{replyRule_.rule(),
                                                  getRunningAppRule_.rule()}
                      << static_cast<uint32_t>(0U)));

        auto value = co_await dbus::AsyncReturn<dbus::Variant>(std::move(
            bus_->createMethodCall(shellName_.data(), "/org/gnome/Shell",
                                   "org.freedesktop.DBus.Properties", "Get")
            << "org.gnome.Shell" << "OverviewActive"));

        if (value.signature() == "b") {
            bool overviewActive = value.dataAs<bool>();
            overviewActive_ = overviewActive;
        }

        shellPid_ = pid;
        shellNameChanged();
        updateState();
    } catch (const std::exception &e) {
        FCITX_IBUS_DEBUG() << "Failed initial shell" << e.what();
        reset();
    }
    // Allow automatic clean up the task after return.
    assert(initShellTask_.has_value());
    std::move(*initShellTask_).detach_handle();
}

void GnomeAppMonitor::reset() {
    // reset resources
    monitorBus_.reset();
    filter_.reset();
    shellPid_.reset();
    propertyChangedSlot_.reset();
    overviewActive_ = false;

    // reset state.
    apps_.clear();
    focus_.clear();
}

void GnomeAppMonitor::initMonitorBus() {
    monitorBus_ = std::make_unique<dbus::Bus>(bus_->address());
    monitorBus_->attachEventLoop(bus_->eventLoop());

    propertyChangedSlot_ = bus_->addMatch(
        shellPropertyChangedRule_, [this](dbus::Message &message) {
            std::string interface;
            std::vector<dbus::DictEntry<std::string, dbus::Variant>>
                changedProperties;
            std::vector<std::string> invalidatedProperties;
            message >> interface >> changedProperties >> invalidatedProperties;
            for (const auto &entry : changedProperties) {
                if (entry.key() == "OverviewActive") {
                    if (entry.value().signature() == "b") {
                        overviewActive_ = entry.value().dataAs<bool>();
                        updateState();
                    }
                    return true;
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
            }
            return true;
        }
        return false;
    });
}

bool GnomeAppMonitor::isAvailable() const { return !monitorBus_; }

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
