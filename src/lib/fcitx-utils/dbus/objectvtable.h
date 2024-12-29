/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_H_

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <fcitx-utils/dbus/message.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief High level API for dbus objects.

namespace fcitx::dbus {
class Message;
class ObjectVTableBase;
class Slot;
class Bus;
class ObjectVTablePrivate;

using ObjectMethod = std::function<bool(Message)>;
using ObjectMethodClosure = std::function<bool(Message, const ObjectMethod &)>;
using PropertyGetMethod = std::function<void(Message &)>;
using PropertySetMethod = std::function<bool(Message &)>;

/**
 * An exception if you want message to return a DBus error.
 *
 * In the registered property or method, you may throw this exception if a DBus
 * error happens.
 *
 * E.g.
 * @code
 * throw dbus::MethodCallError("org.freedesktop.DBus.Error.InvalidArgs", ...);
 * @endcode
 */
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

/**
 * Register a DBus method to current DBus VTable.
 *
 * Usually this class should not be used directly in the code.
 *
 * @see FCITX_OBJECT_VTABLE_METHOD
 */
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

    /**
     * Set a closure function to call the handler with in it.
     *
     * This is useful when you want to do something before and after the dbus
     * message delivery.
     *
     * @param wrapper wrapper function.
     */
    void setClosureFunction(ObjectMethodClosure closure);

private:
    std::unique_ptr<ObjectVTableMethodPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTableMethod);
};

template <typename T>
struct ReturnValueHelper {
    using type = T;
    type ret;

    template <typename U>
    void call(U u) {
        ret = u();
    }
};

template <>
struct ReturnValueHelper<void> {
    using type = std::tuple<>;
    type ret;
    template <typename U>
    void call(U u) {
        u();
    }
};

/**
 * Register a class member function as a DBus method.
 *
 * It will also check if the dbus signature matches the function type.
 *
 * @param FUNCTION a member function of the class
 * @param FUNCTION_NAME a string of DBus method name
 * @param SIGNATURE The dbus signature of arguments.
 * @param RET The dbus signature of the return value.
 *
 * @see https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
 */
#define FCITX_OBJECT_VTABLE_METHOD(FUNCTION, FUNCTION_NAME, SIGNATURE, RET)    \
    ::fcitx::dbus::ObjectVTableMethod FUNCTION##Method {                       \
        this, FUNCTION_NAME, SIGNATURE, RET,                                   \
            ::fcitx::dbus::makeObjectVTablePropertyObjectMethodAdaptor<        \
                FCITX_STRING_TO_DBUS_TYPE(RET),                                \
                FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE)>(                        \
                this, [this](auto &&...args) {                                 \
                    return this->FUNCTION(                                     \
                        std::forward<decltype(args)>(args)...);                \
                })                                                             \
    }

/**
 * Register a new DBus signal.
 *
 * This macro will define two new function, SIGNAL and SIGNALTo.
 *
 * The latter one will only be send to one DBus destination.
 *
 * @param SIGNAL will be used to define two member functions.
 * @param SIGNAL_NAME a string of DBus signal name
 * @param SIGNATURE The dbus signature of the signal.
 *
 * @see https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
 */
#define FCITX_OBJECT_VTABLE_SIGNAL(SIGNAL, SIGNAL_NAME, SIGNATURE)             \
    ::fcitx::dbus::ObjectVTableSignal SIGNAL##Signal{this, SIGNAL_NAME,        \
                                                     SIGNATURE};               \
    using SIGNAL##ArgType = FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE);             \
    template <typename... Args>                                                \
    void SIGNAL(Args &&...args) {                                              \
        auto msg = SIGNAL##Signal.createSignal();                              \
        SIGNAL##ArgType tupleArg{std::forward<Args>(args)...};                 \
        msg << tupleArg;                                                       \
        msg.send();                                                            \
    }                                                                          \
    template <typename... Args>                                                \
    void SIGNAL##To(const std::string &dest, Args &&...args) {                 \
        auto msg = SIGNAL##Signal.createSignal();                              \
        msg.setDestination(dest);                                              \
        SIGNAL##ArgType tupleArg{std::forward<Args>(args)...};                 \
        msg << tupleArg;                                                       \
        msg.send();                                                            \
    }

/**
 * Register a new DBus read-only property.
 *
 * @param PROPERTY will be used to define class member.
 * @param NAME a string of DBus property name
 * @param SIGNATURE The dbus signature of the property.
 * @param GETMETHOD The method used to return the value of the property
 *
 * @see https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
 */
#define FCITX_OBJECT_VTABLE_PROPERTY(PROPERTY, NAME, SIGNATURE, GETMETHOD,     \
                                     ...)                                      \
    ::fcitx::dbus::ObjectVTableProperty PROPERTY##Property{                    \
        this, NAME, SIGNATURE,                                                 \
        ::fcitx::dbus::makeObjectVTablePropertyGetMethodAdaptor<               \
            FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE)>(this, GETMETHOD),           \
        ::fcitx::dbus::PropertyOptions{__VA_ARGS__}};

/**
 * Register a new DBus read-only property.
 *
 * @param PROPERTY will be used to define class member.
 * @param NAME a string of DBus property name
 * @param SIGNATURE The dbus signature of the property.
 * @param GETMETHOD The method used to return the value of the property
 * @param SETMETHOD The method used to update the value of the property
 *
 * @see https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
 */
#define FCITX_OBJECT_VTABLE_WRITABLE_PROPERTY(PROPERTY, NAME, SIGNATURE,       \
                                              GETMETHOD, SETMETHOD, ...)       \
    ::fcitx::dbus::ObjectVTableWritableProperty PROPERTY##Property{            \
        this,                                                                  \
        NAME,                                                                  \
        SIGNATURE,                                                             \
        ::fcitx::dbus::makeObjectVTablePropertyGetMethodAdaptor<               \
            FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE)>(this, GETMETHOD),           \
        ::fcitx::dbus::makeObjectVTablePropertySetMethodAdaptor<               \
            FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE)>(this, SETMETHOD),           \
        ::fcitx::dbus::PropertyOptions{__VA_ARGS__}};

class ObjectVTableSignalPrivate;

/**
 * Register a DBus signal to current DBus VTable.
 *
 * Usually this class should not be used directly in the code.
 *
 * @see FCITX_OBJECT_VTABLE_SIGNAL
 */
class FCITXUTILS_EXPORT ObjectVTableSignal {
public:
    ObjectVTableSignal(ObjectVTableBase *vtable, std::string name,
                       std::string signature);
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

/**
 * Register a DBus read-only property to current DBus VTable.
 *
 * Usually this class should not be used directly in the code.
 *
 * @see FCITX_OBJECT_VTABLE_PROPERTY
 */
class FCITXUTILS_EXPORT ObjectVTableProperty {
public:
    ObjectVTableProperty(ObjectVTableBase *vtable, std::string name,
                         std::string signature, PropertyGetMethod getMethod,
                         PropertyOptions options);
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

/**
 * Register a DBus property to current DBus VTable.
 *
 * Usually this class should not be used directly in the code.
 *
 * @see FCITX_OBJECT_VTABLE_WRITABLE_PROPERTY
 */
class FCITXUTILS_EXPORT ObjectVTableWritableProperty
    : public ObjectVTableProperty {
public:
    ObjectVTableWritableProperty(ObjectVTableBase *vtable, std::string name,
                                 std::string signature,
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

    /**
     * Unregister the dbus object from the bus.
     *
     * The object will automatically unregister itself upon destruction. So this
     * method should only be used if you want to temporarily remove a object
     * from dbus.
     */
    void releaseSlot();

    /// Return the bus that the object is registered to.
    Bus *bus();
    Bus *bus() const;
    /// Return whether this object is registered to a bus.
    bool isRegistered() const;
    /// Return the registered dbus object path of the object.
    const std::string &path() const;
    /// Return the registered dbus interface of the object.
    const std::string &interface() const;

    /**
     * Return the current dbus message for current method.
     *
     * This should only be used with in a registered callback.
     *
     * @return DBus message
     */
    Message *currentMessage() const;

    /**
     * Set the current dbus message.
     *
     * This is only used by internal dbus class and not supposed to be used
     * anywhere else.
     *
     * @param message current message.
     */
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

/**
 * Base class of any DBus object.
 *
 * This should be used with curiously recurring template pattern. Like:
 *
 * @code
 * class Object : public ObjectVTable<OBject> {};
 * @endcode
 *
 * It will instantiate the related shared data for this type.
 *
 */
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

template <typename Ret, typename Args, typename Callback>
class ObjectVTablePropertyObjectMethodAdaptor {
public:
    ObjectVTablePropertyObjectMethodAdaptor(ObjectVTableBase *base,
                                            Callback callback)
        : base_(base), callback_(std::move(callback)) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(
        ObjectVTablePropertyObjectMethodAdaptor);

    bool operator()(Message msg) {
        base_->setCurrentMessage(&msg);
        auto watcher = base_->watch();
        Args args;
        msg >> args;
        try {
            using ReturnType = decltype(callWithTuple(callback_, args));
            static_assert(std::is_same<Ret, ReturnType>::value,
                          "Return type does not match.");
            ReturnValueHelper<ReturnType> helper;
            helper.call(
                [this, &args]() { return callWithTuple(callback_, args); });
            auto reply = msg.createReply();
            reply << helper.ret;
            reply.send();
        } catch (const ::fcitx::dbus::MethodCallError &error) {
            auto reply = msg.createError(error.name(), error.what());
            reply.send();
        }
        if (watcher.isValid()) {
            watcher.get()->setCurrentMessage(nullptr);
        }
        return true;
    }

private:
    ObjectVTableBase *base_;
    Callback callback_;
};

template <typename Ret, typename Args, typename Callback>
auto makeObjectVTablePropertyObjectMethodAdaptor(ObjectVTableBase *base,
                                                 Callback &&callback) {
    return ObjectVTablePropertyObjectMethodAdaptor<Ret, Args, Callback>(
        base, std::forward<Callback>(callback));
}

template <typename Ret, typename Callback>
class ObjectVTablePropertyGetMethodAdaptor {
public:
    ObjectVTablePropertyGetMethodAdaptor(ObjectVTableBase *base,
                                         Callback callback)
        : base_(base), callback_(std::move(callback)) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(
        ObjectVTablePropertyGetMethodAdaptor);

    void operator()(Message &msg) {
        Ret property = callback_();
        msg << property;
    }

private:
    ObjectVTableBase *base_;
    Callback callback_;
};

template <typename Ret, typename Callback>
auto makeObjectVTablePropertyGetMethodAdaptor(ObjectVTableBase *base,
                                              Callback &&callback) {
    return ObjectVTablePropertyGetMethodAdaptor<Ret, Callback>(
        base, std::forward<Callback>(callback));
}

template <typename Ret, typename Callback>
class ObjectVTablePropertySetMethodAdaptor {
public:
    ObjectVTablePropertySetMethodAdaptor(ObjectVTableBase *base,
                                         Callback callback)
        : base_(base), callback_(std::move(callback)) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(
        ObjectVTablePropertySetMethodAdaptor);

    bool operator()(Message &msg) {
        base_->setCurrentMessage(&msg);
        auto watcher = base_->watch();
        Ret args;
        msg >> args;
        callWithTuple(callback_, args);
        auto reply = msg.createReply();
        reply.send();
        if (watcher.isValid()) {
            watcher.get()->setCurrentMessage(nullptr);
        }
        return true;
    }

private:
    ObjectVTableBase *base_;
    Callback callback_;
};

template <typename Ret, typename Callback>
auto makeObjectVTablePropertySetMethodAdaptor(ObjectVTableBase *base,
                                              Callback &&callback) {
    return ObjectVTablePropertySetMethodAdaptor<Ret, Callback>(
        base, std::forward<Callback>(callback));
}

} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
