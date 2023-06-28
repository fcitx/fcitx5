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
#include "fcitx-config/option.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"
#include "classicui_public.h"
#include "plasmathemewatchdog.h"
#include "theme.h"
#ifdef ENABLE_X11
#include "xcb_public.h"
#endif
#ifdef WAYLAND_FOUND
#include "wayland_public.h"
#endif
#ifdef ENABLE_DBUS
#include "fcitx-utils/dbus/bus.h"
#endif

namespace fcitx {
namespace classicui {

inline constexpr std::string_view PlasmaThemeName = "plasma";

class UIInterface {
public:
    UIInterface(std::string name) : name_(name) {}

    const std::string &name() const { return name_; }

    virtual ~UIInterface() {}
    virtual void update(UserInterfaceComponent component,
                        InputContext *inputContext) = 0;
    virtual void updateCursor(InputContext *) {}
    virtual void updateCurrentInputMethod(InputContext *) {}
    virtual void suspend() = 0;
    virtual void resume() {}
    virtual void setEnableTray(bool) = 0;

private:
    const std::string name_;
};

struct NotEmpty {
    bool check(const std::string &value) { return !value.empty(); }
    void dumpDescription(RawConfig &) const {}
};

struct ThemeAnnotation : public EnumAnnotation {
    void setThemes(std::vector<std::pair<std::string, std::string>> themes,
                   bool plasmaTheme) {
        themes_ = std::move(themes);
        plasmaTheme_ = plasmaTheme;
    }
    void dumpDescription(RawConfig &config) const {
        EnumAnnotation::dumpDescription(config);
        config.setValueByPath("LaunchSubConfig", "True");
        for (size_t i = 0; i < themes_.size(); i++) {
            config.setValueByPath("Enum/" + std::to_string(i),
                                  themes_[i].first);
            config.setValueByPath("EnumI18n/" + std::to_string(i),
                                  themes_[i].second);
            if (themes_[i].first != PlasmaThemeName || !plasmaTheme_) {
                config.setValueByPath(
                    "SubConfigPath/" + std::to_string(i),
                    stringutils::concat("fcitx://config/addon/classicui/theme/",
                                        themes_[i].first));
            } else {
                config.setValueByPath("SubConfigPath/" + std::to_string(i), "");
            }
        }
    }

private:
    std::vector<std::pair<std::string, std::string>> themes_;
    bool plasmaTheme_ = false;
};

struct MenuFontAnnotation : private FontAnnotation, private ToolTipAnnotation {
    MenuFontAnnotation()
        : ToolTipAnnotation(
              _("This is only effective when the tray icon is xembed.")) {}

    bool skipDescription() { return false; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &config) {
        FontAnnotation::dumpDescription(config);
        ToolTipAnnotation::dumpDescription(config);
    }
};

FCITX_CONFIGURATION(
    ClassicUIConfig,
    Option<bool> verticalCandidateList{this, "Vertical Candidate List",
                                       _("Vertical Candidate List"), false};
    Option<bool> useWheelForPaging{
        this, "WheelForPaging", _("Use mouse wheel to go to prev or next page"),
        true};

    OptionWithAnnotation<std::string, FontAnnotation> font{
        this, "Font", _("Font"), "Sans 10"};
    OptionWithAnnotation<std::string, MenuFontAnnotation> menuFont{
        this, "MenuFont", _("Menu Font"), "Sans 10"};
    OptionWithAnnotation<std::string, FontAnnotation> trayFont{
        this, "TrayFont", _("Tray Font"), "Sans Bold 10"};
    Option<Color> trayBorderColor{this, "TrayOutlineColor",
                                  _("Tray Label Outline Color"),
                                  Color("#000000ff")};
    Option<Color> trayTextColor{this, "TrayTextColor",
                                _("Tray Label Text Color"), Color("#ffffffff")};
    Option<bool> preferTextIcon{this, "PreferTextIcon", _("Prefer Text Icon"),
                                false};
    OptionWithAnnotation<bool, ToolTipAnnotation> showLayoutNameInIcon{
        this,
        "ShowLayoutNameInIcon",
        _("Show Layout Name In Icon"),
        true,
        {},
        {},
        {_("Show layout name in icon if there is more than one active layout. "
           "If prefer text icon is set to true, this option will be "
           "ignored.")}};
    OptionWithAnnotation<bool, ToolTipAnnotation>
        useInputMethodLanguageToDisplayText{
            this,
            "UseInputMethodLangaugeToDisplayText",
            _("Use input method language to display text"),
            true,
            {},
            {},
            {_("For example, display character with Chinese variant when using "
               "Pinyin and Japanese variant when using Anthy. The font "
               "configuration needs to support this to use this feature.")}};
    Option<std::string, NotEmpty, DefaultMarshaller<std::string>,
           ThemeAnnotation>
        theme{this, "Theme", _("Theme"), "default"};
    Option<std::string, NotEmpty, DefaultMarshaller<std::string>,
           ThemeAnnotation>
        themeDark{this, "DarkTheme", _("Dark Theme"), "default-dark"};
    Option<bool> useDarkTheme{this, "UseDarkTheme",
                              _("Follow system light/dark color scheme"),
                              false};
    OptionWithAnnotation<bool, ToolTipAnnotation> perScreenDPI{
        this,
        "PerScreenDPI",
        _("Use Per Screen DPI on X11"),
        false,
        {},
        {},
        {_("This option will be always disabled on XWayland.")}};
    Option<int, IntConstrain, DefaultMarshaller<int>, ToolTipAnnotation>
        forceWaylandDPI{
            this,
            "ForceWaylandDPI",
            _("Force font DPI on Wayland"),
            0,
            IntConstrain(0),
            {},
            {_("Normally Wayland uses 96 as font DPI in combinition with the "
               "screen scale factor. This option allows you to override the "
               "font DPI. If the value is 0, it means this option is "
               "disabled.")}};

    OptionWithAnnotation<bool, ToolTipAnnotation> fractionalScale{
        this,
        "EnableFractionalScale",
        _("Enable fractional scale under Wayland"),
        true,
        {},
        {},
        {_("This option require support from wayland compositor.")}};);

class ClassicUI final : public UserInterface {
public:
    ClassicUI(Instance *instance);
    ~ClassicUI();

    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(wayland, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(waylandim, instance_->addonManager());
    Instance *instance() const { return instance_; }
    const Configuration *getConfig() const override;
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/classicui.conf");
        reloadTheme();
    }
    const Configuration *getSubConfig(const std::string &path) const override;
    void setSubConfig(const std::string &path,
                      const RawConfig &config) override;
    auto &config() const { return config_; }
    Theme &theme() { return theme_; }
    void suspend() override;
    void resume() override;
    bool suspended() const { return suspended_; }
    bool available() override { return true; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void reloadConfig() override;

    std::vector<unsigned char> labelIcon(const std::string &label,
                                         unsigned int size);
    bool preferTextIcon() const;
    bool showLayoutNameInIcon() const;

private:
    FCITX_ADDON_DEPENDENCY_LOADER(notificationitem, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());
    FCITX_ADDON_EXPORT_FUNCTION(ClassicUI, labelIcon);
    FCITX_ADDON_EXPORT_FUNCTION(ClassicUI, preferTextIcon);
    FCITX_ADDON_EXPORT_FUNCTION(ClassicUI, showLayoutNameInIcon);

    UIInterface *uiForEvent(Event &event);
    UIInterface *uiForInputContext(InputContext *inputContext);
    UIInterface *uiForDisplay(const std::string &display);
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

    std::unique_ptr<EventSource> deferedReloadTheme_;
#ifdef ENABLE_DBUS
    std::unique_ptr<dbus::Slot> appearanceChangedSlot_;
    std::unique_ptr<dbus::Slot> initialReadSlot_;
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
    bool isDark_ = false;

    std::unique_ptr<EventSource> deferedEnableTray_;
    std::unique_ptr<PlasmaThemeWatchdog> plasmaThemeWatchdog_;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_CLASSICUI_H_
