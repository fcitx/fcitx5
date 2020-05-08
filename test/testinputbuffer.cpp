/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/log.h"

void test_basic(bool ascii) {
    using namespace fcitx;
    InputBuffer buffer(InputBufferOptions(ascii ? InputBufferOption::AsciiOnly
                                                : InputBufferOption::None));
    FCITX_ASSERT(buffer.size() == 0);
    FCITX_ASSERT(buffer.cursor() == 0);
    FCITX_ASSERT(buffer.cursorByChar() == 0);
    buffer.type('a');
    FCITX_ASSERT(buffer.size() == 1);
    FCITX_ASSERT(buffer.cursor() == 1);
    buffer.type('b');
    FCITX_ASSERT(buffer.size() == 2);
    FCITX_ASSERT(buffer.cursor() == 2);
    FCITX_ASSERT(buffer.userInput() == "ab");
    buffer.setCursor(1);
    buffer.type("cdefg");
    FCITX_ASSERT(buffer.size() == 7);
    FCITX_ASSERT(buffer.cursor() == 6);
    FCITX_ASSERT(buffer.userInput() == "acdefgb");
    buffer.erase(1, 3);
    FCITX_ASSERT(buffer.size() == 5);
    FCITX_ASSERT(buffer.cursor() == 4);
    FCITX_ASSERT(buffer.userInput() == "aefgb");
    FCITX_ASSERT(buffer.charAt(2) == 'f');
    buffer.erase(2, 5);
    FCITX_ASSERT(buffer.size() == 2);
    FCITX_ASSERT(buffer.cursor() == 2);
}

void test_utf8() {
    using namespace fcitx;
    InputBuffer buffer;
    buffer.type("\xe4\xbd\xa0\xe5\xa5\xbd");
    FCITX_ASSERT(buffer.size() == 2);
    FCITX_ASSERT(buffer.cursor() == 2);
    buffer.erase(1, 2);
    FCITX_ASSERT(buffer.size() == 1);
    FCITX_ASSERT(buffer.cursor() == 1);
    FCITX_ASSERT(buffer.userInput() == "\xe4\xbd\xa0");
    bool throwed = false;
    try {
        buffer.type("\xe4\xbd");
    } catch (const std::invalid_argument &e) {
        throwed = true;
    }
    FCITX_ASSERT(throwed);
    buffer.type("a\xe5\x95\x8a");
    FCITX_ASSERT(buffer.size() == 3);
    FCITX_ASSERT(buffer.cursor() == 3);
    FCITX_ASSERT(buffer.cursorByChar() == 7);
    buffer.setCursor(0);
    FCITX_ASSERT(buffer.cursorByChar() == 0);
    buffer.setCursor(1);
    FCITX_ASSERT(buffer.cursorByChar() == 3);
    buffer.setCursor(2);
    FCITX_ASSERT(buffer.cursorByChar() == 4);
    buffer.clear();
    FCITX_ASSERT(buffer.cursorByChar() == 0);
    FCITX_ASSERT(buffer.cursor() == 0);
    FCITX_ASSERT(buffer.size() == 0);

    buffer.type('a');
    FCITX_ASSERT(buffer.userInput() == "a");
    buffer.type('b');
    FCITX_ASSERT(buffer.userInput() == "ab");
    buffer.type('c');
    FCITX_ASSERT(buffer.userInput() == "abc");
    buffer.type('d');
    FCITX_ASSERT(buffer.userInput() == "abcd");
    buffer.type('e');
    FCITX_ASSERT(buffer.userInput() == "abcde");
    buffer.type('f');
    FCITX_ASSERT(buffer.userInput() == "abcdef");
    buffer.type('g');
    FCITX_ASSERT(buffer.userInput() == "abcdefg");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "abcdef");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "abcde");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "abcd");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "abc");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "ab");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "a");
    buffer.backspace();
    FCITX_ASSERT(buffer.userInput() == "");
}

int main() {
    test_basic(true);
    test_basic(false);
    test_utf8();
    return 0;
}
