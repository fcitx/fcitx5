
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
#ifndef _FCITX_UTILS_RECT_H_
#define _FCITX_UTILS_RECT_H_

#include "fcitxutils_export.h"

namespace fcitx {
class FCITXUTILS_EXPORT Rect {
public:
    Rect(int _x1 = 0, int _y1 = 0, int _x2 = 0, int _y2 = 0) : x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}

    Rect(const Rect &rect) = default;

    inline bool operator==(const Rect &other) const {
        return x1 == other.x1 && x2 == other.x2 && y1 == other.y1 && y2 == other.y2;
    }

    inline bool operator!=(const Rect &other) const { return !operator==(other); }
    int x1, y1, x2, y2;
};
};

#endif // _FCITX_UTILS_RECT_H_
