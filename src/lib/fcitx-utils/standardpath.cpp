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

#include "standardpath.h"
#include "config.h"
#include "fs.h"
#include "stringutils.h"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <stdexcept>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fcitx {

namespace {
bool isAbsolutePath(const std::string &path) {
    return !path.empty() && path[0] == '/';
}

std::string constructPath(const std::string &basepath,
                          const std::string &path) {
    if (basepath.empty()) {
        return {};
    }
    return fs::cleanPath(stringutils::joinPath(basepath, path));
}
} // namespace

StandardPathFile::~StandardPathFile() {}

int StandardPathFile::release() { return fd_.release(); }

StandardPathTempFile::~StandardPathTempFile() { close(); }

int StandardPathTempFile::release() { return fd_.release(); }

void StandardPathTempFile::removeTemp() {
    if (fd_.fd() >= 0) {
        // close it
        fd_.reset();
        unlink(tempPath_.c_str());
    }
}

void StandardPathTempFile::close() {
    if (fd_.fd() >= 0) {
        // sync first.
        fsync(fd_.fd());
        // close it
        fd_.reset();
        if (rename(tempPath_.c_str(), path_.c_str()) < 0) {
            unlink(tempPath_.c_str());
        }
    }
}

class StandardPathPrivate {
public:
    StandardPathPrivate(bool skipFcitxPath) {
        // initialize user directory
        configHome_ = defaultPath("XDG_CONFIG_HOME", ".config");
        pkgconfigHome_ = defaultPath(
            "FCITX_CONFIG_HOME", constructPath(configHome_, "fcitx5").c_str());
        configDirs_ = defaultPaths("XDG_CONFIG_DIRS", "/etc/xdg", nullptr);
        auto pkgconfigDirFallback = configDirs_;
        for (auto &path : pkgconfigDirFallback) {
            path = constructPath(path, "fcitx5");
        }
        pkgconfigDirs_ = defaultPaths(
            "FCITX_CONFIG_DIRS",
            stringutils::join(pkgconfigDirFallback, ":").c_str(), nullptr);

        dataHome_ = defaultPath("XDG_DATA_HOME", ".local/share");
        pkgdataHome_ = defaultPath("FCITX_DATA_HOME",
                                   constructPath(dataHome_, "fcitx5").c_str());
        dataDirs_ = defaultPaths("XDG_DATA_DIRS", "/usr/local/share:/usr/share",
                                 skipFcitxPath ? nullptr : "datadir");
        auto pkgdataDirFallback = dataDirs_;
        for (auto &path : pkgdataDirFallback) {
            path = constructPath(path, "fcitx5");
        }
        pkgdataDirs_ =
            defaultPaths("FCITX_DATA_DIRS",
                         stringutils::join(pkgdataDirFallback, ":").c_str(),
                         skipFcitxPath ? nullptr : "pkgdatadir");
        cacheHome_ = defaultPath("XDG_CACHE_HOME", ".cache");
        const char *tmpdir = getenv("TMPDIR");
        runtimeDir_ = defaultPath("XDG_RUNTIME_DIR",
                                  !tmpdir || !tmpdir[0] ? "/tmp" : tmpdir);
        addonDirs_ =
            defaultPaths("FCITX_ADDON_DIRS", FCITX_INSTALL_ADDONDIR, nullptr);
    }

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(StandardPathPrivate)

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
                if (!home) {
                    throw std::runtime_error("Home is not set");
                }
                dir = stringutils::joinPath(home, defaultPath);
            } else {
                if (strcmp(env, "XDG_RUNTIME_DIR") == 0) {
                    dir = stringutils::joinPath(
                        defaultPath,
                        stringutils::concat("fcitx-runtime-", geteuid()));
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
                (buf.st_mode & 0777) != S_IRWXU) {
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
        std::unordered_set<std::string> uniqueDirs(rawDirs.begin(),
                                                   rawDirs.end());

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
            return configHome_;
        case StandardPath::Type::PkgConfig:
            return pkgconfigHome_;
        case StandardPath::Type::Data:
            return dataHome_;
        case StandardPath::Type::PkgData:
            return pkgdataHome_;
        case StandardPath::Type::Cache:
            return cacheHome_;
            break;
        case StandardPath::Type::Runtime:
            return runtimeDir_;
        default:
            return {};
        }
    }

    std::vector<std::string> directories(StandardPath::Type type) const {
        switch (type) {
        case StandardPath::Type::Config:
            return configDirs_;
        case StandardPath::Type::PkgConfig:
            return pkgconfigDirs_;
        case StandardPath::Type::Data:
            return dataDirs_;
        case StandardPath::Type::PkgData:
            return pkgdataDirs_;
        case StandardPath::Type::Addon:
            return addonDirs_;
        default:
            return {};
        }
    }

    std::string configHome_;
    std::vector<std::string> configDirs_;
    std::string pkgconfigHome_;
    std::vector<std::string> pkgconfigDirs_;
    std::string dataHome_;
    std::vector<std::string> dataDirs_;
    std::string pkgdataHome_;
    std::vector<std::string> pkgdataDirs_;
    std::string cacheHome_;
    std::string runtimeDir_;
    std::vector<std::string> addonDirs_;
};

StandardPath::StandardPath(bool skipFcitxPath)
    : d_ptr(std::make_unique<StandardPathPrivate>(skipFcitxPath)) {}

StandardPath::~StandardPath() {}

const StandardPath &StandardPath::global() {
    const char *skip = getenv("SKIP_FCITX_PATH");
    bool skipFcitx = false;
    if (skip && skip[0] &&
        (strcmp(skip, "True") == 0 || strcmp(skip, "true") == 0 ||
         strcmp(skip, "1") == 0)) {
        skipFcitx = true;
    }
    static StandardPath globalPath(skipFcitx);
    return globalPath;
}

const char *StandardPath::fcitxPath(const char *path) {
    if (!path) {
        return nullptr;
    }

    static const std::unordered_map<std::string, std::string> pathMap = {
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

    auto iter = pathMap.find(path);
    if (iter != pathMap.end()) {
        return iter->second.c_str();
    }

    return nullptr;
}

std::string StandardPath::fcitxPath(const char *path, const char *subPath) {
    return stringutils::joinPath(fcitxPath(path), subPath);
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
    std::function<bool(const std::string &path, bool user)> scanner) const {
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
                       bool user)>
        scanner) const {
    auto scanDir = [scanner](const std::string &fullPath, bool isUser) {
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
    };
    if (isAbsolutePath(path)) {
        scanDir(path, false);
    } else {
        scanDirectories(
            type, [&path, &scanDir](const std::string &dirPath, bool isUser) {
                auto fullPath = constructPath(dirPath, path);
                return scanDir(fullPath, isUser);
            });
    }
}

std::string StandardPath::locate(Type type, const std::string &path) const {
    std::string retPath;
    if (isAbsolutePath(path)) {
        if (fs::isreg(path)) {
            retPath = path;
        }
    } else {
        scanDirectories(type,
                        [&retPath, &path](const std::string &dirPath, bool) {
                            auto fullPath = constructPath(dirPath, path);
                            if (!fs::isreg(fullPath)) {
                                return true;
                            }
                            retPath = fullPath;
                            return false;
                        });
    }
    return retPath;
}

std::vector<std::string>
StandardPath::locateAll(Type type, const std::string &path) const {
    std::vector<std::string> retPaths;
    if (isAbsolutePath(path)) {
        if (fs::isreg(path)) {
            retPaths.push_back(path);
        }
    } else {
        scanDirectories(type,
                        [&retPaths, &path](const std::string &dirPath, bool) {
                            auto fullPath = constructPath(dirPath, path);
                            if (fs::isreg(fullPath)) {
                                retPaths.push_back(fullPath);
                            }
                            return true;
                        });
    }
    return retPaths;
}

StandardPathFile StandardPath::open(Type type, const std::string &path,
                                    int flags) const {
    int retFD = -1;
    std::string fdPath;
    if (isAbsolutePath(path)) {
        int fd = ::open(path.c_str(), flags);
        if (fd >= 0) {
            retFD = fd;
            fdPath = path;
        }
    } else {
        scanDirectories(type, [flags, &retFD, &fdPath,
                               &path](const std::string &dirPath, bool) {
            auto fullPath = constructPath(dirPath, path);
            int fd = ::open(fullPath.c_str(), flags);
            if (fd < 0) {
                return true;
            }
            retFD = fd;
            fdPath = fullPath;
            return false;
        });
    }
    return {retFD, fdPath};
}

StandardPathFile StandardPath::openUser(Type type, const std::string &path,
                                        int flags) const {
    std::string fullPath;
    if (isAbsolutePath(path)) {
        fullPath = path;
    } else {
        auto dirPath = userDirectory(type);
        if (dirPath.empty()) {
            return {};
        }
        fullPath = constructPath(dirPath, path);
    }
    if (fs::makePath(fs::dirName(fullPath))) {
        int fd = ::open(fullPath.c_str(), flags, 0600);
        if (fd >= 0) {
            return {fd, fullPath};
        }
    }
    return {};
}

std::vector<StandardPathFile> StandardPath::openAll(StandardPath::Type type,
                                                    const std::string &path,
                                                    int flags) const {
    std::vector<StandardPathFile> result;
    if (isAbsolutePath(path)) {
        int fd = ::open(path.c_str(), flags);
        if (fd >= 0) {
            result.emplace_back(fd, path);
        }
    } else {
        scanDirectories(
            type, [flags, &result, &path](const std::string &dirPath, bool) {
                auto fullPath = constructPath(dirPath, path);
                int fd = ::open(fullPath.c_str(), flags);
                if (fd < 0) {
                    return true;
                }
                result.emplace_back(fd, fullPath);
                return true;
            });
    }
    return result;
}

StandardPathTempFile
StandardPath::openUserTemp(Type type, const std::string &pathOrig) const {
    std::string path = pathOrig + "_XXXXXX";
    std::string fullPath, fullPathOrig;
    if (isAbsolutePath(pathOrig)) {
        fullPath = path;
        fullPathOrig = pathOrig;
    } else {
        auto dirPath = userDirectory(type);
        if (dirPath.empty()) {
            return {};
        }
        fullPath = constructPath(dirPath, path);
        fullPathOrig = constructPath(dirPath, pathOrig);
    }
    if (fs::makePath(fs::dirName(fullPath))) {
        std::unique_ptr<char, decltype(&std::free)> cPath(
            strdup(fullPath.c_str()), &std::free);
        int fd = mkstemp(cPath.get());
        if (fd >= 0) {
            return {fd, fullPathOrig, cPath.get()};
        }
    }
    return {};
}

bool StandardPath::safeSave(Type type, const std::string &pathOrig,
                            std::function<bool(int)> callback) const {
    auto &standardPath = StandardPath::global();
    auto file = standardPath.openUserTemp(type, pathOrig);
    if (file.fd() < 0) {
        return false;
    }
    try {
        return callback(file.fd());
    } catch (const std::exception &) {
        file.removeTemp();
        return false;
    }
}

StandardPathFileMap StandardPath::multiOpenFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)>
        filter) const {
    StandardPathFileMap result;
    scanFiles(type, path,
              [&result, flags, &filter](const std::string &path,
                                        const std::string dir, bool isUser) {
                  if (!result.count(path) && filter(path, dir, isUser)) {
                      auto fullPath = constructPath(dir, path);
                      int fd = ::open(fullPath.c_str(), flags);
                      if (fd >= 0) {
                          result.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(path),
                                         std::forward_as_tuple(fd, fullPath));
                      }
                  }
                  return true;
              });

    return result;
}

StandardPathFilesMap StandardPath::multiOpenAllFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)>
        filter) const {
    StandardPathFilesMap result;
    scanFiles(type, path,
              [&result, flags, &filter](const std::string &path,
                                        const std::string dir, bool isUser) {
                  if (filter(path, dir, isUser)) {
                      auto fullPath = constructPath(dir, path);
                      int fd = ::open(fullPath.c_str(), flags);
                      if (fd >= 0) {
                          result[path].emplace_back(fd, fullPath);
                      }
                  }
                  return true;
              });

    return result;
}
} // namespace fcitx
