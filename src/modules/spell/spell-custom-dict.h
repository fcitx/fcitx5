/*
 * SPDX-FileCopyrightText: 2012-2012 Yichao Yu <yyc1992@gmail.com>
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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

    std::vector<std::pair<std::string, std::string>>
    hint(const std::string &str, size_t limit);

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
