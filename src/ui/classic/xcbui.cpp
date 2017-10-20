/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "xcbui.h"
#include "xcbinputwindow.h"
#include "xcbtraywindow.h"
#include <fcitx-utils/stringutils.h>
#include <xcb/randr.h>
#include <xcb/xcb_aux.h>
#include <xcb/xinerama.h>

namespace fcitx {
namespace classicui {

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

xcb_visualid_t findVisual(xcb_screen_t *screen) {
    auto visual = xcb_aux_find_visual_by_attrs(screen, -1, 32);
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
            makeXCBReply(xcb_get_property_reply(conn, cookie, nullptr));
        more = false;
        if (reply && reply->format == 8 && reply->type == XCB_ATOM_STRING) {
            auto start =
                static_cast<const char *>(xcb_get_property_value(reply.get()));
            auto end = start + xcb_get_property_value_length(reply.get());
            resources.insert(resources.end(), start, end);
            offset += xcb_get_property_value_length(reply.get());
            more = reply->bytes_after != 0;
        }
    } while (more);

    XCBFontOption option;
    auto parse = [](const std::vector<char> resources, const char *item,
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
            option.hint = XCBHintStyle::None;
        } else if (value == "hintmedium") {
            option.hint = XCBHintStyle::Medium;
        } else if (value == "hintslight") {
            option.hint = XCBHintStyle::Slight;
        }
        // default
    });
    parse(resources, "Xft.rgba:\t", [&option](const std::string &value) {
        if (value == "none") {
            option.rgba = XCBRGBA::None;
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

XCBUI::XCBUI(ClassicUI *parent, const std::string &name, xcb_connection_t *conn,
             int defaultScreen)
    : parent_(parent), name_(name), conn_(conn), defaultScreen_(defaultScreen) {
    ewmh_ = parent_->xcb()->call<IXCBModule::ewmh>(name_);
    inputWindow_ = std::make_unique<XCBInputWindow>(this);
    trayWindow_ = std::make_unique<XCBTrayWindow>(this);

    compMgrAtomString_ = "_NET_WM_CM_S" + std::to_string(defaultScreen_);
    compMgrAtom_ = parent_->xcb()->call<IXCBModule::atom>(
        name_, compMgrAtomString_, false);

    parent_->xcb()->call<IXCBModule::addSelection>(
        name, compMgrAtomString_,
        [this](xcb_atom_t) { refreshCompositeManager(); });

    parent_->xcb()->call<IXCBModule::addEventFilter>(
        name, [this](xcb_connection_t *, xcb_generic_event_t *event) {
            uint8_t response_type = event->response_type & ~0x80;
            switch (response_type) {
            case XCB_CLIENT_MESSAGE: {
                auto client_message =
                    reinterpret_cast<xcb_client_message_event_t *>(event);
                if (client_message->data.data32[1] == compMgrAtom_) {
                    refreshCompositeManager();
                }
                break;
            }
            case XCB_CONFIGURE_NOTIFY: {
                auto configure =
                    reinterpret_cast<xcb_configure_notify_event_t *>(event);
                auto screen = xcb_aux_get_screen(conn_, defaultScreen_);
                if (configure->window == screen->root) {
                    initScreen();
                }
                break;
            }
            }
            if (multiScreen_ == MultiScreenExtension::Randr &&
                xrandrFirstEvent_ == XCB_RANDR_NOTIFY) {
                auto randr =
                    reinterpret_cast<xcb_randr_notify_event_t *>(event);
                if (randr->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
                    initScreen();
                }
            }
            return false;
        });

    xcb_screen_t *screen = xcb_aux_get_screen(conn_, defaultScreen_);
    addEventMaskToWindow(conn_, screen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY);
    fontOption_ = forcedDpi(conn_, screen);
    initScreen();
    refreshCompositeManager();
    trayWindow_->initTray();
}

XCBUI::~XCBUI() {}

void XCBUI::initScreen() {
    auto screen = xcb_aux_get_screen(conn_, defaultScreen_);
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
        auto reply = makeXCBReply(xcb_randr_get_screen_resources_current_reply(
            conn_, cookie, nullptr));
        if (reply) {
            xcb_timestamp_t timestamp = 0;
            xcb_randr_output_t *outputs = nullptr;
            int outputCount =
                xcb_randr_get_screen_resources_current_outputs_length(
                    reply.get());

            std::unique_ptr<xcb_randr_get_screen_resources_reply_t,
                            decltype(&std::free)>
                resourcesReply(nullptr, &std::free);
            if (outputCount) {
                timestamp = reply->config_timestamp;
                outputs =
                    xcb_randr_get_screen_resources_current_outputs(reply.get());
            } else {
                auto resourcesCookie =
                    xcb_randr_get_screen_resources(conn_, screen->root);
                resourcesReply =
                    makeXCBReply(xcb_randr_get_screen_resources_reply(
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
                auto primary = makeXCBReply(xcb_randr_get_output_primary_reply(
                    conn_, primaryCookie, nullptr));
                if (primary) {
                    for (int i = 0; i < outputCount; i++) {
                        auto outputInfoCookie = xcb_randr_get_output_info(
                            conn_, outputs[i], timestamp);
                        auto output =
                            makeXCBReply(xcb_randr_get_output_info_reply(
                                conn_, outputInfoCookie, nullptr));
                        // Invalid, disconnected or disabled output
                        if (!output)
                            continue;

                        if (output->connection !=
                            XCB_RANDR_CONNECTION_CONNECTED) {
                            continue;
                        }

                        if (output->crtc == XCB_NONE) {
                            continue;
                        }

                        auto crtcCookie = xcb_randr_get_crtc_info(
                            conn_, output->crtc, output->timestamp);
                        auto crtc = makeXCBReply(xcb_randr_get_crtc_info_reply(
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
                        }
                    }
                }
            }
        }

    } else if (multiScreen_ == MultiScreenExtension::Xinerama) {
        auto cookie = xcb_xinerama_query_screens(conn_);
        if (auto reply = makeXCBReply(
                xcb_xinerama_query_screens_reply(conn_, cookie, nullptr))) {
            xcb_xinerama_screen_info_iterator_t iter;
            for (iter = xcb_xinerama_query_screens_screen_info_iterator(
                     reply.get());
                 iter.rem; xcb_xinerama_screen_info_next(&iter)) {
                auto info = iter.data;
                auto x = info->x_org;
                auto y = info->y_org;
                auto w = info->width;
                auto h = info->height;
                rects_.emplace_back(Rect(x, y, x + w - 1, y + h - 1), -1);
            }
        }
    }

    if (!rects_.size()) {
        rects_.emplace_back(
            Rect(0, 0, screen->width_in_pixels, screen->height_in_pixels), -1);
    }
}

void XCBUI::refreshCompositeManager() {
    auto cookie = xcb_get_selection_owner(conn_, compMgrAtom_);
    auto reply =
        makeXCBReply(xcb_get_selection_owner_reply(conn_, cookie, nullptr));
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
    inputWindow_->createWindow();
    // mainWindow_->createWindow();
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

int XCBUI::dpi(int dpi) {
    if (dpi < 0) {
        return fontOption_.dpi;
    }
    if (fontOption_.dpi < 0) {
        return dpi;
    }
    return (static_cast<double>(dpi) / maxDpi_) * fontOption_.dpi;
}

void XCBUI::resume() { trayWindow_->resume(); }

void XCBUI::suspend() {
    inputWindow_->update(nullptr);
    trayWindow_->suspend();
}
}
}
