//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_MODULES_CLIPBOARD_CLIPBOARD_PUBLIC_H_
#define _FCITX_MODULES_CLIPBOARD_CLIPBOARD_PUBLIC_H_

#include <fcitx-utils/dbus/bus.h>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>
#include <string>

FCITX_ADDON_DECLARE_FUNCTION(Clipboard, primary,
                             std::string(const fcitx::InputContext *ic));
FCITX_ADDON_DECLARE_FUNCTION(Clipboard, clipboard,
                             std::string(const fcitx::InputContext *ic));

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARD_PUBLIC_H_
