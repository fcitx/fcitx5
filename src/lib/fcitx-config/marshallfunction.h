/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_INTOPTION_H_
#define _FCITX_CONFIG_INTOPTION_H_

#include <vector>
#include <fcitx-utils/color.h>
#include <fcitx-utils/i18nstring.h>
#include <fcitx-utils/semver.h>
#include "rawconfig.h"

namespace fcitx {

class Configuration;

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, bool value);
FCITXCONFIG_EXPORT bool unmarshallOption(bool &value, const RawConfig &config,
                                         bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, int value);
FCITXCONFIG_EXPORT bool unmarshallOption(int &value, const RawConfig &config,
                                         bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config,
                                       const std::string &value);
FCITXCONFIG_EXPORT bool unmarshallOption(std::string &value,
                                         const RawConfig &config, bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config,
                                       const SemanticVersion &value);
FCITXCONFIG_EXPORT bool unmarshallOption(SemanticVersion &value,
                                         const RawConfig &config, bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const Key &value);
FCITXCONFIG_EXPORT bool unmarshallOption(Key &value, const RawConfig &config,
                                         bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const Color &value);
FCITXCONFIG_EXPORT bool unmarshallOption(Color &value, const RawConfig &config,
                                         bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config,
                                       const I18NString &value);
FCITXCONFIG_EXPORT bool unmarshallOption(I18NString &value,
                                         const RawConfig &config, bool partial);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config,
                                       const Configuration &value);
FCITXCONFIG_EXPORT bool unmarshallOption(Configuration &value,
                                         const RawConfig &config, bool partial);

template <typename T>
void marshallOption(RawConfig &config, const std::vector<T> &value) {
    config.removeAll();
    for (size_t i = 0; i < value.size(); i++) {
        marshallOption(config[std::to_string(i)], value[i]);
    }
}

template <typename T>
bool unmarshallOption(std::vector<T> &value, const RawConfig &config,
                      bool partial) {
    value.clear();
    int i = 0;
    while (true) {
        auto subConfigPtr = config.get(std::to_string(i));
        if (!subConfigPtr) {
            break;
        }

        value.emplace_back();

        if (!unmarshallOption(value[i], *subConfigPtr, partial)) {
            return false;
        }
        i++;
    }
    return true;
}
} // namespace fcitx

#endif // _FCITX_CONFIG_INTOPTION_H_
