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
#ifndef _FCITX_UI_CLASSIC_WINDOW_H_
#define _FCITX_UI_CLASSIC_WINDOW_H_

#include "fcitx/userinterface.h"
#include <cairo/cairo.h>

namespace fcitx {
namespace classicui {

class Window {
public:
    Window(UserInterfaceComponent type);

    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    virtual void resize(unsigned int width, unsigned int height);

    virtual cairo_surface_t *prepare() = 0;
    virtual void swap() = 0;
    virtual void acquire() = 0;
    virtual void release() = 0;

protected:
    unsigned int width_ = 100;
    unsigned int height_ = 100;
    UserInterfaceComponent type_;
};
}
}

#endif // _FCITX_UI_CLASSIC_WINDOW_H_
