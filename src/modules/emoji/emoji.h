/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_EMOJI_EMOJI_H_
#define _FCITX5_MODULES_EMOJI_EMOJI_H_

#include <map>
#include <vector>
#include "fcitx/addoninstance.h"
#include "emoji_public.h"

namespace fcitx {
using EmojiMap = std::map<std::string, std::vector<std::string>>;

class Emoji final : public AddonInstance {

public:
    Emoji();
    ~Emoji();

    bool check(const std::string &language, bool fallbackToEn);
    const std::vector<std::string> &query(const std::string &language,
                                          const std::string &key,
                                          bool fallbackToEn);
    void prefix(const std::string &language, const std::string &key,
                bool fallbackToEn,
                const std::function<bool(const std::string &,
                                         const std::vector<std::string> &)> &);

private:
    FCITX_ADDON_EXPORT_FUNCTION(Emoji, query);
    FCITX_ADDON_EXPORT_FUNCTION(Emoji, check);
    FCITX_ADDON_EXPORT_FUNCTION(Emoji, prefix);

    const EmojiMap *loadEmoji(const std::string &language, bool fallbackToEn);
    std::unordered_map<std::string, EmojiMap> langToEmojiMap_;
};
} // namespace fcitx

#endif // _FCITX5_MODULES_EMOJI_EMOJI_H_
