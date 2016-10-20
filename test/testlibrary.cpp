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

#include "fcitx-utils/library.h"
#include "fcitxutils_export.h"
#include <cassert>
#include <string.h>

#define DATA "AAAAAAAAA"
#define MAGIC "MAGIC_TEST_DATA"

extern "C" {
FCITXUTILS_EXPORT
char magic_test[] = MAGIC DATA;

FCITXUTILS_EXPORT
int func() { return 0; }
}

void parser(const char *data) { assert(strcmp(data, DATA) == 0); }

int main() {
    fcitx::Library lib("");
    assert(lib.load(fcitx::LibraryLoadHint::DefaultHint));
    assert(func == lib.resolve("func"));
    assert(lib.findData("magic_test", MAGIC, strlen(MAGIC), parser));

    assert(lib.unload());

    return 0;
}
