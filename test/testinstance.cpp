/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/event.h"
#include "fcitx/inputmethodgroup.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;

void testCheckUpdate(Instance &instance) {
    instance.eventDispatcher().schedule([&instance]() {
        FCITX_ASSERT(!instance.checkUpdate());
        auto hasUpdateTrue =
            instance.watchEvent(EventType::CheckUpdate,
                                EventWatcherPhase::Default, [](Event &event) {
                                    auto &checkUpdate =
                                        static_cast<CheckUpdateEvent &>(event);
                                    checkUpdate.setHasUpdate();
                                });
        FCITX_ASSERT(instance.checkUpdate());
        hasUpdateTrue.reset();
        FCITX_ASSERT(!instance.checkUpdate());
    });
}

void testReloadGlobalConfig(Instance &instance) {
    instance.eventDispatcher().schedule([&instance]() {
        bool globalConfigReloadedEventFired = false;
        auto reloadConfigEventWatcher =
            instance.watchEvent(EventType::GlobalConfigReloaded,
                                EventWatcherPhase::Default, [&](Event &) {
                                    globalConfigReloadedEventFired = true;
                                    FCITX_INFO() << "Global config reloaded";
                                });
        instance.reloadConfig();
        FCITX_ASSERT(globalConfigReloadedEventFired);
    });
}

void testModifierOnlyHotkey(Instance &instance) {
    instance.eventDispatcher().schedule([&instance]() {
        auto defaultGroup = instance.inputMethodManager().currentGroup();
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("keyboard-us"));
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("testim"));
        instance.inputMethodManager().setGroup(std::move(defaultGroup));

        auto *testfrontend = instance.addonManager().addon("testfrontend");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto *ic = instance.inputContextManager().findByUUID(uuid);
        FCITX_ASSERT(ic);

        FCITX_ASSERT(instance.inputMethod(ic) == "keyboard-us");
        // Alt trigger doesn't work since we are at first im.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift_L"), false));
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift+Shift_L"), true));
        FCITX_ASSERT(instance.inputMethod(ic) == "keyboard-us");

        FCITX_ASSERT(instance.inputMethod(ic) == "keyboard-us");
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Control+space"), false));
        FCITX_ASSERT(instance.inputMethod(ic) == "testim");

        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift_L"), false));
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift+Shift_L"), true));
        FCITX_ASSERT(instance.inputMethod(ic) == "keyboard-us");

        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift_L"), false));
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift+Shift_L"), true));
        FCITX_ASSERT(instance.inputMethod(ic) == "testim");

        // Sleep 1 sec between press and release, should not trigger based on
        // default 250ms.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift_L"), false));
        std::this_thread::sleep_until(std::chrono::steady_clock::now() +
                                      std::chrono::seconds(1));
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift+Shift_L"), true));
        FCITX_ASSERT(instance.inputMethod(ic) == "testim");

        // Some other key pressed between shift, should not trigger.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift_L"), false));
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift+A"), false));
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Shift+Shift_L"), true));
        FCITX_ASSERT(instance.inputMethod(ic) == "testim");
    });
}

int main() {
    setupTestingEnvironment(FCITX5_BINARY_DIR,
                            {"testing/testim", "testing/testfrontend"}, {});

    char arg0[] = "testinstance";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend";
    char arg3[] = "--option=name1=opt1a:opt1b,name2=opt2a:opt2b";
    char *argv[] = {arg0, arg1, arg2, arg3};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    FCITX_ASSERT(instance.addonManager().addonOptions("name1") ==
                 std::vector<std::string>{"opt1a", "opt1b"});
    FCITX_ASSERT(instance.addonManager().addonOptions("name2") ==
                 std::vector<std::string>{"opt2a", "opt2b"});
    FCITX_ASSERT(instance.addonManager().addonOptions("name3") ==
                 std::vector<std::string>{});
    testCheckUpdate(instance);
    testReloadGlobalConfig(instance);
    testModifierOnlyHotkey(instance);
    instance.eventDispatcher().schedule([&instance]() { instance.exit(); });
    instance.exec();
    return 0;
}
