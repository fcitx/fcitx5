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
#ifndef _FCITX_UTILS_DBUS_MESSAGE_H_
#define _FCITX_UTILS_DBUS_MESSAGE_H_

#include "fcitxutils_export.h"
#include <string>
#include <memory>
#include <tuple>
#include "unixfd.h"
#include "macros.h"
#include "tuplehelpers.h"
#include "metastring.h"

namespace fcitx {

namespace dbus {

template <typename T>
struct DBusSignatureTraits;

template <char>
struct DBusSignatureToType;

#define DBUS_SIGNATURE_TRAITS(TYPENAME, SIG)                                                                           \
    template <>                                                                                                        \
    struct DBusSignatureTraits<TYPENAME> {                                                                             \
        static constexpr char value = SIG;                                                                             \
    };                                                                                                                 \
                                                                                                                       \
    template <>                                                                                                        \
    struct DBusSignatureToType<SIG> {                                                                                  \
        typedef TYPENAME type;                                                                                         \
    };

DBUS_SIGNATURE_TRAITS(std::string, 's');
DBUS_SIGNATURE_TRAITS(uint8_t, 'y');
DBUS_SIGNATURE_TRAITS(bool, 'b');
DBUS_SIGNATURE_TRAITS(int16_t, 'n');
DBUS_SIGNATURE_TRAITS(uint16_t, 'q');
DBUS_SIGNATURE_TRAITS(int32_t, 'i');
DBUS_SIGNATURE_TRAITS(uint32_t, 'u');
DBUS_SIGNATURE_TRAITS(int64_t, 'x');
DBUS_SIGNATURE_TRAITS(uint64_t, 't');
DBUS_SIGNATURE_TRAITS(double, 'd');
DBUS_SIGNATURE_TRAITS(UnixFD, 'h');

template <char...>
struct DBusSignatureToTuple;

template <char first, char... next>
struct DBusSignatureToTuple<first, next...> {
    typedef typename CombineTuples<std::tuple<typename DBusSignatureToType<first>::type>,
                                   typename DBusSignatureToTuple<next...>::type>::type type;
};

template <>
struct DBusSignatureToTuple<> {
    typedef std::tuple<> type;
};

template <char... c>
auto MetaStringToDBusTuple(MetaString<c...>) -> DBusSignatureToTuple<c...>;

#define STRING_TO_DBUS_TUPLE(STRING) decltype(::fcitx::dbus::MetaStringToDBusTuple(makeMetaString(STRING)()))::type

class Message;
typedef std::function<bool(Message message)> MessageCallback;
class Slot;

enum class MessageType {
    Invalid,
    Signal,
    MethodCall,
    Reply,
    Error,
};

class FCITXUTILS_EXPORT ObjectPath {
public:
    ObjectPath(const std::string &path = {}) : m_path(path) {}

    const std::string &path() const { return m_path; }

private:
    std::string m_path;
};

class FCITXUTILS_EXPORT Signature {
public:
    Signature(const std::string &sig = {}) : m_sig(sig) {}

    const std::string &sig() const { return m_sig; }

private:
    std::string m_sig;
};

class FCITXUTILS_EXPORT Container {
public:
    enum class Type { Array, DictEntry, Struct, Variant };

    Container(Type t = Type::Array, const Signature &content = Signature()) : m_type(t), m_content(content) {}

    Type type() const { return m_type; }
    const Signature &content() const { return m_content; }

private:
    Type m_type;
    Signature m_content;
};

class FCITXUTILS_EXPORT ContainerEnd {};

class MessagePrivate;

template <typename Tuple, std::size_t N>
struct TupleMarshaller {
    static void marshall(Message &msg, const Tuple &t) {
        TupleMarshaller<Tuple, N - 1>::marshall(msg, t);
        msg << std::get<N - 1>(t);
    }
    static void unmarshall(Message &msg, Tuple &t) {
        TupleMarshaller<Tuple, N - 1>::unmarshall(msg, t);
        msg >> std::get<N - 1>(t);
    }
};

template <typename Tuple>
struct TupleMarshaller<Tuple, 1> {
    static void marshall(Message &msg, const Tuple &t) { msg << std::get<0>(t); }
    static void unmarshall(Message &msg, Tuple &t) { msg >> std::get<0>(t); }
};

template <typename Tuple>
struct TupleMarshaller<Tuple, 0> {
    static void marshall(Message &, const Tuple &) {}
    static void unmarshall(Message &, Tuple &) {}
};

class FCITXUTILS_EXPORT Message {
    friend class Bus;

public:
    Message();
    virtual ~Message();

    Message(const Message &other) = delete;
    Message(Message &&other);
    Message createReply() const;
    Message createError(const char *name, const char *message) const;

    MessageType type() const;

    std::string destination() const;
    void setDestination(const std::string &dest);

    std::string signature() const;

    void *nativeHandle() const;

    Message call(uint64_t usec);
    Slot *callAsync(uint64_t usec, MessageCallback callback);
    bool send();

    Message &operator<<(uint8_t i);
    Message &operator<<(bool b);
    Message &operator<<(int16_t i);
    Message &operator<<(uint16_t i);
    Message &operator<<(int32_t i);
    Message &operator<<(uint32_t i);
    Message &operator<<(int64_t i);
    Message &operator<<(uint64_t i);
    Message &operator<<(double d);
    Message &operator<<(const std::string &s);
    Message &operator<<(const ObjectPath &o);
    Message &operator<<(const Signature &s);
    Message &operator<<(const UnixFD &fd);
    Message &operator<<(const Container &c);
    Message &operator<<(const ContainerEnd &c);

    template <typename... Args>
    Message &operator<<(const std::tuple<Args...> &t) {
        TupleMarshaller<decltype(t), sizeof...(Args)>::marshall(*this, t);
        return *this;
    }

    Message &operator>>(uint8_t &i);
    Message &operator>>(bool &b);
    Message &operator>>(int16_t &i);
    Message &operator>>(uint16_t &i);
    Message &operator>>(int32_t &i);
    Message &operator>>(uint32_t &i);
    Message &operator>>(int64_t &i);
    Message &operator>>(uint64_t &i);
    Message &operator>>(double &d);
    Message &operator>>(std::string &s);
    Message &operator>>(ObjectPath &o);
    Message &operator>>(Signature &s);
    Message &operator>>(UnixFD &fd);
    Message &operator>>(const Container &c);
    Message &operator>>(const ContainerEnd &c);

    template <typename... Args>
    Message &operator>>(std::tuple<Args...> &t) {
        TupleMarshaller<decltype(t), sizeof...(Args)>::unmarshall(*this, t);
        return *this;
    }

private:
    std::unique_ptr<MessagePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Message);
};
}
}

#endif // _FCITX_UTILS_DBUS_MESSAGE_H_
