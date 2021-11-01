/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <thread>
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"

using namespace fcitx;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
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
        instance->exit();
    });
}

int main() {
    setupTestingEnvironment(FCITX5_BINARY_DIR, {"testing/testim"}, {});

    char arg0[] = "testinstance";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    std::thread thread(scheduleEvent, &dispatcher, &instance);
    instance.exec();
    thread.join();
    return 0;
}
