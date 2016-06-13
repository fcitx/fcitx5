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

#ifndef _FCITX_UTILS_KEY_H_
#define _FCITX_UTILS_KEY_H_

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <algorithm>
#include "fcitxutils_export.h"
#include "keysym.h"
#include "flags.h"

namespace fcitx {
class Key;
typedef FcitxKeySym KeySym;
typedef Flags<KeyState> KeyStates;
typedef std::vector<Key> KeyList;

class FCITXUTILS_EXPORT Key {
public:
    explicit Key(KeySym sym = FcitxKey_None, KeyStates states = KeyStates()) : m_sym(sym), m_states(states) {}
    Key(const Key &other) : Key(other.m_sym, other.m_states) {}
    explicit Key(const char *keyString);
    explicit Key(const std::string &keyString) : Key(keyString.c_str()) {}
    virtual ~Key();

    bool operator==(const Key &key) const { return m_sym == key.m_sym && m_states == key.m_states; }

    bool check(const Key &key) const;
    bool check(KeySym sym = FcitxKey_None, KeyStates states = KeyStates()) const { return check(Key(sym, states)); }
    bool isDigit() const;
    bool isUAZ() const;
    bool isLAZ() const;
    bool isSimple() const;
    bool isModifier() const;
    bool isCursorMove() const;
    bool hasModifier() const;
    Key normalize() const;

    std::string toString() const;

    inline KeySym sym() const { return m_sym; }
    inline KeyStates states() const { return m_states; }

    static KeyStates keySymToStates(KeySym sym);
    static KeySym keySymFromString(const std::string &keyString);
    static std::string keySymToString(KeySym sym);
    static KeySym keySymFromUnicode(uint32_t unicode);
    static uint32_t keySymToUnicode(KeySym sym);
    static KeyList keyListFromString(const std::string &str);
    template <typename Container>
    static std::string keyListToString(Container c) {
        std::stringstream ss;
        bool first = true;
        for (auto k : c) {
            if (first) {
                first = false;
            } else {
                ss << " ";
            }
            ss << k.toString();
        }
        return ss.str();
    }
    template <typename Container>
    static bool keyListCheck(const Container &c, const Key &key) {
        return std::find_if(c.begin(), c.end(), [&key](const Key &toCheck) { return toCheck.check(key); }) != c.end();
    }

protected:
    KeySym m_sym;
    KeyStates m_states;
};
}

#endif //  _FCITX_UTILS_KEY_H_
