/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_STANDARDPATHS_H_
#define _FCITX_UTILS_STANDARDPATHS_H_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/unixfd.h>

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

namespace pathfilter {

static inline auto extension(const std::string &ext) {
    return [ext](const std::filesystem::path &path) {
        return path.extension() == ext;
    };
}

} // namespace pathfilter

class FCITXUTILS_EXPORT StandardPaths {
public:
    using PathFilterCallback =
        std::function<bool(const std::filesystem::path &)>;

    enum class Mode : uint8_t {
        User = (1 << 0),
        System = (1 << 1),
        Default = User | System,
    };

    using Modes = Flags<Mode>;

    /**
     * Allow to construct a StandardPath with customized internal value.
     *
     * @param packageName the sub directory under other paths.
     * @param builtInPath this will override the value from fcitxPath.
     * @param skipBuiltInPath skip built-in path
     * @param skipUserPath skip user path, useful when doing readonly-test.
     */
    StandardPaths(const std::string &packageName,
                  const std::unordered_map<std::string, std::filesystem::path>
                      &builtInPath,
                  bool skipBuiltInPath, bool skipUserPath);

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

    static bool hasExecutable(const std::filesystem::path &name);

    /**
     * \brief Get user writable directory for given type.
     */
    const std::filesystem::path &userDirectory(StandardPathsType type) const;

    /**
     * \brief Get all directories in the order of priority.
     */
    const std::vector<std::filesystem::path> &
    directories(StandardPathsType type) const;

    /** \brief Check if a file exists. */
    std::filesystem::path locate(StandardPathsType type,
                                 const std::filesystem::path &path,
                                 Modes modes = Mode::Default) const;

    /** \brief Check if a file exists. */
    std::map<std::filesystem::path, std::filesystem::path>
    locate(StandardPathsType type, const std::filesystem::path &path,
           const PathFilterCallback &callback,
           Modes modes = Mode::Default) const;

    /** \brief Open the first matched and succeeded file for read.
     *
     *  This function is preferred over locate if you just want to open the
     *  file. Then you can avoid the race condition.
     */
    UnixFD open(StandardPathsType type, const std::filesystem::path &path,
                Modes modes = Mode::Default,
                std::filesystem::path *outPath = nullptr) const;

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

    int64_t timestamp(StandardPathsType type, const std::filesystem::path &path,
                      Modes modes = Mode::Default) const;

    /**
     * Sync system umask to internal state. This will affect the file
     * permission created by safeSave.
     *
     * @see safeSave
     */
    void syncUmask() const;

private:
    std::unique_ptr<StandardPathsPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StandardPaths);
};

template <typename T>
class StandardPathsTypeConverter {};

} // namespace fcitx

#endif // _FCITX_UTILS_STANDARDPATHS_H_
