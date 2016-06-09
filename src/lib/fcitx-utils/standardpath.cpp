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

#include "standardpath.h"
#include <unordered_set>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include <unordered_map>
#include <fcntl.h>
#include "fs.h"
#include "stringutils.h"
#include "config.h"

namespace fcitx {

class StandardPathPrivate {
public:
    StandardPathPrivate(bool skipFcitxPath) {
        // initialize user directory
        configHome = defaultPath("XDG_CONFIG_HOME", ".config");
        configDirs = defaultPaths("XDG_CONFIG_DIRS", "/etc/xdg", nullptr);
        dataHome = defaultPath("XDG_DATA_HOME", ".local/share");
        dataDirs = defaultPaths("XDG_DATA_DIRS", "/usr/local/share:/usr/share",
                                skipFcitxPath ? nullptr : "datadir");
        cacheHome = defaultPath("XDG_CACHE_HOME", ".cache");
        const char *tmpdir = getenv("TMPDIR");
        runtimeDir = defaultPath("XDG_RUNTIME_DIR",
                                 !tmpdir || !tmpdir[0] ? "/tmp" : tmpdir);
        addonDirs =
            defaultPaths("FCITX_ADDON_DIRS", FCITX_INSTALL_ADDONDIR, nullptr);
    }

    // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    std::string defaultPath(const char *env, const char *defaultPath) {
        char *cdir = getenv(env);
        std::string dir;
        if (cdir && cdir[0]) {
            dir = cdir;
        } else {
            // caller need to ensure HOME is not empty;
            if (defaultPath[0] != '/') {
                const char *home = getenv("HOME");
                dir = std::string(home) + "/" + defaultPath;
            } else {
                if (strcmp(env, "XDG_RUNTIME_DIR") == 0) {
                    dir = std::string(defaultPath) + "/fcitx-runtime-" +
                          std::to_string(geteuid());
                    if (!fs::isdir(dir)) {
                        if (mkdir(dir.c_str(), 0700) != 0) {
                            return {};
                        }
                    }
                } else {
                    dir = defaultPath;
                }
            }
        }

        if (!dir.empty() && strcmp(env, "XDG_RUNTIME_DIR") == 0) {
            struct stat buf;
            if (stat(dir.c_str(), &buf) != 0 || buf.st_uid != geteuid() ||
                buf.st_mode != 0700) {
                return {};
            }
        }
        return dir;
    }

    std::vector<std::string> defaultPaths(const char *env,
                                          const char *defaultPath,
                                          const char *fcitxPath) {
        std::vector<std::string> dirs;

        const char *dir = getenv(env);
        if (!dir) {
            dir = defaultPath;
        }

        auto rawDirs = stringutils::split(dir, ":");
        std::unordered_set<std::string> uniqueDirs(rawDirs.begin(), rawDirs.end());

        for (auto &s : rawDirs) {
            auto iter = uniqueDirs.find(s);
            if (iter != uniqueDirs.end()) {
                uniqueDirs.erase(iter);
                dirs.push_back(s);
            }
        }
        if (fcitxPath) {
            std::string path = StandardPath::fcitxPath(fcitxPath);
            if (!path.empty() &&
                std::find(dirs.begin(), dirs.end(), path) == dirs.end()) {
                dirs.push_back(path);
            }
        }

        return dirs;
    }

    std::string userPath(StandardPath::Type type) const {
        switch (type) {
        case StandardPath::Type::Config:
            return configHome;
        case StandardPath::Type::Data:
            return dataHome;
        case StandardPath::Type::Cache:
            return cacheHome;
            break;
        case StandardPath::Type::Runtime:
            return runtimeDir;
        default:
            return {};
        }
    }

    std::vector<std::string> directories(StandardPath::Type type) const {
        switch (type) {
        case StandardPath::Type::Config:
            return configDirs;
        case StandardPath::Type::Data:
            return dataDirs;
        case StandardPath::Type::Addon:
            return addonDirs;
        default:
            return {};
        }
    }

    std::string configHome;
    std::vector<std::string> configDirs;
    std::string dataHome;
    std::vector<std::string> dataDirs;
    std::string cacheHome;
    std::string runtimeDir;
    std::vector<std::string> addonDirs;
};

std::string constructPath(const std::string &basepath,
                          const std::string &path) {
    if (basepath.empty()) {
        return {};
    }
    return fs::cleanPath(basepath + "/" + path);
}

StandardPath::StandardPath(bool skipFcitxPath)
    : d_ptr(std::make_unique<StandardPathPrivate>(skipFcitxPath)) {}

StandardPath::~StandardPath() {}

std::string StandardPath::fcitxPath(const char *path) {
    if (!path) {
        return {};
    }

    static std::unordered_map<std::string, std::string> pathMap = {
        std::make_pair<std::string, std::string>("datadir",
                                                 FCITX_INSTALL_DATADIR),
        std::make_pair<std::string, std::string>("pkgdatadir",
                                                 FCITX_INSTALL_PKGDATADIR),
        std::make_pair<std::string, std::string>("libdir",
                                                 FCITX_INSTALL_LIBDIR),
        std::make_pair<std::string, std::string>("bindir",
                                                 FCITX_INSTALL_BINDIR),
        std::make_pair<std::string, std::string>("localedir",
                                                 FCITX_INSTALL_LOCALEDIR),
        std::make_pair<std::string, std::string>("addondir",
                                                 FCITX_INSTALL_ADDONDIR),
        std::make_pair<std::string, std::string>("libdatadir",
                                                 FCITX_INSTALL_LIBDATADIR),
    };

    if (pathMap.count(path)) {
        return pathMap[path];
    }

    return {};
}

void closedir0(DIR *dir) {
    if (dir) {
        closedir(dir);
    }
}

std::string StandardPath::userDirectory(Type type) const {
    FCITX_D();
    return d->userPath(type);
}

std::vector<std::string> StandardPath::directories(Type type) const {
    FCITX_D();
    return d->directories(type);
}

void StandardPath::scanDirectories(
    Type type,
    std::function<bool(const std::string &path, bool user)> scanner) {
    FCITX_D();
    std::string userDir = d->userPath(type);
    std::vector<std::string> list = d->directories(type);
    if (userDir.empty() && list.empty()) {
        return;
    }

    size_t len = (!userDir.empty() ? 1 : 0) + (list.size() ? list.size() : 0);

    for (size_t i = 0; i < len; i++) {
        bool isUser = false;
        std::string dirBasePath;
        if (!userDir.empty()) {
            isUser = (i == 0);
            dirBasePath = isUser ? userDir : list[i - 1];
        } else {
            dirBasePath = list[i];
        }

        dirBasePath = fs::cleanPath(dirBasePath);
        if (!scanner(dirBasePath, isUser)) {
            return;
        }
    }
}

void StandardPath::scanFiles(
    Type type, const std::string &path,
    std::function<bool(const std::string &fileName, const std::string &dir,
                       bool user)> scanner) {
    scanDirectories(type,
                    [scanner, &path](const std::string &dirPath, bool isUser) {
                        auto fullPath = constructPath(dirPath, path);
                        std::unique_ptr<DIR, decltype(closedir0) *> scopedDir{
                            opendir(fullPath.c_str()), closedir0};
                        if (scopedDir) {
                            auto dir = scopedDir.get();
                            struct dirent *drt;
                            while ((drt = readdir(dir)) != nullptr) {
                                if (strcmp(drt->d_name, ".") == 0 ||
                                    strcmp(drt->d_name, "..") == 0) {
                                    continue;
                                }

                                if (!scanner(drt->d_name, fullPath, isUser)) {
                                    return false;
                                }
                            }
                        }
                        return true;
                    });
}

std::string StandardPath::locate(Type type, const std::string &path) {
    std::string retPath;
    scanDirectories(type, [&retPath, &path](const std::string &dirPath, bool) {
        auto fullPath = constructPath(dirPath, path);
        if (fs::isreg(fullPath)) {
            return true;
        }
        retPath = fullPath;
        return false;
    });
    return retPath;
}

std::vector<std::string> StandardPath::locateAll(Type type,
                                                 const std::string &path) {
    std::vector<std::string> retPaths;
    scanDirectories(type, [&retPaths, &path](const std::string &dirPath, bool) {
        auto fullPath = constructPath(dirPath, path);
        if (fs::isreg(fullPath)) {
            retPaths.push_back(fullPath);
        }
        return true;
    });
    return retPaths;
}

StandardPathFile StandardPath::open(Type type, const std::string &path,
                                    int flags) {
    StandardPathFile file;
    scanDirectories(type,
                    [flags, &file, &path](const std::string &dirPath, bool) {
                        auto fullPath = constructPath(dirPath, path);
                        int fd = ::open(fullPath.c_str(), flags);
                        if (fd < 0) {
                            return true;
                        }
                        file.first = fd;
                        file.second = fullPath;
                        return false;
                    });
    return file;
}

std::unordered_map<std::string, StandardPathFile> StandardPath::multiOpenFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)> filter) {
    std::unordered_map<std::string, StandardPathFile> result;
    scanFiles(type, path,
              [&result, flags, &filter](const std::string &path,
                                        const std::string dir, bool isUser) {
                  if (!result.count(path) && filter(path, dir, isUser)) {
                      auto fullPath = constructPath(dir, path);
                      int fd = ::open(fullPath.c_str(), flags);
                      if (fd >= 0) {
                          result[path] = std::make_pair(fd, fullPath);
                      }
                  }
                  return true;
              });

    return result;
}

StandardPathFilesMap
StandardPath::multiOpenAllFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)> filter) {
    StandardPathFilesMap result;
    scanFiles(type, path,
              [&result, flags, &filter](const std::string &path,
                                        const std::string dir, bool isUser) {
                  if (filter(path, dir, isUser)) {
                      auto fullPath = constructPath(dir, path);
                      int fd = ::open(fullPath.c_str(), flags);
                      if (fd >= 0) {
                          result[path].push_back(std::make_pair(fd, fullPath));
                      }
                  }
                  return true;
              });

    return result;
}
}
