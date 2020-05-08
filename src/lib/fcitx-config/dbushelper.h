/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_DBUSHELPER_H_
#define _FCITX_CONFIG_DBUSHELPER_H_

#include <vector>
#include <fcitx-config/configuration.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/dbus/message.h>
#include "fcitxconfig_export.h"

namespace fcitx {

// a{sv}
using DBusVariantMap = std::vector<dbus::DictEntry<std::string, dbus::Variant>>;
// name, type, description, defaultValue, constrain.
// (sssva{sv})
using DBusConfigOption = dbus::DBusStruct<std::string, std::string, std::string,
                                          dbus::Variant, DBusVariantMap>;
// a(sa(sssva{sv}))
using DBusConfig =
    std::vector<dbus::DBusStruct<std::string, std::vector<DBusConfigOption>>>;

FCITXCONFIG_EXPORT dbus::Variant rawConfigToVariant(const RawConfig &config);
FCITXCONFIG_EXPORT RawConfig variantToRawConfig(const dbus::Variant &map);
FCITXCONFIG_EXPORT DBusConfig
dumpDBusConfigDescription(const Configuration &config);

} // namespace fcitx

#endif // _FCITX_CONFIG_DBUSHELPER_H_
