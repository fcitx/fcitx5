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
#include "macros.h"

namespace fcitx
{

namespace dbus
{



enum class MessageType
{
    Invalid,
    Signal,
    MethodCall,
    Reply,
    Error,
};


class FCITXUTILS_EXPORT ObjectPath
{
public:
    ObjectPath(const std::string &path = std::string()) : m_path(path) { }

    const std::string &path() const { return m_path; }
private:
    std::string m_path;
};

class FCITXUTILS_EXPORT Signature
{
public:
    Signature(const std::string &sig = std::string()) : m_sig(sig) { }

    const std::string &sig() const { return m_sig; }

private:
    std::string m_sig;
};

class UnixFDPrivate;

class FCITXUTILS_EXPORT UnixFD
{
    friend class Message;
public:
    UnixFD(int fd = -1);
    UnixFD(const UnixFD & other);
    UnixFD(UnixFD &&other);
    ~UnixFD();

    UnixFD& operator=(UnixFD other);

    bool isValid() const;
    void set(int fd);
    int release();
    int fd() const;
private:
    void give(int fd);
    std::shared_ptr<UnixFDPrivate> d;
};

class FCITXUTILS_EXPORT Container
{
public:
    enum class Type {
        Array,
        DictEntry,
        Struct,
    };

    Container (Type t = Type::Array, const Signature &content = Signature()) : m_type(t), m_content(content) {
    }

    Type type() const { return m_type; }
    const Signature& content() const { return m_content; }

private:
    Type m_type;
    Signature m_content;
};

class FCITXUTILS_EXPORT ContainerEnd { };

class MessagePrivate;

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

    void *nativeHandle() const;

    Message &operator <<(uint8_t i);
    Message &operator <<(bool b);
    Message &operator <<(int16_t i);
    Message &operator <<(uint16_t i);
    Message &operator <<(int32_t i);
    Message &operator <<(uint32_t i);
    Message &operator <<(int64_t i);
    Message &operator <<(uint64_t i);
    Message &operator <<(double d);
    Message &operator <<(const std::string &s);
    Message &operator <<(const ObjectPath &o);
    Message &operator <<(const Signature &s);
    Message &operator <<(const UnixFD &fd);
    Message &operator <<(const Container &c);
    Message &operator <<(const ContainerEnd &c);

    Message &operator >>(uint8_t &i);
    Message &operator >>(bool &b);
    Message &operator >>(int16_t &i);
    Message &operator >>(uint16_t &i);
    Message &operator >>(int32_t &i);
    Message &operator >>(uint32_t &i);
    Message &operator >>(int64_t &i);
    Message &operator >>(uint64_t &i);
    Message &operator >>(double &d);
    Message &operator >>(std::string &s);
    Message &operator >>(ObjectPath &o);
    Message &operator >>(Signature &s);
    Message &operator >>(UnixFD &fd);
    Message &operator >>(const Container &c);
    Message &operator >>(const ContainerEnd &c);


private:
    std::unique_ptr<MessagePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Message);
};

}

}

#endif // _FCITX_UTILS_DBUS_MESSAGE_H_
