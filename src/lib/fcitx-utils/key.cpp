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
        {FcitxKey_space, N_("Space")},
        {FcitxKey_Escape, N_("Esc")},
        {FcitxKey_Tab, N_("Tab")},
        {FcitxKey_BackSpace, N_("Backspace")},
        {FcitxKey_Return, N_("Return")},
        {FcitxKey_Insert, N_("Ins")},
        {FcitxKey_Delete, N_("Del")},
        {FcitxKey_Pause, N_("Pause")},
        {FcitxKey_Print, N_("Print")},
        {FcitxKey_Sys_Req, N_("SysReq")},
        {FcitxKey_Home, N_("Home")},
        {FcitxKey_End, N_("End")},
        {FcitxKey_Left, N_("Left")},
        {FcitxKey_Up, N_("Up")},
        {FcitxKey_Right, N_("Right")},
        {FcitxKey_Down, N_("Down")},
        {FcitxKey_Page_Up, N_("PgUp")},
        {FcitxKey_Page_Down, N_("PgDown")},
        {FcitxKey_Caps_Lock, N_("CapsLock")},
        {FcitxKey_Num_Lock, N_("NumLock")},
        {FcitxKey_Scroll_Lock, N_("ScrollLock")},
        {FcitxKey_Menu, N_("Menu")},
        {FcitxKey_Help, N_("Help")},
        {FcitxKey_Back, N_("Back")},
        {FcitxKey_Forward, N_("Forward")},
        {FcitxKey_Stop, N_("Stop")},
        {FcitxKey_Refresh, N_("Refresh")},
        {FcitxKey_AudioLowerVolume, N_("Volume Down")},
        {FcitxKey_AudioMute, N_("Volume Mute")},
        {FcitxKey_AudioRaiseVolume, N_("Volume Up")},
        {FcitxKey_AudioPlay, N_("Media Play")},
        {FcitxKey_AudioStop, N_("Media Stop")},
        {FcitxKey_AudioPrev, N_("Media Previous")},
        {FcitxKey_AudioNext, N_("Media Next")},
        {FcitxKey_AudioRecord, N_("Media Record")},
        {FcitxKey_AudioPause, N_("Media Pause")},
        {FcitxKey_HomePage, N_("Home Page")},
        {FcitxKey_Favorites, N_("Favorites")},
        {FcitxKey_Search, N_("Search")},
        {FcitxKey_Standby, N_("Standby")},
        {FcitxKey_OpenURL, N_("Open URL")},
        {FcitxKey_Mail, N_("Launch Mail")},
        {FcitxKey_Launch0, N_("Launch (0)")},
        {FcitxKey_Launch1, N_("Launch (1)")},
        {FcitxKey_Launch2, N_("Launch (2)")},
        {FcitxKey_Launch3, N_("Launch (3)")},
        {FcitxKey_Launch4, N_("Launch (4)")},
        {FcitxKey_Launch5, N_("Launch (5)")},
        {FcitxKey_Launch6, N_("Launch (6)")},
        {FcitxKey_Launch7, N_("Launch (7)")},
        {FcitxKey_Launch8, N_("Launch (8)")},
        {FcitxKey_Launch9, N_("Launch (9)")},
        {FcitxKey_LaunchA, N_("Launch (A)")},
        {FcitxKey_LaunchB, N_("Launch (B)")},
        {FcitxKey_LaunchC, N_("Launch (C)")},
        {FcitxKey_LaunchD, N_("Launch (D)")},
        {FcitxKey_LaunchE, N_("Launch (E)")},
        {FcitxKey_LaunchF, N_("Launch (F)")},
        {FcitxKey_MonBrightnessUp, N_("Monitor Brightness Up")},
        {FcitxKey_MonBrightnessDown, N_("Monitor Brightness Down")},
        {FcitxKey_KbdLightOnOff, N_("Keyboard Light On/Off")},
        {FcitxKey_KbdBrightnessUp, N_("Keyboard Brightness Up")},
        {FcitxKey_KbdBrightnessDown, N_("Keyboard Brightness Down")},
        {FcitxKey_PowerOff, N_("Power Off")},
        {FcitxKey_WakeUp, N_("Wake Up")},
        {FcitxKey_Eject, N_("Eject")},
        {FcitxKey_ScreenSaver, N_("Screensaver")},
        {FcitxKey_WWW, N_("WWW")},
        {FcitxKey_Sleep, N_("Sleep")},
        {FcitxKey_LightBulb, N_("LightBulb")},
        {FcitxKey_Shop, N_("Shop")},
        {FcitxKey_History, N_("History")},
        {FcitxKey_AddFavorite, N_("Add Favorite")},
        {FcitxKey_HotLinks, N_("Hot Links")},
        {FcitxKey_BrightnessAdjust, N_("Adjust Brightness")},
        {FcitxKey_Finance, N_("Finance")},
        {FcitxKey_Community, N_("Community")},
        {FcitxKey_AudioRewind, N_("Media Rewind")},
        {FcitxKey_BackForward, N_("Back Forward")},
        {FcitxKey_ApplicationLeft, N_("Application Left")},
        {FcitxKey_ApplicationRight, N_("Application Right")},
        {FcitxKey_Book, N_("Book")},
        {FcitxKey_CD, N_("CD")},
        {FcitxKey_Calculator, N_("Calculator")},
        {FcitxKey_Clear, N_("Clear")},
        {FcitxKey_Close, N_("Close")},
        {FcitxKey_Copy, N_("Copy")},
        {FcitxKey_Cut, N_("Cut")},
        {FcitxKey_Display, N_("Display")},
        {FcitxKey_DOS, N_("DOS")},
        {FcitxKey_Documents, N_("Documents")},
        {FcitxKey_Excel, N_("Spreadsheet")},
        {FcitxKey_Explorer, N_("Browser")},
        {FcitxKey_Game, N_("Game")},
        {FcitxKey_Go, N_("Go")},
        {FcitxKey_iTouch, N_("iTouch")},
        {FcitxKey_LogOff, N_("Logoff")},
        {FcitxKey_Market, N_("Market")},
        {FcitxKey_Meeting, N_("Meeting")},
        {FcitxKey_MenuKB, N_("Keyboard Menu")},
        {FcitxKey_MenuPB, N_("Menu PB")},
        {FcitxKey_MySites, N_("My Sites")},
        {FcitxKey_News, N_("News")},
        {FcitxKey_OfficeHome, N_("Home Office")},
        {FcitxKey_Option, N_("Option")},
        {FcitxKey_Paste, N_("Paste")},
        {FcitxKey_Phone, N_("Phone")},
        {FcitxKey_Reply, N_("Reply")},
        {FcitxKey_Reload, N_("Reload")},
        {FcitxKey_RotateWindows, N_("Rotate Windows")},
        {FcitxKey_RotationPB, N_("Rotation PB")},
        {FcitxKey_RotationKB, N_("Rotation KB")},
        {FcitxKey_Save, N_("Save")},
        {FcitxKey_Send, N_("Send")},
        {FcitxKey_Spell, N_("Spellchecker")},
        {FcitxKey_SplitScreen, N_("Split Screen")},
        {FcitxKey_Support, N_("Support")},
        {FcitxKey_TaskPane, N_("Task Panel")},
        {FcitxKey_Terminal, N_("Terminal")},
        {FcitxKey_Tools, N_("Tools")},
        {FcitxKey_Travel, N_("Travel")},
        {FcitxKey_Video, N_("Video")},
        {FcitxKey_Word, N_("Word Processor")},
        {FcitxKey_Xfer, N_("XFer")},
        {FcitxKey_ZoomIn, N_("Zoom In")},
        {FcitxKey_ZoomOut, N_("Zoom Out")},
        {FcitxKey_Away, N_("Away")},
        {FcitxKey_Messenger, N_("Messenger")},
        {FcitxKey_WebCam, N_("WebCam")},
        {FcitxKey_MailForward, N_("Mail Forward")},
        {FcitxKey_Pictures, N_("Pictures")},
        {FcitxKey_Music, N_("Music")},
        {FcitxKey_Battery, N_("Battery")},
        {FcitxKey_Bluetooth, N_("Bluetooth")},
        {FcitxKey_WLAN, N_("Wireless")},
        {FcitxKey_AudioForward, N_("Media Fast Forward")},
        {FcitxKey_AudioRepeat, N_("Audio Repeat")},
        {FcitxKey_AudioRandomPlay, N_("Audio Random Play")},
        {FcitxKey_Subtitle, N_("Subtitle")},
        {FcitxKey_AudioCycleTrack, N_("Audio Cycle Track")},
        {FcitxKey_Time, N_("Time")},
        {FcitxKey_Hibernate, N_("Hibernate")},
        {FcitxKey_View, N_("View")},
        {FcitxKey_TopMenu, N_("Top Menu")},
        {FcitxKey_PowerDown, N_("Power Down")},
        {FcitxKey_Suspend, N_("Suspend")},
        {FcitxKey_AudioMicMute, N_("Microphone Mute")},
        {FcitxKey_Red, N_("Red")},
        {FcitxKey_Green, N_("Green")},
        {FcitxKey_Yellow, N_("Yellow")},
        {FcitxKey_Blue, N_("Blue")},
        {FcitxKey_New, N_("New")},
        {FcitxKey_Open, N_("Open")},
        {FcitxKey_Find, N_("Find")},
        {FcitxKey_Undo, N_("Undo")},
        {FcitxKey_Redo, N_("Redo")},
        {FcitxKey_Print, N_("Print Screen")},
        {FcitxKey_Insert, N_("Insert")},
        {FcitxKey_Delete, N_("Delete")},
        {FcitxKey_Escape, N_("Escape")},
        {FcitxKey_Sys_Req, N_("System Request")},
        {FcitxKey_Select, N_("Select")},
        {FcitxKey_Kanji, N_("Kanji")},
        {FcitxKey_Muhenkan, N_("Muhenkan")},
        {FcitxKey_Henkan, N_("Henkan")},
        {FcitxKey_Romaji, N_("Romaji")},
        {FcitxKey_Hiragana, N_("Hiragana")},
        {FcitxKey_Katakana, N_("Katakana")},
        {FcitxKey_Hiragana_Katakana, N_("Hiragana Katakana")},
        {FcitxKey_Zenkaku, N_("Zenkaku")},
        {FcitxKey_Hankaku, N_("Hankaku")},
        {FcitxKey_Zenkaku_Hankaku, N_("Zenkaku Hankaku")},
        {FcitxKey_Touroku, N_("Touroku")},
        {FcitxKey_Massyo, N_("Massyo")},
        {FcitxKey_Kana_Lock, N_("Kana Lock")},
        {FcitxKey_Kana_Shift, N_("Kana Shift")},
        {FcitxKey_Eisu_Shift, N_("Eisu Shift")},
        {FcitxKey_Eisu_toggle, N_("Eisu toggle")},
        {FcitxKey_Codeinput, N_("Code input")},
        {FcitxKey_MultipleCandidate, N_("Multiple Candidate")},
        {FcitxKey_PreviousCandidate, N_("Previous Candidate")},
        {FcitxKey_Hangul, N_("Hangul")},
        {FcitxKey_Hangul_Start, N_("Hangul Start")},
        {FcitxKey_Hangul_End, N_("Hangul End")},
        {FcitxKey_Hangul_Hanja, N_("Hangul Hanja")},
        {FcitxKey_Hangul_Jamo, N_("Hangul Jamo")},
        {FcitxKey_Hangul_Romaja, N_("Hangul Romaja")},
        {FcitxKey_Hangul_Jeonja, N_("Hangul Jeonja")},
        {FcitxKey_Hangul_Banja, N_("Hangul Banja")},
        {FcitxKey_Hangul_PreHanja, N_("Hangul PreHanja")},
        {FcitxKey_Hangul_PostHanja, N_("Hangul PostHanja")},
        {FcitxKey_Hangul_Special, N_("Hangul Special")},
        {FcitxKey_Cancel, N_("Cancel")},
        {FcitxKey_Execute, N_("Execute")},
        {FcitxKey_TouchpadToggle, N_("Touchpad Toggle")},
        {FcitxKey_TouchpadOn, N_("Touchpad On")},
        {FcitxKey_TouchpadOff, N_("Touchpad Off")},
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

std::string Key::toString() const {

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
        key = keySymToString(sym);
    }

    if (key.empty())
        return std::string();

    std::string str;
#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (states_ & KeyState::VALUE) {                                           \
        str += STR;                                                            \
    }
    _APPEND_MODIFIER_STRING("Control+", Ctrl)
    _APPEND_MODIFIER_STRING("Alt+", Alt)
    _APPEND_MODIFIER_STRING("Shift+", Shift)
    _APPEND_MODIFIER_STRING("Super+", Super)

#undef _APPEND_MODIFIER_STRING
    str += key;

    return str;
}

std::string Key::keySymToDisplayString(KeySym sym) {
    if (auto name = lookupName(sym)) {
        return _(name);
    }
    return keySymToString(sym);
}

std::string Key::toDisplayString() const {
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
        key = keySymToDisplayString(sym);
    }

    if (key.empty())
        return std::string();

    std::string str;
#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (states_ & KeyState::VALUE) {                                           \
        str += STR;                                                            \
        str += "+";                                                            \
    }
    _APPEND_MODIFIER_STRING(_("Control"), Ctrl)
    _APPEND_MODIFIER_STRING(_("Alt"), Alt)
    _APPEND_MODIFIER_STRING(_("Shift"), Shift)
    _APPEND_MODIFIER_STRING(_("Super"), Super)

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

std::string Key::keySymToString(KeySym sym) {
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
