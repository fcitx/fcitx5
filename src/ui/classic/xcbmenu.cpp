/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbmenu.h"
#include <unistd.h>
#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>
#include <cairo.h>
#include <pango/pango-context.h>
#include <pango/pango-font.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-layout.h>
#include <pango/pangocairo.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <yoga/YGEnums.h>
#include <yoga/YGNode.h>
#include <yoga/YGNodeLayout.h>
#include <yoga/YGNodeStyle.h>
#include <yoga/YGValue.h>
#include "fcitx-utils/color.h"
#include "fcitx-utils/connectableobject.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/signals.h"
#include "fcitx-utils/trackableobject.h"
#include "fcitx/action.h"
#include "fcitx/inputcontext.h"
#include "fcitx/menu.h"
#include "fcitx/userinterfacemanager.h"
#include "common.h"
#include "theme.h"
#include "xcbui.h"
#include "xcbwindow.h"

namespace fcitx::classicui {

XCBMenu::XCBMenu(XCBUI *ui, MenuPool *pool, Menu *menu)
    : XCBWindow(ui), pool_(pool), menu_(menu) {
    fontMap_.reset(pango_cairo_font_map_new());
    context_.reset(pango_font_map_create_context(fontMap_.get()));
    rootNode_.reset(YGNodeNew());
    if (auto *ic = ui_->parent()->instance()->mostRecentInputContext()) {
        lastRelevantIc_ = ic->watch();
    }
    createWindow(ui_->visualId());
}

XCBMenu::~XCBMenu() {}

bool XCBMenu::filterEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    switch (response_type) {
    case XCB_EXPOSE: {
        auto *expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            CLASSICUI_DEBUG() << "Menu received expose event";
            update();
            return true;
        }
        break;
    }
    case XCB_FOCUS_IN: {
        auto *focusIn = reinterpret_cast<xcb_focus_in_event_t *>(event);
        if (focusIn->event == wid_) {
            if (focusIn->detail == XCB_NOTIFY_DETAIL_POINTER) {
                return true;
            }
            // FCITX_INFO() << this << " Focus in";
            return true;
        }
        break;
    }
    case XCB_FOCUS_OUT: {
        auto *focusOut = reinterpret_cast<xcb_focus_out_event_t *>(event);
        if (focusOut->event == wid_) {
            if (focusOut->detail == XCB_NOTIFY_DETAIL_POINTER) {
                return true;
            }
            // FCITX_INFO() << this << " Focus out " << subMenuIndex_;
            if (subMenuIndex_ < 0) {
                hideChilds();
                hide();
                hideParents();
            }
            return true;
        }
        break;
    }
    case XCB_BUTTON_PRESS: {
        auto *buttonPress = reinterpret_cast<xcb_button_press_event_t *>(event);
        if (buttonPress->event != wid_) {
            break;
        }
        if (buttonPress->detail != XCB_BUTTON_INDEX_1) {
            hideAll();
            return true;
        }
        if (auto *menu =
                childByPosition(buttonPress->root_x, buttonPress->root_y)) {
            menu->handleButtonPress(
                menu->logicalFromPhysical(buttonPress->root_x - menu->x_),
                menu->logicalFromPhysical(buttonPress->root_y - menu->y_));
        } else {
            hideAll();
            return true;
        }
        return true;
    }
    case XCB_MOTION_NOTIFY: {
        auto *motion = reinterpret_cast<xcb_motion_notify_event_t *>(event);
        if (motion->event != wid_) {
            break;
        }

        if (auto *menu = childByPosition(motion->root_x, motion->root_y)) {
            menu->handleMotionNotify(
                menu->logicalFromPhysical(motion->root_x - menu->x_),
                menu->logicalFromPhysical(motion->root_y - menu->y_));
        }
        return true;
    }
    case XCB_ENTER_NOTIFY: {
        auto *enter = reinterpret_cast<xcb_enter_notify_event_t *>(event);
        if (enter->event != wid_) {
            break;
        }

        if (auto *menu = childByPosition(enter->root_x, enter->root_y)) {
            menu->hasMouse_ = true;
            return true;
        }
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto *leave = reinterpret_cast<xcb_leave_notify_event_t *>(event);
        if (leave->event != wid_) {
            break;
        }
        if (auto *menu = childByPosition(leave->root_x, leave->root_y)) {
            menu->hasMouse_ = false;
            menu->setHoveredIndex(-1);
            return true;
        }
        break;
    }
    case XCB_KEY_PRESS: {
        auto *key = reinterpret_cast<xcb_key_press_event_t *>(event);
        if (key->event == wid_) {
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void XCBMenu::handleButtonPress(int eventX, int eventY) {
    for (size_t i = 0; i < items_.size(); i++) {
        if (items_[i].isSeparator_ ||
            !items_[i].region_.contains(eventX, eventY)) {
            continue;
        }
        if (items_[i].hasSubMenu_) {
            return;
        }
        // Check if actions is still good.
        auto actions = menu_->actions();
        if (i >= actions.size()) {
            break;
        }
        auto *ic = lastRelevantIc();
        if (!ic) {
            ic = ui_->parent()
                     ->instance()
                     ->inputContextManager()
                     .dummyInputContext();
        }

        auto id = actions[i]->id();
        // Why we need to delay the event, because we
        // want to make ic has focus.
        activateTimer_ = ui_->parent()->instance()->eventLoop().addTimeEvent(
            CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
            [this, that = this->watch(), icRef = ic->watch(),
             id](EventSourceTime *, uint64_t) {
                if (!that.isValid()) {
                    return true;
                }
                // FCITX_INFO() << "Timer Triggered";
                if (auto *ic = icRef.get()) {
                    auto *action = ui_->parent()
                                       ->instance()
                                       ->userInterfaceManager()
                                       .lookupActionById(id);
                    if (action) {
                        action->activate(ic);
                    }
                }
                activateTimer_.reset();
                return true;
            });
        break;
    }

    hideAll();
}

void XCBMenu::handleMotionNotify(int eventX, int eventY) {
    for (size_t i = 0; i < items_.size(); i++) {
        if (!items_[i].isSeparator_ &&
            items_[i].region_.contains(eventX, eventY)) {
            setHoveredIndex(i);
            return;
        }
    }
}

void XCBMenu::hide() {
    if (!visible_) {
        return;
    }
    // FCITX_INFO() << "Hide " << this;
    visible_ = false;
    setParent(nullptr);
    xcb_unmap_window(ui_->connection(), wid_);
    if (ui_->pointerGrabber() == this) {
        ui_->ungrabPointer();
    }
}

void XCBMenu::hideParents() {
    // FCITX_INFO() << "Hide Parent " << this;
    if (auto *parent = parent_.get()) {
        parent->hideParents();
        parent->hide();
    }
}

void XCBMenu::hideChilds() {
    // FCITX_INFO() << "Hide Childs " << this;
    if (auto *child = child_.get()) {
        child->hideChilds();
        child->hide();
    }
}

void XCBMenu::hideAll() {
    hideParents();
    hide();
    hideChilds();
}

bool XCBMenu::childHasMouse() const {
    auto ref = child_;
    while (auto *child = ref.get()) {
        if (child->hasMouse_) {
            return true;
        }
        ref = child->child_;
    }
    return false;
}

XCBMenu *XCBMenu::childByPosition(int rootX, int rootY) {
    if (ui_->pointerGrabber() != this) {
        return this;
    }

    XCBMenu *result = this;
    while (auto *child = result->child_.get()) {
        result = child;
    }
    while (result) {
        Rect rect;
        rect.setPosition(result->x_, result->y_)
            .setSize(result->physicalWidth_, result->physicalHeight_);
        if (rect.contains(rootX, rootY)) {
            break;
        }
        result = result->parent_.get();
    }
    return result;
}

void XCBMenu::hideTillMenuHasMouseOrTopLevel() {
    // Go to the innermost child.
    auto *menu = this;
    while (auto *child = menu->child_.get()) {
        menu = child;
    }

    menu->hideTillMenuHasMouseOrTopLevelHelper();
}

void XCBMenu::hideTillMenuHasMouseOrTopLevelHelper() {
    if (parent_.isNull() || hasMouse_) {
        update();
        return;
    }
    auto *parent = parent_.get();
    hide(); // Hide will reset parent.
    if (parent) {
        parent->hideTillMenuHasMouseOrTopLevelHelper();
    }
}

void XCBMenu::setHoveredIndex(int idx) {
    if (hoveredIndex_ == idx) {
        return;
    }
    // FCITX_INFO() << this << " setHoveredIndex(): " << idx
    //              << " hasMouse: " << hasMouse_
    //              << " child is valid: " << child_.isValid();

    hoveredIndex_ = idx;
    update();
    pool_->setPopupMenuTimer(
        ui_->parent()->instance()->eventLoop().addTimeEvent(
            CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 300000, 0,
            [this, that = this->watch()](EventSourceTime *, uint64_t) {
                if (!that.isValid()) {
                    return true;
                }
                do {
                    // FCITX_INFO() << this << " in timer";
                    if (hoveredIndex_ >= 0 && subMenuIndex_ == hoveredIndex_) {
                        // Mouse is on same menu item.
                        break;
                    }

                    if (hoveredIndex_ >= 0) {
                        // FCITX_INFO() << this << " in timer branch 1";
                        // The current subMenu anyway is not the hovered one.
                        hideChilds();
                        subMenuIndex_ = -1;
                        auto item =
                            actionAt(static_cast<size_t>(hoveredIndex_));
                        if (!item.first || !item.second) {
                            break;
                        }
                        // If we agree on this that current item has subMenu
                        if (item.first->hasSubMenu_ && item.second->menu()) {
                            auto *newMenu = pool_->requestMenu(
                                ui_, item.second->menu(), this);
                            subMenuIndex_ = hoveredIndex_;

                            // FCITX_INFO() << this << " in timer show submenu "
                            // << newMenu;
                            newMenu->show(
                                physicalFromLogical(item.first->region_)
                                    .translated(x_, y_),
                                ConstrainAdjustment::Slide);
                        }
                    } else {
                        /// FCITX_INFO() << this << " in timer branch 2";
                        // If we are not display any sub menu, and we don't have
                        // mouse in the window.
                        hideTillMenuHasMouseOrTopLevel();
                    }
                    update();
                } while (0);
                pool_->setPopupMenuTimer(nullptr);
                return true;
            }));
}

std::pair<MenuItem *, Action *> XCBMenu::actionAt(size_t index) {
    if (items_.size() <= index) {
        return {};
    }

    auto actions = menu_->actions();
    if (actions.size() <= index || actions.size() != items_.size()) {
        return {};
    }

    return {&items_[index], actions[index]};
}

void XCBMenu::updateDPI(int x, int y) {
    setScale(scaleForDPI(ui_->dpiByPosition(x, y)));
}

float XCBMenu::absoluteLeft(YGNodeRef node) {
    float offset = 0.0F;
    while (node) {
        offset += YGNodeLayoutGetLeft(node);
        node = YGNodeGetParent(node);
    }
    return offset;
}

float XCBMenu::absoluteTop(YGNodeRef node) {
    float offset = 0.0F;
    while (node) {
        offset += YGNodeLayoutGetTop(node);
        node = YGNodeGetParent(node);
    }
    return offset;
}

void XCBMenu::renderYogaNode(cairo_t *cr, YGNodeRef node) {
    if (!node) {
        return;
    }

    cairo_save(cr);
    cairo_translate(cr, YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));

    cairoSetSourceColor(cr, node == rootNode_.get() ? Color(0, 0, 255, 128)
                                                    : Color(255, 0, 0, 76));
    cairo_rectangle(cr, 0, 0, YGNodeLayoutGetWidth(node),
                    YGNodeLayoutGetHeight(node));
    cairo_stroke(cr);

    const auto childCount = YGNodeGetChildCount(node);
    for (uint32_t i = 0; i < childCount; i++) {
        renderYogaNode(cr, YGNodeGetChild(node, i));
    }

    cairo_restore(cr);
}

void XCBMenu::update() {
    auto *ic = lastRelevantIc();
    if (!ic) {
        ic = ui_->parent()
                 ->instance()
                 ->inputContextManager()
                 .dummyInputContext();
    }

    auto updateIfLarger = [](size_t &m, size_t n) { m = std::max(n, m); };

    auto actions = menu_->actions();
    while (items_.size() < actions.size()) {
        items_.emplace_back(context_.get());
    }
    items_.erase(items_.begin() + actions.size(), items_.end());
    auto &theme = ui_->parent()->theme();
    auto *fontDesc = pango_font_description_from_string(
        ui_->parent()->config().menuFont->c_str());
    pango_context_set_font_description(context_.get(), fontDesc);
    pango_font_description_free(fontDesc);
    ui_->fontOption().setupPangoContext(context_.get());

    const auto &textMargin = *theme.menu->textMargin;
    int i = 0;
    const auto &separator = theme.loadBackground(*theme.menu->separator);
    const auto &checkBox = theme.loadBackground(*theme.menu->checkBox);
    const auto &subMenu = theme.loadBackground(*theme.menu->subMenu);
    const auto &highlightMargin = *theme.menu->highlight->margin;
    size_t maxItemWidth = 0;
    size_t maxItemHeight = 0;

    bool hasCheckable = std::ranges::any_of(actions, [](const Action *action) {
        return action->isCheckable() && !action->isSeparator();
    });
    for (auto *action : actions) {
        auto &item = items_[i];
        item.isHighlight_ =
            hoveredIndex_ >= 0 ? (hoveredIndex_ == i) : (subMenuIndex_ == i);
        i++;
        item.hasSubMenu_ = action->menu() != nullptr;
        item.isSeparator_ = action->isSeparator();
        if (action->isSeparator()) {
            continue;
        }

        // Calculate size for real items.
        auto text = action->shortText(ic);
        pango_layout_context_changed(item.layout_.get());
        pango_layout_set_text(item.layout_.get(), text.c_str(), text.size());
        item.textWidth_ = item.textHeight_ = 0;
        pango_layout_get_pixel_size(item.layout_.get(), &item.textWidth_,
                                    &item.textHeight_);

        size_t itemWidth = 0;
        size_t itemHeight = 0;
        if (hasCheckable) {
            itemWidth += checkBox.width();
            updateIfLarger(itemHeight, checkBox.height());
        }
        item.isChecked_ = action->isChecked(ic);
        itemWidth += item.textWidth_;
        updateIfLarger(itemHeight, item.textHeight_);
        itemWidth += subMenu.width();
        updateIfLarger(itemHeight, subMenu.height());

        updateIfLarger(maxItemWidth, itemWidth);
        updateIfLarger(maxItemHeight, itemHeight);
    }

    YGNodeRemoveAllChildren(rootNode_.get());
    YGNodeReset(rootNode_.get());
    YGNodeStyleSetFlexDirection(rootNode_.get(), YGFlexDirectionColumn);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeLeft,
                          *theme.menu->contentMargin->marginLeft);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeRight,
                          *theme.menu->contentMargin->marginRight);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeTop,
                          *theme.menu->contentMargin->marginTop);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeBottom,
                          *theme.menu->contentMargin->marginBottom);
    YGNodeStyleSetMinWidth(rootNode_.get(), 1);
    YGNodeStyleSetMinHeight(rootNode_.get(), 1);
    YGNodeStyleSetGap(rootNode_.get(), YGGutterRow, *theme.menu->spacing);

    for (size_t index = 0; index < items_.size(); index++) {
        auto &item = items_[index];
        YGNodeRemoveAllChildren(item.self_.get());
        YGNodeRemoveAllChildren(item.leading_.get());
        YGNodeReset(item.self_.get());
        YGNodeReset(item.leading_.get());
        YGNodeReset(item.checkBox_.get());
        YGNodeReset(item.text_.get());
        YGNodeReset(item.subMenu_.get());

        if (item.isSeparator_) {
            YGNodeStyleSetHeight(item.self_.get(), separator.isPattern()
                                                       ? 2
                                                       : separator.height());
            YGNodeInsertChild(rootNode_.get(), item.self_.get(), index);
            continue;
        }

        YGNodeStyleSetFlexDirection(item.self_.get(), YGFlexDirectionRow);
        YGNodeStyleSetJustifyContent(item.self_.get(), YGJustifySpaceBetween);
        YGNodeStyleSetAlignItems(item.self_.get(), YGAlignCenter);
        YGNodeStyleSetWidth(item.self_.get(), maxItemWidth);
        YGNodeStyleSetHeight(item.self_.get(), maxItemHeight);
        YGNodeStyleSetMargin(item.self_.get(), YGEdgeLeft,
                             *textMargin.marginLeft);
        YGNodeStyleSetMargin(item.self_.get(), YGEdgeRight,
                             *textMargin.marginRight);
        YGNodeStyleSetMargin(item.self_.get(), YGEdgeTop,
                             *textMargin.marginTop);
        YGNodeStyleSetMargin(item.self_.get(), YGEdgeBottom,
                             *textMargin.marginBottom);

        YGNodeStyleSetFlexDirection(item.leading_.get(), YGFlexDirectionRow);
        YGNodeStyleSetAlignItems(item.leading_.get(), YGAlignCenter);
        if (hasCheckable) {
            YGNodeStyleSetWidth(item.checkBox_.get(), checkBox.width());
            YGNodeStyleSetHeight(item.checkBox_.get(), checkBox.height());
            YGNodeInsertChild(item.leading_.get(), item.checkBox_.get(), 0);
        }
        YGNodeStyleSetWidth(item.text_.get(), item.textWidth_);
        YGNodeStyleSetHeight(item.text_.get(), item.textHeight_);
        YGNodeStyleSetWidth(item.subMenu_.get(), subMenu.width());
        YGNodeStyleSetHeight(item.subMenu_.get(), subMenu.height());
        YGNodeInsertChild(item.leading_.get(), item.text_.get(), hasCheckable);
        YGNodeInsertChild(item.self_.get(), item.leading_.get(), 0);
        YGNodeInsertChild(item.self_.get(), item.subMenu_.get(), 1);
        YGNodeInsertChild(rootNode_.get(), item.self_.get(), index);
    }

    YGNodeCalculateLayout(rootNode_.get(), YGUndefined, YGUndefined,
                          YGDirectionLTR);
    const auto width =
        static_cast<unsigned int>(YGNodeLayoutGetWidth(rootNode_.get()));
    const auto height =
        static_cast<unsigned int>(YGNodeLayoutGetHeight(rootNode_.get()));

    resize(width, height);

    cairo_t *c = cairo_create(prerender());
    cairo_set_operator(c, CAIRO_OPERATOR_CLEAR);
    cairo_paint(c);
    cairo_set_operator(c, CAIRO_OPERATOR_OVER);
    theme.paint(c, *theme.menu->background, 0, 0, width, height, /*alpha=*/1.0);
    for (auto &item : items_) {
        if (item.isSeparator_) {
            const ThemeImage &separator =
                theme.loadBackground(*theme.menu->separator);
            theme.paint(c, *theme.menu->separator,
                        absoluteLeft(item.self_.get()),
                        absoluteTop(item.self_.get()),
                        width - *theme.menu->contentMargin->marginLeft -
                            *theme.menu->contentMargin->marginRight,
                        (separator.isPattern() ? 2 : -1), /*alpha=*/1.0);
            continue;
        }

        const auto itemLeft = absoluteLeft(item.self_.get());
        const auto itemTop = absoluteTop(item.self_.get());
        const auto itemWidth = YGNodeLayoutGetWidth(item.self_.get());
        const auto itemHeight = YGNodeLayoutGetHeight(item.self_.get());
        item.region_
            .setPosition(itemLeft - *highlightMargin.marginLeft,
                         itemTop - *highlightMargin.marginTop)
            .setSize(itemWidth + *highlightMargin.marginLeft +
                         *highlightMargin.marginRight,
                     itemHeight + *highlightMargin.marginTop +
                         *highlightMargin.marginBottom);
        if (item.isHighlight_) {
            theme.paint(c, *theme.menu->highlight, item.region_.left(),
                        item.region_.top(), item.region_.width(),
                        item.region_.height(),
                        /*alpha=*/1.0);
        }

        if (item.isChecked_) {
            theme.paint(
                c, *theme.menu->checkBox, absoluteLeft(item.checkBox_.get()),
                absoluteTop(item.checkBox_.get()), -1, -1, /*alpha=*/1.0);
        }

        if (item.hasSubMenu_) {
            theme.paint(
                c, *theme.menu->subMenu, absoluteLeft(item.subMenu_.get()),
                absoluteTop(item.subMenu_.get()), -1, -1, /*alpha=*/1.0);
        }

        cairo_save(c);
        if (item.isHighlight_) {
            cairoSetSourceColor(c, theme.menuSelectedItemText());
        } else {
            cairoSetSourceColor(c, theme.menuText());
        }
        cairo_translate(c, absoluteLeft(item.text_.get()),
                        absoluteTop(item.text_.get()));
        pango_cairo_show_layout(c, item.layout_.get());
        cairo_restore(c);
    }

    if (classicui_logcategory().checkLogLevel(Debug)) {
        renderYogaNode(c, rootNode_.get());
    }

    cairo_destroy(c);
    render();
}

void XCBMenu::postCreateWindow() {
    if (ui_->ewmh()->_NET_WM_WINDOW_TYPE_MENU &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE_POPUP_MENU &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE) {
        uint32_t types[] = {ui_->ewmh()->_NET_WM_WINDOW_TYPE_MENU,
                            ui_->ewmh()->_NET_WM_WINDOW_TYPE_POPUP_MENU};
        xcb_ewmh_set_wm_window_type(ui_->ewmh(), wid_, 1, types);
    }

    if (ui_->ewmh()->_NET_WM_PID) {
        xcb_ewmh_set_wm_pid(ui_->ewmh(), wid_, getpid());
    }

    const char name[] = "Fcitx5 Menu Window";
    xcb_icccm_set_wm_name(ui_->connection(), wid_, XCB_ATOM_STRING, 8,
                          sizeof(name) - 1, name);
    const char klass[] = "fcitx\0fcitx";
    xcb_icccm_set_wm_class(ui_->connection(), wid_, sizeof(klass) - 1, klass);
    addEventMaskToWindow(
        ui_->connection(), wid_,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_FOCUS_CHANGE |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_POINTER_MOTION);
}

void XCBMenu::setParent(XCBMenu *parent) {
    if (auto *oldParent = parent_.get()) {
        if (parent == oldParent) {
            return;
        }

        parent_.unwatch();
        oldParent->setChild(nullptr);
    }

    if (parent) {
        parent_ = parent->watch();
        parent->setChild(this);
    } else {
        parent_.unwatch();
    }
}

void XCBMenu::setChild(XCBMenu *child) {
    if (child) {
        child_ = child->watch();
    } else {
        child_.unwatch();
        subMenuIndex_ = -1;
        update();
    }
}

void XCBMenu::setInputContext(TrackableObjectReference<InputContext> ic) {
    lastRelevantIc_ = std::move(ic);
}

InputContext *XCBMenu::lastRelevantIc() {
    if (auto *ic = lastRelevantIc_.get()) {
        return ic;
    }
    return ui_->parent()->instance()->mostRecentInputContext();
}

void XCBMenu::show(Rect rect, ConstrainAdjustment adjustY) {
    // FCITX_INFO() << this << " show() " << hoveredIndex_;
    if (visible_) {
        return;
    }
    visible_ = true;
    hoveredIndex_ = -1;
    subMenuIndex_ = -1;
    int x = rect.left();
    int y = rect.top();
    updateDPI(x, y);
    update();
    const Rect *closestScreen = nullptr;
    int shortestDistance = INT_MAX;
    for (const auto &rect : ui_->screenRects()) {
        int thisDistance = rect.first.distance(x, y);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = &rect.first;
        }
    }

    x = x + rect.width();

    if (closestScreen) {
        if (x + physicalWidth_ > closestScreen->right()) {
            x = rect.left() - physicalWidth_;
        }

        switch (adjustY) {
        case ConstrainAdjustment::Slide:
            if (y + physicalHeight_ > closestScreen->bottom()) {
                y = closestScreen->bottom() - physicalHeight_;
            }
            break;
        case ConstrainAdjustment::Flip:
            if (y + physicalHeight_ > closestScreen->bottom()) {
                y = rect.top() - physicalHeight_;
            }
            break;
        };

        y = std::max(y, closestScreen->top());
    }

    xcb_params_configure_window_t wc;
    wc.x = x;
    wc.y = y;
    wc.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE |
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             &wc);

    xcb_map_window(ui_->connection(), wid_);
    if (parent_.isNull()) {
        ui_->grabPointer(this);
    }
    x_ = x;
    y_ = y;
}

void XCBMenu::raise() {
    xcb_params_configure_window_t wc;
    wc.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE, &wc);
}

XCBMenu *MenuPool::requestMenu(XCBUI *ui, Menu *menu, XCBMenu *parent) {
    auto *xcbMenu = findOrCreateMenu(ui, menu);
    xcbMenu->setParent(parent);
    if (parent) {
        xcbMenu->setInputContext(parent->inputContext());
    } else {
        if (auto *ic = ui->parent()->instance()->mostRecentInputContext()) {
            xcbMenu->setInputContext(ic->watch());
        } else {
            xcbMenu->setInputContext({});
        }
    }
    return xcbMenu;
}

XCBMenu *MenuPool::findOrCreateMenu(XCBUI *ui, Menu *menu) {
    auto iter = pool_.find(menu);
    if (iter != pool_.end()) {
        return &iter->second.first;
    }

    ScopedConnection conn = menu->connect<ObjectDestroyed>([this](void *data) {
        Menu *menu = static_cast<Menu *>(data);
        pool_.erase(menu);
    });

    auto result = pool_.emplace(
        std::piecewise_construct, std::forward_as_tuple(menu),
        std::forward_as_tuple(std::piecewise_construct,
                              std::forward_as_tuple(ui, this, menu),
                              std::forward_as_tuple(std::move(conn))));
    return &result.first->second.first;
}

} // namespace fcitx::classicui
