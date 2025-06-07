/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_STANDARDPATHS_H_
#define _FCITX_UTILS_STANDARDPATHS_H_

#include <sys/types.h>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/unixfd.h>
#include <span>

/**
 * \addtogroup FcitxUtils
 * \{
 * \file
 * \brief New Utility classes to handle application specific path.
 *
 * Comparing to the existing ones, it removes some obsolette functions, and
 * remove The ones can more easily implemented with std::filesystems.
 */

namespace fcitx {

class StandardPathsPrivate;

/** \brief Enum for location type. */
enum class StandardPathsType {
    Config,
    PkgConfig,
    Data,
    Cache,
    Runtime,
    Addon,
    PkgData
};
enum class StandardPathsMode : uint8_t {
    User = (1 << 0),
    System = (1 << 1),
    Default = User | System,
};

/**
 * Options for standard paths.
 *
 * This flag controls the behavior of StandardPaths.
 * If you want to skip some paths, you can use these flags.
 * User Path and System Path are derived from platform specific variables.
 * Built-in Path is based on installation path.
 */
enum class StandardPathsOption : uint8_t {
    SkipUserPath = (1 << 0),    // Skip user path
    SkipSystemPath = (1 << 1),  // Skip system path
    SkipBuiltInPath = (1 << 2), // Skip built-in path
};

using StandardPathsModes = Flags<StandardPathsMode>;
using StandardPathsOptions = Flags<StandardPathsOption>;
using StandardPathsFilterCallback =
    std::function<bool(const std::filesystem::path &)>;

namespace pathfilter {

static inline auto extension(const std::string &ext) {
    return [ext](const std::filesystem::path &path) {
        return path.extension() == ext;
    };
}

} // namespace pathfilter

class FCITXUTILS_EXPORT StandardPaths {
public:
    /**
     * Allow to construct a StandardPath with customized internal value.
     *
     * @param packageName the sub directory under other paths.
     * @param builtInPath this will override the value from fcitxPath.
     * @param options options to customize the behavior.
     */
    explicit StandardPaths(
        const std::string &packageName,
        const std::unordered_map<
            std::string, std::vector<std::filesystem::path>> &builtInPath,
        StandardPathsOptions options);

    virtual ~StandardPaths();

    /**
     * \brief Return the global instance of StandardPath.
     */
    static const StandardPaths &global();

    /** \brief Return fcitx specific path defined at compile time.
     *
     *  Currently, available value of fcitxPath are:
     *  datadir, pkgdatadir, libdir, bindir, localedir, addondir, libdatadir.
     *  Otherwise it will return nullptr.
     */
    static std::filesystem::path
    fcitxPath(const char *path, const std::filesystem::path &subPath = {});

    static std::filesystem::path
    findExecutable(const std::filesystem::path &name);

    static bool hasExecutable(const std::filesystem::path &name);

    /**
     * \brief Get user writable directory for given type.
     *
     * The path will be empty if there is no relevant user directory.
     */
    const std::filesystem::path &userDirectory(StandardPathsType type) const;

    /**
     * \brief Get all directories in the order of priority.
     */
    std::span<const std::filesystem::path>
    directories(StandardPathsType type,
                StandardPathsModes modes = StandardPathsMode::Default) const;

    /** \brief Check if a file exists. */
    std::filesystem::path
    locate(StandardPathsType type, const std::filesystem::path &path,
           StandardPathsModes modes = StandardPathsMode::Default) const;

    /**
     * \brief Check if path exists in all directories.
     *
     * It will enumerate all directories returned by directories() and check
     * if the file exists. The order is same as directories().
     */
    std::vector<std::filesystem::path>
    locateAll(StandardPathsType type, const std::filesystem::path &path,
              StandardPathsModes modes = StandardPathsMode::Default) const;

    /** \brief All subpath under path with filter. */
    std::map<std::filesystem::path, std::filesystem::path>
    locate(StandardPathsType type, const std::filesystem::path &path,
           const StandardPathsFilterCallback &callback,
           StandardPathsModes modes = StandardPathsMode::Default) const;

    /** \brief Open the first matched and succeeded file for read.
     *
     *  This function is preferred over locate if you just want to open the
     *  file. Then you can avoid the race condition.
     */
    UnixFD open(StandardPathsType type, const std::filesystem::path &path,
                StandardPathsModes modes = StandardPathsMode::Default,
                std::filesystem::path *outPath = nullptr) const;

    /**
     * \brief Open the path.
     *
     * This function will use _wopen on Windows and open on Unix.
     * By default, it will open the file with O_RDONLY.
     * _O_BINARY will be set on Windows.
     */
    static UnixFD openPath(const std::filesystem::path &path,
                           std::optional<int> flags = std::nullopt,
                           std::optional<mode_t> mode = std::nullopt);

    /** \brief Open the all matched and file for read.
     */
    std::vector<UnixFD>
    openAll(StandardPathsType type, const std::filesystem::path &path,
            StandardPathsModes modes = StandardPathsMode::Default,
            std::vector<std::filesystem::path> *outPath = nullptr) const;

    /**
     * \brief Save the file safely with write and rename to make sure the
     * operation is atomic.
     *
     * Callback shall not close the file descriptor. If the API you are using
     * does cannot do that, you may use UnixFD to help you dup it first.
     *
     * \param callback Callback function that accept a file descriptor and
     * return whether the save if success or not.
     */
    bool safeSave(StandardPathsType type, const std::filesystem::path &pathOrig,
                  const std::function<bool(int)> &callback) const;

    int64_t
    timestamp(StandardPathsType type, const std::filesystem::path &path,
              StandardPathsModes modes = StandardPathsMode::Default) const;

    /**
     * Sync system umask to internal state. This will affect the file
     * permission created by safeSave.
     *
     * @see safeSave
     */
    void syncUmask() const;

    /**
     * Whether this StandardPath is configured to Skip built-in path.
     *
     * Built-in path is usually configured at build time, hardcoded.
     * In portable environment (Install prefix is not fixed), this should be
     * set to false.
     */
    bool skipBuiltInPath() const;

    /**
     * Whether this StandardPath is configured to Skip user path.
     *
     */
    bool skipUserPath() const;

    /**
     * Whether this StandardPath is configured to Skip system path.
     */
    bool skipSystemPath() const;

    /**
     * \brief Get the options for the StandardPaths.
     *
     * This function returns the current configuration options for the
     * StandardPaths instance.
     */

    StandardPathsOptions options() const;

private:
    std::unique_ptr<StandardPathsPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StandardPaths);
};

template <typename T>
class StandardPathsTypeConverter {};

} // namespace fcitx

#endif // _FCITX_UTILS_STANDARDPATHS_H_
