/*
 * Copyright (C) 2016~2016 by CSSlayer
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
class HandlerTable {
public:
    HandlerTable() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(HandlerTable)

    template <typename... Args>
    std::unique_ptr<HandlerTableEntry<T>> add(Args &&... args) {
        auto result = std::make_unique<ListHandlerTableEntry<T>>(
            std::forward<Args>(args)...);
        handlers_.push_back(*result);
        return result;
    }

    HandlerTableView<T> view() { return {handlers_.begin(), handlers_.end()}; }

    size_t size() const { return handlers_.size(); }

private:
    IntrusiveListFor<ListHandlerTableEntry<T>> handlers_;
};

template <typename Key, typename T>
class MultiHandlerTable {
    friend class MultiHandlerTableEntry<Key, T>;
    typedef std::unordered_map<Key,
                               IntrusiveListFor<MultiHandlerTableEntry<Key, T>>>
        map_type;

public:
    MultiHandlerTable(std::function<void(const Key &)> addKey = {},
                      std::function<void(const Key &)> removeKey = {})
        : addKey_(addKey), removeKey_(removeKey) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(MultiHandlerTable)

    template <typename M>
    std::unique_ptr<HandlerTableEntry<T>> add(const Key &key, M &&t) {
        auto iter = keyToHandlers_.find(key);
        if (iter == keyToHandlers_.end()) {
            if (addKey_) {
                addKey_(key);
            }
            iter = keyToHandlers_
                       .emplace(std::piecewise_construct,
                                std::forward_as_tuple(key),
                                std::forward_as_tuple())
                       .first;
        }
        auto result = std::make_unique<MultiHandlerTableEntry<Key, T>>(
            this, key, std::forward<M>(t));
        iter->second.push_back(*result);
        return result;
    }

    HandlerTableView<T> view(const Key &key) {
        auto iter = keyToHandlers_.find(key);
        if (iter == keyToHandlers_.end()) {
            return {};
        }
        return {iter->second.begin(), iter->second.end()};
    }

    IterRange<KeyIterator<typename map_type::const_iterator>> keys() const {
        return {KeyIterator<typename map_type::const_iterator>(
                    keyToHandlers_.begin()),
                KeyIterator<typename map_type::const_iterator>(
                    keyToHandlers_.end())};
    }

private:
    void postRemove(const Key &k) {
        auto iter = keyToHandlers_.find(k);
        if (iter != keyToHandlers_.end()) {
            if (removeKey_) {
                removeKey_(k);
            }
            keyToHandlers_.erase(iter);
        }
    }
    std::unordered_map<Key, IntrusiveListFor<MultiHandlerTableEntry<Key, T>>>
        keyToHandlers_;
    std::function<void(const Key &)> addKey_;
    std::function<void(const Key &)> removeKey_;
};
}

#endif // _FCITX_UTILS_HANDLERTABLE_H_
