/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "config.h"

// workaround xkb.h using explicit keyword problem
#define explicit no_cxx_explicit
#include <xcb/xkb.h>
#undef explicit

#include <xcb/xcbext.h>
#include <xkbcommon/xkbcommon-x11.h>
#include "fcitx-utils/log.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/misc_p.h"

#ifdef ENABLE_DBUS
#include "dbus_public.h"
#endif

#include "xcb_public.h"
#include "xcbconnection.h"
#include "xcbkeyboard.h"
#include "xcbmodule.h"

// Hack the files so we don't need to include libx11 files.

#ifndef Bool
#define Bool int
#endif

#ifndef _XFUNCPROTOBEGIN
#define _XFUNCPROTOBEGIN extern "C" {
#endif

#ifndef _XFUNCPROTOEND
#define _XFUNCPROTOEND }
#endif
typedef unsigned long Atom;
typedef unsigned char KeyCode;
#define KeySym uint32_t
typedef struct _XDisplay Display;
#include <X11/extensions/XKBstr.h>
// This empty line prevent clang-format to order this two files.
#include <X11/extensions/XKBrules.h>
#undef KeySym

namespace fcitx {

namespace {

std::string xmodmapFile() {
    auto *home = getenv("HOME");
    if (!home) {
        return {};
    }
    auto path = stringutils::joinPath(home, ".Xmodmap");
    if (!fs::isreg(path)) {
        path = stringutils::joinPath(home, ".xmodmap");
    }
    if (!fs::isreg(path)) {
        return {};
    }
    return path;
}

} // namespace

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

void addEventMaskToWindow(xcb_connection_t *conn, xcb_window_t wid,
                          uint32_t mask) {
    auto get_attr_cookie = xcb_get_window_attributes(conn, wid);
    auto get_attr_reply = makeUniqueCPtr(
        xcb_get_window_attributes_reply(conn, get_attr_cookie, nullptr));
    if (get_attr_reply && (get_attr_reply->your_event_mask & mask) != mask) {
        const uint32_t newMask = get_attr_reply->your_event_mask | mask;
        xcb_change_window_attributes(conn, wid, XCB_CW_EVENT_MASK, &newMask);
    }
}

XCBKeyboard::XCBKeyboard(XCBConnection *conn) : conn_(conn) {
    // init xkb, query if extension exists.
    const xcb_query_extension_reply_t *reply =
        xcb_get_extension_data(connection(), &xcb_xkb_id);
    if (!reply || !reply->present) {
        return;
    }
    xkbFirstEvent_ = reply->first_event;
    xkbMajorOpCode_ = reply->major_opcode;
    xcb_xkb_use_extension_cookie_t xkb_query_cookie;

    // Check if the version matches.
    xkb_query_cookie =
        xcb_xkb_use_extension(connection(), XKB_X11_MIN_MAJOR_XKB_VERSION,
                              XKB_X11_MIN_MINOR_XKB_VERSION);
    auto xkb_query = makeUniqueCPtr(
        xcb_xkb_use_extension_reply(connection(), xkb_query_cookie, nullptr));

    if (!xkb_query || !xkb_query->supported) {
        return;
    }
    coreDeviceId_ = xkb_x11_get_core_keyboard_device_id(connection());

    const uint16_t required_map_parts =
        (XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
         XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
         XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_KEY_BEHAVIORS |
         XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP);

    const uint16_t required_events =
        (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
         XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_STATE_NOTIFY);

    // XKB events are reported to all interested clients without
    // regard to the current keyboard input focus or grab state.
    xcb_void_cookie_t select = xcb_xkb_select_events_checked(
        connection(), XCB_XKB_ID_USE_CORE_KBD, required_events, 0,
        required_events, required_map_parts, required_map_parts, 0);
    auto error = makeUniqueCPtr(xcb_request_check(connection(), select));
    if (error) {
        return;
    }
    hasXKB_ = true;
    updateKeymap();
    addEventMaskToWindow(connection(), conn_->root(),
                         XCB_EVENT_MASK_PROPERTY_CHANGE);

    // Force refresh so we can apply xmodmap.
    if (conn_->parent()->config().allowOverrideXKB.value() &&
        !xmodmapFile().empty()) {
        setRMLVOToServer(xkbRule_, xkbModel_,
                         stringutils::join(defaultLayouts_, ","),
                         stringutils::join(defaultVariants_, ","), xkbOptions_);
    }

    eventHandlers_.emplace_back(conn_->instance()->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            if (!hasXKB_ ||
                !conn_->parent()->config().allowOverrideXKB.value()) {
                return;
            }
            auto layoutAndVariant = parseLayout(conn_->instance()
                                                    ->inputMethodManager()
                                                    .currentGroup()
                                                    .defaultLayout());
            FCITX_XCB_DEBUG() << layoutAndVariant;
            setLayoutByName(layoutAndVariant.first, layoutAndVariant.second);
        }));
}

void XCBKeyboard::updateKeymap() {
    if (!context_) {
        context_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        xkb_context_set_log_level(context_.get(), XKB_LOG_LEVEL_CRITICAL);
    }

    if (!context_) {
        return;
    }
    xcb_flush(connection());
    initDefaultLayout();

    keymap_.reset(nullptr);

    struct xkb_state *new_state = nullptr;
    if (hasXKB_) {
        keymap_.reset(xkb_x11_keymap_new_from_device(
            context_.get(), connection(), coreDeviceId_,
            XKB_KEYMAP_COMPILE_NO_FLAGS));
        if (keymap_) {
            new_state = xkb_x11_state_new_from_device(
                keymap_.get(), connection(), coreDeviceId_);
        }
    }

    if (!keymap_) {

        if (!xkbRule_.empty()) {
            struct xkb_rule_names xkbNames;
            auto layout = stringutils::join(defaultLayouts_, ',');
            auto variant = stringutils::join(defaultVariants_, ',');
            xkbNames.rules = xkbRule_.c_str();
            xkbNames.model = xkbModel_.c_str();
            xkbNames.layout = layout.c_str();
            xkbNames.variant = variant.c_str();
            xkbNames.options = xkbOptions_.c_str();

            keymap_.reset(xkb_keymap_new_from_names(
                context_.get(), &xkbNames, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }

        if (!keymap_) {
            struct xkb_rule_names xkbNames;
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

xcb_atom_t XCBKeyboard::xkbRulesNamesAtom() {
    if (!xkbRulesNamesAtom_) {
        xkbRulesNamesAtom_ = conn_->atom(_XKB_RF_NAMES_PROP_ATOM, true);
    }
    return xkbRulesNamesAtom_;
}

XkbRulesNames XCBKeyboard::xkbRulesNames() {
    if (!xkbRulesNamesAtom()) {
        return {};
    }

    xcb_get_property_cookie_t get_prop_cookie =
        xcb_get_property(connection(), false, conn_->root(),
                         xkbRulesNamesAtom(), XCB_ATOM_STRING, 0, 1024);
    auto reply = makeUniqueCPtr(
        xcb_get_property_reply(connection(), get_prop_cookie, nullptr));

    if (!reply || reply->type != XCB_ATOM_STRING || reply->bytes_after > 0 ||
        reply->format != 8) {
        return {};
    }

    auto *data = static_cast<char *>(xcb_get_property_value(reply.get()));
    int length = xcb_get_property_value_length(reply.get());

    XkbRulesNames names;
    if (length) {
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

void XCBKeyboard::initDefaultLayout() {
    auto names = xkbRulesNames();
    conn_->instance()->setXkbParameters(conn_->focusGroup()->display(),
                                        names[0], names[1], names[4]);

    FCITX_XCB_DEBUG() << names[0] << " " << names[1] << " " << names[2] << " "
                      << names[3] << " " << names[4];

    if (!names[0].empty()) {
        xkbRule_ = names[0];
        xkbModel_ = names[1];
        xkbOptions_ = names[4];
        defaultLayouts_ = stringutils::split(
            names[2], ",", stringutils::SplitBehavior::KeepEmpty);
        defaultVariants_ = stringutils::split(
            names[3], ",", stringutils::SplitBehavior::KeepEmpty);
    } else {
        xkbRule_ = DEFAULT_XKB_RULES;
        xkbModel_ = "pc101";
        defaultLayouts_ = {"us"};
        defaultVariants_ = {""};
        xkbOptions_ = "";
    }
}

int XCBKeyboard::findLayoutIndex(const std::string &layout,
                                 const std::string &variant) const {
    FCITX_XCB_DEBUG() << "findLayoutIndex layout:" << layout
                      << " variant:" << variant;
    FCITX_XCB_DEBUG() << "defaultLayouts:" << defaultLayouts_;
    FCITX_XCB_DEBUG() << "defaultVariants:" << defaultVariants_;
    for (size_t i = 0; i < defaultLayouts_.size(); i++) {
        if (defaultLayouts_[i] == layout &&
            ((i < defaultVariants_.size() && variant == defaultVariants_[i]) ||
             (i >= defaultVariants_.size() && variant.empty()))) {
            return i;
        }
    }
    return -1;
}

int XCBKeyboard::findOrAddLayout(const std::string &layout,
                                 const std::string &variant) {
    addNewLayout(layout, variant);
    initDefaultLayout();
    return findLayoutIndex(layout, variant);
}

void XCBKeyboard::addNewLayout(const std::string &layout,
                               const std::string &variant) {
    FCITX_XCB_DEBUG() << "addNewLayout " << layout << " " << variant;

    if (*conn_->parent()->config().alwaysSetToGroupLayout) {
        defaultLayouts_.clear();
        defaultVariants_.clear();
        defaultLayouts_.push_back(layout);
        defaultVariants_.push_back(variant);
    } else {
        while (defaultVariants_.size() < defaultLayouts_.size()) {
            defaultVariants_.emplace_back();
        }

        while (defaultVariants_.size() > defaultLayouts_.size()) {
            defaultVariants_.pop_back();
        }
        auto index = findLayoutIndex(layout, variant);
        if (index == 0) {
            return;
        }

        if (index > 0) {
            defaultLayouts_.erase(std::next(defaultLayouts_.begin(), index));
            defaultVariants_.erase(std::next(defaultVariants_.begin(), index));
        }
        while (defaultLayouts_.size() >= 4) {
            defaultLayouts_.pop_back();
            defaultVariants_.pop_back();
        }
        defaultLayouts_.insert(defaultLayouts_.begin(), layout);
        defaultVariants_.insert(defaultVariants_.begin(), variant);
    }

    setRMLVOToServer(xkbRule_, xkbModel_,
                     stringutils::join(defaultLayouts_, ","),
                     stringutils::join(defaultVariants_, ","), xkbOptions_);
}

void XCBKeyboard::setRMLVOToServer(const std::string &rule,
                                   const std::string &model,
                                   const std::string &layout,
                                   const std::string &variant,
                                   const std::string &options) {
    FCITX_XCB_DEBUG() << "RMLVO tuple: " << rule << " " << model << " "
                      << layout << " " << variant;
    // xcb_xkb_get_kbd_by_name() doesn't fill the buffer for us, need to it
    // ourselves.
    char locale[] = "C";
    std::string ruleFile;
    XkbRF_RulesPtr rules = nullptr;
    if (!rule.empty()) {
        if (rule[0] != '/') {
            ruleFile =
                stringutils::joinPath(XKEYBOARDCONFIG_XKBBASE, "rules", rule);
            rules = XkbRF_Load(ruleFile.data(), locale, true, true);
        }
    }
    if (!rules) {
        char defaultRule[] =
            XKEYBOARDCONFIG_XKBBASE "/rules/" DEFAULT_XKB_RULES;
        rules = XkbRF_Load(defaultRule, locale, true, true);
    }

    if (!rules) {
        FCITX_WARN() << "Could not load XKB rules";
        return;
    }

    XkbRF_VarDefsRec rdefs;
    XkbComponentNamesRec rnames;
    memset(&rdefs, 0, sizeof(XkbRF_VarDefsRec));
    memset(&rnames, 0, sizeof(XkbComponentNamesRec));
    rdefs.model = !model.empty() ? strdup(model.data()) : nullptr;
    rdefs.layout = !layout.empty() ? strdup(layout.data()) : nullptr;
    rdefs.variant = !variant.empty() ? strdup(variant.data()) : nullptr;
    rdefs.options = !options.empty() ? strdup(options.data()) : nullptr;
    XkbRF_GetComponents(rules, &rdefs, &rnames);

    int keymapLen, keycodesLen, typesLen, compatLen, symbolsLen, geometryLen;
    keymapLen = keycodesLen = typesLen = compatLen = symbolsLen = geometryLen =
        0;
#define SET_LENGTH(FIELD)                                                      \
    do {                                                                       \
        if (rnames.FIELD) {                                                    \
            FIELD##Len = static_cast<int>(strlen(rnames.FIELD));               \
            if (FIELD##Len > 255) {                                            \
                FIELD##Len = 255;                                              \
            }                                                                  \
        }                                                                      \
    } while (0)
    SET_LENGTH(keymap);
    SET_LENGTH(keycodes);
    SET_LENGTH(types);
    SET_LENGTH(compat);
    SET_LENGTH(symbols);
    SET_LENGTH(geometry);

    auto len = keymapLen + keycodesLen + typesLen + compatLen + symbolsLen +
               geometryLen + 6;
#define XkbPaddedSize(n) ((((unsigned int)(n) + 3) >> 2) << 2)
    len = XkbPaddedSize(len);
    UniqueCPtr<xcb_xkb_get_kbd_by_name_request_t> request(
        (static_cast<xcb_xkb_get_kbd_by_name_request_t *>(
            calloc(1, sizeof(xcb_xkb_get_kbd_by_name_request_t) + len))));
    auto *data = reinterpret_cast<char *>(request.get() + 1);

    request->major_opcode = xkbMajorOpCode_;
    request->minor_opcode = XCB_XKB_GET_KBD_BY_NAME;
    request->deviceSpec = XCB_XKB_ID_USE_CORE_KBD;
    request->need = XkbGBN_AllComponentsMask;
    request->want = XkbGBN_AllComponentsMask & (~XkbGBN_GeometryMask);
    request->load = true;
    request->length = (sizeof(xcb_xkb_get_kbd_by_name_request_t) + len) / 4;

#define WRITE_DATA(FIELD)                                                      \
    do {                                                                       \
        *data = FIELD##Len;                                                    \
        data += 1;                                                             \
        if (FIELD##Len) {                                                      \
            memcpy(data, rnames.FIELD, FIELD##Len);                            \
            data += FIELD##Len;                                                \
        }                                                                      \
    } while (0)
    WRITE_DATA(keymap);
    WRITE_DATA(keycodes);
    WRITE_DATA(types);
    WRITE_DATA(compat);
    WRITE_DATA(symbols);
    WRITE_DATA(geometry);
    static const xcb_protocol_request_t xcb_req = {.count = 2,
                                                   .ext = &xcb_xkb_id,
                                                   .opcode =
                                                       XCB_XKB_GET_KBD_BY_NAME,
                                                   .isvoid = 0};

    struct iovec xcb_parts[4];
    xcb_xkb_get_kbd_by_name_cookie_t xcb_ret;

    xcb_parts[2].iov_base = reinterpret_cast<void *>(request.get());
    xcb_parts[2].iov_len = sizeof(xcb_xkb_get_kbd_by_name_request_t) + len;
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(connection(), XCB_REQUEST_CHECKED,
                                        xcb_parts + 2, &xcb_req);
    auto reply = makeUniqueCPtr(
        xcb_xkb_get_kbd_by_name_reply(connection(), xcb_ret, nullptr));

    XkbRF_Free(rules, true);
    free(rnames.keymap);
    free(rnames.keycodes);
    free(rnames.types);
    free(rnames.compat);
    free(rnames.symbols);
    free(rnames.geometry);

    free(rdefs.model);
    free(rdefs.layout);
    free(rdefs.variant);
    free(rdefs.options);

    if (reply) {
        std::vector<char> propData;
        propData.insert(propData.end(), rule.begin(), rule.end());
        propData.push_back(0);
        propData.insert(propData.end(), model.begin(), model.end());
        propData.push_back(0);
        propData.insert(propData.end(), layout.begin(), layout.end());
        propData.push_back(0);
        propData.insert(propData.end(), variant.begin(), variant.end());
        propData.push_back(0);
        propData.insert(propData.end(), options.begin(), options.end());
        propData.push_back(0);
        xcb_change_property(connection(), XCB_PROP_MODE_REPLACE, conn_->root(),
                            conn_->atom(_XKB_RF_NAMES_PROP_ATOM, false),
                            XCB_ATOM_STRING, 8, propData.size(),
                            propData.data());
    }
    waitingForRefresh_ = true;
}

bool XCBKeyboard::setLayoutByName(const std::string &layout,
                                  const std::string &variant) {
    int index;
    index = findOrAddLayout(layout, variant);
    if (index < 0) {
        return false;
    }

    FCITX_XCB_DEBUG() << "Lock group " << index;
#ifdef ENABLE_DBUS
    auto *addon = conn_->instance()->addonManager().addon("dbus", true);
    if (addon && addon->call<IDBusModule::lockGroup>(index)) {
        return true;
    }
#endif
    xcb_xkb_latch_lock_state(connection(), XCB_XKB_ID_USE_CORE_KBD, 0, 0, true,
                             index, 0, false, 0);
    xcb_flush(connection());
    return true;
}

bool XCBKeyboard::handleEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    if (!hasXKB_) {
        return false;
    }

    if (response_type == XCB_PROPERTY_NOTIFY) {
        auto *property = reinterpret_cast<xcb_property_notify_event_t *>(event);
        if (property->window == conn_->root() &&
            property->atom == xkbRulesNamesAtom()) {
            updateKeymap();
        }
        return false;
    }

    if (response_type != xkbFirstEvent_) {
        return false;
    }
    _xkb_event *xkbEvent = (_xkb_event *)event;
    if (xkbEvent->any.deviceID == coreDeviceId_) {
        switch (xkbEvent->any.xkbType) {
        case XCB_XKB_STATE_NOTIFY: {
            xcb_xkb_state_notify_event_t *state = &xkbEvent->state_notify;
            xkb_state_update_mask(state_.get(), state->baseMods,
                                  state->latchedMods, state->lockedMods,
                                  state->baseGroup, state->latchedGroup,
                                  state->lockedGroup);
            conn_->instance()->updateXkbStateMask(
                conn_->focusGroup()->display(), state->baseMods,
                state->latchedMods, state->lockedMods);
            return true;
        }
        case XCB_XKB_MAP_NOTIFY: {
            FCITX_XCB_DEBUG() << "XCB_XKB_MAP_NOTIFY";
            updateKeymap();
            return true;
        }
        case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
            xcb_xkb_new_keyboard_notify_event_t *ev =
                &xkbEvent->new_keyboard_notify;
            FCITX_XCB_DEBUG() << "XCB_XKB_NEW_KEYBOARD_NOTIFY";
            if (ev->changed & XCB_XKB_NKN_DETAIL_KEYCODES) {
                updateKeymapEvent_ =
                    conn_->instance()->eventLoop().addTimeEvent(
                        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 10000, 0,
                        [this](EventSourceTime *, uint64_t) {
                            updateKeymap();
                            return true;
                        });
            }

            if (!*conn_->parent()->config().allowOverrideXKB) {
                break;
            }

            if (ev->sequence != lastSequence_) {
                lastSequence_ = ev->sequence;
                xmodmapTimer_ = conn_->instance()->eventLoop().addTimeEvent(
                    CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 15000, 0,
                    [this](EventSourceTime *, uint64_t) {
                        FCITX_XCB_DEBUG() << "Apply Xmodmap.";

                        if (waitingForRefresh_) {
                            waitingForRefresh_ = false;
                            if (auto path = xmodmapFile(); !path.empty()) {
                                startProcess({"xmodmap", path});
                            }
                        }
                        return true;
                    });
            }
            break;
        }
        }
    }
    return true;
}

xcb_connection_t *XCBKeyboard::connection() { return conn_->connection(); }

} // namespace fcitx
