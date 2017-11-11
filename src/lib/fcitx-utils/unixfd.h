/*
 * Copyright (C) 2016~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_UTILS_UNIXFD_H_
#define _FCITX_UTILS_UNIXFD_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Utility class to handle unix file decriptor.

#include "fcitxutils_export.h"
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace fcitx {

class UnixFDPrivate;

/// \brief Class wrap around the unix fd.
class FCITXUTILS_EXPORT UnixFD {
public:
    /// \brief Create an empty UnixFD.
    UnixFD() noexcept;

    /// \brief Create a UnixFD by using dup.
    explicit UnixFD(int fd);
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
}

#endif // _FCITX_UTILS_UNIXFD_H_
