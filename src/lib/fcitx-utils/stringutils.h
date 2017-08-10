/*
 * Copyright (C) 2015~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_UTILS_STRINGUTILS_H_
#define _FCITX_UTILS_STRINGUTILS_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief String handle utilities.

#include "fcitxutils_export.h"
#include <string>
#include <vector>

namespace fcitx {
namespace stringutils {

/// \brief Check if a string starts with a prefix.
FCITXUTILS_EXPORT bool startsWith(const std::string &str,
                                  const std::string &prefix);

/// \brief Check if a string starts with a prefix char.
inline bool startsWith(const std::string &str, char prefix) {
    return str.size() && str.front() == prefix;
}

/// \brief Check if a string ends with a suffix.
FCITXUTILS_EXPORT bool endsWith(const std::string &str,
                                const std::string &suffix);

/// \brief Check if a string ends with a suffix char.
inline bool endsWith(const std::string &str, char suffix) {
    return str.size() && str.back() == suffix;
}

/// \brief Trim the whitespace by returning start end end of first and list non
/// whitespace character position.
///
/// Will return a pair of equal value all characters are whitespace.
FCITXUTILS_EXPORT std::pair<std::string::size_type, std::string::size_type>
trimInplace(const std::string &str);

/// \brief Split the string by delim.
FCITXUTILS_EXPORT std::vector<std::string> split(const std::string &str,
                                                 const std::string &delim);

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
}
}

#endif // _FCITX_UTILS_STRINGUTILS_H_
