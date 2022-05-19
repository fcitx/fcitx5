/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <tuple>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/stringutils.h"

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

    std::string_view testView = stringutils::trimView("\t\n\r ");
    FCITX_ASSERT(testView.empty());

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
    FCITX_ASSERT(stringutils::split(" ", FCITX_WHITESPACE).empty());

    const char *p = "def";
    FCITX_ASSERT(stringutils::concat().empty());
    FCITX_ASSERT(stringutils::concat("abc", 1, p) == "abc1def");
    FCITX_ASSERT(stringutils::joinPath("/", 1, p) == "/1/def");
    FCITX_ASSERT(stringutils::joinPath("/abc", 1, p) == "/abc/1/def");
    FCITX_ASSERT(stringutils::joinPath("///abc", 1, p) == "///abc/1/def");
    FCITX_ASSERT(stringutils::joinPath("///abc") == "///abc");
    FCITX_ASSERT(stringutils::joinPath("///") == "///");
    FCITX_ASSERT(stringutils::joinPath("abc") == "abc");

    FCITX_ASSERT(stringutils::split(",dvorak", ",",
                                    stringutils::SplitBehavior::KeepEmpty) ==
                 (std::vector<std::string>{"", "dvorak"}));
    FCITX_ASSERT(stringutils::split(",dvorak,,", ",",
                                    stringutils::SplitBehavior::KeepEmpty) ==
                 (std::vector<std::string>{"", "dvorak", "", ""}));
    FCITX_ASSERT(stringutils::split("dvorak", ",",
                                    stringutils::SplitBehavior::KeepEmpty) ==
                 (std::vector<std::string>{"dvorak"}));

    FCITX_ASSERT(
        stringutils::split("", ",", stringutils::SplitBehavior::KeepEmpty) ==
        (std::vector<std::string>{""}));

    FCITX_ASSERT(stringutils::escapeForValue("\"") == R"("\"")");
    FCITX_ASSERT(stringutils::escapeForValue("abc") == R"(abc)");
    FCITX_ASSERT(stringutils::escapeForValue("ab\"c") == R"("ab\"c")");
    FCITX_ASSERT(stringutils::escapeForValue("a c") == R"("a c")");

    return 0;
}
