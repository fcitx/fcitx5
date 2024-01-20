/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "emoji.h"
#include <sys/stat.h>
#include <functional>
#include <zlib.h>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonfactory.h"
#include "config.h"

namespace fcitx {

namespace {

uint32_t readInt32(const uint8_t **data, const uint8_t *end) {
    if (*data + 4 > end) {
        throw std::runtime_error("Unknown emoji dict data");
    }
    uint32_t n = FromLittleEndian32(*data);
    *data += 4;
    return n;
}

std::string_view readString(const uint8_t **data, const uint8_t *end) {
    uint32_t length = readInt32(data, end);
    if (*data + length > end) {
        throw std::runtime_error("Unknown emoji dict data");
    }
    std::string_view s(reinterpret_cast<const char *>(*data), length);
    *data += length;
    return s;
}

} // namespace

class EmojiParser {
public:
    EmojiParser(std::function<bool(std::string_view)> filter)
        : filter_(std::move(filter)) {}

    bool load(int fd) {
        struct stat s;
        if (fstat(fd, &s) < 0) {
            return false;
        }
        std::vector<uint8_t> compressed;
        std::vector<uint8_t> data;
        auto size = s.st_size;
        if (size < 4) {
            return false;
        }
        compressed.resize(size);
        if (size != fs::safeRead(fd, compressed.data(), size)) {
            return false;
        }
        uint32_t expectedSize = FromLittleEndian32(compressed.data());
        if (!expectedSize) {
            return false;
        }

        data.resize(expectedSize);

        unsigned long len = expectedSize;
        if (::uncompress(data.data(), &len, compressed.data() + 4, size - 4) !=
            Z_OK) {
            return false;
        }

        try {
            const auto *cur = data.data();
            const auto *end = data.data() + data.size();
            uint32_t nEmoji = readInt32(&cur, end);
            for (uint32_t i = 0; i < nEmoji; i++) {
                std::string_view emoji = readString(&cur, end);
                if (!utf8::validate(emoji)) {
                    throw std::runtime_error("Corrupted emoji data");
                }
                uint32_t nAnnotations = readInt32(&cur, end);
                for (uint32_t j = 0; j < nAnnotations; j++) {
                    std::string_view annotation = readString(&cur, end);
                    if (filter_ && filter_(annotation)) {
                        continue;
                    }
                    auto &emojis = emojiMap_[std::string(annotation)];
                    // Certain word has a very general meaning and has tons of
                    // matches, keep only 1 or 2 for specific.
                    if (emojis.empty() ||
                        (emojis.size() == 1 && emojis[0] != emoji)) {
                        emojis.push_back(std::string(emoji));
                    }
                }
            }
        } catch (const std::runtime_error &) {
            FCITX_ERROR() << "Failed to load emoji dictionary";
            return false;
        }
        return true;
    }

    EmojiMap emojiMap_;

private:
    std::function<bool(std::string_view)> filter_;
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

    if (const auto *result = findValue(*emojiMap, key)) {
        return *result;
    }

    return emptyEmoji;
}

void Emoji::prefix(
    const std::string &language, const std::string &key, bool fallbackToEn,
    const std::function<bool(const std::string &,
                             const std::vector<std::string> &)> &collector) {
    const EmojiMap *emojiMap = loadEmoji(language, fallbackToEn);

    if (!emojiMap) {
        return;
    }
    auto start = emojiMap->lower_bound(key);
    auto end = emojiMap->end();
    for (; start != end; start++) {
        if (!stringutils::startsWith(start->first, key)) {
            break;
        }
        if (!collector(start->first, start->second)) {
            break;
        }
    }
}

namespace {
bool noSpace(std::string_view str) {
    return std::any_of(str.begin(), str.end(), charutils::isspace);
}
} // namespace

const EmojiMap *Emoji::loadEmoji(const std::string &language,
                                 bool fallbackToEn) {
    // This is to match the file in CLDR.
    static const std::unordered_map<std::string, std::string> languageMap = {
        {"zh_TW", "zh_Hant"}, {"zh_CN", "zh"}, {"zh_HK", "zh_Hant_HK"}};

    std::string lang;
    if (const auto *mapped = findValue(languageMap, language)) {
        lang = *mapped;
    } else {
        lang = language;
    }
    auto *emojiMap = findValue(langToEmojiMap_, lang);
    if (!emojiMap) {
        // These are having aspell/hunspell/ispell available.
        static const std::unordered_map<std::string,
                                        std::function<bool(std::string_view)>>
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
                          [](std::string_view str) {
                              return utf8::lengthValidated(str) > 2;
                          }},
                         {"zh_Hant_HK",
                          [](std::string_view str) {
                              return utf8::lengthValidated(str) > 2;
                          }},
                         {"zh_Hant", [](std::string_view str) {
                              return utf8::lengthValidated(str) > 2;
                          }}};
        const auto *filter = findValue(filterMap, lang);
        const auto file = StandardPath::global().open(
            StandardPath::Type::PkgData,
            stringutils::concat("emoji/data/", lang, ".dict"), O_RDONLY);
        EmojiParser parser(filter ? *filter : nullptr);
        if (file.isValid() && parser.load(file.fd())) {
            emojiMap = &(langToEmojiMap_[lang] = std::move(parser.emojiMap_));
            FCITX_INFO() << "Trying to load emoji for " << lang << " from "
                         << file << ": " << emojiMap->size()
                         << " entry(s) loaded.";
        } else {
            if (!fallbackToEn) {
                return nullptr;
            }
            const auto *enMap = loadEmoji("en", false);
            if (enMap) {
                emojiMap = &(langToEmojiMap_[lang] = *enMap);
            }
        }
    }

    return emojiMap;
}

class EmojiModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *) override { return new Emoji; }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::EmojiModuleFactory);
