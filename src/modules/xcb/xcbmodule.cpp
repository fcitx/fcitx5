/*
 * Copyright (C) 2015~2015 by CSSlayer
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

#include "xcbmodule.h"
#include "fcitx-utils/log.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xfixes.h>
#include <xkbcommon/xkbcommon-x11.h>

union _xkb_event {
    /* All XKB events share these fields. */
    struct {
        uint8_t response_type;
        uint8_t xkbType;
        uint16_t sequence;
        xcb_timestamp_t time;
        uint8_t deviceID;
    } any;
    xcb_xkb_new_keyboard_notify_event_t new_keyboard_notify;
    xcb_xkb_map_notify_event_t map_notify;
    xcb_xkb_state_notify_event_t state_notify;
};

namespace fcitx {

ConvertSelectionRequest::ConvertSelectionRequest(
    XCBConnection *conn, xcb_atom_t selection, xcb_atom_t type,
    xcb_atom_t property, XCBConvertSelectionCallback callback)

    : conn_(conn), selection_(selection), property_(property),
      realCallback_(std::move(callback)) {
    if (type == 0) {
        fallbacks_.push_back(XCB_ATOM_STRING);
        auto compoundAtom = conn->atom("COMPOUND_TEXT", true);
        if (compoundAtom) {
            fallbacks_.push_back(compoundAtom);
        }
        auto utf8Atom = conn->atom("UTF8_STRING", true);
        if (utf8Atom) {
            fallbacks_.push_back(utf8Atom);
        }
    } else {
        fallbacks_.push_back(type);
    }
    xcb_delete_property(conn->connection(), conn->serverWindow(), property_);
    xcb_convert_selection(conn->connection(), conn->serverWindow(), selection_,
                          fallbacks_.back(), property_, XCB_TIME_CURRENT_TIME);
    xcb_flush(conn->connection());
    timer_.reset(conn->parent()->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 5000000, 0,
        [this](EventSourceTime *, uint64_t) {
            invokeCallbackAndCleanUp(XCB_ATOM_NONE, nullptr, 0);
            return true;
        }));
}

void ConvertSelectionRequest::cleanUp() {
    realCallback_ = decltype(realCallback_)();
    timer_.reset();
}

void ConvertSelectionRequest::invokeCallbackAndCleanUp(xcb_atom_t type,
                                                       const char *data,
                                                       size_t length) {
    // Make a copy to real callback, because it might delete the this.
    auto realCallback = realCallback_;
    cleanUp();
    if (realCallback) {
        realCallback(type, data, length);
    }
}

void ConvertSelectionRequest::handleReply(xcb_atom_t type, const char *data,
                                          size_t length) {
    if (!realCallback_) {
        return;
    }
    if (type == fallbacks_.back()) {
        fallbacks_.pop_back();
        return invokeCallbackAndCleanUp(type, data, length);
    }

    fallbacks_.pop_back();
    if (fallbacks_.empty()) {
        return invokeCallbackAndCleanUp(XCB_ATOM_NONE, nullptr, 0);
    }

    xcb_delete_property(conn_->connection(), conn_->serverWindow(), property_);
    xcb_convert_selection(conn_->connection(), conn_->serverWindow(),
                          selection_, fallbacks_.back(), property_,
                          XCB_TIME_CURRENT_TIME);
    xcb_flush(conn_->connection());
}

XCBConnection::XCBConnection(XCBModule *xcb, const std::string &name)
    : parent_(xcb), name_(name), conn_(nullptr, xcb_disconnect),
      context_(nullptr, xkb_context_unref), keymap_(nullptr, xkb_keymap_unref),
      state_(nullptr, xkb_state_unref) {
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

    // init xkb
    {
        const xcb_query_extension_reply_t *reply =
            xcb_get_extension_data(conn_.get(), &xcb_xkb_id);
        if (reply && reply->present) {
            xkbFirstEvent_ = reply->first_event;
            xcb_xkb_use_extension_cookie_t xkb_query_cookie;

            xkb_query_cookie = xcb_xkb_use_extension(
                conn_.get(), XKB_X11_MIN_MAJOR_XKB_VERSION,
                XKB_X11_MIN_MINOR_XKB_VERSION);
            XCBReply<xcb_xkb_use_extension_reply_t> xkb_query{
                xcb_xkb_use_extension_reply(conn_.get(), xkb_query_cookie,
                                            nullptr),
                std::free};

            if (xkb_query && xkb_query->supported) {
                coreDeviceId_ =
                    xkb_x11_get_core_keyboard_device_id(conn_.get());

                const uint16_t required_map_parts =
                    (XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
                     XCB_XKB_MAP_PART_MODIFIER_MAP |
                     XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                     XCB_XKB_MAP_PART_KEY_ACTIONS |
                     XCB_XKB_MAP_PART_KEY_BEHAVIORS |
                     XCB_XKB_MAP_PART_VIRTUAL_MODS |
                     XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP);

                const uint16_t required_events =
                    (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                     XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                     XCB_XKB_EVENT_TYPE_STATE_NOTIFY);

                // XKB events are reported to all interested clients without
                // regard
                // to the current keyboard input focus or grab state
                xcb_void_cookie_t select = xcb_xkb_select_events_checked(
                    conn_.get(), XCB_XKB_ID_USE_CORE_KBD, required_events, 0,
                    required_events, required_map_parts, required_map_parts, 0);
                XCBReply<xcb_generic_error_t> error(
                    xcb_request_check(conn_.get(), select), std::free);
                if (!error) {
                    hasXKB_ = true;
                    updateKeymap();
                }
            }
        }
    }

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

    // create a focus group for display server
    group_ =
        new FocusGroup("x11:" + name_, xcb->instance()->inputContextManager());

    filter_.reset(addEventFilter(
        [this](xcb_connection_t *conn, xcb_generic_event_t *event) {
            return filterEvent(conn, event);
        }));
}

XCBConnection::~XCBConnection() {
    xcb_ewmh_connection_wipe(&ewmh_);
    delete group_;
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
    } else if (response_type == xkbFirstEvent_) {
        _xkb_event *xkbEvent = (_xkb_event *)event;
        if (xkbEvent->any.deviceID == coreDeviceId_) {
            switch (xkbEvent->any.xkbType) {
            case XCB_XKB_STATE_NOTIFY: {
                xcb_xkb_state_notify_event_t *state = &xkbEvent->state_notify;
                xkb_state_update_mask(state_.get(), state->baseMods,
                                      state->latchedMods, state->lockedMods,
                                      state->baseGroup, state->latchedGroup,
                                      state->lockedGroup);
            }
                return true;
                break;
            case XCB_XKB_MAP_NOTIFY: {
                updateKeymap();
            }
                return true;
                break;
            case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                xcb_xkb_new_keyboard_notify_event_t *ev =
                    &xkbEvent->new_keyboard_notify;
                if (ev->changed & XCB_XKB_NKN_DETAIL_KEYCODES) {
                    updateKeymap();
                }
            } break;
            }
        }
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
    }
    return false;
}

void XCBConnection::updateKeymap() {
    if (!context_) {
        context_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        xkb_context_set_log_level(context_.get(), XKB_LOG_LEVEL_CRITICAL);
    }

    if (!context_) {
        return;
    }

    keymap_.reset(nullptr);

    struct xkb_state *new_state = nullptr;
    if (hasXKB_) {
        keymap_.reset(xkb_x11_keymap_new_from_device(
            context_.get(), conn_.get(), coreDeviceId_,
            XKB_KEYMAP_COMPILE_NO_FLAGS));
        if (keymap_) {
            new_state = xkb_x11_state_new_from_device(
                keymap_.get(), conn_.get(), coreDeviceId_);
        }
    }

    if (!keymap_) {
        struct xkb_rule_names xkbNames;

        const auto names = xkbRulesNames();
        if (!names[0].empty()) {
            xkbNames.rules = names[0].c_str();
            xkbNames.model = names[1].c_str();
            xkbNames.layout = names[2].c_str();
            xkbNames.variant = names[3].c_str();
            xkbNames.options = names[4].c_str();

            keymap_.reset(xkb_keymap_new_from_names(
                context_.get(), &xkbNames, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }

        if (!keymap_) {
            memset(&xkbNames, 0, sizeof(xkbNames));
            keymap_.reset(xkb_keymap_new_from_names(
                context_.get(), &xkbNames, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }

        if (keymap_) {
            new_state = xkb_state_new(keymap_.get());
        }
    }

    state_.reset(new_state);
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

XkbRulesNames XCBConnection::xkbRulesNames() {
    if (!xkbRulesNamesAtom_) {
        xkbRulesNamesAtom_ = atom("_XKB_RULES_NAMES", true);
    }

    if (!xkbRulesNamesAtom_) {
        return {};
    }

    xcb_get_property_cookie_t get_prop_cookie =
        xcb_get_property(conn_.get(), false, root_, xkbRulesNamesAtom_,
                         XCB_ATOM_STRING, 0, 1024);
    auto reply = makeXCBReply(
        xcb_get_property_reply(conn_.get(), get_prop_cookie, nullptr));

    if (!reply || reply->type != XCB_ATOM_STRING || reply->bytes_after > 0 ||
        reply->format != 8) {
        return {};
    }

    auto data = static_cast<char *>(xcb_get_property_value(reply.get()));
    int length = xcb_get_property_value_length(reply.get());

    XkbRulesNames names;
    if (length) {
        std::string names[5];
        auto p = data, end = data + length;
        int i = 0;
        // The result from xcb_get_property_value() is not necessarily
        // \0-terminated,
        // we need to make sure that too many or missing '\0' symbols are
        // handled safely.
        do {
            auto len = strnlen(&(*p), length);
            names[i++] = std::string(&(*p), len);
            p += len + 1;
            length -= len + 1;
        } while (p < end || i < 5);
    }
    return names;
}

XCBModule::XCBModule(Instance *instance) : instance_(instance) {
    openConnection("");
}

void XCBModule::openConnection(const std::string &name_) {
    std::string name = name_;
    if (name.empty()) {
        auto env = getenv("DISPLAY");
        if (env) {
            name = env;
            mainDisplay_ = name;
        }
    }
    if (name.empty() || conns_.count(name)) {
        return;
    }

    try {
        auto iter = conns_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(name),
                                   std::forward_as_tuple(this, name));
        onConnectionCreated(iter.first->second);
    } catch (const std::exception &e) {
    }
}

void XCBModule::removeConnection(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter != conns_.end()) {
        conns_.erase(iter);
    }
    if (name == mainDisplay_) {
        mainDisplay_.clear();
        if (instance_->quitWhenMainDisplayDisconnected()) {
            instance_->exit();
        }
    }
}

HandlerTableEntry<XCBEventFilter> *
XCBModule::addEventFilter(const std::string &name, XCBEventFilter filter) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.addEventFilter(filter);
}

HandlerTableEntry<XCBConnectionCreated> *
XCBModule::addConnectionCreatedCallback(XCBConnectionCreated callback) {
    auto result = createdCallbacks_.add(callback);

    for (auto &p : conns_) {
        auto &conn = p.second;
        callback(conn.name(), conn.connection(), conn.screen(),
                 conn.focusGroup());
    }
    return result;
}

HandlerTableEntry<XCBConnectionClosed> *
XCBModule::addConnectionClosedCallback(XCBConnectionClosed callback) {
    return closedCallbacks_.add(callback);
}

xkb_state *XCBModule::xkbState(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.xkbState();
}

XkbRulesNames XCBModule::xkbRulesNames(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return {};
    }
    return iter->second.xkbRulesNames();
}

HandlerTableEntry<XCBSelectionNotifyCallback> *
XCBModule::addSelection(const std::string &name, const std::string &atom,
                        XCBSelectionNotifyCallback callback) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.addSelection(atom, callback);
}

HandlerTableEntryBase *
XCBModule::convertSelection(const std::string &name, const std::string &atom,
                            const std::string &type,
                            XCBConvertSelectionCallback callback) {

    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.convertSelection(atom, type, callback);
}

void XCBModule::onConnectionCreated(XCBConnection &conn) {
    for (auto &callback : createdCallbacks_.view()) {
        callback(conn.name(), conn.connection(), conn.screen(),
                 conn.focusGroup());
    }
}

void XCBModule::onConnectionClosed(XCBConnection &conn) {
    for (auto &callback : closedCallbacks_.view()) {
        callback(conn.name(), conn.connection());
    }
}

xcb_atom_t XCBModule::atom(const std::string &name, const std::string &atom,
                           bool exists) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return XCB_ATOM_NONE;
    }
    return iter->second.atom(atom, exists);
}

xcb_ewmh_connection_t *XCBModule::ewmh(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.ewmh();
}

class XCBModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new XCBModule(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::XCBModuleFactory);
