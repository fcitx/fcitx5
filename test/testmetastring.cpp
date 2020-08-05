/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <string>
#include <type_traits>
#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"

int main() {
    using a = ::fcitx::MetaStringTrim<'A', 'B'>::type;
    static_assert(a::size() == 2, "");
    using b = ::fcitx::MetaStringTrim<'A', 'B', '\0', 'C'>::type;
    static_assert(b::size() == 2, "");
    using c = ::fcitx::MetaStringTrim<'\0'>::type;
    static_assert(c::empty(), "");
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
