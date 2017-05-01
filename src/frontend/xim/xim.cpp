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
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontext.h"
#include "fcitx/instance.h"
#include <xcb-imdkit/encoding.h>
#include <xcb/xcb_aux.h>
#include <xkbcommon/xkbcommon.h>

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
    XIMServer(xcb_connection_t *conn, int defaultScreen, FocusGroup *group,
              const std::string &name, XIMModule *xim)
        : group_(group), name_(name), parent_(xim),
          im_(nullptr, xcb_im_destroy), serverWindow_(0) {
        xcb_screen_t *screen = xcb_aux_get_screen(conn, defaultScreen);
        root_ = screen->root;
        serverWindow_ = xcb_generate_id(conn);
        xcb_create_window(
            conn, XCB_COPY_FROM_PARENT, serverWindow_, screen->root, 0, 0, 1, 1,
            1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);

        im_.reset(xcb_im_create(
            conn, defaultScreen, serverWindow_, guess_server_name().c_str(),
            XCB_IM_ALL_LOCALES, &styles, NULL, NULL, &encodings,
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
            &XIMServer::callback, this));

        filter_.reset(parent_->xcb()->call<fcitx::IXCBModule::addEventFilter>(
            name, [this](xcb_connection_t *, xcb_generic_event_t *event) {
                return xcb_im_filter_event(im_.get(), event);
            }));

        xcb_im_open_im(im_.get());
    }

    InputContextManager &inputContextManager() {
        return parent_->instance()->inputContextManager();
    }

    ~XIMServer() {
        if (im_) {
            xcb_im_close_im(im_.get());
        }
    }

    static void callback(xcb_im_t *, xcb_im_client_t *client,
                         xcb_im_input_context_t *xic,
                         const xcb_im_packet_header_fr_t *hdr, void *frame,
                         void *arg, void *user_data) {
        XIMServer *that = static_cast<XIMServer *>(user_data);
        that->callback(client, xic, hdr, frame, arg);
    }

    void callback(xcb_im_client_t *client, xcb_im_input_context_t *xic,
                  const xcb_im_packet_header_fr_t *hdr, void *frame, void *arg);

    auto im() { return im_.get(); }

    auto root() { return root_; }
    auto focusGroup() { return group_; }
    auto xkbState() {
        return parent_->xcb()->call<IXCBModule::xkbState>(name_);
    }

private:
    FocusGroup *group_;
    std::string name_;
    XIMModule *parent_;
    std::unique_ptr<xcb_im_t, decltype(&xcb_im_destroy)> im_;
    xcb_window_t root_;
    xcb_window_t serverWindow_;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
};

class XIMInputContext : public InputContext {
public:
    XIMInputContext(InputContextManager &inputContextManager, XIMServer *server,
                    xcb_im_input_context_t *ic)
        : InputContext(inputContextManager), server_(server), xic_(ic) {
        setFocusGroup(server->focusGroup());
        created();
    }
    ~XIMInputContext() { destroy(); }

protected:
    virtual void commitStringImpl(const std::string &text) override {
        size_t compoundTextLength;
        std::unique_ptr<char, decltype(&std::free)> compoundText(
            xcb_utf8_to_compound_text(text.c_str(), text.size(),
                                      &compoundTextLength),
            std::free);
        if (!compoundText) {
            return;
        }
        xcb_im_commit_string(server_->im(), xic_, XCB_XIM_LOOKUP_CHARS,
                             compoundText.get(), compoundTextLength, 0);
    }
    virtual void deleteSurroundingTextImpl(int, unsigned int) override {}
    virtual void forwardKeyImpl(const ForwardKeyEvent &key) override {
        xcb_key_press_event_t xcbEvent;
        memset(&xcbEvent, 0, sizeof(xcb_key_press_event_t));
        xcbEvent.time = key.time();
        xcbEvent.response_type =
            key.isRelease() ? XCB_KEY_RELEASE : XCB_KEY_PRESS;
        xcbEvent.state = key.rawKey().states();
        if (key.keyCode()) {
            xcbEvent.detail = key.keyCode();
        } else {
            xkb_state *xkbState = server_->xkbState();
            if (xkbState) {
                auto map = xkb_state_get_keymap(xkbState);
                auto min = xkb_keymap_min_keycode(map),
                     max = xkb_keymap_max_keycode(map);
                for (auto keyCode = min; keyCode < max; keyCode++) {
                    if (xkb_state_key_get_one_sym(xkbState, keyCode) ==
                        static_cast<uint32_t>(key.rawKey().sym())) {
                        xcbEvent.detail = keyCode;
                        break;
                    }
                }
            }
        }
        xcbEvent.root = server_->root();
        xcbEvent.event = xcb_im_input_context_get_focus_window(xic_);
        if ((xcbEvent.event = xcb_im_input_context_get_focus_window(xic_)) ==
            XCB_WINDOW_NONE) {
            xcbEvent.event = xcb_im_input_context_get_client_window(xic_);
        }
        xcbEvent.child = XCB_WINDOW_NONE;
        xcbEvent.same_screen = 0;
        xcbEvent.sequence = 0;
        xcb_im_forward_event(server_->im(), xic_, &xcbEvent);
    }
    virtual void updatePreeditImpl() override {
        auto &text = inputPanel().clientPreedit();
        auto strPreedit = text.toString();

        if (strPreedit.empty() && preeditStarted) {
            xcb_im_preedit_draw_fr_t frame;
            memset(&frame, 0, sizeof(xcb_im_preedit_draw_fr_t));
            frame.caret = 0;
            frame.chg_first = 0;
            frame.chg_length = lastPreeditLength;
            frame.length_of_preedit_string = 0;
            frame.preedit_string = NULL;
            frame.feedback_array.size = 0;
            frame.feedback_array.items = NULL;
            frame.status = 1;
            xcb_im_preedit_draw_callback(server_->im(), xic_, &frame);
            xcb_im_preedit_done_callback(server_->im(), xic_);
            preeditStarted = false;
        }

        if (!strPreedit.empty() && !preeditStarted) {
            xcb_im_preedit_start(server_->im(), xic_);
            preeditStarted = true;
        }
        if (!strPreedit.empty()) {
            size_t utf8Length = utf8::length(strPreedit);
            feedbackBuffer.clear();

            for (size_t i = 0, offset = 0; i < text.size(); i++) {
                auto format = text.formatAt(i);
                auto &str = text.stringAt(i);
                uint32_t feedback = 0;
                if (format & TextFormatFlag::UnderLine) {
                    feedback |= XCB_XIM_UNDERLINE;
                }
                if (format & TextFormatFlag::HighLight) {
                    feedback |= XCB_XIM_REVERSE;
                }
                unsigned int strLen = utf8::length(str);
                for (size_t j = 0; j < strLen; j++) {
                    feedbackBuffer.push_back(feedback);
                    offset++;
                }
            }
            while (!feedbackBuffer.empty() && feedbackBuffer.back() == 0) {
                feedbackBuffer.pop_back();
            }

            xcb_im_preedit_draw_fr_t frame;
            memset(&frame, 0, sizeof(xcb_im_preedit_draw_fr_t));
            frame.caret = utf8::lengthN(strPreedit.c_str(), text.cursor());
            frame.chg_first = 0;
            frame.chg_length = lastPreeditLength;
            size_t compoundTextLength;
            std::unique_ptr<char, decltype(&std::free)> compoundText(
                xcb_utf8_to_compound_text(strPreedit.c_str(), strPreedit.size(),
                                          &compoundTextLength),
                std::free);
            if (!compoundText) {
                return;
            }
            frame.length_of_preedit_string = compoundTextLength;
            frame.preedit_string =
                reinterpret_cast<uint8_t *>(compoundText.get());
            frame.feedback_array.size = feedbackBuffer.size();
            frame.feedback_array.items = feedbackBuffer.data();
            frame.status = frame.feedback_array.size ? 0 : 2;
            lastPreeditLength = utf8Length;
            xcb_im_preedit_draw_callback(server_->im(), xic_, &frame);
        }
    }

private:
    XIMServer *server_;
    xcb_im_input_context_t *xic_;
    bool preeditStarted = false;
    int lastPreeditLength = 0;
    std::vector<uint32_t> feedbackBuffer;
};

void XIMServer::callback(xcb_im_client_t *client, xcb_im_input_context_t *xic,
                         const xcb_im_packet_header_fr_t *hdr, void *frame,
                         void *arg) {
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
        ic = new XIMInputContext(parent_->instance()->inputContextManager(),
                                 this, xic);
        xcb_im_input_context_set_data(xic, ic, nullptr);
        break;
    case XCB_XIM_DESTROY_IC:
        delete ic;
        break;
    case XCB_XIM_SET_IC_VALUES:
        // kinds of like notification for position moving
        break;
    case XCB_XIM_FORWARD_EVENT: {
        xkb_state *state = xkbState();
        if (!state) {
            break;
        }
        xcb_key_press_event_t *xevent =
            static_cast<xcb_key_press_event_t *>(arg);
        KeyEvent event(ic, Key(static_cast<KeySym>(xkb_state_key_get_one_sym(
                                   state, xevent->detail)),
                               KeyStates(xevent->state)),
                       (xevent->response_type & ~0x80) == XCB_KEY_RELEASE,
                       xevent->detail, xevent->time);

        if (!ic->keyEvent(event)) {
            xcb_im_forward_event(im(), xic, xevent);
        }
        break;
    }
    case XCB_XIM_RESET_IC:
        ic->reset(ResetReason::Client);
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
    : instance_(instance),
      createdCallback_(xcb()->call<IXCBModule::addConnectionCreatedCallback>(
          [this](const std::string &name, xcb_connection_t *conn,
                 int defaultScreen, FocusGroup *group) {
              XIMServer *server =
                  new XIMServer(conn, defaultScreen, group, name, this);
              servers_[name].reset(server);
          })),
      closedCallback_(xcb()->call<IXCBModule::addConnectionClosedCallback>(
          [this](const std::string &name, xcb_connection_t *) {
              servers_.erase(name);
          })) {
    xcb_compound_text_init();
}

AddonInstance *XIMModule::xcb() {
    auto &addonManager = instance_->addonManager();
    return addonManager.addon("xcb");
}

XIMModule::~XIMModule() {}

class XIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new XIMModule(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::XIMModuleFactory);
