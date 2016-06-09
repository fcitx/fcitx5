/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_CONFIG_INTOPTION_H_
#define _FCITX_CONFIG_INTOPTION_H_

#include "rawconfig.h"
#include <fcitx-utils/key.h>
#include <fcitx-utils/color.h>
#include <vector>
#include <type_traits>

namespace fcitx {

class Configuration;

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const bool value);
FCITXCONFIG_EXPORT bool unmarshallOption(bool &value, const RawConfig &config);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const int value);
FCITXCONFIG_EXPORT bool unmarshallOption(int &value, const RawConfig &config);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const std::string &value);
FCITXCONFIG_EXPORT bool unmarshallOption(std::string &value, const RawConfig &config);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const Key &value);
FCITXCONFIG_EXPORT bool unmarshallOption(Key &value, const RawConfig &config);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const Color &value);
FCITXCONFIG_EXPORT bool unmarshallOption(Color &value, const RawConfig &config);

FCITXCONFIG_EXPORT void marshallOption(RawConfig &config, const Configuration &value);
FCITXCONFIG_EXPORT bool unmarshallOption(Configuration &value, const RawConfig &config);

template <typename T>
void marshallOption(RawConfig &config, const std::vector<T> &value) {
    config.removeAll();
    marshallOption(config["Length"], static_cast<int>(value.size()));
    for (size_t i = 0; i < value.size(); i++) {
        marshallOption(config[std::to_string(i)], value[i]);
    }
}

template <typename T>
bool unmarshallOption(std::vector<T> &value, const RawConfig &config) {
    int size;
    auto lenSubConfigPtr = config.get("Length");
    if (!lenSubConfigPtr || !unmarshallOption(size, *lenSubConfigPtr)) {
        return false;
    }
    if (size < 0) {
        return false;
    }

    value.resize(size);
    for (int i = 0; i < size; i++) {
        auto subConfigPtr = config.get(std::to_string(i));
        if (!subConfigPtr) {
            return false;
        }

        if (!unmarshallOption(value[i], *subConfigPtr)) {
            return false;
        }
    }
    return true;
}
}

#endif // _FCITX_CONFIG_INTOPTION_H_
