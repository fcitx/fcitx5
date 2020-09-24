/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xcbui.h"
#include <xcb/randr.h>
#include <xcb/xcb_aux.h>
#include <xcb/xinerama.h>
#include "fcitx-utils/stringutils.h"
#include "xcbinputwindow.h"
#include "xcbtraywindow.h"

namespace fcitx::classicui {

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

xcb_visualid_t findVisual(xcb_screen_t *screen) {
    auto *visual = xcb_aux_find_visual_by_attrs(screen, -1, 32);
    if (!visual) {
        return screen->root_visual;
    }
    return visual->visual_id;
}

XCBFontOption forcedDpi(xcb_connection_t *conn, xcb_screen_t *screen) {
    int offset = 0;
    std::vector<char> resources;
    bool more = true;
    do {
        auto cookie = xcb_get_property(conn, false, screen->root,
                                       XCB_ATOM_RESOURCE_MANAGER,
                                       XCB_ATOM_STRING, offset / 4, 8192);
        auto reply =
            makeUniqueCPtr(xcb_get_property_reply(conn, cookie, nullptr));
        more = false;
        if (reply && reply->format == 8 && reply->type == XCB_ATOM_STRING) {
            const auto *start =
                static_cast<const char *>(xcb_get_property_value(reply.get()));
            const auto *end =
                start + xcb_get_property_value_length(reply.get());
            resources.insert(resources.end(), start, end);
            offset += xcb_get_property_value_length(reply.get());
            more = reply->bytes_after != 0;
        }
    } while (more);

    XCBFontOption option;
    auto parse = [](const std::vector<char> &resources, const char *item,
                    auto callback) {
        auto iter = resources.begin();
        auto end = resources.end();
        while (iter < end) {
            auto next = std::find(iter, end, '\n');
            int cLen = strlen(item);
            if (next - iter > cLen && std::equal(iter, iter + cLen, item)) {
                std::string value(iter + cLen, next);
                callback(value);
            }

            iter = std::next(next);
        }
    };
    parse(resources, "Xft.dpi:\t", [&option](const std::string &value) {
        try {
            option.dpi = std::stoi(value);
        } catch (const std::exception &e) {
        }
    });
    parse(resources, "Xft.antialias:\t", [&option](const std::string &value) {
        try {
            option.antialias = std::stoi(value) != 0;
        } catch (const std::exception &e) {
        }
    });
    parse(resources, "Xft.hintstyle:\t", [&option](const std::string &value) {
        if (value == "hintfull") {
            option.hint = XCBHintStyle::Full;
        } else if (value == "hintnone") {
            option.hint = XCBHintStyle::NoHint;
        } else if (value == "hintmedium") {
            option.hint = XCBHintStyle::Medium;
        } else if (value == "hintslight") {
            option.hint = XCBHintStyle::Slight;
        }
        // default
    });
    parse(resources, "Xft.rgba:\t", [&option](const std::string &value) {
        if (value == "none") {
            option.rgba = XCBRGBA::NoRGBA;
        } else if (value == "hintnone") {
            option.rgba = XCBRGBA::RGB;
        } else if (value == "hintmedium") {
            option.rgba = XCBRGBA::BGR;
        } else if (value == "hintslight") {
            option.rgba = XCBRGBA::VRGB;
        } else if (value == "hintslight") {
            option.rgba = XCBRGBA::VBGR;
        }
        // default
    });
    return option;
}

void XCBFontOption::setupPangoContext(PangoContext *context) const {
    cairo_hint_style_t hint = CAIRO_HINT_STYLE_DEFAULT;
    cairo_antialias_t aa = CAIRO_ANTIALIAS_DEFAULT;
    cairo_subpixel_order_t subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    switch (this->hint) {
    case XCBHintStyle::NoHint:
        hint = CAIRO_HINT_STYLE_NONE;
        break;
    case XCBHintStyle::Slight:
        hint = CAIRO_HINT_STYLE_SLIGHT;
        break;
    case XCBHintStyle::Medium:
        hint = CAIRO_HINT_STYLE_MEDIUM;
        break;
    case XCBHintStyle::Full:
        hint = CAIRO_HINT_STYLE_FULL;
        break;
    default:
        hint = CAIRO_HINT_STYLE_DEFAULT;
        break;
    }
    switch (rgba) {
    case XCBRGBA::NoRGBA:
        subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
        break;
    case XCBRGBA::RGB:
        subpixel = CAIRO_SUBPIXEL_ORDER_RGB;
        break;
    case XCBRGBA::BGR:
        subpixel = CAIRO_SUBPIXEL_ORDER_BGR;
        break;
    case XCBRGBA::VRGB:
        subpixel = CAIRO_SUBPIXEL_ORDER_VRGB;
        break;
    case XCBRGBA::VBGR:
        subpixel = CAIRO_SUBPIXEL_ORDER_VBGR;
        break;
    default:
        subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
        break;
    }

    if (antialias) {
        if (subpixel != CAIRO_SUBPIXEL_ORDER_DEFAULT) {
            aa = CAIRO_ANTIALIAS_SUBPIXEL;
        } else {
            aa = CAIRO_ANTIALIAS_GRAY;
        }
    } else {
        aa = CAIRO_ANTIALIAS_NONE;
    }

    auto *options = cairo_font_options_create();
    cairo_font_options_set_hint_style(options, hint);
    cairo_font_options_set_subpixel_order(options, subpixel);
    cairo_font_options_set_antialias(options, aa);
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_ON);
    pango_cairo_context_set_font_options(context, options);
    cairo_font_options_destroy(options);
}

XCBUI::XCBUI(ClassicUI *parent, const std::string &name, xcb_connection_t *conn,
             int defaultScreen)
    : parent_(parent), name_(name), conn_(conn), defaultScreen_(defaultScreen) {
    ewmh_ = parent_->xcb()->call<IXCBModule::ewmh>(name_);
    inputWindow_ = std::make_unique<XCBInputWindow>(this);
    trayWindow_ = std::make_unique<XCBTrayWindow>(this);

    compMgrAtomString_ = "_NET_WM_CM_S" + std::to_string(defaultScreen_);
    compMgrAtom_ = parent_->xcb()->call<IXCBModule::atom>(
        name_, compMgrAtomString_, false);

    auto xsettingsSelectionString =
        "_XSETTINGS_S" + std::to_string(defaultScreen_);
    managerAtom_ =
        parent_->xcb()->call<IXCBModule::atom>(name_, "MANAGER", false);
    xsettingsSelectionAtom_ = parent_->xcb()->call<IXCBModule::atom>(
        name_, xsettingsSelectionString, false);
    xsettingsAtom_ = parent_->xcb()->call<IXCBModule::atom>(
        name_, "_XSETTINGS_SETTINGS", false);

    eventHandlers_.emplace_back(parent_->xcb()->call<IXCBModule::addSelection>(
        name, compMgrAtomString_,
        [this](xcb_atom_t) { refreshCompositeManager(); }));

    eventHandlers_.emplace_back(
        parent_->xcb()->call<IXCBModule::addEventFilter>(
            name, [this](xcb_connection_t *, xcb_generic_event_t *event) {
                uint8_t response_type = event->response_type & ~0x80;
                switch (response_type) {
                case XCB_CLIENT_MESSAGE: {
                    auto *client_message =
                        reinterpret_cast<xcb_client_message_event_t *>(event);
                    if (client_message->data.data32[1] == compMgrAtom_) {
                        refreshCompositeManager();
                    } else if (client_message->type == managerAtom_ &&
                               client_message->data.data32[1] ==
                                   xsettingsSelectionAtom_) {
                        CLASSICUI_DEBUG() << "Refresh manager";
                        refreshManager();
                    }
                    break;
                }
                case XCB_DESTROY_NOTIFY: {
                    auto *destroy =
                        reinterpret_cast<xcb_destroy_notify_event_t *>(event);
                    if (destroy->window == xsettingsWindow_) {
                        refreshManager();
                    }
                    break;
                }
                case XCB_PROPERTY_NOTIFY: {
                    auto *property =
                        reinterpret_cast<xcb_property_notify_event_t *>(event);
                    if (xsettingsWindow_ &&
                        property->window == xsettingsWindow_) {
                        readXSettings();
                    }
                    break;
                }
                case XCB_CONFIGURE_NOTIFY: {
                    auto *configure =
                        reinterpret_cast<xcb_configure_notify_event_t *>(event);
                    auto *screen = xcb_aux_get_screen(conn_, defaultScreen_);
                    if (configure->window == screen->root) {
                        initScreen();
                    }
                    break;
                }
                }
                if (multiScreen_ == MultiScreenExtension::Randr &&
                    xrandrFirstEvent_ == XCB_RANDR_NOTIFY) {
                    auto *randr =
                        reinterpret_cast<xcb_randr_notify_event_t *>(event);
                    if (randr->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
                        initScreen();
                    }
                }
                return false;
            }));

    xcb_screen_t *screen = xcb_aux_get_screen(conn_, defaultScreen_);
    addEventMaskToWindow(conn_, screen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY);
    root_ = screen->root;
    fontOption_ = forcedDpi(conn_, screen);
    CLASSICUI_DEBUG() << "Xft.dpi: " << fontOption_.dpi;
    initScreen();
    refreshCompositeManager();
    trayWindow_->initTray();
    refreshManager();
}

XCBUI::~XCBUI() {}

void XCBUI::initScreen() {
    auto *screen = xcb_aux_get_screen(conn_, defaultScreen_);
    int newScreenCount = xcb_setup_roots_length(xcb_get_setup(conn_));
    if (newScreenCount == 1) {
        const xcb_query_extension_reply_t *reply =
            xcb_get_extension_data(conn_, &xcb_randr_id);
        if (reply && reply->present) {
            multiScreen_ = MultiScreenExtension::Randr;
            xrandrFirstEvent_ = reply->first_event;
        } else {
            const xcb_query_extension_reply_t *reply =
                xcb_get_extension_data(conn_, &xcb_xinerama_id);

            if (reply && reply->present) {
                multiScreen_ = MultiScreenExtension::Xinerama;
            }
        }
    }

    maxDpi_ = -1;
    rects_.clear();
    if (multiScreen_ == MultiScreenExtension::Randr) {
        auto cookie =
            xcb_randr_get_screen_resources_current(conn_, screen->root);
        auto reply =
            makeUniqueCPtr(xcb_randr_get_screen_resources_current_reply(
                conn_, cookie, nullptr));
        if (reply) {
            xcb_timestamp_t timestamp = 0;
            xcb_randr_output_t *outputs = nullptr;
            int outputCount =
                xcb_randr_get_screen_resources_current_outputs_length(
                    reply.get());

            if (outputCount) {
                timestamp = reply->config_timestamp;
                outputs =
                    xcb_randr_get_screen_resources_current_outputs(reply.get());
            } else {
                auto resourcesCookie =
                    xcb_randr_get_screen_resources(conn_, screen->root);
                auto resourcesReply =
                    makeUniqueCPtr(xcb_randr_get_screen_resources_reply(
                        conn_, resourcesCookie, nullptr));
                if (resourcesReply) {
                    timestamp = resourcesReply->config_timestamp;
                    outputCount = xcb_randr_get_screen_resources_outputs_length(
                        resourcesReply.get());
                    outputs = xcb_randr_get_screen_resources_outputs(
                        resourcesReply.get());
                }
            }

            if (outputCount) {
                auto primaryCookie =
                    xcb_randr_get_output_primary(conn_, screen->root);
                auto primary =
                    makeUniqueCPtr(xcb_randr_get_output_primary_reply(
                        conn_, primaryCookie, nullptr));
                if (primary) {
                    for (int i = 0; i < outputCount; i++) {
                        auto outputInfoCookie = xcb_randr_get_output_info(
                            conn_, outputs[i], timestamp);
                        auto output =
                            makeUniqueCPtr(xcb_randr_get_output_info_reply(
                                conn_, outputInfoCookie, nullptr));
                        // Invalid, disconnected or disabled output
                        if (!output) {
                            continue;
                        }

                        if (output->connection !=
                            XCB_RANDR_CONNECTION_CONNECTED) {
                            continue;
                        }

                        if (output->crtc == XCB_NONE) {
                            continue;
                        }

                        auto crtcCookie = xcb_randr_get_crtc_info(
                            conn_, output->crtc, output->timestamp);
                        auto crtc =
                            makeUniqueCPtr(xcb_randr_get_crtc_info_reply(
                                conn_, crtcCookie, nullptr));
                        if (crtc) {
                            Rect rect;
                            rect.setPosition(crtc->x, crtc->y);
                            int dpiX = 25.4 * crtc->width / output->mm_width;
                            int dpiY = 25.4 * crtc->height / output->mm_height;
                            if (crtc->rotation ==
                                    XCB_RANDR_ROTATION_ROTATE_90 ||
                                crtc->rotation ==
                                    XCB_RANDR_ROTATION_ROTATE_270) {
                                rect.setSize(crtc->height, crtc->width);
                            } else {
                                rect.setSize(crtc->width, crtc->height);
                            }
                            rects_.emplace_back(rect, std::min(dpiX, dpiY));
                            if (maxDpi_ < rects_.back().second) {
                                maxDpi_ = rects_.back().second;
                            }

                            if (outputs[i] == primary->output) {
                                primaryDpi_ = rects_.back().second;
                            }
                        }
                    }
                }
            }
        }

    } else if (multiScreen_ == MultiScreenExtension::Xinerama) {
        auto cookie = xcb_xinerama_query_screens(conn_);
        if (auto reply = makeUniqueCPtr(
                xcb_xinerama_query_screens_reply(conn_, cookie, nullptr))) {
            xcb_xinerama_screen_info_iterator_t iter;
            for (iter = xcb_xinerama_query_screens_screen_info_iterator(
                     reply.get());
                 iter.rem; xcb_xinerama_screen_info_next(&iter)) {
                auto *info = iter.data;
                auto x = info->x_org;
                auto y = info->y_org;
                auto w = info->width;
                auto h = info->height;
                rects_.emplace_back(Rect(x, y, x + w - 1, y + h - 1), -1);
            }
        }
    }

    if (rects_.empty()) {
        rects_.emplace_back(
            Rect(0, 0, screen->width_in_pixels, screen->height_in_pixels), -1);
    }

    CLASSICUI_DEBUG() << "Screen rects are: " << rects_
                      << " Primary DPI: " << primaryDpi_;
}

void XCBUI::refreshCompositeManager() {
    auto cookie = xcb_get_selection_owner(conn_, compMgrAtom_);
    auto reply =
        makeUniqueCPtr(xcb_get_selection_owner_reply(conn_, cookie, nullptr));
    if (reply) {
        compMgrWindow_ = reply->owner;
    }

    xcb_screen_t *screen = xcb_aux_get_screen(conn_, defaultScreen_);
    if (compMgrWindow_) {
        addEventMaskToWindow(conn_, compMgrWindow_,
                             XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        colorMap_ = xcb_generate_id(conn_);
        xcb_create_colormap(conn_, XCB_COLORMAP_ALLOC_NONE, colorMap_,
                            screen->root, visualId());
    } else {
        colorMap_ = screen->default_colormap;
    }
    inputWindow_->createWindow(visualId());
    // mainWindow_->createWindow();
}

void XCBUI::refreshManager() {
    xcb_grab_server(conn_);
    auto cookie = xcb_get_selection_owner(conn_, xsettingsSelectionAtom_);
    auto reply =
        makeUniqueCPtr(xcb_get_selection_owner_reply(conn_, cookie, nullptr));
    if (reply) {
        xsettingsWindow_ = reply->owner;
    }
    if (xsettingsWindow_) {
        addEventMaskToWindow(conn_, xsettingsWindow_,
                             XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                                 XCB_EVENT_MASK_PROPERTY_CHANGE);
    }
    xcb_ungrab_server(conn_);
    xcb_flush(conn_);

    readXSettings();
}

void XCBUI::readXSettings() {
    if (!xsettingsWindow_) {
        return;
    }

    xcb_grab_server(conn_);
    int offset = 0;
    std::vector<char> data;
    bool more = true;
    bool error = false;
    do {
        auto cookie =
            xcb_get_property(conn_, false, xsettingsWindow_, xsettingsAtom_,
                             xsettingsAtom_, offset / 4, 10);
        auto reply =
            makeUniqueCPtr(xcb_get_property_reply(conn_, cookie, nullptr));
        more = false;
        if (reply && reply->format == 8 && reply->type == xsettingsAtom_) {
            const auto *start =
                static_cast<const char *>(xcb_get_property_value(reply.get()));
            const auto *end =
                start + xcb_get_property_value_length(reply.get());
            data.insert(data.end(), start, end);
            offset += xcb_get_property_value_length(reply.get());
            more = reply->bytes_after != 0;
        }
        if (!reply) {
            error = true;
        }
    } while (more);
    xcb_ungrab_server(conn_);
    xcb_flush(conn_);

    if (error || data.empty()) {
        return;
    }
    enum { BYTE_ORDER_MSB_FIRST = 1, BYTE_ORDER_LSB_FIRST = 0 };
    const uint16_t endian = 1;
    uint8_t byteOrder = 0;
    if (*reinterpret_cast<const char *>(&endian)) {
        byteOrder = BYTE_ORDER_LSB_FIRST;
    } else {
        byteOrder = BYTE_ORDER_MSB_FIRST;
    }
    if (data[0] != BYTE_ORDER_LSB_FIRST && data[0] != BYTE_ORDER_MSB_FIRST) {
        return;
    }

    bool needSwap = byteOrder != data[0];
    auto iter = data.cbegin();
    auto readCard32 = [needSwap, &data, &iter](uint32_t *result) {
        if (std::distance(iter, data.cend()) <
            static_cast<ssize_t>(sizeof(uint32_t))) {
            return false;
        }
        uint32_t x = *reinterpret_cast<const uint32_t *>(&(*iter));

        if (needSwap) {
            *result = (x << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) |
                      (x >> 24);
        } else {
            *result = x;
        }
        iter += sizeof(uint32_t);
        return true;
    };
    auto readCard16 = [needSwap, &data, &iter](uint16_t *result) {
        if (std::distance(iter, data.cend()) <
            static_cast<ssize_t>(sizeof(uint16_t))) {
            return false;
        }
        uint16_t x = *reinterpret_cast<const uint16_t *>(&(*iter));

        if (needSwap) {
            *result = (x << 8) | (x >> 8);
        } else {
            *result = x;
        }
        iter += sizeof(uint16_t);
        return true;
    };
    auto readCard8 = [&data, &iter](uint8_t *result) {
        if (std::distance(iter, data.cend()) <
            static_cast<ssize_t>(sizeof(uint8_t))) {
            return false;
        }
        uint8_t x = *reinterpret_cast<const uint8_t *>(&(*iter));
        *result = x;
        iter += sizeof(uint8_t);
        return true;
    };
    // 1      CARD8    byte-order
    // 3               unused
    // 4      CARD32   SERIAL
    // 4      CARD32   N_SETTINGS
    uint32_t dummy;
    if (!readCard32(&dummy)) {
        return;
    }
    // SERIAL
    if (!readCard32(&dummy)) {
        return;
    }

    uint32_t nSettings;
    if (!readCard32(&nSettings)) {
        return;
    }
    for (uint32_t i = 0; i < nSettings; i++) {
        // 1      SETTING_TYPE  type
        // 1                    unused
        // 2      n             name-len
        // n      STRING8       name
        // P                    unused, p=pad(n)
        // 4      CARD32        last-change-serial
        uint8_t type;
        if (!readCard8(&type)) {
            return;
        }
        // Valid types are 0,1,2.
        if (type > 2) {
            return;
        }
        // Unused
        uint8_t dummy8;
        if (!readCard8(&dummy8)) {
            return;
        }
        uint16_t nameLen;
        if (!readCard16(&nameLen)) {
            return;
        }
#define XSETTINGS_PAD(n, m) ((n + m - 1) & (~(m - 1)))
        uint32_t namePad = XSETTINGS_PAD(nameLen, 4);
        if (std::distance(iter, data.cend()) < namePad) {
            return;
        }
        std::string_view name(&(*iter), nameLen);
        iter += namePad;
        if (!readCard32(&dummy)) {
            return;
        }
        switch (type) {
        case 0: // Integer
            if (!readCard32(&dummy)) {
                return;
            }
            break;
        case 1: {
            // String
            uint32_t len;
            if (!readCard32(&len)) {
                return;
            }
            uint32_t lenPad = XSETTINGS_PAD(len, 4);
            if (std::distance(iter, data.cend()) < lenPad) {
                return;
            }
            std::string_view value(&(*iter), len);
            iter += lenPad;
            if (name == "Net/IconThemeName" && !value.empty()) {
                iconThemeName_ = value;
                if (parent()->theme().setIconTheme(iconThemeName_)) {
                    trayWindow_->update();
                }
            }
            break;
        }
        case 2: // Color
            // 4 card 16, just do it with 2 card32
            if (!readCard32(&dummy)) {
                return;
            }
            if (!readCard32(&dummy)) {
                return;
            }
            break;
        }
    }
}

xcb_visualid_t XCBUI::visualId() const {
    xcb_screen_t *screen = xcb_aux_get_screen(conn_, defaultScreen_);
    if (compMgrWindow_) {
        return findVisual(screen);
    }
    return screen->root_visual;
}

void XCBUI::update(UserInterfaceComponent component,
                   InputContext *inputContext) {
    if (component == UserInterfaceComponent::InputPanel) {
        inputWindow_->update(inputContext);
    }
}

void XCBUI::updateCursor(InputContext *inputContext) {
    inputWindow_->updatePosition(inputContext);
}

void XCBUI::updateCurrentInputMethod(InputContext *) { trayWindow_->update(); }

int XCBUI::dpiByPosition(int x, int y) {
    int shortestDistance = INT_MAX;
    int screenDpi = -1;
    for (const auto &rect : screenRects()) {
        int thisDistance = rect.first.distance(x, y);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            screenDpi = rect.second;
        }
    }

    return scaledDPI(screenDpi);
}

int XCBUI::scaledDPI(int dpi) {
    if (!*parent_->config().perScreenDPI) {
        // CLASSICUI_DEBUG() << "Use font dpi: " << fontOption_.dpi;
        return fontOption_.dpi;
    }
    if (dpi < 0) {
        return fontOption_.dpi;
    }

    double targetDPI;
    if (fontOption_.dpi < 0) {
        targetDPI = dpi;
    } else {
        auto baseDPI = primaryDpi_ > 0 ? primaryDpi_ : maxDpi_;
        targetDPI = (static_cast<double>(dpi) / baseDPI) * fontOption_.dpi;
    }
    double scale = targetDPI / 96;
    if (scale < 1) {
        targetDPI = 96;
    }
    return targetDPI;
}

void XCBUI::resume() { updateTray(); }

void XCBUI::suspend() {
    inputWindow_->update(nullptr);
    updateTray();
}

void XCBUI::updateTray() {
    bool enableTray = enableTray_ && !parent_->suspended();
    if (enableTray) {
        trayWindow_->resume();
    } else {
        trayWindow_->suspend();
    }
}

void XCBUI::setEnableTray(bool enable) {
    if (enable != enableTray_) {
        enableTray_ = enable;
        updateTray();
    }
}
} // namespace fcitx::classicui
