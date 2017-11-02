/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_UI_KIMPANEL_KIMPANEL_H_
#define _FCITX_UI_KIMPANEL_KIMPANEL_H_

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"

namespace fcitx {

class KimpanelProxy;
class Action;

class Kimpanel : public UserInterface {
public:
    Kimpanel(Instance *instance);
    ~Kimpanel();

    Instance *instance() { return instance_; }
    void suspend() override;
    void resume() override;
    bool available() override { return available_; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void updateInputPanel(InputContext *inputContext);
    void updateCurrentInputMethod(InputContext *inputContext);

    void msgV1Handler(dbus::Message &msg);
    void msgV2Handler(dbus::Message &msg);

    void registerAllProperties(InputContext *ic = nullptr);
    std::string inputMethodStatus(InputContext *ic);
    std::string actionToStatus(Action *action, InputContext *ic);

private:
    void setAvailable(bool available);
    Instance *instance_;
    dbus::Bus *bus_;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<KimpanelProxy> proxy_;
    std::unique_ptr<dbus::ServiceWatcherEntry> entry_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    TrackableObjectReference<InputContext> lastInputContext_;
    std::unique_ptr<EventSourceTime> timeEvent_;
    bool available_ = false;
};
}

#endif // _FCITX_UI_KIMPANEL_KIMPANEL_H_
