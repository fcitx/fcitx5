//
// Copyright (C) 2020~2020 by CSSlayer
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
#ifndef _FCITX5_MODULES_EMOJI_EMOJI_PUBLIC_H_
#define _FCITX5_MODULES_EMOJI_EMOJI_PUBLIC_H_

#include <fcitx/addoninstance.h>
#include <string>

FCITX_ADDON_DECLARE_FUNCTION(Emoji, query,
                             const std::vector<std::string> &(
                                 const std::string &language,
                                 const std::string &key, bool fallbackToEn));

#endif // _FCITX5_MODULES_EMOJI_EMOJI_PUBLIC_H_
