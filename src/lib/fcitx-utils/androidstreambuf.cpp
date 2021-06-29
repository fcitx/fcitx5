/*
 * SPDX-FileCopyrightText: 2021-2021 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "androidstreambuf.h"

AndroidStreamBuf::AndroidStreamBuf(size_t buf_size) : buf_size_(buf_size) {
    assert(buf_size_ > 0);
    pbuf_ = new char[buf_size_];
    memset(pbuf_, 0, buf_size_);

    setp(pbuf_, pbuf_ + buf_size_);
}

AndroidStreamBuf::~AndroidStreamBuf() { delete pbuf_; }

int AndroidStreamBuf::sync() {
    auto str_buf = stringutils::trim(std::string(pbuf_));
    auto trim_pbuf = str_buf.c_str();

    int res = __android_log_write(ANDROID_LOG_DEBUG, tag, trim_pbuf);

    memset(pbuf_, 0, buf_size_);
    setp(pbase(), pbase() + buf_size_);
    pbump(0);
    return res;
}

int AndroidStreamBuf::overflow(int c) {
    if (-1 == sync()) {
        return traits_type::eof();
    } else {
        // put c into buffer after successful sync
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            sputc(traits_type::to_char_type(c));
        }

        return traits_type::not_eof(c);
    }
}