//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "xcbmenu.h"
#include "fcitx/userinterfacemanager.h"
#include <fcitx/inputcontext.h>
#include <pango/pangocairo.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>

fcitx::classicui::XCBMenu::XCBMenu(XCBUI *ui, MenuPool *pool, Menu *menu)
    : fcitx::classicui::XCBWindow(ui), pool_(pool),
      context_(nullptr, &g_object_unref), menu_(menu) {
    auto fontMap = pango_cairo_font_map_get_default();
    context_.reset(pango_font_map_create_context(fontMap));
    if (auto ic = ui_->parent()->instance()->mostRecentInputContext()) {
        lastRelevantIc_ = ic->watch();
    }
    createWindow();
}

fcitx::classicui::XCBMenu::~XCBMenu() {}

bool fcitx::classicui::XCBMenu::filterEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    switch (response_type) {
    case XCB_EXPOSE: {
        auto expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            CLASSICUI_DEBUG() << "Menu recevied expose event";
            update();
            return true;
        }
        break;
    }
    case XCB_FOCUS_OUT: {
        auto focusOut = reinterpret_cast<xcb_focus_out_event_t *>(event);
        if (focusOut->event == wid_) {
            if (!child_.isValid()) {
                hideParent();
                xcb_flush(ui_->connection());
            }
            return true;
        }
        break;
    }
    case XCB_BUTTON_PRESS: {
        auto buttonPress = reinterpret_cast<xcb_button_press_event_t *>(event);
        if (buttonPress->event != wid_) {
            break;
        }
        if (buttonPress->detail != XCB_BUTTON_INDEX_1) {
            hideParent();
            hideChilds();
            xcb_flush(ui_->connection());
            return true;
        }
        for (size_t i = 0; i < items_.size(); i++) {
            if (items_[i].isSeparator_ ||
                !items_[i].region_.contains(buttonPress->event_x,
                                            buttonPress->event_y)) {
                continue;
            }
            if (items_[i].hasSubMenu_) {
                return true;
            }
            // Check if actions is still good.
            auto actions = menu_->actions();
            if (i >= actions.size()) {
                break;
            }
            auto ic = lastRelevantIc();
            if (!ic) {
                break;
            }

            auto id = actions[i]->id();
            auto icRef = ic->watch();
            // Why we need to delay the event, because we
            // want to make ic has focus.
            activateTimer_ =
                ui_->parent()->instance()->eventLoop().addTimeEvent(
                    CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
                    [this, icRef, id](EventSourceTime *, uint64_t) {
                        FCITX_INFO() << "Timer Triggered";
                        if (auto ic = icRef.get()) {

                            auto action = ui_->parent()
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
        hideParent();
        xcb_flush(ui_->connection());
        return true;
    }
    case XCB_MOTION_NOTIFY: {
        auto motion = reinterpret_cast<xcb_motion_notify_event_t *>(event);
        if (motion->event == wid_) {
            for (size_t i = 0; i < items_.size(); i++) {
                if (!items_[i].isSeparator_ &&
                    items_[i].region_.contains(motion->event_x,
                                               motion->event_y)) {
                    setHighlightIndex(i);
                    break;
                }
            }
            return true;
        }
        break;
    }
    case XCB_ENTER_NOTIFY: {
        auto enter = reinterpret_cast<xcb_enter_notify_event_t *>(event);
        if (enter->event == wid_) {
            hasMouse_ = true;
            return true;
        }
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto leave = reinterpret_cast<xcb_leave_notify_event_t *>(event);
        if (leave->event == wid_) {
            hasMouse_ = false;
            setHighlightIndex(-1);
            return true;
        }
        break;
    }
    case XCB_KEY_PRESS: {
        auto key = reinterpret_cast<xcb_key_press_event_t *>(event);
        if (key->event == wid_) {
            return true;
        }
        break;
    }
    }
    return false;
}

void fcitx::classicui::XCBMenu::hide() {
    if (!visible_) {
        return;
    }
    FCITX_INFO() << "Hide " << this;
    visible_ = false;
    setParent(nullptr);
    xcb_unmap_window(ui_->connection(), wid_);
}

void fcitx::classicui::XCBMenu::hideParent() {
    FCITX_INFO() << "Hide Parent " << this;
    if (auto parent = parent_.get()) {
        parent->hideParent();
    }
    hide();
}

void fcitx::classicui::XCBMenu::hideChilds() {
    FCITX_INFO() << "Hide Childs " << this;
    hide();
    if (auto child = child_.get()) {
        child->hideChilds();
    }
}

bool fcitx::classicui::XCBMenu::childHasMouse() const {
    auto ref = child_;
    while (auto child = ref.get()) {
        if (child->hasMouse_) {
            return true;
        }
        ref = child->child_;
    }
    return false;
}

void fcitx::classicui::XCBMenu::hideTillMenuHasMouseOrTopLevel() {
    if (auto child = child_.get()) {
        child->hideChilds();
    }
    if (parent_.isNull() || hasMouse_) {
        if (!hasMouse_) {
            highlightIndex_ = -1;
            update();
        }
        xcb_set_input_focus(ui_->connection(), XCB_INPUT_FOCUS_PARENT, wid_,
                            XCB_CURRENT_TIME);
        xcb_flush(ui_->connection());
        return;
    }
    if (auto parent = parent_.get()) {
        parent->hideTillMenuHasMouseOrTopLevel();
    }
    hide();
}

void fcitx::classicui::XCBMenu::setHighlightIndex(int idx) {
    if (highlightIndex_ == idx) {
        return;
    }
    FCITX_INFO() << this << " setHighlightIndex(): " << idx
                 << " hasMouse: " << hasMouse_
                 << " child is valid: " << child_.isValid();

    if (hasMouse_ || !child_.isValid()) {
        highlightIndex_ = idx;
        update();
    }
    popupMenuTimer_ = ui_->parent()->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 300000, 0,
        [this](EventSourceTime *, uint64_t) {
            popupMenuTimer_.reset();
            return true;
        });
}

void fcitx::classicui::XCBMenu::update() {
    auto ic = lastRelevantIc();
    if (!ic) {
        return;
    }

    // Size hint:
    // Height = Margin + Content + Spacing
    // Width = Margin + Max content.

    auto updateIfLarger = [](size_t &m, size_t n) {
        if (n > m) {
            m = n;
        }
    };

    auto actions = menu_->actions();
    while (items_.size() < actions.size()) {
        items_.emplace_back(context_.get());
    }
    items_.erase(items_.begin() + actions.size(), items_.end());
    auto &theme = ui_->parent()->theme();
    auto fontDesc =
        pango_font_description_from_string(theme.menu->font->c_str());
    pango_context_set_font_description(context_.get(), fontDesc);
    pango_cairo_context_set_resolution(context_.get(), dpi_);
    pango_font_description_free(fontDesc);

    const auto &textMargin = *theme.menu->textMargin;
    int i = 0;
    auto &separator = theme.loadBackground(*theme.menu->separator);
    auto &checkBox = theme.loadBackground(*theme.menu->checkBox);
    auto &subMenu = theme.loadBackground(*theme.menu->subMenu);
    const auto &highlightMargin = *theme.menu->highlight->margin;
    size_t maxItemWidth = 0;
    size_t maxItemHeight = 0;

    bool hasCheckable =
        std::any_of(actions.begin(), actions.end(), [](const Action *action) {
            return action->isCheckable() && !action->isSeparator();
        });
    // We need multiple pass to get the size and location right.
    // Pass 1: get max size of all items, and set size.
    for (auto action : actions) {
        auto &item = items_[i];
        item.isHighlight_ = highlightIndex_ == i;
        i++;
        item.hasSubMenu_ = action->menu() ? true : false;
        item.isSeparator_ = action->isSeparator();
        if (action->isSeparator()) {
            continue;
        }

        // Calculate size for real items.
        auto text = action->shortText(ic);
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

    size_t width = *theme.menu->contentMargin->marginLeft;
    size_t height = *theme.menu->contentMargin->marginTop;
    bool prevIsSeparator = false;
    for (auto &item : items_) {
        if (item.isSeparator_) {
            item.layoutX_ = width;
            item.layoutY_ = height;
            height += separator.height();
            prevIsSeparator = true;
            continue;
        }

        if (!prevIsSeparator) {
            height += *theme.menu->spacing;
        }

        item.region_
            .setPosition(
                width + *textMargin.marginLeft - *highlightMargin.marginLeft,
                height + *textMargin.marginTop - *highlightMargin.marginTop)
            .setSize(maxItemWidth + *highlightMargin.marginLeft +
                         *highlightMargin.marginRight,
                     maxItemHeight + *highlightMargin.marginTop +
                         *highlightMargin.marginTop);
        item.layoutX_ = width + *textMargin.marginLeft +
                        (hasCheckable ? checkBox.width() : 0);
        item.layoutY_ = height + *textMargin.marginTop +
                        (maxItemHeight - item.textHeight_) / 2.0;
        item.checkBoxX_ = width + *textMargin.marginLeft;
        item.checkBoxY_ = height + *textMargin.marginTop +
                          (maxItemHeight - checkBox.height()) / 2.0;

        height +=
            maxItemHeight + *textMargin.marginTop + *textMargin.marginBottom;
    }

    width += maxItemWidth + *textMargin.marginLeft + *textMargin.marginRight +
             *theme.menu->contentMargin->marginRight;
    height += *theme.menu->contentMargin->marginBottom;

    updateIfLarger(width, 1);
    updateIfLarger(height, 1);

    resize(width, height);

    cairo_t *c = cairo_create(prerender());

    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    theme.paint(c, *theme.menu->background, width, height);
    cairo_set_operator(c, CAIRO_OPERATOR_OVER);
    for (const auto &item : items_) {
        if (item.isSeparator_) {
            cairo_save(c);
            cairo_translate(c, item.layoutX_, item.layoutY_);
            theme.paint(c, *theme.menu->separator,
                        width - *theme.menu->contentMargin->marginLeft -
                            *theme.menu->contentMargin->marginRight,
                        -1);
            cairo_restore(c);
            continue;
        }

        if (item.isHighlight_) {
            cairo_save(c);
            cairo_translate(c, item.region_.left(), item.region_.top());
            theme.paint(c, *theme.menu->highlight, item.region_.width(),
                        item.region_.height());
            cairo_restore(c);
        }

        if (item.isChecked_) {
            cairo_save(c);
            cairo_translate(c, item.checkBoxX_, item.checkBoxY_);
            theme.paint(c, *theme.menu->checkBox, -1, -1);
            cairo_restore(c);
        }

        cairo_save(c);
        if (item.isHighlight_) {
            cairoSetSourceColor(c, *theme.menu->highlightTextColor);
        } else {
            cairoSetSourceColor(c, *theme.menu->normalColor);
        }
        cairo_translate(c, item.layoutX_, item.layoutY_);
        pango_cairo_show_layout(c, item.layout_.get());
        cairo_restore(c);
    }

    cairo_destroy(c);
    render();
}

void fcitx::classicui::XCBMenu::postCreateWindow() {
    if (ui_->ewmh()->_NET_WM_WINDOW_TYPE_MENU &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE_POPUP_MENU &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE) {
        uint32_t types[] = {ui_->ewmh()->_NET_WM_WINDOW_TYPE_MENU,
                            ui_->ewmh()->_NET_WM_WINDOW_TYPE_POPUP_MENU};
        xcb_ewmh_set_wm_window_type(ui_->ewmh(), wid_, 1, types);
    }
    addEventMaskToWindow(
        ui_->connection(), wid_,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_FOCUS_CHANGE |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_POINTER_MOTION);
}

void fcitx::classicui::XCBMenu::setParent(fcitx::classicui::XCBMenu *parent) {
    if (auto oldParent = parent_.get()) {
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

void fcitx::classicui::XCBMenu::setChild(fcitx::classicui::XCBMenu *child) {
    if (child) {
        child_ = child->watch();
    } else {
        child_.unwatch();
    }
}

void fcitx::classicui::XCBMenu::setInputContext(
    TrackableObjectReference<fcitx::InputContext> ic) {
    lastRelevantIc_ = ic;
}

fcitx::InputContext *fcitx::classicui::XCBMenu::lastRelevantIc() {
    if (auto ic = lastRelevantIc_.get()) {
        return ic;
    }
    return ui_->parent()->instance()->mostRecentInputContext();
}

void fcitx::classicui::XCBMenu::show(Rect rect) {
    FCITX_INFO() << this << " show() " << highlightIndex_;
    if (visible_) {
        return;
    }
    visible_ = true;
    highlightIndex_ = -1;
    int x = rect.left();
    int y = rect.top();
    dpi_ = ui_->dpiByPosition(x, y);
    update();
    const Rect *closestScreen = nullptr;
    int shortestDistance = INT_MAX;
    for (auto &rect : ui_->screenRects()) {
        int thisDistance = rect.first.distance(x, y);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = &rect.first;
        }
    }

    x = x + rect.width();

    if (closestScreen) {
        int newX, newY;

        if (x + width() > closestScreen->right()) {
            newX = rect.left() - width();
            ;
        } else {
            newX = x;
        }

        if (y < closestScreen->top()) {
            newY = closestScreen->top();
        } else {
            newY = y;
        }

        if (newY + height() > closestScreen->bottom()) {
            if (newY > closestScreen->bottom())
                newY = closestScreen->bottom() - height();
            else { /* better position the window */
                newY = newY - height();
            }
        }
        x = newX;
        y = newY;
    }

    xcb_params_configure_window_t wc;
    wc.x = x;
    wc.y = y;
    wc.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE |
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             &wc);

    xcb_set_input_focus(ui_->connection(), XCB_INPUT_FOCUS_PARENT, wid_,
                        XCB_CURRENT_TIME);

    xcb_map_window(ui_->connection(), wid_);
    xcb_flush(ui_->connection());
    x_ = x;
    y_ = y;
}

void fcitx::classicui::XCBMenu::raise() {
    xcb_params_configure_window_t wc;
    wc.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE, &wc);
}

fcitx::classicui::XCBMenu *
fcitx::classicui::MenuPool::requestMenu(fcitx::classicui::XCBUI *ui,
                                        fcitx::Menu *menu,
                                        fcitx::classicui::XCBMenu *parent) {
    auto xcbMenu = findOrCreateMenu(ui, menu);
    xcbMenu->setParent(parent);
    if (parent) {
        xcbMenu->setInputContext(parent->inputContext());
    } else {
        if (auto ic = ui->parent()->instance()->mostRecentInputContext()) {
            xcbMenu->setInputContext(ic->watch());
        } else {
            xcbMenu->setInputContext({});
        }
    }
    return xcbMenu;
}

fcitx::classicui::XCBMenu *
fcitx::classicui::MenuPool::findOrCreateMenu(fcitx::classicui::XCBUI *ui,
                                             fcitx::Menu *menu) {
    auto iter = pool_.find(menu);
    if (iter != pool_.end()) {
        return &iter->second.first;
    }

    ScopedConnection conn =
        menu->connect<ObjectDestroyed>([this, menu](void *data) {
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
