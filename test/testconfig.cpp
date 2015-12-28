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

#include <cassert>
#include <fcitx-config/configuration.h>
#include <fcitx-config/iniparser.h>
#include <iostream>
#include <vector>

FCITX_CONFIGURATION(TestSubSubConfig,
    FCITX_OPTION(intValue, "IntOption", int, 1, "Int Option")
    FCITX_OPTION(keyValue, "KeyOption", fcitx::Key, fcitx::Key(FcitxKey_A, fcitx::KeyState::Ctrl), "Key Option")
);

FCITX_CONFIGURATION(TestSubConfig,
    FCITX_OPTION(intValue, "IntOption", int, 1, "Int Option")
    FCITX_OPTION(colorValue, "KeyOption", fcitx::Key, fcitx::Key(), "Key Option")
    FCITX_OPTION(subSubVectorConfigValue, "SubSubConfig", std::vector<TestSubSubConfig>, [] () {
        std::vector<TestSubSubConfig> value;
        value.resize(2);
        value[0].intValue.setValue(2);
        value[0].keyValue.setValue(fcitx::Key("Alt+b"));
        return value;
    }(), "SubConfig Option");
);

FCITX_CONFIGURATION(TestConfig,
    FCITX_OPTION(intValue, "IntOption", int, 0, "Int Option");
    FCITX_OPTION(colorValue, "ColorOption", fcitx::Color, fcitx::Color(), "Color Option");
    FCITX_OPTION(stringValue, "StringOption", std::string, ("Test String"), "String Option");
    FCITX_OPTION(stringVectorValue, "StringVectorOption", std::vector<std::string>, std::vector<std::string>({"ABC", "CDE"}), "String Option");
    FCITX_OPTION(SubConfigValue, "SubConfig", TestSubConfig, TestSubConfig(), "SubConfig Option");
)

int main()
{
    TestConfig config;
    fcitx::RawConfig rawConfig;
    config.save(rawConfig);

    fcitx::writeAsIni(rawConfig, std::cout);

    assert(*rawConfig.valueByPath("IntOption") == "0");
    return 0;
}
