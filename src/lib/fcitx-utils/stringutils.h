/*
 * Copyright (C) 2015~2015 by CSSlayer
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
#include <string>
#include <vector>
#include "fcitxutils_export.h"

namespace fcitx {
namespace stringutils {
FCITXUTILS_EXPORT bool startsWith(const std::string &str, const std::string &prefix);
FCITXUTILS_EXPORT bool endsWith(const std::string &str, const std::string &suffix);
FCITXUTILS_EXPORT std::pair<std::string::size_type, std::string::size_type>
trimInplace(const std::string &str);
FCITXUTILS_EXPORT std::vector<std::string> split(const std::string &str,
                                                 const std::string &delim);
FCITXUTILS_EXPORT std::string replaceAll(std::string str,
                                         const std::string &before,
                                         const std::string &after);
FCITXUTILS_EXPORT const char* backwardSearch(const char* haystack, size_t l, const char* needle, size_t ol, size_t from);
FCITXUTILS_EXPORT char* backwardSearch(char* haystack, size_t l, const char* needle, size_t ol, size_t from);
FCITXUTILS_EXPORT size_t backwardSearch(const std::string &haystack, const std::string &needle, size_t from);
}
};

#endif // _FCITX_UTILS_STRINGUTILS_H_
