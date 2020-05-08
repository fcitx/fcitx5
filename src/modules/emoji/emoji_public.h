/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_EMOJI_EMOJI_PUBLIC_H_
#define _FCITX5_MODULES_EMOJI_EMOJI_PUBLIC_H_

#include <string>
#include <fcitx/addoninstance.h>

FCITX_ADDON_DECLARE_FUNCTION(Emoji, query,
                             const std::vector<std::string> &(
                                 const std::string &language,
                                 const std::string &key, bool fallbackToEn));

#endif // _FCITX5_MODULES_EMOJI_EMOJI_PUBLIC_H_
