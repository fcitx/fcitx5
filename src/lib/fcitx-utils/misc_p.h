//
// Copyright (C) 2016~2020 by CSSlayer
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
#ifndef _FCITX_UTILS_MISC_P_H_
#define _FCITX_UTILS_MISC_P_H_

#include <algorithm>
#include <list>
#include <string>
#include <type_traits>
#include <unordered_map>

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

    bool insert(const T &v) {
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
        auto iter =
            std::find_if(order_.begin(), order_.end(),
                         [before](const auto &t) { return (t == before); });
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

} // namespace fcitx

#endif // _FCITX_UTILS_MISC_P_H_
