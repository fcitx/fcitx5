/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fs.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <fcitx-utils/fcitxutils_export.h>
#include "misc.h"
#include "mtime_p.h"
#include "standardpath.h"
#include "stringutils.h"
#include "utf8.h" // IWYU pragma: keep

#ifdef _WIN32
#include <wchar.h>
#endif

namespace fcitx::fs {

namespace {

int makeDir(const std::string &name) {
#ifdef _WIN32
    const auto path = utf8::UTF8ToUTF16(name);
    return ::_wmkdir(path.data());
#else
    return ::mkdir(name.c_str(), 0777);
#endif
}

bool makePathHelper(const std::string &name) {
    if (makeDir(name) == 0) {
        return true;
    }
    if (errno == EEXIST) {
        return isdir(name);
    }

    // Check if error is parent not exists.
    if (errno != ENOENT) {
        return false;
    }

    // Try to create because parent doesn't exist.
    auto pos = name.rfind('/');
    if (pos == std::string::npos || pos == 0 || name[pos - 1] == '/') {
        return false;
    }

    std::string parent = name.substr(0, pos);
    if (!makePathHelper(parent)) {
        return false;
    }

    // try again
    if (makeDir(name) == 0) {
        return true;
    }
    return errno == EEXIST && isdir(name);
}

} // namespace

bool isdir(const std::string &path) {
    struct stat stats;
    return (stat(path.c_str(), &stats) == 0 && S_ISDIR(stats.st_mode) &&
            access(path.c_str(), R_OK | X_OK) == 0);
}

bool isreg(const std::string &path) {
    struct stat stats;
    return (stat(path.c_str(), &stats) == 0 && S_ISREG(stats.st_mode) &&
            access(path.c_str(), R_OK) == 0);
}

bool isexe(const std::string &path) {
    struct stat stats;
    return (stat(path.c_str(), &stats) == 0 && S_ISREG(stats.st_mode) &&
            access(path.c_str(), R_OK | X_OK) == 0);
}

bool islnk(const std::string &path) {
#ifdef _WIN32
    FCITX_UNUSED(path);
    return false;
#else
    struct stat stats;
    return lstat(path.c_str(), &stats) == 0 && S_ISLNK(stats.st_mode);
#endif
}

std::string cleanPath(const std::string &path) {
    std::string buf;
    if (path.empty()) {
        return {};
    }

    // skip first group of continous slash, for possible furture windows support
    size_t i = 0;
    while (path[i] == '/') {
        buf.push_back(path[i]);
        i++;
    }
    const size_t leading = i;

    int levels = 0;
    while (true) {
        size_t dotcount = 0;
        const size_t last = buf.size();
        const size_t lasti = i;
        // We have something already in the path, push a new slash.
        if (last > leading) {
            buf.push_back('/');
        }
        // Find still next '/' and count '.'
        while (i < path.size() && path[i] != '/') {
            if (path[i] == '.') {
                dotcount++;
            }

            buf.push_back(path[i]);

            i++;
        }

        // everything is a dot
        if (dotcount == i - lasti) {
            if (dotcount == 1) {
                buf.erase(last);
            } else if (dotcount == 2) {
                // If we already at the beginning, don't go up.
                if (levels > 0 && last != leading) {
                    size_t k;
                    for (k = last; k > leading; k--) {
                        if (buf[k - 1] == '/') {
                            break;
                        }
                    }
                    if (k == leading) {
                        buf.erase(k);
                    } else if (buf[k - 1] == '/') {
                        buf.erase(k - 1);
                    }
                    levels--;
                }
            } else {
                levels++;
            }
        } else {
            levels++;
        }

        while (i < path.size() && path[i] == '/') {
            i++;
        }

        if (i >= path.size()) {
            break;
        }
    }
    if (stringutils::startsWith(buf, "./")) {
        return buf.substr(2);
    }
    return buf;
}

bool makePath(const std::string &path) {
    if (isdir(path)) {
        return true;
    }
    auto opath = cleanPath(path);
    while (!opath.empty() && opath.back() == '/') {
        opath.pop_back();
    }

    if (opath.empty()) {
        return true;
    }

    return makePathHelper(opath);
}

std::string dirName(const std::string &path) {
    auto result = path;
    // remove trailing slash
    while (result.size() > 1 && result.back() == '/') {
        result.pop_back();
    }
    if (result.size() <= 1) {
        return result;
    }

    auto iter = std::find(result.rbegin(), result.rend(), '/');
    if (iter != result.rend()) {
        result.erase(iter.base(), result.end());
        // remove trailing slash
        while (result.size() > 1 && result.back() == '/') {
            result.pop_back();
        }
    } else {
        result = ".";
    }
    return result;
}

FCITXUTILS_DEPRECATED_EXPORT std::string baseName(const std::string &path) {
    return baseName(std::string_view(path));
}

std::string baseName(std::string_view path) {
    // remove trailing slash
    while (path.size() > 1 && path.back() == '/') {
        path.remove_suffix(1);
    }
    if (path.size() <= 1) {
        return std::string{path};
    }

    auto iter = std::find(path.rbegin(), path.rend(), '/');
    if (iter != path.rend()) {
        path.remove_prefix(std::distance(path.begin(), iter.base()));
    }
    return std::string{path};
}

ssize_t safeRead(int fd, void *data, size_t maxlen) {
    ssize_t ret = 0;
    do {
        ret = read(fd, data, maxlen);
    } while (ret == -1 && errno == EINTR);
    return ret;
}
ssize_t safeWrite(int fd, const void *data, size_t maxlen) {
    ssize_t ret = 0;
    do {
        ret = write(fd, data, maxlen);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

std::optional<std::string> readlink(const std::string &path) {
#ifdef _WIN32
    FCITX_UNUSED(path);
#else
    std::string buffer;
    buffer.resize(256);
    ssize_t readSize;

    while (true) {
        readSize = ::readlink(path.data(), buffer.data(), buffer.size());
        if (readSize < 0) {
            return std::nullopt;
        }

        if (static_cast<size_t>(readSize) < buffer.size()) {
            buffer.resize(readSize);
            return buffer;
        }

        buffer.resize(buffer.size() * 2);
    }
#endif
    return std::nullopt;
}

int64_t modifiedTime(const std::string &path) {
    struct stat stats;
    if (stat(path.c_str(), &stats) != 0) {
        return 0;
    }
    return fcitx::modifiedTimeImpl(stats, 0).sec;
}

template <typename FDLike>
UniqueFilePtr openFDImpl(FDLike &fd, const char *modes) {
    if (!fd.isValid()) {
        return nullptr;
    }
    UniqueFilePtr file(fdopen(fd.fd(), modes));
    if (file) {
        fd.release();
    }
    return file;
}

UniqueFilePtr openFD(UnixFD &fd, const char *modes) {
    return openFDImpl(fd, modes);
}

UniqueFilePtr openFD(StandardPathFile &file, const char *modes) {
    return openFDImpl(file, modes);
}

} // namespace fcitx::fs
