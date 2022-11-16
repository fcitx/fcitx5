/*
 * SPDX-FileCopyrightText: 2016-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_MISC_P_H_
#define _FCITX_UTILS_MISC_P_H_

#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <config.h>
#include "endian_p.h"

namespace fcitx {

template <typename M, typename K>
decltype(&std::declval<M>().begin()->second) findValue(M &&m, K &&key) {
    auto iter = m.find(key);
    if (iter != m.end()) {
        return &iter->second;
    }
    return nullptr;
}

template <typename T>
class OrderedSet {
    typedef std::list<T> OrderList;

public:
    auto begin() { return order_.begin(); }

    auto end() { return order_.end(); }
    auto begin() const { return order_.begin(); }

    auto end() const { return order_.end(); }

    bool empty() const { return order_.empty(); }

    const T &front() const { return order_.front(); }

    void clear() {
        dict_.clear();
        order_.clear();
    }

    bool pushFront(const T &v) {
        if (dict_.count(v)) {
            return false;
        }
        order_.emplace_front(v);
        dict_.insert(std::make_pair(v, order_.begin()));
        return true;
    }

    bool pushBack(const T &v) {
        if (dict_.count(v)) {
            return false;
        }
        order_.emplace_back(v);
        dict_.insert(std::make_pair(v, std::prev(order_.end())));
        return true;
    }

    void moveToTop(const T &v) {
        auto iter = dict_.find(v);
        if (iter == dict_.end()) {
            return;
        }
        if (iter->second != order_.begin()) {
            order_.splice(order_.begin(), order_, iter->second);
        }
    }

    auto size() const { return order_.size(); }

    void pop() {
        dict_.erase(order_.back());
        order_.pop_back();
    }

    bool insert(const T &before, const T &v) {
        if (dict_.count(v)) {
            return false;
        }
        typename OrderList::iterator iter;
        if (auto dictIter = dict_.find(before); dictIter != dict_.end()) {
            iter = dictIter->second;
        } else {
            iter = order_.end();
        }
        auto newIter = order_.insert(iter, v);
        dict_.insert(std::make_pair(v, newIter));
        return true;
    }

    bool contains(const T &v) const { return !!dict_.count(v); }

    bool remove(const T &v) {
        auto iter = dict_.find(v);
        if (iter == dict_.end()) {
            return false;
        }
        order_.erase(iter->second);
        dict_.erase(iter);
        return true;
    }

    const OrderList &order() const { return order_; }

private:
    std::unordered_map<T, typename OrderList::iterator> dict_;
    OrderList order_;
};

template <typename K, typename V, class Hash = std::hash<K>,
          class Pred = std::equal_to<K>>
class OrderedMap {
private:
    std::list<std::pair<const K, V>> order_;
    std::unordered_map<K, typename decltype(order_)::iterator, Hash, Pred> map_;

public:
    typedef decltype(map_) map_type;
    typedef decltype(order_) list_type;
    typedef typename map_type::key_type key_type;
    typedef typename list_type::value_type value_type;
    typedef typename value_type::second_type mapped_type;

    typedef typename list_type::pointer pointer;
    typedef typename list_type::const_pointer const_pointer;
    typedef typename list_type::reference reference;
    typedef typename list_type::const_reference const_reference;
    typedef typename list_type::iterator iterator;
    typedef typename list_type::const_iterator const_iterator;
    typedef typename list_type::size_type size_type;
    typedef typename list_type::difference_type difference_type;

    OrderedMap() = default;
    OrderedMap(const OrderedMap &other) : order_(other.order_) { fillMap(); }

    /// Move constructor.
    OrderedMap(OrderedMap &&other) noexcept { operator=(std::move(other)); }

    OrderedMap(std::initializer_list<value_type> l) : order_(l) { fillMap(); }

    template <typename InputIterator>
    OrderedMap(InputIterator first, InputIterator last) : order_(first, last) {
        fillMap();
    }

    /// Copy assignment operator.
    OrderedMap &operator=(const OrderedMap &other) {
        order_ = list_type(other.order_.begin(), other.order_.end());
        fillMap();
        return *this;
    }

    /// Move assignment operator.
    OrderedMap &operator=(OrderedMap &&other) noexcept {
        using std::swap;
        swap(order_, other.order_);
        swap(map_, other.map_);
        other.order_.clear();
        other.map_.clear();
        return *this;
    }

    bool empty() const noexcept { return order_.empty(); }

    size_type size() const noexcept { return order_.size(); }

    iterator begin() noexcept { return order_.begin(); }

    const_iterator begin() const noexcept { return order_.begin(); }

    const_iterator cbegin() const noexcept { return order_.begin(); }

    iterator end() noexcept { return order_.end(); }

    const_iterator end() const noexcept { return order_.end(); }

    const_iterator cend() const noexcept { return order_.end(); }

    template <typename... _Args>
    std::pair<iterator, bool> emplace(_Args &&...__args) {
        order_.emplace_back(std::forward<_Args>(__args)...);
        auto iter = std::prev(order_.end());
        auto mapResult = map_.emplace(iter->first, iter);
        if (mapResult.second) {
            return {iter, true};
        }
        order_.erase(iter);
        iter = mapResult.first->second;
        return {iter, false};
    }

    std::pair<iterator, bool> insert(const value_type &v) { return emplace(v); }

    iterator erase(const_iterator position) {
        map_.erase(position->first);
        return order_.erase(position);
    }

    iterator erase(iterator position) {
        map_.erase(position->first);
        return order_.erase(position);
    }

    size_type erase(const key_type &k) {
        auto iter = map_.find(k);
        if (iter != map_.end()) {
            order_.erase(iter->second);
            map_.erase(iter);
            return 1;
        }
        return 0;
    }

    void clear() noexcept {
        order_.clear();
        map_.clear();
    }

    iterator find(const key_type &k) {
        auto iter = map_.find(k);
        if (iter != map_.end()) {
            return iter->second;
        }
        return order_.end();
    }

    const_iterator find(const key_type &k) const {
        auto iter = map_.find(k);
        if (iter != map_.end()) {
            return iter->second;
        }
        return order_.end();
    }

    size_type count(const key_type &k) const { return map_.count(k); }

    mapped_type &operator[](const key_type &k) {
        auto iter = find(k);
        if (iter != end()) {
            return iter->second;
        }
        auto result = emplace(k, mapped_type());
        return result.first->second;
    }

    mapped_type &operator[](key_type &&k) {
        auto iter = find(k);
        if (iter != end()) {
            return iter->second;
        }
        auto result = emplace(std::move(k), value_type());
        return result.first->second;
    }

private:
    void fillMap() {
        for (auto iter = order_.begin(), end = order_.end(); iter != end;
             iter++) {
            map_[iter->first] = iter;
        }
    }
};

static inline int safePipe(int pipefd[2]) {
#ifdef HAVE_PIPE2
    return ::pipe2(pipefd, O_NONBLOCK | O_CLOEXEC);
#else
    int ret = ::pipe(pipefd);
    if (ret == -1)
        return -1;
    ::fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipefd[0], F_SETFL, ::fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
    ::fcntl(pipefd[1], F_SETFL, ::fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
    return 0;
#endif
}

static inline bool checkBoolEnvVar(const char *name) {
    const char *var = getenv(name);
    bool value = false;
    if (var && var[0] &&
        (strcmp(var, "True") == 0 || strcmp(var, "true") == 0 ||
         strcmp(var, "1") == 0)) {
        value = true;
    }
    return value;
}

template <typename T>
static inline uint32_t FromLittleEndian32(const T *d) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(d);
    uint32_t t;
    memcpy(&t, data, sizeof(t));
    return le32toh(t);
}

} // namespace fcitx

#endif // _FCITX_UTILS_MISC_P_H_
