/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_STANDARDPATHS_P_H_
#define _FCITX_UTILS_STANDARDPATHS_P_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <span>
#include "fs.h"
#include "standardpaths.h"
#include "unixfd.h"

namespace fcitx {

class StandardPathsPrivate {
public:
    StandardPathsPrivate(
        const std::string &packageName,
        const std::unordered_map<
            std::string, std::vector<std::filesystem::path>> &builtInPathMap,
        StandardPathsOptions options);

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
            case StandardPathsType::Cache:
                return cacheDir_;
            case StandardPathsType::Runtime:
                return runtimeDir_;
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

    StandardPathsOptions options() const { return options_; }

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
        if (!pathOrig.has_filename() ||
            options_.test(StandardPathsOption::SkipUserPath)) {
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

    static std::filesystem::path
    fcitxPath(const char *path, const std::filesystem::path &subPath);

    static const std::filesystem::path constEmptyPath;
    static const std::vector<std::filesystem::path> constEmptyPaths;

    static std::mutex globalMutex_;
    static std::unique_ptr<StandardPaths> global_;

    static void setGlobal(std::unique_ptr<StandardPaths> global) {
        std::lock_guard<std::mutex> lock(globalMutex_);
        global_ = std::move(global);
    }

private:
    void maybeUpdateModes(StandardPathsModes &modes) const {
        if (options_.test(StandardPathsOption::SkipUserPath)) {
            modes = modes.unset(StandardPathsMode::User);
        }
    }

    // Return the bottom symlink-ed path, otherwise, if error or not symlink
    // file, return the original path.
    static std::filesystem::path
    symlinkTarget(const std::filesystem::path &path) {
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

    StandardPathsOptions options_;
    std::vector<std::filesystem::path> configDirs_;
    std::vector<std::filesystem::path> pkgconfigDirs_;
    std::vector<std::filesystem::path> dataDirs_;
    std::vector<std::filesystem::path> pkgdataDirs_;
    std::vector<std::filesystem::path> cacheDir_;
    std::vector<std::filesystem::path> runtimeDir_;
    std::vector<std::filesystem::path> addonDirs_;
    std::atomic<mode_t> umask_;
};

} // namespace fcitx

#endif // _FCITX_UTILS_STANDARDPATHS_P_H_
