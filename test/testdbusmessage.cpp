/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <unistd.h>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"

using namespace fcitx::dbus;
using namespace fcitx;

int main() {
    Bus bus(BusType::Session);

    static_assert(std::is_same<DBusSignatureToType<'i', 'u'>::type,
                               std::tuple<int32_t, uint32_t>>::value,
                  "Type is not same");
    static_assert(std::is_same<DBusSignatureToType<'i'>::type, int32_t>::value,
                  "Type is not same");
    static_assert(std::is_same<DBusSignatureToType<'a', 'u'>::type,
                               std::vector<uint32_t>>::value,
                  "Type is not same");
    static_assert(
        std::is_same<DBusSignatureToType<'a', '(', 'i', 'u', ')'>::type,
                     std::vector<DBusStruct<int32_t, uint32_t>>>::value,
        "Type is not same");
    static_assert(
        std::is_same<
            DBusSignatureToType<'a', 'i', 'a', '(', 'i', 'u', ')'>::type,
            std::tuple<std::vector<int32_t>,
                       std::vector<DBusStruct<int32_t, uint32_t>>>>::value,
        "Type is not same");
    static_assert(
        std::is_same<DBusSignatureToType<'a', 'i', '(', 'i', 'u', ')'>::type,
                     std::tuple<std::vector<int32_t>,
                                DBusStruct<int32_t, uint32_t>>>::value,
        "Type is not same");

    static_assert(std::is_same<DBusSignatureToType<'(', 'i', 'i', ')'>::type,
                               dbus::DBusStruct<int32_t, int32_t>>::value,
                  "Type is not same");

    static_assert(
        std::is_same<
            DBusSignatureToType<'(', 'i', ')', '(', 'i', ')'>::type,
            std::tuple<DBusStruct<int32_t>, DBusStruct<int32_t>>>::value,
        "Type is not same");

    using AtSpiObjectReference =
        dbus::DBusStruct<std::string, dbus::ObjectPath>;
    static_assert(
        std::is_same<
            FCITX_STRING_TO_DBUS_TYPE("((so)(so)(so)iiassusau)"),
            dbus::DBusStruct<AtSpiObjectReference, AtSpiObjectReference,
                             AtSpiObjectReference, int32_t, int32_t,
                             std::vector<std::string>, std::string, uint32_t,
                             std::string, std::vector<uint32_t>>>::value,
        "Type is not same");

    // interface name must has dot
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << 1;
        FCITX_ASSERT(msg.signature() == "i");
    }
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << DBusSignatureToType<'i', 'u'>::type(1, 2);
        FCITX_ASSERT(msg.signature() == "iu");
    }
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        auto data = FCITX_STRING_TO_DBUS_TUPLE("siud")("a", 1, 2, 3);
        msg << data;
        FCITX_ASSERT(msg.signature() == "siud");
        FCITX_INFO() << data;
    }
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        auto data = FCITX_STRING_TO_DBUS_TUPLE("as")(
            std::vector<std::string>{"a", "b"});
        msg << data;
        FCITX_ASSERT(msg.signature() == "as");
        FCITX_INFO() << data;
    }
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        auto data = FCITX_STRING_TO_DBUS_TUPLE("a(ss)")(
            std::vector<DBusStruct<std::string, std::string>>{{"a", "a"},
                                                              {"b", "b"}});
        msg << data;
        FCITX_ASSERT(msg.signature() == "a(ss)");
        FCITX_INFO() << data;
    }
    {
        bool b = false;
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << b;
        FCITX_INFO() << msg.signature();
        FCITX_ASSERT(msg.signature() == "b");
    }
    {
        dbus::Variant var;
        FCITX_INFO() << var;

        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        var.setData("abc");
        msg << var;
        FCITX_INFO() << msg.signature();
        FCITX_ASSERT(msg.signature() == "v");
        FCITX_INFO() << var;
    }
    {
        UnixFD fd = UnixFD::own(STDIN_FILENO);
        FCITX_INFO() << fd;

        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << fd;
        FCITX_INFO() << msg.signature();
        FCITX_ASSERT(msg.signature() == "h");
        FCITX_INFO() << fd;
    }

    // Test Copy
    {
        dbus::Variant var;
        var.setData("abcd");
        dbus::Variant var2(var);
        FCITX_INFO() << var;
        FCITX_INFO() << var2;
    }

#if 0
    // Compile fail, because there is redundant tuple.
    dbus::VariantTypeRegistry::defaultRegistry()
        .registerType<std::tuple<std::tuple<std::string, int>>>();
#endif

    return 0;
}
