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
#ifndef _FCITX_UTILS_SIGNALS_H_
#define _FCITX_UTILS_SIGNALS_H_

#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/intrusivelist.h>
#include <fcitx-utils/trackableobject.h>
#include <fcitx-utils/tuplehelpers.h>
#include <tuple>

namespace fcitx {

template <typename T>
class LastValue {
public:
    LastValue(T defaultValue = T()) : m_initial(defaultValue) {}

    template <typename InputIterator>
    T operator()(InputIterator begin, InputIterator end) {
        T v = m_initial;
        for (; begin != end; begin++) {
            v = *begin;
        }
        return v;
    }

private:
    T m_initial;
};
template <>
class LastValue<void> {
public:
    LastValue() {}
    template <typename InputIterator>
    void operator()(InputIterator begin, InputIterator end) {
        for (; begin != end; begin++) {
            *begin;
        }
    }
};

template <typename Ret, typename... Args>
class Invoker {
public:
    Invoker(Args &... args) : m_args(args...) {}

    template <typename Func>
    Ret operator()(Func &func) {
        return callWithTuple(func, m_args);
    }

private:
    std::tuple<Args &...> m_args;
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

    SlotInvokeIterator(invoker_type &invoker, super_iterator iter) : m_parentIter(iter), m_invoker(invoker) {}

    SlotInvokeIterator(const iterator &other) = default;

    iterator &operator=(const iterator &other) = default;

    bool operator==(const iterator &other) const noexcept { return m_parentIter == other.m_parentIter; }
    bool operator!=(const iterator &other) const noexcept { return !operator==(other); }

    iterator &operator++() {
        m_parentIter++;
        return *this;
    }

    iterator operator++(int) {
        auto old = m_parentIter;
        ++(*this);
        return {m_invoker, old};
    }

    reference operator*() { return m_invoker(*m_parentIter); }

private:
    super_iterator m_parentIter;
    invoker_type &m_invoker;
};

template <typename Invoker, typename Iter>
SlotInvokeIterator<typename Iter::value_type> MakeSlotInvokeIterator(Invoker invoker, Iter iter) {
    return {invoker, iter};
}

template <typename T, typename Combiner = LastValue<typename std::function<T>::result_type>>
class Signal;

class ConnectionBody : public TrackableObject<ConnectionBody>, public IntrusiveListNode {
public:
    template <typename T>
    ConnectionBody(HandlerTableEntry<T> *entry) : m_entry(entry) {}

    virtual ~ConnectionBody() { remove(); }

private:
    // Need type erasure here
    std::shared_ptr<void> m_entry;
};

class Connection {
public:
    Connection() {}
    explicit Connection(TrackableObjectReference<ConnectionBody> body) : m_body(std::move(body)) {}
    Connection(const Connection &other) : m_body(other.m_body) {}

    bool connected() { return m_body.isValid(); }

    void disconnect() {
        auto body = m_body.get();
        // delete nullptr is no-op;
        delete body;
    }
    bool operator==(const Connection &other) const { return m_body.get() == other.m_body.get(); }
    bool operator!=(const Connection &other) const { return !(*this == other); }
    Connection &operator=(const Connection &other) {
        if (&other == this)
            return *this;
        m_body = other.m_body;
        return *this;
    }

private:
    TrackableObjectReference<ConnectionBody> m_body;
};

class SignalBase {
public:
    virtual ~SignalBase() {}
};

template <typename Ret, typename Combiner, typename... Args>
class Signal<Ret(Args...), Combiner> : public SignalBase {

public:
    typedef Ret return_type;
    typedef Ret function_type(Args...);
    Signal(const Combiner &combiner = Combiner()) : m_combiner(combiner) {}
    virtual ~Signal() { disconnectAll(); }

    Ret operator()(Args &&... args) {
        auto view = m_table.view();
        Invoker<Ret, Args...> invoker(args...);
        auto iter = MakeSlotInvokeIterator(invoker, view.begin());
        auto end = MakeSlotInvokeIterator(invoker, view.end());
        return m_combiner(iter, end);
    }

    template <typename Func>
    Connection connect(Func &&func) {
        auto body = new ConnectionBody(m_table.add(std::forward<Func>(func)));
        m_connections.push_back(*body);
        return Connection{body->watch()};
    }

    void disconnectAll() {
        while (!m_connections.empty()) {
            delete &m_connections.front();
        }
    }

private:
    HandlerTable<std::function<Ret(Args...)>> m_table;
    IntrusiveList<ConnectionBody> m_connections;
    Combiner m_combiner;
};
}

#endif // _FCITX_UTILS_SIGNALS_H_
