/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_THEME_H_
#define _FCITX_UI_CLASSIC_THEME_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cairo.h>
#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-config/option.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/color.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/i18nstring.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/rect.h"
#include "fcitx/icontheme.h"

namespace fcitx::classicui {
enum class Gravity {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};
FCITX_CONFIG_ENUM_NAME_WITH_I18N(Gravity, N_("Top Left"), N_("Top Center"),
                                 N_("Top Right"), N_("Center Left"),
                                 N_("Center"), N_("Center Right"),
                                 N_("Bottom Left"), N_("Bottom Center"),
                                 N_("Bottom Right"));

enum class PageButtonAlignment {
    Top,
    FirstCandidate,
    Center,
    LastCandidate,
    Bottom
};
FCITX_CONFIG_ENUM_NAME_WITH_I18N(PageButtonAlignment, N_("Top"),
                                 N_("First Candidate"), N_("Center"),
                                 N_("Last Candidate"), N_("Bottom"));

enum class ColorField {
    InputPanel_Background,
    InputPanel_Border,
    InputPanel_HighlightCandidateBackground,
    InputPanel_HighlightCandidateBorder,
    InputPanel_Highlight,
    Menu_Background,
    Menu_Border,
    Menu_SelectedItemBackground,
    Menu_SelectedItemBorder,
    Menu_Separator,
};
FCITX_CONFIG_ENUM_NAME_WITH_I18N(
    ColorField, N_("Input Panel Background"), N_("Input Panel Border"),
    N_("Input Panel Highlight Candidate Background"),
    N_("Input Panel Highlight Candidate Border"), N_("Input Panel Highlight"),
    N_("Menu Background"), N_("Menu Border"),
    N_("Menu Selected Item Background"), N_("Menu Selected Item Border"),
    N_("Menu Separator"));

FCITX_CONFIGURATION(
    MarginConfig,
    Option<int, IntConstrain> marginLeft{this, "Left", _("Margin Left"), 0,
                                         IntConstrain(0)};
    Option<int, IntConstrain> marginRight{this, "Right", _("Margin Right"), 0,
                                          IntConstrain(0)};
    Option<int, IntConstrain> marginTop{this, "Top", _("Margin Top"), 0,
                                        IntConstrain(0)};
    Option<int, IntConstrain> marginBottom{this, "Bottom", _("Margin Bottom"),
                                           0, IntConstrain(0)};)

FCITX_CONFIGURATION(
    BackgroundImageConfig,
    Option<std::string> image{this, "Image", _("Background Image")};
    OptionWithAnnotation<Color, ToolTipAnnotation> color{
        this,
        "Color",
        _("Color"),
        Color("#ffffff"),
        {},
        {},
        {_("This option is only effective if image is not set.")}};
    OptionWithAnnotation<Color, ToolTipAnnotation> borderColor{
        this,
        "BorderColor",
        _("Border Color"),
        Color("#ffffff00"),
        {},
        {},
        {_("This option is only effective if image is not set.")}};
    Option<int, IntConstrain, DefaultMarshaller<int>, ToolTipAnnotation>
        borderWidth{this,
                    "BorderWidth",
                    _("Border width"),
                    0,
                    IntConstrain(0),
                    {},
                    {_("This value should be less than any of margin value.")}};
    Option<std::string> overlay{this, "Overlay", _("Overlay Image")};
    OptionWithAnnotation<Gravity, GravityI18NAnnotation> gravity{
        this, "Gravity", _("Overlay position")};
    Option<int> overlayOffsetX{this, "OverlayOffsetX", _("Overlay X offset")};
    Option<int> overlayOffsetY{this, "OverlayOffsetY", _("Overlay Y offset")};
    Option<bool> hideOverlayIfOversize{this, "HideOverlayIfOversize",
                                       _("Hide overlay if size does not fit"),
                                       false};
    Option<MarginConfig> margin{this, "Margin", _("Margin")};
    Option<MarginConfig> overlayClipMargin{this, "OverlayClipMargin",
                                           _("Overlay Clip Margin")};)

FCITX_CONFIGURATION_EXTEND(HighlightBackgroundImageConfig,
                           BackgroundImageConfig,
                           Option<MarginConfig> clickMargin{
                               this, "HighlightClickMargin",
                               _("Highlight Click Margin")};);

FCITX_CONFIGURATION(ActionImageConfig,
                    Option<std::string> image{this, "Image", _("Image")};
                    Option<MarginConfig> clickMargin{this, "ClickMargin",
                                                     _("Click Margin")};)

FCITX_CONFIGURATION(
    InputPanelThemeConfig,
    Option<Color> normalColor{this, "NormalColor", _("Normal text color"),
                              Color("#000000ff")};
    Option<Color> highlightColor{this, "HighlightColor",
                                 _("Highlight text color"), Color("#ffffffff")};
    Option<Color> highlightBackgroundColor{this, "HighlightBackgroundColor",
                                           _("Highlight Background color"),
                                           Color("#a5a5a5ff")};
    Option<Color> highlightCandidateColor{this, "HighlightCandidateColor",
                                          _("Highlight Candidate Color"),
                                          Color("#ffffffff")};
    Option<bool> enableBlur{this, "EnableBlur", _("Enable Blur on KWin"),
                            false};
    Option<std::string> blurMask{this, "BlurMask", _("Blur mask"), ""};
    Option<MarginConfig> blurMargin{this, "BlurMargin", _("Blur Margin")};
    Option<bool> fullWidthHighlight{
        this, "FullWidthHighlight",
        _("Use all horizontal space for highlight when it is vertical list"),
        true};
    OptionWithAnnotation<PageButtonAlignment, PageButtonAlignmentI18NAnnotation>
        buttonAlignment{this, "PageButtonAlignment",
                        _("Page button vertical alignment"),
                        PageButtonAlignment::Bottom};
    Option<BackgroundImageConfig> background{this, "Background",
                                             _("Background")};
    Option<HighlightBackgroundImageConfig> highlight{this, "Highlight",
                                                     _("Highlight Background")};
    Option<MarginConfig> contentMargin{this, "ContentMargin",
                                       _("Margin around all content")};
    Option<MarginConfig> textMargin{this, "TextMargin",
                                    _("Margin around text")};
    Option<ActionImageConfig> prev{this, "PrevPage", _("Prev Page Button")};
    Option<ActionImageConfig> next{this, "NextPage", _("Next Page Button")};
    Option<MarginConfig> shadowMargin{this, "ShadowMargin",
                                      _("Shadow Margin")};);
FCITX_CONFIGURATION(
    MenuThemeConfig,
    Option<Color> normalColor{this, "NormalColor", _("Normal text color"),
                              Color("#000000ff")};
    Option<Color> highlightTextColor{this, "HighlightCandidateColor",
                                     _("Selected Item text color"),
                                     Color("#ffffffff")};
    Option<int> spacing{this, "Spacing", _("Spacing"), 0};
    Option<BackgroundImageConfig> background{this, "Background",
                                             _("Background")};
    Option<BackgroundImageConfig> highlight{this, "Highlight",
                                            _("Highlight Background")};
    Option<BackgroundImageConfig> separator{this, "Separator",
                                            _("Separator Background")};
    Option<BackgroundImageConfig> checkBox{this, "CheckBox", _("Check box")};
    Option<BackgroundImageConfig> subMenu{this, "SubMenu", _("Sub Menu")};
    Option<MarginConfig> contentMargin{this, "ContentMargin",
                                       _("Margin around all content")};
    Option<MarginConfig> textMargin{this, "TextMargin",
                                    _("Margin around text")};);

FCITX_CONFIGURATION(ThemeMetadata,
                    Option<I18NString> name{this, "Name", _("Name")};
                    Option<int> version{this, "Version", _("Version"), 1};
                    Option<std::string> author{this, "Author", _("Author")};
                    Option<I18NString> description{this, "Description",
                                                   _("Description")};)

FCITX_CONFIGURATION(ThemeConfig,
                    HiddenOption<ThemeMetadata> metadata{this, "Metadata",
                                                         _("Metadata")};
                    Option<InputPanelThemeConfig> inputPanel{this, "InputPanel",
                                                             _("Input Panel")};
                    Option<MenuThemeConfig> menu{this, "Menu", _("Menu")};
                    Option<std::vector<ColorField>> accentColor{
                        this, "AccentColorField", _("Accent Colors")};);

class ClassicUI;
class ClassicUIConfig;

class ThemeImage {
public:
    ThemeImage(const std::string &name, const BackgroundImageConfig &cfg,
               const Color &color, const Color &borderColor);
    ThemeImage(const std::string &name, const ActionImageConfig &cfg);
    ThemeImage(const IconTheme &iconTheme, const std::string &icon,
               const std::string &label, uint32_t size,
               const ClassicUI *classicui);

    static void drawTextIcon(cairo_surface_t *surface, const std::string &label,
                             uint32_t size, const ClassicUIConfig &config);

    operator cairo_surface_t *() const { return image_.get(); }
    auto height() const {
        int height = 1;
        if (image_) {
            height = cairo_image_surface_get_height(image_.get());
        }
        return height <= 0 ? 1 : height;
    }
    auto width() const {
        int width = 1;
        if (image_) {
            width = cairo_image_surface_get_width(image_.get());
        }
        return width <= 0 ? 1 : width;
    }

    auto size() const { return size_; }

    bool valid() const { return valid_; }
    cairo_surface_t *overlay() const { return overlay_.get(); }
    auto overlayWidth() const {
        int width = 1;
        if (overlay_) {
            width = cairo_image_surface_get_width(overlay_.get());
        }
        return width <= 0 ? 1 : width;
    }
    auto overlayHeight() const {
        int height = 1;
        if (overlay_) {
            height = cairo_image_surface_get_height(overlay_.get());
        }
        return height <= 0 ? 1 : height;
    }
    bool isImage() const { return isImage_; }

private:
    bool valid_ = false;
    std::string currentText_;
    uint32_t size_ = 0;
    bool isImage_ = false;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> image_;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> overlay_;
};

class Theme : public ThemeConfig {
public:
    Theme();
    ~Theme();

    const std::string &name() { return name_; }
    void load(std::string_view name);
    void load(std::string_view name, const RawConfig &rawConfig);
    const ThemeImage &loadImage(const std::string &icon,
                                const std::string &label, uint32_t size,
                                const ClassicUI *classicui);
    const ThemeImage &loadBackground(const BackgroundImageConfig &cfg);
    const ThemeImage &loadAction(const ActionImageConfig &cfg);

    void paint(cairo_t *c, const BackgroundImageConfig &cfg, int width,
               int height, double alpha, double scale);

    void paint(cairo_t *c, const ActionImageConfig &cfg, double alpha = 1.0);

    std::vector<Rect> mask(const BackgroundImageConfig &cfg, int width,
                           int height);

    bool setIconTheme(const std::string &name);

    const auto &maskConfig() const { return maskConfig_; }
    const auto &accentColorFields() const { return accentColorFields_; }

    void populateColor(std::optional<Color> accent);

    const auto &inputPanelBackground() const { return inputPanelBackground_; }
    const auto &inputPanelBorder() const { return inputPanelBorder_; }
    const auto &inputPanelHighlightCandidateBackground() const {
        return inputPanelHighlightCandidateBackground_;
    }
    const auto &inputPanelHighlightCandidateBorder() const {
        return inputPanelHighlightCandidateBorder_;
    }
    const auto &inputPanelHighlight() const { return inputPanelHighlight_; }
    const auto &inputPanelText() const { return inputPanelText_; }
    const auto &inputPanelHighlightText() const {
        return inputPanelHighlightText_;
    }
    const auto &inputPanelHighlightCandidateText() const {
        return inputPanelHighlightCandidateText_;
    }

    const auto &menuBackground() const { return menuBackground_; }
    const auto &menuBorder() const { return menuBorder_; }
    const auto &menuSelectedItemBackground() const {
        return menuSelectedItemBackground_;
    }
    const auto &menuSelectedItemBorder() const {
        return menuSelectedItemBorder_;
    }
    const auto &menuSeparator() const { return menuSeparator_; }
    const auto &menuText() const { return menuText_; }
    const auto &menuSelectedItemText() const { return menuSelectedItemText_; }

private:
    void reset();

    std::unordered_map<const BackgroundImageConfig *, ThemeImage>
        backgroundImageTable_;
    std::unordered_map<const ActionImageConfig *, ThemeImage> actionImageTable_;
    std::unordered_map<std::string, ThemeImage> trayImageTable_;
    IconTheme iconTheme_;
    std::string name_;
    BackgroundImageConfig maskConfig_;
    std::unordered_set<ColorField> accentColorFields_;

    Color inputPanelBackground_;
    Color inputPanelBorder_;
    Color inputPanelHighlightCandidateBackground_;
    Color inputPanelHighlightCandidateBorder_;
    Color inputPanelHighlight_;
    Color inputPanelText_;
    Color inputPanelHighlightText_;
    Color inputPanelHighlightCandidateText_;

    Color menuBackground_;
    Color menuBorder_;
    Color menuSelectedItemBackground_;
    Color menuSelectedItemBorder_;
    Color menuSeparator_;
    Color menuText_;
    Color menuSelectedItemText_;
};

inline void cairoSetSourceColor(cairo_t *cr, const Color &color) {
    cairo_set_source_rgba(cr, color.redF(), color.greenF(), color.blueF(),
                          color.alphaF());
}

inline void shrink(Rect &rect, const MarginConfig &margin) {
    int newWidth = rect.width() - *margin.marginLeft - *margin.marginRight;
    int newHeight = rect.height() - *margin.marginTop - *margin.marginBottom;
    if (newWidth < 0) {
        newWidth = 0;
    }
    if (newHeight < 0) {
        newHeight = 0;
    }
    rect.setPosition(rect.left() + *margin.marginLeft,
                     rect.top() + *margin.marginTop);
    rect.setSize(newWidth, newHeight);
}

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_THEME_H_
