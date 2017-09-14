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
#ifndef _FCITX_UTILS_CONNECTABLEOBJECT_H_
#define _FCITX_UTILS_CONNECTABLEOBJECT_H_

#include "fcitxutils_export.h"
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/signals.h>
#include <memory>

#define FCITX_DECLARE_SIGNAL(CLASS_NAME, NAME, ...)                            \
    struct NAME {                                                              \
        using signalType = __VA_ARGS__;                                        \
        using signature = fcitxMakeMetaString(#CLASS_NAME "::" #NAME);         \
    }

#define FCITX_DEFINE_SIGNAL(CLASS_NAME, NAME)                                  \
    ::fcitx::SignalAdaptor<CLASS_NAME::NAME> CLASS_NAME##NAME##Adaptor { this }

#define FCITX_DEFINE_SIGNAL_PRIVATE(CLASS_NAME, NAME)                          \
    ::fcitx::SignalAdaptor<CLASS_NAME::NAME> CLASS_NAME##NAME##Adaptor { q_ptr }

namespace fcitx {

class ConnectableObject;

template <typename T>
class SignalAdaptor {
public:
    SignalAdaptor(ConnectableObject *d);
    ~SignalAdaptor();

private:
    ConnectableObject *self;
};

class ConnectableObjectPrivate;

class FCITXUTILS_EXPORT ConnectableObject {
    template <typename T>
    friend class SignalAdaptor;

public:
    ConnectableObject();
    virtual ~ConnectableObject();

    template <typename SignalType, typename F>
    Connection connect(F &&func) {
        auto signal = findSignal(SignalType::signature::data());
        if (signal) {
            return static_cast<Signal<typename SignalType::signalType> *>(
                       signal)
                ->connect(std::forward<F>(func));
        }
        return {};
    }

    template <typename SignalType>
    void disconnectAll() {
        auto signal = findSignal(SignalType::signature::data());
        static_cast<Signal<typename SignalType::signalType> *>(signal)
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

protected:
    template <typename SignalType, typename... Args>
    auto emit(Args &&... args) {
        auto signal = findSignal(SignalType::signature::data());
        return (*static_cast<Signal<typename SignalType::signalType> *>(
            signal))(std::forward<Args>(args)...);
    }

protected:
    template <typename SignalType>
    void registerSignal() {
        _registerSignal(
            SignalType::signature::data(),
            std::make_unique<Signal<typename SignalType::signalType>>());
    }

    template <typename SignalType>
    void unregisterSignal() {
        _unregisterSignal(SignalType::signature::data());
    }

private:
    void _registerSignal(std::string name, std::unique_ptr<SignalBase> signal);
    void _unregisterSignal(const std::string &name);
    SignalBase *findSignal(const std::string &name);

private:
    std::unique_ptr<ConnectableObjectPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ConnectableObject);
};

template <typename T>
SignalAdaptor<T>::SignalAdaptor(ConnectableObject *d) : self(d) {
    self->registerSignal<T>();
}

template <typename T>
SignalAdaptor<T>::~SignalAdaptor() {
    self->unregisterSignal<T>();
}

using ObjectDestroyed = ConnectableObject::Destroyed;
}

#endif // _FCITX_UTILS_CONNECTABLEOBJECT_H_
