/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_XCBMENU_H_
#define _FCITX_UI_CLASSIC_XCBMENU_H_

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <xcb/xcb.h>
#include <yoga/YGConfig.h>
#include <yoga/YGNode.h>
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/signals.h"
#include "fcitx-utils/trackableobject.h"
#include "fcitx/action.h"
#include "fcitx/inputcontext.h"
#include "fcitx/menu.h"
#include "common.h"
#include "xcbui.h"
#include "xcbwindow.h"

namespace fcitx::classicui {

class MenuPool;

struct MenuItem {
    MenuItem(PangoContext *context) : layout_(pango_layout_new(context)) {
        self_.reset(YGNodeNew());
        leading_.reset(YGNodeNew());
        checkBox_.reset(YGNodeNew());
        text_.reset(YGNodeNew());
        subMenu_.reset(YGNodeNew());
    }

    bool hasSubMenu_ = false;
    bool isHighlight_ = false;
    bool isSeparator_ = false;
    bool isChecked_ = false;
    GObjectUniquePtr<PangoLayout> layout_;
    Rect region_;
    int textWidth_ = 0, textHeight_ = 0;
    UniqueCPtr<YGNode, YGNodeFree> self_;
    UniqueCPtr<YGNode, YGNodeFree> leading_;
    UniqueCPtr<YGNode, YGNodeFree> checkBox_;
    UniqueCPtr<YGNode, YGNodeFree> text_;
    UniqueCPtr<YGNode, YGNodeFree> subMenu_;
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
    static float absoluteLeft(YGNodeRef node);
    static float absoluteTop(YGNodeRef node);
    void renderYogaNode(cairo_t *cr, YGNodeRef node);

    MenuPool *pool_;

    GObjectUniquePtr<PangoFontMap> fontMap_;
    GObjectUniquePtr<PangoContext> context_;
    std::vector<MenuItem> items_;
    UniqueCPtr<YGNode, YGNodeFree> rootNode_;

    ScopedConnection destroyed_;
    TrackableObjectReference<InputContext> lastRelevantIc_;
    Menu *menu_;
    TrackableObjectReference<XCBMenu> parent_;
    TrackableObjectReference<XCBMenu> child_;
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

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_XCBMENU_H_
