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
#include "keydata.h"
#include "keynametable-compat.h"
#include "keynametable.h"
#include "stringutils.h"
#include "utf8.h"
#include <cstring>

namespace fcitx {
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

std::string Key::toString() const {

    std::string key;
    if (code_) {
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
