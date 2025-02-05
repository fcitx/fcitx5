/*
 * SPDX-FileCopyrightText: 2025~2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <istream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>
#include "fcitx-utils/fdstreambuf.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/unixfd.h"
#include "testdir.h"

constexpr char filename[] = FCITX5_BINARY_DIR "/test/testfile";

int main() {
    fcitx::UniqueFilePtr file(fopen(filename, "wb"));

    for (int i = 0; i < 10000; i++) {
        char data;
        for (int j = 0; j < i % 10; j++) {
            data = 'a' + j;
            fwrite(&data, 1, 1, file.get());
        }
        data = '\n';
        fwrite(&data, 1, 1, file.get());
    }
    file.reset();

#if 1
    fcitx::UnixFD fd(open(filename, O_RDONLY));
    fcitx::IFdStreamBuf streamBuf(std::move(fd));
    std::istream stream(&streamBuf);
#else
    std::fstream stream(filename, std::ios::in);
#endif

    std::string line;
    int linenumber = 0;
    while (std::getline(stream, line)) {
        std::string expected;
        for (int j = 0; j < linenumber % 10; j++) {
            expected.push_back('a' + j);
        }
        FCITX_ASSERT(expected == line) << line << " line:" << linenumber;
        linenumber++;
    }

    FCITX_ASSERT(!stream.bad());
    stream.clear();
    FCITX_ASSERT(stream.tellg() == 55000) << stream.tellg();

    FCITX_ASSERT(stream.seekg(0));
    FCITX_ASSERT(stream.tellg() == 0) << stream.tellg();

    FCITX_ASSERT(stream.seekg(0, std::ios::end));
    FCITX_ASSERT(stream.tellg() == 55000) << stream.tellg();

    FCITX_ASSERT(stream.seekg(-10000, std::ios::cur));
    FCITX_ASSERT(stream.tellg() == 45000) << stream.tellg();

    std::vector<char> buffer;
    buffer.resize(10000);
    stream.read(buffer.data(), 5000);
    stream.read(buffer.data() + 5000, 5000);
    buffer.push_back(0);
    std::cout << buffer.data();

    unlink(filename);

    return 0;
}
