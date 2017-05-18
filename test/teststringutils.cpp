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

#include "fcitx-utils/charutils.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/stringutils.h"
#include <cassert>
#include <tuple>

using namespace fcitx;

int main() {
    assert(stringutils::startsWith("abc", "ab"));
    assert(!stringutils::startsWith("abc", "abd"));
    assert(!stringutils::startsWith("abc", "abcd"));
    assert(!stringutils::endsWith("abc", "ab"));
    assert(stringutils::endsWith("abc", "abc"));
    assert(!stringutils::endsWith("abc", "eabc"));

    std::string trim = " ab c\td\n";
    size_t start, end;
    std::tie(start, end) = stringutils::trimInplace(trim);
    assert(start == 1);
    assert(end == 7);
    assert(trim.compare(start, end - start, "ab c\td") == 0);

    std::tie(start, end) = stringutils::trimInplace("\t\n\r ");
    assert(start == end);

    auto replace_result = stringutils::replaceAll("abcabc", "a", "b");
    assert(replace_result == "bbcbbc");

    {
#define REPEAT 2049
        char largeReplace[3 * REPEAT + 1];
        char largeReplaceCorrect[REPEAT + 1];
        char largeReplaceCorrect2[4 * REPEAT + 1];
        int i = 0, j = 0, k = 0;
        for (int n = 0; n < REPEAT; n++) {
            largeReplace[i++] = 'a';
            largeReplace[i++] = 'b';
            largeReplace[i++] = 'c';

            largeReplaceCorrect[j++] = 'e';

            largeReplaceCorrect2[k++] = 'a';
            largeReplaceCorrect2[k++] = 'b';
            largeReplaceCorrect2[k++] = 'c';
            largeReplaceCorrect2[k++] = 'd';
        }

        largeReplace[i] = '\0';
        largeReplaceCorrect[j] = '\0';
        largeReplaceCorrect2[k] = '\0';

        auto replace_result = stringutils::replaceAll(largeReplace, "abc", "e");
        assert(replace_result == largeReplaceCorrect);
        auto replace_result2 =
            stringutils::replaceAll(replace_result, "e", "abcd");
        assert(replace_result2 == largeReplaceCorrect2);
    }

    assert(stringutils::backwardSearch("abcabc", "bc", 0) == std::string::npos);
    assert(stringutils::backwardSearch("abcabc", "bc", 1) == 1);
    assert(stringutils::backwardSearch("abcabc", "bc", 2) == 1);
    assert(stringutils::backwardSearch("abcabc", "bc", 3) == 1);
    assert(stringutils::backwardSearch("abcabc", "bc", 4) == 4);
    assert(stringutils::backwardSearch("abcabc", "bc", 5) == 4);
    assert(stringutils::backwardSearch("abcabc", "bc", 6) == 4);
    assert(stringutils::backwardSearch("abcabc", "bc", 7) == std::string::npos);
    assert(stringutils::backwardSearch("abcabc", "bc", 8) == std::string::npos);

    assert((stringutils::split("a  b c", FCITX_WHITESPACE) ==
            std::vector<std::string>{"a", "b", "c"}));
    assert((stringutils::split("a  b ", FCITX_WHITESPACE) ==
            std::vector<std::string>{"a", "b"}));
    assert((stringutils::split(" ", FCITX_WHITESPACE) ==
            std::vector<std::string>{}));

    return 0;
}
