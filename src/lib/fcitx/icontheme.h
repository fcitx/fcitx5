/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_ICONTHEME_H_
#define _FCITX_UTILS_ICONTHEME_H_

#include <memory>
#include <fcitx-config/enum.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/i18nstring.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/standardpath.h>
#include "fcitxcore_export.h"

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief XDG icon specification helper.

namespace fcitx {

class IconThemeDirectoryPrivate;
class IconThemePrivate;

FCITX_CONFIG_ENUM(IconThemeDirectoryType, Fixed, Scalable, Threshold);

class FCITXCORE_EXPORT IconThemeDirectory {
public:
    IconThemeDirectory(const RawConfig &config = RawConfig());
    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(IconThemeDirectory);

    FCITX_DECLARE_READ_ONLY_PROPERTY(std::string, path);
    FCITX_DECLARE_READ_ONLY_PROPERTY(int, size);
    FCITX_DECLARE_READ_ONLY_PROPERTY(int, scale);
    FCITX_DECLARE_READ_ONLY_PROPERTY(std::string, context);
    FCITX_DECLARE_READ_ONLY_PROPERTY(IconThemeDirectoryType, type);
    FCITX_DECLARE_READ_ONLY_PROPERTY(int, maxSize);
    FCITX_DECLARE_READ_ONLY_PROPERTY(int, minSize);
    FCITX_DECLARE_READ_ONLY_PROPERTY(int, threshold);

    bool matchesSize(int iconsize, int iconscale) const;
    int sizeDistance(int iconsize, int iconscale) const;

private:
    std::unique_ptr<IconThemeDirectoryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(IconThemeDirectory);
};

/// \brief A implementation of freedesktop.org icont specification.
class FCITXCORE_EXPORT IconTheme {
    friend class IconThemePrivate;

public:
    IconTheme(const char *name,
              const StandardPath &standardPath = StandardPath::global())
        : IconTheme(std::string(name), standardPath) {}
    IconTheme(const std::string &name,
              const StandardPath &standardPath = StandardPath::global());
    IconTheme(const StandardPath &standardPath = StandardPath::global());
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(IconTheme);

    // FIXME: remove non-const version when we can break ABI.
    std::string findIcon(const std::string &iconName, unsigned int desiredSize,
                         int scale = 1,
                         const std::vector<std::string> &extensions = {
                             ".svg", ".png", ".xpm"});
    std::string findIcon(const std::string &iconName, unsigned int desiredSize,
                         int scale = 1,
                         const std::vector<std::string> &extensions = {
                             ".svg", ".png", ".xpm"}) const;
    static std::string defaultIconThemeName();

    FCITX_DECLARE_READ_ONLY_PROPERTY(std::string, internalName);
    FCITX_DECLARE_READ_ONLY_PROPERTY(I18NString, name);
    FCITX_DECLARE_READ_ONLY_PROPERTY(I18NString, comment);
    FCITX_DECLARE_READ_ONLY_PROPERTY(std::vector<IconTheme>, inherits);
    FCITX_DECLARE_READ_ONLY_PROPERTY(std::vector<IconThemeDirectory>,
                                     directories);
    FCITX_DECLARE_READ_ONLY_PROPERTY(std::vector<IconThemeDirectory>,
                                     scaledDirectories);
    FCITX_DECLARE_READ_ONLY_PROPERTY(std::string, example);

    /// Rename fcitx-* icon to org.fcitx.Fcitx5.fcitx-* if in flatpak
    static std::string iconName(const std::string &icon,
                                bool inFlatpak = isInFlatpak());

private:
    IconTheme(const std::string &name, IconTheme *parent,
              const StandardPath &standardPath);

    std::unique_ptr<IconThemePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(IconTheme);
};
} // namespace fcitx

#endif // _FCITX_UTILS_ICONTHEME_H_
