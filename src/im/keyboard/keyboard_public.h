/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_IM_KEYBOARD_KEYBOARD_PUBLIC_H_
#define _FCITX_IM_KEYBOARD_KEYBOARD_PUBLIC_H_

#include <functional>
#include <string>
#include <vector>
#include <fcitx/addoninstance.h>

FCITX_ADDON_DECLARE_FUNCTION(
    KeyboardEngine, foreachLayout,
    bool(const std::function<
         bool(const std::string &layout, const std::string &description,
              const std::vector<std::string> &languages)> &callback));

FCITX_ADDON_DECLARE_FUNCTION(
    KeyboardEngine, foreachVariant,
    bool(const std::string &layout,
         const std::function<
             bool(const std::string &variant, const std::string &description,
                  const std::vector<std::string> &languages)> &callback));

#endif // _FCITX_IM_KEYBOARD_KEYBOARD_PUBLIC_H_
