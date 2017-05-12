/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#include "xcbtraywindow.h"
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>

namespace fcitx {
namespace classicui {

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define ATOM_SELECTION 0
#define ATOM_MANAGER 1
#define ATOM_SYSTEM_TRAY_OPCODE 2
#define ATOM_ORIENTATION 3
#define ATOM_VISUAL 4

XCBTrayWindow::XCBTrayWindow(XCBUI *ui) : XCBWindow(ui) {}

bool XCBTrayWindow::filterEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    switch (response_type) {
    case XCB_CLIENT_MESSAGE: {
        auto client_message =
            reinterpret_cast<xcb_client_message_event_t *>(event);
        if (client_message->type == atoms_[ATOM_MANAGER] &&
            client_message->format == 32 &&
            client_message->data.data32[1] == atoms_[ATOM_SELECTION] &&
            dockWindow_ == XCB_WINDOW_NONE) {
            refreshDockWindow();
            return true;
        }
        break;
    }

    case XCB_EXPOSE: {
        auto expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            if (auto surface = prerender()) {
                render();
            }
        }
        break;
    }
    case XCB_CONFIGURE_NOTIFY: {
        auto configure =
            reinterpret_cast<xcb_configure_notify_event_t *>(event);
        if (wid_ == configure->event) {
            if (width() != configure->width && height() != configure->height) {
                resize(configure->width, configure->height);
                xcb_size_hints_t size_hints;
                memset(&size_hints, 0, sizeof(size_hints));
                size_hints.flags = XCB_ICCCM_SIZE_HINT_BASE_SIZE;
                size_hints.base_width = configure->width;
                size_hints.base_height = configure->height;
                xcb_icccm_set_wm_normal_hints(ui_->connection(), wid_,
                                              &size_hints);
            }

            // TODO
            return true;
        }
        break;
    }
    case XCB_BUTTON_PRESS: {
        auto button = reinterpret_cast<xcb_button_press_event_t *>(event);
        if (button->event == wid_) {
            switch (button->detail) {
            case XCB_BUTTON_INDEX_1:
                // TODO
                break;
            case XCB_BUTTON_INDEX_3: {
                // TODO
            } break;
            }
            return true;
        }
    } break;
    case XCB_DESTROY_NOTIFY: {
        auto destroywindow =
            reinterpret_cast<xcb_destroy_notify_event_t *>(event);
        if (destroywindow->event == dockWindow_) {
            refreshDockWindow();
            return true;
        }
        break;
    }
    case XCB_PROPERTY_NOTIFY: {
        auto property = reinterpret_cast<xcb_property_notify_event_t *>(event);
        if (property->atom == atoms_[ATOM_VISUAL] &&
            property->window == dockWindow_) {
            createWindow(trayVisual());
            findDock();
            return true;
        }
        break;
    }
    }
    return false;
}

void XCBTrayWindow::initTray() {
    char trayAtomNameBuf[100];
    const char *atom_names[] = {
        trayAtomNameBuf, "MANAGER", "_NET_SYSTEM_TRAY_OPCODE",
        "_NET_SYSTEM_TRAY_ORIENTATION", "_NET_SYSTEM_TRAY_VISUAL"};

    sprintf(trayAtomNameBuf, "_NET_SYSTEM_TRAY_S%d", ui_->defaultScreen());
    size_t i = 0;
    for (auto name : atom_names) {
        atoms_[i] = internAtom(ui_->connection(), name);
        i++;
    }
    xcb_screen_t *screen =
        xcb_aux_get_screen(ui_->connection(), ui_->defaultScreen());
    addEventMaskToWindow(ui_->connection(), screen->root,
                         XCB_EVENT_MASK_STRUCTURE_NOTIFY);
    dockCallback_.reset(ui_->parent()->xcb()->call<IXCBModule::addSelection>(
        ui_->name(), atom_names[ATOM_SELECTION],
        [this](xcb_atom_t) { refreshDockWindow(); }));
}

void XCBTrayWindow::refreshDockWindow() {
    auto cookie = xcb_get_selection_owner(ui_->connection(), atoms_[0]);
    auto reply = makeXCBReply(
        xcb_get_selection_owner_reply(ui_->connection(), cookie, nullptr));
    if (reply) {
        dockWindow_ = reply->owner;
    }

    if (dockWindow_) {
        addEventMaskToWindow(ui_->connection(), dockWindow_,
                             XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        createWindow(trayVisual());
        findDock();
    } else {
        destroyWindow();
    }
}

void XCBTrayWindow::findDock() {
    if (!wid_) {
        return;
    }

    if (dockWindow_) {
        sendTrayOpcode(SYSTEM_TRAY_REQUEST_DOCK, wid_, 0, 0);
    }
}

void XCBTrayWindow::sendTrayOpcode(long message, long data1, long data2,
                                   long data3) {
    xcb_client_message_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = dockWindow_;
    ev.type = atoms_[ATOM_SYSTEM_TRAY_OPCODE];
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = message;
    ev.data.data32[2] = data1;
    ev.data.data32[3] = data2;
    ev.data.data32[4] = data3;

    xcb_send_event(ui_->connection(), false, dockWindow_,
                   XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<char *>(&ev));
}

xcb_visualid_t XCBTrayWindow::trayVisual() {
    xcb_visualid_t vid = 0;
    if (dockWindow_ != XCB_WINDOW_NONE) {
        auto cookie =
            xcb_get_property(ui_->connection(), false, dockWindow_,
                             atoms_[ATOM_VISUAL], XCB_ATOM_VISUALID, 0, 1);
        auto reply = makeXCBReply(
            xcb_get_property_reply(ui_->connection(), cookie, nullptr));
        if (reply && reply->type == XCB_ATOM_VISUALID && reply->format == 32 &&
            reply->bytes_after == 0) {
            auto data =
                static_cast<char *>(xcb_get_property_value(reply.get()));
            int length = xcb_get_property_value_length(reply.get());
            if (length == 32 / 8) {
                vid = *reinterpret_cast<xcb_visualid_t *>(data);
            }
        }
    }
    return vid;
}
}
}
