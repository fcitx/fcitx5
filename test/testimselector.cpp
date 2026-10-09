/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputmethodgroup.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;

FCITX_DEFINE_STATIC_ADDON_REGISTRY(staticAddon);
FCITX_IMPORT_ADDON_FACTORY(staticAddon, keyboard);

int main() {
    setupTestingEnvironmentPath(FCITX5_BINARY_DIR, {"bin"}, {TEST_DATA_DIR});

    char arg0[] = "testimselector";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testfrontend,testim,keyboard,imselector";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(&staticAddon());
    instance.eventDispatcher().schedule([&instance]() {
        auto *imselector = instance.addonManager().addon("imselector", true);
        auto *testfrontend = instance.addonManager().addon("testfrontend");
        FCITX_ASSERT(imselector);
        FCITX_ASSERT(testfrontend);
        auto &manager = instance.inputMethodManager();
        FCITX_ASSERT(manager.entry("keyboard-th"));

        auto group = manager.currentGroup();
        group.setDefaultLayout("us");
        group.inputMethodList().clear();
        group.inputMethodList().emplace_back("keyboard-us");
        group.inputMethodList().emplace_back("keyboard-th");
        group.inputMethodList().emplace_back("testim");
        group.setDefaultInputMethod("keyboard-th");
        manager.setGroup(group);

        RawConfig config;
        config.setValueByPath("SwitchKey/0", "Control+Alt+Q");
        config.setValueByPath("SwitchKey/1", "Control+Alt+W");
        config.setValueByPath("SwitchKey/2", "Control+Alt+E");
        config.setValueByPath("SwitchKeyLocal/0", "Control+Alt+A");
        config.setValueByPath("SwitchKeyLocal/1", "Control+Alt+S");
        config.setValueByPath("SwitchKeyLocal/2", "Control+Alt+D");
        imselector->setConfig(config);

        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto *ic = instance.inputContextManager().findByUUID(uuid);
        FCITX_ASSERT(ic);
        ic->focusIn();

        // Evdev key codes for the US Q/W/E and A/S/D physical keys.
        auto switchFromThai = [&](Key key, const char *expected, bool local) {
            instance.setCurrentInputMethod(ic, "keyboard-th", false);
            FCITX_ASSERT(instance.inputMethod(ic) == "keyboard-th");
            KeyEvent event(ic, key);
            FCITX_ASSERT(ic->keyEvent(event));
            FCITX_ASSERT(event.rawKey().sym() != event.origKey().sym());
            FCITX_ASSERT(instance.inputMethod(ic) == expected);
            if (local) {
                FCITX_ASSERT(manager.currentGroup().defaultInputMethod() ==
                             "keyboard-th");
            }
        };
        const auto modifiers = KeyStates(KeyState::Ctrl) | KeyState::Alt;
        switchFromThai(Key(FcitxKey_q, modifiers, 24), "keyboard-us", false);
        switchFromThai(Key(FcitxKey_e, modifiers, 26), "testim", false);
        FCITX_ASSERT(manager.currentGroup().defaultInputMethod() == "testim");
        switchFromThai(Key(FcitxKey_a, modifiers, 38), "keyboard-us", true);
        switchFromThai(Key(FcitxKey_d, modifiers, 40), "testim", true);

        // Menu shortcuts replay a key press without a physical key code.
        switchFromThai(Key("Control+Alt+q"), "keyboard-us", false);

        // Matching the converted layout must remain supported as well.
        config.setValueByPath("SwitchKey/0", "Thai_maiyamok");
        imselector->setConfig(config);
        switchFromThai(Key(FcitxKey_q, KeyStates{}, 24), "keyboard-us", false);

        // Physical-key shortcuts must retain their original modifier state.
        config.setValueByPath("SwitchKeyLocal/2",
                              Key::fromKeyCode(40, modifiers).toString());
        imselector->setConfig(config);
        switchFromThai(Key(FcitxKey_d, modifiers, 40), "testim", true);
        instance.exit();
    });
    return instance.exec();
}
