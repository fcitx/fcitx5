/*
 * SPDX-FileCopyrightText: 2012-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_UNICODE_CHARSELECTDATA_H_
#define _FCITX_MODULES_UNICODE_CHARSELECTDATA_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <span>
#include "fcitx-utils/mappedbuffer.h"
#include "littleendianserialization.h"

namespace fcitx::unicode {

class CharSelectData {
public:
    CharSelectData();

    std::string name(uint32_t unicode) const;
    std::vector<std::string> unihanInfo(uint32_t unicode) const;
    std::vector<uint32_t> find(const std::string &needle) const;

    bool load();

private:
    struct CharselectDataHeader {
        unicode::LittleEndian<uint32_t> namesStringsBegin;
        unicode::LittleEndian<uint32_t> namesOffsetsBegin;
        unicode::LittleEndian<uint32_t> detailsStringsBegin;
        unicode::LittleEndian<uint32_t> detailsOffsetsBegin;
        unicode::LittleEndian<uint32_t> blocksStringsBegin;
        unicode::LittleEndian<uint32_t> blocksOffsetsBegin;
        unicode::LittleEndian<uint32_t> sectionsStringsBegin;
        unicode::LittleEndian<uint32_t> sectionsOffsetsBegin;
        unicode::LittleEndian<uint32_t> unihanStringsBegin;
        unicode::LittleEndian<uint32_t> unihanOffsetsBegin;

        std::array<uint32_t, 10> offsets() const noexcept {
            return {
                namesStringsBegin.value(),    namesOffsetsBegin.value(),
                detailsStringsBegin.value(),  detailsOffsetsBegin.value(),
                blocksStringsBegin.value(),   blocksOffsetsBegin.value(),
                sectionsStringsBegin.value(), sectionsOffsetsBegin.value(),
                unihanStringsBegin.value(),   unihanOffsetsBegin.value(),
            };
        }
    };

    struct CharselectDataName {
        unicode::LittleEndian<uint32_t> unicode;
        unicode::LittleEndian<uint32_t> offset;
    };

    struct CharselectDataDetail {
        unicode::LittleEndian<uint32_t> unicode;
        unicode::LittleEndian<uint32_t> aliasesOffset;
        unicode::LittleEndian<uint8_t> aliasesCount;
        unicode::LittleEndian<uint32_t> notesOffset;
        unicode::LittleEndian<uint8_t> notesCount;
        unicode::LittleEndian<uint32_t> approximateEquivalentsOffset;
        unicode::LittleEndian<uint8_t> approximateEquivalentsCount;
        unicode::LittleEndian<uint32_t> equivalentsOffset;
        unicode::LittleEndian<uint8_t> equivalentsCount;
        unicode::LittleEndian<uint32_t> seeAlsoOffset;
        unicode::LittleEndian<uint8_t> seeAlsoCount;
    };

    struct CharselectDataUnihan {
        unicode::LittleEndian<uint32_t> unicode;
        unicode::LittleEndian<uint32_t> definitionOffset;
        unicode::LittleEndian<uint32_t> cantoneseOffset;
        unicode::LittleEndian<uint32_t> mandarinOffset;
        unicode::LittleEndian<uint32_t> tangOffset;
        unicode::LittleEndian<uint32_t> koreanOffset;
        unicode::LittleEndian<uint32_t> japaneseKunOffset;
        unicode::LittleEndian<uint32_t> japaneseOnOffset;

        std::array<uint32_t, 7> offsets() const noexcept {
            return {
                definitionOffset.value(), cantoneseOffset.value(),
                mandarinOffset.value(),   tangOffset.value(),
                koreanOffset.value(),     japaneseKunOffset.value(),
                japaneseOnOffset.value(),
            };
        }
    };

    static_assert(sizeof(CharselectDataHeader) == 40);
    static_assert(sizeof(CharselectDataName) == 8);
    static_assert(sizeof(CharselectDataDetail) == 29);
    static_assert(sizeof(CharselectDataUnihan) == 32);
    static_assert(alignof(CharselectDataHeader) == 1);
    static_assert(alignof(CharselectDataName) == 1);
    static_assert(alignof(CharselectDataDetail) == 1);
    static_assert(alignof(CharselectDataUnihan) == 1);

    bool createIndex();
    void appendToIndex(uint32_t unicode, std::string_view str);
    std::optional<std::string_view> readString(size_t offset, size_t end) const;
    std::vector<uint32_t> seeAlso(uint32_t unicode) const;

    std::set<uint32_t> matchingChars(std::string_view s) const;

    bool loaded_ = false;
    std::optional<MappedBuffer> data_;
    const CharselectDataHeader *header_ = nullptr;
    std::span<const CharselectDataName> leNames_;
    std::span<const CharselectDataDetail> leDetails_;
    std::span<const CharselectDataUnihan> leUnihan_;
    std::unordered_map<std::string_view, std::vector<uint32_t>> index_;
    std::vector<const decltype(index_)::value_type *> indexList_;
};

} // namespace fcitx::unicode

#endif // _FCITX_MODULES_UNICODE_CHARSELECTDATA_H_
