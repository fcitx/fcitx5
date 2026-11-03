/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "mappedbuffer.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <span>
#include "config.h"
#include "fs.h"
#include "macros.h"
#include "unixfd.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

namespace fcitx {

class MappedBufferPrivate {
public:
    MappedBufferPrivate(int fd, const MappedBufferOptions &options);

    ~MappedBufferPrivate() {
#ifdef HAVE_SYS_MMAN_H
        if (mapped_) {
            munmap(data_.data(), data_.size());
        }
#endif
    }

    std::span<std::byte> data_;
    bool mapped_ = false;
    std::unique_ptr<std::byte[]> buffer_;
};

namespace {

size_t bufferSize(int fd, std::optional<size_t> requestedSize) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "Failed to stat file descriptor");
    }
    if (S_ISDIR(st.st_mode)) {
        throw std::invalid_argument("File descriptor refers to a directory");
    }
    if (requestedSize) {
        if (S_ISREG(st.st_mode) &&
            std::cmp_greater(*requestedSize, st.st_size)) {
            throw std::invalid_argument("Requested size exceeds file size");
        }
        return *requestedSize;
    }
    if (!S_ISREG(st.st_mode)) {
        throw std::invalid_argument(
            "An explicit size is required for non-regular file descriptors");
    }
    if (!std::in_range<size_t>(st.st_size)) {
        throw std::length_error("File size cannot be represented");
    }
    return static_cast<size_t>(st.st_size);
}

} // namespace

MappedBufferPrivate::MappedBufferPrivate(int fd,
                                         const MappedBufferOptions &options) {
    const auto size = bufferSize(fd, options.size);
    if (size == 0) {
        return;
    }

#ifdef HAVE_SYS_MMAN_H
    if (options.mmapProtection) {
        void *mapping =
            mmap(nullptr, size, *options.mmapProtection, MAP_PRIVATE, fd, 0);
        if (mapping != MAP_FAILED) {
            data_ = {static_cast<std::byte *>(mapping), size};
            mapped_ = true;
            return;
        }
    }
#endif

    buffer_ = std::make_unique_for_overwrite<std::byte[]>(size);
    data_ = {buffer_.get(), size};
    auto remaining = data_;
    while (!remaining.empty()) {
        const auto count =
            std::min(remaining.size(),
                     static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
        const auto bytesRead = fs::safeRead(fd, remaining.data(), count);
        if (bytesRead < 0) {
            throw std::system_error(errno, std::generic_category(),
                                    "Failed to read file descriptor");
        }
        if (bytesRead == 0) {
            throw std::runtime_error("Unexpected end of file");
        }
        remaining = remaining.subspan(bytesRead);
    }
}

std::optional<int> MappedBufferOptions::defaultMmapProtection() {
#ifdef HAVE_SYS_MMAN_H
    return PROT_READ;
#else
    return std::nullopt;
#endif
}

MappedBuffer::MappedBuffer() = default;

MappedBuffer::MappedBuffer(const UnixFD &fd, MappedBufferOptions options)
    : d_ptr(std::make_unique<MappedBufferPrivate>(fd.fd(), options)) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(MappedBuffer)

void MappedBuffer::reset() noexcept { d_ptr.reset(); }

const std::byte *MappedBuffer::data() const noexcept { return span().data(); }

size_t MappedBuffer::size() const noexcept { return span().size(); }

std::span<const std::byte> MappedBuffer::span() const noexcept {
    if (!d_ptr) {
        return {};
    }
    return d_ptr->data_;
}

std::span<std::byte> MappedBuffer::mutableSpan() noexcept {
    if (!d_ptr) {
        return {};
    }
    return d_ptr->data_;
}

} // namespace fcitx
