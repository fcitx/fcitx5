/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/log.h"
#include "fcitx-utils/rect.h"

using namespace fcitx;

int main() {
    Rect rect;
    FCITX_ASSERT(rect.width() == 0);
    FCITX_ASSERT(rect.height() == 0);
    FCITX_ASSERT(rect.top() == 0);
    FCITX_ASSERT(rect.left() == 0);

    rect.setPosition(-1, -2).setSize(10, 20);

    FCITX_ASSERT(rect.width() == 10);
    FCITX_ASSERT(rect.height() == 20);
    FCITX_ASSERT(rect.left() == -1);
    FCITX_ASSERT(rect.top() == -2);
    FCITX_ASSERT(rect.right() == 9);
    FCITX_ASSERT(rect.bottom() == 18);

    {
        Rect other;
        other.setSize(5, 5);
        FCITX_ASSERT(rect.intersected(other) == other);
        FCITX_ASSERT(other.intersected(rect) == other);
    }

    {
        Rect other;
        other.setSize(100, 100);
        Rect expect(0, 0, 9, 18);
        FCITX_ASSERT(rect.intersected(other) == expect);
        FCITX_ASSERT(other.intersected(rect) == expect);
    }

    {
        Rect other;
        other.setSize(100, 100);
        other = other.translated(-100, -100);
        Rect expect(-1, -2, 0, 0);
        FCITX_ASSERT(rect.intersected(other) == expect);
        FCITX_ASSERT(other.intersected(rect) == expect);
    }

    {
        Rect other;
        other.setSize(100, 100);
        other = other.translated(0, -100);
        Rect expect(0, -2, 9, 0);
        FCITX_ASSERT(rect.intersected(other) == expect)
            << rect.intersected(other);
        FCITX_ASSERT(other.intersected(rect) == expect);
    }

    {
        Rect other;
        other.setSize(3, 100);
        Rect expect(0, 0, 3, 18);
        FCITX_ASSERT(rect.intersected(other) == expect)
            << rect.intersected(other);
        FCITX_ASSERT(other.intersected(rect) == expect);
    }
    return 0;
}
