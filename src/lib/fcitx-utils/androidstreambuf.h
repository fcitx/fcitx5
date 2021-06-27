/*
 * SPDX-FileCopyrightText: 2021-2021 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef FCITX_ANDROIDSTREAMBUF_H
#define FCITX_ANDROIDSTREAMBUF_H

#include <cstring>
#include <iostream>
#include <android/log.h>
#include "stringutils.h"

static const char *tag = "fcitx5";
static const size_t androidBufSize = 512;

class AndroidStreamBuf : public std::streambuf {
public:
    explicit AndroidStreamBuf(size_t buf_size);
    ~AndroidStreamBuf() override;

    int overflow(int c) override;
    int sync() override;

private:
    const size_t buf_size_;
    char *pbuf_;
};

#endif // FCITX_ANDROIDSTREAMBUF_H
