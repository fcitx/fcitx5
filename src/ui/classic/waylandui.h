/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDUI_H_
#define _FCITX_UI_CLASSIC_WAYLANDUI_H_

#include <cairo/cairo.h>
#include "classicui.h"
#include "config.h"
#include "display.h"
#include "waylandpointer.h"
#include "wl_pointer.h"

namespace fcitx {
namespace classicui {

class WaylandWindow;
class WaylandInputWindow;

class WaylandUI : public UIInterface {
public:
    WaylandUI(ClassicUI *parent, const std::string &name, wl_display *display);
    ~WaylandUI();

    ClassicUI *parent() const { return parent_; }
    const std::string &name() const { return name_; }
    wayland::Display *display() const { return display_; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void suspend() override;
    void resume() override;
    void setEnableTray(bool) override {}

    std::unique_ptr<WaylandWindow> newWindow();

private:
    void setupInputWindow();

    ClassicUI *parent_;
    std::string name_;
    wayland::Display *display_;
    ScopedConnection panelConn_, panelRemovedConn_;
    std::unique_ptr<WaylandPointer> pointer_;
    std::unique_ptr<WaylandInputWindow> inputWindow_;
    std::unique_ptr<EventSource> defer_;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDUI_H_
