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
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include <fcitx-utils/tuplehelpers.h>
#include <tuple>

namespace fcitx {

template <typename T>
class LastValue {
public:
    LastValue(T defaultValue = T()) : initial_(defaultValue) {}

    template <typename InputIterator>
    T operator()(InputIterator begin, InputIterator end) {
        T v = initial_;
        for (; begin != end; begin++) {
            v = *begin;
        }
        return v;
    }

private:
    T initial_;
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

template <typename T,
          typename Combiner = LastValue<typename std::function<T>::result_type>>
class Signal;

class ConnectionBody : public TrackableObject<ConnectionBody>,
                       public IntrusiveListNode {
public:
    template <typename T>
    ConnectionBody(HandlerTableEntry<T> *entry) : entry_(entry) {}

    virtual ~ConnectionBody() { remove(); }

private:
    // Need type erasure here
    std::shared_ptr<void> entry_;
};

class Connection {
public:
    Connection() {}
    explicit Connection(TrackableObjectReference<ConnectionBody> body)
        : body_(std::move(body)) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(Connection)

    bool connected() { return body_.isValid(); }

    void disconnect() {
        auto body = body_.get();
        // delete nullptr is no-op;
        delete body;
    }
    bool operator==(const Connection &other) const {
        return body_.get() == other.body_.get();
    }
    bool operator!=(const Connection &other) const { return !(*this == other); }

protected:
    TrackableObjectReference<ConnectionBody> body_;
};

class ScopedConnection : public Connection {
public:
    // You must create two Connection if you really want two ScopedConnection
    // for same actual connection
    ScopedConnection(ScopedConnection &&other) : Connection(std::move(other)) {}
    ScopedConnection(Connection &&other) : Connection(std::move(other)) {}
    ScopedConnection(const ScopedConnection &) = delete;
    ScopedConnection() {}

    ScopedConnection &operator=(ScopedConnection &&other) {
        if (&other == this)
            return *this;
        disconnect();
        Connection::operator=(std::move(other));
        return *this;
    }
    ScopedConnection &operator=(const ScopedConnection &other) = delete;

    virtual ~ScopedConnection() { disconnect(); }

    Connection release() {
        Connection conn(body_);
        body_.unwatch();
        return conn;
    }
};

class SignalBase {
public:
    virtual ~SignalBase() {}
};

template <typename Ret, typename Combiner, typename... Args>
class Signal<Ret(Args...), Combiner> : public SignalBase {
    struct SignalData {
        SignalData(Combiner combiner) : combiner_(std::move(combiner)) {}

        HandlerTable<std::function<Ret(Args...)>> table_;
        IntrusiveList<ConnectionBody> connections_;
        Combiner combiner_;
    };

public:
    typedef Ret return_type;
    typedef Ret function_type(Args...);
    Signal(Combiner combiner = Combiner())
        : d_ptr(std::make_unique<SignalData>(std::move(combiner))) {}
    virtual ~Signal() {
        if (d_ptr) {
            disconnectAll();
        }
    }
    Signal(Signal &&other) noexcept { operator=(std::forward<Signal>(other)); }
    Signal &operator=(Signal &&other) noexcept {
        using std::swap;
        swap(d_ptr, other.d_ptr);
        return *this;
    }

    Ret operator()(Args... args) {
        auto view = d_ptr->table_.view();
        Invoker<Ret, Args...> invoker(args...);
        auto iter = MakeSlotInvokeIterator(invoker, view.begin());
        auto end = MakeSlotInvokeIterator(invoker, view.end());
        return d_ptr->combiner_(iter, end);
    }

    template <typename Func>
    Connection connect(Func &&func) {
        auto body =
            new ConnectionBody(d_ptr->table_.add(std::forward<Func>(func)));
        d_ptr->connections_.push_back(*body);
        return Connection{body->watch()};
    }

    void disconnectAll() {
        while (!d_ptr->connections_.empty()) {
            delete &d_ptr->connections_.front();
        }
    }

private:
    // store data in a unique_ptr to speed up move.
    std::unique_ptr<SignalData> d_ptr;
};
}

#endif // _FCITX_UTILS_SIGNALS_H_
