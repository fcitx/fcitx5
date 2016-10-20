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

#include "fcitx-utils/fs.h"
#include <cassert>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace fcitx::fs;

#define TEST_PATH(PATHSTR, EXPECT)                                                                                     \
    do {                                                                                                               \
        char pathstr[] = PATHSTR;                                                                                      \
        auto cleanStr = cleanPath(pathstr);                                                                            \
        assert(cleanStr == EXPECT);                                                                                    \
    } while (0);

#define TEST_DIRNAME(PATHSTR)                                                                                          \
    do {                                                                                                               \
        char pathstr[] = PATHSTR;                                                                                      \
        auto cleanStr = dirName(pathstr);                                                                              \
        const char *r = dirname(pathstr);                                                                              \
        assert(cleanStr == r);                                                                                         \
    } while (0);

int main() {
    TEST_PATH("/a", "/a");
    TEST_PATH("/a/b", "/a/b");
    TEST_PATH("a/b", "a/b");
    TEST_PATH("///a/b", "///a/b");
    TEST_PATH("///", "///");
    TEST_PATH("///a/..", "///");
    TEST_PATH("///a/./b", "///a/b");
    TEST_PATH("./././.", "");
    TEST_PATH("", "");
    TEST_PATH("../././.", "../");
    TEST_PATH("../././..", "../..");
    TEST_PATH(".././../.", "../../");
    TEST_PATH("///.././../.", "///../../");
    TEST_PATH("///a/./../c", "///c");
    TEST_PATH("./../a/../c/b", "../c/b");
    TEST_PATH("./.../a/../c/b", ".../c/b");

    TEST_DIRNAME("/usr/lib");
    TEST_DIRNAME("/usr/");
    TEST_DIRNAME("usr");
    TEST_DIRNAME("/");
    TEST_DIRNAME(".");
    TEST_DIRNAME("..");
    TEST_DIRNAME("a///b");
    TEST_DIRNAME("a//b///");
    TEST_DIRNAME("///a/b");
    TEST_DIRNAME("/a/b/");
    TEST_DIRNAME("/a/b///");

    assert(!isdir("a"));
    assert(!isdir("a/b"));
    assert(!isdir("a/b/c"));
    assert(makePath("a/b/c"));
    assert(makePath("///"));
    assert(makePath("a/b/c"));
    assert(makePath("a/b/d"));
    assert(makePath("a/b"));
    assert(makePath("a"));
    assert(makePath(""));
    assert(isdir("a"));
    assert(isdir("a/b"));
    assert(isdir("a/b/c"));
    assert(isdir("a/b/d"));
    assert(rmdir("a/b/c") == 0);
    assert(rmdir("a/b/d") == 0);
    assert(rmdir("a/b") == 0);
    assert(rmdir("a") == 0);
    return 0;
}
