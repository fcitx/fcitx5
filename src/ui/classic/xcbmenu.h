/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_XCBMENU_H_
#define _FCITX_UI_CLASSIC_XCBMENU_H_

#include <pango/pango.h>
#include <xcb/xproto.h>
#include "fcitx/menu.h"
#include "common.h"
#include "xcbwindow.h"

namespace fcitx {
namespace classicui {

class MenuPool;

struct MenuItem {
    MenuItem(PangoContext *context) : layout_(pango_layout_new(context)) {}

    bool hasSubMenu_ = false;
    bool isHighlight_ = false;
    bool isSeparator_ = false;
    bool isChecked_ = false;
    GObjectUniquePtr<PangoLayout> layout_;
    int layoutX_ = 0, layoutY_ = 0;
    Rect region_;
    int textWidth_ = 0, textHeight_ = 0;
    int checkBoxX_ = 0, checkBoxY_ = 0;
    int subMenuX_ = 0, subMenuY_ = 0;
};

enum class ConstrainAdjustment { Slide, Flip };

class XCBMenu : public XCBWindow, public TrackableObject<XCBMenu> {
public:
    XCBMenu(XCBUI *ui, MenuPool *pool, Menu *menu);
    ~XCBMenu();
    void show(Rect rect, ConstrainAdjustment adjustY);

    // Hide menu itself.
    void hide();

    // Hide all of its parent.
    void hideParents();

    // Hide all menu on the chain until the one has mouse.
    void hideTillMenuHasMouseOrTopLevel();

    // Hide all of its child.
    void hideChilds();

    void hideAll();

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
    void handleButtonPress(int eventX, int eventY);
    void handleMotionNotify(int eventX, int eventY);
    XCBMenu *childByPosition(int rootX, int rootY);

    void hideTillMenuHasMouseOrTopLevelHelper();
    InputContext *lastRelevantIc();
    void update();
    void setHoveredIndex(int idx);
    void setChild(XCBMenu *child);
    void updateDPI(int x, int y);
    std::pair<MenuItem *, Action *> actionAt(size_t index);

    MenuPool *pool_;

    GObjectUniquePtr<PangoFontMap> fontMap_;
    GObjectUniquePtr<PangoContext> context_;
    std::vector<MenuItem> items_;

    ScopedConnection destroyed_;
    TrackableObjectReference<InputContext> lastRelevantIc_;
    Menu *menu_;
    TrackableObjectReference<XCBMenu> parent_;
    TrackableObjectReference<XCBMenu> child_;
    int dpi_ = -1;
    double fontMapDefaultDPI_ = 96.0;
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
