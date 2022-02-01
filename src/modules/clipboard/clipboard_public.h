/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_CLIPBOARD_CLIPBOARD_PUBLIC_H_
#define _FCITX_MODULES_CLIPBOARD_CLIPBOARD_PUBLIC_H_

#include <string>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>

FCITX_ADDON_DECLARE_FUNCTION(Clipboard, primary,
                             std::string(const fcitx::InputContext *ic));
FCITX_ADDON_DECLARE_FUNCTION(Clipboard, clipboard,
                             std::string(const fcitx::InputContext *ic));
FCITX_ADDON_DECLARE_FUNCTION(Clipboard, setPrimary,
                             void(const std::string &name, const std::string &str));
FCITX_ADDON_DECLARE_FUNCTION(Clipboard, setClipboard,
                             void(const std::string &name, const std::string &str));

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARD_PUBLIC_H_
