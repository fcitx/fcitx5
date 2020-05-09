/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbtraywindow.h"
#include <unistd.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include "fcitx-utils/i18n.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/statusarea.h"
#include "fcitx/userinterfacemanager.h"
#include "common.h"
#include "xcbmenu.h"

namespace fcitx {
namespace classicui {

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define ATOM_SELECTION 0
#define ATOM_MANAGER 1
#define ATOM_SYSTEM_TRAY_OPCODE 2
#define ATOM_ORIENTATION 3
#define ATOM_VISUAL 4

XCBTrayWindow::XCBTrayWindow(XCBUI *ui) : XCBWindow(ui, 48, 48) {
    for (auto &separator : separatorActions_) {
        separator.setSeparator(true);
    }
    groupAction_.setShortText(_("Group"));
    groupAction_.setMenu(&groupMenu_);
    inputMethodAction_.setShortText(_("Input Method"));
    inputMethodAction_.setMenu(&inputMethodMenu_);
    configureAction_.setShortText(_("Configure"));
    restartAction_.setShortText(_("Restart"));
    exitAction_.setShortText(_("Exit"));
    menu_.addAction(&groupAction_);
    menu_.addAction(&inputMethodAction_);
    menu_.addAction(&separatorActions_[0]);
    menu_.addAction(&configureAction_);
    menu_.addAction(&restartAction_);
    menu_.addAction(&exitAction_);

    configureAction_.connect<SimpleAction::Activated>(
        [this](InputContext *) { ui_->parent()->instance()->configure(); });
    restartAction_.connect<SimpleAction::Activated>(
        [this](InputContext *) { ui_->parent()->instance()->restart(); });
    exitAction_.connect<SimpleAction::Activated>(
        [this](InputContext *) { ui_->parent()->instance()->exit(); });

    auto &uiManager = ui_->parent()->instance()->userInterfaceManager();
    uiManager.registerAction(&groupAction_);
    uiManager.registerAction(&inputMethodAction_);
    uiManager.registerAction(&configureAction_);
    uiManager.registerAction(&restartAction_);
    uiManager.registerAction(&exitAction_);

#if 0
    inputMethodMenu_.addAction(&testAction1_);
    inputMethodMenu_.addAction(&testAction2_);
    testAction1_.setMenu(&testMenu1_);
    testAction2_.setMenu(&testMenu2_);
    testMenu1_.addAction(&testSubAction1_);
    testMenu2_.addAction(&testSubAction2_);
    testAction1_.setShortText("TEST1");
    testAction2_.setShortText("TEST2");
    testSubAction1_.setShortText("TESTSUB1");
    testSubAction2_.setShortText("TESTSUB2");
    uiManager.registerAction(&testAction1_);
    uiManager.registerAction(&testAction2_);
    uiManager.registerAction(&testSubAction1_);
    uiManager.registerAction(&testSubAction2_);
#endif
}

bool XCBTrayWindow::filterEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    switch (response_type) {
    case XCB_CLIENT_MESSAGE: {
        auto client_message =
            reinterpret_cast<xcb_client_message_event_t *>(event);
        if (client_message->type == atoms_[ATOM_MANAGER] &&
            client_message->format == 32 &&
            client_message->data.data32[1] == atoms_[ATOM_SELECTION] &&
            dockWindow_ == XCB_WINDOW_NONE) {
            refreshDockWindow();
            return true;
        }
        break;
    }

    case XCB_BUTTON_PRESS: {
        auto press = reinterpret_cast<xcb_button_press_event_t *>(event);
        if (press->event == wid_) {
            if (press->detail == XCB_BUTTON_INDEX_3) {
                updateMenu();
                XCBMenu *menu = menuPool_.requestMenu(ui_, &menu_, nullptr);
                menu->show(Rect()
                               .setPosition(press->root_x, press->root_y)
                               .setSize(1, 1));
            } else if (press->detail == XCB_BUTTON_INDEX_1) {
                ui_->parent()->instance()->toggle();
            }
            return true;
        }
        break;
    }

    case XCB_EXPOSE: {
        auto expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            CLASSICUI_DEBUG() << "Tray recevied expose event";
            update();
        }
        break;
    }
    case XCB_CONFIGURE_NOTIFY: {
        auto configure =
            reinterpret_cast<xcb_configure_notify_event_t *>(event);
        if (wid_ == configure->event) {
            CLASSICUI_DEBUG() << "Tray recevied configure event";
            if (width() != configure->width && height() != configure->height) {
                resize(configure->width, configure->height);
                xcb_size_hints_t size_hints;
                memset(&size_hints, 0, sizeof(size_hints));
                size_hints.flags = XCB_ICCCM_SIZE_HINT_BASE_SIZE;
                size_hints.base_width = configure->width;
                size_hints.base_height = configure->height;
                xcb_icccm_set_wm_normal_hints(ui_->connection(), wid_,
                                              &size_hints);
            }
            return true;
        }
        break;
    }
    case XCB_DESTROY_NOTIFY: {
        auto destroywindow =
            reinterpret_cast<xcb_destroy_notify_event_t *>(event);
        if (destroywindow->event == dockWindow_) {
            refreshDockWindow();
            return true;
        }
        break;
    }
    case XCB_PROPERTY_NOTIFY: {
        auto property = reinterpret_cast<xcb_property_notify_event_t *>(event);
        if (property->atom == atoms_[ATOM_VISUAL] &&
            property->window == dockWindow_) {
            createTrayWindow();
            findDock();
            return true;
        }
        break;
    }
    }
    return false;
}

void XCBTrayWindow::initTray() {
    char trayAtomNameBuf[100];
    const char *atom_names[] = {
        trayAtomNameBuf, "MANAGER", "_NET_SYSTEM_TRAY_OPCODE",
        "_NET_SYSTEM_TRAY_ORIENTATION", "_NET_SYSTEM_TRAY_VISUAL"};

    sprintf(trayAtomNameBuf, "_NET_SYSTEM_TRAY_S%d", ui_->defaultScreen());
    size_t i = 0;
    for (auto name : atom_names) {
        atoms_[i] = ui_->parent()->xcb()->call<IXCBModule::atom>(ui_->name(),
                                                                 name, false);
        i++;
    }
}

void XCBTrayWindow::refreshDockWindow() {
    auto cookie = xcb_get_selection_owner(ui_->connection(), atoms_[0]);
    auto reply = makeXCBReply(
        xcb_get_selection_owner_reply(ui_->connection(), cookie, nullptr));
    if (reply) {
        dockWindow_ = reply->owner;
    }

    if (dockWindow_) {
        CLASSICUI_DEBUG() << "Found dock window";
        addEventMaskToWindow(ui_->connection(), dockWindow_,
                             XCB_EVENT_MASK_STRUCTURE_NOTIFY);
        createTrayWindow();
        findDock();
    } else {
        destroyWindow();
    }
}

void XCBTrayWindow::findDock() {
    if (!wid_) {
        return;
    }

    if (dockWindow_) {
        CLASSICUI_DEBUG() << "Send op code to tray";
        sendTrayOpcode(SYSTEM_TRAY_REQUEST_DOCK, wid_, 0, 0);
    }
}

void XCBTrayWindow::createTrayWindow() {
    trayVid_ = trayVisual();
    createWindow(trayVid_);
}

void XCBTrayWindow::sendTrayOpcode(long message, long data1, long data2,
                                   long data3) {
    xcb_client_message_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = dockWindow_;
    ev.type = atoms_[ATOM_SYSTEM_TRAY_OPCODE];
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = message;
    ev.data.data32[2] = data1;
    ev.data.data32[3] = data2;
    ev.data.data32[4] = data3;

    xcb_send_event(ui_->connection(), false, dockWindow_,
                   XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<char *>(&ev));
    xcb_flush(ui_->connection());
}

xcb_visualid_t XCBTrayWindow::trayVisual() {
    xcb_visualid_t vid = 0;
    if (dockWindow_ != XCB_WINDOW_NONE) {
        auto cookie =
            xcb_get_property(ui_->connection(), false, dockWindow_,
                             atoms_[ATOM_VISUAL], XCB_ATOM_VISUALID, 0, 1);
        auto reply = makeXCBReply(
            xcb_get_property_reply(ui_->connection(), cookie, nullptr));
        if (reply && reply->type == XCB_ATOM_VISUALID && reply->format == 32 &&
            reply->bytes_after == 0) {
            auto data =
                static_cast<char *>(xcb_get_property_value(reply.get()));
            int length = xcb_get_property_value_length(reply.get());
            if (length == 32 / 8) {
                vid = *reinterpret_cast<xcb_visualid_t *>(data);
            }
        }
    }
    return vid;
}

void XCBTrayWindow::postCreateWindow() {
    if (ui_->ewmh()->_NET_WM_WINDOW_TYPE_DOCK &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE) {
        xcb_ewmh_set_wm_window_type(ui_->ewmh(), wid_, 1,
                                    &ui_->ewmh()->_NET_WM_WINDOW_TYPE_DOCK);
    }

    if (ui_->ewmh()->_NET_WM_PID) {
        xcb_ewmh_set_wm_pid(ui_->ewmh(), wid_, getpid());
    }
    const char name[] = "Fcitx5 Tray Window";
    xcb_icccm_set_wm_name(ui_->connection(), wid_, XCB_ATOM_STRING, 8,
                          sizeof(name) - 1, name);

    addEventMaskToWindow(
        ui_->connection(), wid_,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_POINTER_MOTION);

    if (!trayVid_) {
        // Change window attr.
        xcb_change_window_attributes_value_list_t list;
        list.background_pixmap = XCB_BACK_PIXMAP_PARENT_RELATIVE;
        xcb_screen_t *screen =
            xcb_aux_get_screen(ui_->connection(), ui_->defaultScreen());
        list.background_pixel = screen->white_pixel;
        list.border_pixel = screen->black_pixel;
        xcb_change_window_attributes_aux(
            ui_->connection(), wid_,
            XCB_CW_BACKING_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_BACK_PIXMAP,
            &list);
        xcb_flush(ui_->connection());
    }
}

void XCBTrayWindow::paint(cairo_t *c) {
    auto &theme = ui_->parent()->theme();
    auto instance = ui_->parent()->instance();
    auto ic = ui_->parent()->instance()->lastFocusedInputContext();
    std::string icon = "input-keyboard";
    std::string label = "";
    const InputMethodEntry *entry = nullptr;
    if (ic) {
        entry = instance->inputMethodEntry(ic);
    }
    if (entry) {
        icon = entry->icon();
        label = entry->label();
    }

    auto &image = theme.loadImage(icon, label, std::min(height(), width()),
                                  ImagePurpose::Tray);

    cairo_save(c);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    double scaleW = 1.0, scaleH = 1.0;
    if (image.width() != width() || image.height() != height()) {
        scaleW = static_cast<double>(width()) / image.width();
        scaleH = static_cast<double>(height()) / image.height();
        if (scaleW > scaleH)
            scaleH = scaleW;
        else
            scaleW = scaleH;
    }
    int aw = scaleW * image.width();
    int ah = scaleH * image.height();

    cairo_scale(c, scaleW, scaleH);
    cairo_set_source_surface(c, image, (width() - aw) / 2, (height() - ah) / 2);
    cairo_paint(c);
    cairo_restore(c);
}

void XCBTrayWindow::update() {
    if (!wid_) {
        return;
    }

    if (auto surface = prerender()) {
        cairo_t *c = cairo_create(surface);
        paint(c);
        cairo_destroy(c);
        render();
    }
}

void XCBTrayWindow::render() {
    if (!trayVid_) {
        xcb_clear_area(ui_->connection(), false, wid_, 0, 0, width(), height());
    }
    auto cr = cairo_create(surface_.get());
    if (trayVid_) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);
    }
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cr, contentSurface_.get(), 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_flush(surface_.get());
    xcb_flush(ui_->connection());
    CLASSICUI_DEBUG() << "Render";
}

void XCBTrayWindow::resume() {
    if (dockCallback_) {
        return;
    }
    char trayAtomNameBuf[100];
    sprintf(trayAtomNameBuf, "_NET_SYSTEM_TRAY_S%d", ui_->defaultScreen());
    xcb_screen_t *screen =
        xcb_aux_get_screen(ui_->connection(), ui_->defaultScreen());
    addEventMaskToWindow(ui_->connection(), screen->root,
                         XCB_EVENT_MASK_STRUCTURE_NOTIFY);
    dockCallback_ = ui_->parent()->xcb()->call<IXCBModule::addSelection>(
        ui_->name(), trayAtomNameBuf,
        [this](xcb_atom_t) { refreshDockWindow(); });
    refreshDockWindow();
}

void XCBTrayWindow::suspend() {
    if (!dockCallback_) {
        return;
    }
    dockCallback_.reset();
    destroyWindow();
}

void XCBTrayWindow::updateMenu() {
    updateGroupMenu();
    updateInputMethodMenu();

    auto &imManager = ui_->parent()->instance()->inputMethodManager();
    if (imManager.groupCount() > 1) {
        menu_.insertAction(&inputMethodAction_, &groupAction_);
    } else {
        menu_.removeAction(&groupAction_);
    }
    bool start = false;
    for (auto action : menu_.actions()) {
        if (action == &separatorActions_[0]) {
            start = true;
        } else if (action == &configureAction_) {
            break;
        } else if (start) {
            menu_.removeAction(action);
        }
    }

    bool hasAction = false;
    if (auto ic = ui_->parent()->instance()->mostRecentInputContext()) {
        auto &statusArea = ic->statusArea();
        for (auto action : statusArea.allActions()) {
            if (!action->id()) {
                // Obviously it's not registered with ui manager.
                continue;
            }
            menu_.insertAction(&configureAction_, action);
            hasAction = true;
        }
    }
    if (hasAction) {
        menu_.insertAction(&configureAction_, &separatorActions_[1]);
    }
}

void XCBTrayWindow::updateGroupMenu() {
    auto &imManager = ui_->parent()->instance()->inputMethodManager();
    const auto &list = imManager.groups();
    groupActions_.clear();
    for (size_t i = 0; i < list.size(); i++) {
        auto groupName = list[i];
        groupActions_.emplace_back();
        auto &groupAction = groupActions_.back();
        groupAction.setShortText(list[i]);
        groupAction.connect<SimpleAction::Activated>(
            [&imManager, groupName](InputContext *) {
                imManager.setCurrentGroup(groupName);
            });
        groupAction.setCheckable(true);
        groupAction.setChecked(list[i] == imManager.currentGroup().name());

        auto &uiManager = ui_->parent()->instance()->userInterfaceManager();
        uiManager.registerAction(&groupAction);
        groupMenu_.addAction(&groupAction);
    }
}

void XCBTrayWindow::updateInputMethodMenu() {
    auto &imManager = ui_->parent()->instance()->inputMethodManager();
    const auto &list = imManager.currentGroup().inputMethodList();
    inputMethodActions_.clear();
    auto ic = ui_->parent()->instance()->mostRecentInputContext();
    for (size_t i = 0; i < list.size(); i++) {
        auto entry = imManager.entry(list[i].name());
        if (!entry) {
            return;
        }
        inputMethodActions_.emplace_back();
        auto imName = entry->uniqueName();
        auto &inputMethodAction = inputMethodActions_.back();
        inputMethodAction.setShortText(entry->name());
        inputMethodAction.connect<SimpleAction::Activated>(
            [this, imName](InputContext *) {
                ui_->parent()->instance()->setCurrentInputMethod(imName);
            });
        inputMethodAction.setCheckable(true);
        inputMethodAction.setChecked(
            ic ? (ui_->parent()->instance()->inputMethod(ic) == imName)
               : false);

        auto &uiManager = ui_->parent()->instance()->userInterfaceManager();
        uiManager.registerAction(&inputMethodAction);
        inputMethodMenu_.addAction(&inputMethodAction);
    }
}
} // namespace classicui
} // namespace fcitx
