/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "library.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "config.h"
#include "stringutils.h"

namespace fcitx {

class LibraryPrivate {
public:
    LibraryPrivate(const std::string &path) : path_(path), handle_(nullptr) {}
    ~LibraryPrivate() { unload(); }

    bool unload() {
        if (!handle_) {
            return false;
        }
        if (dlclose(handle_)) {
            error_ = dlerror();
            return false;
        }

        handle_ = nullptr;
        return true;
    }

    std::string path_;
    void *handle_;
    std::string error_;
};

Library::Library(const std::string &path)
    : d_ptr(std::make_unique<LibraryPrivate>(path)) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(Library)

bool Library::load(Flags<fcitx::LibraryLoadHint> hint) {
    FCITX_D();
    int flag = 0;
    if (hint & LibraryLoadHint::ResolveAllSymbolsHint) {
        flag |= RTLD_NOW;
    } else {
        flag |= RTLD_LAZY;
    }

    if (hint & LibraryLoadHint::PreventUnloadHint) {
        flag |= RTLD_NODELETE;
    }

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
    auto result = dlsym(d->handle_, name);
    if (!result) {
        d->error_ = dlerror();
    }
    return result;
}

bool Library::findData(const char *slug, const char *magic, size_t lenOfMagic,
                       std::function<void(const char *data)> parser) {
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

    void *needfree = nullptr;
    bool result = false;
    do {
        struct stat statbuf;
        int statresult = fstat(fd, &statbuf);
        if (statresult < 0) {
            d->error_ = strerror(errno);
            break;
        }
        void *needunmap = nullptr;
        void *data = needunmap =
            mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (!data) {
            data = malloc(statbuf.st_size);
            needfree = data;
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
        if (needunmap) {
            munmap(needunmap, statbuf.st_size);
        }
    } while (0);

    close(fd);
    free(needfree);

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
} // namespace fcitx
