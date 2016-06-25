/*
 * Copyright (C) 2015~2015 by CSSlayer
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
#include "fcitx-utils/color.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fcitx;

int main() {
    Color c("100 101 102");
    assert(c.toString() == "#646566ff");
    assert(c.red() == 100);
    assert(c.green() == 101);
    assert(c.blue() == 102);
    assert(c.alpha() == 255);
    assert(ceil(c.redF() * 255) == 100);
    assert(ceil(c.greenF() * 255) == 101);
    assert(ceil(c.blueF() * 255) == 102);
    assert(ceil(c.alphaF() * 255) == 255);
    c.setRed(50);
    assert(c.red() == 50);
    c.setGreen(51);
    assert(c.green() == 51);
    c.setBlue(52);
    assert(c.blue() == 52);
    c.setAlpha(53);
    assert(c.alpha() == 53);
    c.setRedF(70.0f / 255);
    assert(c.red() == 70);
    c.setGreenF(71.0f / 255);
    assert(c.green() == 71);
    c.setBlueF(72.0f / 255);
    assert(c.blue() == 72);
    c.setAlphaF(73.0f / 255);
    assert(c.alpha() == 73);
    Color c2("#aabbccdd");
    assert(c2.toString() == "#aabbccdd");

    try {
        c2.setFromString("#a");
        assert(false);
    } catch (ColorParseException e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
