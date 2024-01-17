/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_HANDLERTABLE_DETAILS_H_
#define _FCITX_UTILS_HANDLERTABLE_DETAILS_H_

#include <list>
#include <memory>
#include <vector>
#include <fcitx-utils/intrusivelist.h>
#include "fcitxutils_export.h"

namespace fcitx {

class FCITXUTILS_EXPORT HandlerTableEntryBase {
public:
    virtual ~HandlerTableEntryBase() = default;
};

// Ugly hack since we don't want to maintain optional.
template <typename T>
using HandlerTableData = std::shared_ptr<std::unique_ptr<T>>;

// Handler Tables are a kind of helper class that helps manage callbacks
// HandlerTableEntry can be deleted
template <typename T>
class HandlerTableEntry : public HandlerTableEntryBase {

public:
    template <typename... Args>
    HandlerTableEntry(Args &&...args)
        : handler_(std::make_shared<std::unique_ptr<T>>(
              std::make_unique<T>(std::forward<Args>(args)...))) {}
    virtual ~HandlerTableEntry() { handler_->reset(); }

    HandlerTableData<T> handler() const { return handler_; }

protected:
    HandlerTableData<T> handler_;
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

    template <typename... Args>
    ListHandlerTableEntry(Args &&...args)
        : HandlerTableEntry<T>(std::forward<Args>(args)...) {}
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
        : HandlerTableEntry<T>(std::move(handler)), table_(table), key_(std::move(key)) {}
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
class HandlerTableView {
    using container_type = std::vector<HandlerTableData<T>>;

public:
    HandlerTableView() = default;

    template <typename _Iter>
    HandlerTableView(_Iter begin, _Iter end) {
        for (; begin != end; begin++) {
            view_.emplace_back(begin->handler());
        }
    }

    class iterator {
    public:
        typedef std::input_iterator_tag iterator_category;
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type &reference;
        typedef value_type *pointer;

        iterator(typename container_type::const_iterator iter,
                 typename container_type::const_iterator end)
            : parentIter_(iter), endIter_(end) {
            while (parentIter_ != endIter_ && !*parentIter_ && !**parentIter_) {
                parentIter_++;
            }
        }

        FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(iterator)

        bool operator==(const iterator &other) const noexcept {
            return parentIter_ == other.parentIter_;
        }
        bool operator!=(const iterator &other) const noexcept {
            return !operator==(other);
        }

        iterator &operator++() {
            do {
                ++parentIter_;
                // *parentIter_ is the shared_ptr, should never be null.
                // **parentIter_ is the optional value, may be null if
                // HandlerEntry is deleted.
            } while (parentIter_ != endIter_ && !(**parentIter_));
            return *this;
        }

        iterator operator++(int) {
            auto old = parentIter_;
            ++(*this);
            return {old, endIter_};
        }

        reference operator*() { return ***parentIter_; }

        pointer operator->() { return (**parentIter_).get(); }

    private:
        typename container_type::const_iterator parentIter_;
        typename container_type::const_iterator endIter_;
    };

    iterator begin() const { return iterator(view_.cbegin(), view_.cend()); }
    iterator end() const { return iterator(view_.cend(), view_.cend()); }

private:
    std::vector<HandlerTableData<T>> view_;
};
} // namespace fcitx

#endif // _FCITX_UTILS_HANDLERTABLE_DETAILS_H_
