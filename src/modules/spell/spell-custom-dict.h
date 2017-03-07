/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
#ifndef _FCITX_MODULE_SPELL_DICT_H
#define _FCITX_MODULE_SPELL_DICT_H

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
}

#endif /* _FCITX_MODULE_SPELL_DICT_H */
