/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
