//
// Copyright (C) 2012~2012 by Yichao Yu
// yyc1992@gmail.com
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_MODULES_SPELL_SPELL_CUSTOM_DICT_H_
#define _FCITX_MODULES_SPELL_SPELL_CUSTOM_DICT_H_

#include <string>
#include <vector>

namespace fcitx {

class SpellCustomDict {
public:
    virtual ~SpellCustomDict() {}

    static SpellCustomDict *requestDict(const std::string &language);
    static bool checkDict(const std::string &language);
    static std::string locateDictFile(const std::string &lang);

    std::vector<std::string> hint(const std::string &str, size_t limit);

protected:
    void loadDict(const std::string &lang);
    int getDistance(const char *word, int utf8Len, const char *dict);
    virtual bool wordCompare(unsigned int c1, unsigned int c2) = 0;
    virtual int wordCheck(const std::string &word) = 0;
    virtual void hintComplete(std::vector<std::string> &hints, int type) = 0;
    std::vector<char> data_;
    std::vector<uint32_t> words_;
    std::string delim_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_SPELL_SPELL_CUSTOM_DICT_H_
