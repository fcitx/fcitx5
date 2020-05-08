/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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
