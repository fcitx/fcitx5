/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_LIBRARY_H_
#define _FCITX_UTILS_LIBRARY_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Class to handler dynamic library.

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>

namespace fcitx {

enum class LibraryLoadHint {
    NoHint = 0,
    ResolveAllSymbolsHint = 0x1,
    PreventUnloadHint = 0x2,
    ExportExternalSymbolsHint = 0x4,
    NewNameSpace = 0x8,
    DefaultHint = PreventUnloadHint,
};

class LibraryPrivate;

class FCITXUTILS_EXPORT Library {
public:
    explicit Library(const std::string &path);
    explicit Library(const std::filesystem::path &path = {});
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(Library);

    bool loaded() const;
    bool load(Flags<LibraryLoadHint> hint = LibraryLoadHint::DefaultHint);
    bool unload();
    void *resolve(const char *name);
    bool findData(const char *slug, const char *magic, size_t lenOfMagic,
                  const std::function<void(const char *data)> &parser);
    std::string error();
    const std::string &path() const;
    const std::filesystem::path &fspath() const;

    template <typename Func>
    static auto toFunction(void *ptr) {
        return reinterpret_cast<Func *>(ptr);
    }

    static bool isNewNamespaceSupported();

private:
    std::unique_ptr<LibraryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Library);
};
} // namespace fcitx

#endif // _FCITX_UTILS_LIBRARY_H_
