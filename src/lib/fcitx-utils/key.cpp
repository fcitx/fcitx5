/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include <cstring>
#include "key.h"
#include "keynametable.h"
#include "keynametable-compat.h"
#include "keydata.h"
#include "charutils.h"
#include "utf8.h"

namespace fcitx {
Key::Key(const char *keyString) {
    KeyStates states;
    /* old compatible code */
    const char *p = keyString;
    const char *lastModifier = keyString;
    const char *found = NULL;

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

    m_sym = keySymFromString(lastModifier);
    m_states = states;
}

Key::~Key() {}

bool Key::check(const Key &key) const {
    bool isModifier = key.isModifier();
    if (isModifier) {
        Key keyAlt = key;
        auto states = key.m_states & (~keySymToStates(key.m_sym));
        keyAlt.m_states |= keySymToStates(key.m_sym);

        return (m_sym == key.m_sym && m_states == states) ||
               (m_sym == keyAlt.m_sym && m_states == keyAlt.m_states);
    }

    auto states =
        m_states & KeyStates({KeyState::Ctrl_Alt_Shift, KeyState::Super});
    return (m_sym == key.m_sym && states == key.m_states);
}

bool Key::isDigit() const {
    return !m_states && m_sym >= FcitxKey_0 && m_sym <= FcitxKey_9;
}

bool Key::isUAZ() const {
    return !m_states && m_sym >= FcitxKey_A && m_sym <= FcitxKey_Z;
}

bool Key::isLAZ() const {
    return !m_states && m_sym >= FcitxKey_a && m_sym <= FcitxKey_z;

    return false;
}

bool Key::isSimple() const {
    return !m_states && m_sym >= FcitxKey_space && m_sym <= FcitxKey_asciitilde;
}

bool Key::isModifier() const {
    return (m_sym == FcitxKey_Control_L || m_sym == FcitxKey_Control_R ||
            m_sym == FcitxKey_Alt_L || m_sym == FcitxKey_Alt_R ||
            m_sym == FcitxKey_Super_L || m_sym == FcitxKey_Super_R ||
            m_sym == FcitxKey_Hyper_L || m_sym == FcitxKey_Hyper_R ||
            m_sym == FcitxKey_Shift_L || m_sym == FcitxKey_Shift_R);
}

bool Key::isCursorMove() const {
    return ((m_sym == FcitxKey_Left || m_sym == FcitxKey_Right ||
             m_sym == FcitxKey_Up || m_sym == FcitxKey_Down ||
             m_sym == FcitxKey_Page_Up || m_sym == FcitxKey_Page_Down ||
             m_sym == FcitxKey_Home || m_sym == FcitxKey_End) &&
            (m_states == KeyState::Ctrl || m_states == KeyState::Ctrl_Shift ||
             m_states == KeyState::Shift || m_states == KeyState::None));
}

bool Key::hasModifier() const { return !!(m_states & KeyState::SimpleMask); }

Key Key::normalize() const {
    Key key(*this);
    /* key state != 0 */
    if (key.m_states) {
        if (key.m_states != KeyState::Shift && Key(key.m_sym).isLAZ()) {
            key.m_sym =
                static_cast<KeySym>(key.m_sym + FcitxKey_A - FcitxKey_a);
        }
        /*
         * alt shift 1 shoud be alt + !
         * shift+s should be S
         */

        if (Key(key.m_sym).isLAZ() || Key(key.m_sym).isUAZ()) {
            if (key.m_states == KeyState::Shift) {
                key.m_states = 0;
            }
        } else {
            if ((key.m_states & KeyState::Shift) &&
                (((Key(key.m_sym).isSimple() ||
                   keySymToUnicode(key.m_sym) != 0) &&
                  key.m_sym != FcitxKey_space &&
                  key.m_sym != FcitxKey_Return) ||
                 (key.m_sym >= FcitxKey_KP_0 && key.m_sym <= FcitxKey_KP_9))) {
                key.m_states ^= KeyState::Shift;
            }
        }
    }

    if (key.m_sym == FcitxKey_ISO_Left_Tab) {
        key.m_sym = FcitxKey_Tab;
    }

    return key;
}

std::string Key::toString() const {
    auto sym = m_sym;
    if (sym == FcitxKey_None) {
        return std::string();
    }

    if (sym == FcitxKey_ISO_Left_Tab)
        sym = FcitxKey_Tab;

    auto key = keySymToString(sym);

    if (key.empty())
        return std::string();

    std::string str;
#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (m_states & KeyState::VALUE) {                                          \
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
    return KeyStates();
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
        [](const KeyNameListCompat &c,
           const std::string &str) { return c.name < str; });
    if (compat != keyNameListCompat + FCITX_ARRAY_SIZE(keyNameListCompat) &&
        compat->name == keyString) {
        return compat->sym;
    }

    if (fcitx::utf8::length(keyString) == 1) {
        auto chr = fcitx::utf8::getCharValidated(keyString);
        if (chr > 0) {
            if (fcitx::utf8::charLength(keyString) == 1) {
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
        [](const KeyNameOffsetByValue &item,
           KeySym key) { return item.sym < key; });
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

    auto lastPos = keyString.find_first_not_of(FCITX_WHITE_SPACE, 0);
    auto pos = keyString.find_first_of(FCITX_WHITE_SPACE, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        Key key(keyString.substr(lastPos, pos - lastPos));

        if (key.sym() != FcitxKey_None) {
            keyList.push_back(key);
        }
        lastPos = keyString.find_first_not_of(FCITX_WHITE_SPACE, pos);
        pos = keyString.find_first_of(FCITX_WHITE_SPACE, lastPos);
    }

    return keyList;
}
}
