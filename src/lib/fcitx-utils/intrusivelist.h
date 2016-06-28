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
#ifndef _FCITX_UTILS_INSTRUSIVELIST_H_
#define _FCITX_UTILS_INSTRUSIVELIST_H_

#include <array>
#include <iterator>
#include <type_traits>

namespace fcitx {

template <class Parent, class Member>
inline std::ptrdiff_t offset_from_pointer_to_member(const Member Parent::*ptr_to_member) {
    const Parent *const parent = 0;
    const char *const member = static_cast<const char *>(static_cast<const void *>(&(parent->*ptr_to_member)));
    return std::ptrdiff_t(member - static_cast<const char *>(static_cast<const void *>(parent)));
}

template <class Parent, class Member>
inline Parent *parent_from_member(Member *member, const Member Parent::*ptr_to_member) {
    return static_cast<Parent *>(static_cast<void *>(static_cast<char *>(static_cast<void *>(member)) -
                                                     offset_from_pointer_to_member(ptr_to_member)));
}

class IntrusiveListBase;

class IntrusiveListNode {
    friend class IntrusiveListBase;

public:
    IntrusiveListNode() {}
    IntrusiveListNode(const IntrusiveListNode &) = delete;

    bool isInList() const { return !!m_list; }
    void remove();
    IntrusiveListNode *prev() const { return m_prev; }
    IntrusiveListNode *next() const { return m_next; }

private:
    IntrusiveListBase *m_list = nullptr;
    IntrusiveListNode *m_prev = nullptr;
    IntrusiveListNode *m_next = nullptr;
};

class IntrusiveListBase {
    friend class IntrusiveListNode;

protected:
    IntrusiveListBase() { root.m_prev = root.m_next = &root; }

    void insertBetween(IntrusiveListNode *add, IntrusiveListNode *prev, IntrusiveListNode *next) noexcept {
        if (add->m_list) {
            throw std::invalid_argument("node can't be insert to two different list");
        }
        next->m_prev = add;
        prev->m_next = add;
        add->m_next = next;
        add->m_prev = prev;
        add->m_list = this;
        size_++;
    }

    void prepend(IntrusiveListNode *add, IntrusiveListNode *pos) noexcept {
        return insertBetween(add, pos, pos->m_next);
    }

    void append(IntrusiveListNode *add, IntrusiveListNode *pos) noexcept {
        return insertBetween(add, pos->m_prev, pos);
    }

    void remove(IntrusiveListNode *pos) noexcept {
        if (pos->m_list != this) {
            throw std::invalid_argument("node doesn't belongs to this list");
        }

        auto next_ = pos->m_next;
        auto prev_ = pos->m_prev;
        prev_->m_next = next_;
        next_->m_prev = prev_;

        pos->m_next = nullptr;
        pos->m_prev = nullptr;
        pos->m_list = nullptr;

        size_--;
    }

    IntrusiveListNode root;
    std::size_t size_ = 0;
};

inline void IntrusiveListNode::remove() {
    if (m_list) {
        m_list->remove(this);
    }
}

template <typename T>
struct IntrusiveListTrivialNodeGetter {
    static_assert(std::is_base_of<IntrusiveListNode, T>::value, "T must be a descendant of IntrusiveListNode");

    static IntrusiveListNode &toNode(T &value) noexcept { return *static_cast<IntrusiveListNode *>(&value); }

    static T &toValue(IntrusiveListNode &node) noexcept { return *static_cast<T *>(&node); }

    static const IntrusiveListNode &toNode(const T &value) noexcept {
        return *static_cast<const IntrusiveListNode *>(&value);
    }

    static const T &toValue(const IntrusiveListNode &node) noexcept { return *reinterpret_cast<const T *>(&node); }
};

template <typename T, typename NodeGetter>
class IntrusiveList;

template <typename T, typename NodeGetter, bool isConst>
class IntrusiveListIterator {
    typedef IntrusiveList<T, NodeGetter> list_type;
    typedef IntrusiveListNode *node_ptr;

public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename std::conditional<isConst, typename list_type::const_reference, typename list_type::reference>::type
        reference;
    typedef typename std::conditional<isConst, typename list_type::const_pointer, typename list_type::pointer>::type
        pointer;

    IntrusiveListIterator() : node(nullptr), nodeGetter(nullptr) {}
    IntrusiveListIterator(node_ptr node_, NodeGetter &nodeGetter_) : node(node_), nodeGetter(&nodeGetter_) {}

    IntrusiveListIterator(const typename list_type::iterator &other)
        : IntrusiveListIterator(other.pointed_node(), other.get_nodeGetter()) {}

    IntrusiveListIterator &operator=(const IntrusiveListIterator &other) {
        node = other.node;
        nodeGetter = other.nodeGetter;
        return *this;
    }

    bool operator==(const IntrusiveListIterator &other) const noexcept { return node == other.node; }
    bool operator!=(const IntrusiveListIterator &other) const noexcept { return !operator==(other); }
    IntrusiveListIterator &operator++() {
        node = node->next();
        return *this;
    }

    IntrusiveListIterator operator++(int) {
        auto old = node;
        ++(*this);
        return {old, *nodeGetter};
    }

    reference operator*() { return nodeGetter->toValue(*node); }

    pointer operator->() { return &nodeGetter->toValue(*node); }

    node_ptr pointed_node() const { return node; }

    NodeGetter &get_nodeGetter() const { return *nodeGetter; }

private:
    node_ptr node;
    NodeGetter *nodeGetter;
};

template <typename T, typename NodeGetter = IntrusiveListTrivialNodeGetter<T>>
class IntrusiveList : public IntrusiveListBase {
public:
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef IntrusiveListIterator<T, NodeGetter, false> iterator;
    typedef IntrusiveListIterator<T, NodeGetter, true> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::size_t size_type;

    IntrusiveList(NodeGetter nodeGetter_ = NodeGetter()) : nodeGetter(nodeGetter_) {}

    virtual ~IntrusiveList() {
        // remove everything from list, since we didn't own anything, then we are good.
        while (size()) {
            pop_back();
        }
    }

    iterator begin() { return {root.next(), nodeGetter}; }
    iterator end() { return {&root, nodeGetter}; }

    const_iterator begin() const { return {root.next(), nodeGetter}; }

    const_iterator end() const { return {&root, nodeGetter}; }

    const_iterator cbegin() const { return {root.next(), nodeGetter}; }

    const_iterator cend() const { return {&root, nodeGetter}; }

    reference front() { return *begin(); }

    const_reference front() const { return *cbegin(); }

    reference back() { return *iterator{root.prev(), nodeGetter}; }

    const_reference back() const {
        return *const_iterator{root.prev(), nodeGetter};
        ;
    }

    iterator iterator_to(reference value) { return iterator(&nodeGetter.toNode(value), nodeGetter); }

    const_iterator iterator_to(const_reference value) { return const_iterator(&nodeGetter.toNode(value), nodeGetter); }

    void push_back(reference value) {
        auto &node = nodeGetter.toNode(value);
        append(&node, &root);
    }

    void pop_back() { remove(root.prev()); }

    iterator erase(const_iterator pos) {
        auto node = pos.pointed_node();
        auto next = node->next();
        remove(node);
        return {next, nodeGetter};
    }

    iterator erase(const_iterator start, const_iterator end) {
        if (start == end) {
            return {start->pointed_node(), nodeGetter};
        }

        iterator iter;
        while ((iter = erase(start)) != end) {
        }
        return iter;
    }

    size_type size() const { return size_; }

    bool empty() const { return root.next() == &root; }

    iterator insert(const_iterator pos, reference value) {
        append(&nodeGetter.toNode(value), pos.pointed_node());
        return {pos.pointed_node()->prev(), nodeGetter};
    }

private:
    NodeGetter nodeGetter;
};
}

#endif // _FCITX_UTILS_INSTRUSIVELIST_H_
