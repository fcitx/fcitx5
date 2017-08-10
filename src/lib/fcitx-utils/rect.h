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

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Provide utility to handle rectangle.

#include "fcitxutils_export.h"

namespace fcitx {
class FCITXUTILS_EXPORT Rect {
public:
    explicit Rect(int _x1 = 0, int _y1 = 0, int _x2 = 0, int _y2 = 0)
        : x1_(_x1), y1_(_y1), x2_(_x2), y2_(_y2) {}

    Rect(const Rect &rect) = default;

    inline Rect &setPosition(int x, int y) noexcept {
        x1_ = x;
        y1_ = y;
        return *this;
    }

    inline Rect &setSize(int width, int height) noexcept {
        x2_ = width + x1_ - 1;
        y2_ = height + y1_ - 1;
        return *this;
    }

    inline Rect &setLeft(int pos) noexcept {
        x1_ = pos;
        return *this;
    }
    inline Rect &setTop(int pos) noexcept {
        y1_ = pos;
        return *this;
    }
    inline Rect &setRight(int pos) noexcept {
        x2_ = pos;
        return *this;
    }
    inline Rect &setBottom(int pos) noexcept {
        y2_ = pos;
        return *this;
    }

    int distance(int x, int y) const {
        int dx = 0;
        int dy = 0;
        if (x < x1_)
            dx = x1_ - x;
        else if (x > x2_)
            dx = x - x2_;
        if (y < y1_)
            dy = y1_ - y;
        else if (y > y2_)
            dy = y - y2_;
        return dx + dy;
    }

    inline int left() const noexcept { return x1_; }
    inline int top() const noexcept { return y1_; }
    inline int right() const noexcept { return x2_; }
    inline int bottom() const noexcept { return y2_; }

    inline int width() const noexcept { return x2_ - x1_ + 1; }
    inline int height() const noexcept { return y2_ - y1_ + 1; }

    inline bool operator==(const Rect &other) const {
        return x1_ == other.x1_ && x2_ == other.x2_ && y1_ == other.y1_ &&
               y2_ == other.y2_;
    }

    inline bool operator!=(const Rect &other) const {
        return !operator==(other);
    }

private:
    int x1_, y1_, x2_, y2_;
};
};

#endif // _FCITX_UTILS_RECT_H_
