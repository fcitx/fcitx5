/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "key.h"
#include <cstring>
#include <unordered_map>
#include "charutils.h"
#include "i18n.h"
#include "keydata.h"
#include "keynametable-compat.h"
#include "keynametable.h"
#include "misc_p.h"
#include "stringutils.h"
#include "utf8.h"

namespace fcitx {

namespace {

std::unordered_map<KeySym, const char *, EnumHash> makeLookupKeyNameMap() {

    static const struct {
        KeySym key;
        const char name[25];
    } keyname[] = {
        {FcitxKey_Alt_L, NC_("Key name", "Left Alt")},
        {FcitxKey_Alt_R, NC_("Key name", "Right Alt")},
        {FcitxKey_Shift_L, NC_("Key name", "Left Shift")},
        {FcitxKey_Shift_R, NC_("Key name", "Right Shift")},
        {FcitxKey_Control_L, NC_("Key name", "Left Control")},
        {FcitxKey_Control_R, NC_("Key name", "Right Control")},
        {FcitxKey_Super_L, NC_("Key name", "Left Super")},
        {FcitxKey_Super_R, NC_("Key name", "Right Super")},
        {FcitxKey_Hyper_L, NC_("Key name", "Left Hyper")},
        {FcitxKey_Hyper_R, NC_("Key name", "Right Hyper")},
        {FcitxKey_space, NC_("Key name", "Space")},
        {FcitxKey_Tab, NC_("Key name", "Tab")},
        {FcitxKey_BackSpace, NC_("Key name", "Backspace")},
        {FcitxKey_Return, NC_("Key name", "Return")},
        {FcitxKey_Pause, NC_("Key name", "Pause")},
        {FcitxKey_Home, NC_("Key name", "Home")},
        {FcitxKey_End, NC_("Key name", "End")},
        {FcitxKey_Left, NC_("Key name", "Left")},
        {FcitxKey_Up, NC_("Key name", "Up")},
        {FcitxKey_Right, NC_("Key name", "Right")},
        {FcitxKey_Down, NC_("Key name", "Down")},
        {FcitxKey_Page_Up, NC_("Key name", "Page Up")},
        {FcitxKey_Page_Down, NC_("Key name", "Page Down")},
        {FcitxKey_Caps_Lock, NC_("Key name", "CapsLock")},
        {FcitxKey_Num_Lock, NC_("Key name", "NumLock")},
        {FcitxKey_KP_Space, NC_("Key name", "Keypad Space")},
        {FcitxKey_KP_Tab, NC_("Key name", "Keypad Tab")},
        {FcitxKey_KP_Enter, NC_("Key name", "Keypad Enter")},
        {FcitxKey_KP_F1, NC_("Key name", "Keypad F1")},
        {FcitxKey_KP_F2, NC_("Key name", "Keypad F2")},
        {FcitxKey_KP_F3, NC_("Key name", "Keypad F3")},
        {FcitxKey_KP_F4, NC_("Key name", "Keypad F4")},
        {FcitxKey_KP_Home, NC_("Key name", "Keypad Home")},
        {FcitxKey_KP_Left, NC_("Key name", "Keypad Left")},
        {FcitxKey_KP_Up, NC_("Key name", "Keypad Up")},
        {FcitxKey_KP_Right, NC_("Key name", "Keypad Right")},
        {FcitxKey_KP_Down, NC_("Key name", "Keypad Down")},
        {FcitxKey_KP_Page_Up, NC_("Key name", "Keypad Page Up")},
        {FcitxKey_KP_Page_Down, NC_("Key name", "Keypad Page Down")},
        {FcitxKey_KP_End, NC_("Key name", "Keypad End")},
        {FcitxKey_KP_Begin, NC_("Key name", "Keypad Begin")},
        {FcitxKey_KP_Insert, NC_("Key name", "Keypad Insert")},
        {FcitxKey_KP_Delete, NC_("Key name", "Keypad Delete")},
        {FcitxKey_KP_Equal, NC_("Key name", "Keypad =")},
        {FcitxKey_KP_Multiply, NC_("Key name", "Keypad *")},
        {FcitxKey_KP_Add, NC_("Key name", "Keypad +")},
        {FcitxKey_KP_Separator, NC_("Key name", "Keypad ,")},
        {FcitxKey_KP_Subtract, NC_("Key name", "Keypad -")},
        {FcitxKey_KP_Decimal, NC_("Key name", "Keypad .")},
        {FcitxKey_KP_Divide, NC_("Key name", "Keypad /")},
        {FcitxKey_KP_0, NC_("Key name", "Keypad 0")},
        {FcitxKey_KP_1, NC_("Key name", "Keypad 1")},
        {FcitxKey_KP_2, NC_("Key name", "Keypad 2")},
        {FcitxKey_KP_3, NC_("Key name", "Keypad 3")},
        {FcitxKey_KP_4, NC_("Key name", "Keypad 4")},
        {FcitxKey_KP_5, NC_("Key name", "Keypad 5")},
        {FcitxKey_KP_6, NC_("Key name", "Keypad 6")},
        {FcitxKey_KP_7, NC_("Key name", "Keypad 7")},
        {FcitxKey_KP_8, NC_("Key name", "Keypad 8")},
        {FcitxKey_KP_9, NC_("Key name", "Keypad 9")},
        {FcitxKey_Scroll_Lock, NC_("Key name", "ScrollLock")},
        {FcitxKey_Menu, NC_("Key name", "Menu")},
        {FcitxKey_Help, NC_("Key name", "Help")},
        {FcitxKey_Back, NC_("Key name", "Back")},
        {FcitxKey_Forward, NC_("Key name", "Forward")},
        {FcitxKey_Stop, NC_("Key name", "Stop")},
        {FcitxKey_Refresh, NC_("Key name", "Refresh")},
        {FcitxKey_AudioLowerVolume, NC_("Key name", "Volume Down")},
        {FcitxKey_AudioMute, NC_("Key name", "Volume Mute")},
        {FcitxKey_AudioRaiseVolume, NC_("Key name", "Volume Up")},
        {FcitxKey_AudioPlay, NC_("Key name", "Media Play")},
        {FcitxKey_AudioStop, NC_("Key name", "Media Stop")},
        {FcitxKey_AudioPrev, NC_("Key name", "Media Previous")},
        {FcitxKey_AudioNext, NC_("Key name", "Media Next")},
        {FcitxKey_AudioRecord, NC_("Key name", "Media Record")},
        {FcitxKey_AudioPause, NC_("Key name", "Media Pause")},
        {FcitxKey_HomePage, NC_("Key name", "Home Page")},
        {FcitxKey_Favorites, NC_("Key name", "Favorites")},
        {FcitxKey_Search, NC_("Key name", "Search")},
        {FcitxKey_Standby, NC_("Key name", "Standby")},
        {FcitxKey_OpenURL, NC_("Key name", "Open URL")},
        {FcitxKey_Mail, NC_("Key name", "Launch Mail")},
        {FcitxKey_Launch0, NC_("Key name", "Launch (0)")},
        {FcitxKey_Launch1, NC_("Key name", "Launch (1)")},
        {FcitxKey_Launch2, NC_("Key name", "Launch (2)")},
        {FcitxKey_Launch3, NC_("Key name", "Launch (3)")},
        {FcitxKey_Launch4, NC_("Key name", "Launch (4)")},
        {FcitxKey_Launch5, NC_("Key name", "Launch (5)")},
        {FcitxKey_Launch6, NC_("Key name", "Launch (6)")},
        {FcitxKey_Launch7, NC_("Key name", "Launch (7)")},
        {FcitxKey_Launch8, NC_("Key name", "Launch (8)")},
        {FcitxKey_Launch9, NC_("Key name", "Launch (9)")},
        {FcitxKey_LaunchA, NC_("Key name", "Launch (A)")},
        {FcitxKey_LaunchB, NC_("Key name", "Launch (B)")},
        {FcitxKey_LaunchC, NC_("Key name", "Launch (C)")},
        {FcitxKey_LaunchD, NC_("Key name", "Launch (D)")},
        {FcitxKey_LaunchE, NC_("Key name", "Launch (E)")},
        {FcitxKey_LaunchF, NC_("Key name", "Launch (F)")},
        {FcitxKey_MonBrightnessUp, NC_("Key name", "Monitor Brightness Up")},
        {FcitxKey_MonBrightnessDown,
         NC_("Key name", "Monitor Brightness Down")},
        {FcitxKey_KbdLightOnOff, NC_("Key name", "Keyboard Light On/Off")},
        {FcitxKey_KbdBrightnessUp, NC_("Key name", "Keyboard Brightness Up")},
        {FcitxKey_KbdBrightnessDown,
         NC_("Key name", "Keyboard Brightness Down")},
        {FcitxKey_PowerOff, NC_("Key name", "Power Off")},
        {FcitxKey_WakeUp, NC_("Key name", "Wake Up")},
        {FcitxKey_Eject, NC_("Key name", "Eject")},
        {FcitxKey_ScreenSaver, NC_("Key name", "Screensaver")},
        {FcitxKey_WWW, NC_("Key name", "WWW")},
        {FcitxKey_Sleep, NC_("Key name", "Sleep")},
        {FcitxKey_LightBulb, NC_("Key name", "LightBulb")},
        {FcitxKey_Shop, NC_("Key name", "Shop")},
        {FcitxKey_History, NC_("Key name", "History")},
        {FcitxKey_AddFavorite, NC_("Key name", "Add Favorite")},
        {FcitxKey_HotLinks, NC_("Key name", "Hot Links")},
        {FcitxKey_BrightnessAdjust, NC_("Key name", "Adjust Brightness")},
        {FcitxKey_Finance, NC_("Key name", "Finance")},
        {FcitxKey_Community, NC_("Key name", "Community")},
        {FcitxKey_AudioRewind, NC_("Key name", "Media Rewind")},
        {FcitxKey_BackForward, NC_("Key name", "Back Forward")},
        {FcitxKey_ApplicationLeft, NC_("Key name", "Application Left")},
        {FcitxKey_ApplicationRight, NC_("Key name", "Application Right")},
        {FcitxKey_Book, NC_("Key name", "Book")},
        {FcitxKey_CD, NC_("Key name", "CD")},
        {FcitxKey_Calculator, NC_("Key name", "Calculator")},
        {FcitxKey_Clear, NC_("Key name", "Clear")},
        {FcitxKey_Close, NC_("Key name", "Close")},
        {FcitxKey_Copy, NC_("Key name", "Copy")},
        {FcitxKey_Cut, NC_("Key name", "Cut")},
        {FcitxKey_Display, NC_("Key name", "Display")},
        {FcitxKey_DOS, NC_("Key name", "DOS")},
        {FcitxKey_Documents, NC_("Key name", "Documents")},
        {FcitxKey_Excel, NC_("Key name", "Spreadsheet")},
        {FcitxKey_Explorer, NC_("Key name", "Browser")},
        {FcitxKey_Game, NC_("Key name", "Game")},
        {FcitxKey_Go, NC_("Key name", "Go")},
        {FcitxKey_iTouch, NC_("Key name", "iTouch")},
        {FcitxKey_LogOff, NC_("Key name", "Logoff")},
        {FcitxKey_Market, NC_("Key name", "Market")},
        {FcitxKey_Meeting, NC_("Key name", "Meeting")},
        {FcitxKey_MenuKB, NC_("Key name", "Keyboard Menu")},
        {FcitxKey_MenuPB, NC_("Key name", "Menu PB")},
        {FcitxKey_MySites, NC_("Key name", "My Sites")},
        {FcitxKey_News, NC_("Key name", "News")},
        {FcitxKey_OfficeHome, NC_("Key name", "Home Office")},
        {FcitxKey_Option, NC_("Key name", "Option")},
        {FcitxKey_Paste, NC_("Key name", "Paste")},
        {FcitxKey_Phone, NC_("Key name", "Phone")},
        {FcitxKey_Reply, NC_("Key name", "Reply")},
        {FcitxKey_Reload, NC_("Key name", "Reload")},
        {FcitxKey_RotateWindows, NC_("Key name", "Rotate Windows")},
        {FcitxKey_RotationPB, NC_("Key name", "Rotation PB")},
        {FcitxKey_RotationKB, NC_("Key name", "Rotation KB")},
        {FcitxKey_Save, NC_("Key name", "Save")},
        {FcitxKey_Send, NC_("Key name", "Send")},
        {FcitxKey_Spell, NC_("Key name", "Spellchecker")},
        {FcitxKey_SplitScreen, NC_("Key name", "Split Screen")},
        {FcitxKey_Support, NC_("Key name", "Support")},
        {FcitxKey_TaskPane, NC_("Key name", "Task Panel")},
        {FcitxKey_Terminal, NC_("Key name", "Terminal")},
        {FcitxKey_Tools, NC_("Key name", "Tools")},
        {FcitxKey_Travel, NC_("Key name", "Travel")},
        {FcitxKey_Video, NC_("Key name", "Video")},
        {FcitxKey_Word, NC_("Key name", "Word Processor")},
        {FcitxKey_Xfer, NC_("Key name", "XFer")},
        {FcitxKey_ZoomIn, NC_("Key name", "Zoom In")},
        {FcitxKey_ZoomOut, NC_("Key name", "Zoom Out")},
        {FcitxKey_Away, NC_("Key name", "Away")},
        {FcitxKey_Messenger, NC_("Key name", "Messenger")},
        {FcitxKey_WebCam, NC_("Key name", "WebCam")},
        {FcitxKey_MailForward, NC_("Key name", "Mail Forward")},
        {FcitxKey_Pictures, NC_("Key name", "Pictures")},
        {FcitxKey_Music, NC_("Key name", "Music")},
        {FcitxKey_Battery, NC_("Key name", "Battery")},
        {FcitxKey_Bluetooth, NC_("Key name", "Bluetooth")},
        {FcitxKey_WLAN, NC_("Key name", "Wireless")},
        {FcitxKey_AudioForward, NC_("Key name", "Media Fast Forward")},
        {FcitxKey_AudioRepeat, NC_("Key name", "Audio Repeat")},
        {FcitxKey_AudioRandomPlay, NC_("Key name", "Audio Random Play")},
        {FcitxKey_Subtitle, NC_("Key name", "Subtitle")},
        {FcitxKey_AudioCycleTrack, NC_("Key name", "Audio Cycle Track")},
        {FcitxKey_Time, NC_("Key name", "Time")},
        {FcitxKey_Hibernate, NC_("Key name", "Hibernate")},
        {FcitxKey_View, NC_("Key name", "View")},
        {FcitxKey_TopMenu, NC_("Key name", "Top Menu")},
        {FcitxKey_PowerDown, NC_("Key name", "Power Down")},
        {FcitxKey_Suspend, NC_("Key name", "Suspend")},
        {FcitxKey_AudioMicMute, NC_("Key name", "Microphone Mute")},
        {FcitxKey_Red, NC_("Key name", "Red")},
        {FcitxKey_Green, NC_("Key name", "Green")},
        {FcitxKey_Yellow, NC_("Key name", "Yellow")},
        {FcitxKey_Blue, NC_("Key name", "Blue")},
        {FcitxKey_New, NC_("Key name", "New")},
        {FcitxKey_Open, NC_("Key name", "Open")},
        {FcitxKey_Find, NC_("Key name", "Find")},
        {FcitxKey_Undo, NC_("Key name", "Undo")},
        {FcitxKey_Redo, NC_("Key name", "Redo")},
        {FcitxKey_Print, NC_("Key name", "Print Screen")},
        {FcitxKey_Insert, NC_("Key name", "Insert")},
        {FcitxKey_Delete, NC_("Key name", "Delete")},
        {FcitxKey_Escape, NC_("Key name", "Escape")},
        {FcitxKey_Sys_Req, NC_("Key name", "System Request")},
        {FcitxKey_Select, NC_("Key name", "Select")},
        {FcitxKey_Kanji, NC_("Key name", "Kanji")},
        {FcitxKey_Muhenkan, NC_("Key name", "Muhenkan")},
        {FcitxKey_Henkan, NC_("Key name", "Henkan")},
        {FcitxKey_Romaji, NC_("Key name", "Romaji")},
        {FcitxKey_Hiragana, NC_("Key name", "Hiragana")},
        {FcitxKey_Katakana, NC_("Key name", "Katakana")},
        {FcitxKey_Hiragana_Katakana, NC_("Key name", "Hiragana Katakana")},
        {FcitxKey_Zenkaku, NC_("Key name", "Zenkaku")},
        {FcitxKey_Hankaku, NC_("Key name", "Hankaku")},
        {FcitxKey_Zenkaku_Hankaku, NC_("Key name", "Zenkaku Hankaku")},
        {FcitxKey_Touroku, NC_("Key name", "Touroku")},
        {FcitxKey_Massyo, NC_("Key name", "Massyo")},
        {FcitxKey_Kana_Lock, NC_("Key name", "Kana Lock")},
        {FcitxKey_Kana_Shift, NC_("Key name", "Kana Shift")},
        {FcitxKey_Eisu_Shift, NC_("Key name", "Eisu Shift")},
        {FcitxKey_Eisu_toggle, NC_("Key name", "Eisu toggle")},
        {FcitxKey_Codeinput, NC_("Key name", "Code input")},
        {FcitxKey_MultipleCandidate, NC_("Key name", "Multiple Candidate")},
        {FcitxKey_PreviousCandidate, NC_("Key name", "Previous Candidate")},
        {FcitxKey_Hangul, NC_("Key name", "Hangul")},
        {FcitxKey_Hangul_Start, NC_("Key name", "Hangul Start")},
        {FcitxKey_Hangul_End, NC_("Key name", "Hangul End")},
        {FcitxKey_Hangul_Hanja, NC_("Key name", "Hangul Hanja")},
        {FcitxKey_Hangul_Jamo, NC_("Key name", "Hangul Jamo")},
        {FcitxKey_Hangul_Romaja, NC_("Key name", "Hangul Romaja")},
        {FcitxKey_Hangul_Jeonja, NC_("Key name", "Hangul Jeonja")},
        {FcitxKey_Hangul_Banja, NC_("Key name", "Hangul Banja")},
        {FcitxKey_Hangul_PreHanja, NC_("Key name", "Hangul PreHanja")},
        {FcitxKey_Hangul_PostHanja, NC_("Key name", "Hangul PostHanja")},
        {FcitxKey_Hangul_Special, NC_("Key name", "Hangul Special")},
        {FcitxKey_Cancel, NC_("Key name", "Cancel")},
        {FcitxKey_Execute, NC_("Key name", "Execute")},
        {FcitxKey_TouchpadToggle, NC_("Key name", "Touchpad Toggle")},
        {FcitxKey_TouchpadOn, NC_("Key name", "Touchpad On")},
        {FcitxKey_TouchpadOff, NC_("Key name", "Touchpad Off")},
        {FcitxKey_VoidSymbol, NC_("Key name", "Void Symbol")},
    };
    std::unordered_map<KeySym, const char *, EnumHash> result;
    for (const auto &item : keyname) {
        result[item.key] = item.name;
    }
    return result;
}

const char *lookupName(KeySym sym) {
    static const std::unordered_map<KeySym, const char *, EnumHash> map =
        makeLookupKeyNameMap();
    const auto *result = findValue(map, sym);
    return result ? *result : nullptr;
}
} // namespace

Key::Key(const char *keyString) : Key() {
    KeyStates states;
    /* old compatible code */
    const char *p = keyString;
    const char *lastModifier = keyString;
    const char *found = nullptr;

#define _CHECK_MODIFIER(NAME, VALUE)                                           \
    if ((found = strstr(p, NAME))) {                                           \
        states |= KeyState::VALUE;                                             \
        if (found + strlen(NAME) > lastModifier) {                             \
            lastModifier = found + strlen(NAME);                               \
        }                                                                      \
    }

    _CHECK_MODIFIER("CTRL_", Ctrl)
    _CHECK_MODIFIER("Control+", Ctrl)
    _CHECK_MODIFIER("ALT_", Alt)
    _CHECK_MODIFIER("Alt+", Alt)
    _CHECK_MODIFIER("SHIFT_", Shift)
    _CHECK_MODIFIER("Shift+", Shift)
    _CHECK_MODIFIER("SUPER_", Super)
    _CHECK_MODIFIER("Super+", Super)
    _CHECK_MODIFIER("HYPER_", Mod3)
    _CHECK_MODIFIER("Hyper+", Mod3)

#undef _CHECK_MODIFIER

    // Special code for keycode baesd parsing.
    std::string keyValue = lastModifier;
    if (stringutils::startsWith(keyValue, "<") &&
        stringutils::endsWith(keyValue, ">")) {
        try {
            code_ = std::stoi(keyValue.substr(1, keyValue.size() - 2));
        } catch (const std::exception &) {
        }
    } else {
        sym_ = keySymFromString(lastModifier);
    }
    states_ = states;
}

bool Key::isReleaseOfModifier(const Key &key) const {
    if (!key.isModifier()) {
        return false;
    }
    auto states = keySymToStates(key.sym()) | key.states();
    // Now we need reverse of keySymToStates.

    std::vector<Key> keys;
    keys.emplace_back(key.sym(), states);
    if (key.states() & KeyState::Ctrl) {
        keys.emplace_back(FcitxKey_Control_L, states);
        keys.emplace_back(FcitxKey_Control_R, states);
    }
    if (key.states() & KeyState::Alt) {
        keys.emplace_back(FcitxKey_Alt_L, states);
        keys.emplace_back(FcitxKey_Alt_R, states);
        keys.emplace_back(FcitxKey_Meta_L, states);
        keys.emplace_back(FcitxKey_Meta_R, states);
    }
    if (key.states() & KeyState::Shift) {
        keys.emplace_back(FcitxKey_Shift_L, states);
        keys.emplace_back(FcitxKey_Shift_R, states);
    }
    if ((key.states() & KeyState::Super) || (key.states() & KeyState::Super2)) {
        keys.emplace_back(FcitxKey_Super_L, states);
        keys.emplace_back(FcitxKey_Super_R, states);
    }
    if ((key.states() & KeyState::Super) || (key.states() & KeyState::Hyper2)) {
        keys.emplace_back(FcitxKey_Hyper_L, states);
        keys.emplace_back(FcitxKey_Hyper_R, states);
    }

    return checkKeyList(keys);
}

bool Key::check(const Key &key) const {
    auto states = states_ & KeyStates({KeyState::Ctrl_Alt_Shift,
                                       KeyState::Super, KeyState::Mod3});
    if (states_.test(KeyState::Super2)) {
        states |= KeyState::Super;
    }

    // key is keycode based, do key code based check.
    if (key.code()) {
        return key.states_ == states && key.code_ == code_;
    }

    if (key.sym() == FcitxKey_None || key.sym() == FcitxKey_VoidSymbol) {
        return false;
    }

    if (isModifier()) {
        Key keyAlt = *this;
        auto states = states_ & (~keySymToStates(sym_));
        keyAlt.states_ |= keySymToStates(sym_);

        return (key.sym_ == sym_ && key.states_ == states) ||
               (key.sym_ == keyAlt.sym_ && key.states_ == keyAlt.states_);
    }

    return (key.sym_ == sym_ && key.states_ == states);
}

bool Key::isDigit() const {
    return !states_ && ((sym_ >= FcitxKey_0 && sym_ <= FcitxKey_9) ||
                        (sym_ >= FcitxKey_KP_0 && sym_ <= FcitxKey_KP_9));
}

int Key::digit() const {
    if (states_) {
        return -1;
    }
    if (sym_ >= FcitxKey_0 && sym_ <= FcitxKey_9) {
        return sym_ - FcitxKey_0;
    }
    if (sym_ >= FcitxKey_KP_0 && sym_ <= FcitxKey_KP_9) {
        return sym_ - FcitxKey_KP_0;
    }
    return -1;
}

int Key::digitSelection(KeyStates states) const {
    auto filteredStates =
        states_ &
        KeyStates({KeyState::Ctrl_Alt_Shift, KeyState::Super, KeyState::Mod3});
    if (filteredStates != states) {
        return -1;
    }

    const int idx = Key(sym_).digit();
    if (idx >= 0) {
        return (idx + 9) % 10;
    }
    return -1;
}

bool Key::isUAZ() const {
    return !states_ && sym_ >= FcitxKey_A && sym_ <= FcitxKey_Z;
}

bool Key::isLAZ() const {
    return !states_ && sym_ >= FcitxKey_a && sym_ <= FcitxKey_z;

    return false;
}

bool Key::isSimple() const {
    return !states_ && sym_ >= FcitxKey_space && sym_ <= FcitxKey_asciitilde;
}

bool Key::isModifier() const {
    return (sym_ == FcitxKey_Control_L || sym_ == FcitxKey_Control_R ||
            sym_ == FcitxKey_Meta_L || sym_ == FcitxKey_Meta_R ||
            sym_ == FcitxKey_Alt_L || sym_ == FcitxKey_Alt_R ||
            sym_ == FcitxKey_Super_L || sym_ == FcitxKey_Super_R ||
            sym_ == FcitxKey_Hyper_L || sym_ == FcitxKey_Hyper_R ||
            sym_ == FcitxKey_Shift_L || sym_ == FcitxKey_Shift_R);
}

bool Key::isCursorMove() const {
    return ((sym_ == FcitxKey_Left || sym_ == FcitxKey_Right ||
             sym_ == FcitxKey_Up || sym_ == FcitxKey_Down ||
             sym_ == FcitxKey_Page_Up || sym_ == FcitxKey_Page_Down ||
             sym_ == FcitxKey_Home || sym_ == FcitxKey_End ||
             sym_ == FcitxKey_KP_Left || sym_ == FcitxKey_KP_Right ||
             sym_ == FcitxKey_KP_Up || sym_ == FcitxKey_KP_Down ||
             sym_ == FcitxKey_KP_Page_Up || sym_ == FcitxKey_KP_Page_Down ||
             sym_ == FcitxKey_KP_Home || sym_ == FcitxKey_KP_End) &&
            (states_ == KeyState::Ctrl || states_ == KeyState::Ctrl_Shift ||
             states_ == KeyState::Shift || states_ == KeyState::NoState));
}

bool Key::isKeyPad() const {
    return ((sym_ >= FcitxKey_KP_Multiply && sym_ <= FcitxKey_KP_9) ||
            (sym_ >= FcitxKey_KP_F1 && sym_ <= FcitxKey_KP_Delete) ||
            sym_ == FcitxKey_KP_Space || sym_ == FcitxKey_KP_Tab ||
            sym_ == FcitxKey_KP_Enter || sym_ == FcitxKey_KP_Equal);
}

bool Key::hasModifier() const { return !!(states_ & KeyState::SimpleMask); }

bool Key::isVirtual() const { return states_.test(KeyState::Virtual); }

Key Key::normalize() const {
    Key key(*this);

    if (key.sym_ == FcitxKey_ISO_Left_Tab) {
        key.sym_ = FcitxKey_Tab;
    }

    /* key state != 0 */
    key.states_ =
        key.states_ & KeyStates({KeyState::Ctrl_Alt_Shift, KeyState::Super,
                                 KeyState::Mod3, KeyState::Super2});
    if (key.states_.test(KeyState::Super2)) {
        key.states_ = key.states_.unset(KeyState::Super2);
        key.states_ |= KeyState::Super;
    }
    if (key.states_) {
        if (key.states_ != KeyState::Shift && Key(key.sym_).isLAZ()) {
            key.sym_ = static_cast<KeySym>(key.sym_ + FcitxKey_A - FcitxKey_a);
        }
        /*
         * alt shift 1 should be alt + !
         * shift+s should be S
         */

        if (Key(key.sym_).isLAZ() || Key(key.sym_).isUAZ()) {
            if (key.states_ == KeyState::Shift) {
                key.states_ = 0;
            }
        } else {
            if ((key.states_ & KeyState::Shift) &&
                (((Key(key.sym_).isSimple() ||
                   keySymToUnicode(key.sym_) != 0) &&
                  key.sym_ != FcitxKey_space && key.sym_ != FcitxKey_Return &&
                  key.sym_ != FcitxKey_Tab) ||
                 (key.sym_ >= FcitxKey_KP_0 && key.sym_ <= FcitxKey_KP_9))) {
                key.states_ ^= KeyState::Shift;
            }
        }
    }

    return key;
}

bool Key::isValid() const {
    // Sym is valid, or code is valid.
    return (sym_ != FcitxKey_None && sym_ != FcitxKey_VoidSymbol) || code_ != 0;
}

std::string Key::toString(KeyStringFormat format) const {

    std::string key;
    if (code_ && sym_ == FcitxKey_None) {
        key = "<";
        key += std::to_string(code_);
        key += ">";
    } else {
        auto sym = sym_;
        if (sym == FcitxKey_None) {
            return std::string();
        }

        if (sym == FcitxKey_ISO_Left_Tab) {
            sym = FcitxKey_Tab;
        }
        key = keySymToString(sym, format);
    }

    if (key.empty()) {
        return std::string();
    }

    std::string str;
    auto states = states_;
    if (format == KeyStringFormat::Localized && isModifier()) {
        states &= (~keySymToStates(sym_));
    }

#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (states & VALUE) {                                                      \
        str += STR;                                                            \
        str += "+";                                                            \
    }
    if (format == KeyStringFormat::Portable) {
        _APPEND_MODIFIER_STRING("Control", KeyState::Ctrl)
        _APPEND_MODIFIER_STRING("Alt", KeyState::Alt)
        _APPEND_MODIFIER_STRING("Shift", KeyState::Shift)
        _APPEND_MODIFIER_STRING("Super",
                                (KeyStates{KeyState::Super, KeyState::Super2}))
        _APPEND_MODIFIER_STRING("Hyper",
                                (KeyStates{KeyState::Hyper, KeyState::Hyper2}))
    } else {
        _APPEND_MODIFIER_STRING(C_("Key name", "Control"), KeyState::Ctrl)
        _APPEND_MODIFIER_STRING(C_("Key name", "Alt"), KeyState::Alt)
        _APPEND_MODIFIER_STRING(C_("Key name", "Shift"), KeyState::Shift)
        _APPEND_MODIFIER_STRING(C_("Key name", "Super"),
                                (KeyStates{KeyState::Super, KeyState::Super2}))
        _APPEND_MODIFIER_STRING(C_("Key name", "Hyper"),
                                (KeyStates{KeyState::Hyper, KeyState::Hyper2}))
    }

#undef _APPEND_MODIFIER_STRING
    str += key;

    return str;
}

KeyStates Key::keySymToStates(KeySym sym) {
    switch (sym) {
    case FcitxKey_Control_L:
    case FcitxKey_Control_R:
        return KeyState::Ctrl;
    case FcitxKey_Alt_L:
    case FcitxKey_Alt_R:
    case FcitxKey_Meta_L:
    case FcitxKey_Meta_R:
        return KeyState::Alt;
    case FcitxKey_Shift_L:
    case FcitxKey_Shift_R:
        return KeyState::Shift;
    case FcitxKey_Super_L:
    case FcitxKey_Super_R:
    case FcitxKey_Hyper_L:
    case FcitxKey_Hyper_R:
        return KeyState::Super;
    default:
        return KeyStates();
    }
}

KeySym Key::keySymFromString(const std::string &keyString) {
    const auto *value = std::lower_bound(
        keyValueByNameOffset,
        keyValueByNameOffset + FCITX_ARRAY_SIZE(keyValueByNameOffset),
        keyString, [](const uint32_t &idx, const std::string &str) {
            return keyNameList[&idx - keyValueByNameOffset] < str;
        });

    if (value !=
            keyValueByNameOffset + FCITX_ARRAY_SIZE(keyValueByNameOffset) &&
        keyString == keyNameList[value - keyValueByNameOffset]) {
        return static_cast<KeySym>(*value);
    }

    const auto *compat = std::lower_bound(
        keyNameListCompat,
        keyNameListCompat + FCITX_ARRAY_SIZE(keyNameListCompat), keyString,
        [](const KeyNameListCompat &c, const std::string &str) {
            return c.name < str;
        });
    if (compat != keyNameListCompat + FCITX_ARRAY_SIZE(keyNameListCompat) &&
        compat->name == keyString) {
        return compat->sym;
    }

    if (fcitx::utf8::lengthValidated(keyString) == 1) {
        auto chr = fcitx::utf8::getChar(keyString);
        if (chr > 0) {
            if (fcitx::utf8::ncharByteLength(keyString.begin(), 1) == 1) {
                return static_cast<KeySym>(keyString[0]);
            }
            return keySymFromUnicode(chr);
        }
    }

    return FcitxKey_None;
}

std::string Key::keySymToString(KeySym sym, KeyStringFormat format) {
    if (format == KeyStringFormat::Localized) {
        if (const auto *name = lookupName(sym)) {
            return C_("Key name", name);
        }
        auto code = keySymToUnicode(sym);
        if (code < 0x7f) {
            if (charutils::isprint(code)) {
                return utf8::UCS4ToUTF8(code);
            }
        } else {
            return utf8::UCS4ToUTF8(code);
        }
    }

    const KeyNameOffsetByValue *result = std::lower_bound(
        keyNameOffsetByValue,
        keyNameOffsetByValue + FCITX_ARRAY_SIZE(keyNameOffsetByValue), sym,
        [](const KeyNameOffsetByValue &item, KeySym key) {
            return item.sym < key;
        });
    if (result !=
            keyNameOffsetByValue + FCITX_ARRAY_SIZE(keyNameOffsetByValue) &&
        result->sym == sym) {
        return keyNameList[result->offset];
    }
    return std::string();
}

KeySym Key::keySymFromUnicode(uint32_t unicode) {
    const auto &tab = unicode_to_keysym_tab();
    int min = 0;
    int max = tab.size() - 1;
    int mid;

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((unicode >= 0x0020 && unicode <= 0x007e) ||
        (unicode >= 0x00a0 && unicode <= 0x00ff))
        return static_cast<KeySym>(unicode);

    /* special keysyms */
    if ((unicode >= (FcitxKey_BackSpace & 0x7f) &&
         unicode <= (FcitxKey_Clear & 0x7f)) ||
        unicode == (FcitxKey_Return & 0x7f) ||
        unicode == (FcitxKey_Escape & 0x7f))
        return static_cast<KeySym>(unicode | 0xff00);
    if (unicode == (FcitxKey_Delete & 0x7f))
        return FcitxKey_Delete;

    /* Unicode non-symbols and code points outside Unicode planes */
    if ((unicode >= 0xd800 && unicode <= 0xdfff) ||
        (unicode >= 0xfdd0 && unicode <= 0xfdef) || unicode > 0x10ffff ||
        (unicode & 0xfffe) == 0xfffe)
        return FcitxKey_None;

    /* Binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (tab[mid].ucs < unicode) {
            min = mid + 1;
        } else if (tab[mid].ucs > unicode) {
            max = mid - 1;
        } else {
            /* found it */
            return static_cast<KeySym>(tab[mid].keysym);
        }
    }

    /*
     * No matching keysym value found, return Unicode value plus 0x01000000
     * (a convention introduced in the UTF-8 work on xterm).
     */
    return static_cast<KeySym>(unicode | 0x01000000);
}

uint32_t Key::keySymToUnicode(KeySym sym) {
    int min = 0;
    int max =
        sizeof(keysym_to_unicode_tab) / sizeof(keysym_to_unicode_tab[0]) - 1;
    int mid;

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((sym >= 0x0020 && sym <= 0x007e) || (sym >= 0x00a0 && sym <= 0x00ff)) {
        return sym;
    }

    /* patch encoding botch */
    if (sym == FcitxKey_KP_Space) {
        return FcitxKey_space & 0x7f;
    }

    /* special syms */
    if ((sym >= FcitxKey_BackSpace && sym <= FcitxKey_Clear) ||
        (sym >= FcitxKey_KP_Multiply && sym <= FcitxKey_KP_9) ||
        sym == FcitxKey_Return || sym == FcitxKey_Escape ||
        sym == FcitxKey_Delete || sym == FcitxKey_KP_Tab ||
        sym == FcitxKey_KP_Enter || sym == FcitxKey_KP_Equal) {
        return sym & 0x7f;
    }

    /* also check for directly encoded Unicode codepoints */

    /* Exclude surrogates: they are invalid in UTF-32.
     * See https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G28875
     * for further details.
     */
    if (0x0100d800 <= sym && sym <= 0x0100dfff) {
        return 0;
    }
    /*
     * In theory, this is supposed to start from 0x100100, such that the ASCII
     * range, which is already covered by 0x00-0xff, can't be encoded in two
     * ways. However, changing this after a couple of decades probably won't
     * go well, so it stays as it is.
     */
    if (0x01000000 <= sym && sym <= 0x0110ffff) {
        const uint32_t code = sym - 0x01000000;
        if (utf8::UCS4IsValid(code)) {
            return code;
        }
        return 0;
    }

    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (keysym_to_unicode_tab[mid].keysym < sym) {
            min = mid + 1;
        } else if (keysym_to_unicode_tab[mid].keysym > sym) {
            max = mid - 1;
        } else {
            /* found it */
            return keysym_to_unicode_tab[mid].ucs;
        }
    }

    /* No matching Unicode value found */
    return 0;
}

std::string Key::keySymToUTF8(KeySym sym) {
    auto code = keySymToUnicode(sym);
    if (!utf8::UCS4IsValid(code)) {
        return "";
    }
    return utf8::UCS4ToUTF8(code);
}

std::vector<Key> Key::keyListFromString(const std::string &str) {
    std::vector<Key> keyList;

    auto lastPos = str.find_first_not_of(FCITX_WHITESPACE, 0);
    auto pos = str.find_first_of(FCITX_WHITESPACE, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        Key key(str.substr(lastPos, pos - lastPos));

        if (key.sym() != FcitxKey_None) {
            keyList.push_back(key);
        }
        lastPos = str.find_first_not_of(FCITX_WHITESPACE, pos);
        pos = str.find_first_of(FCITX_WHITESPACE, lastPos);
    }

    return keyList;
}
} // namespace fcitx
