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
#include <ranges>
#include <span>
#include "config.h" // IWYU pragma: keep
#include "environ.h"
#include "fs.h"
#include "log.h"
#include "macros.h"
#include "misc.h"
#include "misc_p.h"
#include "stringutils.h"
#include "unixfd.h"

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

namespace fcitx {

namespace {

const std::filesystem::path constEmptyPath;
const std::vector<std::filesystem::path> constEmptyPaths = {
    std::filesystem::path()};

// A simple wrapper that return null if the variable name is nullptr.
std::optional<std::string> getEnvironmentNull(const char *env) {
    if (!env) {
        return std::nullopt;
    }
    return getEnvironment(env);
}

// Return the bottom symlink-ed path, otherwise, if error or not symlink file,
// return the original path.
std::filesystem::path symlinkTarget(const std::filesystem::path &path) {
    int maxDepth = 128;
    std::filesystem::path fullPath = path;
    std::error_code ec;
    while (--maxDepth && std::filesystem::is_symlink(fullPath, ec)) {
        auto linked = std::filesystem::read_symlink(fullPath, ec);
        if (!ec) {
            fullPath = linked;
        } else {
            return path;
        }
    }
    if (maxDepth > 0) {
        return fullPath;
    }
    return path;
}

constexpr std::string_view envListSeparator = isWindows() ? ";" : ":";

std::vector<std::filesystem::path> pathFromEnvironment() {
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
    auto paths = stringutils::split(sEnv, envListSeparator);
    std::vector<std::filesystem::path> result;
    result.reserve(paths.size());
    for (auto &path : paths) {
        result.push_back(path);
    }
    return result;
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
        pkgdataDirs_ = defaultPaths(
            (isFcitx ? "FCITX_DATA_HOME" : nullptr), dataDirs_[0] / packagePath,
            (isFcitx ? "FCITX_DATA_DIRS" : nullptr), pkgdataDirFallback,
            nullptr, builtInPathMap);
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

    std::span<const std::filesystem::path>
    directories(StandardPathsType type, StandardPathsModes modes) const {
        maybeUpdateModes(modes);
        const auto &directoriesByType =
            [type, this]() -> const std::vector<std::filesystem::path> & {
            switch (type) {
            case StandardPathsType::Config:
                return configDirs_;
            case StandardPathsType::PkgConfig:
                return pkgconfigDirs_;
            case StandardPathsType::Data:
                return dataDirs_;
            case StandardPathsType::PkgData:
                return pkgdataDirs_;
            case StandardPathsType::Addon:
                return addonDirs_;
            default:
                return constEmptyPaths;
            }
        }();
        assert(directoriesByType.size() >= 1);
        size_t from =
            modes.test(StandardPathsMode::User) && !directoriesByType[0].empty()
                ? 0
                : 1;
        size_t to = modes.test(StandardPathsMode::System)
                        ? directoriesByType.size()
                        : 1;
        std::span<const std::filesystem::path> dirs(directoriesByType);
        dirs = dirs.subspan(from, to - from);
        return dirs;
    }

    template <typename Callback>
    void
    scanDirectories(StandardPathsType type, const std::filesystem::path &path,
                    StandardPathsModes modes, const Callback &callback) const {
        std::span<const std::filesystem::path> dirs =
            path.is_absolute() ? constEmptyPaths : directories(type, modes);
        for (const auto &dir : dirs) {
            if (!callback(dir / path)) {
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

    std::tuple<UnixFD, std::filesystem::path, std::filesystem::path>
    openUserTemp(StandardPathsType type,
                 const std::filesystem::path &pathOrig) const {
        if (!pathOrig.has_filename() || skipUserPath_) {
            return {};
        }
        std::filesystem::path fullPathOrig;
        if (pathOrig.is_absolute()) {
            fullPathOrig = pathOrig;
        } else {
            const auto &dirPaths = directories(type, StandardPathsMode::User);
            if (dirPaths.empty() || dirPaths[0].empty()) {
                return {};
            }
            fullPathOrig = dirPaths[0] / pathOrig;
        }

        fullPathOrig = symlinkTarget(fullPathOrig);

        std::filesystem::path fullPath = fullPathOrig;
        fullPath += "_XXXXXX";
        if (fs::makePath(fullPath.parent_path())) {
            std::string fullPathStr = fullPath.string();
            std::vector<char> cPath(fullPathStr.c_str(),
                                    fullPathStr.c_str() + fullPathStr.size() +
                                        1);
            int fd = mkstemp(cPath.data());
            if (fd >= 0) {
                return {UnixFD::own(fd), std::filesystem::path(cPath.data()),
                        fullPathOrig};
            }
        }
        return {};
    }

private:
    // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    std::vector<std::filesystem::path>
    defaultPaths(const char *homeEnv, const std::filesystem::path &homeFallback,
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
        homeDir = homeDir.lexically_normal();

        std::vector<std::filesystem::path> dirs;
        dirs.push_back(std::move(homeDir));

        if (const auto dir = getEnvironmentNull(systemEnv)) {
            const auto rawDirs = stringutils::split(*dir, ":");
            std::ranges::transform(
                rawDirs, std::back_inserter(dirs), [](const auto &s) {
                    return std::filesystem::path(s).lexically_normal();
                });
        } else {
            std::ranges::copy(systemFallback, std::back_inserter(dirs));
        }

        if (builtInPathType && !skipBuiltInPath_) {
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

        for (auto &dir : dirs) {
            // Remove trailing slash, if present.
            if (!dir.has_filename()) {
                dir = dir.parent_path();
            }
        }

        std::unordered_set<std::filesystem::path> seen;
        std::erase_if(dirs, [&seen](const auto &dir) {
            if (seen.contains(dir)) {
                return true;
            }
            seen.insert(dir);
            return false;
        });

        return dirs;
    }

    void maybeUpdateModes(StandardPathsModes &modes) const {
        if (skipUserPath_) {
            modes = modes.unset(StandardPathsMode::User);
        }
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
};

StandardPaths::StandardPaths(
    const std::string &packageName,
    const std::unordered_map<std::string, std::filesystem::path> &builtInPath,
    bool skipBuiltInPath, bool skipUserPath)
    : d_ptr(std::make_unique<StandardPathsPrivate>(
          packageName, builtInPath, skipBuiltInPath, skipUserPath)) {}

StandardPaths::~StandardPaths() = default;

const StandardPaths &StandardPaths::global() {
    static bool skipFcitx = checkBoolEnvVar("SKIP_FCITX_PATH");
    static bool skipUser = checkBoolEnvVar("SKIP_FCITX_USER_PATH");
    static StandardPaths globalPath("fcitx5", {}, skipFcitx, skipUser);
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

    if (const auto *p = findValue(pathMap, path)) {
        return *p / subPath;
    }

    return {};
}

std::filesystem::path
StandardPaths::findExecutable(const std::filesystem::path &name) {
    if (name.is_absolute()) {
        return fs::isexe(name.string()) ? name : std::filesystem::path();
    }

    for (const auto &path : pathFromEnvironment()) {
        auto fullPath = path / name;
        if (fs::isexe(fullPath.string())) {
            return fullPath;
        }
    }
    return {};
}

bool StandardPaths::hasExecutable(const std::filesystem::path &name) {
    return !findExecutable(name).empty();
}

const std::filesystem::path &
StandardPaths::userDirectory(StandardPathsType type) const {
    FCITX_D();
    if (d->skipUser()) {
        return constEmptyPath;
    }
    auto dirs = d->directories(type, StandardPathsMode::User);
    return dirs.empty() ? constEmptyPath : dirs[0];
}

std::span<const std::filesystem::path>
StandardPaths::directories(StandardPathsType type,
                           StandardPathsModes modes) const {
    FCITX_D();
    return d->directories(type, modes);
}

std::filesystem::path StandardPaths::locate(StandardPathsType type,
                                            const std::filesystem::path &path,
                                            StandardPathsModes modes) const {
    FCITX_D();
    std::filesystem::path retPath;
    d->scanDirectories(type, path, modes,
                       [&retPath](const std::filesystem::path &fullPath) {
                           std::error_code ec;
                           if (!std::filesystem::exists(fullPath, ec)) {
                               return true;
                           }
                           retPath = fullPath;
                           return false;
                       });
    return retPath;
}

std::vector<std::filesystem::path>
StandardPaths::locateAll(StandardPathsType type,
                         const std::filesystem::path &path,
                         StandardPathsModes modes) const {
    FCITX_D();
    std::vector<std::filesystem::path> retPaths;
    d->scanDirectories(type, path, modes,
                       [&retPaths](std::filesystem::path fullPath) {
                           std::error_code ec;
                           if (!std::filesystem::exists(fullPath, ec)) {
                               return true;
                           }
                           retPaths.push_back(std::move(fullPath));
                           return true;
                       });
    return retPaths;
}

std::map<std::filesystem::path, std::filesystem::path>
StandardPaths::locate(StandardPathsType type, const std::filesystem::path &path,
                      const StandardPathsFilterCallback &callback,
                      StandardPathsModes modes) const {
    FCITX_D();
    std::map<std::filesystem::path, std::filesystem::path> retPath;
    d->scanDirectories(
        type, path, modes,
        [&retPath, &callback](const std::filesystem::path &fullPath) {
            std::error_code ec;
            for (auto directoryIterator =
                     std::filesystem::directory_iterator(fullPath, ec);
                 directoryIterator != std::filesystem::directory_iterator();
                 directoryIterator.increment(ec)) {
                if (ec) {
                    return true;
                }
                if (retPath.contains(directoryIterator->path().filename())) {
                    continue;
                }
                if (callback(directoryIterator->path())) {
                    retPath[directoryIterator->path().filename()] =
                        directoryIterator->path();
                }
            }
            return true;
        });
    return retPath;
}

UnixFD StandardPaths::open(StandardPathsType type,
                           const std::filesystem::path &path,
                           StandardPathsModes modes,
                           std::filesystem::path *outPath) const {
    FCITX_D();
    UnixFD retFD;
    d->scanDirectories(type, path, modes,
                       [&retFD, outPath](std::filesystem::path fullPath) {
                           retFD = openPath(fullPath);
                           if (!retFD.isValid()) {
                               return true;
                           }
                           if (outPath) {
                               *outPath = std::move(fullPath);
                           }
                           return false;
                       });
    return retFD;
}

UnixFD StandardPaths::openPath(const std::filesystem::path &path) {
#ifdef _WIN32
    return UnixFD::own(::_wopen(path.c_str(), O_RDONLY));
#else
    return UnixFD::own(::open(path.c_str(), O_RDONLY));
#endif
}

std::vector<UnixFD>
StandardPaths::openAll(StandardPathsType type,
                       const std::filesystem::path &path,
                       StandardPathsModes modes,
                       std::vector<std::filesystem::path> *outPaths) const {
    FCITX_D();
    std::vector<UnixFD> retFDs;
    if (outPaths) {
        outPaths->clear();
    }
    d->scanDirectories(type, path, modes,
                       [&retFDs, outPaths](std::filesystem::path fullPath) {
                           UnixFD fd = StandardPaths::openPath(fullPath);
                           if (!fd.isValid()) {
                               return true;
                           }
                           retFDs.push_back(std::move(fd));
                           if (outPaths) {
                               outPaths->push_back(std::move(fullPath));
                           }
                           return true;
                       });
    return retFDs;
}

bool StandardPaths::safeSave(StandardPathsType type,
                             const std::filesystem::path &pathOrig,
                             const std::function<bool(int)> &callback) const {
    FCITX_D();
    auto [file, path, fullPathOrig] = d->openUserTemp(type, pathOrig);
    if (!file.isValid()) {
        return false;
    }
    try {
        if (callback(file.fd())) {
            // sync first.
#ifdef _WIN32
            ::_wchmod(path.c_str(), 0666 & ~(d->umask()));
            _commit(file.fd());
#else
            // close it
            fchmod(file.fd(), 0666 & ~(d->umask()));
            fsync(file.fd());
#endif
            file.reset();
            std::filesystem::rename(path, fullPathOrig);
            return true;
        }
    } catch (const std::exception &e) {
        FCITX_ERROR() << "Failed to write file: " << fullPathOrig << e.what();
    }
#ifdef _WIN32
    _wunlink(path.c_str());
#else
    unlink(path.c_str());
#endif
    return false;
}

int64_t StandardPaths::timestamp(StandardPathsType type,
                                 const std::filesystem::path &path,
                                 StandardPathsModes modes) const {
    FCITX_D();

    int64_t timestamp = 0;
    d->scanDirectories(type, path, modes,
                       [&timestamp](const std::filesystem::path &fullPath) {
                           const auto time = fs::modifiedTime(fullPath);
                           timestamp = std::max(timestamp, time);
                           return true;
                       });

    return timestamp;
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
