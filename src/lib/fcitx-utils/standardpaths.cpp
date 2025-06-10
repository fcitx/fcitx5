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
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>
#include <span>
#include "fcitx-utils/standardpaths_p.h"
#include "config.h" // IWYU pragma: keep
#include "environ.h"
#include "fs.h"
#include "log.h"
#include "macros.h"
#include "misc.h"
#include "misc_p.h"
#include "standardpaths_p.h"
#include "stringutils.h"
#include "unixfd.h"

namespace fcitx {

const std::filesystem::path StandardPathsPrivate::constEmptyPath;
const std::vector<std::filesystem::path> StandardPathsPrivate::constEmptyPaths =
    {std::filesystem::path()};

std::mutex StandardPathsPrivate::globalMutex_;
std::unique_ptr<StandardPaths> StandardPathsPrivate::global_;

namespace {

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

StandardPaths::StandardPaths(
    const std::string &packageName,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &builtInPath,
    StandardPathsOptions options)
    : d_ptr(std::make_unique<StandardPathsPrivate>(packageName, builtInPath,
                                                   options)) {}

StandardPaths::~StandardPaths() = default;

const StandardPaths &StandardPaths::global() {
    std::lock_guard<std::mutex> lock(StandardPathsPrivate::globalMutex_);
    if (!StandardPathsPrivate::global_) {
        bool skipFcitx = checkBoolEnvVar("SKIP_FCITX_PATH");
        bool skipFcitxSystem = checkBoolEnvVar("SKIP_FCITX_SYSTEM_PATH");
        bool skipUser = checkBoolEnvVar("SKIP_FCITX_USER_PATH");
        StandardPathsOptions options;
        if (skipUser) {
            options |= StandardPathsOption::SkipUserPath;
        }
        if (skipFcitxSystem) {
            options |= StandardPathsOption::SkipSystemPath;
        }
        if (skipFcitx) {
            options |= StandardPathsOption::SkipBuiltInPath;
        }
        StandardPathsPrivate::global_ = std::make_unique<StandardPaths>(
            "fcitx5",
            std::unordered_map<std::string,
                               std::vector<std::filesystem::path>>{},
            options);
    }

    return *StandardPathsPrivate::global_;
}

std::filesystem::path
StandardPaths::fcitxPath(const char *path,
                         const std::filesystem::path &subPath) {
    return StandardPathsPrivate::fcitxPath(path, subPath);
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
    if (skipUserPath()) {
        return StandardPathsPrivate::constEmptyPath;
    }
    auto dirs = d->directories(type, StandardPathsMode::User);
    return dirs.empty() ? StandardPathsPrivate::constEmptyPath : dirs[0];
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

UnixFD StandardPaths::openPath(const std::filesystem::path &path,
                               std::optional<int> flags,
                               std::optional<mode_t> mode) {
    int f = flags.value_or(O_RDONLY);

    auto openFunc = [](auto path, int flag, auto... extra) {
#ifdef _WIN32
        flag |= _O_BINARY;
        return ::_wopen(path, flag, extra...);
#else
        return ::open(path, flag, extra...);
#endif
    };
    if (mode.has_value()) {
        return UnixFD::own(openFunc(path.c_str(), f, mode.value()));
    }
    return UnixFD::own(openFunc(path.c_str(), f));
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
    return d->options() & StandardPathsOption::SkipBuiltInPath;
}

bool StandardPaths::skipUserPath() const {
    FCITX_D();
    return d->options() & StandardPathsOption::SkipUserPath;
}

bool StandardPaths::skipSystemPath() const {
    FCITX_D();
    return d->options() & StandardPathsOption::SkipSystemPath;
}

StandardPathsOptions StandardPaths::options() const {
    FCITX_D();
    return d->options();
}

} // namespace fcitx
