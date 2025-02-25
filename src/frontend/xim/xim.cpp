/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xim.h"
#include <sys/types.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <xcb-imdkit/encoding.h>
#include <xcb-imdkit/imdkit.h>
#include <xcb-imdkit/ximproto.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/environ.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/textformatflags.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"
#include "xcb_public.h"

#define XIM_DEBUG() FCITX_LOGC(::xim, Debug)
#define XIM_KEY_DEBUG() FCITX_LOGC(::xim_key, Debug)

namespace {

FCITX_DEFINE_LOG_CATEGORY(xim, "xim")
FCITX_DEFINE_LOG_CATEGORY(xim_key, "xim_key")

uint32_t style_array[] = {
    XCB_IM_PreeditPosition | XCB_IM_StatusArea,    // OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNothing, // OverTheSpot
    XCB_IM_PreeditPosition | XCB_IM_StatusNone,    // OverTheSpot
    XCB_IM_PreeditNothing | XCB_IM_StatusNothing,  // Root
    XCB_IM_PreeditNothing | XCB_IM_StatusNone,     // Root
};

uint32_t onthespot_style_array[] = {
    XCB_IM_PreeditPosition | XCB_IM_StatusNothing,
    XCB_IM_PreeditCallbacks | XCB_IM_StatusNothing,
    XCB_IM_PreeditNothing | XCB_IM_StatusNothing,
    XCB_IM_PreeditPosition | XCB_IM_StatusCallbacks,
    XCB_IM_PreeditCallbacks | XCB_IM_StatusCallbacks,
    XCB_IM_PreeditNothing | XCB_IM_StatusCallbacks,
};

char COMPOUND_TEXT[] = "COMPOUND_TEXT";
char UTF8_STRING[] = "UTF8_STRING";

char *encoding_array[] = {COMPOUND_TEXT, UTF8_STRING};

xcb_im_encodings_t encodings = {FCITX_ARRAY_SIZE(encoding_array),
                                encoding_array};

xcb_im_styles_t styles = {FCITX_ARRAY_SIZE(style_array), style_array};
xcb_im_styles_t onthespot_styles = {FCITX_ARRAY_SIZE(onthespot_style_array),
                                    onthespot_style_array};

std::string guess_server_name() {
    const auto envValue = fcitx::getEnvironment("XMODIFIERS");
    if (envValue) {
        std::string_view server = *envValue;
        if (fcitx::stringutils::consumePrefix(server, "@im=")) {
            return std::string{server};
        }
    }

    return "fcitx";
}
} // namespace

namespace fcitx {

namespace {

void XimLogFunc(const char *fmt, ...) {
    std::va_list argp;
    va_start(argp, fmt);
    char onechar[1];
    int len = std::vsnprintf(onechar, 1, fmt, argp);
    va_end(argp);
    if (len < 1) {
        return;
    }
    std::vector<char> buf;
    buf.resize(len + 1);
    buf.back() = 0;
    va_start(argp, fmt);
    std::vsnprintf(buf.data(), len, fmt, argp);
    va_end(argp);
    XIM_DEBUG() << buf.data();
}

uint32_t getWindowPid(xcb_ewmh_connection_t *ewmh, xcb_window_t w) {
    auto cookie = xcb_ewmh_get_wm_pid(ewmh, w);
    uint32_t pid = 0;
    if (xcb_ewmh_get_wm_pid_reply(ewmh, cookie, &pid, nullptr) == 1) {
        return pid;
    }
    return 0;
}

} // namespace

class XIMServer {
public:
    XIMServer(xcb_connection_t *conn, int defaultScreen, FocusGroup *group,
              const std::string &name, XIMModule *xim)
        : conn_(conn), group_(group), name_(name), parent_(xim),
          serverWindow_(0) {
        xcb_screen_t *screen = xcb_aux_get_screen(conn, defaultScreen);
        root_ = screen->root;
        serverWindow_ = xcb_generate_id(conn);
        xcb_create_window(
            conn, XCB_COPY_FROM_PARENT, serverWindow_, screen->root, 0, 0, 1, 1,
            1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, nullptr);

        auto *inputStyles =
            (*parent_->config().useOnTheSpot ? &onthespot_styles : &styles);
        for (uint32_t i = 0; i < inputStyles->nStyles; i++) {
            supportedStyles_.insert(inputStyles->styles[i]);
        }
        im_.reset(xcb_im_create(
            conn, defaultScreen, serverWindow_, guess_server_name().c_str(),
            XCB_IM_ALL_LOCALES, inputStyles, nullptr, nullptr, &encodings,
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
            &XIMServer::callback, this));

        if (::xim().checkLogLevel(LogLevel::Debug)) {
            xcb_im_set_log_handler(im_.get(), XimLogFunc);
        }
        xcb_im_set_use_sync_mode(im_.get(), true);

        filter_ = parent_->xcb()->call<fcitx::IXCBModule::addEventFilter>(
            name, [this](xcb_connection_t *, xcb_generic_event_t *event) {
                bool result = xcb_im_filter_event(im_.get(), event);
                if (result) {
                    XIM_DEBUG() << "XIM filtered event";
                }
                return result;
            });

        ewmh_ = parent_->xcb()->call<fcitx::IXCBModule::ewmh>(name_);

        auto retry = 3;
        while (retry) {
            if (!xcb_im_open_im(im_.get())) {
                FCITX_ERROR() << "Failed to open xim, retrying.";
                retry -= 1;
                sleep(1);
            } else {
                break;
            }
        }
    }

    Instance *instance() { return parent_->instance(); }
    const auto &supportedStyles() { return supportedStyles_; }

    ~XIMServer() {
        if (im_) {
            xcb_im_close_im(im_.get());
        }
    }

    static void callback(xcb_im_t * /*unused*/, xcb_im_client_t *client,
                         xcb_im_input_context_t *xic,
                         const xcb_im_packet_header_fr_t *hdr, void *frame,
                         void *arg, void *user_data) {
        XIMServer *that = static_cast<XIMServer *>(user_data);
        that->callback(client, xic, hdr, frame, arg);
    }

    void callback(xcb_im_client_t *client, xcb_im_input_context_t *xic,
                  const xcb_im_packet_header_fr_t *hdr, void *frame, void *arg);

    auto im() { return im_.get(); }
    auto conn() { return conn_; }
    auto root() const { return root_; }
    auto ewmh() { return ewmh_; }
    auto focusGroup() { return group_; }
    auto xkbState() {
        return parent_->xcb()->call<IXCBModule::xkbState>(name_);
    }

    std::string getProgramName(xcb_im_input_context_t *ic) {
        auto w = xcb_im_input_context_get_client_window(ic);
        if (!w) {
            w = xcb_im_input_context_get_focus_window(ic);
        }
        if (w) {
            while (w != root_) {
                if (auto pid = getWindowPid(ewmh_, w)) {
                    return getProcessName(pid);
                }

                auto cookie = xcb_query_tree(conn_, w);
                auto reply = makeUniqueCPtr(
                    xcb_query_tree_reply(conn_, cookie, nullptr));
                if (!reply) {
                    break;
                }
                // This should never happen, but just as a sanity check.
                if (reply->root != root_ || w == reply->parent) {
                    break;
                }
                w = reply->parent;
            }
        }
        return {};
    }

private:
    xcb_connection_t *conn_;
    FocusGroup *group_;
    std::string name_;
    XIMModule *parent_;
    UniqueCPtr<xcb_im_t, xcb_im_destroy> im_;
    xcb_window_t root_;
    xcb_window_t serverWindow_;
    xcb_ewmh_connection_t *ewmh_;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
    // bool value: isUtf8
    std::unordered_map<xcb_im_client_t *, bool> clientEncodingMapping_;
    std::unordered_set<uint32_t> supportedStyles_;
    UniqueCPtr<struct xkb_state, xkb_state_unref> localState_;
};

class XIMInputContext final : public InputContext {
public:
    XIMInputContext(InputContextManager &inputContextManager, XIMServer *server,
                    xcb_im_input_context_t *ic, bool useUtf8)
        : InputContext(inputContextManager, server->getProgramName(ic)),
          server_(server), xic_(ic), useUtf8_(useUtf8) {
        setFocusGroup(server->focusGroup());
        xcb_im_input_context_set_data(xic_, this, nullptr);

        created();
        CapabilityFlags flags = CapabilityFlag::ReportKeyRepeat;
        if (validatedInputStyle() & XCB_IM_PreeditCallbacks) {
            flags = flags | CapabilityFlag::Preedit;
            flags = flags | CapabilityFlag::FormattedPreedit;
        }
        setCapabilityFlags(flags);
    }
    ~XIMInputContext() {
        xcb_im_input_context_set_data(xic_, nullptr, nullptr);
        destroy();
    }

    uint32_t validatedInputStyle() {
        auto style = xcb_im_input_context_get_input_style(xic_);
        if (server_->supportedStyles().contains(style)) {
            return style;
        }
        auto preeditStyle = (style & 0xff) | XCB_IM_StatusNothing;
        if (server_->supportedStyles().contains(preeditStyle)) {
            return preeditStyle;
        }
        auto statusStyle = (style & 0xff00) | XCB_IM_PreeditNothing;
        if (server_->supportedStyles().contains(statusStyle)) {
            return statusStyle;
        }

        return XCB_IM_StatusNothing | XCB_IM_PreeditNothing;
    }

    const char *frontend() const override { return "xim"; }

    void maybeUpdateCursorLocationForRootStyle() {
        if ((validatedInputStyle() & XCB_IM_PreeditPosition) ==
            XCB_IM_PreeditPosition) {
            return;
        }
        updateCursorLocation();
    }

    void updateCursorLocation() {
        // kinds of like notification for position moving
        auto mask = xcb_im_input_context_get_preedit_attr_mask(xic_);
        auto w = xcb_im_input_context_get_focus_window(xic_);
        if (!w) {
            w = xcb_im_input_context_get_client_window(xic_);
        }
        if (!w) {
            return;
        }
        if (mask & XCB_XIM_XNArea_MASK) {
            auto a = xcb_im_input_context_get_preedit_attr(xic_)->area;
            auto trans_cookie = xcb_translate_coordinates(
                server_->conn(), w, server_->root(), a.x, a.y);
            auto reply = makeUniqueCPtr(xcb_translate_coordinates_reply(
                server_->conn(), trans_cookie, nullptr));
            if (!reply) {
                return;
            }
            setCursorRect(Rect()
                              .setPosition(reply->dst_x, reply->dst_y)
                              .setSize(a.width, a.height));
        } else if (mask & XCB_XIM_XNSpotLocation_MASK) {
            auto p = xcb_im_input_context_get_preedit_attr(xic_)->spot_location;
            auto trans_cookie = xcb_translate_coordinates(
                server_->conn(), w, server_->root(), p.x, p.y);
            auto reply = makeUniqueCPtr(xcb_translate_coordinates_reply(
                server_->conn(), trans_cookie, nullptr));
            if (!reply) {
                return;
            }
            setCursorRect(
                Rect().setPosition(reply->dst_x, reply->dst_y).setSize(0, 0));
        } else {
            auto getgeo_cookie = xcb_get_geometry(server_->conn(), w);
            auto reply = makeUniqueCPtr(xcb_get_geometry_reply(
                server_->conn(), getgeo_cookie, nullptr));
            if (!reply) {
                return;
            }
            auto trans_cookie = xcb_translate_coordinates(
                server_->conn(), w, server_->root(), 0, 0);
            auto trans_reply = makeUniqueCPtr(xcb_translate_coordinates_reply(
                server_->conn(), trans_cookie, nullptr));
            if (!trans_reply) {
                return;
            }

            setCursorRect(Rect()
                              .setPosition(trans_reply->dst_x,
                                           trans_reply->dst_y + reply->height)
                              .setSize(0, 0));
        }
    }

    KeyStates updateAutoRepeatState(xcb_key_press_event_t *xevent) {
        // Client may or may not call XkbSetDetectableAutoRepeat, so we must
        // handle both cases.
        bool isAutoRepeat = false;
        bool isRelease = (xevent->response_type & ~0x80) == XCB_KEY_RELEASE;
        if (isRelease) {
            // Always mark key release as non auto repeat, because we don't know
            // if it is real release.
            isAutoRepeat = false;
        } else {
            // If timestamp is same as last release
            if (lastIsRelease_) {
                if (lastTime_ && lastTime_ == xevent->time &&
                    lastKeyCode_ == xevent->detail) {
                    isAutoRepeat = true;
                }
            } else {
                if (lastKeyCode_ == xevent->detail) {
                    isAutoRepeat = true;
                }
            }
        }

        lastKeyCode_ = xevent->detail;
        lastIsRelease_ = isRelease;
        lastTime_ = xevent->time;
        KeyStates states(xevent->state);
        if (isAutoRepeat) {
            // KeyState::Repeat
            states = states | KeyState::Repeat;
        }
        return states;
    }

    void resetAutoRepeatState() {
        lastKeyCode_ = 0;
        lastIsRelease_ = false;
        lastTime_ = 0;
    }

protected:
    void commitStringImpl(const std::string &text) override {
        UniqueCPtr<char> compoundText;
        const char *commit = text.data();
        size_t length = text.size();
        if (!useUtf8_) {
            size_t compoundTextLength;
            compoundText.reset(xcb_utf8_to_compound_text(
                text.c_str(), text.size(), &compoundTextLength));
            if (!compoundText) {
                return;
            }
            commit = compoundText.get();
            length = compoundTextLength;
        }
        XIM_DEBUG() << "XIM commit: " << text;

        xcb_im_commit_string(server_->im(), xic_, XCB_XIM_LOOKUP_CHARS, commit,
                             length, 0);
    }
    void deleteSurroundingTextImpl(int /*offset*/,
                                   unsigned int /*size*/) override {}
    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        xcb_key_press_event_t xcbEvent;
        memset(&xcbEvent, 0, sizeof(xcb_key_press_event_t));
        xcbEvent.time = key.time();
        xcbEvent.response_type =
            key.isRelease() ? XCB_KEY_RELEASE : XCB_KEY_PRESS;
        xcbEvent.state = key.rawKey().states();
        if (key.rawKey().code()) {
            xcbEvent.detail = key.rawKey().code();
        } else {
            xkb_state *xkbState = server_->xkbState();
            if (xkbState) {
                auto *map = xkb_state_get_keymap(xkbState);
                auto min = xkb_keymap_min_keycode(map);
                auto max = xkb_keymap_max_keycode(map);
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
        if (xcbEvent.event == XCB_WINDOW_NONE) {
            xcbEvent.event = xcb_im_input_context_get_client_window(xic_);
        }
        xcbEvent.child = XCB_WINDOW_NONE;
        xcbEvent.same_screen = 0;
        xcbEvent.sequence = 0;
        xcb_im_forward_event(server_->im(), xic_, &xcbEvent);
    }
    void updatePreeditImpl() override {
        auto text = server_->instance()->outputFilter(
            this, inputPanel().clientPreedit());
        auto strPreedit = text.toString();

        if (strPreedit.empty() && preeditStarted) {
            xcb_im_preedit_draw_fr_t frame;
            memset(&frame, 0, sizeof(xcb_im_preedit_draw_fr_t));
            frame.caret = 0;
            frame.chg_first = 0;
            frame.chg_length = lastPreeditLength_;
            frame.length_of_preedit_string = 0;
            frame.preedit_string = nullptr;
            frame.feedback_array.size = 0;
            frame.feedback_array.items = nullptr;
            frame.status = 1;
            xcb_im_preedit_draw_callback(server_->im(), xic_, &frame);
            xcb_im_preedit_done_callback(server_->im(), xic_);
            preeditStarted = false;
            lastPreeditLength_ = 0;
        }

        if (!strPreedit.empty() && !preeditStarted) {
            xcb_im_preedit_start_callback(server_->im(), xic_);
            preeditStarted = true;
        }
        if (!strPreedit.empty()) {
            size_t utf8Length = utf8::length(strPreedit);
            if (utf8Length == utf8::INVALID_LENGTH) {
                return;
            }
            feedbackBuffer_.clear();

            for (size_t i = 0; i < text.size(); i++) {
                auto format = text.formatAt(i);
                const auto &str = text.stringAt(i);
                uint32_t feedback = 0;
                if (format & TextFormatFlag::Underline) {
                    feedback |= XCB_XIM_UNDERLINE;
                }
                if (format & TextFormatFlag::HighLight) {
                    feedback |= XCB_XIM_REVERSE;
                }
                unsigned int strLen = utf8::length(str);
                for (size_t j = 0; j < strLen; j++) {
                    feedbackBuffer_.push_back(feedback);
                }
            }
            feedbackBuffer_.push_back(0);

            xcb_im_preedit_draw_fr_t frame;
            memset(&frame, 0, sizeof(xcb_im_preedit_draw_fr_t));
            if (text.cursor() >= 0 &&
                static_cast<size_t>(text.cursor()) <= strPreedit.size()) {
                frame.caret =
                    utf8::length(strPreedit.begin(),
                                 std::next(strPreedit.begin(), text.cursor()));
            }
            UniqueCPtr<char> compoundText;
            frame.chg_first = 0;
            frame.chg_length = lastPreeditLength_;
            if (useUtf8_) {
                frame.preedit_string =
                    reinterpret_cast<uint8_t *>(strPreedit.data());
                frame.length_of_preedit_string = strPreedit.size();
            } else {
                size_t compoundTextLength;
                compoundText.reset(xcb_utf8_to_compound_text(
                    strPreedit.c_str(), strPreedit.size(),
                    &compoundTextLength));
                if (!compoundText) {
                    return;
                }
                frame.length_of_preedit_string = compoundTextLength;
                frame.preedit_string =
                    reinterpret_cast<uint8_t *>(compoundText.get());
            }
            frame.feedback_array.size = feedbackBuffer_.size();
            frame.feedback_array.items = feedbackBuffer_.data();
            frame.status = frame.feedback_array.size ? 0 : 2;
            lastPreeditLength_ = utf8Length;
            xcb_im_preedit_draw_callback(server_->im(), xic_, &frame);
        }
    }

private:
    XIMServer *server_;
    xcb_im_input_context_t *xic_;
    const bool useUtf8_ = false;
    bool preeditStarted = false;
    int lastPreeditLength_ = 0;
    std::vector<uint32_t> feedbackBuffer_;
    bool lastIsRelease_ = false;
    unsigned int lastTime_ = 0;
    unsigned int lastKeyCode_ = 0;
};

void XIMServer::callback(xcb_im_client_t *client, xcb_im_input_context_t *xic,
                         const xcb_im_packet_header_fr_t *hdr, void *frame,
                         void *arg) {
    FCITX_UNUSED(hdr);
    FCITX_UNUSED(frame);

    switch (hdr->major_opcode) {
    case XCB_XIM_ENCODING_NEGOTIATION:
        if (arg) {
            auto encodingIndex = *static_cast<uint16_t *>(arg);
            XIM_DEBUG() << "Client encoding: " << client << " "
                        << encodingIndex;
            if (encodingIndex != 0) {
                clientEncodingMapping_[client] = encodingIndex == 1;
            }
        }
        return;
    case XCB_XIM_DISCONNECT:
        XIM_DEBUG() << "Client disconnect: " << client;
        clientEncodingMapping_.erase(client);
        return;
    default:
        break;
    }

    if (!xic) {
        return;
    }

    XIM_DEBUG() << "XIM header opcode: " << static_cast<int>(hdr->major_opcode);

    XIMInputContext *ic = nullptr;
    if (hdr->major_opcode != XCB_XIM_CREATE_IC) {
        ic = static_cast<XIMInputContext *>(xcb_im_input_context_get_data(xic));
        if (!ic) {
            return;
        }
    }

    switch (hdr->major_opcode) {
    case XCB_XIM_CREATE_IC: {
        bool useUtf8 = false;
        if (auto *entry = findValue(clientEncodingMapping_, client);
            entry && *entry) {
            useUtf8 = true;
        }
        new XIMInputContext(parent_->instance()->inputContextManager(), this,
                            xic, useUtf8);
    } break;
    case XCB_XIM_DESTROY_IC:
        delete ic;
        break;
    case XCB_XIM_SET_IC_VALUES: {
        ic->updateCursorLocation();
        break;
    }
    case XCB_XIM_FORWARD_EVENT: {
        xkb_state *state = xkbState();
        if (!state) {
            break;
        }

        auto *keymap = xkb_state_get_keymap(state);
        if (!localState_ || xkb_state_get_keymap(localState_.get()) !=
                                xkb_state_get_keymap(state)) {
            localState_.reset(xkb_state_new(keymap));
        }
        auto *xevent = static_cast<xcb_key_press_event_t *>(arg);
        auto layout =
            xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
        // Always use the state that is forwarded by client.
        // For xkb_state_key_get_one_sym, no need to distinguish
        // depressed/latched/locked.

        // Remove all button mask before send to XKB.
        // XKB May have internal virtual state bit, which collides with this
        // bits. See: https://github.com/fcitx/fcitx5/issues/1106
        constexpr uint16_t button_mask = XCB_BUTTON_MASK_1 | XCB_BUTTON_MASK_2 |
                                         XCB_BUTTON_MASK_3 | XCB_BUTTON_MASK_4 |
                                         XCB_BUTTON_MASK_5;

        xkb_state_update_mask(localState_.get(),
                              (xevent->state & (~button_mask)), 0, 0, 0, 0,
                              layout);
        KeyEvent event(ic,
                       Key(static_cast<KeySym>(xkb_state_key_get_one_sym(
                               localState_.get(), xevent->detail)),
                           ic->updateAutoRepeatState(xevent), xevent->detail),
                       (xevent->response_type & ~0x80) == XCB_KEY_RELEASE,
                       xevent->time);
        XIM_KEY_DEBUG() << "XIM Key Event: "
                        << static_cast<int>(xevent->response_type) << " "
                        << event.rawKey().toString() << " time:" << xevent->time
                        << " detail: " << static_cast<int>(xevent->detail)
                        << " state: " << xevent->state
                        << " sequence:" << xevent->sequence;
        if (!ic->hasFocus()) {
            ic->focusIn();
        }

        bool result;
        {
            InputContextEventBlocker blocker(ic);
            result = ic->keyEvent(event);
        }
        if (!result) {
            xcb_im_forward_event(im(), xic, xevent);
        }
        break;
    }
    case XCB_XIM_RESET_IC:
        ic->reset();
        break;
    case XCB_XIM_SET_IC_FOCUS:
        ic->focusIn();
        ic->updateCursorLocation();
        break;
    case XCB_XIM_UNSET_IC_FOCUS:
        ic->resetAutoRepeatState();
        ic->focusOut();
        break;
    default:
        break;
    }
}

XIMModule::XIMModule(Instance *instance) : instance_(instance) {
    xcb_compound_text_init();
    reloadConfig();
    createdCallback_ = xcb()->call<IXCBModule::addConnectionCreatedCallback>(
        [this](const std::string &name, xcb_connection_t *conn,
               int defaultScreen, FocusGroup *group) {
            XIMServer *server =
                new XIMServer(conn, defaultScreen, group, name, this);
            servers_[name].reset(server);
        });
    closedCallback_ = xcb()->call<IXCBModule::addConnectionClosedCallback>(
        [this](const std::string &name, xcb_connection_t *) {
            servers_.erase(name);
        });

    updateRootStyleCallback_ = instance_->watchEvent(
        EventType::InputContextFlushUI, EventWatcherPhase::PreInputMethod,
        [](Event &event) {
            auto &uiEvent = static_cast<InputContextFlushUIEvent &>(event);
            auto *ic = uiEvent.inputContext();
            if (uiEvent.component() == UserInterfaceComponent::InputPanel &&
                ic->frontendName() == "xim") {
                auto *xic = static_cast<XIMInputContext *>(ic);
                xic->maybeUpdateCursorLocationForRootStyle();
            }
        });
}

XIMModule::~XIMModule() {}

void XIMModule::reloadConfig() { readAsIni(config_, "conf/xim.conf"); }

class XIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new XIMModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY_V2(xim, fcitx::XIMModuleFactory);
