/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <algorithm>
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/log.h"
#include "theme.h"

using namespace fcitx;
using namespace fcitx::classicui;

int main() {
    ThemeConfig panelOnly;
    RawConfig panelOnlyRaw;
    panelOnlyRaw.setValueByPath("InputPanel/NormalColor", "#eeeeee");
    panelOnlyRaw.setValueByPath("InputPanel/HighlightCandidateColor",
                                "#ffffff");
    panelOnlyRaw.setValueByPath("InputPanel/Background/Image", "panel.svg");
    panelOnlyRaw.setValueByPath("InputPanel/Highlight/Image", "highlight.svg");
    panelOnlyRaw.setValueByPath("InputPanel/ContentMargin/Left", "7");
    panelOnlyRaw.setValueByPath("InputPanel/TextMargin/Left", "12");
    panelOnlyRaw.setValueByPath("AccentColorField/0", "Input Panel Background");
    panelOnlyRaw.setValueByPath("AccentColorField/1",
                                "Input Panel Highlight Candidate Background");
    panelOnly.load(panelOnlyRaw, true);
    inheritMenuStyle(panelOnly, panelOnlyRaw.get("Menu") != nullptr);
    FCITX_ASSERT(*panelOnly.menu->normalColor == Color("#eeeeee"));
    FCITX_ASSERT(*panelOnly.menu->highlightTextColor == Color("#ffffff"));
    FCITX_ASSERT(*panelOnly.menu->background->image == "panel.svg");
    FCITX_ASSERT(*panelOnly.menu->highlight->image == "highlight.svg");
    FCITX_ASSERT(*panelOnly.menu->contentMargin->marginLeft == 7);
    FCITX_ASSERT(*panelOnly.menu->textMargin->marginLeft == 12);
    const auto &inheritedAccents = *panelOnly.accentColor;
    FCITX_ASSERT(std::find(inheritedAccents.begin(), inheritedAccents.end(),
                           ColorField::Menu_Background) !=
                 inheritedAccents.end());
    FCITX_ASSERT(std::find(inheritedAccents.begin(), inheritedAccents.end(),
                           ColorField::Menu_SelectedItemBackground) !=
                 inheritedAccents.end());

    ThemeConfig explicitMenu;
    RawConfig explicitMenuRaw = panelOnlyRaw;
    explicitMenuRaw.setValueByPath("Menu/Background/Image", "menu.svg");
    explicitMenuRaw.setValueByPath("Menu/NormalColor", "#123456");
    explicitMenu.load(explicitMenuRaw, true);
    inheritMenuStyle(explicitMenu, explicitMenuRaw.get("Menu") != nullptr);
    FCITX_ASSERT(*explicitMenu.menu->background->image == "menu.svg");
    FCITX_ASSERT(*explicitMenu.menu->normalColor == Color("#123456"));
    const auto &explicitAccents = *explicitMenu.accentColor;
    FCITX_ASSERT(std::find(explicitAccents.begin(), explicitAccents.end(),
                           ColorField::Menu_Background) ==
                 explicitAccents.end());
    return 0;
}
