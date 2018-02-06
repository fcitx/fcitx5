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

#include "fs.h"
#include <algorithm>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fcitx {
namespace fs {

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

bool islnk(const std::string &path) {
    struct stat stats;
    return lstat(path.c_str(), &stats) == 0 && S_ISLNK(stats.st_mode);
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

    int levels = 0;
    while (true) {
        size_t dotcount = 0;
        size_t last = buf.size();
        size_t lasti = i;
        while (i < path.size() && path[i] != '/') {
            if (path[i] == '.') {
                dotcount++;
            }

            buf.push_back(path[i]);

            i++;
        }

        bool eaten = false;
        // everything is a dot
        if (dotcount == i - lasti) {
            if (dotcount == 1) {
                buf.erase(last);
                eaten = true;
            } else if (dotcount == 2) {
                if (levels > 0) {
                    for (int k = last - 2; k >= 0; k--) {
                        if (buf[k] == '/') {
                            buf.erase(k + 1);
                            eaten = true;
                            break;
                        }
                    }
                }
            } else {
                levels++;
            }
        } else {
            levels++;
        }

        if (i >= path.size()) {
            break;
        }

        while (path[i] == '/') {
            i++;
        }

        if (!eaten) {
            buf.push_back('/');
        }
    }
    return buf;
}

bool makePath(const std::string &path) {
    if (isdir(path))
        return true;
    auto opath = cleanPath(path);
    while (opath.size() && opath.back() == '/') {
        opath.pop_back();
    }

    if (opath.empty()) {
        return true;
    }

    // skip first /, the root directory or unc is not what we can create
    auto iter = opath.begin();
    while (iter != opath.end() && *iter == '/') {
        iter++;
    }
    while (true) {
        if (iter == opath.end() || *iter == '/') {
            std::string curpath(opath.begin(), iter);

            if (mkdir(curpath.c_str(), S_IRWXU) != 0) {
                if (errno == EEXIST) {
                    if (!isdir(curpath.c_str())) {
                        return false;
                    }
                }
            }
        }

        if (iter == opath.end()) {
            break;
        } else {
            iter++;
        }
    }
    return true;
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

std::string baseName(const std::string &path) {
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
        result.erase(result.begin(), iter.base());
    }
    return result;
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
} // namespace fs
} // namespace fcitx
