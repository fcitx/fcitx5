/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <exception>
#include <string>
#include <utility>
#include <vector>
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/event.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputmethodgroup.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/tempmode.h"
#include "fcitx/tempmodemanager.h"
#include "testdir.h"
#include "testfrontend_public.h"
#include "testim_public.h"

namespace {

using namespace fcitx;

struct TestTempModeState {};

class TestTempMode : public SimpleTempMode<TestTempModeState> {
public:
    using Base = SimpleTempMode<TestTempModeState>;

    explicit TestTempMode(Key triggerKey)
        : triggerKeys_{triggerKey.normalize()} {}

    using Base::isActive;

    bool triggerTempMode(const KeyEvent &keyEvent) override {
        if (keyEvent.isRelease()) {
            return false;
        }
        if (keyEvent.key().checkKeyList(triggerKeys_)) {
            triggerCount_++;
            property(keyEvent.inputContext())->setActive(true);
            return true;
        }
        return false;
    }

    bool keyEvent(const KeyEvent &keyEvent) override {
        handledKeys_.push_back(keyEvent.key().toString());
        property(keyEvent.inputContext())->setActive(false);
        return true;
    }

    bool invokeAction(InvokeActionEvent &) override {
        invokeActionCount_++;
        return true;
    }

    std::string_view name() const override { return "testtempmode"; }

    int triggerCount() const { return triggerCount_; }
    int invokeActionCount() const { return invokeActionCount_; }
    const std::vector<std::string> &handledKeys() const { return handledKeys_; }

private:
    KeyList triggerKeys_;
    int triggerCount_ = 0;
    int invokeActionCount_ = 0;
    std::vector<std::string> handledKeys_;
};

void scheduleEvent(Instance *instance, TestTempMode *tempMode) {
    instance->eventDispatcher().schedule([instance, tempMode]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto *testim = instance->addonManager().addon("testim");
        FCITX_ASSERT(testfrontend);
        FCITX_ASSERT(testim);

        std::vector<std::string> imHandledKeys;
        testim->call<ITestIM::setHandler>(
            [&imHandledKeys](const InputMethodEntry &, KeyEvent &keyEvent) {
                if (!keyEvent.isRelease()) {
                    imHandledKeys.push_back(keyEvent.key().toString());
                }
            });

        auto group = instance->inputMethodManager().currentGroup();
        group.inputMethodList().clear();
        group.inputMethodList().push_back(InputMethodGroupItem("testim"));
        instance->inputMethodManager().setGroup(std::move(group));

        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto *ic = instance->inputContextManager().findByUUID(uuid);
        FCITX_ASSERT(ic);
        FCITX_ASSERT(instance->inputMethod(ic) == "testim");

        FCITX_ASSERT(!tempMode->isRegistered());
        instance->tempModeManager().registerTempMode(*tempMode);
        FCITX_ASSERT(tempMode->isRegistered());
        instance->tempModeManager().registerTempMode(*tempMode);
        FCITX_ASSERT(tempMode->isRegistered());

        const Key triggerKey("Control+Alt+Shift+t");
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, triggerKey, false));
        FCITX_ASSERT(tempMode->triggerCount() == 1);
        FCITX_ASSERT(tempMode->isActive(ic));

        InvokeActionEvent invokeActionEvent(
            InvokeActionEvent::Action::LeftClick, 0, ic);
        ic->invokeAction(invokeActionEvent);
        FCITX_ASSERT(invokeActionEvent.accepted());
        FCITX_ASSERT(tempMode->invokeActionCount() == 1);

        const auto imHandledBeforeTempKey = imHandledKeys.size();
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("a"), false));
        FCITX_ASSERT(!tempMode->isActive(ic));
        FCITX_ASSERT(tempMode->handledKeys().size() == 1);
        FCITX_ASSERT(tempMode->handledKeys().back() == "a");
        FCITX_ASSERT(imHandledKeys.size() == imHandledBeforeTempKey);

        tempMode->unregister();
        FCITX_ASSERT(!tempMode->isRegistered());

        const auto tempHandledBeforeRelease = tempMode->handledKeys().size();
        const auto imHandledBeforeReleasedTrigger = imHandledKeys.size();
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, triggerKey, false));
        FCITX_ASSERT(imHandledKeys.size() ==
                     imHandledBeforeReleasedTrigger + 1);
        FCITX_ASSERT(tempMode->triggerCount() == 1);

        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("b"), false));
        FCITX_ASSERT(tempMode->handledKeys().size() ==
                     tempHandledBeforeRelease);
        FCITX_ASSERT(imHandledKeys.back() == "b");

        instance->exit();
    });
}

} // namespace

int main() {
    setupTestingEnvironmentPath(FCITX5_BINARY_DIR, {"bin"}, {"test"});

    char arg0[] = "testtempmode";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend";
    char *argv[] = {arg0, arg1, arg2};

    try {
        Instance instance(FCITX_ARRAY_SIZE(argv), argv);
        instance.addonManager().registerDefaultLoader(nullptr);
        TestTempMode tempMode(Key("Control+Alt+Shift+t"));
        scheduleEvent(&instance, &tempMode);
        return instance.exec();
    } catch (const InstanceQuietQuit &) {
        return 0;
    } catch (const std::exception &e) {
        FCITX_ERROR() << "Exception: " << e.what();
        return 1;
    }

    return 0;
}
