#include "buffer.h"
#include "fcitx-utils/stringutils.h"
#include "theme.h"
#include "wl_buffer.h"
#include "wl_callback.h"
#include "wl_shm.h"
#include "wl_shm_pool.h"
#include "wl_surface.h"
#include <cairo/cairo.h>
#include <cassert>
#include <fcntl.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include <wayland-client.h>

namespace fcitx {
namespace wayland {

Buffer::Buffer(WlShm *shm, uint32_t width, uint32_t height,
               wl_shm_format format)
    : surface_(nullptr, &cairo_surface_destroy), width_(width),
      height_(height) {
    const char *path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        throw std::runtime_error("XDG_RUNTIME_DIR is not set");
    }
    uint32_t stride = width * 4;
    uint32_t alloc = stride * height;
    auto filename = stringutils::joinPath(path, "fcitx-wayland-shm-XXXXXX");
    std::vector<char> v(filename.begin(), filename.end());
    v.push_back('\0');
    UnixFD fd = UnixFD::own(mkstemp(v.data()));
    if (!fd.isValid()) {
        return;
    }
    int flags = fcntl(fd.fd(), F_GETFD);
    if (flags == -1) {
        return;
    }
    if (fcntl(fd.fd(), F_SETFD, flags | FD_CLOEXEC) == -1) {
        return;
    }

    if (posix_fallocate(fd.fd(), 0, alloc) != 0) {
        return;
    }
    uint8_t *data = (uint8_t *)mmap(nullptr, alloc, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd.fd(), 0);
    unlink(v.data());

    if (data == static_cast<uint8_t *>(MAP_FAILED)) {
        return;
    }

    pool_.reset(shm->createPool(fd.fd(), alloc));
    buffer_.reset(pool_->createBuffer(0, width, height, stride, format));
    buffer_->release().connect([this]() { busy_ = false; });

    surface_.reset(cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, width, height, stride));
}

Buffer::~Buffer() {}

void Buffer::attachToSurface(WlSurface *surface) {
    if (busy_) {
        return;
    }
    busy_ = true;
    callback_.reset(surface->frame());
    callback_->done().connect([this](uint32_t) {
        // CLASSICUI_DEBUG() << "Shm window rendered. " << this;
        busy_ = false;
        rendered_();
        callback_.reset();
    });

    surface->attach(buffer(), 0, 0);
    surface->damage(0, 0, width_, height_);
    surface->commit();
}

} // namspace wayland
} // namespace fcitx
