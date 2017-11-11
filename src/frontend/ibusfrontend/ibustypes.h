/*
* Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_FRONTEND_IBUSFRONTEND_IBUSTYPES_H_
#define _FCITX_FRONTEND_IBUSFRONTEND_IBUSTYPES_H_

#include "fcitx-utils/dbus/message.h"

namespace fcitx {

class IBusSerializable {
public:
    IBusSerializable() = default;

    void serializeTo(dbus::Message &argument) const;
    void deserializeFrom(const dbus::Message &argument);

    std::string name;
    Hash<std::string, dbus::Message> attachments;
};

class IBusAttribute : private IBusSerializable {
public:
    enum Type {
        Invalid = 0,
        Underline = 1,
        Foreground = 2,
        Background = 3,
    };

    enum Underline {
        UnderlineNone = 0,
        UnderlineSingle = 1,
        UnderlineDouble = 2,
        UnderlineLow = 3,
        UnderlineError = 4,
    };

    IBusAttribute();

    TextCharFormat format() const;

    void serializeTo(dbus::Message &argument) const;
    void deserializeFrom(const dbus::Message &argument);

    Type type;
    quint32 value;
    quint32 start;
    quint32 end;
};

class IBusAttributeList : private IBusSerializable {
public:
    IBusAttributeList();

    List<InputMethodEvent::Attribute> imAttributes() const;

    void serializeTo(dbus::Message &argument) const;
    void deserializeFrom(const dbus::Message &argument);

    Vector<IBusAttribute> attributes;
};

class IBusText : private IBusSerializable {
public:
    IBusText();

    void serializeTo(dbus::Message &argument) const;
    void deserializeFrom(const dbus::Message &argument);

    std::string text;
    IBusAttributeList attributes;
};

} // namespace fcitx

#endif // _FCITX_FRONTEND_IBUSFRONTEND_IBUSTYPES_H_
