//
// Copyright (C) 2016~2016 by CSSlayer
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

#include "classicui.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/instance.h"
#include "fcitx/userinterfacemanager.h"
#include "notificationitem_public.h"
#include "waylandui.h"
#include "xcbui.h"
#include <fcntl.h>

namespace fcitx {
namespace classicui {

ClassicUI::ClassicUI(Instance *instance)
    : UserInterface(), instance_(instance) {
    reloadConfig();

    if (auto xcbAddon = xcb()) {
        xcbCreatedCallback_ =
            xcbAddon->call<IXCBModule::addConnectionCreatedCallback>(
                [this](const std::string &name, xcb_connection_t *conn,
                       int screen, FocusGroup *) {
                    uis_["x11:" + name].reset(
                        new XCBUI(this, name, conn, screen));
                });
        xcbClosedCallback_ =
            xcbAddon->call<IXCBModule::addConnectionClosedCallback>(
                [this](const std::string &name, xcb_connection_t *) {
                    uis_.erase("x11:" + name);
                });
    }

    if (auto waylandAddon = wayland()) {
        waylandCreatedCallback_ =
            waylandAddon->call<IWaylandModule::addConnectionCreatedCallback>(
                [this](const std::string &name, wl_display *display,
                       FocusGroup *) {
                    try {
                        uis_["wayland:" + name].reset(
                            new WaylandUI(this, name, display));
                    } catch (const std::runtime_error &) {
                    }
                });
        waylandClosedCallback_ =
            waylandAddon->call<IWaylandModule::addConnectionClosedCallback>(
                [this](const std::string &name, wl_display *) {
                    uis_.erase("wayland:" + name);
                });
    }
}

ClassicUI::~ClassicUI() {}

void ClassicUI::reloadConfig() {
    readAsIni(config_, "conf/classicui.conf");

    auto themeConfigFile = StandardPath::global().open(
        StandardPath::Type::PkgData,
        stringutils::joinPath("themes", *config_.theme, "theme.conf"),
        O_RDONLY);
    RawConfig themeConfig;
    readFromIni(themeConfig, themeConfigFile.fd());
    theme_.load(*config_.theme, themeConfig);
}

AddonInstance *ClassicUI::xcb() {
    auto &addonManager = instance_->addonManager();
    return addonManager.addon("xcb");
}

AddonInstance *ClassicUI::wayland() {
    auto &addonManager = instance_->addonManager();
    return addonManager.addon("wayland");
}

void ClassicUI::suspend() {
    suspended_ = true;
    for (auto &p : uis_) {
        p.second->suspend();
    }

    if (auto sni = notificationitem()) {
        sni->call<INotificationItem::disable>();
    }
    eventHandlers_.clear();
}

UIInterface *ClassicUI::uiForEvent(Event &event) {
    if (suspended_) {
        return nullptr;
    }

    if (!event.isInputContextEvent()) {
        return nullptr;
    }

    auto &icEvent = static_cast<InputContextEvent &>(event);
    return uiForInputContext(icEvent.inputContext());
}

UIInterface *ClassicUI::uiForInputContext(InputContext *inputContext) {
    if (suspended_ || !inputContext) {
        return nullptr;
    }
    if (!inputContext->hasFocus()) {
        return nullptr;
    }
    auto iter = uis_.find(inputContext->display());
    if (iter == uis_.end()) {
        return nullptr;
    }
    return iter->second.get();
}

void ClassicUI::resume() {
    suspended_ = false;
    for (auto &p : uis_) {
        p.second->resume();
    }

    if (auto sni = notificationitem()) {
        if (!sniHandler_) {
            sniHandler_ =
                sni->call<INotificationItem::watch>([this](bool enable) {
                    for (auto &p : uis_) {
                        p.second->setEnableTray(!enable);
                    }
                });
        }
        sni->call<INotificationItem::enable>();
        auto registered = sni->call<INotificationItem::registered>();
        for (auto &p : uis_) {
            p.second->setEnableTray(!registered);
        }
    } else {
        for (auto &p : uis_) {
            p.second->setEnableTray(true);
        }
    }

    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextCursorRectChanged, EventWatcherPhase::Default,
        [this](Event &event) {
            if (auto ui = uiForEvent(event)) {
                auto &icEvent = static_cast<InputContextEvent &>(event);
                ui->updateCursor(icEvent.inputContext());
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default,
        [this](Event &event) {
            if (auto ui = uiForEvent(event)) {
                auto &icEvent = static_cast<InputContextEvent &>(event);
                ui->updateCursor(icEvent.inputContext());
                ui->updateCurrentInputMethod(icEvent.inputContext());
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextSwitchInputMethod, EventWatcherPhase::Default,
        [this](Event &event) {
            if (auto ui = uiForEvent(event)) {
                auto &icEvent = static_cast<InputContextEvent &>(event);
                ui->updateCurrentInputMethod(icEvent.inputContext());
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            instance_->inputContextManager().foreachFocused(
                [this](InputContext *ic) {
                    if (auto ui = uiForInputContext(ic)) {
                        ui->updateCurrentInputMethod(ic);
                    }
                    return true;
                });
        }));
}

void ClassicUI::update(UserInterfaceComponent component,
                       InputContext *inputContext) {
    auto iter = uis_.find(inputContext->display());
    if (iter == uis_.end()) {
        return;
    }
    auto ui = iter->second.get();
    ui->update(component, inputContext);
}

class ClassicUIFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new ClassicUI(manager->instance());
    }
};
} // namespace classicui
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::classicui::ClassicUIFactory);
