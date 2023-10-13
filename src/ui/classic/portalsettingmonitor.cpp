/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "portalsettingmonitor.h"
#include <memory>
#include <string>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/misc_p.h"
#include "common.h"

namespace fcitx {
namespace {
constexpr const char XDG_PORTAL_DESKTOP_SERVICE[] =
    "org.freedesktop.portal.Desktop";
constexpr const char XDG_PORTAL_DESKTOP_PATH[] =
    "/org/freedesktop/portal/desktop";
constexpr const char XDG_PORTAL_DESKTOP_SETTINGS_INTERFACE[] =
    "org.freedesktop.portal.Settings";
constexpr size_t PORTAL_RETRY_LIMIT = 3;
} // namespace

PortalSettingMonitor::PortalSettingMonitor(dbus::Bus &bus)
    : bus_(bus), serviceWatcher_(bus),
      watcherMap_(
          [this](const PortalSettingKey &key) {
              PortalSettingData data{};
              data.matchSlot = bus_.addMatch(
                  dbus::MatchRule(XDG_PORTAL_DESKTOP_SERVICE,
                                  XDG_PORTAL_DESKTOP_PATH,
                                  XDG_PORTAL_DESKTOP_SETTINGS_INTERFACE,
                                  "SettingChanged", {key.interface, key.name}),
                  [this, key](dbus::Message &msg) {
                      std::string interface, name;
                      msg >> interface >> name;
                      if (interface != key.interface || name != key.name) {
                          return true;
                      }

                      dbus::Variant variant;
                      msg >> variant;
                      if (variant.signature() == "v") {
                          variant = variant.dataAs<dbus::Variant>();
                      }
                      watcherData_[key].querySlot.reset();

                      auto view = watcherMap_.view(key);
                      for (auto &entry : view) {
                          entry(variant);
                      }
                      return false;
                  });
              if (!data.matchSlot) {
                  return false;
              }
              auto [iter, success] = watcherData_.emplace(
                  std::piecewise_construct, std::forward_as_tuple(key),
                  std::forward_as_tuple(std::move(data)));
              iter->second.querySlot = queryValue(key);
              return true;
          },
          [this](const PortalSettingKey &key) { watcherData_.erase(key); }) {

    serviceWatcherEntry_ = serviceWatcher_.watchService(
        XDG_PORTAL_DESKTOP_SERVICE,
        [this](const std::string &, const std::string &,
               const std::string &newOwner) {
            setPortalServiceOwner(newOwner);
        });
}

std::unique_ptr<PortalSettingEntry>
PortalSettingMonitor::watch(const std::string &interface,
                            const std::string &name,
                            PortalSettingCallback callback) {
    return watcherMap_.add({interface, name}, callback);
}

void PortalSettingMonitor::setPortalServiceOwner(const std::string &name) {
    if (serviceName_ == name) {
        return;
    }
    serviceName_ = name;
    if (serviceName_.empty()) {
        return;
    }

    CLASSICUI_INFO() << "A new portal show up, start a new query.";
    for (auto &[key, data] : watcherData_) {
        // Reset retry since it's a new service.
        data.retry = 0;
        data.querySlot = queryValue(key);
    }
}

std::unique_ptr<dbus::Slot>
PortalSettingMonitor::queryValue(const PortalSettingKey &key) {
    auto call = bus_.createMethodCall(
        XDG_PORTAL_DESKTOP_SERVICE, XDG_PORTAL_DESKTOP_PATH,
        XDG_PORTAL_DESKTOP_SETTINGS_INTERFACE, "Read");
    call << key.interface << key.name;
    return call.callAsync(5000000, [this, key](dbus::Message &msg) {
        // Key does not exist.
        auto data = findValue(watcherData_, key);
        if (!data) {
            return true;
        }
        // Key itself may be gone later, put it on the stack.
        // XDG portal seems didn't unwrap the variant.
        // Check this special case just in case.
        if (msg.isError()) {
            CLASSICUI_ERROR() << "DBus call error: " << msg.errorName() << " "
                              << msg.errorMessage();
            if (msg.errorName() == "org.freedesktop.DBus.Error.NoReply") {
                if (data->retry < PORTAL_RETRY_LIMIT) {
                    data->retry += 1;
                    data->querySlot = queryValue(key);
                } else {
                    CLASSICUI_ERROR()
                        << "Query portal value reaches retry limit.";
                }
            }
            return true;
        }
        if (msg.signature() != "v") {
            return true;
        }
        dbus::Variant variant;
        msg >> variant;
        if (variant.signature() == "v") {
            variant = variant.dataAs<dbus::Variant>();
        }
        auto view = watcherMap_.view(key);
        for (auto &entry : view) {
            entry(variant);
        }
        data->querySlot.reset();
        return false;
    });
}

} // namespace fcitx