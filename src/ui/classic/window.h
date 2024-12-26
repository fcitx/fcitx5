/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WINDOW_H_
#define _FCITX_UI_CLASSIC_WINDOW_H_

#include <cairo.h>

namespace fcitx::classicui {

class Window {
public:
    Window();
    virtual ~Window() = default;

    int width() const { return width_; }
    int height() const { return height_; }
    virtual void resize(unsigned int width, unsigned int height);

    virtual cairo_surface_t *prerender() = 0;
    virtual void render() = 0;

protected:
    unsigned int width_ = 100;
    unsigned int height_ = 100;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_WINDOW_H_
