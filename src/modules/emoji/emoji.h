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
#ifndef _FCITX5_MODULES_EMOJI_EMOJI_H_
#define _FCITX5_MODULES_EMOJI_EMOJI_H_

#include <unordered_map>
#include <vector>
#include "fcitx/addoninstance.h"
#include "emoji_public.h"

namespace fcitx {
using EmojiMap = std::unordered_map<std::string, std::vector<std::string>>;

class Emoji final : public AddonInstance {

public:
    Emoji();
    ~Emoji();

    const std::vector<std::string> &query(const std::string &language,
                                          const std::string &key,
                                          bool fallbackToEn);

private:
    FCITX_ADDON_EXPORT_FUNCTION(Emoji, query);

    const EmojiMap *loadEmoji(const std::string &language, bool fallbackToEn);
    std::unordered_map<std::string, EmojiMap> langToEmojiMap_;
};
} // namespace fcitx

#endif // _FCITX5_MODULES_EMOJI_EMOJI_H_
