/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2021-2021 Danh Doan <congdanhqx@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_CONNECTABLEOBJECT_H_
#define _FCITX_UTILS_CONNECTABLEOBJECT_H_

#include <memory>
#include <string>
#include <utility>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/signals.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Utilities to enable use object with signal.

/// \brief Declare signal by type.
#define FCITX_DECLARE_SIGNAL(CLASS_NAME, NAME, ...)                                      \
    struct NAME {                                                                        \
        using signalType = __VA_ARGS__;                                                  \
        using combinerType = ::fcitx::LastValue<std::function<signalType>::result_type>; \
        using signature = fcitxMakeMetaString(#CLASS_NAME "::" #NAME);                   \
    }

/// \brief Declare signal by type with combiner.
///
/// This macro is intended to be used outside of class declaration,
/// because the custom combiner is an implementation detail,
/// thus it'll be put in source files.
#define FCITX_DECLARE_SIGNAL_WITH_COMBINER(CLASS_NAME, NAME, COMBINER, ...)    \
    struct CLASS_NAME::NAME {                                                  \
        using signalType = __VA_ARGS__;                                        \
        using combinerType = COMBINER;                                         \
        using signature = fcitxMakeMetaString(#CLASS_NAME "::" #NAME);         \
    }

/// \brief Declare a signal.
#define FCITX_DEFINE_SIGNAL(CLASS_NAME, NAME)                                  \
    ::fcitx::SignalAdaptor<CLASS_NAME::NAME> CLASS_NAME##NAME##Adaptor { this }

/// \brief Declare a signal in pimpl class.
#define FCITX_DEFINE_SIGNAL_PRIVATE(CLASS_NAME, NAME)                          \
    ::fcitx::SignalAdaptor<CLASS_NAME::NAME> CLASS_NAME##NAME##Adaptor { q_ptr }

namespace fcitx {

class ConnectableObject;

/// \brief Helper class to register class.
template <typename T, typename Combiner = typename T::combinerType>
class SignalAdaptor {
    // FIXME remove Combiner when we can break ABI.
    static_assert(std::is_same<Combiner, typename T::combinerType>::value);
public:
    SignalAdaptor(ConnectableObject *d);
    ~SignalAdaptor();

private:
    ConnectableObject *self;
};

class ConnectableObjectPrivate;

/// \brief Base class for all object supports connection.
class FCITXUTILS_EXPORT ConnectableObject {
    template <typename T, typename Combiner>
    friend class SignalAdaptor;

public:
    ConnectableObject();
    virtual ~ConnectableObject();

    template <typename SignalType, typename F>
    Connection connect(F &&func) {
        auto signal = findSignal(SignalType::signature::data());
        if (signal) {
            return static_cast<Signal<typename SignalType::signalType, typename SignalType::combinerType> *>(
                       signal)
                ->connect(std::forward<F>(func));
        }
        return {};
    }

    template <typename SignalType>
    void disconnectAll() {
        auto signal = findSignal(SignalType::signature::data());
        static_cast<Signal<typename SignalType::signalType, typename SignalType::combinerType> *>(signal)
            ->disconnectAll();
    }

    FCITX_DECLARE_SIGNAL(ConnectableObject, Destroyed, void(void *));

protected:
    /// \brief Allow user to notify the destroy event earlier.
    /// Due the C++ destructor calling order, the subclass is not "subclass"
    /// anymore at the time when parent destructor is called. This protected
    /// function allow user to notify the destruction of objects when they are
    /// still the original type.
    void destroy();

    template <typename SignalType, typename... Args>
    auto emit(Args &&...args) {
        return std::as_const(*this).emit<SignalType>(
            std::forward<Args>(args)...);
    }

    template <typename SignalType, typename... Args>
    auto emit(Args &&...args) const {
        auto signal = findSignal(SignalType::signature::data());
        return (*static_cast<Signal<typename SignalType::signalType, typename SignalType::combinerType> *>(
            signal))(std::forward<Args>(args)...);
    }

    template <typename SignalType,
              typename Combiner = typename SignalType::combinerType>
    void registerSignal() {
        // FIXME remove Combiner when we can break ABI.
        static_assert(std::is_same<Combiner, typename SignalType::combinerType>::value);
        _registerSignal(
            SignalType::signature::data(),
            std::make_unique<
                Signal<typename SignalType::signalType, Combiner>>());
    }

    template <typename SignalType>
    void unregisterSignal() {
        _unregisterSignal(SignalType::signature::data());
    }

private:
    void _registerSignal(std::string name, std::unique_ptr<SignalBase> signal);
    void _unregisterSignal(const std::string &name);
    // FIXME: remove non-const variant when we can break ABI.
    SignalBase *findSignal(const std::string &name);
    SignalBase *findSignal(const std::string &name) const;

    std::unique_ptr<ConnectableObjectPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ConnectableObject);
};

template <typename T, typename Combiner>
SignalAdaptor<T, Combiner>::SignalAdaptor(ConnectableObject *d) : self(d) {
    self->registerSignal<T, Combiner>();
}

template <typename T, typename Combiner>
SignalAdaptor<T, Combiner>::~SignalAdaptor() {
    self->unregisterSignal<T>();
}

/// \brief Short hand for destroyed signal.
using ObjectDestroyed = ConnectableObject::Destroyed;
} // namespace fcitx

#endif // _FCITX_UTILS_CONNECTABLEOBJECT_H_
