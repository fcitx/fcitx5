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
#ifndef _XCB_XCB_PUBLIC_H_
#define _XCB_XCB_PUBLIC_H_

#include <string>
#include <xcb/xcb.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>
#include <fcitx-utils/metastring.h>

struct xkb_state;

namespace fcitx {
typedef std::function<bool(xcb_connection_t *conn, xcb_generic_event_t *event)> XCBEventFilter;
typedef std::function<void(const std::string &name, xcb_connection_t *conn, int screen, FocusGroup *group)>
    XCBConnectionCreated;
typedef std::function<void(const std::string &name, xcb_connection_t *conn)> XCBConnectionClosed;
}

FCITX_ADDON_DECLARE_FUNCTION(XCBModule, addEventFilter, int(const std::string &, XCBEventFilter));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, removeEventFilter, void(const std::string &, int));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, addConnectionCreatedCallback, int(XCBConnectionCreated));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, addConnectionClosedCallback, int(XCBConnectionClosed));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, removeConnectionCreatedCallback, void(int));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, removeConnectionClosedCallback, void(int));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbState, xkb_state *(const std::string &));

#endif // _XCB_XCB_PUBLIC_H_
