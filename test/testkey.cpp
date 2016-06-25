#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "fcitx-utils/key.h"
#include "fcitx-utils/keynametable-compat.h"
#include "fcitx-utils/keynametable.h"

#define CHECK_ARRAY_ORDER(ARRAY, COMPARE_FUNC)                                                                         \
    for (size_t i = 0; i < FCITX_ARRAY_SIZE(ARRAY) - 1; i++) {                                                         \
        assert(COMPARE_FUNC(ARRAY[i], ARRAY[i + 1]));                                                                  \
    }

int main() {
#define _STRING_LESS(A, B) (strcmp((A), (B)) < 0)
#define _STRING_LESS_2(A, B) (strcmp((A).name, (B).name) < 0)
#define _SYM_LESS(A, B) ((A).sym < (B).sym)

    CHECK_ARRAY_ORDER(keyNameList, _STRING_LESS);
    CHECK_ARRAY_ORDER(keyNameOffsetByValue, _SYM_LESS);
    CHECK_ARRAY_ORDER(keyNameListCompat, _STRING_LESS_2);

    // Test convert
    for (size_t i = 0; i < FCITX_ARRAY_SIZE(keyValueByNameOffset); i++) {
        assert(!fcitx::Key::keySymToString(static_cast<fcitx::KeySym>(keyValueByNameOffset[i])).empty());
        assert(fcitx::Key::keySymFromString(keyNameList[i]) == keyValueByNameOffset[i]);
    }

    assert(fcitx::Key::keySymFromUnicode(' ') == FcitxKey_space);
    assert(fcitx::Key("1").isDigit());
    assert(!fcitx::Key("Ctrl+1").isDigit());
    assert(!fcitx::Key("a").isDigit());
    assert(fcitx::Key("a").isLAZ());
    assert(!fcitx::Key("Shift_L").isLAZ());
    assert(fcitx::Key("A").isUAZ());
    assert(!fcitx::Key("BackSpace").isUAZ());
    assert(fcitx::Key("space").isSimple());
    assert(!fcitx::Key("EuroSign").isSimple());
    assert(fcitx::Key("Control+Alt_L").isModifier());
    assert(!fcitx::Key("a").isModifier());
    assert(fcitx::Key("Left").isCursorMove());
    assert(!fcitx::Key("Cancel").isCursorMove());
    assert(fcitx::Key("Shift+S").normalize().check(fcitx::Key("S")));
    assert(fcitx::Key("Shift+F4").normalize().check(fcitx::Key("Shift+F4")));
    assert(fcitx::Key("Ctrl+a").normalize().check(fcitx::Key("Ctrl+A")));
    assert(fcitx::Key("Alt+Shift+exclam").normalize().check(fcitx::Key("Alt+exclam")));
    assert(fcitx::Key("").sym() == FcitxKey_None);
    assert(fcitx::Key("-").sym() == FcitxKey_minus);

    // Test complex parse
    auto keyList = fcitx::Key::keyListFromString("CTRL_A Control+B Control+Alt+c Control+Alt+Shift+d "
                                                 "Control+Alt+Shift+Super+E Super+Alt+=");

    fcitx::Key hotkey[] = {
        fcitx::Key(FcitxKey_A, fcitx::KeyState::Ctrl),
        fcitx::Key(FcitxKey_B, fcitx::KeyState::Ctrl),
        fcitx::Key(FcitxKey_c, fcitx::KeyState::Ctrl_Alt),
        fcitx::Key(FcitxKey_d, fcitx::KeyState::Ctrl_Alt_Shift),
        fcitx::Key(FcitxKey_E, fcitx::KeyStates({fcitx::KeyState::Ctrl_Alt_Shift, fcitx::KeyState::Super})),
        fcitx::Key(FcitxKey_equal, fcitx::KeyStates({fcitx::KeyState::Super, fcitx::KeyState::Alt})),
    };

    for (size_t i = 0; i < FCITX_ARRAY_SIZE(hotkey); i++) {
        assert(fcitx::Key::keyListCheck(keyList, hotkey[i]));
    }

    keyList.emplace_back(FcitxKey_A);

    auto keyString = fcitx::Key::keyListToString(keyList);
    assert(keyString == "Control+A Control+B Control+Alt+c Control+Alt+Shift+d "
                        "Control+Alt+Shift+Super+E Alt+Super+equal A");

    keyList.clear();
    keyString = fcitx::Key::keyListToString(keyList);
    assert(keyString == "");

    fcitx::Key modifier = fcitx::Key("Control_L").normalize();
    assert(fcitx::Key("Control+Control_L").check(modifier));
    assert(fcitx::Key("Control_L").check(modifier));

    return 0;
}
