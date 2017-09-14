/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "fcitx-utils/log.h"
#include "fcitx/surroundingtext.h"

using namespace fcitx;

int main() {
    SurroundingText surroundingText;
    FCITX_ASSERT(!surroundingText.isValid());
    surroundingText.setText("abcd", 1, 1);

    FCITX_ASSERT(surroundingText.isValid());
    FCITX_ASSERT(surroundingText.text() == "abcd");
    surroundingText.deleteText(-1, 2);
    FCITX_ASSERT(surroundingText.text() == "cd");
    FCITX_ASSERT(surroundingText.anchor() == 0);
    FCITX_ASSERT(surroundingText.cursor() == 0);
    FCITX_ASSERT(surroundingText.isValid());
    surroundingText.invalidate();
    FCITX_ASSERT(!surroundingText.isValid());

    return 0;
}
