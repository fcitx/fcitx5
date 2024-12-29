/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_MESSAGE_H_
#define _FCITX_UTILS_DBUS_MESSAGE_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <fcitx-utils/dbus/message_details.h> // IWYU pragma: export
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/tuplehelpers.h>
#include <fcitx-utils/unixfd.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief API for DBus message.

namespace fcitx::dbus {

class Message;
class Variant;

/**
 * A type to represent DBus struct.
 *
 * It is used for message serialization.
 */
template <typename... Args>
struct DBusStruct {
    using tuple_type = std::tuple<Args...>;

    DBusStruct() = default;

    template <
        typename Element, typename... Elements,
        typename = typename std::enable_if_t<
            sizeof...(Elements) != 0 ||
            !std::is_same<typename std::decay_t<Element>, DBusStruct>::value>>
    DBusStruct(Element &&ele, Elements &&...elements)
        : data_(std::forward<Element>(ele),
                std::forward<Elements>(elements)...) {}

    DBusStruct(const DBusStruct &) = default;
    DBusStruct(DBusStruct &&) noexcept(
        std::is_nothrow_move_constructible<tuple_type>::value) = default;
    DBusStruct &operator=(const DBusStruct &other) = default;
    DBusStruct &operator=(DBusStruct &&other) noexcept(
        std::is_nothrow_move_assignable<tuple_type>::value) = default;

    explicit DBusStruct(const tuple_type &other) : data_(std::forward(other)) {}
    explicit DBusStruct(tuple_type &&other)
        : data_(std::forward<tuple_type>(other)) {}

    constexpr tuple_type &data() { return data_; }
    constexpr const tuple_type &data() const { return data_; }

private:
    tuple_type data_;
};

struct FCITXUTILS_EXPORT VariantHelperBase {
public:
    virtual ~VariantHelperBase() = default;
    virtual std::shared_ptr<void> copy(const void *) const = 0;
    virtual void serialize(dbus::Message &msg, const void *data) const = 0;
    virtual void print(LogMessageBuilder &builder, const void *data) const = 0;
    virtual void deserialize(dbus::Message &msg, void *data) const = 0;
    virtual std::string signature() const = 0;
};

template <typename Value>
class FCITXUTILS_EXPORT VariantHelper : public VariantHelperBase {
    std::shared_ptr<void> copy(const void *src) const override {
        if (src) {
            auto *s = static_cast<const Value *>(src);
            return std::make_shared<Value>(*s);
        }
        return std::make_shared<Value>();
    }
    void serialize(dbus::Message &msg, const void *data) const override {
        auto *s = static_cast<const Value *>(data);
        msg << *s;
    }
    void deserialize(dbus::Message &msg, void *data) const override {
        auto *s = static_cast<Value *>(data);
        msg >> *s;
    }
    void print(LogMessageBuilder &builder, const void *data) const override {
        auto *s = static_cast<const Value *>(data);
        builder << *s;
    }
    std::string signature() const override {
        return DBusSignatureTraits<Value>::signature::data();
    }
};

/**
 * A type to represent DBus dict entry.
 *
 * It is used for message serialization for type like a{sv}.
 */
template <typename Key, typename Value>
class DictEntry {
public:
    DictEntry() = default;
    DictEntry(const DictEntry &) = default;
    DictEntry(DictEntry &&) noexcept = default;
    DictEntry &operator=(const DictEntry &other) = default;
    DictEntry &operator=(DictEntry &&other) noexcept = default;

    DictEntry(const Key &key, const Value &value) : key_(key), value_(value) {}

    constexpr Key &key() { return key_; }
    constexpr const Key &key() const { return key_; }
    constexpr Value &value() { return value_; }
    constexpr const Value &value() const { return value_; }

private:
    Key key_;
    Value value_;
};

class Message;
using MessageCallback = std::function<bool(Message &)>;
class Slot;

enum class MessageType {
    Invalid,
    Signal,
    MethodCall,
    Reply,
    Error,
};

/**
 * String like type object path 'o'
 */
class FCITXUTILS_EXPORT ObjectPath {
public:
    ObjectPath(const std::string &path = {}) : path_(path) {}

    const std::string &path() const { return path_; }

private:
    std::string path_;
};

/**
 * String like type object signature 'g'
 */
class FCITXUTILS_EXPORT Signature {
public:
    Signature(const std::string &sig = {}) : sig_(sig) {}

    const std::string &sig() const { return sig_; }

private:
    std::string sig_;
};

/// Helper type for serialization, should not be used directly.
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

/// Helper type for serialization, should not be used directly.
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
    static void marshall(Message & /*unused*/, const Tuple & /*unused*/) {}
    static void unmarshall(Message & /*unused*/, Tuple & /*unused*/) {}
};

/**
 * Basic DBus type of a DBus message.
 */
class FCITXUTILS_EXPORT Message {
    friend class Bus;

public:
    Message();

    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(Message);
    /// Create a reply to this message.
    Message createReply() const;
    /// Create a error reply to this message.
    Message createError(const char *name, const char *message) const;

    /// Return the message type.
    MessageType type() const;

    /// Check if the message is error.
    inline bool isError() const { return type() == MessageType::Error; }

    /// Return the destination of the message.
    std::string destination() const;

    /**
     * Set the destination of the message.
     *
     * Should only be used on message to send.
     *
     * @param dest destination
     */
    void setDestination(const std::string &dest);

    /// Return the sender of the message.
    std::string sender() const;

    /// Return the member of the message.
    std::string member() const;

    /// Return the interface of the message.
    std::string interface() const;

    /// Return the signature of the message
    std::string signature() const;

    /**
     * Return the error name of the message.
     *
     * Should only be used when message is a received error.
     *
     * @return DBus Error type
     */
    std::string errorName() const;

    /**
     * Return the error message of the message.
     *
     * Should only be used when message is a received error.
     *
     * @return DBus Error type
     */
    std::string errorMessage() const;

    /// Return the path of the message.
    std::string path() const;

    /**
     * Return the low level internal pointer of the message.
     *
     * @see dbus::Bus::impl
     *
     * @return internal pointer
     */
    void *nativeHandle() const;

    /// Synchronously call a dbus method with a timeout in microseconds.
    Message call(uint64_t usec);

    /**
     * Asynchronously call a dbus method with a timeout in microseconds.
     *
     * @param usec timeout
     * @param callback Callback function if anything happens.
     */
    std::unique_ptr<Slot> callAsync(uint64_t usec, MessageCallback callback);

    /// Send this message.
    bool send();

    /// Check if message is not empty and has no serialization error.
    operator bool() const;

    /// Check if message reaches end.
    bool end() const;

    /// Clear serialization error.
    void resetError();

    /// Rewind the message to the beginning.
    void rewind();

    /// Skip the next data.
    void skip();

    /// Check the next type of data in the message
    std::pair<char, std::string> peekType();

    Message &operator<<(uint8_t v);
    Message &operator<<(bool b);
    Message &operator<<(int16_t v);
    Message &operator<<(uint16_t v);
    Message &operator<<(int32_t v);
    Message &operator<<(uint32_t v);
    Message &operator<<(int64_t v);
    Message &operator<<(uint64_t v);
    Message &operator<<(double v);
    Message &operator<<(const std::string &s);
    Message &operator<<(const char *s);

    Message &operator<<(const ObjectPath &o);
    Message &operator<<(const Signature &s);
    Message &operator<<(const UnixFD &fd);
    Message &operator<<(const Container &c);
    Message &operator<<(const ContainerEnd &c);
    Message &operator<<(const Variant &v);

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
        using value_type = DBusStruct<Args...>;
        using signature =
            typename DBusContainerSignatureTraits<value_type>::signature;
        if (*this << Container(Container::Type::Struct,
                               Signature(signature::data()))) {
            TupleMarshaller<typename value_type::tuple_type,
                            sizeof...(Args)>::marshall(*this, t.data());
            if (*this) {
                *this << ContainerEnd();
            }
        }
        return *this;
    }

    template <typename Key, typename Value>
    Message &operator<<(const DictEntry<Key, Value> &t) {
        using value_type = DictEntry<Key, Value>;
        using signature =
            typename DBusContainerSignatureTraits<value_type>::signature;
        if (*this << Container(Container::Type::DictEntry,
                               Signature(signature::data()))) {
            *this << t.key();
            if (!(*this)) {
                return *this;
            }
            *this << t.value();
            if (!(*this)) {
                return *this;
            }
            if (*this) {
                *this << ContainerEnd();
            }
        }
        return *this;
    }

    template <typename T>
    Message &operator<<(const std::vector<T> &t) {
        using value_type = std::vector<T>;
        using signature =
            typename DBusContainerSignatureTraits<value_type>::signature;
        if (*this << Container(Container::Type::Array,
                               Signature(signature::data()))) {
            for (auto &v : t) {
                *this << v;
            }
            *this << ContainerEnd();
        }
        return *this;
    }

    Message &operator>>(uint8_t &v);
    Message &operator>>(bool &b);
    Message &operator>>(int16_t &v);
    Message &operator>>(uint16_t &v);
    Message &operator>>(int32_t &v);
    Message &operator>>(uint32_t &v);
    Message &operator>>(int64_t &v);
    Message &operator>>(uint64_t &v);
    Message &operator>>(double &v);
    Message &operator>>(std::string &s);
    Message &operator>>(ObjectPath &o);
    Message &operator>>(Signature &s);
    Message &operator>>(UnixFD &fd);
    Message &operator>>(const Container &c);
    Message &operator>>(const ContainerEnd &c);
    Message &operator>>(Variant &variant);

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
        using value_type = DBusStruct<Args...>;
        using tuple_type = typename value_type::tuple_type;
        using signature =
            typename DBusContainerSignatureTraits<value_type>::signature;
        if (*this >>
            Container(Container::Type::Struct, Signature(signature::data()))) {
            TupleMarshaller<tuple_type, sizeof...(Args)>::unmarshall(*this,
                                                                     t.data());
            if (*this) {
                *this >> ContainerEnd();
            }
        }
        return *this;
    }

    template <typename Key, typename Value>
    Message &operator>>(DictEntry<Key, Value> &t) {
        using value_type = DictEntry<Key, Value>;
        using signature =
            typename DBusContainerSignatureTraits<value_type>::signature;
        if (*this >> Container(Container::Type::DictEntry,
                               Signature(signature::data()))) {
            *this >> t.key();
            if (!(*this)) {
                return *this;
            }
            *this >> t.value();
            if (!(*this)) {
                return *this;
            }
            if (*this) {
                *this >> ContainerEnd();
            }
        }
        return *this;
    }

    template <typename T>
    Message &operator>>(std::vector<T> &t) {
        using value_type = std::vector<T>;
        using signature =
            typename DBusContainerSignatureTraits<value_type>::signature;
        if (*this >>
            Container(Container::Type::Array, Signature(signature::data()))) {
            t.clear();
            while (!end()) {
                T temp;
                if (*this >> temp) {
                    t.push_back(temp);
                } else {
                    break;
                }
            }
            *this >> ContainerEnd();
        }
        return *this;
    }

private:
    std::unique_ptr<MessagePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Message);
};

template <typename K, typename V>
inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                     const DictEntry<K, V> &entry) {
    builder << "(" << entry.key() << ", " << entry.value() << ")";
    return builder;
}

template <typename... Args>
inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                     const DBusStruct<Args...> &st) {
    builder << st.data();
    return builder;
}

static inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                            const Signature &sig) {
    builder << "Signature(" << sig.sig() << ")";
    return builder;
}

static inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                            const ObjectPath &path) {
    builder << "ObjectPath(" << path.path() << ")";
    return builder;
}

} // namespace fcitx::dbus

namespace std {

template <std::size_t i, typename... _Elements>
constexpr auto &get(fcitx::dbus::DBusStruct<_Elements...> &t) noexcept {
    return std::get<i>(t.data());
}

template <std::size_t i, typename... _Elements>
constexpr auto &get(const fcitx::dbus::DBusStruct<_Elements...> &t) noexcept {
    return std::get<i>(t.data());
}

template <typename T, typename... _Elements>
constexpr auto &get(fcitx::dbus::DBusStruct<_Elements...> &t) noexcept {
    return std::get<T>(t.data());
}

template <typename T, typename... _Elements>
constexpr auto &get(const fcitx::dbus::DBusStruct<_Elements...> &t) noexcept {
    return std::get<T>(t.data());
}
} // namespace std

#endif // _FCITX_UTILS_DBUS_MESSAGE_H_
