/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UTILS_FS_H_
#define _FCITX_UTILS_FS_H_

#include "fcitxutils_export.h"
#include <string>

namespace fcitx {
namespace fs {

FCITXUTILS_EXPORT bool isdir(const std::string &path);
FCITXUTILS_EXPORT bool isreg(const std::string &path);
FCITXUTILS_EXPORT bool islnk(const std::string &path);

FCITXUTILS_EXPORT std::string cleanPath(const std::string &path);
FCITXUTILS_EXPORT bool makePath(const std::string &path);
FCITXUTILS_EXPORT std::string dirName(const std::string &path);
FCITXUTILS_EXPORT std::string baseName(const std::string &path);

FCITXUTILS_EXPORT ssize_t safeRead(int fd, void *data, size_t maxlen);
FCITXUTILS_EXPORT ssize_t safeWrite(int fd, const void *data, size_t maxlen);
}
}

#endif // _FCITX_UTILS_FS_H_
