//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include <string.h>
#include "fcitx-utils/library.h"
#include "fcitx-utils/log.h"
#include "fcitxutils_export.h"

#define DATA "AAAAAAAAA"
#define MAGIC "MAGIC_TEST_DATA"

extern "C" {
FCITXUTILS_EXPORT
char magic_test[] = MAGIC DATA;

FCITXUTILS_EXPORT
int func() { return 0; }
}

void parser(const char *data) { FCITX_ASSERT(strcmp(data, DATA) == 0); }

int main() {
    fcitx::Library lib("");
    FCITX_ASSERT(lib.load(fcitx::LibraryLoadHint::DefaultHint));
    FCITX_ASSERT(func == lib.resolve("func"));
    FCITX_ASSERT(lib.findData("magic_test", MAGIC, strlen(MAGIC), parser));

    FCITX_ASSERT(lib.unload());

    return 0;
}
