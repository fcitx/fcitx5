/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UI_CLASSIC_CLASSICUI_H_
#define _FCITX_UI_CLASSIC_CLASSICUI_H_

#include "config.h"

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"
#include "theme.h"
#include "wayland_public.h"
#include "xcb_public.h"

namespace fcitx {
namespace classicui {

class UIInterface {
public:
    virtual ~UIInterface() {}
    virtual void update(UserInterfaceComponent component,
                        InputContext *inputContext) = 0;
    virtual void updateCursor(InputContext *) {}
    virtual void updateCurrentInputMethod(InputContext *) {}
    virtual void suspend() = 0;
    virtual void resume() {}
    virtual void setEnableTray(bool) = 0;
};

struct NotEmpty {
    bool check(const std::string &value) const { return !value.empty(); }
    void dumpDescription(RawConfig &) const {}
};

FCITX_CONFIGURATION(
    ClassicUIConfig,
    Option<bool> verticalCandidateList{this, "Vertical Candidate List",
                                       _("Vertical Candidate List"), false};
    Option<bool> perScreenDPI{this, "PerScreenDPI", _("Use Per Screen DPI"),
                              true};

    OptionWithAnnotation<std::string, FontAnnotation> font{this, "Font", "Font",
                                                           "Sans 9"};
    Option<std::string, NotEmpty> theme{this, "Theme", _("Theme"), "default"};);

class ClassicUI : public UserInterface {
public:
    ClassicUI(Instance *instance);
    ~ClassicUI();

    AddonInstance *xcb();
    AddonInstance *wayland();
    Instance *instance() { return instance_; }
    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/classicui.conf");
    }
    auto &config() { return config_; }
    Theme &theme() { return theme_; }
    void suspend() override;
    void resume() override;
    bool suspended() const { return suspended_; }
    bool available() override { return true; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void reloadConfig() override;

private:
    FCITX_ADDON_DEPENDENCY_LOADER(notificationitem, instance_->addonManager());

    UIInterface *uiForEvent(Event &event);
    UIInterface *uiForInputContext(InputContext *inputContext);

    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
        xcbCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> xcbClosedCallback_;

    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
        waylandCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
        waylandClosedCallback_;

    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    std::unique_ptr<HandlerTableEntryBase> sniHandler_;

    std::unordered_map<std::string, std::unique_ptr<UIInterface>> uis_;

    Instance *instance_;
    ClassicUIConfig config_;
    Theme theme_;
    bool suspended_ = true;
};
}
}

#endif // _FCITX_UI_CLASSIC_CLASSICUI_H_
