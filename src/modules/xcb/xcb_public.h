/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_XCB_XCB_PUBLIC_H_
#define _FCITX_MODULES_XCB_XCB_PUBLIC_H_

#include <string>
#include <tuple>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>

#ifndef FCITX_NO_XCB
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#endif

struct xkb_state;

namespace fcitx {
using XkbRulesNames = std::array<std::string, 5>;

#ifndef FCITX_NO_XCB
using XCBEventFilter =
    std::function<bool(xcb_connection_t *conn, xcb_generic_event_t *event)>;
using XCBConnectionCreated =
    std::function<void(const std::string &name, xcb_connection_t *conn,
                       int screen, FocusGroup *group)>;
using XCBConnectionClosed =
    std::function<void(const std::string &name, xcb_connection_t *conn)>;
using XCBSelectionNotifyCallback = std::function<void(xcb_atom_t selection)>;
using XCBConvertSelectionCallback =
    std::function<void(xcb_atom_t, const char *, size_t)>;
#endif

} // namespace fcitx

FCITX_ADDON_DECLARE_FUNCTION(XCBModule, openConnection,
                             void(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, openConnectionChecked,
                             bool(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbState,
                             xkb_state *(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbRulesNames,
                             XkbRulesNames(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, setXkbOption,
                             void(const std::string &, const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, mainDisplay, std::string());
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, isXWayland, bool(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, exists, bool(const std::string &name));

#ifndef FCITX_NO_XCB
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
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, ewmh,
                             xcb_ewmh_connection_t *(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, atom,
                             xcb_atom_t(const std::string &,
                                        const std::string &, bool));
FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addSelection,
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>(
        const std::string &, const std::string &, XCBSelectionNotifyCallback));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, convertSelection,
                             std::unique_ptr<HandlerTableEntryBase>(
                                 const std::string &, const std::string &,
                                 const std::string &,
                                 XCBConvertSelectionCallback));
#endif

#endif // _FCITX_MODULES_XCB_XCB_PUBLIC_H_
