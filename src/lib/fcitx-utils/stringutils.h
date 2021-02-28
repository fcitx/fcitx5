/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_STRINGUTILS_H_
#define _FCITX_UTILS_STRINGUTILS_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief String handle utilities.

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include "fcitxutils_export.h"
#include "stringutils_details.h"

namespace fcitx {
namespace stringutils {

/// \brief Check if a string starts with a prefix.
FCITXUTILS_EXPORT bool startsWith(std::string_view str,
                                  std::string_view prefix);

/// \brief Check if a string starts with a prefix char.
inline bool startsWith(std::string_view str, char prefix) {
    return !str.empty() && str.front() == prefix;
}

/// \brief Check if a string ends with a suffix.
FCITXUTILS_EXPORT bool endsWith(std::string_view str, std::string_view suffix);

/// \brief Check if a string ends with a suffix char.
inline bool endsWith(std::string_view str, char suffix) {
    return !str.empty() && str.back() == suffix;
}

/// \brief Check if a string is a concatenation of two other strings
inline bool isConcatOf(std::string_view str, std::string_view sub1,
                       std::string_view sub2) {
    return str.size() == sub1.size() + sub2.size() && startsWith(str, sub1) &&
           endsWith(str, sub2);
}

/// \brief Trim the whitespace by returning start end end of first and list non
/// whitespace character position.
///
/// Will return a pair of equal value all characters are whitespace.
FCITXUTILS_EXPORT std::pair<std::string::size_type, std::string::size_type>
trimInplace(const std::string &str);

/// \brief Trim the white space in str.
/// \see trimInplace
FCITXUTILS_EXPORT std::string trim(const std::string &str);

/// \brief Split the string by delim.
FCITXUTILS_EXPORT std::vector<std::string> split(const std::string &str,
                                                 const std::string &delim);

enum class SplitBehavior { KeepEmpty, SkipEmpty };

/// \brief Split the string by delim.
FCITXUTILS_EXPORT std::vector<std::string>
split(const std::string &str, const std::string &delim, SplitBehavior behavior);

/// \brief Replace all substring appearance of before with after.
FCITXUTILS_EXPORT std::string replaceAll(std::string str,
                                         const std::string &before,
                                         const std::string &after);

/// \brief Search string needle of size ol in string haystack.
/// \param from the number of bytes from end.
/// \return point to data or null.
FCITXUTILS_EXPORT const char *backwardSearch(const char *haystack, size_t l,
                                             const char *needle, size_t ol,
                                             size_t from);

/// \brief The non-const version of backwardSearch
/// \see backwardSearch()
FCITXUTILS_EXPORT char *backwardSearch(char *haystack, size_t l,
                                       const char *needle, size_t ol,
                                       size_t from);

/// \brief Fast backward substring search.
/// \return back from end.
///
/// Example:
/// stringutils::backwardSearch("abcabc", "bc", 1) == 1
/// stringutils::backwardSearch("abcabc", "bc", 1) == 1
/// stringutils::backwardSearch("abcabc", "bc", 4) == 4
FCITXUTILS_EXPORT size_t backwardSearch(const std::string &haystack,
                                        const std::string &needle, size_t from);

/// \brief Join a range of string with delim.
template <typename Iter, typename T>
FCITXUTILS_EXPORT std::string join(Iter start, Iter end, T &&delim) {
    std::string result;
    if (start != end) {
        result += (*start);
        start++;
    }
    for (; start != end; start++) {
        result += (delim);
        result += (*start);
    }
    return result;
}

/// \brief Join a set of string with delim.
template <typename C, typename T>
inline std::string join(C &&container, T &&delim) {
    using std::begin;
    using std::end;
    return join(begin(container), end(container), delim);
}

/// \brief Join the strings with delim.
template <typename C, typename T>
inline std::string join(std::initializer_list<C> &&container, T &&delim) {
    using std::begin;
    using std::end;
    return join(begin(container), end(container), delim);
}

template <typename... Args>
std::string concat(const Args &...args) {
    using namespace ::fcitx::stringutils::details;
    return concatPieces({static_cast<const UniversalPiece &>(
                             details::UniversalPieceHelper<Args>::forward(args))
                             .toPair()...});
}

template <typename FirstArg, typename... Args>
std::string joinPath(const FirstArg &firstArg, const Args &...args) {
    using namespace ::fcitx::stringutils::details;
    return concatPathPieces(
        {static_cast<const UniversalPiece &>(
             UniversalPieceHelper<FirstArg>::forward(firstArg))
             .toPathPair(false),
         static_cast<const UniversalPiece &>(
             UniversalPieceHelper<Args>::forward(args))
             .toPathPair()...});
}

constexpr bool literalEqual(char const *a, char const *b) {
    return *a == *b && (*a == '\0' || literalEqual(a + 1, b + 1));
}

/// \brief Inplace unescape a string contains slash, new line, optionally quote.
FCITXUTILS_EXPORT bool unescape(std::string &str, bool unescapeQuote);

} // namespace stringutils
} // namespace fcitx

#endif // _FCITX_UTILS_STRINGUTILS_H_
