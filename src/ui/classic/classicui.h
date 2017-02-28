/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_UI_CLASSIC_CLASSICUI_H_
#define _FCITX_UI_CLASSIC_CLASSICUI_H_

#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/userinterface.h"
#include "wayland_public.h"
#include "xcb_public.h"

namespace fcitx {
namespace classicui {

class UIInterface {};

class ClassicUI : public UserInterface {
public:
    ClassicUI(Instance *instance);
    ~ClassicUI();

    AddonInstance *xcb();
    AddonInstance *wayland();
    Instance *instance() { return instance_; }
    void suspend() override;
    void resume() override;
    void update(UserInterfaceComponent component) override;

private:
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>> xcbCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> xcbClosedCallback_;

    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>> waylandCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>> waylandClosedCallback_;

    std::unordered_map<std::string, std::unique_ptr<UIInterface>> uis_;

    Instance *instance_;
};
}
}

#endif // _FCITX_UI_CLASSIC_CLASSICUI_H_
