//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_H_

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>

namespace fcitx {
namespace dbus {
class Message;
class ObjectVTableBase;
class Slot;
class Bus;
class ObjectVTablePrivate;

typedef std::function<bool(Message)> ObjectMethod;
typedef std::function<void(Message &)> PropertyGetMethod;
typedef std::function<bool(Message &)> PropertySetMethod;

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

class ObjectVTableMethodPrivate;
class FCITXUTILS_EXPORT ObjectVTableMethod {
public:
    ObjectVTableMethod(ObjectVTableBase *vtable, const std::string &name,
                       const std::string &signature, const std::string &ret,
                       ObjectMethod handler);

    virtual ~ObjectVTableMethod();

    const std::string &name() const;
    const std::string &signature() const;
    const std::string &ret() const;
    const ObjectMethod &handler() const;
    ObjectVTableBase *vtable() const;

private:
    std::unique_ptr<ObjectVTableMethodPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTableMethod);
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
        this, FUNCTION_NAME, SIGNATURE, RET,                                   \
            [this](::fcitx::dbus::Message msg) {                               \
                this->setCurrentMessage(&msg);                                 \
                auto watcher = static_cast<ObjectVTableBase *>(this)->watch(); \
                FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) args;                    \
                msg >> args;                                                   \
                auto func = [](auto that, auto &&... args) {                   \
                    return that->FUNCTION(                                     \
                        std::forward<decltype(args)>(args)...);                \
                };                                                             \
                auto argsWithThis =                                            \
                    std::tuple_cat(std::make_tuple(this), std::move(args));    \
                typedef decltype(                                              \
                    callWithTuple(func, argsWithThis)) ReturnType;             \
                ::fcitx::dbus::ReturnValueHelper<ReturnType> helper;           \
                auto functor = [&argsWithThis, func]() {                       \
                    return callWithTuple(func, argsWithThis);                  \
                };                                                             \
                try {                                                          \
                    helper.call(functor);                                      \
                    auto reply = msg.createReply();                            \
                    static_assert(std::is_same<FCITX_STRING_TO_DBUS_TYPE(RET), \
                                               ReturnType>::value,             \
                                  "Return type does not match: " RET);         \
                    reply << helper.ret;                                       \
                    reply.send();                                              \
                } catch (const ::fcitx::dbus::MethodCallError &error) {        \
                    auto reply = msg.createError(error.name(), error.what());  \
                    reply.send();                                              \
                }                                                              \
                if (watcher.isValid()) {                                       \
                    watcher.get()->setCurrentMessage(nullptr);                 \
                }                                                              \
                return true;                                                   \
            }                                                                  \
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

#define FCITX_OBJECT_VTABLE_PROPERTY(PROPERTY, NAME, SIGNATURE, GETMETHOD,     \
                                     ...)                                      \
    ::fcitx::dbus::ObjectVTableProperty PROPERTY##Property{                    \
        this, NAME, SIGNATURE,                                                 \
        [method = GETMETHOD](::fcitx::dbus::Message &msg) {                    \
            typedef FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) property_type;       \
            property_type property = method();                                 \
            msg << property;                                                   \
        },                                                                     \
        ::fcitx::dbus::PropertyOptions{__VA_ARGS__}};

#define FCITX_OBJECT_VTABLE_WRITABLE_PROPERTY(PROPERTY, NAME, SIGNATURE,       \
                                              GETMETHOD, SETMETHOD, ...)       \
    ::fcitx::dbus::ObjectVTableWritableProperty PROPERTY##Property{            \
        this,                                                                  \
        NAME,                                                                  \
        SIGNATURE,                                                             \
        [method = GETMETHOD](::fcitx::dbus::Message &msg) {                    \
            typedef FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) property_type;       \
            property_type property = method();                                 \
            msg << property;                                                   \
        },                                                                     \
        [this, method = SETMETHOD](::fcitx::dbus::Message &msg) {              \
            this->setCurrentMessage(&msg);                                     \
            auto watcher = static_cast<ObjectVTableBase *>(this)->watch();     \
            FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) args;                        \
            msg >> args;                                                       \
            callWithTuple(method, args);                                       \
            auto reply = msg.createReply();                                    \
            reply.send();                                                      \
            if (watcher.isValid()) {                                           \
                watcher.get()->setCurrentMessage(nullptr);                     \
            }                                                                  \
            return true;                                                       \
        },                                                                     \
        ::fcitx::dbus::PropertyOptions{__VA_ARGS__}};

class ObjectVTableSignalPrivate;
class FCITXUTILS_EXPORT ObjectVTableSignal {
public:
    ObjectVTableSignal(ObjectVTableBase *vtable, const std::string &name,
                       const std::string signature);
    virtual ~ObjectVTableSignal();

    Message createSignal();
    const std::string &name() const;
    const std::string &signature() const;

private:
    std::unique_ptr<ObjectVTableSignalPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTableSignal);
};

enum class PropertyOption : uint32_t { Hidden = (1 << 0) };

using PropertyOptions = Flags<PropertyOption>;

class ObjectVTablePropertyPrivate;
class FCITXUTILS_EXPORT ObjectVTableProperty {
public:
    ObjectVTableProperty(ObjectVTableBase *vtable, const std::string &name,
                         const std::string signature,
                         PropertyGetMethod getMethod, PropertyOptions options);
    virtual ~ObjectVTableProperty();

    const std::string &name() const;
    const std::string &signature() const;
    bool writable() const;
    const PropertyGetMethod &getMethod() const;
    const PropertyOptions &options() const;

protected:
    ObjectVTableProperty(std::unique_ptr<ObjectVTablePropertyPrivate> d);

    std::unique_ptr<ObjectVTablePropertyPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTableProperty);
};

class FCITXUTILS_EXPORT ObjectVTableWritableProperty
    : public ObjectVTableProperty {
public:
    ObjectVTableWritableProperty(ObjectVTableBase *vtable,
                                 const std::string &name,
                                 const std::string signature,
                                 PropertyGetMethod getMethod,
                                 PropertySetMethod setMethod,
                                 PropertyOptions options);

    const PropertySetMethod &setMethod() const;
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
        static std::shared_ptr<ObjectVTablePrivate> d(newSharedPrivateData());
        return d.get();
    }
};
} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
