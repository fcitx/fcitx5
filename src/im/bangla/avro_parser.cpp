/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "avro_parser.h"

#include <algorithm>
#include <cctype>

namespace fcitx {

namespace {
std::vector<AvroPattern> makePatterns();
std::string makeVowels();
std::string makeConsonants();
std::string makeCaseSensitive();
} // namespace

AvroParser::AvroParser()
    : patterns_(makePatterns()), vowel_(makeVowels()), consonant_(makeConsonants()),
      caseSensitive_(makeCaseSensitive()) {}

bool AvroParser::isExact(const std::string &needle, const std::string &haystack, int start,
                         int end, bool negate) {
    const bool equal = start >= 0 && end <= static_cast<int>(haystack.size()) &&
                       haystack.substr(start, end - start) == needle;
    return equal ^ negate;
}

bool AvroParser::isVowel(char c) const {
    const auto lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return vowel_.find(lower) != std::string::npos;
}

bool AvroParser::isConsonant(char c) const {
    const auto lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return consonant_.find(lower) != std::string::npos;
}

bool AvroParser::isPunctuation(char c) const { return !(isVowel(c) || isConsonant(c)); }

bool AvroParser::isCaseSensitive(char c) const {
    const auto lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return caseSensitive_.find(lower) != std::string::npos;
}

std::string AvroParser::fixString(const std::string &input) const {
    std::string fixed;
    fixed.reserve(input.size());
    for (auto c : input) {
        if (isCaseSensitive(c)) {
            fixed.push_back(c);
        } else {
            fixed.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return fixed;
}

std::string AvroParser::parse(const std::string &input) const {
    const auto fixed = fixString(input);
    std::string output;
    output.reserve(fixed.size() * 2);

    int cur = 0;
    while (cur < static_cast<int>(fixed.size())) {
        const int start = cur;
        bool matched = false;

        for (const auto &pattern : patterns_) {
            const int end = cur + static_cast<int>(pattern.find.size());
            if (end > static_cast<int>(fixed.size()) ||
                fixed.substr(start, pattern.find.size()) != pattern.find) {
                continue;
            }
            const int prev = start - 1;

            if (!pattern.rules.empty()) {
                for (const auto &rule : pattern.rules) {
                    bool replace = true;
                    for (const auto &m : rule.matches) {
                        const int chk = m.type == "suffix" ? end : prev;
                        bool cond = false;
                        if (m.scope == "punctuation") {
                            cond = (chk < 0 && m.type == "prefix") ||
                                   (chk >= static_cast<int>(fixed.size()) && m.type == "suffix") ||
                                   (chk >= 0 && chk < static_cast<int>(fixed.size()) &&
                                    isPunctuation(fixed[chk]));
                        } else if (m.scope == "vowel") {
                            const bool inRange = (chk >= 0 && m.type == "prefix") ||
                                                 (chk < static_cast<int>(fixed.size()) &&
                                                  m.type == "suffix");
                            cond = inRange && chk >= 0 && chk < static_cast<int>(fixed.size()) &&
                                   isVowel(fixed[chk]);
                        } else if (m.scope == "consonant") {
                            const bool inRange = (chk >= 0 && m.type == "prefix") ||
                                                 (chk < static_cast<int>(fixed.size()) &&
                                                  m.type == "suffix");
                            cond = inRange && chk >= 0 && chk < static_cast<int>(fixed.size()) &&
                                   isConsonant(fixed[chk]);
                        } else if (m.scope == "exact") {
                            int s = 0;
                            int e = 0;
                            if (m.type == "suffix") {
                                s = end;
                                e = end + static_cast<int>(m.value.size());
                            } else {
                                s = start - static_cast<int>(m.value.size());
                                e = start;
                            }
                            cond = isExact(m.value, fixed, s, e, false);
                        }

                        if (((!cond) ^ m.negative)) {
                            replace = false;
                            break;
                        }
                    }

                    if (replace) {
                        output += rule.replace;
                        cur = end;
                        matched = true;
                        break;
                    }
                }
            }

            if (matched) {
                break;
            }

            output += pattern.replace;
            cur = end;
            matched = true;
            break;
        }

        if (!matched) {
            output.push_back(fixed[cur]);
            ++cur;
        }
    }
    return output;
}

namespace {
#include "avro_rules.inc"
} // namespace

} // namespace fcitx
