/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_UTILS_LIBRARY_H_
#define _FCITX_UTILS_LIBRARY_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Class to handler dynamic library.

#include "fcitxutils_export.h"
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <functional>
#include <memory>

namespace fcitx {

enum class LibraryLoadHint {
    NoHint = 0,
    ResolveAllSymbolsHint = 0x1,
    PreventUnloadHint = 0x2,
    ExportExternalSymbolsHint = 0x4,
    DefaultHint = PreventUnloadHint,
};

class LibraryPrivate;

class FCITXUTILS_EXPORT Library {
public:
    Library(const std::string &path = {});
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(Library);

    bool loaded() const;
    bool load(Flags<LibraryLoadHint> hint = LibraryLoadHint::DefaultHint);
    bool unload();
    void *resolve(const char *name);
    bool findData(const char *slug, const char *magic, size_t lenOfMagic,
                  std::function<void(const char *data)> library);
    std::string error();

    template <typename Func>
    static auto toFunction(void *ptr) {
        return reinterpret_cast<Func *>(ptr);
    }

private:
    std::unique_ptr<LibraryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Library);
};
}

#endif // _FCITX_UTILS_LIBRARY_H_
