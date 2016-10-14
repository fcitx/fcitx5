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
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_H_

#include "fcitxutils_export.h"
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include <functional>
#include <memory>

namespace fcitx {
namespace dbus {
class Message;
class ObjectVTable;
class Slot;
class Bus;

typedef std::function<bool(Message)> ObjectMethod;
typedef std::function<Message()> PropertyGetMethod;
typedef std::function<bool(Message)> PropertySetMethod;

class FCITXUTILS_EXPORT ObjectVTableMethod {
public:
    ObjectVTableMethod(ObjectVTable *vtable, const std::string &name, const std::string &signature,
                       const std::string &ret, ObjectMethod handler);

    const std::string &name() const { return name_; }
    const std::string &signature() const { return signature_; }
    const std::string &ret() const { return ret_; }
    ObjectMethod &handler() { return handler_; }
    ObjectVTable *vtable() const { return vtable_; }

private:
    const std::string name_;
    const std::string signature_;
    const std::string ret_;
    ObjectMethod handler_;
    ObjectVTable *vtable_;
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

#define FCITX_OBJECT_VTABLE_METHOD(FUNCTION, FUNCTION_NAME, SIGNATURE, RET)                                            \
    ::fcitx::dbus::ObjectVTableMethod FUNCTION##Method {                                                               \
        this, FUNCTION_NAME, SIGNATURE, RET, [this](::fcitx::dbus::Message msg) {                                      \
            this->setCurrentMessage(&msg);                                                                             \
            FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) args;                                                                \
            msg >> args;                                                                                               \
            auto func = [this](auto &&... args) { return this->FUNCTION(std::forward<decltype(args)>(args)...); };     \
            typedef decltype(callWithTuple(func, args)) ReturnType;                                                    \
            ::fcitx::dbus::ReturnValueHelper<ReturnType> helper;                                                       \
            auto functor = [this, &args, func]() { return callWithTuple(func, args); };                                \
            helper.call(functor);                                                                                      \
            auto reply = msg.createReply();                                                                            \
            reply << helper.ret;                                                                                       \
            reply.send();                                                                                              \
            return true;                                                                                               \
        }                                                                                                              \
    }

#define FCITX_OBJECT_VTABLE_SIGNAL(SIGNAL, SIGNAL_NAME, SIGNATURE)                                                     \
    ::fcitx::dbus::ObjectVTableSignal SIGNAL##Signal{this, SIGNAL_NAME, SIGNATURE};                                    \
    template <typename... Args>                                                                                        \
    void SIGNAL(Args... args) {                                                                                        \
        auto msg = SIGNAL##Signal.createSignal();                                                                      \
        FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) tupleArg = std::make_tuple(args...);                                     \
        msg << tupleArg;                                                                                               \
        msg.send();                                                                                                    \
    }                                                                                                                  \
    template <typename... Args>                                                                                        \
    void SIGNAL##To(const std::string &dest, Args... args) {                                                           \
        auto msg = SIGNAL##Signal.createSignal();                                                                      \
        msg.setDestination(dest);                                                                                      \
        FCITX_STRING_TO_DBUS_TUPLE(SIGNATURE) tupleArg = std::make_tuple(args...);                                     \
        msg << tupleArg;                                                                                               \
        msg.send();                                                                                                    \
    }

class FCITXUTILS_EXPORT ObjectVTableSignal {
public:
    ObjectVTableSignal(ObjectVTable *vtable, const std::string &name, const std::string signature);

    Message createSignal();

private:
    const std::string name_;
    const std::string signature_;
    ObjectVTable *vtable_;
};

class FCITXUTILS_EXPORT ObjectVTableProperty {
public:
    ObjectVTableProperty(ObjectVTable *vtable, const std::string &name, const std::string signature,
                         PropertyGetMethod getMethod);

protected:
    const std::string name_;
    const std::string signature_;
    PropertyGetMethod getMethod_;
    bool writable_;
};

class FCITXUTILS_EXPORT ObjectVTableWritableProperty : public ObjectVTableProperty {
public:
    ObjectVTableWritableProperty(ObjectVTable *vtable, const std::string &name, const std::string signature,
                                 PropertyGetMethod getMethod, PropertySetMethod setMethod);

private:
    PropertySetMethod setMethod_;
};

class ObjectVTablePrivate;
class MessageSetter;

class FCITXUTILS_EXPORT ObjectVTable : public TrackableObject<ObjectVTable> {
    friend class Bus;
    friend class MessageSetter;

public:
    ObjectVTable();
    virtual ~ObjectVTable();

    void addMethod(ObjectVTableMethod *method);
    void addSignal(ObjectVTableSignal *sig);
    void addProperty(ObjectVTableProperty *property);
    void releaseSlot();

    Bus *bus();
    const std::string &path() const;
    const std::string &interface() const;
    Message *currentMessage() const;

    void setCurrentMessage(Message *message);

private:
    void setSlot(Slot *slot);

    std::unique_ptr<ObjectVTablePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTable);
};
}
}

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
