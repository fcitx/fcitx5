/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "addon/dummyaddon_public.h"
#include "testdir.h"

double f(int) { return 0; }

int main() {
    fcitx::Log::setLogRule("default=5");
    fcitx::setupTestingEnvironment(FCITX5_BINARY_DIR, {"bin"},
                                   {FCITX5_SOURCE_DIR "/test/addon2/fcitx5"});
    fcitx::AddonManager manager;
    manager.registerDefaultLoader(nullptr);
    manager.load();
    auto *addon = manager.addon("dummyaddon");
    FCITX_ASSERT(addon);
    FCITX_ASSERT(6 ==
                 addon->callWithSignature<int(int)>("DummyAddon::addOne", 5));
    FCITX_ASSERT(6 ==
                 addon->callWithSignature<int(int)>("DummyAddon::addOne", 5.3));
    auto result =
        7 ==
        addon->callWithMetaString<fcitxMakeMetaString("DummyAddon::addOne")>(6);
    FCITX_ASSERT(result);
    auto result2 = 8 == addon->call<fcitx::IDummyAddon::addOne>(7);
    FCITX_ASSERT(result2);

    FCITX_ASSERT(!addon->call<fcitx::IDummyAddon::testCopy>().isCopy());

    auto *addon2 = manager.addon("dummyaddon2");
    FCITX_ASSERT(addon2);
    auto *addon3 = manager.addon("dummyaddon3");
    FCITX_ASSERT(!addon3);
    // loading dummyaddondeps will change log rule level to 3.
    FCITX_ASSERT(
        !fcitx::Log::defaultCategory().checkLogLevel(fcitx::LogLevel::Info));
    FCITX_ASSERT(
        fcitx::Log::defaultCategory().checkLogLevel(fcitx::LogLevel::Warn));
    fcitx::Log::setLogRule("default=5");

    return 0;
}
