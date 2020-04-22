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
#ifndef _FCITX_UI_CLASSIC_XCBMENU_H_
#define _FCITX_UI_CLASSIC_XCBMENU_H_

#include <pango/pango.h>
#include "fcitx/menu.h"
#include "xcbwindow.h"

namespace fcitx {
namespace classicui {

class MenuPool;

struct MenuItem {
    MenuItem(PangoContext *context)
        : layout_(pango_layout_new(context), &g_object_unref) {}

    bool hasSubMenu_ = false;
    bool isHighlight_ = false;
    bool isSeparator_ = false;
    bool isChecked_ = false;
    std::unique_ptr<PangoLayout, decltype(&g_object_unref)> layout_;
    int layoutX_ = 0, layoutY_ = 0;
    Rect region_;
    int textWidth_ = 0, textHeight_ = 0;
    int checkBoxX_ = 0, checkBoxY_ = 0;
    int subMenuX_ = 0, subMenuY_ = 0;
};

class XCBMenu : public XCBWindow, public TrackableObject<XCBMenu> {
public:
    XCBMenu(XCBUI *ui, MenuPool *pool, Menu *menu);
    ~XCBMenu();
    void show(Rect rect);

    // Hide menu itself.
    void hide();

    // Hide all of its parent.
    void hideParents();

    // Hide all menu on the chain until the one has mouse.
    void hideTillMenuHasMouseOrTopLevel();

    // Hide all of its child.
    void hideChilds();

    // Raise the menu.
    void raise();

    bool filterEvent(xcb_generic_event_t *event) override;
    void postCreateWindow() override;

    void setParent(XCBMenu *parent);
    void setInputContext(TrackableObjectReference<InputContext> ic);
    TrackableObjectReference<InputContext> inputContext() const {
        return lastRelevantIc_;
    }

    bool childHasMouse() const;

private:
    void hideTillMenuHasMouseOrTopLevelHelper();
    InputContext *lastRelevantIc();
    void update();
    void setHoveredIndex(int idx);
    void setChild(XCBMenu *child);
    void setFocus();
    std::pair<MenuItem *, Action *> actionAt(size_t index);

    MenuPool *pool_;

    std::unique_ptr<PangoContext, decltype(&g_object_unref)> context_;
    std::vector<MenuItem> items_;

    ScopedConnection destroyed_;
    TrackableObjectReference<InputContext> lastRelevantIc_;
    Menu *menu_;
    TrackableObjectReference<XCBMenu> parent_;
    TrackableObjectReference<XCBMenu> child_;
    int dpi_ = 96;
    int x_ = 0;
    int y_ = 0;
    bool hasMouse_ = false;
    bool visible_ = false;
    int subMenuIndex_ = -1;
    int hoveredIndex_ = -1;
    std::unique_ptr<EventSourceTime> activateTimer_;
};

class MenuPool {
public:
    XCBMenu *requestMenu(XCBUI *ui, Menu *menu, XCBMenu *parent);

    void setPopupMenuTimer(std::unique_ptr<EventSourceTime> popupMenuTimer) {
        popupMenuTimer_ = std::move(popupMenuTimer);
    }

private:
    XCBMenu *findOrCreateMenu(XCBUI *ui, Menu *menu);

    std::unordered_map<Menu *, std::pair<XCBMenu, ScopedConnection>> pool_;
    std::unique_ptr<EventSourceTime> popupMenuTimer_;
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_XCBMENU_H_
