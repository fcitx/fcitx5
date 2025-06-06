/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iterator>
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
#include "config.h"
#include "environ.h"
#include "misc_p.h"
#include "standardpaths.h"
#include "standardpaths_p.h"
#include "stringutils.h"

namespace fcitx {

namespace {

// A simple wrapper that return null if the variable name is nullptr.
std::optional<std::string> getEnvironmentNull(const char *env) {
    if (!env) {
        return std::nullopt;
    }
    return getEnvironment(env);
}

// http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
std::vector<std::filesystem::path> defaultPaths(
    StandardPathsOptions options, bool isFcitx, const char *homeEnv,
    const std::filesystem::path &homeFallback, const char *systemEnv = nullptr,
    const std::vector<std::filesystem::path> &systemFallback = {},
    const char *builtInPathType = nullptr,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
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

    if (!options.test(StandardPathsOption::SkipSystemPath)) {
        if (const auto dir = getEnvironmentNull(systemEnv)) {
            const auto rawDirs = stringutils::split(*dir, ":");
            std::ranges::transform(
                rawDirs, std::back_inserter(dirs), [](const auto &s) {
                    return std::filesystem::path(s).lexically_normal();
                });
        } else {
            std::ranges::copy(systemFallback, std::back_inserter(dirs));
        }
    }

    if (builtInPathType &&
        !options.test(StandardPathsOption::SkipBuiltInPath)) {
        std::vector<std::filesystem::path> builtInPaths;
        if (const auto *value = findValue(builtInPathMap, builtInPathType)) {
            builtInPaths = *value;
        } else {
            if (std::string_view(builtInPathType).starts_with("pkg") &&
                isFcitx) {
                builtInPaths = {StandardPaths::fcitxPath(builtInPathType)};
            }
        }
        for (const auto &builtInPath : builtInPaths) {
            const auto path = builtInPath.lexically_normal();
            if (!path.empty()) {
                dirs.push_back(path);
            }
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

} // namespace

StandardPathsPrivate::StandardPathsPrivate(
    const std::string &packageName,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &builtInPathMap,
    StandardPathsOptions options)
    : options_(options) {
    bool isFcitx = (packageName == "fcitx5");
    std::filesystem::path packagePath =
        std::u8string(packageName.begin(), packageName.end());
    // initialize user directory
    configDirs_ = defaultPaths(options_, isFcitx, "XDG_CONFIG_HOME", ".config",
                               "XDG_CONFIG_DIRS", {"/etc/xdg"}, "configdir",
                               builtInPathMap);
    std::vector<std::filesystem::path> pkgconfigDirFallback;
    std::ranges::copy(
        configDirs_ | std::views::drop(1) |
            std::views::transform(
                [&packagePath](const auto &dir) { return dir / packagePath; }),
        std::back_inserter(pkgconfigDirFallback));
    pkgconfigDirs_ = defaultPaths(
        options_, isFcitx, (isFcitx ? "FCITX_CONFIG_HOME" : nullptr),
        configDirs_[0] / packagePath, (isFcitx ? "FCITX_CONFIG_DIRS" : nullptr),
        pkgconfigDirFallback, "pkgconfigdir", builtInPathMap);

    dataDirs_ = defaultPaths(
        options_, isFcitx, "XDG_DATA_HOME", ".local/share", "XDG_DATA_DIRS",
        {"/usr/local/share", "/usr/share"}, "datadir", builtInPathMap);
    std::vector<std::filesystem::path> pkgdataDirFallback;
    std::ranges::copy(
        dataDirs_ | std::views::drop(1) |
            std::views::transform(
                [&packagePath](const auto &dir) { return dir / packagePath; }),
        std::back_inserter(pkgdataDirFallback));
    pkgdataDirs_ = defaultPaths(
        options_, isFcitx, (isFcitx ? "FCITX_DATA_HOME" : nullptr),
        dataDirs_[0] / packagePath, (isFcitx ? "FCITX_DATA_DIRS" : nullptr),
        pkgdataDirFallback, "pkgdatadir", builtInPathMap);
    cacheDir_ = defaultPaths(options_, isFcitx, "XDG_CACHE_HOME", ".cache");
    assert(cacheDir_.size() == 1);
    std::error_code ec;
    auto tmpdir = std::filesystem::temp_directory_path(ec);
    runtimeDir_ =
        defaultPaths(options_, isFcitx, "XDG_RUNTIME_DIR",
                     tmpdir.empty() ? std::filesystem::path("/tmp") : tmpdir);
    assert(runtimeDir_.size() == 1);
    // Though theoretically, this is also fcitxPath, we just simply don't
    // use it here.
    addonDirs_ =
        defaultPaths(options_, isFcitx, nullptr, {}, "FCITX_ADDON_DIRS",
                     {FCITX_INSTALL_ADDONDIR}, "addondir", builtInPathMap);

    syncUmask();
}

std::filesystem::path
StandardPathsPrivate::fcitxPath(const char *path,
                                const std::filesystem::path &subPath) {

    if (!path) {
        return {};
    }

    static const std::unordered_map<std::string, std::filesystem::path>
        pathMap = {
            std::make_pair<std::string, std::filesystem::path>(
                "basedir", FCITX_INSTALL_PREFIX),
            std::make_pair<std::string, std::filesystem::path>(
                "sysconfdir", FCITX_INSTALL_SYSCONFDIR),
            std::make_pair<std::string, std::filesystem::path>(
                "configdir", FCITX_INSTALL_SYSCONFDIR "/xdg"),
            std::make_pair<std::string, std::filesystem::path>(
                "pkgconfigdir", FCITX_INSTALL_SYSCONFDIR "/xdg/fcitx5"),
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

} // namespace fcitx
