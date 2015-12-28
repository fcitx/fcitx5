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

#include "marshallfunction.h"
#include "configuration.h"

namespace fcitx
{
void marshallOption(RawConfig& config, const int value)
{
    config = std::to_string(value);
}

bool unmarshallOption(int& value, const RawConfig& config)
{
    try {
        value = std::stoi(config.value());
    } catch (std::invalid_argument) {
        return false;
    } catch (std::out_of_range) {
        return false;
    }

    return true;
}

void marshallOption(RawConfig& config, const std::string& value)
{
    config = value;
}

bool unmarshallOption(std::string& value, const RawConfig& config)
{
    value = config.value();
    return true;
}

void marshallOption(RawConfig& config, const Key& value)
{
    config = value.toString();
}

bool unmarshallOption(Key& value, const RawConfig& config)
{
    value = Key(config.value());
    return true;
}

void marshallOption(RawConfig& config, const Color& value)
{
    config = value.toString();
}

bool unmarshallOption(Color& value, const RawConfig& config)
{
    try {
        value = Color(config.value());
    } catch (ColorParseException) {
        return false;
    }
    return true;
}

void marshallOption(RawConfig& config, const Configuration& value)
{
    value.save(config);
}

bool unmarshallOption(Configuration& value, const RawConfig& config)
{
    value.load(config);
    return true;
}


}
