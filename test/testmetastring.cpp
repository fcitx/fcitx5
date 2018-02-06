//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include <string>
#include <type_traits>

int main() {
    ::fcitx::MetaStringTrim<'A', 'B'>::type a;
    static_assert(a.size() == 2, "");
    ::fcitx::MetaStringTrim<'A', 'B', '\0', 'C'>::type b;
    static_assert(b.size() == 2, "");
    ::fcitx::MetaStringTrim<'\0'>::type c;
    static_assert(c.size() == 0, "");
    static_assert(std::is_same<::fcitx::MetaStringTrim<'A'>::type,
                               fcitx::MetaString<'A'>>::value,
                  "");
    static_assert(
        std::is_same<fcitxMakeMetaString("ABCDEF"),
                     fcitx::MetaString<'A', 'B', 'C', 'D', 'E', 'F'>>::value,
        "");
    auto test = fcitxMakeMetaString("ABCD")::data() == std::string("ABCD");
    FCITX_ASSERT(test);

    using StringABCD = fcitxMakeMetaString("ABCD");

    static_assert(
        std::is_same<StringABCD, fcitxMakeMetaString(StringABCD::str())>::value,
        "");

    static_assert(
        std::is_same<fcitx::MetaStringBasenameType<fcitxMakeMetaString("/abc")>,
                     fcitxMakeMetaString("abc")>::value,
        "");

    static_assert(
        std::is_same<
            fcitx::MetaStringBasenameType<fcitxMakeMetaString("abc/def")>,
            fcitxMakeMetaString("def")>::value,
        "");

    static_assert(
        std::is_same<
            fcitx::MetaStringBasenameType<fcitxMakeMetaString("//abc///def")>,
            fcitxMakeMetaString("def")>::value,
        "");

    static_assert(
        std::is_same<fcitx::MetaStringBasenameType<fcitxMakeMetaString("")>,
                     fcitxMakeMetaString("")>::value,
        "");

    static_assert(
        std::is_same<fcitx::MetaStringBasenameType<fcitxMakeMetaString("1")>,
                     fcitxMakeMetaString("1")>::value,
        "");

    return 0;
}
