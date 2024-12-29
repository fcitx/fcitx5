/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "dbushelper.h"
#include <string>
#include <utility>
#include "fcitx-utils/dbus/variant.h"
#include "configuration.h"
#include "rawconfig.h"

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
    const auto &map = variant.dataAs<DBusVariantMap>();
    for (const auto &entry : map) {
        if (entry.key().empty()) {
            config = entry.value().dataAs<std::string>();
        } else {
            auto subConfig = config.get(entry.key(), true);
            variantFillRawConfig(entry.value(), *subConfig);
        }
    }
}
} // namespace

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
            if (!typeField || !descField) {
                continue;
            }
            dbusOptions.emplace_back();
            auto &dbusOption = dbusOptions.back();
            std::get<0>(dbusOption) = option;
            std::get<1>(dbusOption) = typeField->value();
            std::get<2>(dbusOption) = descField->value();
            std::get<3>(dbusOption) = rawConfigToVariant(
                defaultValueField ? *defaultValueField : RawConfig());
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
