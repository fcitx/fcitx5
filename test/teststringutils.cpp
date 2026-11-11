/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <ranges>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/stringutils.h"

using namespace fcitx;

namespace {

void testBasic() {
    FCITX_ASSERT(stringutils::startsWith("abc", "ab"));
    FCITX_ASSERT(!stringutils::startsWith("abc", "abd"));
    FCITX_ASSERT(!stringutils::startsWith("abc", "abcd"));
    FCITX_ASSERT(stringutils::startsWith("abc", ""));
    FCITX_ASSERT(!stringutils::endsWith("abc", "ab"));
    FCITX_ASSERT(stringutils::endsWith("abc", "abc"));
    FCITX_ASSERT(!stringutils::endsWith("abc", "eabc"));

    std::string trim = " ab c\td\n";
    auto pair = stringutils::trimInplace(trim);
    auto start = pair.first;
    auto end = pair.second;
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
        constexpr int REPEAT = 1000;
        char largeReplace[(3 * REPEAT) + 1];
        char largeReplaceCorrect[REPEAT + 1];
        char largeReplaceCorrect2[(4 * REPEAT) + 1];
        int i = 0;
        int j = 0;
        int k = 0;
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

    FCITX_ASSERT(stringutils::replaceAll("ababba", "a", "cc") == "ccbccbbcc");
    FCITX_ASSERT(stringutils::replaceAll("\"a\nb\"", "\"", "\\\"") ==
                 "\\\"a\nb\\\"");

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

    for (int i = 0; i < 16; i++) {
        if (i < 10) {
            FCITX_ASSERT(charutils::toHex(i) == i + '0');
        } else {
            FCITX_ASSERT(charutils::toHex(i) == i - 10 + 'a');
        }
    }

    std::string_view str = "abc";
    FCITX_ASSERT(!stringutils::consumePrefix(str, "ae"));
    FCITX_ASSERT(str == "abc");
    FCITX_ASSERT(stringutils::consumePrefix(str, "ab"));
    FCITX_ASSERT(str == "c");
}

void testSplitView() {
    using stringutils::SplitBehavior;
    using SkipView = decltype(stringutils::splitView("", ""));
    using KeepView =
        decltype(stringutils::splitView<SplitBehavior::KeepEmpty>("", ""));
    static_assert(std::ranges::view<SkipView>);
    static_assert(std::ranges::forward_range<SkipView>);
    static_assert(std::ranges::forward_range<const SkipView>);
    static_assert(std::ranges::view<KeepView>);
    static_assert(std::ranges::forward_range<KeepView>);
    static_assert(std::ranges::forward_range<const KeepView>);
    static_assert(std::ranges::borrowed_range<SkipView>);
    static_assert(std::ranges::borrowed_range<KeepView>);
    static_assert([] {
        const auto view = stringutils::splitView("a,,b", ",");
        if (view.empty() || view.front() != "a") {
            return false;
        }
        auto it = view.begin();
        if (*it++ != "a" || *it++ != "b") {
            return false;
        }
        return it == view.end();
    }());

    struct {
        std::string_view input;
        std::string_view delim;
        std::vector<std::string_view> skipEmpty;
        std::vector<std::string_view> keepEmpty;
    } cases[] = {
        {"a  b c", FCITX_WHITESPACE, {"a", "b", "c"}, {"a", "", "b", "c"}},
        {",dvorak,,", ",", {"dvorak"}, {"", "dvorak", "", ""}},
        {"abc", ",", {"abc"}, {"abc"}},
        {"", ",", {}, {""}},
        {std::string_view{}, ",", {}, {""}},
        {",,", ",", {}, {"", "", ""}},
        {"abc", "", {"abc"}, {"abc"}},
        {"", "", {}, {""}},
        {std::string_view{}, std::string_view{}, {}, {""}},
        {"abc", std::string_view{}, {"abc"}, {"abc"}},
        {"a,b;c", ",;", {"a", "b", "c"}, {"a", "b", "c"}},
        {std::string_view("a\0b,c", 5),
         std::string_view("\0,", 2),
         {"a", "b", "c"},
         {"a", "b", "c"}},
    };

    for (const auto &c : cases) {
        const auto skip = stringutils::splitView(c.input, c.delim);
        const auto keep =
            stringutils::splitView<SplitBehavior::KeepEmpty>(c.input, c.delim);
        FCITX_ASSERT(std::ranges::equal(skip, c.skipEmpty));
        FCITX_ASSERT(std::ranges::equal(keep, c.keepEmpty));
        FCITX_ASSERT(std::ranges::equal(
            skip,
            stringutils::split(c.input, c.delim, SplitBehavior::SkipEmpty)));
        FCITX_ASSERT(std::ranges::equal(
            keep,
            stringutils::split(c.input, c.delim, SplitBehavior::KeepEmpty)));

        std::vector<std::string_view> fields;
        for (auto part : skip) {
            fields.push_back(part);
        }
        FCITX_ASSERT(fields == c.skipEmpty);
        FCITX_ASSERT(std::ranges::equal(skip, fields));
    }

    std::string input = "_a,b_";
    const auto views =
        stringutils::splitView(std::string_view(input).substr(1, 3), ",");
    auto first = views.begin();
    auto copy = first;
    auto previous = first++;
    FCITX_ASSERT(previous == copy);
    FCITX_ASSERT(first != copy);
    FCITX_ASSERT(*copy == "a");
    FCITX_ASSERT((*copy).data() == input.data() + 1);
    FCITX_ASSERT(*first == "b");
    FCITX_ASSERT((*first).data() == input.data() + 3);
    ++first;
    FCITX_ASSERT(first == views.end());
    FCITX_ASSERT(views.end() == first);
    FCITX_ASSERT(first == SkipView::Iterator{});
    FCITX_ASSERT(SkipView::Iterator{} == SkipView::Iterator{});
    FCITX_ASSERT(SkipView::Iterator{} == views.end());
    FCITX_ASSERT(*copy == "a");
    input[1] = 'c';
    FCITX_ASSERT(*copy == "c");

    auto toMove = stringutils::splitView("a,b", ",");
    auto beforeMove = toMove.begin();
    auto moved = std::move(toMove);
    FCITX_ASSERT(*beforeMove == "a");
    ++beforeMove;
    FCITX_ASSERT(*beforeMove == "b");
    FCITX_ASSERT(
        std::ranges::equal(moved, std::vector<std::string_view>{"a", "b"}));
    auto rvalueIt = stringutils::splitView("x=y", "=").begin();
    FCITX_ASSERT(*rvalueIt == "x");
    auto foundIt = std::ranges::find(stringutils::splitView("a,b,c", ","), "b");
    FCITX_ASSERT(*foundIt == "b");
    FCITX_ASSERT(stringutils::join(stringutils::splitView("a,,b,c", ","),
                                   "-") == "a-b-c");
    FCITX_ASSERT(std::ranges::equal(stringutils::splitView("a,,b,c", ",") |
                                        std::views::drop(1) |
                                        std::views::take(1),
                                    std::vector<std::string_view>{"b"}));

    int count = 0;
    for (auto part : stringutils::splitView(",a,b,", ",")) {
        FCITX_ASSERT(part == "a");
        ++count;
        break;
    }
    FCITX_ASSERT(count == 1);
}

void testEscape() {

    FCITX_ASSERT(stringutils::escapeForValue("\"") == R"("\"")");
    FCITX_ASSERT(stringutils::escapeForValue("\"\"\n") == R"("\"\"\n")");
    FCITX_ASSERT(stringutils::escapeForValue("abc") == R"(abc)");
    FCITX_ASSERT(stringutils::escapeForValue("ab\"c") == R"("ab\"c")");
    FCITX_ASSERT(stringutils::escapeForValue("a c") == R"("a c")");

    FCITX_ASSERT(stringutils::unescapeForValue("\"") == "\"");
    FCITX_ASSERT(stringutils::unescapeForValue("\"\"\n") == "\"\"\n");
    FCITX_ASSERT(stringutils::unescapeForValue("abc") == "abc");
    FCITX_ASSERT(stringutils::unescapeForValue("\"ab\\\"c\"") == "ab\"c");
    FCITX_ASSERT(stringutils::unescapeForValue("a c") == "a c");
    FCITX_ASSERT(stringutils::unescapeForValue("\"工 \"") == "工 ");
    FCITX_ASSERT(stringutils::unescapeForValue("\"abc\"") == "abc");
    FCITX_ASSERT(stringutils::unescapeForValue(R"("\r\v\f\n\t\\\" ")") ==
                 "\r\v\f\n\t\\\" ");

    struct {
        std::string_view input;
        std::string expected;
        std::string expectedInput;
        std::string expectedOutput;
    } cases[] = {
        {.input = "",
         .expected = "",
         .expectedInput = "",
         .expectedOutput = ""},
        {.input = "abc",
         .expected = "abc",
         .expectedInput = "",
         .expectedOutput = "abc"},
        {.input = "ab\"c",
         .expected = "ab\"c",
         .expectedInput = "",
         .expectedOutput = "ab\"c"},
        {.input = "a c",
         .expected = "a",
         .expectedInput = " c",
         .expectedOutput = "a"},
        {.input = " a ",
         .expected = "a",
         .expectedInput = " ",
         .expectedOutput = "a"},
        {.input = R"(  "a"   )",
         .expected = R"("a")",
         .expectedInput = R"(   )",
         .expectedOutput = "a"},
        {.input = R"(  "a b\n"   )",
         .expected = R"("a b\n")",
         .expectedInput = R"(   )",
         .expectedOutput = "a b\n"},
        {.input = R"(  "a\"b\n"  c)",
         .expected = R"("a\"b\n")",
         .expectedInput = R"(  c)",
         .expectedOutput = "a\"b\n"},
        {.input = R"(  "a\\b\n"  c)",
         .expected = R"("a\\b\n")",
         .expectedInput = R"(  c)",
         .expectedOutput = "a\\b\n"},
        {.input = R"(  "aaa c)",
         .expected = R"("aaa)",
         .expectedInput = R"( c)",
         .expectedOutput = "\"aaa"},
        {.input = R"("")",
         .expected = R"("")",
         .expectedInput = "",
         .expectedOutput = ""},
        {.input = " \t   ",
         .expected = "",
         .expectedInput = "",
         .expectedOutput = ""},
        {.input = "\" “ ”",
         .expected = "\"",
         .expectedInput = " “ ”",
         .expectedOutput = "\""},
    };

    for (const auto &c : cases) {
        std::string_view input = c.input;
        FCITX_ASSERT(stringutils::consumeMaybeEscapedValue(
                         input, FCITX_WHITESPACE, nullptr) == c.expected)
            << "expected: " << c.expected;
        FCITX_ASSERT(input == c.expectedInput)
            << "expected input: \"" << c.expectedInput << "\", got: \"" << input
            << "\"";

        input = c.input;
        std::string output;
        FCITX_ASSERT(stringutils::consumeMaybeEscapedValue(
                         input, FCITX_WHITESPACE, &output) == c.expected)
            << "expected: " << c.expected;
        FCITX_ASSERT(output == c.expectedOutput)
            << "expected: \"" << c.expectedOutput << "\", got: \"" << output
            << "\"";
    }
}

} // namespace

int main() {
    testBasic();
    testSplitView();
    testEscape();
    return 0;
}
