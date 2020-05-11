/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "emoji.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonfactory.h"
#include "../../im/keyboard/xmlparser.h"
#include "config.h"

namespace fcitx {
class EmojiParser : public XMLParser {
public:
    EmojiParser(std::function<bool(const std::string &)> filter)
        : filter_(std::move(filter)) {}

    void startElement(const XML_Char *name, const XML_Char **attrs) override {
        // Data are like <annotation cp="..."> ...</annotation>
        if (strcmp(name, "annotation") == 0) {
            int i = 0;
            while (attrs && attrs[i * 2] != 0) {
                if (strcmp(reinterpret_cast<const char *>(attrs[i * 2]),
                           "cp") == 0) {
                    currentEmoji_ =
                        reinterpret_cast<const char *>(attrs[i * 2 + 1]);
                }
                i++;
            }
        }
    }
    void endElement(const XML_Char *name) override {
        if (strcmp(name, "annotation") == 0) {
            currentEmoji_.clear();
        }
    }
    void characterData(const XML_Char *ch, int len) override {
        if (currentEmoji_.empty()) {
            return;
        }
        std::string temp(reinterpret_cast<const char *>(ch), len);
        auto tokens = stringutils::split(temp, "|");
        std::transform(tokens.begin(), tokens.end(), tokens.begin(),
                       stringutils::trim);
        for (auto token : tokens) {
            if (token.empty() || filter_(token)) {
                continue;
            }
            auto &emojis = emojiMap_[token];
            // Certain word has a very general meaning and has tons of matches,
            // keep only 1 or 2 for specific.
            if (emojis.size() == 0 ||
                (emojis.size() == 1 && emojis[0] != currentEmoji_)) {
                emojis.push_back(currentEmoji_);
            }
        }
    }

    EmojiMap emojiMap_;

private:
    std::string currentEmoji_;
    std::function<bool(const std::string &)> filter_;
};

static const std::vector<std::string> emptyEmoji;

Emoji::Emoji() {}

Emoji::~Emoji() {}

bool Emoji::check(const std::string &language, bool fallbackToEn) {
    const EmojiMap *emojiMap = loadEmoji(language, fallbackToEn);
    return emojiMap;
}

const std::vector<std::string> &Emoji::query(const std::string &language,
                                             const std::string &key,
                                             bool fallbackToEn) {
    const EmojiMap *emojiMap = loadEmoji(language, fallbackToEn);

    if (!emojiMap) {
        return emptyEmoji;
    }

    if (auto result = findValue(*emojiMap, key)) {
        return *result;
    }

    return emptyEmoji;
}

namespace {
bool noSpace(const std::string &str) {
    return std::any_of(str.begin(), str.end(), charutils::isspace);
}
} // namespace

const EmojiMap *Emoji::loadEmoji(const std::string &language,
                                 bool fallbackToEn) {
    std::string lang = language;
    auto emojiMap = findValue(langToEmojiMap_, lang);
    if (!emojiMap) {
        static const std::unordered_map<
            std::string, std::function<bool(const std::string &)>>
            // These are having aspell/hunspell/ispell available.
            filterMap = {{"en", noSpace},
                         {"de", noSpace},
                         {"es", noSpace},
                         {"fr", noSpace},
                         {"nl", noSpace},
                         {"ca", noSpace},
                         {"cs", noSpace},
                         {"el", noSpace},
                         {"hu", noSpace},
                         {"he", noSpace},
                         {"it", noSpace},
                         {"nb", noSpace},
                         {"nn", noSpace},
                         {"pl", noSpace},
                         {"pt", noSpace},
                         {"ro", noSpace},
                         {"ru", noSpace},
                         {"sv", noSpace},
                         {"uk", noSpace},
                         {"zh",
                          [](const std::string &str) {
                              return utf8::lengthValidated(str) > 2;
                          }},
                         {"zh_Hant_HK",
                          [](const std::string &str) {
                              return utf8::lengthValidated(str) > 2;
                          }},
                         {"zh_Hant", [](const std::string &str) {
                              return utf8::lengthValidated(str) > 2;
                          }}};
        auto filter = findValue(filterMap, lang);
        if (!filter) {
            if (!fallbackToEn) {
                return nullptr;
            } else {
                filter = findValue(filterMap, "en");
                lang = "en";
            }
        }
        const auto file =
            stringutils::joinPath(CLDR_EMOJI_ANNOTATION_PREFIX,
                                  "/share/unicode/cldr/common/annotations",
                                  stringutils::concat(lang, ".xml"));
        EmojiParser parser(*filter);
        parser.parse(file);
        emojiMap = &(langToEmojiMap_[lang] = std::move(parser.emojiMap_));
        FCITX_INFO() << "Trying to load emoji for " << lang << " from " << file
                     << ": " << emojiMap->size() << " entry(s) loaded.";
    }

    return emojiMap;
}

class EmojiModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *) override { return new Emoji; }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::EmojiModuleFactory);
