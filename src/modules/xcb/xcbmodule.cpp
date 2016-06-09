/*
 * Copyright (C) 2015~2015 by CSSlayer
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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xkbcommon/xkbcommon-x11.h>
#include "xcbmodule.h"
#include "fcitx/instance.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"

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

template <typename T>
using XCBReply = std::unique_ptr<T, decltype(&std::free)>;

template <typename T>
XCBReply<T> makeXCBReply(T *ptr) {
    return {ptr, &std::free};
}

XCBConnection::XCBConnection(XCBModule *xcb, const std::string &name)
    : m_parent(xcb), m_name(name), m_conn(nullptr, xcb_disconnect), m_screen(0), m_atom(0), m_serverWindow(0),
      m_root(0), m_group(nullptr), m_hasXKB(false), m_xkbRulesNamesAtom(0), m_xkbFirstEvent(0), m_coreDeviceId(0),
      m_context(nullptr, xkb_context_unref), m_keymap(nullptr, xkb_keymap_unref), m_state(nullptr, xkb_state_unref) {
    m_conn.reset(xcb_connect(name.c_str(), &m_screen));
    if (!m_conn || xcb_connection_has_error(m_conn.get())) {
        throw std::runtime_error("Failed to open xcb connection");
    }

    xcb_intern_atom_cookie_t atom_cookie =
        xcb_intern_atom(m_conn.get(), false, strlen("_FCITX_SERVER"), "_FCITX_SERVER");
    auto atom_reply = makeXCBReply(xcb_intern_atom_reply(m_conn.get(), atom_cookie, NULL));
    if (!atom_reply) {
        throw std::runtime_error("Failed to intern atom");
    }
    m_atom = atom_reply->atom;
    xcb_window_t w = xcb_generate_id(m_conn.get());
    xcb_screen_t *screen = xcb_aux_get_screen(m_conn.get(), m_screen);
    xcb_create_window(m_conn.get(), XCB_COPY_FROM_PARENT, w, screen->root, 0, 0, 1, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, 0, NULL);

    xcb_set_selection_owner(m_conn.get(), w, m_atom, XCB_CURRENT_TIME);
    m_serverWindow = w;
    int fd = xcb_get_file_descriptor(m_conn.get());
    auto &eventLoop = m_parent->instance()->eventLoop();
    m_ioEvent.reset(eventLoop.addIOEvent(fd, IOEventFlag::In, [this](EventSource *, int, IOEventFlags) {
        onIOEvent();
        return true;
    }));

    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_conn.get(), &xcb_xkb_id);
    if (reply && reply->present) {
        m_xkbFirstEvent = reply->first_event;
        xcb_xkb_use_extension_cookie_t xkb_query_cookie;

        xkb_query_cookie =
            xcb_xkb_use_extension(m_conn.get(), XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION);
        XCBReply<xcb_xkb_use_extension_reply_t> xkb_query{
            xcb_xkb_use_extension_reply(m_conn.get(), xkb_query_cookie, NULL), std::free};

        if (xkb_query && xkb_query->supported) {
            m_coreDeviceId = xkb_x11_get_core_keyboard_device_id(m_conn.get());

            const uint16_t required_map_parts =
                (XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS | XCB_XKB_MAP_PART_MODIFIER_MAP |
                 XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS | XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_KEY_BEHAVIORS |
                 XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP);

            const uint16_t required_events = (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                                              XCB_XKB_EVENT_TYPE_STATE_NOTIFY);

            // XKB events are reported to all interested clients without regard
            // to the current keyboard input focus or grab state
            xcb_void_cookie_t select =
                xcb_xkb_select_events_checked(m_conn.get(), XCB_XKB_ID_USE_CORE_KBD, required_events, 0,
                                              required_events, required_map_parts, required_map_parts, 0);
            XCBReply<xcb_generic_error_t> error(xcb_request_check(m_conn.get(), select), std::free);
            if (!error) {
                m_hasXKB = true;
                updateKeymap();
            }
        }
    }

    // create a focus group for display server
    m_group = new FocusGroup(xcb->instance()->inputContextManager());

    addEventFilter([this](xcb_connection_t *conn, xcb_generic_event_t *event) { return filterEvent(conn, event); });
}

XCBConnection::~XCBConnection() { delete m_group; }

void XCBConnection::onIOEvent() {
    if (xcb_connection_has_error(m_conn.get())) {
        return m_parent->removeConnection(m_name);
    }

    while (auto event = makeXCBReply(xcb_poll_for_event(m_conn.get()))) {
        for (auto &callback : m_filters) {
            if (callback.second(m_conn.get(), event.get())) {
                break;
            }
        }
    }
}

bool XCBConnection::filterEvent(xcb_connection_t *, xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    if (response_type == XCB_CLIENT_MESSAGE) {
        xcb_client_message_event_t *client_message = (xcb_client_message_event_t *)event;
        if (client_message->window == m_serverWindow && client_message->format == 8 && client_message->type == m_atom) {
            ICUUID uuid;
            memcpy(uuid.data(), client_message->data.data8, uuid.size());
            InputContext *ic = m_parent->instance()->inputContextManager().findByUUID(uuid);
            if (ic) {
                ic->setFocusGroup(m_group);
            }
        }
    } else if (response_type == m_xkbFirstEvent) {
        _xkb_event *xkbEvent = (_xkb_event *)event;
        if (xkbEvent->any.deviceID == m_coreDeviceId) {
            switch (xkbEvent->any.xkbType) {
            case XCB_XKB_STATE_NOTIFY: {
                xcb_xkb_state_notify_event_t *state = &xkbEvent->state_notify;
                xkb_state_update_mask(m_state.get(), state->baseMods, state->latchedMods, state->lockedMods,
                                      state->baseGroup, state->latchedGroup, state->lockedGroup);
            }
                return true;
                break;
            case XCB_XKB_MAP_NOTIFY: {
                updateKeymap();
            }
                return true;
                break;
            case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                xcb_xkb_new_keyboard_notify_event_t *ev = &xkbEvent->new_keyboard_notify;
                if (ev->changed & XCB_XKB_NKN_DETAIL_KEYCODES) {
                    updateKeymap();
                }
            } break;
            }
        }
    }
    return false;
}

void XCBConnection::updateKeymap() {
    if (!m_context) {
        m_context.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        xkb_context_set_log_level(m_context.get(), XKB_LOG_LEVEL_CRITICAL);
    }

    if (!m_context) {
        return;
    }

    m_keymap.reset(nullptr);

    struct xkb_state *new_state = NULL;
    if (m_hasXKB) {
        m_keymap.reset(
            xkb_x11_keymap_new_from_device(m_context.get(), m_conn.get(), m_coreDeviceId, XKB_KEYMAP_COMPILE_NO_FLAGS));
        if (m_keymap) {
            new_state = xkb_x11_state_new_from_device(m_keymap.get(), m_conn.get(), m_coreDeviceId);
        }
    }

    if (!m_keymap) {
        struct xkb_rule_names xkbNames;

        const auto nameString = xkbRulesNames();
        if (!nameString.empty()) {
            int length = nameString.size();
            const char *names[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
            auto p = nameString.begin(), end = nameString.end();
            int i = 0;
            // The result from xcb_get_property_value() is not necessarily
            // \0-terminated,
            // we need to make sure that too many or missing '\0' symbols are
            // handled safely.
            do {
                uint len = strnlen(&(*p), length);
                names[i++] = &(*p);
                p += len + 1;
                length -= len + 1;
            } while (p < end || i < 5);

            xkbNames.rules = names[0];
            xkbNames.model = names[1];
            xkbNames.layout = names[2];
            xkbNames.variant = names[3];
            xkbNames.options = names[4];

            m_keymap.reset(xkb_keymap_new_from_names(m_context.get(), &xkbNames, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }

        if (!m_keymap) {
            memset(&xkbNames, 0, sizeof(xkbNames));
            m_keymap.reset(xkb_keymap_new_from_names(m_context.get(), &xkbNames, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }

        if (m_keymap) {
            new_state = xkb_state_new(m_keymap.get());
        }
    }

    m_state.reset(new_state);
}

int XCBConnection::addEventFilter(XCBEventFilter filter) {
    m_filters.emplace(m_filterIdx, filter);
    return m_filterIdx++;
}

std::vector<char> XCBConnection::xkbRulesNames() {
    if (!m_xkbRulesNamesAtom) {
        xcb_intern_atom_cookie_t cookie =
            xcb_intern_atom(m_conn.get(), true, strlen("_XKB_RULES_NAMES"), "_XKB_RULES_NAMES");
        auto reply = makeXCBReply(xcb_intern_atom_reply(m_conn.get(), cookie, NULL));
        if (reply) {
            m_xkbRulesNamesAtom = reply->atom;
        }
    }

    if (!m_xkbRulesNamesAtom) {
        return {};
    }

    xcb_get_property_cookie_t get_prop_cookie =
        xcb_get_property(m_conn.get(), false, m_root, m_xkbRulesNamesAtom, XCB_ATOM_STRING, 0, 1024);
    auto reply = makeXCBReply(xcb_get_property_reply(m_conn.get(), get_prop_cookie, NULL));

    if (!reply || reply->type != XCB_ATOM_STRING || reply->bytes_after > 0 || reply->format != 8) {
        return {};
    }

    auto data = static_cast<char *>(xcb_get_property_value(reply.get()));
    int length = xcb_get_property_value_length(reply.get());

    return {data, data + length};
}

XCBModule::XCBModule(Instance *instance) : m_instance(instance) { openConnection(""); }

void XCBModule::openConnection(const std::string &name_) {
    std::string name = name_;
    if (name.empty()) {
        auto env = getenv("DISPLAY");
        if (env) {
            name = env;
        }
    }
    if (name.empty() || m_conns.count(name)) {
        return;
    }

    try {
        auto iter =
            m_conns.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(this, name));
        onConnectionCreated(iter.first->second);
    } catch (const std::exception &e) {
    }
}

void XCBModule::removeConnection(const std::string &name) {
    auto iter = m_conns.find(name);
    if (iter != m_conns.end()) {
        m_conns.erase(iter);
    }
}

int XCBModule::addEventFilter(const std::string &name, XCBEventFilter filter) {
    auto iter = m_conns.find(name);
    if (iter == m_conns.end()) {
        return -1;
    }
    return iter->second.addEventFilter(filter);
}

void XCBModule::removeEventFilter(const std::string &name, int id) {
    auto iter = m_conns.find(name);
    if (iter == m_conns.end()) {
        return;
    }
    return iter->second.removeEventFilter(id);
}

int XCBModule::addConnectionCreatedCallback(XCBConnectionCreated callback) {
    m_createdCallbacks.emplace(m_createdCallbacksIdx, callback);
    for (auto &p : m_conns) {
        auto &conn = p.second;
        callback(conn.name(), conn.connection(), conn.screen(), conn.focusGroup());
    }
    return m_createdCallbacksIdx++;
}

int XCBModule::addConnectionClosedCallback(XCBConnectionClosed callback) {
    m_closedCallbacks.emplace(m_closedCallbacksIdx, callback);
    return m_closedCallbacksIdx++;
}

void XCBModule::removeConnectionCreatedCallback(int id) { m_createdCallbacks.erase(id); }

void XCBModule::removeConnectionClosedCallback(int id) { m_closedCallbacks.erase(id); }

xkb_state *XCBModule::xkbState(const std::string &name) {
    auto iter = m_conns.find(name);
    if (iter == m_conns.end()) {
        return nullptr;
    }
    return iter->second.xkbState();
}

void XCBModule::onConnectionCreated(XCBConnection &conn) {
    for (auto &callback : m_createdCallbacks) {
        callback.second(conn.name(), conn.connection(), conn.screen(), conn.focusGroup());
    }
}

void XCBModule::onConnectionClosed(XCBConnection &conn) {
    for (auto &callback : m_closedCallbacks) {
        callback.second(conn.name(), conn.connection());
    }
}
}
