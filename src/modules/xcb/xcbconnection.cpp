/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xcbconnection.h"
#include <stdexcept>
#include <fmt/format.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xfixes.h>
#include <xcb/xproto.h>
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/misc_p.h"
#include "notifications_public.h"
#include "xcbconvertselection.h"
#include "xcbeventreader.h"
#include "xcbkeyboard.h"
#include "xcbmodule.h"

namespace fcitx {

bool extensionCheckXWayland(xcb_connection_t *conn) {
    constexpr std::string_view xwaylandExt("XWAYLAND");
    auto queryCookie =
        xcb_query_extension(conn, xwaylandExt.length(), xwaylandExt.data());
    auto extQuery =
        makeUniqueCPtr(xcb_query_extension_reply(conn, queryCookie, nullptr));
    return (extQuery && extQuery->present);
}

bool xrandrCheckXWayland(xcb_connection_t *conn, xcb_screen_t *screen) {
    const xcb_query_extension_reply_t *queryExtReply =
        xcb_get_extension_data(conn, &xcb_randr_id);
    if (!queryExtReply || !queryExtReply->present) {
        return false;
    }

    auto cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
    auto reply = makeUniqueCPtr(
        xcb_randr_get_screen_resources_current_reply(conn, cookie, nullptr));
    if (!reply) {
        return false;
    }
    xcb_timestamp_t timestamp = 0;
    xcb_randr_output_t *outputs = nullptr;
    int outputCount =
        xcb_randr_get_screen_resources_current_outputs_length(reply.get());

    if (outputCount) {
        timestamp = reply->config_timestamp;
        outputs = xcb_randr_get_screen_resources_current_outputs(reply.get());
    } else {
        auto resourcesCookie =
            xcb_randr_get_screen_resources(conn, screen->root);
        auto resourcesReply =
            makeUniqueCPtr(xcb_randr_get_screen_resources_reply(
                conn, resourcesCookie, nullptr));
        if (resourcesReply) {
            timestamp = resourcesReply->config_timestamp;
            outputCount = xcb_randr_get_screen_resources_outputs_length(
                resourcesReply.get());
            outputs =
                xcb_randr_get_screen_resources_outputs(resourcesReply.get());
        }
    }

    for (int i = 0; i < outputCount; i++) {
        auto outputInfoCookie =
            xcb_randr_get_output_info(conn, outputs[i], timestamp);
        auto output = makeUniqueCPtr(
            xcb_randr_get_output_info_reply(conn, outputInfoCookie, nullptr));
        // Invalid, disconnected or disabled output
        if (!output) {
            continue;
        }

        std::string_view outputName(
            reinterpret_cast<char *>(
                xcb_randr_get_output_info_name(output.get())),
            xcb_randr_get_output_info_name_length(output.get()));
        if (stringutils::startsWith(outputName, "XWAYLAND")) {
            return true;
        }
    }
    return false;
}

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

    eventHandlers_.emplace_back(parent_->instance()->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            auto &imManager = parent_->instance()->inputMethodManager();
            setDoGrab(imManager.groupCount() > 1);
        }));

    eventHandlers_.emplace_back(parent_->instance()->watchEvent(
        EventType::GlobalConfigReloaded, EventWatcherPhase::Default,
        [this](Event &) {
            // Ungrab first, in order to grab new config.
            setDoGrab(false);
            auto &imManager = parent_->instance()->inputMethodManager();
            setDoGrab(imManager.groupCount() > 1);
        }));

    connections_.emplace_back(
        parent_->instance()
            ->inputMethodManager()
            .connect<InputMethodManager::GroupAdded>(
                [this](const std::string &) {
                    auto &imManager = parent_->instance()->inputMethodManager();
                    setDoGrab(imManager.groupCount() > 1);
                }));
    connections_.emplace_back(
        parent_->instance()
            ->inputMethodManager()
            .connect<InputMethodManager::GroupRemoved>(
                [this](const std::string &) {
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
            auto xfixes_query = makeUniqueCPtr(xcb_xfixes_query_version_reply(
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
        if (!xcb_ewmh_init_atoms_replies(&ewmh_, cookie, nullptr)) {
            memset(&ewmh_, 0, sizeof(ewmh_));
        }
    }

    FCITX_XCB_INFO() << "Connecting to X11 display, display name:" << name_
                     << ".";
    if (extensionCheckXWayland(conn_.get()) ||
        xrandrCheckXWayland(conn_.get(), screen)) {
        isXWayland_ = true;
        FCITX_XCB_INFO() << "X11 display: " << name_ << " is xwayland.";
    }

    syms_.reset(xcb_key_symbols_alloc(conn_.get()));

    filter_ = addEventFilter(
        [this](xcb_connection_t *conn, xcb_generic_event_t *event) {
            return filterEvent(conn, event);
        });
    auto &imManager = parent_->instance()->inputMethodManager();
    setDoGrab(imManager.groupCount() > 1);
    reader_ = std::make_unique<XCBEventReader>(this);
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
        if (doGrab) {
            grabKey();
        } else {
            ungrabKey();
        }
        doGrab_ = doGrab;
    }
}

std::tuple<xcb_keycode_t, uint32_t> XCBConnection::getKeyCode(const Key &key) {
    xcb_keycode_t keycode = 0;
    uint32_t modifiers = key.states();
    if (key.code()) {
        keycode = key.code();
    } else {
        xcb_keysym_t sym = static_cast<xcb_keysym_t>(key.sym());
        UniqueCPtr<xcb_keycode_t> xcbKeycode(
            xcb_key_symbols_get_keycode(syms_.get(), sym));
        if (key.isModifier()) {
            modifiers &= ~Key::keySymToStates(key.sym());
        }
        if (!xcbKeycode) {
            FCITX_XCB_WARN()
                << "Can not convert keyval=" << sym << " to keycode!";
        } else {
            keycode = *xcbKeycode;
        }
    }

    return {keycode, modifiers};
}

void XCBConnection::grabKey(const Key &key) {
    auto [keycode, modifiers] = getKeyCode(key);

    if (!keycode) {
        return;
    }

    FCITX_XCB_DEBUG() << "grab keycode " << static_cast<int>(keycode)
                      << " modifiers " << modifiers;
    auto cookie =
        xcb_grab_key_checked(conn_.get(), true, root_, modifiers, keycode,
                             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    UniqueCPtr<xcb_generic_error_t> error(
        xcb_request_check(conn_.get(), cookie));
    if (error) {
        FCITX_XCB_DEBUG() << "grab key error "
                          << static_cast<int>(error->error_code) << " "
                          << root_;
    }
}

void XCBConnection::ungrabKey(const Key &key) {
    auto [keycode, modifiers] = getKeyCode(key);
    if (!keycode) {
        return;
    }

    xcb_ungrab_key(conn_.get(), keycode, root_, modifiers);
    xcb_flush(conn_.get());
}

void XCBConnection::grabKey() {
    FCITX_XCB_DEBUG() << "Grab key for X11 display: " << name_;
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
    if (keyboardGrabbed_) {
        return false;
    }
    FCITX_XCB_DEBUG() << "Grab keyboard for display: " << name_;
    auto cookie = xcb_grab_keyboard(conn_.get(), false, root_, XCB_CURRENT_TIME,
                                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    auto reply =
        makeUniqueCPtr(xcb_grab_keyboard_reply(conn_.get(), cookie, nullptr));

    if (reply && reply->status == XCB_GRAB_STATUS_SUCCESS) {
        keyboardGrabbed_ = true;
    }
    return keyboardGrabbed_;
}

void XCBConnection::ungrabXKeyboard() {
    if (!keyboardGrabbed_) {
        // grabXKeyboard() may fail sometimes, so don't fail, but at least warn
        // anyway
        FCITX_XCB_DEBUG()
            << "ungrabXKeyboard() called but keyboard not grabbed!";
    }
    FCITX_XCB_DEBUG() << "Ungrab keyboard for display: " << name_;
    keyboardGrabbed_ = false;
    xcb_ungrab_keyboard(conn_.get(), XCB_CURRENT_TIME);
    xcb_flush(conn_.get());
}

void XCBConnection::processEvent() {
    auto events = reader_->events();
    for (const auto &event : events) {
        for (auto &callback : filters_.view()) {
            if (callback(conn_.get(), event.get())) {
                break;
            }
        }
    }
    xcb_flush(conn_.get());
    reader_->wakeUp();
}

bool XCBConnection::filterEvent(xcb_connection_t *,
                                xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    if (response_type == XCB_CLIENT_MESSAGE) {
        auto *client_message =
            reinterpret_cast<xcb_client_message_event_t *>(event);
        if (client_message->window == serverWindow_ &&
            client_message->format == 8 && client_message->type == atom_) {
            ICUUID uuid;
            std::copy(client_message->data.data8,
                      client_message->data.data8 + 16, uuid.begin());
            InputContext *ic =
                parent_->instance()->inputContextManager().findByUUID(uuid);
            if (ic) {
                ic->setFocusGroup(group_);
            }
        }
    } else if (keyboard_->handleEvent(event)) {
        return true;
    } else if (hasXFixes_ && response_type == XCB_XFIXES_SELECTION_NOTIFY +
                                                  xfixesFirstEvent_) {
        auto *selectionNofity =
            reinterpret_cast<xcb_xfixes_selection_notify_event_t *>(event);
        auto callbacks = selections_.view(selectionNofity->selection);
        for (auto &callback : callbacks) {
            callback(selectionNofity->selection);
        }
    } else if (response_type == XCB_SELECTION_NOTIFY) {
        auto *selectionNotify =
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
                constexpr size_t bufLimit = 256;
                xcb_get_property_cookie_t get_prop_cookie = xcb_get_property(
                    conn_.get(), false, serverWindow_,
                    selectionNotify->property, XCB_ATOM_ANY, 0, bufLimit);

                auto reply = makeUniqueCPtr(xcb_get_property_reply(
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

        auto *keypress = reinterpret_cast<xcb_key_press_event_t *>(event);
        if (keypress->event == root_) {
            FCITX_XCB_DEBUG() << "Received key event from root";
            auto sym = xcb_key_press_lookup_keysym(syms_.get(), keypress, 0);
            auto state = keypress->state;
            bool forward;
            Key key(static_cast<KeySym>(sym), KeyStates(state),
                    keypress->detail);
            key = key.normalize();
            if ((forward = key.checkKeyList(forwardGroup_)) ||
                key.checkKeyList(backwardGroup_)) {
                if (keyboardGrabbed_) {
                    navigateGroup(key, forward);
                } else {
                    if (grabXKeyboard()) {
                        groupIndex_ = 0;
                        currentKey_ = key;
                        navigateGroup(key, forward);
                    } else {
                        parent_->instance()
                            ->inputMethodManager()
                            .enumerateGroup(forward);
                    }
                }
            }
            return true;
        }
    } else if (response_type == XCB_KEY_RELEASE) {
        auto *keyrelease = reinterpret_cast<xcb_key_release_event_t *>(event);
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
    for (int i = XCB_MAP_INDEX_SHIFT; i <= XCB_MAP_INDEX_5; ++i) {
        if ((mk & (1 << i)) != 0) {
            if (mod_index >= 0) {
                return;
            }
            mod_index = i;
        }
    }
    bool release = false;
    if (mod_index == -1) {
        release = true;
    } else {
        auto cookie = xcb_get_modifier_mapping(conn_.get());
        auto reply = makeUniqueCPtr(
            xcb_get_modifier_mapping_reply(conn_.get(), cookie, nullptr));
        if (reply) {
            auto *keycodes = xcb_get_modifier_mapping_keycodes(reply.get());
            for (int i = 0; i < reply->keycodes_per_modifier; i++) {
                if (keycodes[reply->keycodes_per_modifier * mod_index + i] ==
                    event->detail) {
                    release = true;
                }
            }
        }
    }
    if (!release) {
        return;
    }
    if (keyboardGrabbed_) {
        acceptGroupChange();
    }
}

void XCBConnection::acceptGroupChange() {
    FCITX_XCB_DEBUG() << "Accept group change";
    if (keyboardGrabbed_) {
        ungrabXKeyboard();
    }

    auto &imManager = parent_->instance()->inputMethodManager();
    auto groups = imManager.groups();
    if (groups.size() > groupIndex_) {
        if (isSingleKey(currentKey_)) {
            imManager.enumerateGroupTo(groups[groupIndex_]);
        } else {
            imManager.setCurrentGroup(groups[groupIndex_]);
        }
    }
    groupIndex_ = 0;
}

void XCBConnection::navigateGroup(const Key &key, bool forward) {
    auto &imManager = parent_->instance()->inputMethodManager();
    if (imManager.groupCount() < 2) {
        return;
    }
    groupIndex_ = (groupIndex_ + (forward ? 1 : imManager.groupCount() - 1)) %
                  imManager.groupCount();
    FCITX_XCB_DEBUG() << "Switch to group " << groupIndex_;

    if (parent_->notifications() && !isSingleKey(key)) {
        parent_->notifications()->call<INotifications::showTip>(
            "enumerate-group", _("Input Method"), "input-keyboard",
            _("Switch group"),
            fmt::format(_("Switch group to {0}"),
                        imManager.groups()[groupIndex_]),
            3000);
    }
}

std::unique_ptr<HandlerTableEntry<XCBEventFilter>>
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
    if (auto *atomP = findValue(atomCache_, atomName)) {
        return *atomP;
    }

    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(conn_.get(), exists, atomName.size(), atomName.c_str());
    auto reply =
        makeUniqueCPtr(xcb_intern_atom_reply(conn_.get(), cookie, nullptr));
    xcb_atom_t result = XCB_ATOM_NONE;
    if (reply) {
        result = reply->atom;
    }
    if (result != XCB_ATOM_NONE || !exists) {
        atomCache_.emplace(std::make_pair(atomName, result));
    }
    return result;
}

xcb_ewmh_connection_t *XCBConnection::ewmh() { return &ewmh_; }

void XCBConnection::setXkbOption(const std::string &option) {
    keyboard_->setXkbOption(option);
}

std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
XCBConnection::addSelection(const std::string &selection,
                            XCBSelectionNotifyCallback callback) {
    auto atomValue = atom(selection, true);
    if (atomValue) {
        return selections_.add(atomValue, std::move(callback));
    }
    return nullptr;
}

std::unique_ptr<HandlerTableEntryBase>
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
                                  std::move(callback));
}

Instance *XCBConnection::instance() { return parent_->instance(); }

struct xkb_state *XCBConnection::xkbState() { return keyboard_->xkbState(); }

XkbRulesNames XCBConnection::xkbRulesNames() {
    return keyboard_->xkbRulesNames();
}

} // namespace fcitx
