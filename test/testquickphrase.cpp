/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "quickphrase_public.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;

std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>> handle;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        auto *quickphrase = instance->addonManager().addon("quickphrase", true);
        handle = quickphrase->call<IQuickPhrase::addProvider>(
            [](InputContext *, const std::string &text,
               const QuickPhraseAddCandidateCallback &callback) {
                FCITX_INFO() << "Quickphrase text: " << text;
                if (text == "test") {
                    callback("TEST", "DISPLAY", QuickPhraseAction::Commit);
                    return false;
                }
                return true;
            });
        FCITX_ASSERT(quickphrase);
    });
    dispatcher->schedule([dispatcher, instance]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        for (const auto *expectation :
             {"TEST", "abc", "abcd", "DEF", "abcd", "DEF1", "test1", "CALLBACK",
              "AUTOCOMMIT"}) {
            testfrontend->call<ITestFrontend::pushCommitExpectation>(
                expectation);
        }
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto *ic = instance->inputContextManager().findByUUID(uuid);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("t"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_BackSpace), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("e"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("s"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("t"), false);
        ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel,
                                true);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("1"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("b"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("c"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        // QuickPhrase.mb
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("A"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("B"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("C"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("D"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("1"), false);

        // test.mb
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("A"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("B"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("C"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("1"), false);

        // QuickPhrase.mb
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("A"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("B"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("C"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("2"), false);

        // test-disable.mb
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("D"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("E"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("F"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("1"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        handle.reset();

        // test-disable.mb
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("t"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("e"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("s"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("t"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("1"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        auto *quickphrase = instance->addonManager().addon("quickphrase", true);
        handle = quickphrase->call<IQuickPhrase::addProvider>(
            [](InputContext *, const std::string &text,
               const QuickPhraseAddCandidateCallback &callback) {
                FCITX_INFO() << "Quickphrase text: " << text;
                if (text == "abc") {
                    callback("", "", QuickPhraseAction::AlphaSelection);
                    callback("d", "DISPLAY1", QuickPhraseAction::TypeToBuffer);
                    callback("e", "DISPLAY2", QuickPhraseAction::TypeToBuffer);
                    callback("f", "DISPLAY3", QuickPhraseAction::TypeToBuffer);
                    return false;
                }
                if (text == "abcf") {
                    callback("", "", QuickPhraseAction::AlphaSelection);
                    callback("CALLBACK", "DISPLAY3", QuickPhraseAction::Commit);
                    return false;
                }
                if (text == "auto") {
                    callback("AUTOCOMMIT", "", QuickPhraseAction::AutoCommit);
                    return false;
                }
                return true;
            });

        // test-disable.mb
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("b"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("c"), false);
        ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel,
                                true);
        // Alpha Selection: c
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("c"), false);
        // Alpha Selection: a
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Super+grave"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("u"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("t"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);

        dispatcher->schedule([dispatcher, instance]() {
            handle.reset();
            dispatcher->detach();
            instance->exit();
        });
    });
}

int main() {
    setupTestingEnvironment(
        FCITX5_BINARY_DIR,
        {"src/modules/quickphrase", "testing/testfrontend", "testing/testui",
         "testing/testim"},
        {"test", "src/modules", FCITX5_SOURCE_DIR "/test/addon/fcitx5"});

    char arg0[] = "testquickphrase";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,quickphrase,testui";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    scheduleEvent(&dispatcher, &instance);
    instance.exec();
    return 0;
}
