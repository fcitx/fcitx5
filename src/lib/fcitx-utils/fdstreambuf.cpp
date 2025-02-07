/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "fdstreambuf.h"
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <ios>
#include <limits>
#include <memory>
#include <streambuf>
#include <utility>
#include "config.h"
#include "fs.h"
#include "macros.h"
#include "unixfd.h"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

namespace fcitx {

namespace {

inline constexpr int IBufferSize = 8192;
inline constexpr int OBufferSize = 8192;

// Wrapper handling partial write.
std::streamsize xwrite(int fd, const char *s, std::streamsize bytesToWrite) {
    const std::streamsize n = bytesToWrite;

    while (true) {
        const std::streamsize bytesWritten = fs::safeWrite(fd, s, bytesToWrite);
        if (bytesWritten < 0) {
            break;
        }
        bytesToWrite -= bytesWritten;
        if (bytesToWrite == 0) {
            break;
        }

        s += bytesWritten;
    }

    return n - bytesToWrite;
}

#ifdef HAVE_SYS_UIO_H
std::streamsize xwritev(int fd, const char *s1, std::streamsize bytesToWrite1,
                        const char *s2, std::streamsize bytesToWrite2) {
    assert(bytesToWrite1 >= 0 && bytesToWrite2 >= 0);
    const std::streamsize n = bytesToWrite1 + bytesToWrite2;

    struct iovec iov[2];
    iov[1].iov_base = const_cast<char *>(s2);
    iov[1].iov_len = bytesToWrite2;

    // Use writev to write s1 & s2
    while (bytesToWrite1 > 0) {
        iov[0].iov_base = const_cast<char *>(s1);
        iov[0].iov_len = bytesToWrite1;

        std::streamsize bytesWritten;
        do {
            bytesWritten = writev(fd, iov, 2);
        } while (bytesWritten == -1 && errno == EINTR);
        if (bytesWritten < 0) {
            break;
        }

        if (bytesToWrite1 < bytesWritten) {
            // s2 is also partially done, update bytesToWrite2.
            const std::streamsize bytesWritten2 = bytesWritten - bytesToWrite1;
            bytesToWrite2 -= bytesWritten2;
            s2 += bytesToWrite2;
            bytesToWrite1 = 0;
        } else {
            bytesToWrite1 -= bytesWritten;
        }
    }

    if (bytesToWrite1 == 0 && bytesToWrite2 > 0) {
        bytesToWrite2 -= xwrite(fd, s2, bytesToWrite2);
    }

    return n - bytesToWrite1 - bytesToWrite2;
}
#endif

} // namespace

class IFDStreamBufPrivate : public QPtrHolder<IFDStreamBuf> {
public:
    IFDStreamBufPrivate(IFDStreamBuf *q) : QPtrHolder(q) {
        buffer_ = std::make_unique<char[]>(IBufferSize);
        resetBuffer(0);
    }

    void resetBuffer(size_t nread) {
        FCITX_Q();
        q->setg(buffer_.get(), buffer_.get(), buffer_.get() + nread);
    }

    int fd_ = -1;
    UnixFD fdOwner_;

    std::unique_ptr<char[]> buffer_;
};

IFDStreamBuf::IFDStreamBuf(UnixFD fd)
    : d_ptr(std::make_unique<IFDStreamBufPrivate>(this)) {
    FCITX_D();
    d->fd_ = fd.fd();
    d->fdOwner_ = std::move(fd);
}

IFDStreamBuf::IFDStreamBuf(int fd)
    : d_ptr(std::make_unique<IFDStreamBufPrivate>(this)) {
    FCITX_D();
    d->fd_ = fd;
}

IFDStreamBuf::~IFDStreamBuf() {}

bool IFDStreamBuf::is_open() const noexcept {
    FCITX_D();
    return d->fd_ != -1;
}

int IFDStreamBuf::fd() const noexcept {
    FCITX_D();
    return d->fd_;
}

IFDStreamBuf *IFDStreamBuf::close() {
    FCITX_D();
    d->fd_ = -1;
    d->fdOwner_.reset();
    return this;
}

IFDStreamBuf::int_type IFDStreamBuf::underflow() {
    FCITX_D();
    if (gptr() >= egptr()) {
        auto bytesRead = fs::safeRead(d->fd_, d->buffer_.get(), IBufferSize);
        if (bytesRead <= 0) {
            // EOF
            return traits_type::eof();
        }

        // Reset the buffer
        d->resetBuffer(bytesRead);
    }

    // Return the next character
    return traits_type::to_int_type(*gptr());
}

std::streamsize IFDStreamBuf::xsgetn(char *s, std::streamsize bytesToRead) {
    FCITX_D();
    const std::streamsize bytesAvailable = egptr() - gptr();

    if (bytesToRead < bytesAvailable + IBufferSize) {
        // For small read, reuse std::streambuf logic
        // It will call overflow().
        return std::streambuf::xsgetn(s, bytesToRead);
    }

    assert(bytesToRead >= bytesAvailable);
    const std::streamsize n = bytesToRead;

    // Copy all existing buffer to output
    s = std::copy(gptr(), egptr(), s);
    bytesToRead -= bytesAvailable;

    while (bytesToRead > 0) {
        const auto bytesRead = fs::safeRead(d->fd_, s, bytesToRead);
        if (bytesRead <= 0) {
            // EOF
            break;
        }

        s += bytesRead;
        bytesToRead -= bytesRead;
    }

    d->resetBuffer(0);

    return n - bytesToRead;
}

IFDStreamBuf::pos_type
IFDStreamBuf::seekoff(off_type off, std::ios_base::seekdir dir,
                      std::ios_base::openmode /*unused*/) {
    FCITX_D();
    if (!is_open()) {
        return -1;
    }

    if (off != 0 || dir != std::ios_base::cur) {
        d->resetBuffer(0);
    }

    if constexpr (sizeof(off_type) > sizeof(off_t)) {
        if (off > std::numeric_limits<off_t>::max() ||
            off < std::numeric_limits<off_t>::min()) {
            return -1;
        }
    }
    return lseek(fd(), off, dir);
}

IFDStreamBuf::pos_type
IFDStreamBuf::seekpos(pos_type pos, std::ios_base::openmode /*unused*/) {
    std::fpos<mbstate_t> f;
    return seekoff(pos - pos_type(0), std::ios_base::beg);
}

class OFDStreamBufPrivate : public QPtrHolder<OFDStreamBuf> {
public:
    OFDStreamBufPrivate(OFDStreamBuf *q) : QPtrHolder(q) {
        buffer_ = std::make_unique<char[]>(OBufferSize);
        resetBuffer();
    }

    void resetBuffer() {
        FCITX_Q();
        q->setp(buffer_.get(), buffer_.get() + OBufferSize - 1);
    }

    int fd_ = -1;
    UnixFD fdOwner_;

    std::unique_ptr<char[]> buffer_;
};

OFDStreamBuf::OFDStreamBuf(UnixFD fd)
    : d_ptr(std::make_unique<OFDStreamBufPrivate>(this)) {
    FCITX_D();
    d->fd_ = fd.fd();
    d->fdOwner_ = std::move(fd);
}

OFDStreamBuf::OFDStreamBuf(int fd)
    : d_ptr(std::make_unique<OFDStreamBufPrivate>(this)) {
    FCITX_D();
    d->fd_ = fd;
}

OFDStreamBuf::~OFDStreamBuf() {
    if (is_open()) {
        sync();
    }
}

bool OFDStreamBuf::is_open() const noexcept {
    FCITX_D();
    return d->fd_ != -1;
}

int OFDStreamBuf::fd() const noexcept {
    FCITX_D();
    return d->fd_;
}

OFDStreamBuf *OFDStreamBuf::close() {
    FCITX_D();
    d->fd_ = -1;
    d->fdOwner_.reset();

    return this;
}

OFDStreamBuf::int_type OFDStreamBuf::overflow(int_type ch) {
    FCITX_D();
    const char *p = pbase();
    std::streamsize bytesToWrite = pptr() - p;

    const bool isEOF = traits_type::eq_int_type(ch, traits_type::eof());

    if (!isEOF) {
        // We always reserver 1 byte in buffer, so even if
        // pptr == epptr, we can still write 1 byte.
        *pptr() = ch;
        ++bytesToWrite;
    }

    const auto bytesWritten = xwrite(d->fd_, p, bytesToWrite);
    d->resetBuffer();

    if (bytesToWrite == bytesWritten) {
        return traits_type::not_eof(ch);
    }
    return traits_type::eof();
}

int OFDStreamBuf::sync() {
    const auto ret = overflow(traits_type::eof());
    return traits_type::eq_int_type(ret, traits_type::eof()) ? 0 : -1;
}

std::streamsize OFDStreamBuf::xsputn(const char *s, std::streamsize n) {
    FCITX_D();
    const std::streamsize bufferCapacity = epptr() - pptr();
    if (n < std::min<std::streamsize>(OBufferSize / 2, bufferCapacity)) {
        // For small write, reuse std::streambuf logic.
        return std::streambuf::xsputn(s, n);
    }

    // Flush buffer if not empty.
    if (pbase() != pptr()) {
#ifdef HAVE_SYS_UIO_H
        const std::streamsize bytesInBuffer = pptr() - pbase();
        const std::streamsize bytesWritten =
            xwritev(d->fd_, pbase(), bytesInBuffer, s, n);
        d->resetBuffer();
        // Return only bytes written belong to s.
        return (bytesWritten > bytesInBuffer) ? (bytesWritten - bytesInBuffer)
                                              : 0;
#else
        if (sync() < 0) {
            return 0;
        }
#endif
    }

    return xwrite(d->fd_, s, n);
}

OFDStreamBuf::pos_type
OFDStreamBuf::seekoff(off_type off, std::ios_base::seekdir dir,
                      std::ios_base::openmode /*unused*/) {
    FCITX_D();
    if (!is_open()) {
        return -1;
    }

    if (off != 0 || dir != std::ios_base::cur) {
        d->resetBuffer();
    }

    if constexpr (sizeof(off_type) > sizeof(off_t)) {
        if (off > std::numeric_limits<off_t>::max() ||
            off < std::numeric_limits<off_t>::min()) {
            return -1;
        }
    }
    return lseek(fd(), off, dir);
}

OFDStreamBuf::pos_type
OFDStreamBuf::seekpos(pos_type pos, std::ios_base::openmode /*unused*/) {
    std::fpos<mbstate_t> f;
    return seekoff(pos - pos_type(0), std::ios_base::beg);
}

} // namespace fcitx
