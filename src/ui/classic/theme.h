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
#ifndef _FCITX_UI_CLASSIC_THEME_H_
#define _FCITX_UI_CLASSIC_THEME_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-utils/log.h"
#include "fcitx/icontheme.h"
#include <cairo/cairo.h>

namespace fcitx {

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
    Option<int> overlayOffsetY{this, "OverlayOffsetY", "Overlay Y offset"};)

FCITX_CONFIGURATION(
    InputPanelThemeConfig,
    Option<std::string> font{this, "Font", "Font", "Sans 9"};
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
    ThemeConfig, Option<I18NString> name{this, "Metadata/Name", "Skin Name"};
    Option<int> version{this, "Metadata/Version", "Version", 1};
    Option<std::string> author{this, "Metadata/Author", "Author"};
    Option<std::string> description{this, "Metadata/Description",
                                    "Description"};
    Option<bool> scaleWithDPI{this, "Metadata/ScaleWithDPI", "Scale with DPI"};
    Option<std::string> trayFont{this, "TrayFont", "Tray Font", "Sans 9"};
    Option<InputPanelThemeConfig> inputPanel{this, "InputPanel",
                                             "Input Panel Theme"};);

enum class ImagePurpose { General, Tray };

class ThemeImage {
public:
    ThemeImage(const std::string &name, const BackgroundImageConfig &cfg);
    ThemeImage(const std::string &icon, const std::string &label,
               const std::string &font, uint32_t size);

    operator cairo_surface_t *() const { return image_.get(); }

    auto size() { return size_; }

private:
    std::string currentText_;
    uint32_t size_ = 0;
    std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)> image_;
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
               int height);

private:
    std::unordered_map<const BackgroundImageConfig *, ThemeImage>
        backgroundImageTable_;
    std::unordered_map<std::string, ThemeImage> imageTable_;
    std::unordered_map<std::string, ThemeImage> trayImageTable_;
    IconTheme iconTheme_;
    std::string name_;
};

inline void cairoSetSourceColor(cairo_t *cr, const Color &color) {
    cairo_set_source_rgba(cr, color.redF(), color.blueF(), color.greenF(),
                          color.alphaF());
}

FCITX_DECLARE_LOG_CATEGORY(classicui_logcategory);
#define CLASSICUI_DEBUG() FCITX_LOGC(::fcitx::classicui_logcategory, Debug)
}

#endif // _FCITX_UI_CLASSIC_THEME_H_
