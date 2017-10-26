/*
* Copyright (C) 2017~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
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

#include "xcbconnection.h"
#include "config.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/misc_p.h"
#include "xcbconvertselection.h"
#include "xcbkeyboard.h"
#include "xcbmodule.h"
#include <xcb/xcb_aux.h>
#include <xcb/xfixes.h>

namespace fcitx {

XCBConnection::XCBConnection(XCBModule *xcb, const std::string &name)
    : parent_(xcb), name_(name) {
    // Open connection
    conn_.reset(xcb_connect(name.c_str(), &screen_));
    if (!conn_ || xcb_connection_has_error(conn_.get())) {
        throw std::runtime_error("Failed to open xcb connection");
    }

    // Create atom for ourselves
    atom_ = atom("_FCITX_SERVER", false);
    if (!atom_) {
        throw std::runtime_error("Failed to intern atom");
    }
    xcb_window_t w = xcb_generate_id(conn_.get());
    xcb_screen_t *screen = xcb_aux_get_screen(conn_.get(), screen_);
    root_ = screen->root;
    xcb_create_window(conn_.get(), XCB_COPY_FROM_PARENT, w, screen->root, 0, 0,
                      1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, nullptr);

    xcb_set_selection_owner(conn_.get(), w, atom_, XCB_CURRENT_TIME);
    serverWindow_ = w;
    int fd = xcb_get_file_descriptor(conn_.get());
    auto &eventLoop = parent_->instance()->eventLoop();
    ioEvent_.reset(eventLoop.addIOEvent(
        fd, IOEventFlag::In, [this](EventSource *, int, IOEventFlags) {
            onIOEvent();
            return true;
        }));

    eventHandlers_.emplace_back(parent_->instance()->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            auto &imManager = parent_->instance()->inputMethodManager();
            setDoGrab(imManager.groupCount() > 1);
        }));

    // create a focus group for display server
    group_ =
        new FocusGroup("x11:" + name_, xcb->instance()->inputContextManager());

    keyboard_ = std::make_unique<XCBKeyboard>(this);

    // init xfixes
    {
        const auto *reply = xcb_get_extension_data(conn_.get(), &xcb_xfixes_id);
        if (reply && reply->present) {

            xcb_xfixes_query_version_cookie_t xfixes_query_cookie =
                xcb_xfixes_query_version(conn_.get(), XCB_XFIXES_MAJOR_VERSION,
                                         XCB_XFIXES_MINOR_VERSION);
            auto xfixes_query = makeXCBReply(xcb_xfixes_query_version_reply(
                conn_.get(), xfixes_query_cookie, nullptr));
            if (xfixes_query && xfixes_query->major_version >= 2) {
                hasXFixes_ = true;
                xfixesFirstEvent_ = reply->first_event;
            }
        }
    }
    /// init ewmh
    memset(&ewmh_, 0, sizeof(ewmh_));
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn_.get(), &ewmh_);
    if (cookie) {
        // They will wipe for us. and cookie will be free'd anyway.
        if (!xcb_ewmh_init_atoms_replies(&ewmh_, cookie, NULL)) {
            memset(&ewmh_, 0, sizeof(ewmh_));
        }
    }

    syms_.reset(xcb_key_symbols_alloc(conn_.get()));

    filter_.reset(addEventFilter(
        [this](xcb_connection_t *conn, xcb_generic_event_t *event) {
            return filterEvent(conn, event);
        }));
    auto &imManager = parent_->instance()->inputMethodManager();
    setDoGrab(imManager.groupCount() > 1);
}

XCBConnection::~XCBConnection() {
    setDoGrab(false);
    if (keyboardGrabbed_) {
        ungrabXKeyboard();
    }
    xcb_ewmh_connection_wipe(&ewmh_);
    delete group_;
}

void XCBConnection::setDoGrab(bool doGrab) {
    if (doGrab_ != doGrab) {
        ;
        if (doGrab) {
            grabKey();
        } else {
            ungrabKey();
        }
        doGrab_ = doGrab;
    }
}

void XCBConnection::grabKey(const Key &key) {
    xcb_keysym_t sym = static_cast<xcb_keysym_t>(key.sym());
    uint modifiers = key.states();
    std::unique_ptr<xcb_keycode_t, decltype(&std::free)> keycode(
        xcb_key_symbols_get_keycode(syms_.get(), sym), &std::free);
    if (!keycode) {
        FCITX_LOG(Warn) << "Can not convert keyval=" << sym << " to keycode!";
    } else {
        FCITX_LOG(Debug) << "grab keycode " << *keycode << " modifiers "
                         << modifiers;
        auto cookie =
            xcb_grab_key_checked(conn_.get(), true, root_, modifiers, *keycode,
                                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        xcb_generic_error_t *error = xcb_request_check(conn_.get(), cookie);
        if (error) {
            FCITX_LOG(Debug)
                << "grab key error " << static_cast<int>(error->error_code)
                << " " << root_;
        }
        free(error);
    }
}

void addEventMaskToWindow(xcb_connection_t *conn, xcb_window_t wid,
                          uint32_t mask) {
    auto get_attr_cookie = xcb_get_window_attributes(conn, wid);
    auto get_attr_reply = makeXCBReply(
        xcb_get_window_attributes_reply(conn, get_attr_cookie, nullptr));
    if (get_attr_reply && (get_attr_reply->your_event_mask & mask) != mask) {
        const uint32_t newMask = get_attr_reply->your_event_mask | mask;
        xcb_change_window_attributes(conn, wid, XCB_CW_EVENT_MASK, &newMask);
    }
}

void XCBConnection::ungrabKey(const Key &key) {
    xcb_keysym_t sym = static_cast<xcb_keysym_t>(key.sym());
    uint modifiers = key.states();
    std::unique_ptr<xcb_keycode_t, decltype(&std::free)> keycode(
        xcb_key_symbols_get_keycode(syms_.get(), sym), &std::free);
    if (!keycode) {
        FCITX_LOG(Warn) << "Can not convert keyval=" << sym << " to keycode!";
    } else {
        xcb_ungrab_key(conn_.get(), *keycode, root_, modifiers);
    }
    xcb_flush(conn_.get());
}

void XCBConnection::grabKey() {
    FCITX_LOG(Debug) << "Grab key for X11 display: " << name_;
    auto &globalConfig = parent_->instance()->globalConfig();
    forwardGroup_ = globalConfig.enumerateGroupForwardKeys();
    backwardGroup_ = globalConfig.enumerateGroupBackwardKeys();
    for (const Key &key : forwardGroup_) {
        grabKey(key);
    }
    for (const Key &key : backwardGroup_) {
        grabKey(key);
    }
    // addEventMaskToWindow(conn_.get(), root_, XCB_EVENT_MASK_KEY_PRESS |
    // XCB_EVENT_MASK_KEY_RELEASE);
    xcb_flush(conn_.get());
}

void XCBConnection::ungrabKey() {
    for (const Key &key : forwardGroup_) {
        ungrabKey(key);
    }
    for (const Key &key : backwardGroup_) {
        ungrabKey(key);
    }
}

bool XCBConnection::grabXKeyboard() {
    if (keyboardGrabbed_)
        return false;
    FCITX_LOG(Debug) << "Grab keyboard for display: " << name_;
    auto cookie = xcb_grab_keyboard(conn_.get(), false, root_, XCB_CURRENT_TIME,
                                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    auto reply =
        makeXCBReply(xcb_grab_keyboard_reply(conn_.get(), cookie, NULL));

    if (reply && reply->status == XCB_GRAB_STATUS_SUCCESS) {
        keyboardGrabbed_ = true;
    }
    return keyboardGrabbed_;
}

void XCBConnection::ungrabXKeyboard() {
    if (!keyboardGrabbed_) {
        // grabXKeyboard() may fail sometimes, so don't fail, but at least warn
        // anyway
        FCITX_LOG(Debug)
            << "ungrabXKeyboard() called but keyboard not grabbed!";
    }
    FCITX_LOG(Debug) << "Ungrab keyboard for display: " << name_;
    keyboardGrabbed_ = false;
    xcb_ungrab_keyboard(conn_.get(), XCB_CURRENT_TIME);
    xcb_flush(conn_.get());
}

void XCBConnection::onIOEvent() {
    if (xcb_connection_has_error(conn_.get())) {
        return parent_->removeConnection(name_);
    }

    while (auto event = makeXCBReply(xcb_poll_for_event(conn_.get()))) {
        for (auto &callback : filters_.view()) {
            if (callback(conn_.get(), event.get())) {
                break;
            }
        }
    }
}

bool XCBConnection::filterEvent(xcb_connection_t *,
                                xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    if (response_type == XCB_CLIENT_MESSAGE) {
        auto client_message =
            reinterpret_cast<xcb_client_message_event_t *>(event);
        if (client_message->window == serverWindow_ &&
            client_message->format == 8 && client_message->type == atom_) {
            ICUUID uuid;
            memcpy(uuid.data(), client_message->data.data8, uuid.size());
            InputContext *ic =
                parent_->instance()->inputContextManager().findByUUID(uuid);
            if (ic) {
                ic->setFocusGroup(group_);
            }
        }
    } else if (keyboard_->handleEvent(event)) {
        return true;
    } else if (hasXFixes_ &&
               response_type ==
                   XCB_XFIXES_SELECTION_NOTIFY + xfixesFirstEvent_) {
        auto selectionNofity =
            reinterpret_cast<xcb_xfixes_selection_notify_event_t *>(event);
        auto callbacks = selections_.view(selectionNofity->selection);
        for (auto &callback : callbacks) {
            callback(selectionNofity->selection);
        }
    } else if (response_type == XCB_SELECTION_NOTIFY) {
        auto selectionNotify =
            reinterpret_cast<xcb_selection_notify_event_t *>(event);
        if (selectionNotify->requestor != serverWindow_) {
            return false;
        }
        for (auto &callback : convertSelections_.view()) {
            if (callback.property() != selectionNotify->property &&
                callback.selection() != selectionNotify->selection) {
                continue;
            }

            do {
                constexpr size_t bufLimit = 4096;
                xcb_get_property_cookie_t get_prop_cookie = xcb_get_property(
                    conn_.get(), false, serverWindow_,
                    selectionNotify->property, XCB_ATOM_ANY, 0, bufLimit);

                auto reply = makeXCBReply(xcb_get_property_reply(
                    conn_.get(), get_prop_cookie, nullptr));
                const char *data = nullptr;
                int length = 0;
                xcb_atom_t type = XCB_ATOM_NONE;
                if (reply && reply->type != XCB_NONE &&
                    reply->bytes_after == 0) {
                    type = reply->type;
                    data = static_cast<const char *>(
                        xcb_get_property_value(reply.get()));
                    length = xcb_get_property_value_length(reply.get());
                }
                callback.handleReply(type, data, length);
            } while (0);
        }
    } else if (response_type == XCB_KEY_PRESS) {
#define USED_MASK                                                              \
    (XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_1 |              \
     XCB_MOD_MASK_4)

        auto keypress = reinterpret_cast<xcb_key_press_event_t *>(event);
        if (keypress->event == root_) {
            FCITX_LOG(Debug) << "Received key event from root";
            auto sym = xcb_key_press_lookup_keysym(syms_.get(), keypress, 0);
            auto state = keypress->state;
            bool forward;
            Key key(static_cast<KeySym>(sym), KeyStates(state),
                    keypress->detail);
            key = key.normalize();
            if ((forward = key.checkKeyList(forwardGroup_)) ||
                key.checkKeyList(backwardGroup_)) {
                if (keyboardGrabbed_) {
                    navigateGroup(forward);
                } else {
                    if (grabXKeyboard()) {
                        groupIndex_ = 0;
                        navigateGroup(forward);
                    } else {
                        parent_->instance()->enumerateGroup(forward);
                    }
                }
            }
            return true;
        }
    } else if (response_type == XCB_KEY_RELEASE) {
        auto keyrelease = reinterpret_cast<xcb_key_release_event_t *>(event);
        if (keyrelease->event == root_) {
            keyRelease(keyrelease);
            return true;
        }
    }
    return false;
}

void XCBConnection::keyRelease(const xcb_key_release_event_t *event) {
    unsigned int mk = event->state & USED_MASK;
    // ev.state is state before the key release, so just checking mk being 0
    // isn't enough
    // using XQueryPointer() also doesn't seem to work well, so the check that
    // all
    // modifiers are released: only one modifier is active and the currently
    // released
    // key is this modifier - if yes, release the grab
    int mod_index = -1;
    for (int i = XCB_MAP_INDEX_SHIFT; i <= XCB_MAP_INDEX_5; ++i)
        if ((mk & (1 << i)) != 0) {
            if (mod_index >= 0)
                return;
            mod_index = i;
        }
    bool release = false;
    if (mod_index == -1)
        release = true;
    else {
        auto cookie = xcb_get_modifier_mapping(conn_.get());
        auto reply = xcb_get_modifier_mapping_reply(conn_.get(), cookie, NULL);
        if (reply) {
            auto keycodes = xcb_get_modifier_mapping_keycodes(reply);
            for (int i = 0; i < reply->keycodes_per_modifier; i++) {
                if (keycodes[reply->keycodes_per_modifier * mod_index + i] ==
                    event->detail) {
                    release = true;
                }
            }
        }
        free(reply);
    }
    if (!release) {
        return;
    }
    if (keyboardGrabbed_) {
        acceptGroupChange();
    }
}

void XCBConnection::acceptGroupChange() {
    FCITX_LOG(Debug) << "Accept group change";
    if (keyboardGrabbed_) {
        ungrabXKeyboard();
    }

    auto &imManager = parent_->instance()->inputMethodManager();
    auto groups = imManager.groups();
    if (groups.size() > groupIndex_) {
        imManager.setCurrentGroup(groups[groupIndex_]);
    }
    groupIndex_ = 0;
}

void XCBConnection::navigateGroup(bool forward) {
    auto &imManager = parent_->instance()->inputMethodManager();
    if (imManager.groupCount() < 2) {
        return;
    }
    groupIndex_ = (groupIndex_ + (forward ? 1 : imManager.groupCount() - 1)) %
                  imManager.groupCount();
    FCITX_LOG(Debug) << "Switch to group " << groupIndex_;
}

HandlerTableEntry<XCBEventFilter> *
XCBConnection::addEventFilter(XCBEventFilter filter) {
    return filters_.add(std::move(filter));
}

void XCBConnection::addSelectionAtom(xcb_atom_t atom) {
    xcb_xfixes_select_selection_input(
        conn_.get(), serverWindow_, atom,
        XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
            XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
            XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE);
    xcb_flush(conn_.get());
}

void XCBConnection::removeSelectionAtom(xcb_atom_t atom) {
    xcb_xfixes_select_selection_input(conn_.get(), serverWindow_, atom, 0);
}

xcb_atom_t XCBConnection::atom(const std::string &atomName, bool exists) {
    if (auto atomP = findValue(atomCache_, atomName)) {
        return *atomP;
    }

    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(conn_.get(), exists, atomName.size(), atomName.c_str());
    auto reply =
        makeXCBReply(xcb_intern_atom_reply(conn_.get(), cookie, nullptr));
    xcb_atom_t result = XCB_ATOM_NONE;
    if (reply) {
        result = reply->atom;
    }
    atomCache_.emplace(std::make_pair(atomName, result));
    return result;
}

xcb_ewmh_connection_t *XCBConnection::ewmh() { return &ewmh_; }

HandlerTableEntry<XCBSelectionNotifyCallback> *
XCBConnection::addSelection(const std::string &selection,
                            XCBSelectionNotifyCallback callback) {
    auto atomValue = atom(selection, true);
    if (atomValue) {
        return selections_.add(atomValue, std::move(callback));
    }
    return nullptr;
}

HandlerTableEntryBase *
XCBConnection::convertSelection(const std::string &selection,
                                const std::string &type,
                                XCBConvertSelectionCallback callback) {
    auto atomValue = atom(selection, true);
    if (atomValue == XCB_ATOM_NONE) {
        return nullptr;
    }

    xcb_atom_t typeAtom;
    if (type.empty()) {
        typeAtom = XCB_ATOM_NONE;
    } else {
        typeAtom = atom(type, true);
        if (typeAtom == XCB_ATOM_NONE) {
            return nullptr;
        }
    }
    std::string name = "FCITX_X11_SEL_" + selection;
    auto propertyAtom = atom(name, false);
    if (propertyAtom == XCB_ATOM_NONE) {
        return nullptr;
    }

    return convertSelections_.add(this, atomValue, typeAtom, propertyAtom,
                                  callback);
}

Instance *XCBConnection::instance() { return parent_->instance(); }

struct xkb_state *XCBConnection::xkbState() {
    return keyboard_->xkbState();
}

XkbRulesNames XCBConnection::xkbRulesNames() {
    return keyboard_->xkbRulesNames();
}

} // namespace fcitx
