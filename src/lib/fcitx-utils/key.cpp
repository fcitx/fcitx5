/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "key.h"
#include "charutils.h"
#include "i18n.h"
#include "keydata.h"
#include "keynametable-compat.h"
#include "keynametable.h"
#include "stringutils.h"
#include "utf8.h"
#include <cstring>
#include <fcitx/misc_p.h>
#include <unordered_map>

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
        {FcitxKey_Page_Up, NC_("Key name", "PgUp")},
        {FcitxKey_Page_Down, NC_("Key name", "PgDown")},
        {FcitxKey_Caps_Lock, NC_("Key name", "CapsLock")},
        {FcitxKey_Num_Lock, NC_("Key name", "NumLock")},
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
    };
    std::unordered_map<KeySym, const char *, EnumHash> result;
    for (const auto &item : keyname) {
        result[item.key] = item.name;
    }
    return result;
}

const char *lookupName(KeySym sym) {
    static std::unordered_map<KeySym, const char *, EnumHash> map =
        makeLookupKeyNameMap();
    auto result = findValue(map, sym);
    return result ? *result : nullptr;
}
}

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

bool Key::check(const Key &key) const {
    auto states =
        states_ & KeyStates({KeyState::Ctrl_Alt_Shift, KeyState::Super});

    // key is keycode based, do key code based check.
    if (key.code()) {
        return key.states_ == states && key.code_ == code_;
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
    return !states_ && sym_ >= FcitxKey_0 && sym_ <= FcitxKey_9;
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
            sym_ == FcitxKey_Alt_L || sym_ == FcitxKey_Alt_R ||
            sym_ == FcitxKey_Super_L || sym_ == FcitxKey_Super_R ||
            sym_ == FcitxKey_Hyper_L || sym_ == FcitxKey_Hyper_R ||
            sym_ == FcitxKey_Shift_L || sym_ == FcitxKey_Shift_R);
}

bool Key::isCursorMove() const {
    return ((sym_ == FcitxKey_Left || sym_ == FcitxKey_Right ||
             sym_ == FcitxKey_Up || sym_ == FcitxKey_Down ||
             sym_ == FcitxKey_Page_Up || sym_ == FcitxKey_Page_Down ||
             sym_ == FcitxKey_Home || sym_ == FcitxKey_End) &&
            (states_ == KeyState::Ctrl || states_ == KeyState::Ctrl_Shift ||
             states_ == KeyState::Shift || states_ == KeyState::None));
}

bool Key::hasModifier() const { return !!(states_ & KeyState::SimpleMask); }

Key Key::normalize() const {
    Key key(*this);
    /* key state != 0 */
    key.states_ =
        key.states_ & KeyStates({KeyState::Ctrl_Alt_Shift, KeyState::Super});
    if (key.states_) {
        if (key.states_ != KeyState::Shift && Key(key.sym_).isLAZ()) {
            key.sym_ = static_cast<KeySym>(key.sym_ + FcitxKey_A - FcitxKey_a);
        }
        /*
         * alt shift 1 shoud be alt + !
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
                  key.sym_ != FcitxKey_space && key.sym_ != FcitxKey_Return) ||
                 (key.sym_ >= FcitxKey_KP_0 && key.sym_ <= FcitxKey_KP_9))) {
                key.states_ ^= KeyState::Shift;
            }
        }
    }

    if (key.sym_ == FcitxKey_ISO_Left_Tab) {
        key.sym_ = FcitxKey_Tab;
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

        if (sym == FcitxKey_ISO_Left_Tab)
            sym = FcitxKey_Tab;
        key = keySymToString(sym, format);
    }

    if (key.empty())
        return std::string();

    std::string str;
#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (states_ & KeyState::VALUE) {                                           \
        str += STR;                                                            \
        str += "+";                                                            \
    }
    if (format == KeyStringFormat::Portable) {
        _APPEND_MODIFIER_STRING("Control", Ctrl)
        _APPEND_MODIFIER_STRING("Alt", Alt)
        _APPEND_MODIFIER_STRING("Shift", Shift)
        _APPEND_MODIFIER_STRING("Super", Super)
    } else {
        _APPEND_MODIFIER_STRING(C_("Key name", "Control"), Ctrl)
        _APPEND_MODIFIER_STRING(C_("Key name", "Alt"), Alt)
        _APPEND_MODIFIER_STRING(C_("Key name", "Shift"), Shift)
        _APPEND_MODIFIER_STRING(C_("Key name", "Super"), Super)
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
        return KeyState::Alt;
    case FcitxKey_Shift_L:
    case FcitxKey_Shift_R:
        return KeyState::Shift;
    case FcitxKey_Super_L:
    case FcitxKey_Super_R:
        return KeyState::Super;
    case FcitxKey_Hyper_L:
    case FcitxKey_Hyper_R:
        return KeyState::Hyper;
    default:
        return KeyStates();
    }
}

KeySym Key::keySymFromString(const std::string &keyString) {
    auto value = std::lower_bound(
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

    auto compat = std::lower_bound(
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
            } else {
                return keySymFromUnicode(chr);
            }
        }
    }

    return FcitxKey_None;
}

std::string Key::keySymToString(KeySym sym, KeyStringFormat format) {
    if (format == KeyStringFormat::Localized) {
        if (auto name = lookupName(sym)) {
            return C_("Key name", name);
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

KeySym Key::keySymFromUnicode(uint32_t wc) {
    int min = 0;
    int max = sizeof(gdk_unicode_to_keysym_tab) /
                  sizeof(gdk_unicode_to_keysym_tab[0]) -
              1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((wc >= 0x0020 && wc <= 0x007e) || (wc >= 0x00a0 && wc <= 0x00ff))
        return static_cast<KeySym>(wc);

    /* Binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_unicode_to_keysym_tab[mid].ucs < wc)
            min = mid + 1;
        else if (gdk_unicode_to_keysym_tab[mid].ucs > wc)
            max = mid - 1;
        else {
            /* found it */
            return static_cast<KeySym>(gdk_unicode_to_keysym_tab[mid].keysym);
        }
    }

    /*
    * No matching keysym value found, return Unicode value plus 0x01000000
    * (a convention introduced in the UTF-8 work on xterm).
    */
    return static_cast<KeySym>(wc | 0x01000000);
}

uint32_t Key::keySymToUnicode(KeySym keyval) {
    int min = 0;
    int max = sizeof(gdk_keysym_to_unicode_tab) /
                  sizeof(gdk_keysym_to_unicode_tab[0]) -
              1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((keyval >= 0x0020 && keyval <= 0x007e) ||
        (keyval >= 0x00a0 && keyval <= 0x00ff))
        return keyval;

    /* Also check for directly encoded 24-bit UCS characters:
    */
    if ((keyval & 0xff000000) == 0x01000000)
        return keyval & 0x00ffffff;

    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_keysym_to_unicode_tab[mid].keysym < keyval)
            min = mid + 1;
        else if (gdk_keysym_to_unicode_tab[mid].keysym > keyval)
            max = mid - 1;
        else {
            /* found it */
            return gdk_keysym_to_unicode_tab[mid].ucs;
        }
    }

    /* No matching Unicode value found */
    return 0;
}

std::string Key::keySymToUTF8(KeySym keyval) {
    return utf8::UCS4ToUTF8(keySymToUnicode(keyval));
}

std::vector<Key> Key::keyListFromString(const std::string &keyString) {
    std::vector<Key> keyList;

    auto lastPos = keyString.find_first_not_of(FCITX_WHITESPACE, 0);
    auto pos = keyString.find_first_of(FCITX_WHITESPACE, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        Key key(keyString.substr(lastPos, pos - lastPos));

        if (key.sym() != FcitxKey_None) {
            keyList.push_back(key);
        }
        lastPos = keyString.find_first_not_of(FCITX_WHITESPACE, pos);
        pos = keyString.find_first_of(FCITX_WHITESPACE, lastPos);
    }

    return keyList;
}
}
