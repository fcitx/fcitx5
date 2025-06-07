/*
 * SPDX-FileCopyrightText: 2025~2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream> // IWYU pragma: keep
#include <iostream>
#include <istream>
#include <string>
#include <utility>
#include <vector>
#include "fcitx-utils/fdstreambuf.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/unixfd.h"
#include "testdir.h"

using namespace fcitx;

constexpr char filename[] = FCITX5_BINARY_DIR "/test/testfile";

// Uncomment to compare behavior
// #define TEST_USE_STD

int main() {
    std::filesystem::path path(filename);
    {
#ifndef TEST_USE_STD
        UnixFD outfd =
            StandardPaths::openPath(path, O_WRONLY | O_TRUNC | O_CREAT, 0600);

        OFDStreamBuf ostreamBuf(std::move(outfd));
        std::ostream out(&ostreamBuf);
#else
        std::ofstream out(path);
#endif

        for (int i = 0; i < 10000; i++) {
            char data;
            for (int j = 0; j < i % 10; j++) {
                data = 'a' + j;
                FCITX_ASSERT(out.write(&data, 1));
            }
            data = '\n';
            FCITX_ASSERT(out.write(&data, 1));
        }
    }

    {
#ifndef TEST_USE_STD
        UnixFD fd = StandardPaths::openPath(path);
        IFDStreamBuf streamBuf(std::move(fd));
        std::istream stream(&streamBuf);
#else
        std::ifstream stream(path);
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

        unlink(filename);
    }

    return 0;
}
