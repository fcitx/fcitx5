/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "portalappmonitor.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx/misc_p.h"

namespace fcitx {

constexpr const char KDE_PORTAL[] = "org.freedesktop.impl.portal.desktop.kde";

fcitx::PortalAppMonitor::PortalAppMonitor(dbus::Bus *bus)
    : bus_(bus), watcher_(*bus) {
    auto serviceChanged = [this](const std::string &serviceName,
                                 const std::string &oldOwner,
                                 const std::string &newOwner) {
        FCITX_INFO() << "PORTAL CHANGED " << serviceName << " " << oldOwner
                     << " " << newOwner;
        if (newOwner.empty()) {
            if (oldOwner == currentOwner_) {
                currentOwner_ = std::string();
                changedSignalSlot_.reset();
            }
            return;
        }

        if (currentOwner_ != newOwner) {
            currentOwner_ = newOwner;
            changedSignalSlot_ = bus_->addMatch(
                dbus::MatchRule(newOwner, "/org/freedesktop/portal/desktop",
                                "org.freedesktop.impl.portal.Background",
                                "RunningApplicationsChanged"),
                [this](dbus::Message &) {
                    refreshAppState();
                    return true;
                });
        }
    };

    entries_.push_back(watcher_.watchService(
        "org.freedesktop.impl.portal.desktop.kde", serviceChanged));
}

void PortalAppMonitor::refreshAppState() {
    if (currentOwner_.empty()) {
        return;
    }
    FCITX_INFO() << "Call GetAppState " << currentOwner_;
    auto msg = bus_->createMethodCall(
        currentOwner_.data(), "/org/freedesktop/portal/desktop",
        "org.freedesktop.impl.portal.Background", "GetAppState");
    getAppStateSlot_ = msg.callAsync(0, [this](dbus::Message &msg) {
        auto pivot = std::move(getAppStateSlot_);
        if (msg.isError()) {
            return true;
        }
        FCITX_INFO() << "GetAppState reply: " << msg.signature();
        if (msg.signature() != "a{sv}") {
            return true;
        }
        std::vector<dbus::DictEntry<std::string, dbus::Variant>> result;
        msg >> result;
        appState_.clear();
        for (const auto &item : result) {
            if (item.value().signature() != "u") {
                continue;
            }
            appState_[item.key()] = item.value().dataAs<uint32_t>();
        }
        FCITX_INFO() << appState_;
        return true;
    });
}

} // namespace fcitx
