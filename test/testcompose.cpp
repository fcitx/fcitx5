/*
 * SPDX-FileCopyrightText: 2022-2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <exception>
#include <iostream>
#include <xkbcommon/xkbcommon-compose.h>
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonloader.h"
#include "fcitx/instance.h"
#include "fcitx/instance_p.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;

FCITX_DEFINE_STATIC_ADDON_REGISTRY(staticAddon);
FCITX_IMPORT_ADDON_FACTORY(staticAddon, keyboard);

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        auto *keyboard = instance->addonManager().addon("keyboard", true);
        FCITX_ASSERT(keyboard);
        FCITX_ASSERT(!instance->inputMethodManager()
                          .currentGroup()
                          .inputMethodList()
                          .empty());
        // Without xcb addon keyboard-us should be the default one.
        FCITX_ASSERT(instance->inputMethodManager()
                         .currentGroup()
                         .inputMethodList()[0]
                         .name() == "keyboard-us");
    });
    dispatcher->schedule([dispatcher, instance]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ḙ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("Ḅ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>(" ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("̣");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_dead_circumflex), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_dead_circumflex), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key(FcitxKey_e),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_dead_belowdot), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key(FcitxKey_B),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_dead_belowdot), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key(FcitxKey_A),
                                                    false);

        dispatcher->schedule([dispatcher, instance]() {
            dispatcher->detach();
            instance->exit();
        });
    });
}

class MyInstance : public Instance {
public:
    using Instance::Instance;
    using Instance::privateData;
};

constexpr char testCompose[] = {
    "<dead_circumflex> <dead_circumflex> <e> : U1E19\n"
    "<dead_belowdot> <B> : \"Ḅ\" U1E04 # LATIN CAPITAL LETTER B WITH DOT "
    "BELOW\n"};

int main() {
    setupTestingEnvironment(FCITX5_BINARY_DIR, {"bin"}, {"test"});

    char arg0[] = "testcompose";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testfrontend,keyboard,testui";
    char *argv[] = {arg0, arg1, arg2};
    try {
        MyInstance instance(FCITX_ARRAY_SIZE(argv), argv);
        instance.privateData()->xkbComposeTable_.reset(
            xkb_compose_table_new_from_buffer(
                instance.privateData()->xkbContext_.get(), testCompose,
                FCITX_ARRAY_SIZE(testCompose), "", XKB_COMPOSE_FORMAT_TEXT_V1,
                XKB_COMPOSE_COMPILE_NO_FLAGS));
        instance.addonManager().registerDefaultLoader(&staticAddon());
        EventDispatcher dispatcher;
        dispatcher.attach(&instance.eventLoop());
        scheduleEvent(&dispatcher, &instance);

        return instance.exec();
    } catch (const InstanceQuietQuit &) {
    } catch (const std::exception &e) {
        std::cerr << "Received exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
