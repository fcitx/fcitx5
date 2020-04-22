//
// Copyright (C) 2015~2015 by CSSlayer
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

#include "testconfig.h"
#include <vector>
#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-config/iniparser.h>
#include "fcitx-utils/log.h"

using namespace fcitx;

void validate(const RawConfig &config) {
    for (const auto &item : config.subItems()) {
        auto subConfig = config.get(item);
        FCITX_ASSERT(subConfig->name() == item);
        FCITX_ASSERT(subConfig);
        FCITX_ASSERT(subConfig->parent() == &config);
        validate(*subConfig);
    }
}

void testBasics() {
    TestConfig config;

    I18NString str;
    str.set("A", "zh_CN");
    str.set("ABCD");
    config.i18nStringValue.setValue(str);
    RawConfig rawConfig;
    config.save(rawConfig);

    FCITX_ASSERT(*config.intVector == std::vector<int>{0});
    *config.intVector.mutableValue() = std::vector<int>{1, 2};
    FCITX_ASSERT((*config.intVector == std::vector<int>{1, 2}));
    *config.intVector.mutableValue() = std::vector<int>{-1, 2};
    FCITX_INFO() << *config.intVector;
    // Invalid value is not set.
    FCITX_ASSERT((*config.intVector == std::vector<int>{1, 2}));

    writeAsIni(rawConfig, stdout);

    FCITX_ASSERT(*rawConfig.valueByPath("IntOption") == "0");

    config.intValue.setValue(5);
    FCITX_ASSERT(config.intValue.value() == 5);
    // violates constrain
    config.intValue.setValue(20);
    // still have the old value
    FCITX_ASSERT(config.intValue.value() == 5);
    rawConfig.setValueByPath("EnumOption", "EnumB");

    config.subConfigValue.mutableValue()->intValue.setValue(5);
    FCITX_ASSERT(*config.subConfigValue->intValue == 5);

    FCITX_INFO() << rawConfig;
    config.load(rawConfig);
    FCITX_ASSERT(config.intValue.value() == 0);
    FCITX_ASSERT(config.enumValue.value() == TestEnum::EnumB);

    FCITX_ASSERT(config.i18nStringValue.value().match("") == "ABCD");
    FCITX_ASSERT(config.i18nStringValue.value().match("zh_CN") == "A");

    RawConfig rawDescConfig;
    config.dumpDescription(rawDescConfig);
    writeAsIni(rawDescConfig, stdout);

    auto intOption = rawConfig.get("IntOption")->detach();
    FCITX_ASSERT(intOption);
    FCITX_ASSERT(intOption->value() == "0");
    FCITX_ASSERT(!rawConfig.get("IntOption"));
    FCITX_ASSERT(!intOption->parent());

    validate(rawConfig);
    validate(rawDescConfig);
}

void testMove() {
    RawConfig config;
    config.setValue("A");
    auto &sub = config["B"];
    sub.setValue("C");
    FCITX_ASSERT(sub.parent() == &config);

    validate(config);

    auto newConfig = std::move(config);
    FCITX_ASSERT(newConfig.value() == "A");
    FCITX_ASSERT(newConfig.subItems() == std::vector<std::string>{"B"});
    auto &newSub = newConfig["B"];
    FCITX_ASSERT(newSub == sub);
    auto copySub = newSub;
    FCITX_ASSERT(copySub == sub);
    FCITX_ASSERT(copySub == newSub);
    validate(newConfig);
}

void testAssign() {
    RawConfig config;
    config["A"]["B"].setValue("1");
    config["A"]["C"].setValue("2");
    FCITX_INFO() << config;

    RawConfig newConfig;
    newConfig = config["A"];
    RawConfig newConfig2(config["A"]);
    FCITX_ASSERT(newConfig2.name().empty());

    RawConfig expect;
    expect["B"].setValue("1");
    expect["C"].setValue("2");

    FCITX_INFO() << newConfig;
    FCITX_INFO() << newConfig2;
    FCITX_ASSERT(newConfig == expect);
    FCITX_ASSERT(newConfig2 == expect);
    validate(newConfig);
    validate(newConfig2);
    validate(expect);

    config["A"]["B"] = expect;
    FCITX_ASSERT(config["A"]["B"] == expect);

    RawConfig expect2;
    expect2["A"]["B"]["B"].setValue("1");
    expect2["A"]["B"]["C"].setValue("2");
    expect2["A"]["C"].setValue("2");
    FCITX_INFO() << config;
    FCITX_ASSERT(config == expect2);
    validate(config);
}

void testRecursiveAssign() {
    {
        RawConfig config;
        config["A"]["B"]["C"] = "DEF";
        config["A"] = config["A"]["B"]["C"];
        FCITX_INFO() << config;
        validate(config);
    }
    {
        RawConfig config;
        config["A"]["B"]["C"] = "DEF";
        config["A"]["B"]["C"] = config["A"];
        FCITX_INFO() << config;
        validate(config);
    }
}

int main() {
    testBasics();
    testMove();
    testAssign();
    testRecursiveAssign();
    return 0;
}
