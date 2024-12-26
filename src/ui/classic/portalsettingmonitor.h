/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_PORTALSETTINGMONITOR_H_
#define _FCITX_UI_CLASSIC_PORTALSETTINGMONITOR_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/handlertable_details.h"
#include "fcitx-utils/macros.h"
#include "fcitx/misc_p.h"

namespace fcitx {

struct PortalSettingKey {
    std::string interface;
    std::string name;

    bool operator==(const PortalSettingKey &other) const {
        return interface == other.interface && name == other.name;
    }
};

} // namespace fcitx

template <>
struct std::hash<fcitx::PortalSettingKey> {
    std::size_t operator()(const fcitx::PortalSettingKey &key) const noexcept {
        std::size_t seed = 0;
        fcitx::hash_combine(seed, std::hash<std::string>()(key.interface));
        fcitx::hash_combine(seed, std::hash<std::string>()(key.name));
        return seed;
    }
};

namespace fcitx {

using PortalSettingCallback = std::function<void(const dbus::Variant &)>;
using PortalSettingEntry = HandlerTableEntry<PortalSettingCallback>;

class PortalSettingMonitor {
    struct PortalSettingData {
        std::unique_ptr<dbus::Slot> matchSlot;
        std::unique_ptr<dbus::Slot> querySlot;
        size_t retry = 0;
    };

public:
    PortalSettingMonitor(dbus::Bus &bus);

    FCITX_NODISCARD std::unique_ptr<PortalSettingEntry>
    watch(const std::string &interface, const std::string &name,
          PortalSettingCallback callback);

private:
    void setPortalServiceOwner(const std::string &name);
    std::unique_ptr<dbus::Slot> queryValue(const PortalSettingKey &key);
    dbus::Bus &bus_;
    std::string serviceName_;
    dbus::ServiceWatcher serviceWatcher_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>>
        serviceWatcherEntry_;
    MultiHandlerTable<PortalSettingKey, PortalSettingCallback> watcherMap_;
    std::unordered_map<PortalSettingKey, PortalSettingData> watcherData_;
};

} // namespace fcitx

#endif