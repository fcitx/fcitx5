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

    const std::vector<std::filesystem::path> &
    directories(StandardPathsType type) const {
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
            return emptyPaths_;
        }
    }

    template <typename Callback>
    void scanDirectories(StandardPathsType type,
                         const std::filesystem::path &path,
                         StandardPathsModes modes,
                         const Callback &callback) const {
        maybeUpdateModes(modes);
        std::span<const std::filesystem::path> dirs =
            path.is_absolute() ? emptyPaths_ : directories(type);
        size_t from = modes.test(StandardPathsMode::User) ? 0 : 1;
        size_t to = modes.test(StandardPathsMode::System) ? dirs.size() : 1;
        dirs = dirs.subspan(from, to - from);
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

    std::tuple<UnixFD, std::filesystem::path>
    openUserTemp(StandardPathsType type,
                 const std::filesystem::path &pathOrig) const {
        if (!pathOrig.has_filename() || skipUserPath_) {
            return {};
        }
        std::string fullPathOrig;
        if (pathOrig.is_absolute()) {
            fullPathOrig = pathOrig;
        } else {
            const auto &dirPaths = directories(type);
            if (dirPaths.empty() || dirPaths[0].empty()) {
                return {};
            }
            fullPathOrig = dirPaths[0] / pathOrig;
            if (std::filesystem::exists(fullPathOrig)) {
                std::error_code ec;
                auto linked = std::filesystem::read_symlink(fullPathOrig, ec);
                if (!ec) {
                    fullPathOrig = linked;
                }
            }
        }
        std::filesystem::path fullPath = fullPathOrig;
        fullPath += "_XXXXXX";
        if (fs::makePath(fullPath.parent_path())) {
            std::vector<char> cPath(fullPath.c_str(),
                                    fullPath.c_str() +
                                        fullPath.native().size() + 1);
            int fd = mkstemp(cPath.data());
            if (fd >= 0) {
                return {UnixFD::own(fd), std::string(cPath.data())};
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
    static const inline std::filesystem::path emptyPath_;
    static const inline std::vector<std::filesystem::path> emptyPaths_ = {
        std::filesystem::path()};
};

StandardPaths::StandardPaths(
    const std::string &packageName,
    const std::unordered_map<std::string, std::filesystem::path> &builtInPath,
    bool skipBuiltInPath, bool skipUserPath)
    : d_ptr(std::make_unique<StandardPathsPrivate>(
          packageName, builtInPath, skipBuiltInPath, skipUserPath)) {}

StandardPaths::~StandardPaths() = default;

const StandardPaths &StandardPaths::global() {
    bool skipFcitx = checkBoolEnvVar("SKIP_FCITX_PATH");
    bool skipUser = checkBoolEnvVar("SKIP_FCITX_USER_PATH");
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

bool StandardPaths::hasExecutable(const std::filesystem::path &name) {
    // FIXME
    return true;
}

const std::filesystem::path &
StandardPaths::userDirectory(StandardPathsType type) const {
    FCITX_D();
    return d->directories(type)[0];
}

const std::vector<std::filesystem::path> &
StandardPaths::directories(StandardPathsType type) const {
    FCITX_D();
    return d->directories(type);
}

std::filesystem::path StandardPaths::locate(StandardPathsType type,
                                            const std::filesystem::path &path,
                                            StandardPathsModes modes) const {
    FCITX_D();
    std::string retPath;
    d->scanDirectories(type, path, modes,
                       [&retPath](const std::filesystem::path &fullPath) {
                           if (!std::filesystem::is_regular_file(fullPath)) {
                               return true;
                           }
                           retPath = std::move(fullPath);
                           return false;
                       });
    return retPath;
}

std::map<std::filesystem::path, std::filesystem::path>
StandardPaths::locate(StandardPathsType type, const std::filesystem::path &path,
                      const StandardPathsFilterCallback &callback, StandardPathsModes modes) const {
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
                if (directoryIterator->is_regular_file()) {
                    if (callback(directoryIterator->path())) {
                        retPath[directoryIterator->path().filename()] =
                            directoryIterator->path();
                    }
                }
            }
            return true;
        });
    return retPath;
}

UnixFD StandardPaths::open(StandardPathsType type,
                           const std::filesystem::path &path, StandardPathsModes modes,
                           std::filesystem::path *outPath) const {
    FCITX_D();
    UnixFD retFD;
    d->scanDirectories(type, path, modes,
                       [&retFD, outPath](std::string fullPath) {
                           retFD.give(::open(fullPath.c_str(), O_RDONLY));
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

bool StandardPaths::safeSave(StandardPathsType type,
                             const std::filesystem::path &pathOrig,
                             const std::function<bool(int)> &callback) const {
    FCITX_D();
    auto [file, path] = d->openUserTemp(type, pathOrig);
    if (!file.isValid()) {
        return false;
    }
    try {
        if (callback(file.fd())) {
            // sync first.
#ifdef _WIN32
            auto wfile = utf8::UTF8ToUTF16(file.tempPath());
            ::_wchmod(wfile.data(), 0666 & ~(d->umask()));
            _commit(fd_.fd());
#else
            // close it
            fchmod(file.fd(), 0666 & ~(d->umask()));
            fsync(file.fd());
#endif
            file.reset();
            std::filesystem::rename(path, pathOrig);
            return true;
        }
    } catch (const std::exception &) {
    }
    unlink(path.c_str());
    return false;
}

int64_t StandardPaths::timestamp(StandardPathsType type,
                                 const std::filesystem::path &path,
                                 StandardPathsModes modes) const {
    FCITX_D();

    int64_t timestamp = 0;
    d->scanDirectories(
        type, path, modes, [&timestamp](const std::filesystem::path &fullPath) {
            std::error_code ec;
            const auto time = std::filesystem::last_write_time(fullPath, ec);
            if (ec) {
                return true;
            }
            int64_t timeInSeconds =
                std::chrono::time_point_cast<std::chrono::seconds>(time)
                    .time_since_epoch()
                    .count();
            timestamp = std::max(timestamp, timeInSeconds);
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
