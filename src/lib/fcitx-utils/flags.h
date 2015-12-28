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

#include <type_traits>

namespace fcitx {
template <typename Enum>
class Flags {
public:
    typedef std::underlying_type_t<Enum> storage_type;
    Flags(Enum f) : m_flags(static_cast<storage_type>(f)) {}
    Flags(const Flags &other) : m_flags(other.m_flags) {}
    explicit Flags(storage_type i = 0) : m_flags(i) {}
    Flags(const std::initializer_list<Enum> &l) : m_flags(0) {
        for (Enum e : l) {
            operator|=(e);
        }
    }
    inline operator storage_type() const { return m_flags; }
    Flags &operator=(Enum f) {
        m_flags = static_cast<storage_type>(f);
        return *this;
    }
    Flags &operator=(storage_type f) {
        m_flags = f;
        return *this;
    }

    bool operator!() const { return !m_flags; }
    Flags &operator&=(Flags flag) {
        m_flags &= flag.m_flags;
        return *this;
    }
    Flags &operator&=(Enum flag) {
        m_flags &= static_cast<storage_type>(flag);
        return *this;
    }
    Flags &operator|=(Flags flag) {
        m_flags |= flag.m_flags;
        return *this;
    }
    Flags &operator|=(Enum flag) {
        m_flags |= static_cast<storage_type>(flag);
        return *this;
    }
    Flags &operator^=(Flags flag) {
        m_flags ^= flag.m_flags;
        return *this;
    }
    Flags &operator^=(Enum flag) {
        m_flags ^= static_cast<storage_type>(flag);
        return *this;
    }
    inline Flags operator|(Flags f) const { return Flags(m_flags | f.m_flags); }
    inline Flags operator|(Enum f) const {
        return Flags(m_flags | static_cast<storage_type>(f));
    }
    inline Flags operator^(Flags f) const { return Flags(m_flags ^ f.m_flags); }
    inline Flags operator^(Enum f) const {
        return Flags(m_flags ^ static_cast<storage_type>(f));
    }
    inline Flags operator&(Flags f) const { return Flags(m_flags & f.m_flags); }
    inline Flags operator&(Enum f) const {
        return Flags(m_flags & static_cast<storage_type>(f));
    }
    inline Flags operator~() const { return Flags(~m_flags); }

    bool operator==(const Flags &f) const { return m_flags == f.m_flags; }
    bool operator==(Enum f) const {
        return m_flags == static_cast<storage_type>(f);
    }
    bool operator!=(const Flags &f) const { return !operator==(f); }
    bool operator!=(Enum f) const { return !operator==(f); }

private:
    storage_type m_flags;
};
}

#endif // _FCITX_UTILS_FLAG_H_
