/*
 * Copyright (C) 2012~2012 by Yichao Yu
 * yyc1992@gmail.com
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "spell-custom-dict.h"
#include "fcitx-utils/cutf8.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpath.h"
#include <fcntl.h>
#include <sys/stat.h>
#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif

#define case_a_z                                                               \
    case 'a':                                                                  \
    case 'b':                                                                  \
    case 'c':                                                                  \
    case 'd':                                                                  \
    case 'e':                                                                  \
    case 'f':                                                                  \
    case 'g':                                                                  \
    case 'h':                                                                  \
    case 'i':                                                                  \
    case 'j':                                                                  \
    case 'k':                                                                  \
    case 'l':                                                                  \
    case 'm':                                                                  \
    case 'n':                                                                  \
    case 'o':                                                                  \
    case 'p':                                                                  \
    case 'q':                                                                  \
    case 'r':                                                                  \
    case 's':                                                                  \
    case 't':                                                                  \
    case 'u':                                                                  \
    case 'v':                                                                  \
    case 'w':                                                                  \
    case 'x':                                                                  \
    case 'y':                                                                  \
    case 'z'

#define case_A_Z                                                               \
    case 'A':                                                                  \
    case 'B':                                                                  \
    case 'C':                                                                  \
    case 'D':                                                                  \
    case 'E':                                                                  \
    case 'F':                                                                  \
    case 'G':                                                                  \
    case 'H':                                                                  \
    case 'I':                                                                  \
    case 'J':                                                                  \
    case 'K':                                                                  \
    case 'L':                                                                  \
    case 'M':                                                                  \
    case 'N':                                                                  \
    case 'O':                                                                  \
    case 'P':                                                                  \
    case 'Q':                                                                  \
    case 'R':                                                                  \
    case 'S':                                                                  \
    case 'T':                                                                  \
    case 'U':                                                                  \
    case 'V':                                                                  \
    case 'W':                                                                  \
    case 'X':                                                                  \
    case 'Y':                                                                  \
    case 'Z'

#define DICT_BIN_MAGIC "FSCD0000"

bool checkLang(const std::string &full_lang, const std::string &lang) {
    if (full_lang.empty() || lang.empty())
        return false;
    if (full_lang.compare(0, lang.size(), lang) != 0)
        return false;
    switch (full_lang[lang.size()]) {
    case '\0':
    case '_':
        return true;
    default:
        break;
    }
    return false;
}

static inline uint32_t load_le32(const void *p) {
    return le32toh(*(uint32_t *)p);
}

static bool isFirstCapital(const std::string &str) {
    if (str.empty())
        return false;
    auto iter = str.begin();
    switch (*iter) {
    case_A_Z:
        break;
    default:
        return false;
    }
    for (; iter != str.end(); iter++) {
        switch (*iter) {
        case_A_Z:
            return false;
        default:
            continue;
        }
    }
    return true;
}

static bool isAllCapital(const std::string &str) {
    if (str.empty())
        return false;
    for (auto iter = str.begin(); iter != str.end(); iter++) {
        switch (*iter) {
        case_a_z:
            return false;
        default:
            continue;
        }
    }
    return true;
}

enum {
    CUSTOM_DEFAULT,
    CUSTOM_FIRST_CAPITAL,
    CUSTOM_ALL_CAPITAL,
};

static void toUpperString(std::string &str) {
    if (str.empty())
        return;
    for (auto iter = str.begin(); iter != str.end(); iter++) {
        switch (*iter) {
        case_a_z:
            *iter += 'A' - 'a';
            break;
        default:
            break;
        }
    }
}

namespace fcitx {

class SpellCustomDictEn : public SpellCustomDict {
public:
    SpellCustomDictEn() {
        delim_ = " _-,./?!%";
        loadDict("en");
    }

    bool wordCompare(unsigned int c1, unsigned int c2) override {
        switch (c1) {
        case_A_Z:
            c1 += 'a' - 'A';
            break;
        case_a_z:
            break;
        default:
            return c1 == c2;
        }
        switch (c2) {
        case_A_Z:
            c2 += 'a' - 'A';
            break;
        case_a_z:
            break;
        default:
            break;
        }
        return c1 == c2;
    }
    int wordCheck(const std::string &str) override {
        if (isFirstCapital(str))
            return CUSTOM_FIRST_CAPITAL;
        if (isAllCapital(str))
            return CUSTOM_ALL_CAPITAL;
        return CUSTOM_DEFAULT;
    }

    void hintComplete(std::vector<std::string> &hints, int type) override {
        switch (type) {
        case CUSTOM_ALL_CAPITAL:
            for (auto &hint : hints) {
                toUpperString(hint);
            }
            break;
        case CUSTOM_FIRST_CAPITAL:
            for (auto &hint : hints) {
                if (hint.empty()) {
                    continue;
                }
                switch (hint[0]) {
                case_a_z:
                    hint[0] += 'A' - 'a';
                    break;
                default:
                    break;
                }
            }
        default:
            break;
        }
    }
};

#if 0
static inline uint16_t
load_le16(const void* p)
{
    return le16toh(*(uint16_t*)p);
}
#endif

/**
 * Open the dict file, return -1 if failed.
 **/
std::string SpellCustomDict::locateDictFile(const std::string &lang) {
    auto templatePath = "spell/" + lang + "_dict.fscd";
    auto &standardPath = StandardPath::global();
    std::string path;
    standardPath.scanDirectories(
        StandardPath::Type::PkgData,
        [&lang, &path, &templatePath](const std::string &dirPath, bool isUser) {
            if (isUser) {
                return true;
            }
            auto fullPath = stringutils::joinPath(dirPath, templatePath);
            if (fs::isreg(fullPath)) {
                path = fullPath;
                return false;
            }
            return true;
        });
    return path;
}

void SpellCustomDict::loadDict(const std::string &lang) {
    auto file = locateDictFile(lang);
    auto fd = UnixFD::own(open(file.c_str(), O_RDONLY));

    if (!fd.isValid()) {
        throw std::runtime_error("failed to open dict file");
    }

    do {
        struct stat stat_buf;
        size_t total_len;
        char magic_buff[sizeof(DICT_BIN_MAGIC) - 1];
        if (fstat(fd.fd(), &stat_buf) == -1 ||
            static_cast<size_t>(stat_buf.st_size) <=
                sizeof(uint32_t) + sizeof(magic_buff)) {
            break;
        }
        if (fs::safeRead(fd.fd(), magic_buff, sizeof(magic_buff)) !=
            sizeof(magic_buff)) {
            break;
        }
        if (memcmp(DICT_BIN_MAGIC, magic_buff, sizeof(magic_buff))) {
            break;
        }
        total_len = stat_buf.st_size - sizeof(magic_buff);
        data_.resize(total_len + 1);
        if (fs::safeRead(fd.fd(), data_.data(), total_len) !=
            static_cast<ssize_t>(total_len)) {
            break;
        }
        data_[total_len] = '\0';

        auto lcount = load_le32(data_.data());
        words_.resize(lcount);

        /* save words offset's. */
        size_t i, j;
        for (i = sizeof(uint32_t), j = 0; i < total_len && j < lcount; i += 1) {
            i += sizeof(uint16_t);
            int l = strlen(data_.data() + i);
            if (!l) {
                continue;
            }
            words_[j++] = i;
            i += l;
        }
        if (j < lcount || i < total_len) {
            break;
        }
        return;
    } while (0);

    throw std::runtime_error("failed to read dict file");
}

SpellCustomDict *SpellCustomDict::requestDict(const std::string &lang) {
    if (checkLang(lang, "en")) {
        return new SpellCustomDictEn;
    }
    return nullptr;
}

bool SpellCustomDict::checkDict(const std::string &lang) {
    return !locateDictFile(lang).empty();
}

int SpellCustomDict::getDistance(const char *word, int utf8Len,
                                 const char *dict) {
#define REPLACE_WEIGHT 3
#define INSERT_WEIGHT 3
#define REMOVE_WEIGHT 3
#define END_WEIGHT 1
    /*
     * three kinds of error, replace, insert and remove
     * replace means apple vs aplle
     * insert means apple vs applee
     * remove means apple vs aple
     *
     * each error need to follow a correct match.
     *
     * number of "remove error" shoud be no more than "maxremove"
     * while maxremove equals to (length - 2) / 3
     *
     * and the total error number should be no more than "maxdiff"
     * while maxdiff equales to length / 3.
     */
    int replace = 0;
    int insert = 0;
    int remove = 0;
    int diff = 0;
    int maxdiff;
    int maxremove;
    unsigned int cur_word_c;
    unsigned int cur_dict_c;
    unsigned int next_word_c;
    unsigned int next_dict_c;
    maxdiff = utf8Len / 3;
    maxremove = (utf8Len - 2) / 3;
    word = fcitx_utf8_get_char(word, &cur_word_c);
    dict = fcitx_utf8_get_char(dict, &cur_dict_c);
    while ((diff = replace + insert + remove) <= maxdiff &&
           remove <= maxremove) {
        /*
         * cur_word_c and cur_dict_c are the current characters
         * and dict and word are pointing to the next one.
         */
        if (!cur_word_c) {
            return (
                (replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT +
                 remove * REMOVE_WEIGHT) +
                (cur_dict_c ? (fcitx_utf8_strlen(dict) + 1) * END_WEIGHT : 0));
        }
        word = fcitx_utf8_get_char(word, &next_word_c);

        /* check remove error */
        if (!cur_dict_c) {
            if (next_word_c)
                return -1;
            remove++;
            if (diff <= maxdiff && remove <= maxremove) {
                return (replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT +
                        remove * REMOVE_WEIGHT);
            }
            return -1;
        }
        dict = fcitx_utf8_get_char(dict, &next_dict_c);
        if (cur_word_c == cur_dict_c || (wordCompare(cur_word_c, cur_dict_c))) {
            cur_word_c = next_word_c;
            cur_dict_c = next_dict_c;
            continue;
        }
        if (next_word_c == cur_dict_c ||
            (next_word_c && wordCompare(next_word_c, cur_dict_c))) {
            word = fcitx_utf8_get_char(word, &cur_word_c);
            cur_dict_c = next_dict_c;
            remove++;
            continue;
        }

        /* check insert error */
        if (cur_word_c == next_dict_c ||
            (next_dict_c && wordCompare(cur_word_c, next_dict_c))) {
            cur_word_c = next_word_c;
            dict = fcitx_utf8_get_char(dict, &cur_dict_c);
            insert++;
            continue;
        }

        /* check replace error */
        if (next_word_c == next_dict_c ||
            (next_word_c && next_dict_c &&
             wordCompare(next_word_c, next_dict_c))) {
            if (next_word_c) {
                dict = fcitx_utf8_get_char(dict, &cur_dict_c);
                word = fcitx_utf8_get_char(word, &cur_word_c);
            } else {
                cur_word_c = 0;
                cur_dict_c = 0;
            }
            replace++;
            continue;
        }
        break;
    }
    return -1;
}

std::vector<std::string> SpellCustomDict::hint(const std::string &str,
                                               size_t limit) {
    const char *word = str.c_str();
    const char *real_word = word;
    std::vector<std::string> result;
    std::vector<std::pair<const char *, int>> tops;
    if (!delim_.empty()) {
        size_t delta;
        while (real_word[delta = strcspn(real_word, delim_.c_str())]) {
            real_word += delta + 1;
        }
    }
    if (!real_word[0])
        return {};
    auto word_type = wordCheck(real_word);
    int word_len = fcitx_utf8_strlen(real_word);
    auto compare = [](const std::pair<const char *, int> &lhs,
                      const std::pair<const char *, int> &rhs) {
        return lhs.second < rhs.second;
    };
    for (const auto &wordOffset : words_) {
        int dist;
        const char *dictWord = data_.data() + wordOffset;
        if ((dist = getDistance(real_word, word_len, dictWord)) >= 0) {
            tops.emplace_back(dictWord, dist);
            std::push_heap(tops.begin(), tops.end(), compare);
            if (tops.size() > limit) {
                std::pop_heap(tops.begin(), tops.end(), compare);
                tops.pop_back();
            }
        }
    }

    // Or sort heap?..
    std::sort(tops.begin(), tops.end(), compare);

    for (auto &top : tops) {
        result.emplace_back(top.first);
    }
    hintComplete(result, word_type);
    return result;
}
}
