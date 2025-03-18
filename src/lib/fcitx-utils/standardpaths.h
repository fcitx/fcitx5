/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <cstdint>
#include <filesystem>
#include <functional>
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

class FCITXUTILS_EXPORT StandardPaths {
public:
    /** \brief Enum for location type. */
    enum class Type { Config, PkgConfig, Data, Cache, Runtime, Addon, PkgData };

    enum class Mode : uint8_t {
        User = (1 << 0),
        System = (1 << 1),
        Files = (1 << 1),
        Dirs = (1 << 2),
        Default = User | System | Files,
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

    /**
     * \brief Get user writable directory for given type.
     */
    const std::filesystem::path &userDirectory(Type type) const;

    /**
     * \brief Get all directories in the order of priority.
     */
    const std::vector<std::filesystem::path> &directories(Type type) const;

    /** \brief Check if a file exists. */
    std::filesystem::path locate(Type type, const std::filesystem::path &path,
                                 Modes modes = Mode::Default) const;

    /** \brief list all matched files. */
    std::vector<std::filesystem::path>
    locateAll(Type type, const std::filesystem::path &path,
              Modes modes = Mode::Default) const;

    /** \brief Open the first matched and succeeded file for read.
     *
     *  This function is preferred over locate if you just want to open the
     *  file. Then you can avoid the race condition.
     */
    UnixFD open(Type type, const std::filesystem::path &path,
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
    bool safeSave(Type type, const std::filesystem::path &pathOrig,
                  const std::function<bool(int)> &callback) const;

    /**
     * \brief Open all files match the first [directory]/[path].
     */
    void
    openAll(Type type, const std::filesystem::path &path,
            const std::function<void(UnixFD, const std::filesystem::path &)>
                &callback) const;

    int64_t timestamp(Type type, const std::filesystem::path &path,
                      Modes modes = Mode::Default) const;

    /**
     * Sync system umask to internal state. This will affect the file
     * permission created by safeSave.
     *
     * @see safeSave
     */
    void syncUmask() const;

    /**
     * Whether this StandardPaths is configured to Skip built-in path.
     *
     * Built-in path is usually configured at build time, hardcoded.
     * In portable environment (Install prefix is not fixed), this should be
     * set to false.
     */
    bool skipBuiltInPath() const;

    /**
     * Whether this StandardPaths is configured to Skip user path.
     */
    bool skipUserPath() const;

private:
    std::unique_ptr<StandardPathsPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StandardPaths);
};

} // namespace fcitx
