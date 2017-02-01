#include "buffer.h"
#include "wl_buffer.h"
#include "wl_shm.h"
#include "wl_shm_pool.h"
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

Buffer::Buffer(WlShm *shm, int width, int height, wl_shm_format format)
    : surface_(nullptr, &cairo_surface_destroy), cairo_(nullptr, &cairo_destroy), width_(width), height_(height) {
    const char *path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        throw std::runtime_error("XDG_RUNTIME_DIR is not set");
    }
    int stride = width * 4;
    int alloc = stride * height;
    auto filename = std::string(path) + "/fcitx-wayland-shm-XXXXXX";
    std::vector<char> v(filename.begin(), filename.end());
    v.push_back('\0');
    int fd = mkstemp(v.data());
    if (fd < 0) {
        return;
    }
    int flags = fcntl(fd, F_GETFD);
    if (flags != -1)
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);

    if (ftruncate(fd, alloc) < 0) {
        close(fd);
        return;
    }
    uint8_t *data = (uint8_t *)mmap(NULL, alloc, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unlink(v.data());

    if (data == static_cast<uint8_t *>(MAP_FAILED)) {
        close(fd);
        return;
    }

    pool_.reset(shm->createPool(fd, alloc));
    buffer_.reset(pool_->createBuffer(0, width, height, stride, format));
    buffer_->release().connect([this]() { busy_ = false; });

    surface_.reset(cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, width, height, stride));
    cairo_.reset(cairo_create(surface_.get()));

    close(fd);
}

Buffer::~Buffer() {}
}
}
