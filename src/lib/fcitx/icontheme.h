//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_UTILS_ICONTHEME_H_
#define _FCITX_UTILS_ICONTHEME_H_

#include "fcitxcore_export.h"
#include <cstdlib>
#include <fcitx-config/enum.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/i18nstring.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/standardpath.h>
#include <memory>

/// \addtogroup FcitxUtils
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

    std::string findIcon(const std::string &iconName, uint desiredSize,
                         int scale = 1,
                         const std::vector<std::string> &extensions = {
                             ".svg", ".png", ".xpm"});
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

private:
    IconTheme(const std::string &name, IconTheme *parent,
              const StandardPath &standardPath);

    std::unique_ptr<IconThemePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(IconTheme);
};
} // namespace fcitx

#endif // _FCITX_UTILS_ICONTHEME_H_
