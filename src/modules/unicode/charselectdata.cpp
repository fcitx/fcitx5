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
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <format>
#include <span>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "littleendianserialization.h"

namespace fcitx::unicode {

namespace {

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

const char JAMO_L_TABLE[][4] = {
    "G",  "GG", "N", "D",  "DD", "R", "M", "B", "BB", "S",
    "SS", "",   "J", "JJ", "C",  "K", "T", "P", "H",
};

const char JAMO_V_TABLE[][4] = {
    "A",  "AE", "YA", "YAE", "EO", "E",  "YEO", "YE", "O",  "WA", "WAE",
    "OE", "YO", "U",  "WEO", "WE", "WI", "YU",  "EU", "YI", "I",
};

const char JAMO_T_TABLE[][4] = {
    "",   "G",  "GG", "GS", "N",  "NJ", "NH", "D", "L",  "LG",
    "LM", "LB", "LS", "LT", "LP", "LH", "M",  "B", "BS", "S",
    "SS", "NG", "J",  "C",  "K",  "T",  "P",  "H",
};

std::string FormatCode(uint32_t code, int length, const char *prefix) {
    return std::format("{0}{1:0{2}x}", prefix, code, length);
}

int viewCaseCompare(std::string_view s1, std::string_view s2) noexcept {
    const size_t minSize = std::min(s1.size(), s2.size());
    for (size_t i = 0; i < minSize; ++i) {
        const auto c1 = static_cast<unsigned char>(charutils::tolower(s1[i]));
        const auto c2 = static_cast<unsigned char>(charutils::tolower(s2[i]));
        if (c1 != c2) {
            return static_cast<int>(c1) - static_cast<int>(c2);
        }
    }
    if (s1.size() != s2.size()) {
        return s1.size() < s2.size() ? -1 : 1;
    }
    return 0;
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
    if (!(s.starts_with("0x") || s.starts_with("0X") || s.starts_with("u+") ||
          s.starts_with("U+"))) {
        return false;
    }

    auto i = s.begin() + 2;
    while (i != s.end()) {
        if (!charutils::isxdigit(*i)) {
            return false;
        }
        i++;
    }
    return true;
}

} // namespace

CharSelectData::CharSelectData() = default;

bool CharSelectData::load() {
    if (loaded_) {
        return data_.has_value();
    }
    loaded_ = true;
    auto file = StandardPaths::global().open(StandardPathsType::PkgData,
                                             "unicode/charselectdata");
    if (!file.isValid()) {
        return false;
    }

    try {
        data_.emplace(file);
    } catch (const std::exception &) {
        return false;
    }
    if (!createIndex()) {
        data_.reset();
        return false;
    }
    return true;
}

std::optional<std::string_view> CharSelectData::readString(size_t offset,
                                                           size_t end) const {
    if (offset >= end || end > data_->size()) {
        return std::nullopt;
    }
    const auto *begin = reinterpret_cast<const char *>(data_->data() + offset);
    const size_t maxLen = end - offset;
    const auto *nul =
        static_cast<const char *>(std::memchr(begin, '\0', maxLen));
    if (!nul) {
        return std::nullopt;
    }
    return std::string_view(begin, static_cast<size_t>(nul - begin));
}

std::vector<std::string> CharSelectData::unihanInfo(uint32_t unicode) const {
    if (!data_) {
        return {};
    }

    std::vector<std::string> res;

    const auto iter = std::ranges::lower_bound(
        leUnihan_, unicode, {}, [](const CharselectDataUnihan &entry) {
            return entry.unicode.value();
        });
    if (iter == leUnihan_.end() || iter->unicode.value() != unicode) {
        return res;
    }

    res.reserve(7);
    const uint32_t unihanStringsBegin = header_->unihanStringsBegin.value();
    const uint32_t unihanOffsetsBegin = header_->unihanOffsetsBegin.value();
    for (const uint32_t offset : iter->offsets()) {
        if (offset >= unihanStringsBegin && offset < unihanOffsetsBegin) {
            if (auto str = readString(offset, unihanOffsetsBegin)) {
                res.emplace_back(*str);
                continue;
            }
        }
        res.emplace_back();
    }
    return res;
}

std::string CharSelectData::name(uint32_t unicode) const {
    if (!data_) {
        return {};
    }

    std::string result;
    do {
        // Need to match UnicodeData.txt
        if ((unicode >= 0x3400 && unicode <= 0x4DBF) ||
            (unicode >= 0x4E00 && unicode <= 0x9FFF) ||
            (unicode >= 0x20000 && unicode <= 0x2A6DF) ||
            (unicode >= 0x2A700 && unicode <= 0x2B739) ||
            (unicode >= 0x2B740 && unicode <= 0x2B81D) ||
            (unicode >= 0x2B820 && unicode <= 0x2CEA1) ||
            (unicode >= 0x2CEB0 && unicode <= 0x2EBE0) ||
            (unicode >= 0x2EBF0 && unicode <= 0x2EE5D) ||
            (unicode >= 0x30000 && unicode <= 0x3134A) ||
            (unicode >= 0x31350 && unicode <= 0x323AF)) {
            result = _("CJK UNIFIED IDEOGRAPH-{0:X}", unicode);
        } else if (unicode >= 0xac00 && unicode <= 0xd7af) {
            /* compute hangul syllable name as per UAX #15 */
            int SIndex = unicode - SBase;
            int LIndex;
            int VIndex;
            int TIndex;

            if (SIndex < 0 || SIndex >= SCount) {
                break;
            }

            LIndex = SIndex / NCount;
            VIndex = (SIndex % NCount) / TCount;
            TIndex = SIndex % TCount;

            result = _("HANGUL SYLLABLE {0}{1}{2}", JAMO_L_TABLE[LIndex],
                       JAMO_V_TABLE[VIndex], JAMO_T_TABLE[TIndex]);
        } else if (unicode >= 0xD800 && unicode <= 0xDB7F) {
            result = _("<Non Private Use High Surrogate>");
        } else if (unicode >= 0xDB80 && unicode <= 0xDBFF) {
            result = _("<Private Use High Surrogate>");
        } else if (unicode >= 0xDC00 && unicode <= 0xDFFF) {
            result = _("<Low Surrogate>");
        } else if (unicode >= 0xE000 && unicode <= 0xF8FF) {
            result = _("<Private Use>");
        } else if ((unicode >= 0x17000 && unicode <= 0x187F7) ||
                   (unicode >= 0x18D00 && unicode <= 0x18D08)) {
            result = _("TANGUT IDEOGRAPH-{0:X}", unicode);
        } else if (unicode >= 0xF0000 && unicode <= 0xFFFFD) {
            result = _("<Plane 15 Private Use>");
        } else if (unicode >= 0x100000 && unicode <= 0x10FFFD) {
            result = _("<Plane 16 Private Use>");
        } else {
            const auto iter = std::ranges::lower_bound(
                leNames_, unicode, {}, [](const CharselectDataName &entry) {
                    return entry.unicode.value();
                });
            if (iter != leNames_.end() && iter->unicode.value() == unicode) {
                const uint32_t offset = iter->offset.value();
                if (offset >= header_->namesStringsBegin.value() &&
                    offset < header_->namesOffsetsBegin.value()) {
                    if (auto str = readString(
                            offset + 1, header_->namesOffsetsBegin.value())) {
                        result = *str;
                    }
                }
            }
        }
    } while (0);

    if (result.empty()) {
        result = _("<not assigned>");
    }
    return result;
}

std::vector<uint32_t> CharSelectData::find(const std::string &needle) const {
    if (!data_) {
        return {};
    }

    std::set<uint32_t> result;
    std::vector<uint32_t> returnRes;

    auto simplified = Simplified(needle);
    auto searchStrings = stringutils::split(simplified, FCITX_WHITESPACE);

    if (simplified.size() == 1) {
        auto format =
            FormatCode(static_cast<unsigned char>(simplified[0]), 4, "U+");
        searchStrings.clear();
        searchStrings.push_back(format);
    }

    if (searchStrings.empty()) {
        return returnRes;
    }

    auto appendUnique = [](std::vector<uint32_t> &vec, uint32_t value) {
        if (std::ranges::find(vec, value) == vec.end()) {
            vec.push_back(value);
        }
    };

    for (auto &s : searchStrings) {
        auto convertAndPush = [&returnRes, &appendUnique](
                                  const std::string &str, int base) {
            try {
                size_t end;
                auto uni = static_cast<uint32_t>(std::stoul(str, &end, base));
                if (end == str.size()) {
                    appendUnique(returnRes, uni);
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

    const size_t directCount = returnRes.size();
    for (size_t i = 0; i < directCount; ++i) {
        for (const auto seeAlsoCode : seeAlso(returnRes[i])) {
            appendUnique(returnRes, seeAlsoCode);
        }
    }

    for (auto &s : searchStrings) {
        auto partResult = matchingChars(s);
        if (result.empty()) {
            result = std::move(partResult);
        } else {
            auto iter = result.begin();
            while (iter != result.end()) {
                if (partResult.contains(*iter)) {
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
    std::ranges::copy(result, std::back_inserter(returnRes));
    return returnRes;
}

std::set<uint32_t> CharSelectData::matchingChars(std::string_view s) const {
    std::set<uint32_t> result;
    auto iter = std::ranges::lower_bound(
        indexList_, s,
        [](const auto &lhs, const auto &rhs) {
            return viewCaseCompare(lhs, rhs) < 0;
        },
        [](const auto &value) { return value->first; });

    while (iter != indexList_.end() && (*iter)->first.size() >= s.size() &&
           viewCaseCompare(s, (*iter)->first.substr(0, s.size())) == 0) {
        for (auto c : (*iter)->second) {
            result.insert(c);
        }
        iter++;
    }

    return result;
}

std::vector<uint32_t> CharSelectData::seeAlso(uint32_t unicode) const {
    std::vector<uint32_t> result;
    const auto iter = std::ranges::lower_bound(
        leDetails_, unicode, {}, [](const CharselectDataDetail &detail) {
            return detail.unicode.value();
        });
    if (iter == leDetails_.end() || iter->unicode.value() != unicode) {
        return result;
    }

    const uint32_t offset = iter->seeAlsoOffset.value();
    const uint8_t count = iter->seeAlsoCount.value();
    const uint32_t stringsBegin = header_->detailsStringsBegin.value();
    const uint32_t offsetsBegin = header_->detailsOffsetsBegin.value();
    if (count == 0 || offset < stringsBegin ||
        static_cast<size_t>(offset) +
                (static_cast<size_t>(count) * sizeof(LittleEndian<uint32_t>)) >
            offsetsBegin) {
        return result;
    }

    const std::span leSeeAlso(reinterpret_cast<const LittleEndian<uint32_t> *>(
                                  data_->data() + offset),
                              count);
    result.reserve(count);
    for (const auto &entry : leSeeAlso) {
        result.push_back(entry.value());
    }
    return result;
}

void CharSelectData::appendToIndex(uint32_t unicode, std::string_view str) {
    for (std::string_view s : stringutils::splitView(str, FCITX_WHITESPACE)) {
        auto iter = index_.find(s);
        if (iter == index_.end()) {
            iter = index_.emplace(s, std::vector<uint32_t>()).first;
        }
        iter->second.push_back(unicode);
    }
}

bool CharSelectData::createIndex() {
    if (data_->size() < sizeof(CharselectDataHeader)) {
        return false;
    }
    const std::byte *data = data_->data();
    const auto *header = reinterpret_cast<const CharselectDataHeader *>(data);
    const auto headerOffsets = header->offsets();
    if (headerOffsets.front() < sizeof(CharselectDataHeader) ||
        headerOffsets.back() > data_->size() ||
        !std::ranges::is_sorted(headerOffsets)) {
        return false;
    }
    header_ = header;

    // character names
    const uint32_t namesStringsBegin = header_->namesStringsBegin.value();
    const uint32_t nameOffsetBegin = header_->namesOffsetsBegin.value();
    const uint32_t nameOffsetEnd = header_->detailsStringsBegin.value();
    leNames_ = std::span(
        reinterpret_cast<const CharselectDataName *>(data + nameOffsetBegin),
        (nameOffsetEnd - nameOffsetBegin) / sizeof(CharselectDataName));

    for (const auto &leName : leNames_) {
        const uint32_t offset = leName.offset.value();
        if (offset >= namesStringsBegin && offset < nameOffsetBegin) {
            if (auto name = readString(offset + 1, nameOffsetBegin)) {
                appendToIndex(leName.unicode.value(), *name);
            }
        }
    }

    // details
    const uint32_t detailsStringsBegin = header_->detailsStringsBegin.value();
    const uint32_t detailsOffsetBegin = header_->detailsOffsetsBegin.value();
    const uint32_t detailsOffsetEnd = header_->blocksStringsBegin.value();

    leDetails_ = std::span(
        reinterpret_cast<const CharselectDataDetail *>(data +
                                                       detailsOffsetBegin),
        (detailsOffsetEnd - detailsOffsetBegin) / sizeof(CharselectDataDetail));
    auto appendDetailStrings =
        [this, detailsStringsBegin,
         detailsOffsetBegin](uint32_t unicode, uint32_t offset, uint8_t count) {
            if (count == 0 || offset < detailsStringsBegin) {
                return;
            }
            size_t cur = offset;
            for (uint8_t i = 0; i < count; i++) {
                auto str = readString(cur, detailsOffsetBegin);
                if (!str) {
                    break;
                }
                appendToIndex(unicode, *str);
                cur += str->size() + 1;
            }
        };
    for (const auto &detail : leDetails_) {
        const uint32_t unicode = detail.unicode.value();
        appendDetailStrings(unicode, detail.aliasesOffset.value(),
                            detail.aliasesCount.value());
        appendDetailStrings(unicode, detail.notesOffset.value(),
                            detail.notesCount.value());
        appendDetailStrings(unicode,
                            detail.approximateEquivalentsOffset.value(),
                            detail.approximateEquivalentsCount.value());
        appendDetailStrings(unicode, detail.equivalentsOffset.value(),
                            detail.equivalentsCount.value());
    }

    // unihan data
    const uint32_t unihanStringsBegin = header_->unihanStringsBegin.value();
    const uint32_t unihanOffsetBegin = header_->unihanOffsetsBegin.value();
    const size_t unihanOffsetEnd = data_->size();
    leUnihan_ = std::span(
        reinterpret_cast<const CharselectDataUnihan *>(data +
                                                       unihanOffsetBegin),
        (unihanOffsetEnd - unihanOffsetBegin) / sizeof(CharselectDataUnihan));

    for (const auto &leUnihan : leUnihan_) {
        const uint32_t unicode = leUnihan.unicode.value();
        for (const uint32_t offset : leUnihan.offsets()) {
            if (offset >= unihanStringsBegin && offset < unihanOffsetBegin) {
                if (auto str = readString(offset, unihanOffsetBegin)) {
                    appendToIndex(unicode, *str);
                }
            }
        }
    }

    indexList_.reserve(index_.size());
    for (auto &p : index_) {
        indexList_.push_back(&p);
    }
    std::ranges::sort(indexList_, [](auto lhs, auto rhs) {
        return viewCaseCompare(lhs->first, rhs->first) < 0;
    });
    return true;
}

} // namespace fcitx::unicode