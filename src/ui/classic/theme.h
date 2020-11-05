/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_THEME_H_
#define _FCITX_UI_CLASSIC_THEME_H_

#include <cairo/cairo.h>
#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-utils/log.h"
#include "fcitx/icontheme.h"

namespace fcitx {
namespace classicui {
FCITX_CONFIG_ENUM(Gravity, TopLeft, TopCenter, TopRight, CenterLeft, Center,
                  CenterRight, BottomLeft, BottomCenter, BottomRight)

FCITX_CONFIGURATION(MarginConfig,
                    Option<int, IntConstrain> marginLeft{
                        this, "Left", "Margin Left", 0, IntConstrain(0)};
                    Option<int, IntConstrain> marginRight{this, "Right",
                                                          "Margin Right", 0,
                                                          IntConstrain(0)};
                    Option<int, IntConstrain> marginTop{
                        this, "Top", "Margin Top", 0, IntConstrain(0)};
                    Option<int, IntConstrain> marginBottom{
                        this, "Bottom", "Margin Bottom", 0, IntConstrain(0)};)

FCITX_CONFIGURATION(
    BackgroundImageConfig,
    Option<std::string> image{this, "Image", "Background Image"};
    Option<Color> color{this, "Color", "Color", Color("#ffffff")};
    Option<MarginConfig> margin{this, "Margin", "Margin"};
    Option<MarginConfig> clickMargin{this, "ClickMargin", "Click Margin"};
    Option<std::string> overlay{this, "Overlay", "Overlay Image"};
    Option<Gravity> gravity{this, "Gravity", "Overlay position"};
    Option<int> overlayOffsetX{this, "OverlayOffsetX", "Overlay X offset"};
    Option<int> overlayOffsetY{this, "OverlayOffsetY", "Overlay Y offset"};
    Option<MarginConfig> overlayClipMargin{this, "OverlayClipMargin",
                                           "Overlay Clip Margin"};
    Option<bool> hideOverlayIfOversize{this, "HideOverlayIfOversize",
                                       "Hide overlay if size does not fit",
                                       false};)

FCITX_CONFIGURATION(
    InputPanelThemeConfig,
    Option<BackgroundImageConfig> background{this, "Background", "Background"};
    Option<BackgroundImageConfig> highlight{this, "Highlight",
                                            "Highlight Background"};
    Option<MarginConfig> contentMargin{this, "ContentMargin",
                                       "Margin around all content"};
    Option<MarginConfig> textMargin{this, "TextMargin", "Margin around text"};
    Option<Color> normalColor{this, "NormalColor", "Normal text color",
                              Color("#000000ff")};
    Option<Color> highlightCandidateColor{this, "HighlightCandidateColor",
                                          "Highlight Candidate Color",
                                          Color("#ffffffff")};
    Option<BackgroundImageConfig> prev{this, "PrevPage", ""};
    Option<BackgroundImageConfig> next{this, "NextPage", ""};
    Option<int> spacing{this, "Spacing", "Spacing", 0};
    Option<bool> fullWidthHighlight{
        this, "FullWidthHighlight",
        "Use all horizontal space for highlight when it is vertical list",
        true};
    Option<Color> highlightColor{this, "HighlightColor", "Highlight text color",
                                 Color("#ffffffff")};
    Option<Color> highlightBackgroundColor{this, "HighlightBackgroundColor",
                                           "Highlight Background color",
                                           Color("#a5a5a5ff")};);
FCITX_CONFIGURATION(
    MenuThemeConfig, Option<std::string> font{this, "Font", "Font", "Sans 9"};
    Option<BackgroundImageConfig> background{this, "Background", "Background"};
    Option<BackgroundImageConfig> highlight{this, "Highlight",
                                            "Highlight Background"};
    Option<BackgroundImageConfig> separator{this, "Separator",
                                            "Separator Background"};
    Option<BackgroundImageConfig> checkBox{this, "CheckBox", ""};
    Option<BackgroundImageConfig> subMenu{this, "SubMenu", ""};
    Option<MarginConfig> contentMargin{this, "ContentMargin",
                                       "Margin around all content"};
    Option<MarginConfig> textMargin{this, "TextMargin", "Margin around text"};
    Option<Color> normalColor{this, "NormalColor", "Normal text color",
                              Color("#000000ff")};
    Option<Color> highlightTextColor{this, "HighlightCandidateColor",
                                     "Highlight Candidate Color",
                                     Color("#ffffffff")};
    Option<int> spacing{this, "Spacing", "Spacing", 0};);

FCITX_CONFIGURATION(ThemeMetadata,
                    Option<I18NString> name{this, "Name", "Skin Name"};
                    Option<int> version{this, "Version", "Version", 1};
                    Option<std::string> author{this, "Author", "Author"};
                    Option<std::string> description{this, "Description",
                                                    "Description"};)

FCITX_CONFIGURATION(ThemeGeneralConfig,
                    Option<std::string> trayFont{this, "TrayFont", "Tray Font",
                                                 "Sans 9"};
                    Option<bool> scaleWithDPI{this, "ScaleWithDPI",
                                              "Scale with DPI"};);

FCITX_CONFIGURATION(
    ThemeConfig, Option<ThemeMetadata> metadata{this, "Metadata", "Metadata"};
    Option<ThemeGeneralConfig> config{this, "General", "General"};
    Option<InputPanelThemeConfig> inputPanel{this, "InputPanel",
                                             "Input Panel Theme"};
    Option<MenuThemeConfig> menu{this, "Menu", "Menu Theme"};);

enum class ImagePurpose { General, Tray };

class ThemeImage {
public:
    ThemeImage(const std::string &name, const BackgroundImageConfig &cfg);
    ThemeImage(const std::string &icon, const std::string &label,
               const std::string &font, uint32_t size);

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

private:
    bool valid_ = false;
    std::string currentText_;
    uint32_t size_ = 0;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> image_;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> overlay_;
};

class Theme : public ThemeConfig {
public:
    Theme();
    ~Theme();

    void load(const std::string &name, const RawConfig &rawConfig);
    const ThemeImage &loadImage(const std::string &icon,
                                const std::string &label, uint32_t size,
                                ImagePurpose purpose = ImagePurpose::General);
    const ThemeImage &loadBackground(const BackgroundImageConfig &cfg);

    void paint(cairo_t *c, const BackgroundImageConfig &cfg, int width,
               int height, double alpha = 1.0);

    bool setIconTheme(const std::string &name);

private:
    std::unordered_map<const BackgroundImageConfig *, ThemeImage>
        backgroundImageTable_;
    std::unordered_map<std::string, ThemeImage> imageTable_;
    std::unordered_map<std::string, ThemeImage> trayImageTable_;
    IconTheme iconTheme_;
    std::string name_;
};

inline void cairoSetSourceColor(cairo_t *cr, const Color &color) {
    cairo_set_source_rgba(cr, color.redF(), color.greenF(), color.blueF(),
                          color.alphaF());
}
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_THEME_H_
