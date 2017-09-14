/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "fcitx-utils/charutils.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/stringutils.h"
#include <tuple>

using namespace fcitx;

int main() {
    FCITX_ASSERT(stringutils::startsWith("abc", "ab"));
    FCITX_ASSERT(!stringutils::startsWith("abc", "abd"));
    FCITX_ASSERT(!stringutils::startsWith("abc", "abcd"));
    FCITX_ASSERT(!stringutils::endsWith("abc", "ab"));
    FCITX_ASSERT(stringutils::endsWith("abc", "abc"));
    FCITX_ASSERT(!stringutils::endsWith("abc", "eabc"));

    std::string trim = " ab c\td\n";
    auto pair = stringutils::trimInplace(trim);
    auto start = pair.first, end = pair.second;
    FCITX_ASSERT(start == 1);
    FCITX_ASSERT(end == 7);
    FCITX_ASSERT(trim.compare(start, end - start, "ab c\td") == 0);

    pair = stringutils::trimInplace("\t\n\r ");
    start = pair.first;
    end = pair.second;
    FCITX_ASSERT(start == end);

    auto replace_result = stringutils::replaceAll("abcabc", "a", "b");
    FCITX_ASSERT(replace_result == "bbcbbc");

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
        FCITX_ASSERT(replace_result == largeReplaceCorrect);
        auto replace_result2 =
            stringutils::replaceAll(replace_result, "e", "abcd");
        FCITX_ASSERT(replace_result2 == largeReplaceCorrect2);
    }

    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 0) ==
                 std::string::npos);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 1) == 1);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 2) == 1);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 3) == 1);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 4) == 4);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 5) == 4);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 6) == 4);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 7) ==
                 std::string::npos);
    FCITX_ASSERT(stringutils::backwardSearch("abcabc", "bc", 8) ==
                 std::string::npos);

    FCITX_ASSERT((stringutils::split("a  b c", FCITX_WHITESPACE) ==
                  std::vector<std::string>{"a", "b", "c"}));
    FCITX_ASSERT((stringutils::split("a  b ", FCITX_WHITESPACE) ==
                  std::vector<std::string>{"a", "b"}));
    FCITX_ASSERT((stringutils::split(" ", FCITX_WHITESPACE) ==
                  std::vector<std::string>{}));

    return 0;
}
