/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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
} // namespace fcitx

#endif // _FCITX_UI_KIMPANEL_KIMPANEL_H_
