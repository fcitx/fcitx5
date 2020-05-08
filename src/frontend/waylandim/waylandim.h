/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
#define _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_

#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include "wayland_public.h"

namespace fcitx {

class WaylandIMModule;
class WaylandIMServer;

class WaylandIMModule : public AddonInstance {
public:
    WaylandIMModule(Instance *instance);
    ~WaylandIMModule();

    AddonInstance *wayland();
    Instance *instance() { return instance_; }

private:
    Instance *instance_;
    std::unordered_map<std::string, std::unique_ptr<WaylandIMServer>> servers_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
        createdCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>> closedCallback_;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
