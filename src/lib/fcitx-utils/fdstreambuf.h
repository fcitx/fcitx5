/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <ios>
#include <memory>
#include <streambuf>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/unixfd.h>

namespace fcitx {

class IFdStreamBufPrivate;

class FCITXUTILS_EXPORT IFdStreamBuf : public std::streambuf {

public:
    using base_type = std::streambuf;
    using char_type = base_type::char_type;
    using traits_type = base_type::traits_type;
    using int_type = base_type::int_type;
    using pos_type = base_type::pos_type;
    using off_type = base_type::off_type;

    IFdStreamBuf(UnixFD fd);
    IFdStreamBuf(int fd);
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(IFdStreamBuf);

    bool is_open() const noexcept;

    IFdStreamBuf *close();

    int fd() const noexcept;

protected:
    int_type underflow() override;
    std::streamsize xsgetn(char *s, std::streamsize n) override;
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode = std::ios_base::in |
                                               std::ios_base::out) override;
    pos_type seekpos(pos_type pos,
                     std::ios_base::openmode = std::ios_base::in |
                                               std::ios_base::out) override;

private:
    FCITX_DECLARE_PRIVATE(IFdStreamBuf);
    std::unique_ptr<IFdStreamBufPrivate> d_ptr;
};

} // namespace fcitx