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

#include "library.h"
#include <dlfcn.h>
#include <cstring>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "stringutils.h"

namespace fcitx {

class LibraryPrivate {
public:
    LibraryPrivate(const std::string &path_) : path(path_), handle(nullptr) {}
    std::string path;
    void *handle;
    std::string error;
};

Library::Library(const std::string &path)
    : d_ptr(std::make_unique<LibraryPrivate>(path)) {}

Library::~Library() {
    if (d_ptr) {
        unload();
    }
}

Library::Library(Library &&other) noexcept : d_ptr(std::move(other.d_ptr)) {}

Library &Library::operator=(Library other) noexcept {
    using std::swap;
    swap(d_ptr, other.d_ptr);
    return *this;
}

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

    // allow dlopen self
    d->handle = dlopen(!d->path.empty() ? d->path.c_str() : nullptr, flag);
    if (!d->handle) {
        d->error = dlerror();
        return false;
    }

    return true;
}

bool Library::loaded() const {
    FCITX_D();
    return !!d->handle;
}

bool Library::unload() {
    FCITX_D();
    if (!d->handle) {
        return false;
    }
    if (dlclose(d->handle)) {
        d->error = dlerror();
        return false;
    }

    d->handle = nullptr;
    return true;
}

void *Library::resolve(const char *name) {
    FCITX_D();
    auto result = dlsym(d->handle, name);
    if (!result) {
        d->error = dlerror();
    }
    return result;
}

bool Library::findData(const char *slug, const char *magic, size_t lenOfMagic,
                       std::function<void(const char *data)> parser) {
    FCITX_D();
    if (d->handle) {
        void *data = dlsym(d->handle, slug);
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

    int fd = open(d->path.c_str(), O_RDONLY);
    if (fd < 0) {
        d->error = strerror(errno);
        return false;
    }

    void *needfree = nullptr;
    bool result = false;
    do {
        struct stat statbuf;
        int statresult = fstat(fd, &statbuf);
        if (statresult < 0) {
            d->error = strerror(errno);
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
    return d->error;
}
}
