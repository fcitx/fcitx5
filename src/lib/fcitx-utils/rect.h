/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_RECT_H_
#define _FCITX_UTILS_RECT_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Provide utility to handle rectangle.

#include <algorithm>
#include <ostream>
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
        x2_ = width + x1_;
        y2_ = height + y1_;
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

    inline Rect intersected(const Rect &rect) noexcept {
        Rect tmp;
        tmp.x1_ = std::max(x1_, rect.x1_);
        tmp.x2_ = std::min(x2_, rect.x2_);
        tmp.y1_ = std::max(y1_, rect.y1_);
        tmp.y2_ = std::min(y2_, rect.y2_);
        if (tmp.x1_ < tmp.x2_ && tmp.y1_ < tmp.y2_) {
            return tmp;
        }
        return Rect();
    }

    // Return a rect with same size, but move by a offset.
    Rect translated(int offsetX, int offsetY) const {
        return Rect()
            .setPosition(x1_ + offsetX, y1_ + offsetY)
            .setSize(width(), height());
    }

    bool contains(int x, int y) const noexcept {
        return (x1_ <= x && x <= x2_) && (y1_ <= y && y <= y2_);
    }

    bool contains(const Rect &other) const noexcept {
        return contains(other.x1_, other.y1_) && contains(other.x2_, other.y2_);
    }

    int distance(int x, int y) const noexcept {
        int dx = 0;
        int dy = 0;
        if (x < x1_) {
            dx = x1_ - x;
        } else if (x > x2_) {
            dx = x - x2_;
        }
        if (y < y1_) {
            dy = y1_ - y;
        } else if (y > y2_) {
            dy = y - y2_;
        }
        return dx + dy;
    }

    inline int left() const noexcept { return x1_; }
    inline int top() const noexcept { return y1_; }
    inline int right() const noexcept { return x2_; }
    inline int bottom() const noexcept { return y2_; }

    inline int width() const noexcept { return x2_ - x1_; }
    inline int height() const noexcept { return y2_ - y1_; }

    inline bool operator==(const Rect &other) const {
        return x1_ == other.x1_ && x2_ == other.x2_ && y1_ == other.y1_ &&
               y2_ == other.y2_;
    }

    inline bool operator!=(const Rect &other) const {
        return !operator==(other);
    }

    inline bool isEmpty() const { return width() <= 0 || height() <= 0; }

private:
    int x1_, y1_, x2_, y2_;
};

inline std::ostream &operator<<(std::ostream &os, const Rect &r) {
    os << "Rect(" << r.left() << "," << r.top() << "+" << r.width() << "x"
       << r.height() << ")";
    return os;
}

}; // namespace fcitx

#endif // _FCITX_UTILS_RECT_H_
