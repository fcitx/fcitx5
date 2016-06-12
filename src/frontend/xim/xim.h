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
#ifndef _XIM_XIMMODULE_H_
#define _XIM_XIMMODULE_H_

#include <xcb-imdkit/imdkit.h>

#include "fcitx-utils/event.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonfactory.h"
#include "fcitx/focusgroup.h"
#include "fcitx/addonmanager.h"
#include "modules/xcb/xcb_public.h"
#include <unordered_map>
#include <list>
#include <vector>

namespace fcitx {

class XIMModule;
class XIMServer;

class XIMModule : public AddonInstance {
public:
    XIMModule(Instance *instance);
    ~XIMModule();

    AddonInstance *xcb();
    Instance *instance() { return m_instance; }

private:
    Instance *m_instance;
    std::unordered_map<std::string, std::unique_ptr<XIMServer>> m_servers;
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>> m_createdCallback;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> m_closedCallback;
};

class XIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new XIMModule(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::XIMModuleFactory);

#endif // _XIM_XIMMODULE_H_
