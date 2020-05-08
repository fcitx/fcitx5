/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"

using namespace fcitx::fs;

#define TEST_PATH(PATHSTR, EXPECT)                                             \
    do {                                                                       \
        char pathstr[] = PATHSTR;                                              \
        auto cleanStr = cleanPath(pathstr);                                    \
        FCITX_ASSERT(cleanStr == EXPECT);                                      \
    } while (0);

#define TEST_DIRNAME(PATHSTR)                                                  \
    do {                                                                       \
        char pathstr[] = PATHSTR;                                              \
        auto cleanStr = dirName(pathstr);                                      \
        const char *r = dirname(pathstr);                                      \
        FCITX_ASSERT(cleanStr == r);                                           \
    } while (0);

#define TEST_BASENAME(PATHSTR)                                                 \
    do {                                                                       \
        char pathstr[] = PATHSTR;                                              \
        auto cleanStr = baseName(pathstr);                                     \
        const char *r = basename(pathstr);                                     \
        FCITX_ASSERT(cleanStr == r);                                           \
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

    TEST_BASENAME("/usr/lib");
    TEST_BASENAME("/usr/");
    TEST_BASENAME("usr");
    TEST_BASENAME("/");
    TEST_BASENAME(".");
    TEST_BASENAME("..");
    TEST_BASENAME("a///b");
    TEST_BASENAME("a//b///");
    TEST_BASENAME("///a/b");
    TEST_BASENAME("/a/b/");
    TEST_BASENAME("/a/b///");

    FCITX_ASSERT(!isdir("a"));
    FCITX_ASSERT(!isdir("a/b"));
    FCITX_ASSERT(!isdir("a/b/c"));
    FCITX_ASSERT(makePath("a/b/c"));
    FCITX_ASSERT(makePath("///"));
    FCITX_ASSERT(makePath("a/b/c"));
    FCITX_ASSERT(makePath("a/b/d"));
    FCITX_ASSERT(makePath("a/b"));
    FCITX_ASSERT(makePath("a"));
    FCITX_ASSERT(makePath(""));
    FCITX_ASSERT(isdir("a"));
    FCITX_ASSERT(isdir("a/b"));
    FCITX_ASSERT(isdir("a/b/c"));
    FCITX_ASSERT(isdir("a/b/d"));
    FCITX_ASSERT(rmdir("a/b/c") == 0);
    FCITX_ASSERT(rmdir("a/b/d") == 0);
    FCITX_ASSERT(rmdir("a/b") == 0);
    FCITX_ASSERT(rmdir("a") == 0);
    return 0;
}
