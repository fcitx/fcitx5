//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
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
