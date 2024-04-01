/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_SIGNALS_H_
#define _FCITX_UTILS_SIGNALS_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief A signal-slot implemention.

#include <tuple>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/intrusivelist.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals_details.h> // IWYU pragma: export
#include <fcitx-utils/trackableobject.h>
#include <fcitx-utils/tuplehelpers.h>

namespace fcitx {

/// \brief Combiner that return the last value.
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
    LastValue() = default;
    template <typename InputIterator>
    void operator()(InputIterator begin, InputIterator end) {
        for (; begin != end; begin++) {
            *begin;
        }
    }
};

/// \brief A connection instance. Can be used to query the existence of
/// connection.
class Connection {
public:
    Connection() = default;
    explicit Connection(TrackableObjectReference<ConnectionBody> body)
        : body_(std::move(body)) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(Connection)

    // FIXME: merge this
    bool connected() { return body_.isValid(); }
    bool connected() const { return body_.isValid(); }

    void disconnect() {
        auto *body = body_.get();
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

/// \brief Connection that will disconnection when it goes out of scope.
/// \see Connection
class ScopedConnection : public Connection {
public:
    // You must create two Connection if you really want two ScopedConnection
    // for same actual connection
    ScopedConnection(ScopedConnection &&other) noexcept
        : Connection(std::move(other)) {}
    ScopedConnection(Connection &&other) noexcept
        : Connection(std::move(other)) {}
    ScopedConnection(const ScopedConnection &) = delete;
    ScopedConnection() = default;

    ScopedConnection &operator=(ScopedConnection &&other) noexcept {
        if (&other == this) {
            return *this;
        }
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
    virtual ~SignalBase() = default;
};

/// \brief Class to represent a signal. May be used like a functor.
template <typename T,
          typename Combiner = LastValue<typename std::function<T>::result_type>>
class Signal;

template <typename Ret, typename Combiner, typename... Args>
class Signal<Ret(Args...), Combiner> : public SignalBase {
    struct SignalData {
        SignalData(Combiner combiner) : combiner_(std::move(combiner)) {}

        HandlerTable<std::function<Ret(Args...)>> table_;
        IntrusiveList<ConnectionBody> connections_;
        Combiner combiner_;
    };

public:
    using return_type = Ret;
    using function_type = Ret(Args...);
    Signal(Combiner combiner = Combiner())
        : d_ptr(std::make_unique<SignalData>(std::move(combiner))) {}
    ~Signal() override {
        if (d_ptr) {
            disconnectAll();
        }
    }
    Signal(Signal &&other) noexcept { operator=(std::move(other)); }
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
        auto *body =
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
} // namespace fcitx

#endif // _FCITX_UTILS_SIGNALS_H_
