/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include "fcitx-utils/fcitxutils_export.h"
#include "fs.h"
#include "misc.h"
#include "standardpath.h"

namespace fcitx::fs {

// FIXME: Remove this deprecated API in future releases.
FCITXUTILS_DEPRECATED_EXPORT bool makePath(const std::string &path) {
    return makePath(std::filesystem::path(path));
}

FCITXUTILS_DEPRECATED_EXPORT std::string baseName(const std::string &path) {
    return baseName(std::string_view(path));
}

FCITXUTILS_DEPRECATED_EXPORT int64_t modifiedTime(const std::string &path) {
    return modifiedTime(std::filesystem::path(path));
}

UniqueFilePtr openFD(StandardPathFile &file, const char *modes) {
    if (!file.isValid()) {
        return nullptr;
    }
    UniqueFilePtr fd(fdopen(file.fd(), modes));
    if (fd) {
        file.release();
    }
    return fd;
}

} // namespace fcitx::fs
