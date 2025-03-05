/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "standardpaths.h"
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
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <format>
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/stringutils.h"
#include "config.h" // IWYU pragma: keep
#include "environ.h"
#include "fs.h"
#include "macros.h"
#include "misc.h"

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

namespace fcitx {

class StandardPathsPrivate {
public:
    StandardPathsPrivate(
        const std::string &packageName,
        const std::unordered_map<std::string, std::filesystem::path>
            &builtInPathMap,
        bool skipBuiltInPath, bool skipUserPath)
        : skipBuiltInPath_(skipBuiltInPath), skipUserPath_(skipUserPath) {
        bool isFcitx = (packageName == "fcitx5");
        std::filesystem::path packagePath =
            std::u8string(packageName.begin(), packageName.end());
        // initialize user directory
        configHome_ = defaultPath("XDG_CONFIG_HOME", ".config");
        pkgconfigHome_ = defaultPath((isFcitx ? "FCITX_CONFIG_HOME" : nullptr),
                                     configHome_ / packagePath);
        configDirs_ = defaultPaths("XDG_CONFIG_DIRS", {"/etc/xdg"},
                                   builtInPathMap, nullptr);
        auto pkgconfigDirFallback = configDirs_;
        for (auto &path : pkgconfigDirFallback) {
            path = path / packagePath;
        }
        pkgconfigDirs_ =
            defaultPaths((isFcitx ? "FCITX_CONFIG_DIRS" : nullptr),
                         pkgconfigDirFallback, builtInPathMap, nullptr);

        dataHome_ = defaultPath("XDG_DATA_HOME", ".local/share");
        pkgdataHome_ = defaultPath((isFcitx ? "FCITX_DATA_HOME" : nullptr),
                                   dataHome_ / packageName);
        dataDirs_ = defaultPaths(
            "XDG_DATA_DIRS", {"/usr/local/share", "/usr/share"}, builtInPathMap,
            skipBuiltInPath_ ? nullptr : "datadir");
        auto pkgdataDirFallback = dataDirs_;
        for (auto &path : pkgdataDirFallback) {
            path = path / packageName;
        }
        pkgdataDirs_ = defaultPaths((isFcitx ? "FCITX_DATA_DIRS" : nullptr),
                                    pkgdataDirFallback, builtInPathMap,
                                    skipBuiltInPath_ ? nullptr : "pkgdatadir");
        cacheHome_ = defaultPath("XDG_CACHE_HOME", ".cache");
        std::error_code ec;
        auto tmpdir = std::filesystem::temp_directory_path(ec);
        runtimeDir_ = defaultPath("XDG_RUNTIME_DIR",
                                  tmpdir.empty() ? std::filesystem::path("/tmp")
                                                 : tmpdir);
        // Though theoratically, this is also fcitxPath, we just simply don't
        // use it here.
        addonDirs_ = defaultPaths("FCITX_ADDON_DIRS", {FCITX_INSTALL_ADDONDIR},
                                  builtInPathMap, nullptr);

        syncUmask();
    }

    const std::filesystem::path &userPath(StandardPaths::Type type) const {
        if (skipUserPath_) {
            return emptyPath_;
        }
        switch (type) {
        case StandardPaths::Type::Config:
            return configHome_;
        case StandardPaths::Type::PkgConfig:
            return pkgconfigHome_;
        case StandardPaths::Type::Data:
            return dataHome_;
        case StandardPaths::Type::PkgData:
            return pkgdataHome_;
        case StandardPaths::Type::Cache:
            return cacheHome_;
        case StandardPaths::Type::Runtime:
            return runtimeDir_;
        default:
            return emptyPath_;
        }
    }

    std::vector<std::filesystem::path>
    directories(StandardPaths::Type type) const {
        switch (type) {
        case StandardPaths::Type::Config:
            return configDirs_;
        case StandardPaths::Type::PkgConfig:
            return pkgconfigDirs_;
        case StandardPaths::Type::Data:
            return dataDirs_;
        case StandardPaths::Type::PkgData:
            return pkgdataDirs_;
        case StandardPaths::Type::Addon:
            return addonDirs_;
        default:
            return {};
        }
    }

    template <typename Callback>
    void scanDirectories(StandardPaths::Type type, Modes modes,
                         Callback callback) const {
        auto user =
            modes.test(StandardPaths::Mode::User) ? {userPath(type)} : {};
        for (const auto &dir : directories(type)) {
            if (!callback(dir, false)) {
                return;
            }
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
    static std::filesystem::path
    defaultPath(const char *env, const std::filesystem::path &defaultPath) {
        std::optional<std::string> cdir;
        if (env) {
            cdir = getEnvironment(env);
        }
        std::filesystem::path dir;
        if (cdir && !cdir->empty()) {
            dir = *cdir;
        } else {
            // caller need to ensure HOME is not empty;
            if (!defaultPath.is_absolute()) {
                auto home = getEnvironment("HOME");
                if (!home) {
                    throw std::runtime_error("Home is not set");
                }
                dir = *home / defaultPath;
            } else {
                if (env && strcmp(env, "XDG_RUNTIME_DIR") == 0) {
                    dir = defaultPath /
                          std::format("fcitx-runtime-{}", geteuid());
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

    static std::vector<std::filesystem::path>
    defaultPaths(const char *env,
                 const std::vector<std::filesystem::path> &defaultPath,
                 const std::unordered_map<std::string, std::filesystem::path>
                     &builtInPathMap,
                 const char *builtInPathType) {
        std::vector<std::filesystem::path> dirs;

        if (const auto dir = env ? getEnvironment(env) : std::nullopt) {
            const auto rawDirs = stringutils::split(*dir, ":");
            std::ranges::transform(
                rawDirs, std::back_inserter(dirs), [](const auto &s) {
                    return std::filesystem::path(s).lexically_normal();
                });
        } else {
            dirs = defaultPath;
        }

        std::unordered_set<std::filesystem::path> dirSet(dirs.begin(),
                                                         dirs.end());

        std::vector<std::filesystem::path> uniqueDirs;

        for (auto &s : dirs) {
            if (dirSet.erase(s)) {
                uniqueDirs.push_back(s);
            }
        }
        if (builtInPathType) {
            std::filesystem::path builtInPath;
            if (const auto *value =
                    findValue(builtInPathMap, builtInPathType)) {
                builtInPath = *value;
            } else {
                builtInPath = StandardPaths::fcitxPath(builtInPathType);
            }
            const auto path = builtInPath.lexically_normal();
            if (!path.empty() && std::ranges::find(dirs, path) == dirs.end()) {
                dirs.push_back(path);
            }
        }

        return dirs;
    }

    bool skipBuiltInPath_;
    bool skipUserPath_;
    std::filesystem::path configHome_;
    std::vector<std::filesystem::path> configDirs_;
    std::filesystem::path pkgconfigHome_;
    std::vector<std::filesystem::path> pkgconfigDirs_;
    std::filesystem::path dataHome_;
    std::vector<std::filesystem::path> dataDirs_;
    std::filesystem::path pkgdataHome_;
    std::vector<std::filesystem::path> pkgdataDirs_;
    std::filesystem::path cacheHome_;
    std::filesystem::path runtimeDir_;
    std::vector<std::filesystem::path> addonDirs_;
    std::atomic<mode_t> umask_;
    static const inline std::filesystem::path emptyPath_;
};

StandardPaths::StandardPaths(
    const std::string &packageName,
    const std::unordered_map<std::string, std::string> &builtInPath,
    bool skipBuiltInPath, bool skipUserPath)
    : d_ptr(std::make_unique<StandardPathsPrivate>(
          packageName, builtInPath, skipBuiltInPath, skipUserPath)) {}

StandardPaths::~StandardPaths() = default;

const StandardPaths &StandardPaths::global() {
    bool skipFcitx = checkBoolEnvVar("SKIP_FCITX_PATH");
    bool skipUser = checkBoolEnvVar("SKIP_FCITX_USER_PATH");
    static StandardPaths globalPath("fcitx", {}, skipFcitx, skipUser);
    return globalPath;
}

std::filesystem::path
StandardPaths::fcitxPath(const char *path,
                         const std::filesystem::path &subPath) {
    if (!path) {
        return {};
    }

    static const std::unordered_map<std::string, std::filesystem::path>
        pathMap = {
            std::make_pair<std::string, std::filesystem::path>(
                "datadir", FCITX_INSTALL_DATADIR),
            std::make_pair<std::string, std::filesystem::path>(
                "pkgdatadir", FCITX_INSTALL_PKGDATADIR),
            std::make_pair<std::string, std::filesystem::path>(
                "libdir", FCITX_INSTALL_LIBDIR),
            std::make_pair<std::string, std::filesystem::path>(
                "bindir", FCITX_INSTALL_BINDIR),
            std::make_pair<std::string, std::filesystem::path>(
                "localedir", FCITX_INSTALL_LOCALEDIR),
            std::make_pair<std::string, std::filesystem::path>(
                "addondir", FCITX_INSTALL_ADDONDIR),
            std::make_pair<std::string, std::filesystem::path>(
                "libdatadir", FCITX_INSTALL_LIBDATADIR),
            std::make_pair<std::string, std::filesystem::path>(
                "libexecdir", FCITX_INSTALL_LIBEXECDIR),
        };

    if (auto p = findValue(pathMap, path)) {
        return *p / subPath;
    }

    return {};
}

std::filesystem::path StandardPaths::userDirectory(Type type) const {
    FCITX_D();
    return d->userPath(type);
}

std::vector<std::filesystem::path> StandardPaths::directories(Type type) const {
    FCITX_D();
    return d->directories(type);
}

std::filesystem::path StandardPaths::locate(Type type,
                                            const std::filesystem::path &path,
                                            Modes modes) const {
    std::string retPath;
    std::error_code ec;
    if (path.is_absolute()) {
        if (std::filesystem::is_regular_file(path, ec)) {
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

std::vector<std::filesystem::path>
StandardPaths::locateAll(Type type, const std::filesystem::path &path) const {
    std::vector<std::string> retPaths;
    std::error_code ec;
    if (path.is_absolute()) {
        if (std::filesystem::is_regular_file(path, ec)) {
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

UnixFD StandardPaths::open(Type type, const std::filesystem::path &path,
                           int flags, Modes modes,
                           std::filesystem::path *outPath) const {
    FCITX_D();
    UnixFD retFD;
    if (path.is_absolute()) {
        retFD.give(::open(path.c_str(), flags));
        if (retFD.isValid() && outPath) {
            *outPath = path;
        }
    } else {
        for (const auto &directory : d->directories(type, modes)) {
        }
        scanDirectories(type, [flags, &retFD, outPath,
                               &path](const std::string &dirPath, bool) {
            auto fullPath = constructPath(dirPath, path);
            retFD.give(::open(fullPath.c_str(), flags));
            if (!retFD.isValid()) {
                return true;
            }
            if (outPath) {
                *outPath = std::move(fullPath);
            }
            return false;
        });
    }
    return retFD;
}

UnixFD StandardPaths::openUser(Type type, const std::filesystem::path &path,
                               int flags,
                               std::filesystem::path *outPath = nullptr) const {
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

StandardPathsFile StandardPaths::openSystem(Type type, const std::string &path,
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

std::vector<StandardPathsFile> StandardPaths::openAll(StandardPaths::Type type,
                                                      const std::string &path,
                                                      int flags) const {
    std::vector<StandardPathsFile> result;
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

StandardPathsTempFile
StandardPaths::openUserTemp(Type type, const std::string &pathOrig) const {
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

bool StandardPaths::safeSave(Type type, const std::string &pathOrig,
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

std::map<std::string, std::string> StandardPaths::locateWithFilter(
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

StandardPathsFileMap StandardPaths::multiOpenFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)>
        filter) const {
    StandardPathsFileMap result;
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

StandardPathsFilesMap StandardPaths::multiOpenAllFilter(
    Type type, const std::string &path, int flags,
    std::function<bool(const std::string &path, const std::string &dir,
                       bool user)>
        filter) const {
    StandardPathsFilesMap result;
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

int64_t StandardPaths::timestamp(Type type, const std::string &path) const {
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

std::string StandardPaths::findExecutable(const std::string &name) {
    if (name.empty()) {
        return "";
    }

    if (name[0] == '/') {
        return fs::isexe(name) ? name : "";
    }

    std::string sEnv;
    if (auto pEnv = getEnvironment("PATH")) {
        sEnv = std::move(*pEnv);
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

bool StandardPaths::hasExecutable(const std::string &name) {
    return !findExecutable(name).empty();
}

void StandardPaths::syncUmask() const { d_ptr->syncUmask(); }

bool StandardPaths::skipBuiltInPath() const {
    FCITX_D();
    return d->skipBuiltIn();
}

bool StandardPaths::skipUserPath() const {
    FCITX_D();
    return d->skipUser();
}

} // namespace fcitx
