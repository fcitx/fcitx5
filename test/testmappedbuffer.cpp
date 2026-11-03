/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <fcntl.h>
#include <cerrno>
#include <cstddef>
#include <fstream>
#include <ios>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include "fcitx-utils/log.h"
#include "fcitx-utils/mappedbuffer.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/unixfd.h"
#include "config.h"
#include "testdir.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

namespace {

using namespace fcitx;

static_assert(!std::is_copy_constructible_v<MappedBuffer>);
static_assert(!std::is_copy_assignable_v<MappedBuffer>);
static_assert(std::is_nothrow_move_constructible_v<MappedBuffer>);
static_assert(std::is_nothrow_move_assignable_v<MappedBuffer>);

void checkEmpty(const MappedBuffer &memory) {
    FCITX_ASSERT(memory.data() == nullptr);
    FCITX_ASSERT(memory.size() == 0);
    FCITX_ASSERT(memory.span().empty());
}

void checkData(const MappedBuffer &memory, std::string_view expected) {
    FCITX_ASSERT(memory.size() == expected.size());
    if (!expected.empty()) {
        FCITX_ASSERT(memory.data());
        FCITX_ASSERT(
            std::string_view(reinterpret_cast<const char *>(memory.data()),
                             memory.size()) == expected);
    }
}

template <typename Exception, typename Func>
void checkThrows(Func &&func) {
    bool caught = false;
    try {
        func();
    } catch (const Exception &) {
        caught = true;
    }
    FCITX_ASSERT(caught);
}

template <typename Func>
void checkBadFD(Func &&func) {
    bool caught = false;
    try {
        func();
    } catch (const std::system_error &error) {
        FCITX_ASSERT(error.code() ==
                     std::error_code(EBADF, std::generic_category()));
        caught = true;
    }
    FCITX_ASSERT(caught);
}

void testInvalidFD(MappedBufferOptions options) {
    const UnixFD invalid;
    checkBadFD([&] { MappedBuffer memory(invalid, options); });
}

void testDefault() {
    MappedBuffer memory;
    checkEmpty(memory);
}

std::string sourcePath() {
    return std::string(FCITX5_SOURCE_DIR) + "/test/testmappedbuffer.cpp";
}

std::string sourceContents(const std::string &path) {
    std::ifstream source(path, std::ios::binary);
    FCITX_ASSERT(source);
    return {std::istreambuf_iterator<char>(source),
            std::istreambuf_iterator<char>()};
}

UnixFD openSourceFile(const std::string &path) {
    auto file = StandardPaths::openPath(path);
    FCITX_ASSERT(file.isValid());
    return file;
}

void testEmptyFile(const std::string &path) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file, {.size = 0});
    checkData(memory, {});
}

void testFullFile(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file);
    checkData(memory, contents);
}

#ifdef HAVE_SYS_MMAN_H
void testMmapFile(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file, {.mmapProtection = PROT_READ});
    checkData(memory, contents);
}
#endif

void testPartialFile(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file, {.size = 5});
    checkData(memory, contents.substr(0, 5));
}

void testMutableFile(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer writable(file, {.size = 5, .mmapProtection = std::nullopt});
    writable.mutableSpan()[0] = std::byte{'x'};
    std::string expected(contents.substr(0, 5));
    expected[0] = 'x';
    checkData(writable, expected);
}

void testOversizedFile(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    checkThrows<std::invalid_argument>(
        [&] { MappedBuffer buffer(file, {.size = contents.size() + 1}); });
}

void testFailedReplacement(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file);
    checkData(memory, contents);

    // A failed replacement leaves the previously constructed buffer intact.
    const UnixFD invalid;
    checkBadFD([&] { memory = MappedBuffer(invalid); });
    checkData(memory, contents);
}

void testMove(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file);
    const std::byte *data = memory.data();
    MappedBuffer moved(std::move(memory));
    checkEmpty(memory);
    FCITX_ASSERT(moved.data() == data);
    memory = std::move(moved);
    checkEmpty(moved);
    checkData(memory, contents);
}

void testFileLifetime(const std::string &path, std::string_view contents) {
    auto file = openSourceFile(path);
    MappedBuffer memory(file);
    // The memory survives closing the caller-owned descriptor.
    file.reset();
    checkData(memory, contents);
    memory.reset();
    checkEmpty(memory);
    memory.reset();
}

void testDirectory(MappedBufferOptions options) {
    auto directory = UnixFD::own(open(".", O_RDONLY));
    if (directory.isValid()) {
        checkThrows<std::invalid_argument>(
            [&] { MappedBuffer buffer(directory, options); });
    } else {
        FCITX_INFO() << "Unable to open a directory for testing.";
    }
}

} // namespace

int main() {
    testDefault();
    testInvalidFD({});
    testInvalidFD({.size = 16});
    testInvalidFD({.size = 0});
    const auto path = sourcePath();
    const auto contents = sourceContents(path);
    testEmptyFile(path);
    testFullFile(path, contents);
#ifdef HAVE_SYS_MMAN_H
    testMmapFile(path, contents);
#endif
    testPartialFile(path, contents);
    testMutableFile(path, contents);
    testOversizedFile(path, contents);
    testFailedReplacement(path, contents);
    testMove(path, contents);
    testFileLifetime(path, contents);
    testDirectory({});
    testDirectory({.size = 1});
    testDirectory({.size = 0});

    return 0;
}
