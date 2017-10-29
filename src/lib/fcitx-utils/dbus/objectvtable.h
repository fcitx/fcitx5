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
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_H_

#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace fcitx {
namespace dbus {
class Message;
class ObjectVTableBase;
class Slot;
class Bus;
class ObjectVTablePrivate;

typedef std::function<bool(Message)> ObjectMethod;
typedef std::function<void(Message &)> PropertyGetMethod;
typedef std::function<bool(Message)> PropertySetMethod;

class FCITXUTILS_EXPORT MethodCallError : public std::exception {
public:
    MethodCallError(const char *name, const char *error)
        : name_(name), error_(error) {}

    const char *what() const noexcept override { return error_.c_str(); }

    const char *name() const { return name_.c_str(); }

private:
    std::string name_;
    std::string error_;
};

class FCITXUTILS_EXPORT ObjectVTableMethod {
public:
    ObjectVTableMethod(ObjectVTableBase *vtable, const std::string &name,
                       const std::string &signature, const std::string &ret,
                       ObjectMethod handler);

    const std::string &name() const { return name_; }
    const std::string &signature() const { return signature_; }
    const std::string &ret() const { return ret_; }
    ObjectMethod handler() { return handler_; }
    ObjectVTableBase *vtable() const { return vtable_; }

private:
    const std::string name_;
    const std::string signature_;
    const std::string ret_;
    ObjectMethod handler_;
    ObjectVTableBase *vtable_;
};

template <typename T>
struct ReturnValueHelper {
    typedef T type;
    type ret;

    template <typename U>
    void call(U u) {
        ret = u();
    }
};

template <>
struct ReturnValueHelper<void> {
    typedef std::tuple<> type;
    type ret;
    template <typename U>
    void call(U u) {
        u();
    }
};

#define FCITX_OBJECT_VTABLE_METHOD(FUNCTION, FUNCTION_NAME, SIGNATURE, RET)    \
    ::fcitx::dbus::ObjectVTableMethod FUNCTION##Method {                       \
        this, FUNCTION_NAME, SIGNATURE, RET, [this](                           \
                                                 ::fcitx::dbus::Message msg) { \
            this->setCurrentMessage(&msg);                                     \
            FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) args;                        \
            msg >> args;                                                       \
            auto func = [this](auto that, auto &&... args) {                   \
                return that->FUNCTION(std::forward<decltype(args)>(args)...);  \
            };                                                                 \
            auto argsWithThis =                                                \
                std::tuple_cat(std::make_tuple(this), std::move(args));        \
            typedef decltype(callWithTuple(func, argsWithThis)) ReturnType;    \
            ::fcitx::dbus::ReturnValueHelper<ReturnType> helper;               \
            auto functor = [&argsWithThis, func]() {                           \
                return callWithTuple(func, argsWithThis);                      \
            };                                                                 \
            try {                                                              \
                helper.call(functor);                                          \
                auto reply = msg.createReply();                                \
                reply << helper.ret;                                           \
                reply.send();                                                  \
            } catch (const ::fcitx::dbus::MethodCallError &error) {            \
                auto reply = msg.createError(error.name(), error.what());      \
                reply.send();                                                  \
            }                                                                  \
            return true;                                                       \
        }                                                                      \
    }

#define FCITX_OBJECT_VTABLE_SIGNAL(SIGNAL, SIGNAL_NAME, SIGNATURE)             \
    ::fcitx::dbus::ObjectVTableSignal SIGNAL##Signal{this, SIGNAL_NAME,        \
                                                     SIGNATURE};               \
    typedef FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) SIGNAL##ArgType;             \
    template <typename... Args>                                                \
    void SIGNAL(Args &&... args) {                                             \
        auto msg = SIGNAL##Signal.createSignal();                              \
        SIGNAL##ArgType tupleArg{std::forward<Args>(args)...};                 \
        msg << tupleArg;                                                       \
        msg.send();                                                            \
    }                                                                          \
    template <typename... Args>                                                \
    void SIGNAL##To(const std::string &dest, Args &&... args) {                \
        auto msg = SIGNAL##Signal.createSignal();                              \
        msg.setDestination(dest);                                              \
        SIGNAL##ArgType tupleArg{std::forward<Args>(args)...};                 \
        msg << tupleArg;                                                       \
        msg.send();                                                            \
    }

#define FCITX_OBJECT_VTABLE_PROPERTY(PROPERTY, NAME, SIGNATURE, GETMETHOD)     \
    ::fcitx::dbus::ObjectVTableProperty PROPERTY##Property{                    \
        this, NAME, SIGNATURE, [this](::fcitx::dbus::Message &msg) {           \
            auto method = GETMETHOD;                                           \
            msg << method();                                                   \
        }};

class FCITXUTILS_EXPORT ObjectVTableSignal {
public:
    ObjectVTableSignal(ObjectVTableBase *vtable, const std::string &name,
                       const std::string signature);

    Message createSignal();
    const std::string &name() const { return name_; }
    const std::string &signature() const { return signature_; }

private:
    const std::string name_;
    const std::string signature_;
    ObjectVTableBase *vtable_;
};

class FCITXUTILS_EXPORT ObjectVTableProperty {
public:
    ObjectVTableProperty(ObjectVTableBase *vtable, const std::string &name,
                         const std::string signature,
                         PropertyGetMethod getMethod);

    const std::string &name() const { return name_; }

    const std::string &signature() const { return signature_; }

    bool writable() { return writable_; }

    auto getMethod() { return getMethod_; }

protected:
    const std::string name_;
    const std::string signature_;
    PropertyGetMethod getMethod_;
    bool writable_;
};

class FCITXUTILS_EXPORT ObjectVTableWritableProperty
    : public ObjectVTableProperty {
public:
    ObjectVTableWritableProperty(ObjectVTableBase *vtable,
                                 const std::string &name,
                                 const std::string signature,
                                 PropertyGetMethod getMethod,
                                 PropertySetMethod setMethod);

    auto setMethod() { return setMethod_; }

private:
    PropertySetMethod setMethod_;
};

class ObjectVTableBasePrivate;
class MessageSetter;

class FCITXUTILS_EXPORT ObjectVTableBase
    : public TrackableObject<ObjectVTableBase> {
    friend class Bus;
    friend class MessageSetter;

public:
    ObjectVTableBase();
    virtual ~ObjectVTableBase();

    void addMethod(ObjectVTableMethod *method);
    void addSignal(ObjectVTableSignal *sig);
    void addProperty(ObjectVTableProperty *property);
    void releaseSlot();

    Bus *bus();
    const std::string &path() const;
    const std::string &interface() const;
    Message *currentMessage() const;

    void setCurrentMessage(Message *message);

    ObjectVTableMethod *findMethod(const std::string &name);
    ObjectVTableProperty *findProperty(const std::string &name);

protected:
    virtual std::mutex &privateDataMutexForType() = 0;
    virtual ObjectVTablePrivate *privateDataForType() = 0;
    static std::shared_ptr<ObjectVTablePrivate> newSharedPrivateData();

private:
    void setSlot(Slot *slot);

    std::unique_ptr<ObjectVTableBasePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTableBase);
};

template <typename T>
class ObjectVTable : public ObjectVTableBase {
public:
    std::mutex &privateDataMutexForType() override {
        return privateDataMutex();
    }
    ObjectVTablePrivate *privateDataForType() override { return privateData(); }
    static std::mutex &privateDataMutex() {
        static std::mutex mutex;
        return mutex;
    }
    static ObjectVTablePrivate *privateData() {
        std::mutex mutex; // not using privateDataMutex to avoid deadlock
        static std::shared_ptr<ObjectVTablePrivate> d;
        std::lock_guard<std::mutex> lock(mutex);
        if (!d) {
            d = newSharedPrivateData();
        }
        return d.get();
    }

private:
};
}
}

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
