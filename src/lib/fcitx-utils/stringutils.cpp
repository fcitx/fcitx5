/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "stringutils.h"
#include <climits>
#include <cstring>
#include <string>
#include "charutils.h"
#include "macros.h"

namespace fcitx::stringutils {
namespace details {

std::string
concatPieces(std::initializer_list<std::pair<const char *, std::size_t>> list) {
    std::size_t size = 0;
    for (auto pair : list) {
        size += pair.second;
    }
    std::string result;
    result.reserve(size);
    for (const auto &pair : list) {
        result.append(pair.first, pair.first + pair.second);
    }
    assert(result.size() == size);
    return result;
}

std::string concatPathPieces(
    std::initializer_list<std::pair<const char *, std::size_t>> list) {
    if (!list.size()) {
        return {};
    }

    bool first = true;
    bool firstPieceIsSlash = false;
    std::size_t size = 0;
    for (const auto &pair : list) {
        if (first) {
            if (pair.first[pair.second - 1] == '/') {
                firstPieceIsSlash = true;
            }
            first = false;
        } else {
            size += 1;
        }
        size += pair.second;
    }
    if (list.size() > 1 && firstPieceIsSlash) {
        size -= 1;
    }
    std::string result;
    result.reserve(size);
    first = true;
    for (auto pair : list) {
        if (first) {
            first = false;
        } else if (firstPieceIsSlash) {
            firstPieceIsSlash = false;
        } else {
            result += '/';
        }

        result.append(pair.first, pair.first + pair.second);
    }
    assert(result.size() == size);
    return result;
}
} // namespace details

FCITXUTILS_DEPRECATED_EXPORT bool startsWith(const std::string &str,
                                             const std::string &prefix) {
    return startsWith(std::string_view(str), std::string_view(prefix));
}

bool startsWith(std::string_view str, std::string_view prefix) {
    if (str.size() < prefix.size()) {
        return false;
    }

    return (str.compare(0, prefix.size(), prefix) == 0);
}

FCITXUTILS_DEPRECATED_EXPORT bool endsWith(const std::string &str,
                                           const std::string &suffix) {
    return endsWith(std::string_view(str), std::string_view(suffix));
}

bool endsWith(std::string_view str, std::string_view suffix) {
    if (str.size() < suffix.size()) {
        return false;
    }

    return (str.compare(str.size() - suffix.size(), suffix.size(), suffix) ==
            0);
}

inline std::pair<std::string::size_type, std::string::size_type>
trimInplaceImpl(std::string_view str) {
    auto start = str.find_first_not_of(FCITX_WHITESPACE);
    if (start == std::string::npos) {
        return {str.size(), str.size()};
    }

    auto end = str.size();
    while (end > start && charutils::isspace(str[end - 1])) {
        --end;
    }

    return {start, end};
}

FCITXUTILS_DEPRECATED_EXPORT
std::pair<std::string::size_type, std::string::size_type>
trimInplace(const std::string &str) {
    return trimInplaceImpl(str);
}

std::pair<std::string::size_type, std::string::size_type>
trimInplace(std::string_view str) {
    return trimInplaceImpl(str);
}

FCITXUTILS_DEPRECATED_EXPORT
std::string trim(const std::string &str) { return trim(std::string_view(str)); }

std::string trim(std::string_view str) {
    auto pair = trimInplaceImpl(str);
    return {str.begin() + pair.first, str.begin() + pair.second};
}

std::string_view trimView(std::string_view str) {
    auto pair = trimInplace(str);
    return str.substr(pair.first, pair.second - pair.first);
}

FCITXUTILS_DEPRECATED_EXPORT
std::vector<std::string> split(const std::string &str, const std::string &delim,
                               SplitBehavior behavior) {
    return split(std::string_view(str), std::string_view(delim), behavior);
}

std::vector<std::string> split(std::string_view str, std::string_view delim,
                               SplitBehavior behavior) {
    std::vector<std::string> strings;
    std::string::size_type lastPos;
    std::string::size_type pos;
    if (behavior == SplitBehavior::SkipEmpty) {
        lastPos = str.find_first_not_of(delim, 0);
    } else {
        lastPos = 0;
    }
    pos = str.find_first_of(delim, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        strings.push_back(std::string(str.substr(lastPos, pos - lastPos)));
        if (behavior == SplitBehavior::SkipEmpty) {
            lastPos = str.find_first_not_of(delim, pos);
        } else {
            if (pos == std::string::npos) {
                break;
            }
            lastPos = pos + 1;
        }
        pos = str.find_first_of(delim, lastPos);
    }

    return strings;
}

FCITXUTILS_DEPRECATED_EXPORT std::vector<std::string>
split(const std::string &str, const std::string &delim) {
    return split(std::string_view(str), std::string_view(delim));
}

std::vector<std::string> split(std::string_view str, std::string_view delim) {
    return split(str, delim, SplitBehavior::SkipEmpty);
}

std::string replaceAll(std::string str, const std::string &before,
                       const std::string &after) {
    if (before.empty()) {
        return str;
    }

    constexpr int MAX_REPLACE_INDICES_NUM = 128;

    size_t pivot = 0;
    std::string newString;
    size_t lastLen = 0;
    size_t indices[MAX_REPLACE_INDICES_NUM];

    size_t newStringPos = 0;
    size_t oldStringPos = 0;

    auto copyAndMoveOn = [&newString, &newStringPos](std::string_view source,
                                                     size_t pos,
                                                     size_t length) {
        if (length == 0) {
            return;
        }
        // Append source[pos..pos+length] to newString.
        newString.replace(newStringPos, length, source, pos, length);
        newStringPos += length;
    };

    do {

        int nIndices = 0;
        while (nIndices < MAX_REPLACE_INDICES_NUM) {
            pivot = str.find(before, pivot);
            if (pivot == std::string::npos) {
                break;
            }

            indices[nIndices++] = pivot;
            pivot += before.size();
        }

        if (nIndices) {
            if (!lastLen) {
                lastLen = str.size() + nIndices * after.size() -
                          nIndices * before.size();
                newString.resize(lastLen);
            } else {
                size_t newLen = lastLen + nIndices * after.size() -
                                nIndices * before.size();
                lastLen = newLen;
                newString.resize(newLen);
            }

            // string s is split as
            // oldStringPos, indices[0], indices[0] + before.size(), indices[1],
            // indices[1] + before.size()
            // .... indices[nIndices - 1], indices[nIndices - 1] + before.size()
            copyAndMoveOn(str, oldStringPos, indices[0] - oldStringPos);
            copyAndMoveOn(after, 0, after.size());

            for (int i = 1; i < nIndices; i++) {
                copyAndMoveOn(str, indices[i - 1] + before.size(),
                              indices[i] - (indices[i - 1] + before.size()));
                copyAndMoveOn(after, 0, after.size());
            }

            oldStringPos = indices[nIndices - 1] + before.size();
        }
    } while (pivot != std::string::npos);

    if (!lastLen) {
        return str;
    }

    copyAndMoveOn(str, oldStringPos, str.size() - oldStringPos);
    newString.resize(newStringPos);

    return newString;
}

#define REHASH(a)                                                              \
    if (ol_minus_1 < sizeof(unsigned int) * CHAR_BIT)                          \
        hashHaystack -= (a) << ol_minus_1;                                     \
    hashHaystack <<= 1

const char *backwardSearch(const char *haystack, size_t l, const char *needle,
                           size_t ol, size_t from) {
    if (ol > l) {
        return nullptr;
    }
    size_t delta = l - ol;
    if (from > l) {
        return nullptr;
    }
    if (from > delta) {
        from = delta;
    }

    const char *end = haystack;
    haystack += from;
    const unsigned int ol_minus_1 = ol - 1;
    const char *n = needle + ol_minus_1;
    const char *h = haystack + ol_minus_1;
    unsigned int hashNeedle = 0;
    unsigned int hashHaystack = 0;
    size_t idx;
    for (idx = 0; idx < ol; ++idx) {
        hashNeedle = ((hashNeedle << 1) + *(n - idx));
        hashHaystack = ((hashHaystack << 1) + *(h - idx));
    }
    hashHaystack -= *haystack;
    while (haystack >= end) {
        hashHaystack += *haystack;
        if (hashHaystack == hashNeedle && memcmp(needle, haystack, ol) == 0) {
            return haystack;
        }
        --haystack;
        REHASH(*(haystack + ol));
    }
    return nullptr;
}

char *backwardSearch(char *haystack, size_t l, const char *needle, size_t ol,
                     size_t from) {
    return const_cast<char *>(backwardSearch(
        static_cast<const char *>(haystack), l, needle, ol, from));
}

size_t backwardSearch(const std::string &haystack, const std::string &needle,
                      size_t from) {
    const auto *cstr = haystack.c_str();
    const auto *result = backwardSearch(cstr, haystack.size(), needle.c_str(),
                                        needle.size(), from);
    if (result) {
        return result - cstr;
    }
    return std::string::npos;
}

enum class UnescapeState { NORMAL, ESCAPE };

bool unescape(std::string &str, bool unescapeQuote) {
    if (str.empty()) {
        return true;
    }

    size_t i = 0;
    size_t j = 0;
    UnescapeState state = UnescapeState::NORMAL;
    do {
        switch (state) {
        case UnescapeState::NORMAL:
            if (str[i] == '\\') {
                state = UnescapeState::ESCAPE;
            } else {
                str[j] = str[i];
                j++;
            }
            break;
        case UnescapeState::ESCAPE:
            if (str[i] == '\\') {
                str[j] = '\\';
                j++;
            } else if (str[i] == 'n') {
                str[j] = '\n';
                j++;
            } else if (str[i] == '\"' && unescapeQuote) {
                str[j] = '\"';
                j++;
            } else {
                return false;
            }
            state = UnescapeState::NORMAL;
            break;
        }
    } while (str[i++]);
    str.resize(j - 1);
    return true;
}

std::optional<std::string> unescapeForValue(std::string_view str) {
    bool unescapeQuote = false;
    // having quote at beginning and end, escape
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        unescapeQuote = true;
        str = str.substr(1, str.size() - 2);
    }
    if (str.empty()) {
        return std::string();
    }

    std::string value(str);
    if (!stringutils::unescape(value, unescapeQuote)) {
        return std::nullopt;
    }
    return value;
}

std::string escapeForValue(std::string_view str) {
    std::string value;
    value.reserve(str.size());
    const bool needQuote =
        str.find_first_of("\f\r\t\v \"") != std::string::npos;
    if (needQuote) {
        value.push_back('"');
    }
    for (char c : str) {
        switch (c) {
        case '\\':
            value.append("\\\\");
            break;
        case '\n':
            value.append("\\n");
            break;
        case '"':
            value.append("\\\"");
            break;
        default:
            value.push_back(c);
            break;
        }
    }
    if (needQuote) {
        value.push_back('"');
    }

    return value;
}
} // namespace fcitx::stringutils
