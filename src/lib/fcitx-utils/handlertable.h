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
#ifndef _FCITX_UTILS_HANDLERTABLE_H_
#define _FCITX_UTILS_HANDLERTABLE_H_

#include "intrusivelist.h"

namespace fcitx
{

template<typename T>
class HandlerTableEntry : public IntrusiveListNode
{
public:
    HandlerTableEntry(T handler) : m_handler(handler) {
    }
    virtual ~HandlerTableEntry() {
        remove();
    }

    T& handler() { return m_handler; };

private:
    T m_handler;
};

template<typename T>
class HandlerTable : protected IntrusiveList<HandlerTableEntry<T>>
{
    typedef IntrusiveList<HandlerTableEntry<T>> super;
public:
    template<typename M>
    HandlerTableEntry<T> *add(M&& t) {
        auto result = new HandlerTableEntry<T>(std::forward<M>(t));
        this->push_back(*result);
        return result;
    }

    using super::begin;
    using super::end;
};



}

#endif // _FCITX_UTILS_HANDLERTABLE_H_
