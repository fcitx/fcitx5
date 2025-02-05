#include "fcitx-utils/fdstreambuf.h"
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <ios>
#include <limits>
#include <memory>
#include <streambuf>
#include <string>
#include <utility>
#include <bits/types/mbstate_t.h>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/unixfd.h"

namespace fcitx {

static inline constexpr int BufferSize = 4096;
static inline constexpr int PutBackSize = 4;

// Wrapper handling partial write.
static std::streamsize xwrite(int fd, const char *s, std::streamsize n) {
    std::streamsize nleft = n;

    for (;;) {
        const std::streamsize ret = fs::safeWrite(fd, s, nleft);
        if (ret == -1L) {
            break;
        }
        nleft -= ret;
        if (nleft == 0) {
            break;
        }

        s += ret;
    }

    return n - nleft;
}

class IFdStreamBufPrivate : public QPtrHolder<IFdStreamBuf> {
public:
    IFdStreamBufPrivate(IFdStreamBuf *q) : QPtrHolder(q) {
        buffer_ = std::make_unique<char[]>(BufferSize + PutBackSize);
        resetBuffer(0, 0);
    }

    void resetBuffer(size_t nputback, size_t nread) {
        FCITX_Q();
        q->setg(buffer_.get() + (PutBackSize - nputback),
                buffer_.get() + PutBackSize,
                buffer_.get() + PutBackSize + nread);
    }

    int fd_ = -1;
    UnixFD fdOwner_;

    std::unique_ptr<char[]> buffer_;
};

IFdStreamBuf::IFdStreamBuf(UnixFD fd)
    : d_ptr(std::make_unique<IFdStreamBufPrivate>(this)) {
    FCITX_D();
    d->fd_ = fd.fd();
    d->fdOwner_ = std::move(fd);
}

IFdStreamBuf::IFdStreamBuf(int fd)
    : d_ptr(std::make_unique<IFdStreamBufPrivate>(this)) {
    FCITX_D();
    d->fd_ = fd;
}

IFdStreamBuf::~IFdStreamBuf() {}

bool IFdStreamBuf::is_open() const noexcept {
    FCITX_D();
    return d->fd_ != -1;
}

int IFdStreamBuf::fd() const noexcept {
    FCITX_D();
    return d->fd_;
}

IFdStreamBuf *IFdStreamBuf::close() {
    FCITX_D();
    d->fd_ = -1;
    d->fdOwner_.reset();
    return this;
}

IFdStreamBuf::int_type IFdStreamBuf::underflow() {
    FCITX_D();
    if (gptr() >= egptr()) {

        // Move the putback_size most-recently-read characters into the putback
        // area
        size_t nputback = std::min<size_t>(gptr() - eback(), PutBackSize);
        std::memmove(d->buffer_.get() + (PutBackSize - nputback),
                     gptr() - nputback, nputback);

        // Now read new characters from the file descriptor
        auto nread =
            fs::safeRead(d->fd_, d->buffer_.get() + PutBackSize, BufferSize);
        if (nread <= 0) {
            // EOF
            return traits_type::eof();
        }

        // Reset the buffer
        d->resetBuffer(nputback, nread);
    }

    // Return the next character
    return traits_type::to_int_type(*gptr());
}

std::streamsize IFdStreamBuf::xsgetn(char *s, std::streamsize n) {
    FCITX_D();
    // Use heuristic to decide whether to read directly
    // Read directly only if n >= bytes_available + 4096

    std::streamsize bytes_available = egptr() - gptr();

    if (n < bytes_available + BufferSize) {
        // Not worth it to do a direct read
        return std::streambuf::xsgetn(s, n);
    }

    std::streamsize total_bytes_read = 0;

    // First, copy out the bytes currently in the buffer
    s = std::copy(gptr(), egptr(), s);
    n -= bytes_available;
    total_bytes_read += bytes_available;

    // Now do the direct read
    while (n > 0) {
        const auto bytesRead = fs::safeRead(d->fd_, s, n);
        if (bytesRead <= 0) {
            // EOF
            break;
        }

        s += bytesRead;
        n -= bytesRead;
        total_bytes_read += bytesRead;
    }

    // Fill up the putback area with the most recently read characters
    size_t nputback = std::min<size_t>(total_bytes_read, PutBackSize);
    std::memcpy(d->buffer_.get() + (PutBackSize - nputback), s - nputback,
                nputback);

    // Reset the buffer with no bytes available for reading, but with some
    // putback characters
    d->resetBuffer(nputback, 0);

    // Return the total number of bytes read
    return total_bytes_read;
}

IFdStreamBuf::pos_type
IFdStreamBuf::seekoff(off_type off, std::ios_base::seekdir dir,
                      std::ios_base::openmode /*unused*/) {
    FCITX_D();
    if (!is_open()) {
        return -1L;
    }

    if (off != 0 || dir != std::ios_base::cur) {
        d->resetBuffer(0, 0);
    }

    if constexpr (sizeof(off_type) > sizeof(off_t)) {
        if (off > std::numeric_limits<off_t>::max() ||
            off < std::numeric_limits<off_t>::min()) {
            return -1L;
        }
    }
    return lseek(fd(), off, dir);
}

IFdStreamBuf::pos_type
IFdStreamBuf::seekpos(pos_type pos, std::ios_base::openmode /*unused*/) {
    std::fpos<mbstate_t> f;
    return seekoff(pos - pos_type(0), std::ios_base::beg);
}

} // namespace fcitx
