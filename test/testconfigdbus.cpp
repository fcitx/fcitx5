/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <unistd.h>
#include "fcitx-config/dbushelper.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/dbus/variant.h"
#include "testconfig.h"

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
