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

#include <exception>
#include <fcitx-utils/handlertable_details.h>
#include <fcitx-utils/intrusivelist.h>
#include <functional>
#include <unordered_map>

namespace fcitx {

template <typename T>
class HandlerTableEntry;

template <typename T>
class HandlerTableView;

template <typename T>
class HandlerTable : protected IntrusiveListFor<ListHandlerTableEntry<T>> {
    typedef IntrusiveListFor<ListHandlerTableEntry<T>> super;

public:
    template <typename M>
    HandlerTableEntry<T> *add(M &&t) {
        auto result = new ListHandlerTableEntry<T>(std::forward<M>(t));
        this->push_back(*result);
        return result;
    }

    HandlerTableView<T> view() { return {this->begin(), this->end()}; }
};

template <typename Key, typename T>
class MultiHandlerTable : protected std::unordered_map<Key, IntrusiveListFor<MultiHandlerTableEntry<Key, T>>> {
    friend class MultiHandlerTableEntry<Key, T>;
    typedef std::unordered_map<Key, IntrusiveListFor<MultiHandlerTableEntry<Key, T>>> super;

public:
    MultiHandlerTable(std::function<void(const Key &)> addKey, std::function<void(const Key &)> removeKey)
        : m_addKey(addKey), m_removeKey(removeKey) {}

    template <typename M>
    HandlerTableEntry<T> *add(const Key &key, M &&t) {
        auto iter = super::find(key);
        if (iter == super::end()) {
            m_addKey(key);
            iter = super::emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple()).first;
        }
        auto result = new MultiHandlerTableEntry<Key, T>(this, key, std::forward<M>(t));
        iter->second.push_back(*result);
        return result;
    }

    HandlerTableView<T> view(const Key &key) {
        auto iter = super::find(key);
        if (iter == super::end()) {
            return {};
        }
        return {iter->second.begin(), iter->second.end()};
    }

private:
    void postRemove(const Key &k) {
        auto iter = this->find(k);
        if (iter != this->end()) {
            m_removeKey(k);
            this->erase(iter);
        }
    }
    std::function<void(const Key &)> m_addKey;
    std::function<void(const Key &)> m_removeKey;
};
}

#endif // _FCITX_UTILS_HANDLERTABLE_H_
