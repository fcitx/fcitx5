/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "standardpath.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "config.h"
#include "fs.h"
#include "macros.h"
#include "misc.h"
#include "misc_p.h"
#include "stringutils.h"

#ifdef _WIN32
#include <io.h>
#endif

#if __has_include(<paths.h>)
#include <paths.h>
#endif

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

namespace fcitx {

namespace {
bool isAbsolutePath(const std::string &path) {
    return !path.empty() && path[0] == '/';
}

std::string constructPath(const std::string &basepath,
                          const std::string &subpath) {
    if (basepath.empty()) {
        return {};
    }
    return fs::cleanPath(stringutils::joinPath(basepath, subpath));
}

} // namespace

StandardPathFile::~StandardPathFile() = default;

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
#ifdef _WIN32
        _commit(fd_.fd());
#else
        fsync(fd_.fd());
#endif
        fd_.reset();
        if (rename(tempPath_.c_str(), path_.c_str()) < 0) {
            unlink(tempPath_.c_str());
        }
    }
}

class StandardPathPrivate {
public:
    StandardPathPrivate(
        const std::string &packageName,
        const std::unordered_map<std::string, std::string> &builtInPathMap,
        bool skipBuiltInPath, bool skipUserPath)
        : skipBuiltInPath_(skipBuiltInPath), skipUserPath_(skipUserPath) {
        bool isFcitx = (packageName == "fcitx5");
        // initialize user directory
        configHome_ = defaultPath("XDG_CONFIG_HOME", ".config");
        pkgconfigHome_ =
            defaultPath((isFcitx ? "FCITX_CONFIG_HOME" : nullptr),
                        constructPath(configHome_, packageName).c_str());
        configDirs_ = defaultPaths("XDG_CONFIG_DIRS", "/etc/xdg",
                                   builtInPathMap, nullptr);
        auto pkgconfigDirFallback = configDirs_;
        for (auto &path : pkgconfigDirFallback) {
            path = constructPath(path, packageName);
        }
        pkgconfigDirs_ =
            defaultPaths((isFcitx ? "FCITX_CONFIG_DIRS" : nullptr),
                         stringutils::join(pkgconfigDirFallback, ":").c_str(),
                         builtInPathMap, nullptr);

        dataHome_ = defaultPath("XDG_DATA_HOME", ".local/share");
        pkgdataHome_ =
            defaultPath((isFcitx ? "FCITX_DATA_HOME" : nullptr),
                        constructPath(dataHome_, packageName).c_str());
        dataDirs_ = defaultPaths("XDG_DATA_DIRS", "/usr/local/share:/usr/share",
                                 builtInPathMap,
                                 skipBuiltInPath_ ? nullptr : "datadir");
        auto pkgdataDirFallback = dataDirs_;
        for (auto &path : pkgdataDirFallback) {
            path = constructPath(path, packageName);
        }
        pkgdataDirs_ = defaultPaths(
            (isFcitx ? "FCITX_DATA_DIRS" : nullptr),
            stringutils::join(pkgdataDirFallback, ":").c_str(), builtInPathMap,
            skipBuiltInPath_ ? nullptr : "pkgdatadir");
        cacheHome_ = defaultPath("XDG_CACHE_HOME", ".cache");
        const char *tmpdir = getenv("TMPDIR");
        runtimeDir_ = defaultPath("XDG_RUNTIME_DIR",
                                  !tmpdir || !tmpdir[0] ? "/tmp" : tmpdir);
        // Though theoratically, this is also fcitxPath, we just simply don't
        // use it here.
        addonDirs_ = defaultPaths("FCITX_ADDON_DIRS", FCITX_INSTALL_ADDONDIR,
                                  builtInPathMap, nullptr);

        syncUmask();
    }

    std::string userPath(StandardPath::Type type) const {
        if (skipUserPath_) {
            return {};
        }
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

    bool skipUser() const { return skipUserPath_; }
    bool skipBuiltIn() const { return skipBuiltInPath_; }

    void syncUmask() {
        // read umask, use 022 which is likely the default value, so less likely
        // to mess things up.
        mode_t old = ::umask(022);
        // restore
        ::umask(old);
        umask_.store(old, std::memory_order_relaxed);
    }

    mode_t umask() const { return umask_.load(std::memory_order_relaxed); }

private:
    // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    static std::string defaultPath(const char *env, const char *defaultPath) {
        char *cdir = nullptr;
        if (env) {
            cdir = getenv(env);
        }
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
                if (env && strcmp(env, "XDG_RUNTIME_DIR") == 0) {
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

        if (!dir.empty() && env && strcmp(env, "XDG_RUNTIME_DIR") == 0) {
            struct stat buf;
            if (stat(dir.c_str(), &buf) != 0 || buf.st_uid != geteuid() ||
                (buf.st_mode & 0777) != S_IRWXU) {
                return {};
            }
        }
        return dir;
    }

    static std::vector<std::string> defaultPaths(
        const char *env, const char *defaultPath,
        const std::unordered_map<std::string, std::string> &builtInPathMap,
        const char *builtInPathType) {
        std::vector<std::string> dirs;

        const char *dir = nullptr;
        if (env) {
            dir = getenv(env);
        }
        if (!dir) {
            dir = defaultPath;
        }

        auto rawDirs = stringutils::split(dir, ":");
        for (auto &rawDir : rawDirs) {
            rawDir = fs::cleanPath(rawDir);
        }
        std::unordered_set<std::string> uniqueDirs(rawDirs.begin(),
                                                   rawDirs.end());

        for (auto &s : rawDirs) {
            auto iter = uniqueDirs.find(s);
            if (iter != uniqueDirs.end()) {
                uniqueDirs.erase(iter);
                dirs.push_back(s);
            }
        }
        if (builtInPathType) {
            std::string builtInPath;
            if (const auto *value =
                    findValue(builtInPathMap, builtInPathType)) {
                builtInPath = *value;
            } else {
                builtInPath = StandardPath::fcitxPath(builtInPathType);
            }
            std::string path = fs::cleanPath(builtInPath);
            if (!path.empty() &&
                std::find(dirs.begin(), dirs.end(), path) == dirs.end()) {
                dirs.push_back(path);
            }
        }

        return dirs;
    }

    bool skipBuiltInPath_;
    bool skipUserPath_;
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
    std::atomic<mode_t> umask_;
};

StandardPath::StandardPath(
    const std::string &packageName,
    const std::unordered_map<std::string, std::string> &builtInPath,
    bool skipBuiltInPath, bool skipUserPath)
    : d_ptr(std::make_unique<StandardPathPrivate>(
          packageName, builtInPath, skipBuiltInPath, skipUserPath)) {}

StandardPath::StandardPath(bool skipFcitxPath, bool skipUserPath)
    : StandardPath("fcitx5", {}, skipFcitxPath, skipUserPath) {}

StandardPath::StandardPath(bool skipFcitxPath)
    : StandardPath(skipFcitxPath, false) {}

StandardPath::~StandardPath() = default;

const StandardPath &StandardPath::global() {
    bool skipFcitx = checkBoolEnvVar("SKIP_FCITX_PATH");
    bool skipUser = checkBoolEnvVar("SKIP_FCITX_USER_PATH");
    static StandardPath globalPath(skipFcitx, skipUser);
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
        std::make_pair<std::string, std::string>("libexecdir",
                                                 FCITX_INSTALL_LIBEXECDIR),
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
    const std::function<bool(const std::string &path, bool user)> &scanner)
    const {
    FCITX_D();
    std::string userDir = d->userPath(type);
    std::vector<std::string> list = d->directories(type);
    if (userDir.empty() && list.empty()) {
        return;
    }
    scanDirectories(userDir, list, scanner);
}

void StandardPath::scanDirectories(
    const std::string &userDir, const std::vector<std::string> &directories,
    const std::function<bool(const std::string &path, bool user)> &scanner)
    const {
    std::string_view userDirView(userDir);
    FCITX_D();
    if (d->skipUser()) {
        userDirView = "";
    }

    if (userDirView.empty() && directories.empty()) {
        return;
    }

    size_t len = (!userDirView.empty() ? 1 : 0) + directories.size();

    for (size_t i = 0; i < len; i++) {
        bool isUser = false;
        std::string dirBasePath;
        if (!userDirView.empty()) {
            isUser = (i == 0);
            dirBasePath = isUser ? userDirView : directories[i - 1];
        } else {
            dirBasePath = directories[i];
        }

        dirBasePath = fs::cleanPath(dirBasePath);
        if (!scanner(dirBasePath, isUser)) {
            return;
        }
    }
}

void StandardPath::scanFiles(
    Type type, const std::string &path,
    const std::function<bool(const std::string &fileName,
                             const std::string &dir, bool user)> &scanner)
    const {
    auto scanDir = [scanner](const std::string &fullPath, bool isUser) {
        UniqueCPtr<DIR, closedir> scopedDir{opendir(fullPath.c_str())};
        if (auto *dir = scopedDir.get()) {
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
                            std::string fullPath = constructPath(dirPath, path);
                            if (!fs::isreg(fullPath)) {
                                return true;
                            }
                            retPath = std::move(fullPath);
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
            fdPath = std::move(fullPath);
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

    // Try ensure directory exists if we want to write.
    if ((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
        if (!fs::makePath(fs::dirName(fullPath))) {
            return {};
        }
    }

    int fd = ::open(fullPath.c_str(), flags, 0600);
    if (fd >= 0) {
        return {fd, fullPath};
    }
    return {};
}

StandardPathFile StandardPath::openSystem(Type type, const std::string &path,
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
                               &path](const std::string &dirPath, bool user) {
            if (user) {
                return true;
            }
            auto fullPath = constructPath(dirPath, path);
            int fd = ::open(fullPath.c_str(), flags);
            if (fd < 0) {
                return true;
            }
            retFD = fd;
            fdPath = std::move(fullPath);
            return false;
        });
    }
    return {retFD, fdPath};
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
    std::string fullPath;
    std::string fullPathOrig;
    if (isAbsolutePath(pathOrig)) {
        fullPath = std::move(path);
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
        std::vector<char> cPath(fullPath.data(),
                                fullPath.data() + fullPath.size() + 1);
        int fd = mkstemp(cPath.data());
        if (fd >= 0) {
            return {fd, fullPathOrig, cPath.data()};
        }
    }
    return {};
}

bool StandardPath::safeSave(Type type, const std::string &pathOrig,
                            const std::function<bool(int)> &callback) const {
    FCITX_D();
    auto file = openUserTemp(type, pathOrig);
    if (!file.isValid()) {
        return false;
    }
    try {
        if (callback(file.fd())) {
#ifdef _WIN32
            auto wfile = utf8::UTF8ToUTF16(file.tempPath());
            ::_wchmod(wfile.data(), 0666 & ~(d->umask()));
#else
            // close it
            fchmod(file.fd(), 0666 & ~(d->umask()));
#endif
            return true;
        }
    } catch (const std::exception &) {
    }
    file.removeTemp();
    return false;
}

std::map<std::string, std::string> StandardPath::locateWithFilter(
    Type type, const std::string &path,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)>
        filter) const {
    std::map<std::string, std::string> result;
    scanFiles(type, path,
              [&result, &filter](const std::string &path,
                                 const std::string &dir, bool isUser) {
                  if (!result.count(path) && filter(path, dir, isUser)) {
                      auto fullPath = constructPath(dir, path);
                      if (fs::isreg(fullPath)) {
                          result.emplace(path, std::move(fullPath));
                      }
                  }
                  return true;
              });

    return result;
}

StandardPathFileMap StandardPath::multiOpenFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)>
        filter) const {
    StandardPathFileMap result;
    scanFiles(type, path,
              [&result, flags, &filter](const std::string &path,
                                        const std::string &dir, bool isUser) {
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
                                        const std::string &dir, bool isUser) {
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

int64_t StandardPath::timestamp(Type type, const std::string &path) const {
    if (isAbsolutePath(path)) {
        return fs::modifiedTime(path);
    }

    int64_t timestamp = 0;
    scanDirectories(
        type, [&timestamp, &path](const std::string &dirPath, bool) {
            auto fullPath = constructPath(dirPath, path);
            timestamp = std::max(timestamp, fs::modifiedTime(fullPath));
            return true;
        });
    return timestamp;
}

std::string StandardPath::findExecutable(const std::string &name) {
    if (name.empty()) {
        return "";
    }

    if (name[0] == '/') {
        return fs::isexe(name) ? name : "";
    }

    std::string sEnv;
    const char *pEnv = getenv("PATH");
    if (pEnv) {
        sEnv = pEnv;
    } else {
#if defined(_PATH_DEFPATH)
        sEnv = _PATH_DEFPATH;
#elif defined(_CS_PATH)
        size_t n = confstr(_CS_PATH, nullptr, 0);
        if (n) {
            std::vector<char> data;
            data.resize(n + 1);
            confstr(_CS_PATH, data.data(), data.size());
            data.push_back('\0');
            sEnv = data.data();
        }
#endif
    }
    auto paths = stringutils::split(sEnv, ":");
    for (auto &path : paths) {
        path = fs::cleanPath(path);
        auto fullPath = constructPath(path, name);
        if (!fullPath.empty() && fs::isexe(fullPath)) {
            return fullPath;
        }
    }
    return "";
}

bool StandardPath::hasExecutable(const std::string &name) {
    return !findExecutable(name).empty();
}

void StandardPath::syncUmask() const { d_ptr->syncUmask(); }

bool StandardPath::skipBuiltInPath() const {
    FCITX_D();
    return d->skipBuiltIn();
}

bool StandardPath::skipUserPath() const {
    FCITX_D();
    return d->skipUser();
}

} // namespace fcitx
