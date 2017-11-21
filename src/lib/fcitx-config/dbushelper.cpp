/*
* Copyright (C) 2017~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; see the file COPYING. If not,
* see <http://www.gnu.org/licenses/>.
*/

#include "dbushelper.h"
#include "fcitx-utils/dbus/variant.h"

namespace fcitx {

namespace {

void variantFillRawConfig(const dbus::Variant &variant, RawConfig &config) {
    if (variant.signature() == "s") {
        config = variant.dataAs<std::string>();
        return;
    }
    if (variant.signature() != "a{sv}") {
        return;
    }
    const DBusVariantMap &map = variant.dataAs<DBusVariantMap>();
    for (const auto &entry : map) {
        if (entry.key().empty()) {
            config = entry.value().dataAs<std::string>();
        } else {
            auto subConfig = config.get(entry.key(), true);
            variantFillRawConfig(entry.value(), *subConfig);
        }
    }
}
}

dbus::Variant rawConfigToVariant(const RawConfig &config) {
    if (!config.hasSubItems()) {
        return dbus::Variant{config.value()};
    }

    DBusVariantMap map;
    if (!config.value().empty()) {
        map.emplace_back("", dbus::Variant(config.value()));
    }
    if (config.hasSubItems()) {
        auto options = config.subItems();
        for (auto &option : options) {
            auto subConfig = config.get(option);
            map.emplace_back(option, rawConfigToVariant(*subConfig));
        }
    }
    return dbus::Variant{std::move(map)};
}

RawConfig variantToRawConfig(const dbus::Variant &map) {
    RawConfig config;
    variantFillRawConfig(map, config);
    return config;
}

DBusConfig dumpDBusConfigDescription(const RawConfig &config) {
    DBusConfig result;
    // First level will always be type.
    // Second level will always be option.
    auto types = config.subItems();
    for (auto &type : types) {
        result.emplace_back();
        auto &dbusType = result.back();
        std::get<0>(dbusType) = type;
        auto &dbusOptions = std::get<1>(dbusType);
        auto typeConfig = config.get(type);
        auto options = typeConfig->subItems();
        for (auto &option : options) {
            auto optionConfig = typeConfig->get(option);
            auto typeField = optionConfig->get("Type");
            auto descField = optionConfig->get("Description");
            auto defaultValueField = optionConfig->get("DefaultValue");
            if (!typeField || !descField || !defaultValueField) {
                continue;
            }
            dbusOptions.emplace_back();
            auto &dbusOption = dbusOptions.back();
            std::get<0>(dbusOption) = option;
            std::get<1>(dbusOption) = typeField->value();
            std::get<2>(dbusOption) = descField->value();
            std::get<3>(dbusOption) = rawConfigToVariant(*defaultValueField);
            optionConfig->visitSubItems([&dbusOption](const RawConfig &config,
                                                      const std::string &path) {
                // skip field that already handled.
                if (path == "Type" || path == "Description" ||
                    path == "DefaultValue") {
                    return true;
                }
                auto &extra = std::get<4>(dbusOption);
                extra.emplace_back(path, rawConfigToVariant(config));
                return true;
            });
        }
    }
    return result;
}

DBusConfig dumpDBusConfigDescription(const Configuration &config) {
    RawConfig rawDesc;
    config.dumpDescription(rawDesc);
    return dumpDBusConfigDescription(rawDesc);
}

} // namespace fcitx
