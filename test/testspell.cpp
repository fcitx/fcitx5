/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonloader.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodgroup.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;

FCITX_DEFINE_STATIC_ADDON_REGISTRY(staticAddon);
FCITX_IMPORT_ADDON_FACTORY(staticAddon, keyboard);

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        auto *spell = instance->addonManager().addon("spell", true);
        FCITX_ASSERT(spell);
        InputMethodGroup group("Test");
        // Make sure custom xkb does not kick in.
        group.setDefaultLayout("us");
        group.inputMethodList().push_back(InputMethodGroupItem("keyboard-us"));
        instance->inputMethodManager().addEmptyGroup("Test");
        instance->inputMethodManager().setGroup(group);
        instance->inputMethodManager().setCurrentGroup("Test");
    });
    dispatcher->schedule([dispatcher, instance]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("apple-green");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+Alt+h"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("p"), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_BackSpace), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("p"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("p"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("l"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("e"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("-"), false);
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
        {"src/modules/spell", "testing/testfrontend", "testing/testui",
         "testing/testim"},
        {"test", "src/modules", FCITX5_SOURCE_DIR "/src/modules"});

    char arg0[] = "testspell";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=keyboard,testfrontend,spell,testui";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(&staticAddon());
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    scheduleEvent(&dispatcher, &instance);
    instance.exec();
    return 0;
}
