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
#include "xim.h"
#include "modules/xcb/xcb_public.h"
#include <xcb/xcb_aux.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx/instance.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontext.h"
#include "fcitx-utils/stringutils.h"

namespace {

static uint32_t style_array[] = {
    XCB_IM_PreeditPosition | XCB_IM_StatusArea,    // OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNothing, // OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNone,    // OverTheSpot
    XCB_IM_PreeditNothing | XCB_IM_StatusNothing,  // Root
    XCB_IM_PreeditNothing | XCB_IM_StatusNone,     // Root
};

static char COMPOUND_TEXT[] = "COMPOUND_TEXT";

static char *encoding_array[] = {
    COMPOUND_TEXT,
};

static xcb_im_encodings_t encodings = {1, encoding_array};

static xcb_im_styles_t styles = {5, style_array};

std::string guess_server_name() {
    char *env = getenv("XMODIFIERS");
    if (env && fcitx::stringutils::startsWith(env, "@im=")) {
        return env + 4; // length of "@im="
    }

    return "fcitx";
}
}

namespace fcitx {

class XIMServer {
public:
    XIMServer(xcb_connection_t *conn, int defaultScreen, FocusGroup *group, const std::string &name, XIMModule *xim)
        : m_group(group), m_name(name), m_parent(xim), m_im(nullptr, xcb_im_destroy), m_serverWindow(0) {
        xcb_screen_t *screen = xcb_aux_get_screen(conn, defaultScreen);
        m_serverWindow = xcb_generate_id(conn);
        xcb_create_window(conn, XCB_COPY_FROM_PARENT, m_serverWindow, screen->root, 0, 0, 1, 1, 1,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);

        m_im.reset(xcb_im_create(conn, defaultScreen, m_serverWindow, guess_server_name().c_str(), XCB_IM_ALL_LOCALES,
                                 &styles, NULL, NULL, &encodings, XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
                                 &XIMServer::callback, this));

        m_filterId = m_parent->xcb()->call<fcitx::IXCBModule::addEventFilter>(
            name,
            [this](xcb_connection_t *, xcb_generic_event_t *event) { return xcb_im_filter_event(m_im.get(), event); });

        xcb_im_open_im(m_im.get());
    }

    InputContextManager &inputContextManager() { return m_parent->instance()->inputContextManager(); }

    ~XIMServer() {
        if (m_im) {
            xcb_im_close_im(m_im.get());
        }
        if (m_filterId >= 0) {
            m_parent->xcb()->call<fcitx::IXCBModule::removeEventFilter>(m_name, m_filterId);
        }
    }

    static void callback(xcb_im_t *, xcb_im_client_t *client, xcb_im_input_context_t *xic,
                         const xcb_im_packet_header_fr_t *hdr, void *frame, void *arg, void *user_data) {
        XIMServer *that = static_cast<XIMServer *>(user_data);
        that->callback(client, xic, hdr, frame, arg);
    }

    void callback(xcb_im_client_t *client, xcb_im_input_context_t *xic, const xcb_im_packet_header_fr_t *hdr,
                  void *frame, void *arg);

    auto im() { return m_im.get(); }

private:
    FocusGroup *m_group;
    std::string m_name;
    XIMModule *m_parent;
    std::unique_ptr<xcb_im_t, decltype(&xcb_im_destroy)> m_im;
    xcb_window_t m_serverWindow;
    int m_filterId = -1;
};

class XIMInputContext : public InputContext {
public:
    XIMInputContext(InputContextManager &inputContextManager, XIMServer *server, xcb_im_input_context_t *ic)
        : InputContext(inputContextManager), m_server(server), m_xic(ic) {}

    virtual void commitString(const std::string &text) override {
        xcb_im_commit_string(m_server->im(), m_xic, XCB_XIM_LOOKUP_CHARS, text.c_str(), text.size(), 0);
    }
    virtual void deleteSurroundingText(int, unsigned int) {}
    virtual void forwardKey(const KeyEvent &key) {
        // TODO;
    }
    virtual void updatePreedit() {}

private:
    XIMServer *m_server;
    xcb_im_input_context_t *m_xic;
};

void XIMServer::callback(xcb_im_client_t *client, xcb_im_input_context_t *xic, const xcb_im_packet_header_fr_t *hdr,
                         void *frame, void *arg) {
    FCITX_UNUSED(client);
    FCITX_UNUSED(hdr);
    FCITX_UNUSED(frame);
    FCITX_UNUSED(arg);

    if (!xic) {
        return;
    }

    XIMInputContext *ic = nullptr;
    if (hdr->major_opcode != XCB_XIM_CREATE_IC) {
        ic = static_cast<XIMInputContext *>(xcb_im_input_context_get_data(xic));
    }

    switch (hdr->major_opcode) {
    case XCB_XIM_CREATE_IC:
        ic = new XIMInputContext(m_parent->instance()->inputContextManager(), this, xic);
        ic->setFocusGroup(m_group);
        xcb_im_input_context_set_data(xic, ic, nullptr);
        break;
    case XCB_XIM_DESTROY_IC:
        delete ic;
        break;
    case XCB_XIM_SET_IC_VALUES:
        // kinds of like notification for position moving
        break;
    case XCB_XIM_FORWARD_EVENT: {
        xkb_state *xkbState = m_parent->xcb()->call<IXCBModule::xkbState>(m_name);
        if (!xkbState) {
            break;
        }
        xcb_key_press_event_t *xevent = static_cast<xcb_key_press_event_t *>(arg);
        KeyEvent event;
        event.rawKey =
            Key(static_cast<KeySym>(xkb_state_key_get_one_sym(xkbState, xevent->detail)), KeyStates(xevent->state));
        event.isRelease = (xevent->response_type & ~0x80) != XCB_KEY_RELEASE;
        event.keyCode = xevent->detail;
        event.key = event.rawKey.normalize();
        event.time = xevent->time;
        if (!ic->keyEvent(event)) {
            xcb_im_forward_event(im(), xic, xevent);
        }
        break;
    }
    case XCB_XIM_RESET_IC:
        ic->reset();
        break;
    case XCB_XIM_SET_IC_FOCUS:
        ic->focusIn();
        break;
    case XCB_XIM_UNSET_IC_FOCUS:
        ic->focusOut();
        break;
    }
}

XIMModule::XIMModule(Instance *instance)
    : m_instance(instance),
      m_createdCallbackId(xcb()->call<IXCBModule::addConnectionCreatedCallback>(
          [this](const std::string &name, xcb_connection_t * conn, int defaultScreen, FocusGroup * group) {
              XIMServer *server = new XIMServer(conn, defaultScreen, group, name, this);
              m_servers[name].reset(server);
          })),
      m_closedCallbackId(xcb()->call<IXCBModule::addConnectionClosedCallback>(
          [this](const std::string &name, xcb_connection_t *) { m_servers.erase(name); })) {}

AddonInstance *XIMModule::xcb() {
    auto &addonManager = m_instance->addonManager();
    return addonManager.addon("xcb");
}

XIMModule::~XIMModule() {
    xcb()->call<IXCBModule::removeConnectionCreatedCallback>(m_createdCallbackId);
    xcb()->call<IXCBModule::removeConnectionCreatedCallback>(m_closedCallbackId);
}
}
