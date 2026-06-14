/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef FCITX_AVRO_PARSER_H
#define FCITX_AVRO_PARSER_H

#include <string>
#include <vector>

namespace fcitx {

struct AvroMatchRule {
    std::string type;
    std::string scope;
    std::string value;
    bool negative;
};

struct AvroRule {
    std::vector<AvroMatchRule> matches;
    std::string replace;
};

struct AvroPattern {
    std::string find;
    std::string replace;
    std::vector<AvroRule> rules;
};

class AvroParser {
public:
    AvroParser();
    std::string parse(const std::string &input) const;

private:
    std::vector<AvroPattern> patterns_;
    std::string vowel_;
    std::string consonant_;
    std::string caseSensitive_;

    static bool isExact(const std::string &needle, const std::string &haystack, int start,
                        int end, bool negate);
    bool isVowel(char c) const;
    bool isConsonant(char c) const;
    bool isPunctuation(char c) const;
    bool isCaseSensitive(char c) const;
    std::string fixString(const std::string &input) const;
};

} // namespace fcitx

#endif
