/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_UTILS_SIGNAL_DETAILS_H_
#define _FCITX_UTILS_SIGNAL_DETAILS_H_

#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/intrusivelist.h>
#include <fcitx-utils/tuplehelpers.h>
#include <fcitx-utils/trackableobject.h>
#include <functional>
#include <iterator>
#include <tuple>

namespace fcitx {

template <typename Ret, typename... Args>
class Invoker {
public:
    Invoker(Args &... args) : args_(args...) {}

    template <typename Func>
    Ret operator()(Func &func) {
        return callWithTuple(func, args_);
    }

private:
    std::tuple<Args &...> args_;
};

template <typename T>
class SlotInvokeIterator;

template <typename Ret, typename... Args>
class SlotInvokeIterator<std::function<Ret(Args...)>> {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::function<Ret(Args...)> function_type;
    typedef typename function_type::result_type value_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type reference;
    typedef typename HandlerTableView<function_type>::iterator super_iterator;
    typedef SlotInvokeIterator iterator;
    typedef Invoker<Ret, Args...> invoker_type;

    SlotInvokeIterator(invoker_type &invoker, super_iterator iter)
        : parentIter_(iter), invoker_(invoker) {}

    SlotInvokeIterator(const iterator &other) = default;

    iterator &operator=(const iterator &other) = default;

    bool operator==(const iterator &other) const noexcept {
        return parentIter_ == other.parentIter_;
    }
    bool operator!=(const iterator &other) const noexcept {
        return !operator==(other);
    }

    iterator &operator++() {
        parentIter_++;
        return *this;
    }

    iterator operator++(int) {
        auto old = parentIter_;
        ++(*this);
        return {invoker_, old};
    }

    reference operator*() { return invoker_(*parentIter_); }

private:
    super_iterator parentIter_;
    invoker_type &invoker_;
};

template <typename Invoker, typename Iter>
SlotInvokeIterator<typename Iter::value_type>
MakeSlotInvokeIterator(Invoker invoker, Iter iter) {
    return {invoker, iter};
}

class ConnectionBody : public TrackableObject<ConnectionBody>,
                       public IntrusiveListNode {
public:
    template <typename T>
    ConnectionBody(HandlerTableEntry<T> *entry) : entry_(entry) {}

    virtual ~ConnectionBody() { remove(); }

private:
    std::unique_ptr<HandlerTableEntryBase> entry_;
};
}

#endif // _FCITX_UTILS_SIGNAL_DETAILS_H_
