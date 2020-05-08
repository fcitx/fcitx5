/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "marshallfunction.h"
#include "fcitx-utils/stringutils.h"
#include "configuration.h"

namespace fcitx {
void marshallOption(RawConfig &config, const bool value) {
    config = value ? "True" : "False";
}

bool unmarshallOption(bool &value, const RawConfig &config, bool) {
    if (config.value() == "True" || config.value() == "False") {
        value = config.value() == "True";
        return true;
    }

    return false;
}
void marshallOption(RawConfig &config, const int value) {
    config = std::to_string(value);
}

bool unmarshallOption(int &value, const RawConfig &config, bool) {
    try {
        value = std::stoi(config.value());
    } catch (const std::exception &) {
        return false;
    }

    return true;
}

void marshallOption(RawConfig &config, const std::string &value) {
    config = value;
}

bool unmarshallOption(std::string &value, const RawConfig &config, bool) {
    value = config.value();
    return true;
}

void marshallOption(RawConfig &config, const Key &value) {
    config = value.toString();
}

bool unmarshallOption(Key &value, const RawConfig &config, bool) {
    value = Key(config.value());
    return true;
}

void marshallOption(RawConfig &config, const Color &value) {
    config = value.toString();
}

bool unmarshallOption(Color &value, const RawConfig &config, bool) {
    try {
        value = Color(config.value());
    } catch (const ColorParseException &) {
        return false;
    }
    return true;
}

void marshallOption(RawConfig &config, const I18NString &value) {
    config = value.defaultString();
    for (auto &p : value.localizedStrings()) {
        (*config.parent())[stringutils::concat(config.name(), "[", p.first,
                                               "]")] = p.second;
    }
}

bool unmarshallOption(I18NString &value, const RawConfig &config, bool) {
    value.clear();
    value.set(config.value());
    if (!config.parent()) {
        return true;
    }
    config.parent()->visitSubItems([&value, &config](const RawConfig &config_,
                                                     const std::string &path) {
        if (stringutils::startsWith(path, config.name() + "[") &&
            stringutils::endsWith(path, "]")) {
            auto locale = path.substr(config.name().size() + 1,
                                      path.size() - config.name().size() - 2);
            value.set(config_.value(), locale);
        }
        return true;
    });
    return true;
}

void marshallOption(RawConfig &config, const Configuration &value) {
    value.save(config);
}

bool unmarshallOption(Configuration &value, const RawConfig &config,
                      bool partial) {
    value.load(config, partial);
    return true;
}
} // namespace fcitx
