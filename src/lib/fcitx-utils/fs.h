/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_FS_H_
#define _FCITX_UTILS_FS_H_

#include <cstdio>
#include <optional>
#include <string>
#include <fcitx-utils/misc.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Simple file system related API for checking file status.

namespace fcitx {

class UnixFD;
class StandardPathFile;

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
/**
 * \brief Return modified time in seconds of given path. 0 will be returned upon
 * error.
 *
 * \since 5.0.10
 */
FCITXUTILS_EXPORT int64_t modifiedTime(const std::string &path);

/**
 * \brief open the unix fd with fdopen.
 *
 * since fdopen'ed fd will be closed upon fclose, the ownership of fd will be
 * transferred to the FILE if the file descriptor is opened. Otherwise fd is
 * untouched. This helps user to avoid double close on the same file descriptor,
 * since the file descriptor number might be reused.
 *
 * \param fd file descriptor
 * \param modes modes passed to fdopen
 * \return FILE pointer if fd is valid and successfully opened.
 * \since 5.0.16
 */
FCITXUTILS_EXPORT UniqueFilePtr openFD(UnixFD &fd, const char *modes);

/**
 * \brief open the standard path file fd with fdopen.
 *
 * \param fd file descriptor
 * \param modes modes passed to fdopen
 * \return FILE pointer if fd is valid and successfully opened.
 * \see openUnixFD
 * \since 5.0.16
 */
FCITXUTILS_EXPORT UniqueFilePtr openFD(StandardPathFile &file,
                                           const char *modes);
} // namespace fs
} // namespace fcitx

#endif // _FCITX_UTILS_FS_H_
