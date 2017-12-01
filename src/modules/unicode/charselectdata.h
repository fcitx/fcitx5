/*
 * Copyright (C) 2012~2017 by CSSlayer
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
#ifndef _FCITX_MODULES_UNICODE_CHARSELECTDATA_H_
#define _FCITX_MODULES_UNICODE_CHARSELECTDATA_H_

#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class CharSelectData {
public:
    CharSelectData();

    std::vector<std::string> unihanInfo(uint32_t unicode);
    std::string name(uint32_t unicode) const;
    std::vector<uint32_t> find(const std::string &needle) const;

private:
    void createIndex();
    void appendToIndex(uint32_t unicode, const char *str);
    uint32_t findDetailIndex(uint32_t unicode) const;

    std::vector<std::string> findStringResult(uint32_t unicode,
                                              size_t countOffset,
                                              size_t offsetOfOffset) const;

    std::vector<std::string> aliases(uint32_t unicode) const;
    std::vector<std::string> notes(uint32_t unicode) const;
    std::vector<uint32_t> seeAlso(uint32_t unicode) const;
    std::vector<std::string> equivalents(uint32_t unicode) const;
    std::vector<std::string> approximateEquivalents(uint32_t unicode) const;

    std::set<uint32_t> matchingChars(const std::string &s) const;

    std::vector<char> data_;
    std::unordered_map<std::string, std::vector<uint32_t>> index_;
    std::vector<const decltype(index_)::value_type *> indexList_;
};

#endif // _FCITX_MODULES_UNICODE_CHARSELECTDATA_H_/
