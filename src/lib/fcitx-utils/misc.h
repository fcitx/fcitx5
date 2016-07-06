/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UTILS_MISC_H_
#define _FCITX_UTILS_MISC_H_

#include <cstdint>
#include <memory>

namespace fcitx {

template <class Parent, class Member>
inline std::ptrdiff_t offsetFromPointerToMember(const Member Parent::*ptr_to_member) {
    const Parent *const parent = 0;
    const char *const member = static_cast<const char *>(static_cast<const void *>(&(parent->*ptr_to_member)));
    return std::ptrdiff_t(member - static_cast<const char *>(static_cast<const void *>(parent)));
}

template <class Parent, class Member>
inline Parent *parentFromMember(Member *member, const Member Parent::*ptr_to_member) {
    return static_cast<Parent *>(static_cast<void *>(static_cast<char *>(static_cast<void *>(member)) -
                                                     offsetFromPointerToMember(ptr_to_member)));
}

template <class Parent, class Member>
inline const Parent *parentFromMember(const Member *member, const Member Parent::*ptr_to_member) {
    return static_cast<const Parent *>(static_cast<const void *>(
        static_cast<const char *>(static_cast<const void *>(member)) - offsetFromPointerToMember(ptr_to_member)));
}

template<typename T>
class EnableWeakRef;

template<typename T>
class WeakRef {
    friend class EnableWeakRef<T>;
public:
    bool isValid() const {
        return !m_that.expired();
    }

    T *get() const {
        return m_that.expired() ? nullptr : m_rawThat;
    }

private:
    WeakRef(std::weak_ptr<T*> that, T* rawThat) :
        m_that(std::move(that)), m_rawThat(rawThat) {
    }

    std::weak_ptr<T*> m_that;
    T *m_rawThat;
};


template<typename T>
class EnableWeakRef {
public:
    EnableWeakRef() : m_self(std::make_shared<T*>(static_cast<T*>(this))) { }
    EnableWeakRef(const EnableWeakRef &) = delete;

    WeakRef<T> watch() {
        return WeakRef<T>(m_self, static_cast<T*>(this));
    }

private:
    std::shared_ptr<T*> m_self;
};

}

#endif // _FCITX_UTILS_MISC_H_
