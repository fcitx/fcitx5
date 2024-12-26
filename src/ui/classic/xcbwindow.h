/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_XCBWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBWINDOW_H_

#include <memory>
#include <cairo.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/misc.h"
#include "window.h"
#include "xcb_public.h"
#include "xcbui.h"

namespace fcitx::classicui {

class XCBWindow : public Window {
public:
    XCBWindow(XCBUI *ui, int width = 1, int height = 1);
    ~XCBWindow();

    void createWindow(xcb_visualid_t vid, bool overrideRedirect = true);
    virtual void postCreateWindow() {}
    void destroyWindow();
    void resize(unsigned int width, unsigned int height) override;

    cairo_surface_t *prerender() override;
    void render() override;

    virtual bool filterEvent(xcb_generic_event_t *event) = 0;

    xcb_window_t wid() const { return wid_; }

protected:
    XCBUI *ui_;
    xcb_window_t wid_ = 0;
    xcb_colormap_t colorMapNeedFree_ = 0;
    xcb_visualid_t vid_ = 0;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> eventFilter_;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> surface_;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> contentSurface_;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_XCBWINDOW_H_
