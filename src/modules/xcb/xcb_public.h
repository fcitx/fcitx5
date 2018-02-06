//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_MODULES_XCB_XCB_PUBLIC_H_
#define _FCITX_MODULES_XCB_XCB_PUBLIC_H_

#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>
#include <string>
#include <tuple>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

struct xkb_state;

namespace fcitx {
typedef std::array<std::string, 5> XkbRulesNames;
typedef std::function<bool(xcb_connection_t *conn, xcb_generic_event_t *event)>
    XCBEventFilter;
typedef std::function<void(const std::string &name, xcb_connection_t *conn,
                           int screen, FocusGroup *group)>
    XCBConnectionCreated;
typedef std::function<void(const std::string &name, xcb_connection_t *conn)>
    XCBConnectionClosed;
typedef std::function<void(xcb_atom_t selection)> XCBSelectionNotifyCallback;
typedef std::function<void(xcb_atom_t, const char *, size_t)>
    XCBConvertSelectionCallback;

template <typename T>
using XCBReply = std::unique_ptr<T, decltype(&std::free)>;

template <typename T>
static XCBReply<T> makeXCBReply(T *ptr) noexcept {
    return {ptr, &std::free};
}
} // namespace fcitx
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, openConnection,
                             void(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, addEventFilter,
                             std::unique_ptr<HandlerTableEntry<XCBEventFilter>>(
                                 const std::string &, XCBEventFilter));
FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addConnectionCreatedCallback,
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>(
        XCBConnectionCreated));
FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addConnectionClosedCallback,
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>>(
        XCBConnectionClosed));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbState,
                             xkb_state *(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, ewmh,
                             xcb_ewmh_connection_t *(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, atom,
                             xcb_atom_t(const std::string &,
                                        const std::string &, bool));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbRulesNames,
                             XkbRulesNames(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addSelection,
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>(
        const std::string &, const std::string &, XCBSelectionNotifyCallback));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, convertSelection,
                             std::unique_ptr<HandlerTableEntryBase>(
                                 const std::string &, const std::string &,
                                 const std::string &,
                                 XCBConvertSelectionCallback));

#endif // _FCITX_MODULES_XCB_XCB_PUBLIC_H_
