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
 * License along with this library; see the  file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#include "fcitx-utils/dbus.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/metastring.h"
#include <cassert>

using namespace fcitx::dbus;
using namespace fcitx;

int main() {
    Bus bus(BusType::Session);

    static_assert(std::is_same<DBusSignatureToTuple<'i', 'u'>::type, std::tuple<int32_t, uint32_t>>::value,
                  "Type is not same");

    // interface name must has dot
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << 1;
        assert(msg.signature() == "i");
    }
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << DBusSignatureToTuple<'i', 'u'>::type(1, 2);
        assert(msg.signature() == "iu");
    }
    {
        auto msg = bus.createSignal("/test", "test.a.b.c", "test");
        msg << STRING_TO_DBUS_TUPLE("siud")("a", 1, 2, 3);
        assert(msg.signature() == "siud");
    }
    return 0;
}
