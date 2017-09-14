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
#ifndef _FCITX_UTILS_INSTRUSIVELIST_H_
#define _FCITX_UTILS_INSTRUSIVELIST_H_

#include "misc.h"
#include <array>
#include <cassert>
#include <iterator>
#include <type_traits>

namespace fcitx {

class IntrusiveListBase;

class IntrusiveListNode {
    friend class IntrusiveListBase;

public:
    IntrusiveListNode() {}
    IntrusiveListNode(const IntrusiveListNode &) = delete;
    virtual ~IntrusiveListNode() { remove(); }

    bool isInList() const { return !!list_; }
    void remove();
    IntrusiveListNode *prev() const { return prev_; }
    IntrusiveListNode *next() const { return next_; }

private:
    IntrusiveListBase *list_ = nullptr;
    IntrusiveListNode *prev_ = nullptr;
    IntrusiveListNode *next_ = nullptr;
};

class IntrusiveListBase {
    friend class IntrusiveListNode;

protected:
    IntrusiveListBase() noexcept { root_.prev_ = root_.next_ = &root_; }
    IntrusiveListBase(IntrusiveListBase &&other) noexcept
        : IntrusiveListBase() {
        operator=(std::forward<IntrusiveListBase>(other));
    }

    virtual ~IntrusiveListBase() { removeAll(); }

    IntrusiveListBase &operator=(IntrusiveListBase &&other) noexcept {
        using std::swap;
        // no need to swap empty list.
        if (size_ == 0 && other.size_ == 0) {
            return *this;
        }

        // clear current one.
        removeAll();
        while (other.size_) {
            auto node = other.root_.prev_;
            // pop_back
            other.remove(other.root_.prev_);
            // push_front
            prepend(node, root_.next_);
        }

        return *this;
    }

    void insertBetween(IntrusiveListNode *add, IntrusiveListNode *prev,
                       IntrusiveListNode *next) noexcept {
        next->prev_ = add;
        prev->next_ = add;
        add->next_ = next;
        add->prev_ = prev;
        add->list_ = this;
        size_++;
    }

    void append(IntrusiveListNode *add, IntrusiveListNode *pos) noexcept {
        add->remove();
        return insertBetween(add, pos, pos->next_);
    }

    void prepend(IntrusiveListNode *add, IntrusiveListNode *pos) noexcept {
        add->remove();
        return insertBetween(add, pos->prev_, pos);
    }

    void remove(IntrusiveListNode *pos) noexcept {
        auto next_ = pos->next_;
        auto prev_ = pos->prev_;
        prev_->next_ = next_;
        next_->prev_ = prev_;

        pos->next_ = nullptr;
        pos->prev_ = nullptr;
        pos->list_ = nullptr;

        size_--;
    }

    void removeAll() {
        // remove everything from list, since we didn't own anything, then we
        // are good.
        while (size_) {
            remove(root_.prev_);
        }
    }

    IntrusiveListNode root_;
    std::size_t size_ = 0;
};

inline void IntrusiveListNode::remove() {
    if (list_) {
        list_->remove(this);
    }
}

template <typename T>
struct IntrusiveListTrivialNodeGetter {
    static_assert(std::is_base_of<IntrusiveListNode, T>::value,
                  "T must be a descendant of IntrusiveListNode");

    static IntrusiveListNode &toNode(T &value) noexcept {
        return *static_cast<IntrusiveListNode *>(&value);
    }

    static T &toValue(IntrusiveListNode &node) noexcept {
        return *static_cast<T *>(&node);
    }

    static const IntrusiveListNode &toNode(const T &value) noexcept {
        return *static_cast<const IntrusiveListNode *>(&value);
    }

    static const T &toValue(const IntrusiveListNode &node) noexcept {
        return *static_cast<const T *>(&node);
    }
};

template <typename T, IntrusiveListNode T::*ptrToNode>
struct IntrusiveListMemberNodeGetter {
    static IntrusiveListNode &toNode(T &value) noexcept {
        return value.*ptrToNode;
    }

    static T &toValue(IntrusiveListNode &node) noexcept {
        return *parentFromMember(&node, ptrToNode);
    }

    static const IntrusiveListNode &toNode(const T &value) noexcept {
        return value.*ptrToNode;
    }

    static const T &toValue(const IntrusiveListNode &node) noexcept {
        return *parentFromMember(&node, ptrToNode);
    }
};

template <typename T, typename NodeGetter>
class IntrusiveList;

template <typename T, typename NodeGetter, bool isConst>
class IntrusiveListIterator {
    typedef IntrusiveList<T, NodeGetter> list_type;
    typedef IntrusiveListNode *node_ptr;
    struct enabler {};

public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef std::ptrdiff_t difference_type;
    typedef
        typename std::conditional<isConst, typename list_type::const_reference,
                                  typename list_type::reference>::type
            reference;
    typedef
        typename std::conditional<isConst, typename list_type::const_pointer,
                                  typename list_type::pointer>::type pointer;

    IntrusiveListIterator() : node(nullptr), nodeGetter(nullptr) {}
    IntrusiveListIterator(node_ptr node_, NodeGetter &nodeGetter_)
        : node(node_), nodeGetter(&nodeGetter_) {}

    // Enable non-const to const conversion.
    template <bool fromConst>
    IntrusiveListIterator(
        const IntrusiveListIterator<T, NodeGetter, fromConst> &other,
        std::enable_if_t<isConst && !fromConst, enabler> = enabler())
        : IntrusiveListIterator(other.pointed_node(), other.get_nodeGetter()) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(IntrusiveListIterator)

    bool operator==(const IntrusiveListIterator &other) const noexcept {
        return node == other.node;
    }
    bool operator!=(const IntrusiveListIterator &other) const noexcept {
        return !operator==(other);
    }
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

    IntrusiveList(NodeGetter nodeGetter_ = NodeGetter())
        : nodeGetter(nodeGetter_) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(IntrusiveList)

    iterator begin() { return {root_.next(), nodeGetter}; }
    iterator end() { return {&root_, nodeGetter}; }

    const_iterator begin() const { return {root_.next(), nodeGetter}; }

    const_iterator end() const { return {&root_, nodeGetter}; }

    const_iterator cbegin() const { return {root_.next(), nodeGetter}; }

    const_iterator cend() const { return {&root_, nodeGetter}; }

    reference front() { return *begin(); }

    const_reference front() const { return *cbegin(); }

    reference back() { return *iterator{root_.prev(), nodeGetter}; }

    const_reference back() const {
        return *const_iterator{root_.prev(), nodeGetter};
        ;
    }

    iterator iterator_to(reference value) {
        return iterator(&nodeGetter.toNode(value), nodeGetter);
    }

    const_iterator iterator_to(const_reference value) {
        return const_iterator(&nodeGetter.toNode(value), nodeGetter);
    }

    void push_back(reference value) {
        auto &node = nodeGetter.toNode(value);
        prepend(&node, &root_);
    }

    void pop_back() { remove(root_.prev()); }

    void push_front(reference value) { insert(begin(), value); }

    void pop_front() { erase(begin()); }

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

    bool empty() const { return root_.next() == &root_; }

    iterator insert(const_iterator pos, reference value) {
        // insert value before pos.
        prepend(&nodeGetter.toNode(value), pos.pointed_node());
        return {pos.pointed_node()->prev(), nodeGetter};
    }

private:
    NodeGetter nodeGetter;
};

template <typename T>
using IntrusiveListFor = IntrusiveList<T, typename T::node_getter_type>;
}

#endif // _FCITX_UTILS_INSTRUSIVELIST_H_
