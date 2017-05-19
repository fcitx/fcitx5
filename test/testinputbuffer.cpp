/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#include "fcitx-utils/inputbuffer.h"
#include <cassert>

void test_basic(bool ascii) {
    using namespace fcitx;
    InputBuffer buffer(ascii);
    assert(buffer.size() == 0);
    assert(buffer.cursor() == 0);
    assert(buffer.cursorByChar() == 0);
    buffer.type('a');
    assert(buffer.size() == 1);
    assert(buffer.cursor() == 1);
    buffer.type('b');
    assert(buffer.size() == 2);
    assert(buffer.cursor() == 2);
    assert(buffer.userInput() == "ab");
    buffer.setCursor(1);
    buffer.type("cdefg");
    assert(buffer.size() == 7);
    assert(buffer.cursor() == 6);
    assert(buffer.userInput() == "acdefgb");
    buffer.erase(1, 3);
    assert(buffer.size() == 5);
    assert(buffer.cursor() == 4);
    assert(buffer.userInput() == "aefgb");
    assert(buffer.charAt(2) == 'f');
    buffer.erase(2, 5);
    assert(buffer.size() == 2);
    assert(buffer.cursor() == 2);
}

void test_utf8() {
    using namespace fcitx;
    InputBuffer buffer;
    buffer.type("\xe4\xbd\xa0\xe5\xa5\xbd");
    assert(buffer.size() == 2);
    assert(buffer.cursor() == 2);
    buffer.erase(1, 2);
    assert(buffer.size() == 1);
    assert(buffer.cursor() == 1);
    assert(buffer.userInput() == "\xe4\xbd\xa0");
    bool throwed = false;
    try {
        buffer.type("\xe4\xbd");
    } catch (const std::invalid_argument &e) {
        throwed = true;
    }
    assert(throwed);
    buffer.type("a\xe5\x95\x8a");
    assert(buffer.size() == 3);
    assert(buffer.cursor() == 3);
    assert(buffer.cursorByChar() == 7);
    buffer.setCursor(0);
    assert(buffer.cursorByChar() == 0);
    buffer.setCursor(1);
    assert(buffer.cursorByChar() == 3);
    buffer.setCursor(2);
    assert(buffer.cursorByChar() == 4);
    buffer.clear();
    assert(buffer.cursorByChar() == 0);
    assert(buffer.cursor() == 0);
    assert(buffer.size() == 0);

    buffer.type('a');
    assert(buffer.userInput() == "a");
    buffer.type('b');
    assert(buffer.userInput() == "ab");
    buffer.type('c');
    assert(buffer.userInput() == "abc");
    buffer.type('d');
    assert(buffer.userInput() == "abcd");
    buffer.type('e');
    assert(buffer.userInput() == "abcde");
    buffer.type('f');
    assert(buffer.userInput() == "abcdef");
    buffer.type('g');
    assert(buffer.userInput() == "abcdefg");
    buffer.backspace();
    assert(buffer.userInput() == "abcdef");
    buffer.backspace();
    assert(buffer.userInput() == "abcde");
    buffer.backspace();
    assert(buffer.userInput() == "abcd");
    buffer.backspace();
    assert(buffer.userInput() == "abc");
    buffer.backspace();
    assert(buffer.userInput() == "ab");
    buffer.backspace();
    assert(buffer.userInput() == "a");
    buffer.backspace();
    assert(buffer.userInput() == "");
}

int main() {
    test_basic(true);
    test_basic(false);
    test_utf8();
    return 0;
}
