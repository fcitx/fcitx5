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
#include "modules/xcb/xcb_public.h"

namespace fcitx {
namespace classicui {
class ClassicUI : public AddonInstance {
public:
    ClassicUI(Instance *instance);
    ~ClassicUI();

    AddonInstance *xcb();
    AddonInstance *wayland();
    Instance *instance() { return instance_; }

private:
    Instance *instance_;
};

class ClassicUIFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new ClassicUI(manager->instance()); }
};
}
}

FCITX_ADDON_FACTORY(fcitx::classicui::ClassicUIFactory);

#endif // _FCITX_UI_CLASSIC_CLASSICUI_H_
