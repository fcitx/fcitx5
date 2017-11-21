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

#include "fcitx-config/dbushelper.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/dbus/variant.h"
#include "testconfig.h"
#include <unistd.h>

using namespace fcitx;

int main() {

    RawConfig config;
    *config.get("OptionA/SubOptionB", true) = "abc";
    *config.get("OptionC", true) = "def";

    auto map = rawConfigToVariant(config);
    FCITX_INFO() << map;

    RawConfig config2 = variantToRawConfig(map);
    FCITX_ASSERT(config == config2);

    writeAsIni(config2, STDOUT_FILENO);

    RawConfig desc;
    TestConfig testconfig;
    testconfig.dumpDescription(desc);
    FCITX_INFO() << dumpDBusConfigDescription(testconfig);

    return 0;
}
