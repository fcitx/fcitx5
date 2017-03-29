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
#include <fcitx-utils/dbus/message_details.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/tuplehelpers.h>
#include <fcitx-utils/unixfd.h>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace fcitx {

namespace dbus {

template <typename... Args>
struct DBusStruct : public std::tuple<Args...> {
    typedef std::tuple<Args...> tuple_type;

    DBusStruct() = default;

    DBusStruct(const tuple_type &other) : tuple_type(std::forward(other)) {}

    DBusStruct(tuple_type &&other)
        : tuple_type(std::forward<tuple_type>(other)) {}
};

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
    ObjectPath(const std::string &path = {}) : path_(path) {}

    const std::string &path() const { return path_; }

private:
    std::string path_;
};

class FCITXUTILS_EXPORT Signature {
public:
    Signature(const std::string &sig = {}) : sig_(sig) {}

    const std::string &sig() const { return sig_; }

private:
    std::string sig_;
};

class FCITXUTILS_EXPORT Container {
public:
    enum class Type { Array, DictEntry, Struct, Variant };

    Container(Type t = Type::Array, const Signature &content = Signature())
        : type_(t), content_(content) {}

    Type type() const { return type_; }
    const Signature &content() const { return content_; }

private:
    Type type_;
    Signature content_;
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
    static void marshall(Message &msg, const Tuple &t) {
        msg << std::get<0>(t);
    }
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

    Message(const Message &other);
    Message(Message &&other) noexcept;
    Message createReply() const;
    Message createError(const char *name, const char *message) const;

    MessageType type() const;
    inline bool isError() const { return type() == MessageType::Error; }

    std::string destination() const;
    void setDestination(const std::string &dest);

    std::string sender() const;

    std::string signature() const;

    void *nativeHandle() const;

    Message call(uint64_t usec);
    Slot *callAsync(uint64_t usec, MessageCallback callback);
    bool send();

    operator bool() const;
    bool end() const;

    void resetError();
    void rewind();

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

    template <typename K, typename V>
    Message &operator<<(const std::pair<K, V> &t) {
        if (!(*this)) {
            return *this;
        }
        *this << std::get<0>(t);
        if (!(*this)) {
            return *this;
        }
        *this << std::get<1>(t);
        return *this;
    }

    template <typename... Args>
    Message &operator<<(const std::tuple<Args...> &t) {
        TupleMarshaller<decltype(t), sizeof...(Args)>::marshall(*this, t);
        return *this;
    }

    template <typename... Args>
    Message &operator<<(const DBusStruct<Args...> &t) {
        typedef DBusStruct<Args...> value_type;
        typedef typename DBusContainerSignatureTraits<value_type>::signature
            signature;
        if (*this << Container(Container::Type::Struct,
                               Signature(signature::data()))) {
            ;
            TupleMarshaller<typename value_type::tuple_type, sizeof...(Args)>::
                marshall(
                    *this,
                    static_cast<const typename value_type::tuple_type &>(t));
            if (*this) {
                *this << ContainerEnd();
            }
        }
        return *this;
    }

    template <typename T>
    Message &operator<<(const std::vector<T> &t) {
        typedef std::vector<T> value_type;
        typedef typename DBusContainerSignatureTraits<value_type>::signature
            signature;
        if (*this << Container(Container::Type::Array,
                               Signature(signature::data()))) {
            ;
            for (auto &v : t) {
                *this << v;
            }
            *this << ContainerEnd();
        }
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

    template <typename K, typename V>
    Message &operator>>(std::pair<K, V> &t) {
        if (!(*this)) {
            return *this;
        }
        *this >> std::get<0>(t);
        if (!(*this)) {
            return *this;
        }
        *this >> std::get<1>(t);
        return *this;
    }

    template <typename... Args>
    Message &operator>>(std::tuple<Args...> &t) {
        TupleMarshaller<decltype(t), sizeof...(Args)>::unmarshall(*this, t);
        return *this;
    }

    template <typename... Args>
    Message &operator>>(DBusStruct<Args...> &t) {
        typedef DBusStruct<Args...> value_type;
        typedef typename value_type::tuple_type tuple_type;
        typedef typename DBusContainerSignatureTraits<value_type>::signature
            signature;
        if (*this >>
            Container(Container::Type::Struct, Signature(signature::data()))) {
            TupleMarshaller<tuple_type, sizeof...(Args)>::unmarshall(
                *this, static_cast<tuple_type &>(t));
            if (*this) {
                *this >> ContainerEnd();
            }
        }
        return *this;
    }

    template <typename T>
    Message &operator>>(std::vector<T> &t) {
        typedef std::vector<T> value_type;
        typedef typename DBusContainerSignatureTraits<value_type>::signature
            signature;
        if (*this >>
            Container(Container::Type::Array, Signature(signature::data()))) {
            ;
            T temp;
            while (!end() && *this >> temp) {
                t.push_back(temp);
            }
            *this >> ContainerEnd();
        }
        return *this;
    }

private:
    std::unique_ptr<MessagePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Message);
};
}
}

#endif // _FCITX_UTILS_DBUS_MESSAGE_H_
