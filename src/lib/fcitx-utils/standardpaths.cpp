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
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <ranges>
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/stringutils.h"
#include "config.h" // IWYU pragma: keep
#include "environ.h"
#include "fs.h"
#include "macros.h"

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

namespace fcitx {

namespace {

// A simple wrapper that return null if then variable name is nullptr.
std::optional<std::string> getEnvironmentNull(const char *env) {
    if (!env) {
        return std::nullopt;
    }
    return getEnvironment(env);
}

} // namespace

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
        configDirs_ =
            defaultPaths("XDG_CONFIG_HOME", ".config", "XDG_CONFIG_DIRS",
                         {"/etc/xdg"}, nullptr, builtInPathMap);
        std::vector<std::filesystem::path> pkgconfigDirFallback;
        std::ranges::copy(
            configDirs_ | std::views::drop(1) |
                std::views::transform([&packagePath](const auto &dir) {
                    return dir / packagePath;
                }),
            std::back_inserter(pkgconfigDirFallback));
        pkgconfigDirs_ =
            defaultPaths((isFcitx ? "FCITX_CONFIG_HOME" : nullptr),
                         configDirs_[0] / packagePath,
                         (isFcitx ? "FCITX_CONFIG_DIRS" : nullptr),
                         pkgconfigDirFallback, nullptr, builtInPathMap);

        dataDirs_ = defaultPaths(
            "XDG_DATA_HOME", ".local/share", "XDG_DATA_DIRS",
            {"/usr/local/share", "/usr/share"},
            (skipBuiltInPath_ ? nullptr : "datadir"), builtInPathMap);
        std::vector<std::filesystem::path> pkgdataDirFallback;
        std::ranges::copy(
            dataDirs_ | std::views::drop(1) |
                std::views::transform([&packagePath](const auto &dir) {
                    return dir / packagePath;
                }),
            std::back_inserter(pkgdataDirFallback));
        dataDirs_ = defaultPaths((isFcitx ? "FCITX_DATA_HOME" : nullptr),
                                 dataDirs_[0] / packagePath,
                                 (isFcitx ? "FCITX_DATA_DIRS" : nullptr),
                                 pkgdataDirFallback, nullptr, builtInPathMap);
        cacheDir_ = defaultPaths("XDG_CACHE_HOME", ".cache");
        assert(cacheDir_.size() == 1);
        std::error_code ec;
        auto tmpdir = std::filesystem::temp_directory_path(ec);
        runtimeDir_ = defaultPaths(
            "XDG_RUNTIME_DIR",
            tmpdir.empty() ? std::filesystem::path("/tmp") : tmpdir);
        assert(runtimeDir_.size() == 1);
        // Though theoratically, this is also fcitxPath, we just simply don't
        // use it here.
        addonDirs_ =
            defaultPaths(nullptr, {}, "FCITX_ADDON_DIRS",
                         {FCITX_INSTALL_ADDONDIR}, nullptr, builtInPathMap);

        syncUmask();
    }

    const std::vector<std::filesystem::path> &
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
            return emptyPaths_;
        }
    }

    template <typename Callback>
    void scanDirectories(StandardPaths::Type type, StandardPaths::Modes modes,
                         Callback callback) const {
        const auto &dirs = directories(type);
        size_t from = modes.test(StandardPaths::Mode::User) ? 0 : 1;
        size_t to = modes.test(StandardPaths::Mode::System) ? dirs.size() : 1;

        for (size_t i = from; i < to; i++) {
            if (!callback(dirs[i], i == 0)) {
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

    UnixFD openUserTemp(Type type, const std::string &pathOrig) const {
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

private:
    // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    static std::vector<std::filesystem::path>
    defaultPaths(const char *homeEnv, std::filesystem::path homeFallback,
                 const char *systemEnv = nullptr,
                 const std::vector<std::filesystem::path> &systemFallback = {},
                 const char *builtInPathType = nullptr,
                 const std::unordered_map<std::string, std::filesystem::path>
                     &builtInPathMap = {}) {

        const auto homeVar = getEnvironmentNull(homeEnv);
        std::filesystem::path homeDir;
        if (homeVar && !homeVar->empty()) {
            homeDir = *homeVar;
        } else if (!homeFallback.is_absolute() && !homeFallback.empty()) {
            // caller need to ensure HOME is not empty;
            auto home = getEnvironment("HOME");
            if (!home) {
                throw std::runtime_error("Home is not set");
            }
            homeDir = *home / homeFallback;
        } else {
            homeDir = homeFallback;
        }

        std::vector<std::filesystem::path> dirs;

        if (const auto dir =
                systemEnv ? getEnvironment(systemEnv) : std::nullopt) {
            const auto rawDirs = stringutils::split(*dir, ":");
            std::ranges::transform(
                rawDirs, std::back_inserter(dirs), [](const auto &s) {
                    return std::filesystem::path(s).lexically_normal();
                });
        } else {
            std::ranges::copy(systemFallback, std::back_inserter(dirs));
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
            if (!path.empty()) {
                dirs.push_back(path);
            }
        }

        std::unordered_set<std::filesystem::path> seen;
        std::erase_if(dirs, [&seen](const auto &dir) {
            if (seen.contains(dir)) {
                return true;
            } else {
                seen.insert(dir);
                return false;
            }
        });

        return dirs;
    }

    bool skipBuiltInPath_;
    bool skipUserPath_;
    std::vector<std::filesystem::path> configDirs_;
    std::vector<std::filesystem::path> pkgconfigDirs_;
    std::vector<std::filesystem::path> dataDirs_;
    std::vector<std::filesystem::path> pkgdataDirs_;
    std::vector<std::filesystem::path> cacheDir_;
    std::vector<std::filesystem::path> runtimeDir_;
    std::vector<std::filesystem::path> addonDirs_;
    std::atomic<mode_t> umask_;
    static const inline std::filesystem::path emptyPath_;
    static const inline std::vector<std::filesystem::path> emptyPaths_;
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
    return d->directories(type)[0];
}

std::vector<std::filesystem::path> StandardPaths::directories(Type type) const {
    FCITX_D();
    return d->directories(type);
}

std::filesystem::path StandardPaths::locate(Type type,
                                            const std::filesystem::path &path,
                                            Modes modes) const {
    FCITX_D();
    std::string retPath;
    std::error_code ec;
    if (path.is_absolute()) {
        if (std::filesystem::is_regular_file(path, ec)) {
            retPath = path;
        }
    } else {
        d->scanDirectories(
            type, modes,
            [&retPath, &path](const std::filesystem::path &dirPath, bool) {
                std::string fullPath = dirPath / path;
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
    FCITX_D();
    std::vector<std::filesystem::path> retPaths;
    std::error_code ec;
    if (path.is_absolute()) {
        if (std::filesystem::is_regular_file(path, ec)) {
            retPaths.push_back(path);
        }
    } else {
        d->scanDirectories(
            type, {Mode::User, Mode::System},
            [&retPaths, &path](const std::filesystem::path &dirPath, bool) {
                auto fullPath = dirPath / path;
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
        d->scanDirectories(
            type, modes,
            [flags, &retFD, outPath, &path](const std::string &dirPath, bool) {
                auto fullPath = dirPath / path;
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
    std::filesystem::path fullPath;
    if (path.is_absolute()) {
        fullPath = path;
    } else {
        auto dirPath = userDirectory(type);
        if (dirPath.empty()) {
            return {};
        }
        fullPath = dirPath / path;
    }

    // Try ensure directory exists if we want to write.
    if ((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
        if (!fs::makePath(fs::dirName(fullPath))) {
            return {};
        }
    }

    UnixFD fd = UnixFD::own(::open(fullPath.c_str(), flags, 0600));
    if (fd.isValid() && outPath) {
        *outPath = std::move(fullPath);
    }
    return fd;
}

bool StandardPaths::safeSave(Type type, const std::string &pathOrig,
                             const std::function<bool(int)> &callback) const {
    FCITX_D();
    UnixFD file = openUserTemp(type, pathOrig);
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

int64_t StandardPaths::timestamp(Type type, const std::filesystem::path &path,
                                 Modes modes) const {
    FCITX_D();
    std::error_code ec;
    if (path.is_absolute()) {
        const auto time = std::filesystem::last_write_time(path, ec);
        if (ec) {
            return 0;
        }
        auto timeMs =
            std::chrono::time_point_cast<std::chrono::microseconds>(time);
        return time.time_since_epoch().count();
    }

    int64_t timestamp = 0;
    d->scanDirectories(
        type, modes,
        [&timestamp, &path](const std::filesystem::path &dirPath, bool) {
            auto fullPath = dirPath / path;
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
