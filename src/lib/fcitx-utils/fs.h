//
// Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UTILS_FS_H_
#define _FCITX_UTILS_FS_H_

#include <optional>
#include <string>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Simple file system related API for checking file status.

namespace fcitx {
namespace fs {

/// \brief check whether path is a directory.
FCITXUTILS_EXPORT bool isdir(const std::string &path);
/// \brief check whether path is a regular file.
FCITXUTILS_EXPORT bool isreg(const std::string &path);
/// \brief check whether path is a link.
FCITXUTILS_EXPORT bool islnk(const std::string &path);

/// \brief Get the clean path by removing . , .. , and duplicate / in the path.
FCITXUTILS_EXPORT std::string cleanPath(const std::string &path);
/// \brief Create directory recursively.
FCITXUTILS_EXPORT bool makePath(const std::string &path);
/// \brief Get directory name of path
FCITXUTILS_EXPORT std::string dirName(const std::string &path);
/// \brief Get base file name of path.
FCITXUTILS_EXPORT std::string baseName(const std::string &path);

/// \brief a simple wrapper around read(), ignore EINTR.
FCITXUTILS_EXPORT ssize_t safeRead(int fd, void *data, size_t maxlen);
/// \brief a simple wrapper around write(), ignore EINTR.
FCITXUTILS_EXPORT ssize_t safeWrite(int fd, const void *data, size_t maxlen);
/// \brief read symlink.
FCITXUTILS_EXPORT std::optional<std::string> readlink(const std::string &path);
} // namespace fs
} // namespace fcitx

#endif // _FCITX_UTILS_FS_H_
