/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

namespace fcitx {

FCITX_CONFIG_ENUM(Gravity, TopLeft, TopCenter, TopRight, CenterLeft, Center, CenterRight, BottomLeft, BottomCenter,
                  BottomRight)

FCITX_CONFIGURATION(MarginConfig, Option<int> marginLeft{this, "MarginLeft", "Margin Left"};
                    Option<int> marginRight{this, "MarginRight", "Margin Right"};
                    Option<int> marginTop{this, "MarginTop", "Margin Top"};
                    Option<int> marginBottom{this, "MarginBottom", "Margin Bottom"};)

FCITX_CONFIGURATION(BackgroundImageConfig, Option<std::string> image{this, "Image", "Background Image"};
                    Option<MarginConfig> margin{this, "Margin", "Margin"};
                    Option<MarginConfig> clickMargin{this, "ClickMargin", "Click Margin"};
                    Option<std::string> overlay{this, "Overlay", "Overlay Image"};
                    Option<Gravity> overlayGravity{this, "Overlay", "Overlay position"};
                    Option<int> overlayOffsetX{this, "OverlayOffsetX", "Overlay X offset"};
                    Option<int> overlayOffsetY{this, "OverlayOffsetY", "Overlay Y offset"};)

FCITX_CONFIGURATION(InputPanelThemeConfig, Option<std::string> font{this, "Font", "Font"};
                    Option<Color> normalColor{this, "NormatTextColor", "Normal text color"};
                    Option<Color> userInputColor{this, "UserInputColor", "User input text color"};
                    Option<Color> candidateIndexColor{this, "CandidateIndexColor", "Candidate Index color"};
                    Option<Color> currentCandiate{this, "CurrentCandiateColor", "Current candidate color"};
                    Option<Color> userPhraseColor{this, "UserPhraseColor", "User phrase text color"};
                    Option<Color> hintColor{this, "HintColor", "Hint color"};)

FCITX_CONFIGURATION(MenuThemeConfig, Option<std::string> font{this, "Font", "Font"};)

FCITX_CONFIGURATION(ThemeConfig, Option<I18NString> name{this, "Metadata/Name", "Skin Name"};
                    Option<int> version{this, "Metadata/Version", "Version", 1};
                    Option<std::string> author{this, "Metadata/Author", "Author"};
                    Option<std::string> description{this, "Metadata/Description", "Description"};
                    Option<bool> scaleWithDPI{this, "Metadata/ScaleWithDPI", "Scale with DPI"};);

class Theme : public ThemeConfig {
public:
    Theme();
    ~Theme();
};
}

#endif // _FCITX_UI_CLASSIC_THEME_H_
