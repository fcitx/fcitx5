/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"

using namespace fcitx;

void testCheckUpdate(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        FCITX_ASSERT(!instance->checkUpdate());
        auto hasUpdateTrue =
            instance->watchEvent(EventType::CheckUpdate,
                                 EventWatcherPhase::Default, [](Event &event) {
                                     auto &checkUpdate =
                                         static_cast<CheckUpdateEvent &>(event);
                                     checkUpdate.setHasUpdate();
                                 });
        FCITX_ASSERT(instance->checkUpdate());
        hasUpdateTrue.reset();
        FCITX_ASSERT(!instance->checkUpdate());
    });
}

void testReloadGlobalConfig(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        bool globalConfigReloadedEventFired = false;
        auto reloadConfigEventWatcher =
            instance->watchEvent(EventType::GlobalConfigReloaded,
                                 EventWatcherPhase::Default, [&](Event &) {
                                     globalConfigReloadedEventFired = true;
                                     FCITX_INFO() << "Global config reloaded";
                                 });
        instance->reloadConfig();
        FCITX_ASSERT(globalConfigReloadedEventFired);
        instance->exit();
    });
}

int main() {
    setupTestingEnvironment(FCITX5_BINARY_DIR, {"testing/testim"}, {});

    char arg0[] = "testinstance";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim";
    char arg3[] = "--option=name1=opt1a,opt1b,name2=opt2a,opt2b";
    char *argv[] = {arg0, arg1, arg2, arg3};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    FCITX_ASSERT(instance.addonManager().addonOptions("name1") ==
                 std::vector<std::string>{"opt1a", "opt1b"});
    FCITX_ASSERT(instance.addonManager().addonOptions("name2") ==
                 std::vector<std::string>{"opt2a", "opt2b"});
    FCITX_ASSERT(instance.addonManager().addonOptions("name3") ==
                 std::vector<std::string>{});
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    testCheckUpdate(&dispatcher, &instance);
    testReloadGlobalConfig(&dispatcher, &instance);
    instance.exec();
    return 0;
}
