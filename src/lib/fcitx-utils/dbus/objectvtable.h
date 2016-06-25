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

#include <memory>
#include <functional>
#include "macros.h"
#include "fcitxutils_export.h"

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

    const std::string &name() const { return m_name; }
    const std::string &signature() const { return m_signature; }
    const std::string &ret() const { return m_ret; }
    ObjectMethod &handler() { return m_handler; }

private:
    const std::string m_name;
    const std::string m_signature;
    const std::string m_ret;
    ObjectMethod m_handler;
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
            ::fcitx::dbus::MessageSetter msgSetter(&msg, this);                                                                       \
            STRING_TO_DBUS_TUPLE(SIGNATURE) args;                                                                      \
            msg >> args;                                                                                               \
            auto func = &std::remove_reference<decltype(*this)>::type::FUNCTION;                                       \
            typedef decltype(callWithTuple(this, func, args)) ReturnType;                                              \
            ::fcitx::dbus::ReturnValueHelper<ReturnType> helper;                                                       \
            auto functor = [this, &args, func]() { return callWithTuple(this, func, args); };                          \
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
        STRING_TO_DBUS_TUPLE(SIGNATURE) tupleArg = std::make_tuple(args...);                                           \
        msg << tupleArg;                                                                                               \
        msg.send();                                                                                                    \
    }

class FCITXUTILS_EXPORT ObjectVTableSignal {
public:
    ObjectVTableSignal(ObjectVTable *vtable, const std::string &name, const std::string signature);

    Message createSignal();

private:
    const std::string m_name;
    const std::string m_signature;
    ObjectVTable *m_vtable;
};

class FCITXUTILS_EXPORT ObjectVTableProperty {
public:
    ObjectVTableProperty(ObjectVTable *vtable, const std::string &name, const std::string signature,
                         PropertyGetMethod getMethod);

protected:
    const std::string m_name;
    const std::string m_signature;
    PropertyGetMethod m_getMethod;
    bool m_writable;
};

class FCITXUTILS_EXPORT ObjectVTableWritableProperty : public ObjectVTableProperty {
public:
    ObjectVTableWritableProperty(ObjectVTable *vtable, const std::string &name, const std::string signature,
                                 PropertyGetMethod getMethod, PropertySetMethod setMethod);

private:
    PropertySetMethod m_setMethod;
};

class ObjectVTablePrivate;

class FCITXUTILS_EXPORT ObjectVTable {
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

private:
    void setCurrentMessage(Message *message);
    void setSlot(Slot *slot);

    std::unique_ptr<ObjectVTablePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ObjectVTable);
};


class FCITXUTILS_EXPORT MessageSetter {
public:
    MessageSetter(Message *message, ObjectVTable *vtable) : m_vtable(vtable) {
        vtable->setCurrentMessage(message);
    }

    ~MessageSetter() {
        m_vtable->setCurrentMessage(nullptr);
    }
private:
    ObjectVTable *m_vtable;
};

}
}

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_H_
