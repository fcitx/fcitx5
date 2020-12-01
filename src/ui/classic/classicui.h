/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_CLASSICUI_H_
#define _FCITX_UI_CLASSIC_CLASSICUI_H_

#include "config.h"

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"
#include "theme.h"
#ifdef ENABLE_X11
#include "xcb_public.h"
#endif
#ifdef WAYLAND_FOUND
#include "wayland_public.h"
#endif

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
    bool check(const std::string &value) { return !value.empty(); }
    void dumpDescription(RawConfig &) const {}
};

struct ThemeAnnotation : public EnumAnnotation {
    void setThemes(std::vector<std::pair<std::string, std::string>> themes) {
        themes_ = std::move(themes);
    }
    void dumpDescription(RawConfig &config) const {
        EnumAnnotation::dumpDescription(config);
        config.setValueByPath("LaunchSubConfig", "True");
        for (size_t i = 0; i < themes_.size(); i++) {
            config.setValueByPath("Enum/" + std::to_string(i),
                                  themes_[i].first);
            config.setValueByPath("EnumI18n/" + std::to_string(i),
                                  themes_[i].second);
            config.setValueByPath(
                "SubConfigPath/" + std::to_string(i),
                stringutils::concat("fcitx://config/addon/classicui/theme/",
                                    themes_[i].first));
        }
    }

private:
    std::vector<std::pair<std::string, std::string>> themes_;
};

FCITX_CONFIGURATION(ClassicUIConfig,
                    Option<bool> verticalCandidateList{
                        this, "Vertical Candidate List",
                        _("Vertical Candidate List"), false};
                    Option<bool> perScreenDPI{this, "PerScreenDPI",
                                              _("Use Per Screen DPI"), true};
                    Option<bool> useWheelForPaging{
                        this, "WheelForPaging",
                        _("Use mouse wheel to go to prev or next page"), true};

                    OptionWithAnnotation<std::string, FontAnnotation> font{
                        this, "Font", "Font", "Sans 9"};
                    Option<std::string, NotEmpty,
                           DefaultMarshaller<std::string>, ThemeAnnotation>
                        theme{this, "Theme", _("Theme"), "default"};);

class ClassicUI final : public UserInterface {
public:
    ClassicUI(Instance *instance);
    ~ClassicUI();

    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(wayland, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(waylandim, instance_->addonManager());
    Instance *instance() { return instance_; }
    const Configuration *getConfig() const override;
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/classicui.conf");
        reloadTheme();
    }
    const Configuration *getSubConfig(const std::string &path) const override;
    void setSubConfig(const std::string &path,
                      const RawConfig &config) override;
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
    void reloadTheme();

#ifdef ENABLE_X11
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
        xcbCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> xcbClosedCallback_;
#endif

#ifdef WAYLAND_FOUND
    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
        waylandCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
        waylandClosedCallback_;
#endif

    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    std::unique_ptr<HandlerTableEntryBase> sniHandler_;

    std::unordered_map<std::string, std::unique_ptr<UIInterface>> uis_;

    Instance *instance_;
    ClassicUIConfig config_;
    Theme theme_;
    mutable Theme subconfigTheme_;
    bool suspended_ = true;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_CLASSICUI_H_
