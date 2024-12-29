/*
 * SPDX-FileCopyrightText: 2016-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_UNIXFD_H_
#define _FCITX_UTILS_UNIXFD_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Utility class to handle unix file decriptor.

#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>

namespace fcitx {

/// \brief Class wrap around the unix fd.
class FCITXUTILS_EXPORT UnixFD {
public:
    /// \brief Create an empty UnixFD.
    UnixFD() noexcept;

    /// \brief Create a UnixFD by using dup.
    explicit UnixFD(int fd);
    /**
     * Create UnixFD with dup, with give parameter to dup.
     *
     * @param fd file descriptor to duplicate.
     * @param min minimum file descriptor number
     * @since 5.1.6
     */
    explicit UnixFD(int fd, int min);
    UnixFD(const UnixFD &other) = delete;
    FCITX_DECLARE_MOVE(UnixFD);
    ~UnixFD() noexcept;

    /// \brief Create a UnixFD by owning the fd.
    static UnixFD own(int fd) {
        UnixFD unixFD;
        unixFD.give(fd);
        return unixFD;
    }

    /// \brief Check if fd is not empty.
    bool isValid() const noexcept;

    /// \brief Set a new FD.
    ///
    /// if fd is -1, reset it. Otherwise use dup to make copy.
    void set(int fd);

    /**
     * Set a new FD.
     *
     * if fd is -1, reset it. Otherwise use dup to make copy.
     *
     * @param fd file descriptor to duplicate.
     * @param min minimum file descriptor number
     * @since 5.1.6
     */
    void set(int fd, int min);

    /// \brief Clear the FD and close it.
    void reset() noexcept;

    /// \brief Get the internal fd and release the ownership.
    int release() noexcept;

    /// \brief Get the internal fd.
    int fd() const noexcept;

    /// \brief Set a new FD and transfer the ownership to UnixFD.
    void give(int fd) noexcept;

private:
    int fd_ = -1;
};

static inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                            const UnixFD &fd) {
    builder << "UnixFD(fd=" << fd.fd() << ")";
    return builder;
}
} // namespace fcitx

#endif // _FCITX_UTILS_UNIXFD_H_
