/*
 * SPDX-FileCopyrightText: 2017-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "buffer.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cairo/cairo.h>
#include <sys/syscall.h>
#include <wayland-client.h>
#include "fcitx-utils/stringutils.h"
#include "theme.h"
#include "wl_buffer.h"
#include "wl_callback.h"
#include "wl_shm.h"
#include "wl_shm_pool.h"
#include "wl_surface.h"

namespace fcitx::wayland {

#define RETRY_ON_EINTR(...)                                                    \
    do {                                                                       \
        __VA_ARGS__;                                                           \
    } while (ret < 0 && errno == EINTR)

UnixFD openShm(void) {
    int ret;
    // We support multiple different methods, memfd / shm_open on BSD /
    // O_TMPFILE. While linux has shm_open, it doesn't have SHM_ANON extension
    // like bsd. Since memfd is introduced in 3.17, which is already quite old,
    // don't bother to add another implementation for Linux, since we need to
    // fallback to file as a final resolution.
#if defined(__NR_memfd_create)
    do {
        static bool hasMemfdCreate = true;
        if (!hasMemfdCreate) {
            break;
        }
        int options = MFD_CLOEXEC;
#if defined(MFD_ALLOW_SEALING)
        options |= MFD_ALLOW_SEALING;
#endif
        RETRY_ON_EINTR(
            ret = syscall(__NR_memfd_create, "fcitx-wayland-shm", options));

        if (ret < 0 && errno == ENOSYS) {
            hasMemfdCreate = false;
            break;
        }
#if defined(F_ADD_SEALS) && defined(F_SEAL_SHRINK)
        if (ret >= 0) {
            fcntl(ret, F_ADD_SEALS, F_SEAL_SHRINK);
            return UnixFD::own(ret);
        }
#endif
    } while (0);
#endif

    // Try shm_open(SHM_ANON) on BSD.
#if defined(__FreeBSD__)
    RETRY_ON_EINTR(
        ret = shm_open(SHM_ANON, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600));
    if (ret >= 0) {
        return UnixFD::own(ret);
    }
#endif

    const char *path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        throw std::runtime_error("XDG_RUNTIME_DIR is not set");
    }

    // Prefer O_TMPFILE over mkstemp.
    // The value check is to ensure it has the right value.
    // It is said that some old glibc may have problem.
#if defined(O_TMPFILE) && (O_TMPFILE & O_DIRECTORY) == O_DIRECTORY
    do {
        std::string pathStr = fs::cleanPath(path);
        RETRY_ON_EINTR(ret =
                           open(pathStr.data(),
                                O_TMPFILE | O_CLOEXEC | O_EXCL | O_RDWR, 0600));
        if (errno == EOPNOTSUPP || errno == EISDIR) {
            break;
        }
        if (ret >= 0) {
            return UnixFD::own(ret);
        }
        return {};
    } while (0);
#endif

    auto filename = stringutils::joinPath(path, "fcitx-wayland-shm-XXXXXX");
    std::vector<char> v(filename.begin(), filename.end());
    v.push_back('\0');
    RETRY_ON_EINTR(ret = mkstemp(v.data()));

    if (ret >= 0) {
        unlink(v.data());
        if (int flags = fcntl(ret, F_GETFD); flags != -1) {
            fcntl(ret, F_SETFD, flags | FD_CLOEXEC);
        }
        return UnixFD::own(ret);
    }

    return {};
}

Buffer::Buffer(WlShm *shm, uint32_t width, uint32_t height,
               wl_shm_format format)
    : width_(width), height_(height) {
    uint32_t stride = width * 4;
    uint32_t alloc = stride * height;
    UnixFD fd = openShm();
    if (!fd.isValid()) {
        return;
    }

    if (posix_fallocate(fd.fd(), 0, alloc) != 0) {
        return;
    }
    uint8_t *data = (uint8_t *)mmap(nullptr, alloc, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd.fd(), 0);

    if (data == static_cast<uint8_t *>(MAP_FAILED)) {
        return;
    }

    data_ = data;
    dataSize_ = alloc;

    pool_.reset(shm->createPool(fd.fd(), alloc));
    buffer_.reset(pool_->createBuffer(0, width, height, stride, format));
    buffer_->release().connect([this]() { busy_ = false; });

    surface_.reset(cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, width, height, stride));
}

Buffer::~Buffer() {
    callback_.reset();
    surface_.reset();
    buffer_.reset();
    pool_.reset();
    if (data_) {
        munmap(data_, dataSize_);
    }
}

void Buffer::attachToSurface(WlSurface *surface, int scale) {
    if (busy_) {
        return;
    }
    busy_ = true;
    callback_.reset(surface->frame());
    callback_->done().connect([this](uint32_t) {
        // CLASSICUI_DEBUG() << "Shm window rendered. " << this;
        // Need to ensure buffer won't be deleted.
        busy_ = false;
        rendered_();
        callback_.reset();
    });

    surface->attach(buffer(), 0, 0);
    surface->setBufferScale(scale);
    surface->damage(0, 0, width_, height_);
    surface->commit();
}

} // namespace fcitx::wayland
