/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/flags.h"
#include "fcitx-utils/log.h"

using namespace fcitx;

enum class F { F1 = 1, F2 = 2, F3 = 4 };

int main() {
    Flags<F> f{F::F1, F::F3};
    FCITX_ASSERT(f.test(F::F1));
    FCITX_ASSERT(!f.test(F::F2));
    FCITX_ASSERT(f.test(F::F3));
    FCITX_ASSERT(f.testAny(F::F1));
    FCITX_ASSERT(f.testAny(Flags<F>{F::F1, F::F2}));
    FCITX_ASSERT(f.testAny(F::F3));
    return 0;
}
