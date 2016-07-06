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
#include <vector>
#include <memory>

namespace fcitx {

// Handler Tables are a kind of helper class that helps manage callbacks
// HandlerTableEntry can be deleted
template <typename T>
class HandlerTableEntry {

public:
    HandlerTableEntry(T handler) : m_handler(std::make_shared<T>(handler)) {}
    virtual ~HandlerTableEntry() { *m_handler = T(); }

    std::shared_ptr<T> handler() { return m_handler; };

protected:
    std::shared_ptr<T> m_handler;
};

template <typename T>
class ListHandlerTableEntry : public HandlerTableEntry<T> {
    IntrusiveListNode m_node;
    friend struct IntrusiveListMemberNodeGetter<ListHandlerTableEntry<T>, &ListHandlerTableEntry<T>::m_node>;

public:
    typedef struct IntrusiveListMemberNodeGetter<ListHandlerTableEntry, &ListHandlerTableEntry::m_node>
        node_getter_type;
    ListHandlerTableEntry(T handler) : HandlerTableEntry<T>(handler) {}
    virtual ~ListHandlerTableEntry() { m_node.remove(); }
};

template <typename Key, typename T>
class MultiHandlerTable;

template <typename Key, typename T>
class MultiHandlerTableEntry : public HandlerTableEntry<T> {
    typedef MultiHandlerTable<Key, T> table_type;
    table_type *m_table;
    Key m_key;
    IntrusiveListNode m_node;

    friend struct IntrusiveListMemberNodeGetter<MultiHandlerTableEntry, &MultiHandlerTableEntry::m_node>;

public:
    typedef struct IntrusiveListMemberNodeGetter<MultiHandlerTableEntry, &MultiHandlerTableEntry::m_node>
        node_getter_type;
    MultiHandlerTableEntry(table_type *table, Key key, T handler)
        : HandlerTableEntry<T>(handler), m_table(table), m_key(key) {}
    ~MultiHandlerTableEntry();

private:
};

template <typename Key, typename T>
MultiHandlerTableEntry<Key, T>::~MultiHandlerTableEntry() {
    if (m_node.isInList()) {
        m_node.remove();
        m_table->postRemove(m_key);
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
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type &reference;
        typedef value_type *pointer;

        iterator(typename super::const_iterator iter, typename super::const_iterator end)
            : m_parentIter(iter), m_endIter(end) {
             while (m_parentIter != m_endIter && !*m_parentIter) {
                m_parentIter++;
            }
        }

        iterator(const iterator &other) = default;

        iterator &operator=(const iterator &other) = default;

        bool operator==(const iterator &other) const noexcept { return m_parentIter == other.m_parentIter; }
        bool operator!=(const iterator &other) const noexcept { return !operator==(other); }

        iterator &operator++() {
            do {
                m_parentIter++;
            } while (m_parentIter != m_endIter && !(**m_parentIter));
            return *this;
        }

        iterator operator++(int) {
            auto old = m_parentIter;
            ++(*this);
            return {old, m_endIter};
        }

        reference operator*() { return **m_parentIter; }

        pointer operator->() { return (*m_parentIter).get(); }

    private:
        typename super::const_iterator m_parentIter;
        typename super::const_iterator m_endIter;
    };

    iterator begin() const { return iterator(super::cbegin(), super::cend()); }
    iterator end() const { return iterator(super::cend(), super::cend()); }
};
}

#endif // _FCITX_UTILS_HANDLERTABLE_DETAILS_H_
