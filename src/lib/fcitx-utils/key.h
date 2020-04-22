//
// Copyright (C) 2015~2015 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#ifndef _FCITX_UTILS_KEY_H_
#define _FCITX_UTILS_KEY_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Class to represent a key.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/keysym.h>
#include "fcitxutils_export.h"

namespace fcitx {
class Key;
typedef FcitxKeySym KeySym;
typedef Flags<KeyState> KeyStates;
typedef std::vector<Key> KeyList;

/// Control the behavior of toString function.
enum class KeyStringFormat {
    /// Can be used to parse from a string.
    Portable,
    /// Return the human readable string in localized format.
    Localized,
};

/// Describe a Key in fcitx.
class FCITXUTILS_EXPORT Key {
public:
    explicit Key(KeySym sym = FcitxKey_None, KeyStates states = KeyStates(),
                 int code = 0)
        : sym_(sym), states_(states), code_(code) {}

    /// Parse a key from string. If string is invalid, it will be set to
    /// FcitxKey_None
    explicit Key(const char *keyString);

    /// Parse a key from std::string.
    /// \see fcitx::Key::Key(const char *)
    explicit Key(const std::string &keyString) : Key(keyString.c_str()) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(Key)

    /// Create a key code based key with empty key symbol.
    static Key fromKeyCode(int code = 0, KeyStates states = KeyStates()) {
        return Key(FcitxKey_None, states, code);
    }

    /// Check if key is exactly same.
    bool operator==(const Key &key) const {
        return sym_ == key.sym_ && states_ == key.states_ && code_ == key.code_;
    }

    /// Check if current key match the key.
    bool check(const Key &key) const;

    /// Check if current key match the sym and states.
    /// \see fcitx::Key::check(const Key &key)
    bool check(KeySym sym = FcitxKey_None,
               KeyStates states = KeyStates()) const {
        return check(Key(sym, states));
    }

    /// Check if key is digit key.
    bool isDigit() const;

    /// Check if key is upper case.
    bool isUAZ() const;

    /// Check if key is lower case.
    bool isLAZ() const;

    /// Check if key is in the range of ascii and has no states.
    bool isSimple() const;

    /// Check if the key is a modifier press.
    bool isModifier() const;

    /// Check if this key will cause cursor to move, e.g. arrow key and page up/
    /// down.
    bool isCursorMove() const;

    /// Check if states has modifier.
    bool hasModifier() const;

    /// \brief Normalize a key, usually used when key is from frontend.
    ///
    /// states will be filtered to have only ctrl alt shift and super.
    /// Shift will be removed if it is key symbol is a-z/A-Z.
    /// Shift + any other modifier and a-z will be reset to A-Z. So
    /// key in configuration does not need to bother the case.
    Key normalize() const;

    /// \brief Convert key to a string.
    ///
    /// \arg format will control the format of return value.
    std::string
    toString(KeyStringFormat format = KeyStringFormat::Portable) const;

    /// Check if the sym is not FcitxKey_None or FcitxKey_VoidSymbol.
    bool isValid() const;

    inline KeySym sym() const { return sym_; }
    inline KeyStates states() const { return states_; }
    inline int code() const { return code_; }

    /// Convert the modifier symbol to its corresponding states.
    static KeyStates keySymToStates(KeySym sym);

    /// Convert a key symbol string to KeySym.
    static KeySym keySymFromString(const std::string &keyString);

    /// \brief Convert keysym to a string.
    ///
    /// \arg format will control the format of return value.
    static std::string
    keySymToString(KeySym sym,
                   KeyStringFormat format = KeyStringFormat::Portable);

    /// Convert unicode to key symbol. Useful when you want to create a
    /// synthetic key event.
    static KeySym keySymFromUnicode(uint32_t unicode);

    /// Convert keysym to a unicode. Will return a valid value UCS-4 value if
    /// this key may produce a character.
    static uint32_t keySymToUnicode(KeySym sym);

    /// Convert keysym to a unicode string. Will return a non empty value UTF-8
    /// string if this key may produce a character.
    /// \see fcitx::Key::keySymToUnicode
    static std::string keySymToUTF8(KeySym sym);

    /// Parse a list of key string into a KeyList.
    static KeyList keyListFromString(const std::string &str);

    /// Convert a key list to string.
    template <typename Container>
    static std::string
    keyListToString(Container container,
                    KeyStringFormat format = KeyStringFormat::Portable) {
        std::string result;
        bool first = true;
        for (auto k : container) {
            if (first) {
                first = false;
            } else {
                result += " ";
            }
            result += k.toString(format);
        }
        return result;
    }

    /// Check the current key against a key list.
    /// \see fcitx::Key::check
    template <typename Container>
    bool checkKeyList(const Container &c) {
        return std::find_if(c.begin(), c.end(), [this](const Key &toCheck) {
                   return check(toCheck);
               }) != c.end();
    }

    /// Check the current key against a key list and get the matched key index.
    /// \return Returns the matched key index or -1 if there is no match.
    /// \see fcitx::Key::check
    template <typename Container>
    int keyListIndex(const Container &c) {
        size_t idx = 0;
        for (auto &toCheck : c) {
            if (check(toCheck)) {
                break;
            }
            idx++;
        }
        if (idx == c.size()) {
            return -1;
        }
        return idx;
    }

private:
    KeySym sym_;
    KeyStates states_;
    int code_;
};
} // namespace fcitx

#endif //  _FCITX_UTILS_KEY_H_
