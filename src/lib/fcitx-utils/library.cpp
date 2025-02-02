/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "library.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include "config.h"
#include "macros.h"
#include "misc.h"
#include "stringutils.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

namespace fcitx {

namespace {

#ifdef RTLD_NODELETE
constexpr bool hasRTLDNoDelete = true;
#else
constexpr bool hasRTLDNoDelete = false;
#endif

} // namespace

class LibraryPrivate {
public:
    LibraryPrivate(std::string path) : path_(std::move(path)) {}
    ~LibraryPrivate() { unload(); }

    bool unload() {
        if (!handle_) {
            return false;
        }

        // If there is no RTLD_NODELETE and we don't want to unload, we leak
        // it on purpose.
        if (hasRTLDNoDelete ||
            !loadFlag_.test(LibraryLoadHint::PreventUnloadHint)) {
            if (dlclose(handle_)) {
                error_ = dlerror();
                return false;
            }
        }

        handle_ = nullptr;
        return true;
    }

    std::string path_;
    void *handle_ = nullptr;
    std::string error_;
    Flags<fcitx::LibraryLoadHint> loadFlag_;
};

Library::Library(const std::string &path)
    : d_ptr(std::make_unique<LibraryPrivate>(path)) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(Library)

bool Library::load(Flags<fcitx::LibraryLoadHint> hint) {
    if (loaded()) {
        return true;
    }
    FCITX_D();
    int flag = 0;
    if (hint & LibraryLoadHint::ResolveAllSymbolsHint) {
        flag |= RTLD_NOW;
    } else {
        flag |= RTLD_LAZY;
    }

#ifdef RTLD_NODELETE
    if (hint & LibraryLoadHint::PreventUnloadHint) {
        flag |= RTLD_NODELETE;
    }
#endif

    if (hint & LibraryLoadHint::ExportExternalSymbolsHint) {
        flag |= RTLD_GLOBAL;
    }

#ifdef HAS_DLMOPEN
    if (hint & LibraryLoadHint::NewNameSpace) {
        // allow dlopen self
        d->handle_ = dlmopen(
            LM_ID_NEWLM, !d->path_.empty() ? d->path_.c_str() : nullptr, flag);
    } else
#endif
    {
        d->handle_ =
            dlopen(!d->path_.empty() ? d->path_.c_str() : nullptr, flag);
    }
    if (!d->handle_) {
        d->error_ = dlerror();
        return false;
    }

    d->loadFlag_ = hint;

    return true;
}

bool Library::loaded() const {
    FCITX_D();
    return !!d->handle_;
}

bool Library::unload() {
    FCITX_D();
    return d->unload();
}

void *Library::resolve(const char *name) {
    FCITX_D();
    auto *result = dlsym(d->handle_, name);
    if (!result) {
        d->error_ = dlerror();
    }
    return result;
}

bool Library::findData(const char *slug, const char *magic, size_t lenOfMagic,
                       const std::function<void(const char *data)> &parser) {
    FCITX_D();
    if (d->handle_) {
        void *data = dlsym(d->handle_, slug);
        if (!data) {
            return false;
        }

        if (memcmp(data, magic, lenOfMagic) != 0) {
            return false;
        }

        data = static_cast<char *>(data) + lenOfMagic;
        parser(static_cast<const char *>(data));
        return true;
    }

    int fd = open(d->path_.c_str(), O_RDONLY);
    if (fd < 0) {
        d->error_ = strerror(errno);
        return false;
    }

    UniqueCPtr<void> needfree;
    bool result = false;
    do {
        struct stat statbuf;
        int statresult = fstat(fd, &statbuf);
        if (statresult < 0) {
            d->error_ = strerror(errno);
            break;
        }
        void *data = nullptr;
#ifdef HAVE_SYS_MMAN_H
        data = mmap(nullptr, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        void *needunmap = nullptr;
        needunmap = data;
#endif
        if (!data) {
            data = malloc(statbuf.st_size);
            needfree.reset(data);
            if (!data) {
                break;
            }
            if (read(fd, data, statbuf.st_size) != statbuf.st_size) {
                break;
            }
        }
        const char *pos = stringutils::backwardSearch(
            static_cast<char *>(data), static_cast<size_t>(statbuf.st_size),
            magic, lenOfMagic, 0);
        pos += lenOfMagic;

        if (parser) {
            parser(pos);
        }
        result = true;
#ifdef HAVE_SYS_MMAN_H
        if (needunmap) {
            munmap(needunmap, statbuf.st_size);
        }
#endif
    } while (false);

    close(fd);

    return result;
}

std::string Library::error() {
    FCITX_D();
    return d->error_;
}

bool Library::isNewNamespaceSupported() {
#ifdef HAS_DLMOPEN
    return true;
#else
    return false;
#endif
}

const std::string &Library::path() const {
    FCITX_D();
    return d->path_;
}

} // namespace fcitx
