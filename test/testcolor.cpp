/*
 * Copyright (C) 2015~2015 by CSSlayer
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
#include "fcitx-utils/color.h"
#include "fcitx-utils/log.h"
#include <cmath>

using namespace fcitx;

int main() {
    Color c("100 101 102");
    FCITX_ASSERT(c.toString() == "#646566ff");
    FCITX_ASSERT(c.red() == 100);
    FCITX_ASSERT(c.green() == 101);
    FCITX_ASSERT(c.blue() == 102);
    FCITX_ASSERT(c.alpha() == 255);
    FCITX_ASSERT(ceil(c.redF() * 255) == 100);
    FCITX_ASSERT(ceil(c.greenF() * 255) == 101);
    FCITX_ASSERT(ceil(c.blueF() * 255) == 102);
    FCITX_ASSERT(ceil(c.alphaF() * 255) == 255);
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

    try {
        c2.setFromString("#a");
        FCITX_ASSERT(false);
    } catch (ColorParseException e) {
        FCITX_LOG(Info) << e.what();
    }
    return 0;
}
