/*
 * SPDX-FileCopyrightText: 2012-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

//
// SPDX-FileCopyrightText: 2007 Daniel Laidig <d.laidig@gmx.de>
// this file is ported from kdelibs/kdeui/kcharselectdata.cpp
//
// original file is licensed under LGPLv2+
//

#include "charselectdata.h"
#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <fmt/format.h>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/endian_p.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"

using namespace fcitx;

/* constants for hangul (de)composition, see UAX #15 */
#define SBase 0xAC00
#define LBase 0x1100
#define VBase 0x1161
#define TBase 0x11A7
#define LCount 19
#define VCount 21
#define TCount 28
#define NCount (VCount * TCount)
#define SCount (LCount * NCount)

static const char JAMO_L_TABLE[][4] = {"G", "GG", "N", "D",  "DD", "R", "M",
                                       "B", "BB", "S", "SS", "",   "J", "JJ",
                                       "C", "K",  "T", "P",  "H"};

static const char JAMO_V_TABLE[][4] = {
    "A",  "AE", "YA", "YAE", "EO", "E",  "YEO", "YE", "O",  "WA", "WAE",
    "OE", "YO", "U",  "WEO", "WE", "WI", "YU",  "EU", "YI", "I"};

static const char JAMO_T_TABLE[][4] = {"",   "G",  "GG", "GS", "N",  "NJ", "NH",
                                       "D",  "L",  "LG", "LM", "LB", "LS", "LT",
                                       "LP", "LH", "M",  "B",  "BS", "S",  "SS",
                                       "NG", "J",  "C",  "K",  "T",  "P",  "H"};

std::string FormatCode(uint32_t code, int length, const char *prefix);

uint32_t FromLittleEndian32(const char *d) {
    const uint8_t *data = (const uint8_t *)d;
    uint32_t t;
    memcpy(&t, data, sizeof(t));
    return le32toh(t);
}

uint16_t FromLittleEndian16(const char *d) {
    const uint8_t *data = (const uint8_t *)d;
    uint16_t t;
    memcpy(&t, data, sizeof(t));
    return le16toh(t);
}

CharSelectData::CharSelectData() {
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "unicode/charselectdata", O_RDONLY);
    if (file.fd() < 0) {
        throw std::runtime_error("Failed to open unicode data");
    }

    struct stat s;
    if (fstat(file.fd(), &s) < 0) {
        throw std::runtime_error("Failed to fstat the unicode data");
    }
    auto size = s.st_size;
    data_.resize(size);
    if (size != fs::safeRead(file.fd(), data_.data(), size)) {
        throw std::runtime_error("Failed to read all data");
    };

    createIndex();
}

std::vector<std::string> CharSelectData::unihanInfo(uint32_t unicode) {
    std::vector<std::string> res;

    const char *data = data_.data();
    const uint32_t offsetBegin = FromLittleEndian32(data + 36);
    const uint32_t offsetEnd = data_.size();

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 32) - 1;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint32_t midUnicode =
            FromLittleEndian16(data + offsetBegin + mid * 32);
        if (unicode > midUnicode) {
            min = mid + 1;
        } else if (unicode < midUnicode) {
            max = mid - 1;
        } else {
            int i;
            for (i = 0; i < 7; i++) {
                uint32_t offset = FromLittleEndian32(data + offsetBegin +
                                                     mid * 32 + 4 + i * 4);
                if (offset != 0) {
                    const char *r = data + offset;
                    res.emplace_back(r);
                } else {
                    res.emplace_back("");
                }
            }
            return res;
        }
    }

    return res;
}

uint32_t CharSelectData::findDetailIndex(uint32_t unicode) const {
    const char *data = data_.data();
    // Convert from little-endian, so that this code works on PPC too.
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=482286
    const uint32_t offsetBegin = FromLittleEndian32(data + 12);
    const uint32_t offsetEnd = FromLittleEndian32(data + 16);

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 29) - 1;

    static uint32_t most_recent_searched;
    static uint32_t most_recent_result;

    if (unicode == most_recent_searched) {
        return most_recent_result;
    }

    most_recent_searched = unicode;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint32_t midUnicode =
            FromLittleEndian16(data + offsetBegin + mid * 29);
        if (unicode > midUnicode) {
            min = mid + 1;
        } else if (unicode < midUnicode) {
            max = mid - 1;
        } else {
            most_recent_result = offsetBegin + mid * 29;

            return most_recent_result;
        }
    }

    most_recent_result = 0;
    return 0;
}

std::string CharSelectData::name(uint32_t unicode) const {
    std::string result;
    do {
        if ((unicode >= 0x3400 && unicode <= 0x4DB5) ||
            (unicode >= 0x4e00 && unicode <= 0x9fa5) ||
            (unicode >= 0x20000 && unicode <= 0x2A6D6)) {
            std::stringstream ss;
            ss << "CJK UNIFIED IDEOGRAPH-%x" << std::hex << unicode;
            result = ss.str();
        } else if (unicode >= 0xac00 && unicode <= 0xd7af) {
            /* compute hangul syllable name as per UAX #15 */
            int SIndex = unicode - SBase;
            int LIndex, VIndex, TIndex;

            if (SIndex < 0 || SIndex >= SCount) {
                break;
            }

            LIndex = SIndex / NCount;
            VIndex = (SIndex % NCount) / TCount;
            TIndex = SIndex % TCount;

            result += "HANGUL SYLLABLE ";
            result += JAMO_L_TABLE[LIndex];
            result += JAMO_V_TABLE[VIndex];
            result += JAMO_T_TABLE[TIndex];
        } else if (unicode >= 0xD800 && unicode <= 0xDB7F) {
            result = _("<Non Private Use High Surrogate>");
        } else if (unicode >= 0xDB80 && unicode <= 0xDBFF) {
            result = _("<Private Use High Surrogate>");
        } else if (unicode >= 0xDC00 && unicode <= 0xDFFF) {
            result = _("<Low Surrogate>");
        } else if (unicode >= 0xE000 && unicode <= 0xF8FF) {
            result = _("<Private Use>");
        } else {

            const char *data = data_.data();
            const uint32_t offsetBegin = FromLittleEndian32(data + 4);
            const uint32_t offsetEnd = FromLittleEndian32(data + 8);

            int min = 0;
            int mid;
            int max = ((offsetEnd - offsetBegin) / 8) - 1;

            while (max >= min) {
                mid = (min + max) / 2;
                const uint32_t midUnicode =
                    FromLittleEndian32(data + offsetBegin + mid * 8);
                if (unicode > midUnicode) {
                    min = mid + 1;
                } else if (unicode < midUnicode) {
                    max = mid - 1;
                } else {
                    uint32_t offset =
                        FromLittleEndian32(data + offsetBegin + mid * 8 + 4);
                    result = (data_.data() + offset + 1);
                    break;
                }
            }
        }
    } while (0);

    if (result.empty()) {
        result = _("<not assigned>");
    }
    return result;
}

std::string Simplified(const std::string &src) {
    std::string result = src;
    auto s = result.begin();
    auto p = s;
    int lastIsSpace = 0;
    while (s != result.end()) {
        char c = *s;

        if (charutils::isspace(c)) {
            if (!lastIsSpace) {
                *p = ' ';
                p++;
            }
            lastIsSpace = 1;
        } else {
            *p = c;
            p++;
            lastIsSpace = 0;
        }
        s++;
    }
    result.erase(p, result.end());
    return result;
}

bool IsHexString(const std::string &s) {
    if (s.size() < 6) {
        return false;
    }
    if (!((s[0] == '0' && s[1] == 'x') || (s[0] == '0' && s[1] == 'X') ||
          (s[0] == 'u' && s[1] == '+') || (s[0] == 'U' && s[1] == '+'))) {
        return false;
    }

    auto i = s.begin() + 2;
    while (i != s.end()) {
        if (!isxdigit(*i)) {
            return 0;
        }
        i++;
    }
    return 1;
}

std::vector<uint32_t> CharSelectData::find(const std::string &needle) const {
    std::set<uint32_t> result;
    std::vector<uint32_t> returnRes;

    auto simplified = Simplified(needle);
    auto searchStrings = stringutils::split(simplified, FCITX_WHITESPACE);

    if (simplified.size() == 1) {
        auto format = FormatCode(simplified[0], 4, "U+");
        searchStrings.clear();
        searchStrings.push_back(format);
    }

    if (searchStrings.empty()) {
        return returnRes;
    }

    for (auto &s : searchStrings) {
        auto convertAndPush = [&returnRes](const std::string &str, int base) {
            try {
                size_t end;
                uint32_t uni = (uint32_t)std::stoul(str, &end, base);
                if (end == str.size()) {
                    returnRes.push_back(uni);
                }
            } catch (const std::exception &) {
            }
        };
        if (IsHexString(s)) {
            auto sub = s.substr(2);
            convertAndPush(sub, 16);
            convertAndPush(sub, 10);
        } else {
            convertAndPush(s, 10);
        }
    }

    for (auto &s : searchStrings) {
        auto partResult = matchingChars(s);
        if (result.empty()) {
            result = std::move(partResult);
        } else {
            auto iter = result.begin();
            while (iter != result.end()) {
                if (partResult.count(*iter)) {
                    iter++;
                } else {
                    iter = result.erase(iter);
                }
            }
        }
        if (result.empty()) {
            break;
        }
    }

    // remove results found by matching the code point to prevent duplicate
    // results
    // while letting these characters stay at the beginning
    for (auto c : returnRes) {
        result.erase(c);
    }

    returnRes.reserve(returnRes.size() + result.size());
    std::copy(result.begin(), result.end(), std::back_inserter(returnRes));
    return returnRes;
}

std::set<uint32_t> CharSelectData::matchingChars(const std::string &s) const {
    std::set<uint32_t> result;
    auto iter = std::lower_bound(
        indexList_.begin(), indexList_.end(), s, [](auto lhs, auto rhs) {
            return strcasecmp(lhs->first.c_str(), rhs.c_str()) < 0;
        });

    while (iter != indexList_.end() &&
           strncasecmp(s.c_str(), (*iter)->first.c_str(), s.size()) == 0) {
        for (auto c : (*iter)->second) {
            result.insert(c);
        }
        iter++;
    }

    return result;
}

std::vector<std::string>
CharSelectData::findStringResult(uint32_t unicode, size_t countOffset,
                                 size_t offsetOfOffset) const {
    std::vector<std::string> result;
    const int detailIndex = findDetailIndex(unicode);
    if (detailIndex == 0) {
        return result;
    }

    const char *data = data_.data();
    const uint8_t count = *(uint8_t *)(data + detailIndex + countOffset);
    uint32_t offset = FromLittleEndian32(data + detailIndex + offsetOfOffset);

    int i;
    for (i = 0; i < count; i++) {
        const char *r = data + offset;
        result.emplace_back(r);
        offset += result.back().size() + 1;
    }
    return result;
}

std::vector<std::string> CharSelectData::aliases(uint32_t unicode) const {
    return findStringResult(unicode, 8, 4);
}

std::vector<std::string> CharSelectData::notes(uint32_t unicode) const {
    return findStringResult(unicode, 13, 9);
}

std::vector<uint32_t> CharSelectData::seeAlso(uint32_t unicode) const {
    std::vector<uint32_t> seeAlso;
    const int detailIndex = findDetailIndex(unicode);
    if (detailIndex == 0) {
        return seeAlso;
    }

    const char *data = data_.data();
    const uint8_t count = *(uint8_t *)(data + detailIndex + 28);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 24);

    int i;
    for (i = 0; i < count; i++) {
        uint32_t c = FromLittleEndian16(data + offset);
        seeAlso.push_back(c);
        offset += 2;
    }

    return seeAlso;
}

std::vector<std::string> CharSelectData::equivalents(uint32_t unicode) const {
    return findStringResult(unicode, 23, 19);
}

std::vector<std::string>
CharSelectData::approximateEquivalents(uint32_t unicode) const {
    return findStringResult(unicode, 18, 14);
}

std::string FormatCode(uint32_t code, int length, const char *prefix) {
    return fmt::format("{0}{1:0{2}x}", prefix, code, length);
}

void CharSelectData::appendToIndex(uint32_t unicode, const std::string &str) {
    auto strings = stringutils::split(str, FCITX_WHITESPACE);
    for (auto &s : strings) {
        auto iter = index_.find(s);
        if (iter == index_.end()) {
            iter = index_.emplace(s, std::vector<uint32_t>()).first;
        }
        iter->second.push_back(unicode);
    }
}

void CharSelectData::createIndex() {
    // character names
    const char *data = data_.data();
    const uint32_t nameOffsetBegin = FromLittleEndian32(data + 4);
    const uint32_t nameOffsetEnd = FromLittleEndian32(data + 8);

    int max = ((nameOffsetEnd - nameOffsetBegin) / 8) - 1;

    int pos, j;

    for (pos = 0; pos <= max; pos++) {
        const uint32_t unicode =
            FromLittleEndian32(data + nameOffsetBegin + pos * 8);
        uint32_t offset =
            FromLittleEndian32(data + nameOffsetBegin + pos * 8 + 4);
        // TODO
        appendToIndex(unicode, (data + offset + 1));
    }

    // details
    const uint32_t detailsOffsetBegin = FromLittleEndian32(data + 12);
    const uint32_t detailsOffsetEnd = FromLittleEndian32(data + 16);

    max = ((detailsOffsetEnd - detailsOffsetBegin) / 29) - 1;
    for (pos = 0; pos <= max; pos++) {
        const uint32_t unicode =
            FromLittleEndian32(data + detailsOffsetBegin + pos * 29);

        // aliases
        const uint8_t aliasCount =
            *(uint8_t *)(data + detailsOffsetBegin + pos * 29 + 8);
        uint32_t aliasOffset =
            FromLittleEndian32(data + detailsOffsetBegin + pos * 29 + 4);

        for (j = 0; j < aliasCount; j++) {
            appendToIndex(unicode, data + aliasOffset);
            aliasOffset += std::strlen(data + aliasOffset) + 1;
        }

        // notes
        const uint8_t notesCount =
            *(uint8_t *)(data + detailsOffsetBegin + pos * 29 + 13);
        uint32_t notesOffset =
            FromLittleEndian32(data + detailsOffsetBegin + pos * 29 + 9);

        for (j = 0; j < notesCount; j++) {
            appendToIndex(unicode, data + notesOffset);
            notesOffset += std::strlen(data + notesOffset) + 1;
        }

        // approximate equivalents
        const uint8_t apprCount =
            *(uint8_t *)(data + detailsOffsetBegin + pos * 29 + 18);
        uint32_t apprOffset =
            FromLittleEndian32(data + detailsOffsetBegin + pos * 29 + 14);

        for (j = 0; j < apprCount; j++) {
            appendToIndex(unicode, data + apprOffset);
            apprOffset += strlen(data + apprOffset) + 1;
        }

        // equivalents
        const uint8_t equivCount =
            *(uint8_t *)(data + detailsOffsetBegin + pos * 29 + 23);
        uint32_t equivOffset =
            FromLittleEndian32(data + detailsOffsetBegin + pos * 29 + 19);

        for (j = 0; j < equivCount; j++) {
            appendToIndex(unicode, data + equivOffset);
            equivOffset += strlen(data + equivOffset) + 1;
        }

        // see also - convert to string (hex)
        const uint8_t seeAlsoCount =
            *(uint8_t *)(data + detailsOffsetBegin + pos * 29 + 28);
        uint32_t seeAlsoOffset =
            FromLittleEndian32(data + detailsOffsetBegin + pos * 29 + 24);

        for (j = 0; j < seeAlsoCount; j++) {
            uint32_t seeAlso = FromLittleEndian16(data + seeAlsoOffset);
            auto code = FormatCode(seeAlso, 4, "");
            appendToIndex(unicode, code);
            equivOffset += strlen(data + equivOffset) + 1;
        }
    }

    // unihan data
    // temporary disabled due to the huge amount of data
    const uint32_t unihanOffsetBegin = FromLittleEndian32(data + 36);
    const uint32_t unihanOffsetEnd = data_.size();
    max = ((unihanOffsetEnd - unihanOffsetBegin) / 32) - 1;

    for (pos = 0; pos <= max; pos++) {
        const uint32_t unicode =
            FromLittleEndian32(data + unihanOffsetBegin + pos * 32);
        for (j = 0; j < 7; j++) {
            uint32_t offset = FromLittleEndian32(data + unihanOffsetBegin +
                                                 pos * 32 + 4 + j * 4);
            if (offset != 0) {
                appendToIndex(unicode, (data + offset));
            }
        }
    }

    for (auto &p : index_) {
        indexList_.push_back(&p);
    }
    std::sort(indexList_.begin(), indexList_.end(), [](auto lhs, auto rhs) {
        return strcasecmp(lhs->first.c_str(), rhs->first.c_str()) < 0;
    });
}
