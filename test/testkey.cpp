/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <cstdio>
#include <cstring>
#include "fcitx-utils/key.h"
#include "fcitx-utils/keydata.h"
#include "fcitx-utils/keynametable-compat.h"
#include "fcitx-utils/keynametable.h"
#include "fcitx-utils/log.h"

#define CHECK_ARRAY_ORDER(ARRAY, COMPARE_FUNC)                                 \
    for (size_t i = 0; i < FCITX_ARRAY_SIZE(ARRAY) - 1; i++) {                 \
        FCITX_ASSERT(COMPARE_FUNC(ARRAY[i], ARRAY[i + 1])) << i;               \
    }

int main() {
#define _STRING_LESS(A, B) (strcmp((A), (B)) < 0)
#define _STRING_LESS_2(A, B) (strcmp((A).name, (B).name) < 0)
#define _SYM_LESS(A, B) ((A).sym < (B).sym)
#define _KEYSYM_LESS(A, B) ((A).keysym < (B).keysym)

    CHECK_ARRAY_ORDER(keyNameList, _STRING_LESS);
    CHECK_ARRAY_ORDER(keyNameOffsetByValue, _SYM_LESS);
    CHECK_ARRAY_ORDER(keyNameListCompat, _STRING_LESS_2);
    CHECK_ARRAY_ORDER(fcitx::keysym_to_unicode_tab, _KEYSYM_LESS);

    // Test convert
    for (size_t i = 0; i < FCITX_ARRAY_SIZE(keyValueByNameOffset); i++) {
        FCITX_ASSERT(!fcitx::Key::keySymToString(
                          static_cast<fcitx::KeySym>(keyValueByNameOffset[i]))
                          .empty());
        FCITX_ASSERT(fcitx::Key::keySymFromString(keyNameList[i]) ==
                     keyValueByNameOffset[i]);
    }

    const std::pair<FcitxKeySym, uint32_t> keySymUnicode[]{
        {FcitxKey_BackSpace, '\b'}, {FcitxKey_Tab, '\t'},
        {FcitxKey_Linefeed, '\n'},  {FcitxKey_Clear, '\v'},
        {FcitxKey_Return, '\r'},    {FcitxKey_Escape, '\033'},
        {FcitxKey_space, ' '},
    };

    for (const auto &pair : keySymUnicode) {
        FCITX_ASSERT(fcitx::Key::keySymToUnicode(pair.first) == pair.second);
        FCITX_ASSERT(fcitx::Key::keySymFromUnicode(pair.second) == pair.first);
    }
    FCITX_ASSERT(fcitx::Key::keySymFromUnicode(' ') == FcitxKey_space);
    FCITX_ASSERT(fcitx::Key("1").isDigit());
    FCITX_ASSERT(fcitx::Key(FcitxKey_KP_7).isDigit());
    FCITX_ASSERT(fcitx::Key(FcitxKey_KP_6).digit() == 6);
    FCITX_ASSERT(fcitx::Key(FcitxKey_3).digit() == 3);
    FCITX_ASSERT(fcitx::Key(FcitxKey_space).digit() == -1);
    FCITX_ASSERT(fcitx::Key(FcitxKey_KP_5).digitSelection() == 4);
    FCITX_ASSERT(fcitx::Key("Control+KP_3").digitSelection() == -1);
    FCITX_ASSERT(
        fcitx::Key("Control+KP_3").digitSelection(fcitx::KeyState::Ctrl) == 2);
    FCITX_ASSERT(!fcitx::Key("Ctrl+1").isDigit());
    FCITX_ASSERT(!fcitx::Key("a").isDigit());
    FCITX_ASSERT(fcitx::Key("a").isLAZ());
    FCITX_ASSERT(!fcitx::Key("Shift_L").isLAZ());
    FCITX_ASSERT(fcitx::Key("A").isUAZ());
    FCITX_ASSERT(!fcitx::Key("BackSpace").isUAZ());
    FCITX_ASSERT(fcitx::Key("space").isSimple());
    FCITX_ASSERT(!fcitx::Key("EuroSign").isSimple());
    FCITX_ASSERT(fcitx::Key("Control+Alt_L").isModifier());
    FCITX_ASSERT(!fcitx::Key("a").isModifier());
    FCITX_ASSERT(fcitx::Key("Left").isCursorMove());
    FCITX_ASSERT(!fcitx::Key("Cancel").isCursorMove());
    FCITX_ASSERT(fcitx::Key("S").check(fcitx::Key("Shift+S").normalize()));
    FCITX_ASSERT(
        fcitx::Key("Shift+F4").check(fcitx::Key("Shift+F4").normalize()));
    FCITX_ASSERT(
        fcitx::Key("Shift+Tab").check(fcitx::Key("Shift+Tab").normalize()));
    FCITX_ASSERT(fcitx::Key("Shift+Return")
                     .check(fcitx::Key("Shift+Return").normalize()));
    FCITX_ASSERT(
        fcitx::Key("Shift+space").check(fcitx::Key("Shift+space").normalize()));
    FCITX_ASSERT(
        fcitx::Key("Control+A").check(fcitx::Key("Control+a").normalize()));
    FCITX_ASSERT(fcitx::Key("Alt+exclam")
                     .check(fcitx::Key("Alt+Shift+exclam").normalize()));
    FCITX_ASSERT(fcitx::Key("").sym() == FcitxKey_None);
    FCITX_ASSERT(fcitx::Key("-").sym() == FcitxKey_minus);
    FCITX_ASSERT(fcitx::Key("`").sym() == FcitxKey_grave);
    FCITX_ASSERT(fcitx::Key("Alt+Shift+Shift_L")
                     .isReleaseOfModifier(fcitx::Key("Alt+Shift_L")));
    FCITX_ASSERT(fcitx::Key("Alt+Shift+Meta_L")
                     .isReleaseOfModifier(fcitx::Key("Alt+Shift_L")));
    FCITX_ASSERT(fcitx::Key("Alt+Shift+Meta_R")
                     .isReleaseOfModifier(fcitx::Key("Alt+Shift_L")));
    FCITX_ASSERT(!fcitx::Key("Shift+Shift_L")
                      .isReleaseOfModifier(fcitx::Key("Alt+Shift_L")));
    FCITX_ASSERT(fcitx::Key(FcitxKey_space, fcitx::KeyState::Super2)
                     .normalize()
                     .states() == fcitx::KeyState::Super);
    FCITX_ASSERT(fcitx::Key(FcitxKey_space, fcitx::KeyState::Super2)
                     .check(fcitx::Key("Super+space")));
    FCITX_ASSERT(!fcitx::Key(FcitxKey_None).check(fcitx::Key(FcitxKey_None)));

    // Test complex parse
    auto keyList = fcitx::Key::keyListFromString(
        "CTRL_A Control+B Control+Alt+c Control+Alt+Shift+d "
        "Control+Alt+Shift+Super+E Super+Alt+=");

    fcitx::Key hotkey[] = {
        fcitx::Key(FcitxKey_A, fcitx::KeyState::Ctrl),
        fcitx::Key(FcitxKey_B, fcitx::KeyState::Ctrl),
        fcitx::Key(FcitxKey_c, fcitx::KeyState::Ctrl_Alt),
        fcitx::Key(FcitxKey_d, fcitx::KeyState::Ctrl_Alt_Shift),
        fcitx::Key(FcitxKey_E,
                   fcitx::KeyStates({fcitx::KeyState::Ctrl_Alt_Shift,
                                     fcitx::KeyState::Super})),
        fcitx::Key(FcitxKey_equal, fcitx::KeyStates({fcitx::KeyState::Super,
                                                     fcitx::KeyState::Alt})),
    };

    for (size_t i = 0; i < FCITX_ARRAY_SIZE(hotkey); i++) {
        FCITX_ASSERT(hotkey[i].checkKeyList(keyList));
    }

    keyList.emplace_back(FcitxKey_A);

    auto keyString = fcitx::Key::keyListToString(keyList);
    FCITX_ASSERT(keyString ==
                 "Control+A Control+B Control+Alt+c Control+Alt+Shift+d "
                 "Control+Alt+Shift+Super+E Alt+Super+equal A");

    keyList.clear();
    keyString = fcitx::Key::keyListToString(keyList);
    FCITX_ASSERT(keyString.empty());

    fcitx::Key modifier = fcitx::Key("Control_L").normalize();
    FCITX_ASSERT(modifier.check(fcitx::Key("Control+Control_L")));
    FCITX_ASSERT(modifier.check(fcitx::Key("Control_L")));

    FCITX_ASSERT(
        fcitx::Key(FcitxKey_a, fcitx::KeyState::NumLock).normalize().sym() ==
        FcitxKey_a);

    FCITX_ASSERT(
        fcitx::Key("Control+<25>")
            .check(fcitx::Key::fromKeyCode(25, fcitx::KeyState::Ctrl)));
    FCITX_ASSERT(
        fcitx::Key::fromKeyCode(25, fcitx::KeyState::Ctrl).toString() ==
        "Control+<25>");

    FCITX_ASSERT(fcitx::Key("Hyper+a").toString() == "Hyper+a");
    FCITX_ASSERT(fcitx::Key("Hyper+space")
                     .check(FcitxKey_space, fcitx::KeyState::Hyper));

    FCITX_INFO() << fcitx::Key::keySymToString(
        FcitxKey_Insert, fcitx::KeyStringFormat::Localized);
    FCITX_ASSERT(fcitx::Key::keySymToUnicode(
                     static_cast<fcitx::KeySym>(0x100fdd7)) == 0xfdd7);
    FCITX_ASSERT(fcitx::Key::keySymToUnicode(
                     static_cast<fcitx::KeySym>(0x120fdd7)) == 0);

    return 0;
}
