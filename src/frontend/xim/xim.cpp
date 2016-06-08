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
#include <xcb/xcb_aux.h>
#include "fcitx/instance.h"
#include "fcitx/focusgroup.h"
#include "fcitx-utils/stringutils.h"

namespace {

static uint32_t style_array[] = {
    XCB_IM_PreeditPosition | XCB_IM_StatusArea, //OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNothing,      //OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNone, //OverTheSpot
    XCB_IM_PreeditNothing | XCB_IM_StatusNothing,       //Root
    XCB_IM_PreeditNothing | XCB_IM_StatusNone,  //Root
};

static char COMPOUND_TEXT[] = "COMPOUND_TEXT";

static char* encoding_array[] = {
    COMPOUND_TEXT,
};

static xcb_im_encodings_t encodings = {
    1, encoding_array
};

static xcb_im_styles_t styles = {
    5, style_array
};


std::string guess_server_name()
{
    char* env = getenv("XMODIFIERS");
    if (env && fcitx::stringutils::startsWith(env, "@im=")) {
        return env + 4; // length of "@im="
    }

    return "fcitx";
}

}

namespace fcitx
{

class XIMServer
{
public:
    XIMServer(int defaultScreen,
        XIMModule* xim,
        FocusGroup* group,
        const std::string &name,
        xcb_connection_t *conn
    ) {
        xcb_screen_t* screen = xcb_aux_get_screen(conn, defaultScreen);
        xcb_window_t w = xcb_generate_id (conn);
        xcb_create_window (conn, XCB_COPY_FROM_PARENT, w, screen->root,
                        0, 0, 1, 1, 1,
                        XCB_WINDOW_CLASS_INPUT_OUTPUT,
                        screen->root_visual,
                        0, NULL);

        im = xcb_im_create(conn,
                                defaultScreen,
                                w,
                                guess_server_name().c_str(),
                                XCB_IM_ALL_LOCALES,
                                &styles,
                                NULL,
                                NULL,
                                &encodings,
                                XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
                                &XIMServer::callback,
                                this);
    }

    static void callback(xcb_im_t* im, xcb_im_client_t* client, xcb_im_input_context_t* xic,
              const xcb_im_packet_header_fr_t* hdr,
              void* frame, void* arg, void* user_data)
    {
        XIMServer* that = static_cast<XIMServer*>(user_data);
    }
private:
    xcb_im_t* im;
};

typedef std::function<void(const std::string &name, xcb_connection_t *conn,
                           int screen, FocusGroup *group)> XCBConnectionCreated;


XIMModule::XIMModule(Instance* instance) : m_instance(instance)
{
    auto &addonManager = instance->addonManager();
    auto xcb = addonManager.addon("xcb");
    xcb->call<void(XCBConnectionCreated)>("addConnectionCreatedCallback", [this] (const std::string &name, xcb_connection_t *conn,
                                                                              int defaultScreen, FocusGroup *group) {
        XIMServer *server = new XIMServer(defaultScreen,
            this,
            group,
            name,
            conn
        );
        m_servers[name].reset(server);
    });
}

XIMModule::~XIMModule() {
}

}
