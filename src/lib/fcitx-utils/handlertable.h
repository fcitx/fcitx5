/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_HANDLERTABLE_H_
#define _FCITX_UTILS_HANDLERTABLE_H_

#include <functional>
#include <unordered_map>
#include <fcitx-utils/handlertable_details.h>
#include <fcitx-utils/intrusivelist.h>

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
    FCITX_NODISCARD std::unique_ptr<HandlerTableEntry<T>> add(Args &&... args) {
        auto result = std::make_unique<ListHandlerTableEntry<T>>(
            std::forward<Args>(args)...);
        handlers_.push_back(*result);
        return result;
    }

    HandlerTableView<T> view() { return {handlers_.begin(), handlers_.end()}; }

    size_t size() const { return handlers_.size(); }
    bool empty() const { return size() == 0; }

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
    MultiHandlerTable(std::function<bool(const Key &)> addKey = {},
                      std::function<void(const Key &)> removeKey = {})
        : addKey_(addKey), removeKey_(removeKey) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(MultiHandlerTable)

    template <typename M>
    FCITX_NODISCARD std::unique_ptr<HandlerTableEntry<T>> add(const Key &key,
                                                              M &&t) {
        auto iter = keyToHandlers_.find(key);
        if (iter == keyToHandlers_.end()) {
            if (addKey_) {
                if (!addKey_(key)) {
                    return nullptr;
                }
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

    bool hasKey(const Key &key) const {
        auto iter = keyToHandlers_.find(key);
        return iter != keyToHandlers_.end();
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
        if (iter != keyToHandlers_.end() && iter->second.empty()) {
            if (removeKey_) {
                removeKey_(k);
            }
            keyToHandlers_.erase(iter);
        }
    }
    std::unordered_map<Key, IntrusiveListFor<MultiHandlerTableEntry<Key, T>>>
        keyToHandlers_;
    std::function<bool(const Key &)> addKey_;
    std::function<void(const Key &)> removeKey_;
};
} // namespace fcitx

#endif // _FCITX_UTILS_HANDLERTABLE_H_
