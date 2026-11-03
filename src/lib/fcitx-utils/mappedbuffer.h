/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UTILS_MAPPEDBUFFER_H_
#define _FCITX_UTILS_MAPPEDBUFFER_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/macros.h>
#include <span>

namespace fcitx {

class MappedBufferPrivate;
class UnixFD;

struct FCITXUTILS_EXPORT MappedBufferOptions {
    /// Return PROT_READ when mmap is supported, otherwise std::nullopt.
    static std::optional<int> defaultMmapProtection();

    /// Load this number of bytes, or the whole regular file if unset.
    std::optional<size_t> size = std::nullopt;

    /// mmap protection bits, such as PROT_READ. Unset disables mmap.
    std::optional<int> mmapProtection = defaultMmapProtection();
};

/**
 * Memory loaded from a file descriptor.
 *
 * Uses a private mmap when available, falling back to reading into an owned
 * buffer. The file descriptor is never owned and may be closed after loading.
 * The memory is released when this object is reset or destroyed.
 *
 * Directories are rejected. Non-regular file descriptors require an explicit
 * size because their stat size does not describe the available data.
 *
 * The descriptor must be readable and, if seekable, already positioned at the
 * beginning. Loading does not seek. The read fallback advances the file offset;
 * mmap leaves it unchanged. Non-seekable descriptors are read from their
 * current position and require an explicit size.
 *
 * The data is not null-terminated. A mapped file must not be truncated while
 * the memory is in use.
 *
 * @since 5.1.23
 */
class FCITXUTILS_EXPORT MappedBuffer {
public:
    /// Create an empty buffer.
    MappedBuffer();

    /**
     * Load data from fd.
     *
     * If read is used, fd will be read from its current position.
     * If mmap is used, offset is zero by default.
     * It is up to the to ensure fd is readable and positioned correctly. The
     * file descriptor is not owned.
     *
     * @throws std::system_error if a file operation fails.
     * @throws std::invalid_argument if fd refers to a directory, or to a
     * non-regular file when Options::size is unset.
     * @throws std::length_error if the file size cannot be represented.
     * @throws std::runtime_error on premature EOF in the read fallback.
     * @throws std::bad_alloc if memory allocation fails.
     */
    explicit MappedBuffer(const UnixFD &fd, MappedBufferOptions options = {});

    MappedBuffer(const MappedBuffer &) = delete;
    MappedBuffer &operator=(const MappedBuffer &) = delete;
    FCITX_DECLARE_MOVE(MappedBuffer);
    ~MappedBuffer();

    /// Release the memory and make the object empty.
    void reset() noexcept;

    /// The loaded bytes, or nullptr if empty, reset, or moved from.
    const std::byte *data() const noexcept;

    /// Number of loaded bytes, or zero if empty, reset, or moved from.
    size_t size() const noexcept;

    /// Read-only view of the loaded bytes, invalidated when memory is released.
    std::span<const std::byte> span() const noexcept;

    /**
     * Mutable view of the loaded bytes, invalidated when memory is released.
     *
     * User should make sure not change data if it is mapped as read-only.
     */
    std::span<std::byte> mutableSpan() noexcept;

private:
    std::unique_ptr<MappedBufferPrivate> d_ptr;
};

} // namespace fcitx

#endif // _FCITX_UTILS_MAPPEDBUFFER_H_
