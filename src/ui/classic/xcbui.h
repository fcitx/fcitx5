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
#ifndef _FCITX_UI_CLASSIC_XCBUI_H_
#define _FCITX_UI_CLASSIC_XCBUI_H_

#include "classicui.h"

namespace fcitx {
namespace classicui {

class XCBUI : public UIInterface {
public:
    XCBUI(ClassicUI *parent, const std::string &name, xcb_connection_t *conn, int defaultScreen);

    ClassicUI *parent() const { return parent_; }
    const std::string &name() const { return name_; }
    xcb_connection_t *connection() const { return conn_; }
    int defaultScreen() const { return defaultScreen_; }
    xcb_colormap_t colorMap() const { return colorMap_; }
    xcb_visualid_t visualId() const { return visualId_; }

private:
    ClassicUI *parent_;
    std::string name_;
    xcb_connection_t *conn_;
    int defaultScreen_;
    xcb_colormap_t colorMap_;
    xcb_visualid_t visualId_;
};
}
}

#endif // _FCITX_UI_CLASSIC_XCBUI_H_
