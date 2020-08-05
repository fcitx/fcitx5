/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <cmath>
#include "fcitx-utils/color.h"
#include "fcitx-utils/log.h"

using namespace fcitx;

int main() {
    Color c("100 101 102");
    FCITX_ASSERT(c.toString() == "#646566");
    FCITX_ASSERT(c.red() == 100);
    FCITX_ASSERT(c.green() == 101);
    FCITX_ASSERT(c.blue() == 102);
    FCITX_ASSERT(c.alpha() == 255);
    FCITX_ASSERT(std::ceil(c.redF() * 255) == 100);
    FCITX_ASSERT(std::ceil(c.greenF() * 255) == 101);
    FCITX_ASSERT(std::ceil(c.blueF() * 255) == 102);
    FCITX_ASSERT(std::ceil(c.alphaF() * 255) == 255);
    c.setRed(50);
    FCITX_ASSERT(c.red() == 50);
    c.setGreen(51);
    FCITX_ASSERT(c.green() == 51);
    c.setBlue(52);
    FCITX_ASSERT(c.blue() == 52);
    c.setAlpha(53);
    FCITX_ASSERT(c.alpha() == 53);
    c.setRedF(70.0f / 255);
    FCITX_ASSERT(c.red() == 70);
    c.setGreenF(71.0f / 255);
    FCITX_ASSERT(c.green() == 71);
    c.setBlueF(72.0f / 255);
    FCITX_ASSERT(c.blue() == 72);
    c.setAlphaF(73.0f / 255);
    FCITX_ASSERT(c.alpha() == 73);
    Color c2("#aabbccdd");
    FCITX_ASSERT(c2.toString() == "#aabbccdd");
    Color c3("#aabbccff");
    FCITX_ASSERT(c3.toString() == "#aabbcc");
    Color c4("#aabbcc");
    FCITX_ASSERT(c4.toString() == "#aabbcc");

    try {
        c2.setFromString("#a");
        FCITX_ASSERT(false);
    } catch (const ColorParseException &e) {
        FCITX_INFO() << e.what();
    }
    return 0;
}
