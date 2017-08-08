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
#ifndef _FCITX_UTILS_FLAG_H_
#define _FCITX_UTILS_FLAG_H_

#include "fcitx-utils/macros.h"
#include <initializer_list>
#include <type_traits>

namespace fcitx {
template <typename Enum>
class Flags {
public:
    typedef typename std::underlying_type_t<Enum> storage_type;
    constexpr Flags(Enum f) : flags_(static_cast<storage_type>(f)) {}
    explicit Flags(storage_type i = 0) : flags_(i) {}
    constexpr Flags(const std::initializer_list<Enum> &l) : flags_(0) {
        for (Enum e : l) {
            operator|=(e);
        }
    }

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(Flags)

    constexpr inline operator storage_type() const { return flags_; }
    constexpr inline storage_type toInteger() const { return flags_; }

    Flags &operator=(Enum f) {
        flags_ = static_cast<storage_type>(f);
        return *this;
    }
    Flags &operator=(storage_type f) {
        flags_ = f;
        return *this;
    }

    constexpr bool operator!() const { return !flags_; }
    Flags &operator&=(Flags flag) {
        flags_ &= flag.flags_;
        return *this;
    }
    Flags &operator&=(Enum flag) {
        flags_ &= static_cast<storage_type>(flag);
        return *this;
    }
    Flags &operator|=(Flags flag) {
        flags_ |= flag.flags_;
        return *this;
    }
    Flags &operator|=(Enum flag) {
        flags_ |= static_cast<storage_type>(flag);
        return *this;
    }
    Flags &operator^=(Flags flag) {
        flags_ ^= flag.flags_;
        return *this;
    }
    Flags &operator^=(Enum flag) {
        flags_ ^= static_cast<storage_type>(flag);
        return *this;
    }
    constexpr inline Flags operator|(Flags f) const {
        return Flags(flags_ | f.flags_);
    }
    constexpr inline Flags operator|(Enum f) const {
        return Flags(flags_ | static_cast<storage_type>(f));
    }
    constexpr inline Flags operator^(Flags f) const {
        return Flags(flags_ ^ f.flags_);
    }
    constexpr inline Flags operator^(Enum f) const {
        return Flags(flags_ ^ static_cast<storage_type>(f));
    }
    constexpr inline Flags operator&(Flags f) const {
        return Flags(flags_ & f.flags_);
    }
    constexpr inline Flags operator&(Enum f) const {
        return Flags(flags_ & static_cast<storage_type>(f));
    }
    constexpr inline Flags operator~() const { return Flags(~flags_); }

    template <typename T>
    constexpr inline bool test(T f) const {
        return (*this & f) == f;
    }
    template <typename T>
    constexpr inline bool testAny(T f) const {
        return (*this & f) != 0;
    }

    constexpr bool operator==(const Flags &f) const {
        return flags_ == f.flags_;
    }
    constexpr bool operator==(Enum f) const {
        return flags_ == static_cast<storage_type>(f);
    }
    constexpr bool operator!=(const Flags &f) const { return !operator==(f); }
    constexpr bool operator!=(Enum f) const { return !operator==(f); }

private:
    storage_type flags_;
};
}

#endif // _FCITX_UTILS_FLAG_H_
