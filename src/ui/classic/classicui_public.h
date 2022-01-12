/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_UI_CLASSIC_CLASSICUI_PUBLIC_H_
#define _FCITX5_UI_CLASSIC_CLASSICUI_PUBLIC_H_

#include <string>
#include <vector>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>

FCITX_ADDON_DECLARE_FUNCTION(
    ClassicUI, labelIcon,
    std::vector<unsigned char>(const std::string &label, unsigned int size));
FCITX_ADDON_DECLARE_FUNCTION(ClassicUI, preferTextIcon, bool());
FCITX_ADDON_DECLARE_FUNCTION(ClassicUI, showLayoutNameInIcon, bool());

#endif // _FCITX5_UI_CLASSIC_CLASSICUI_PUBLIC_H_
