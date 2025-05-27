/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <string.h>
#include "fcitx-utils/library.h"
#include "fcitx-utils/log.h"

#define DATA "AAAAAAAAA"
#define MAGIC "MAGIC_TEST_DATA"

#if defined(_WIN32)
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
MY_EXPORT char magic_test[] = MAGIC DATA;

MY_EXPORT int func() { return 0; }
}

void parser(const char *data) { FCITX_ASSERT(strcmp(data, DATA) == 0); }

int main() {
    fcitx::Library lib;
    FCITX_ASSERT(lib.load(fcitx::LibraryLoadHint::DefaultHint));
    FCITX_ASSERT(func == lib.resolve("func"));
    FCITX_ASSERT(lib.findData("magic_test", MAGIC, strlen(MAGIC), parser));

    FCITX_ASSERT(lib.unload());

    return 0;
}
