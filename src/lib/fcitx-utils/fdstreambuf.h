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

class IFDStreamBufPrivate;

class FCITXUTILS_EXPORT IFDStreamBuf : public std::streambuf {

public:
    using base_type = std::streambuf;
    using char_type = base_type::char_type;
    using traits_type = base_type::traits_type;
    using int_type = base_type::int_type;
    using pos_type = base_type::pos_type;
    using off_type = base_type::off_type;

    IFDStreamBuf(UnixFD fd);
    IFDStreamBuf(int fd);
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(IFDStreamBuf);

    bool is_open() const noexcept;

    IFDStreamBuf *close();

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
    FCITX_DECLARE_PRIVATE(IFDStreamBuf);
    std::unique_ptr<IFDStreamBufPrivate> d_ptr;
};

class OFDStreamBufPrivate;

class FCITXUTILS_EXPORT OFDStreamBuf : public std::streambuf {

public:
    using base_type = std::streambuf;
    using char_type = base_type::char_type;
    using traits_type = base_type::traits_type;
    using int_type = base_type::int_type;
    using pos_type = base_type::pos_type;
    using off_type = base_type::off_type;

    OFDStreamBuf(UnixFD fd);
    OFDStreamBuf(int fd);
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(OFDStreamBuf);

    bool is_open() const noexcept;

    OFDStreamBuf *close();

    int fd() const noexcept;

protected:
    int_type overflow(int_type ch = traits_type::eof()) override;
    int sync() override;
    std::streamsize xsputn(const char *s, std::streamsize n) override;
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode = std::ios_base::in |
                                               std::ios_base::out) override;
    pos_type seekpos(pos_type pos,
                     std::ios_base::openmode = std::ios_base::in |
                                               std::ios_base::out) override;

private:
    FCITX_DECLARE_PRIVATE(OFDStreamBuf);
    std::unique_ptr<OFDStreamBufPrivate> d_ptr;
};

} // namespace fcitx
