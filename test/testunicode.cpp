/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <thread>
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        auto unicode = instance->addonManager().addon("unicode", true);
        FCITX_ASSERT(unicode);
    });
    dispatcher->schedule([dispatcher, instance]() {
        auto testfrontend = instance->addonManager().addon("testfrontend");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("🍏");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key("Control+Alt+Shift+u"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("p"), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_BackSpace), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("p"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("p"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("l"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("e"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key(" "), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("g"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("r"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("e"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("e"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("n"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Alt+1"), false);

        dispatcher->schedule([dispatcher, instance]() {
            dispatcher->detach();
            instance->exit();
        });
    });
}

int main() {
    setupTestingEnvironment(
        FCITX5_BINARY_DIR,
        {"src/modules/unicode", "testing/testfrontend", "testing/testui",
         "testing/testim"},
        {"test", "src/modules", FCITX5_SOURCE_DIR "/src/modules"});
    Instance instance(0, nullptr);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    std::thread thread(scheduleEvent, &dispatcher, &instance);
    instance.exec();
    thread.join();
    return 0;
}
