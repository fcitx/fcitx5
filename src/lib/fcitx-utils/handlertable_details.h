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
#ifndef _FCITX_UTILS_HANDLERTABLE_DETAILS_H_
#define _FCITX_UTILS_HANDLERTABLE_DETAILS_H_

#include <fcitx-utils/intrusivelist.h>
#include <list>
#include <memory>
#include <vector>

namespace fcitx {

// Handler Tables are a kind of helper class that helps manage callbacks
// HandlerTableEntry can be deleted
template <typename T>
class HandlerTableEntry {

public:
    HandlerTableEntry(T handler) : handler_(std::make_shared<T>(handler)) {}
    virtual ~HandlerTableEntry() { *handler_ = T(); }

    std::shared_ptr<T> handler() { return handler_; };

protected:
    std::shared_ptr<T> handler_;
};

template <typename T>
class ListHandlerTableEntry : public HandlerTableEntry<T> {
    IntrusiveListNode node_;
    friend struct IntrusiveListMemberNodeGetter<
        ListHandlerTableEntry<T>, &ListHandlerTableEntry<T>::node_>;

public:
    typedef struct IntrusiveListMemberNodeGetter<ListHandlerTableEntry,
                                                 &ListHandlerTableEntry::node_>
        node_getter_type;
    ListHandlerTableEntry(T handler) : HandlerTableEntry<T>(handler) {}
    virtual ~ListHandlerTableEntry() { node_.remove(); }
};

template <typename Key, typename T>
class MultiHandlerTable;

template <typename Key, typename T>
class MultiHandlerTableEntry : public HandlerTableEntry<T> {
    typedef MultiHandlerTable<Key, T> table_type;

private:
    table_type *table_;
    Key key_;
    IntrusiveListNode node_;
    friend struct IntrusiveListMemberNodeGetter<MultiHandlerTableEntry,
                                                &MultiHandlerTableEntry::node_>;

public:
    typedef struct IntrusiveListMemberNodeGetter<MultiHandlerTableEntry,
                                                 &MultiHandlerTableEntry::node_>
        node_getter_type;
    MultiHandlerTableEntry(table_type *table, Key key, T handler)
        : HandlerTableEntry<T>(handler), table_(table), key_(key) {}
    ~MultiHandlerTableEntry();
};

template <typename Key, typename T>
MultiHandlerTableEntry<Key, T>::~MultiHandlerTableEntry() {
    if (node_.isInList()) {
        node_.remove();
        table_->postRemove(key_);
    }
}

template <typename T>
class HandlerTableView : public std::vector<std::shared_ptr<T>> {
public:
    typedef std::vector<std::shared_ptr<T>> super;
    HandlerTableView() : super() {}

    template <typename _Iter>
    HandlerTableView(_Iter begin, _Iter end) {
        for (; begin != end; begin++) {
            this->emplace_back(begin->handler());
        }
    }

    class iterator {
    public:
        typedef std::input_iterator_tag iterator_category;
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type &reference;
        typedef value_type *pointer;

        iterator(typename super::const_iterator iter,
                 typename super::const_iterator end)
            : parentIter_(iter), endIter_(end) {
            while (parentIter_ != endIter_ && !*parentIter_) {
                parentIter_++;
            }
        }

        iterator(const iterator &other) = default;

        iterator &operator=(const iterator &other) = default;

        bool operator==(const iterator &other) const noexcept {
            return parentIter_ == other.parentIter_;
        }
        bool operator!=(const iterator &other) const noexcept {
            return !operator==(other);
        }

        iterator &operator++() {
            do {
                parentIter_++;
            } while (parentIter_ != endIter_ && !(**parentIter_));
            return *this;
        }

        iterator operator++(int) {
            auto old = parentIter_;
            ++(*this);
            return {old, endIter_};
        }

        reference operator*() { return **parentIter_; }

        pointer operator->() { return (*parentIter_).get(); }

    private:
        typename super::const_iterator parentIter_;
        typename super::const_iterator endIter_;
    };

    iterator begin() const { return iterator(super::cbegin(), super::cend()); }
    iterator end() const { return iterator(super::cend(), super::cend()); }
};
}

#endif // _FCITX_UTILS_HANDLERTABLE_DETAILS_H_
