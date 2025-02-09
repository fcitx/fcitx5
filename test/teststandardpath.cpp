/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
#include "fcitx-utils/environ.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpath.h"
#include "testdir.h"

using namespace fcitx;

#define TEST_ADDON_DIR FCITX5_SOURCE_DIR "/test/addon"

#ifdef _WIN32
#define PATH_DELIMITER ";"
#else
#define PATH_DELIMITER ":"
#endif

void test_basic() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS",
                   "/TEST/PATH1/" PATH_DELIMITER "/TEST/PATH2" PATH_DELIMITER
                   "/TEST/PATH2/" PATH_DELIMITER "/TEST/PATH1");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    StandardPath standardPath(true);

    FCITX_ASSERT(standardPath.userDirectory(StandardPath::Type::Config) ==
                 "/TEST/PATH");
    // The order to the path should be kept for their first appearance.
    FCITX_ASSERT(standardPath.directories(StandardPath::Type::Config) ==
                 std::vector<std::string>({"/TEST/PATH1", "/TEST/PATH2"}));

    {
        auto result = standardPath.multiOpen(
            StandardPath::Type::PkgData, "addon", O_RDONLY,
            filter::Not(filter::User()), filter::Suffix(".conf"));
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf",
                                              "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            FCITX_ASSERT(p.second.fd() >= 0);
        }

        FCITX_ASSERT(names == expect_names);
    }

    {
        auto result = standardPath.locate(StandardPath::Type::PkgData, "addon",
                                          filter::Not(filter::User()),
                                          filter::Suffix(".conf"));
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf",
                                              "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
        }

        FCITX_ASSERT(names == expect_names);
    }

    {
        auto result = standardPath.multiOpen(
            StandardPath::Type::PkgData, "addon", O_RDONLY,
            filter::Not(filter::User()), filter::Suffix("im.conf"));
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            FCITX_ASSERT(p.second.fd() >= 0);
        }

        FCITX_ASSERT(names == expect_names);
    }

    auto file = standardPath.open(StandardPath::Type::PkgData,
                                  "addon/testim.conf", O_RDONLY);
    FCITX_ASSERT(file.path() ==
                 fs::cleanPath(TEST_ADDON_DIR "/fcitx5/addon/testim.conf"));

    auto file2 = standardPath.open(StandardPath::Type::Data,
                                   "fcitx5/addon/testim2.conf", O_RDONLY);
    FCITX_ASSERT(file2.fd() == -1);
}

void test_nouser() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS",
                   "/TEST/PATH1" PATH_DELIMITER "/TEST/PATH2");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    StandardPath standardPath(true, true);

    FCITX_ASSERT(
        standardPath.userDirectory(StandardPath::Type::Config).empty());
    FCITX_ASSERT(standardPath.directories(StandardPath::Type::Config) ==
                 std::vector<std::string>({"/TEST/PATH1", "/TEST/PATH2"}));

    {
        auto result = standardPath.multiOpen(
            StandardPath::Type::PkgData, "addon", O_RDONLY,
            filter::Not(filter::User()), filter::Suffix(".conf"));
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf",
                                              "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            FCITX_ASSERT(p.second.fd() >= 0);
        }

        FCITX_ASSERT(names == expect_names);
    }

    {
        auto result = standardPath.multiOpen(
            StandardPath::Type::PkgData, "addon", O_RDONLY,
            filter::Not(filter::User()), filter::Suffix("im.conf"));
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            FCITX_ASSERT(p.second.fd() >= 0);
        }

        FCITX_ASSERT(names == expect_names);
    }

    auto file = standardPath.open(StandardPath::Type::PkgData,
                                  "addon/testim.conf", O_RDONLY);
    FCITX_ASSERT(file.path() ==
                 fs::cleanPath(TEST_ADDON_DIR "/fcitx5/addon/testim.conf"));

    auto file2 = standardPath.open(StandardPath::Type::Data,
                                   "fcitx5/addon/testim2.conf", O_RDONLY);
    FCITX_ASSERT(file2.fd() == -1);
}

void test_custom() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS",
                   "/TEST/PATH1" PATH_DELIMITER "/TEST/PATH2");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    StandardPath path("mypackage", {{"datadir", "/TEST/PATH3"}}, false, false);
    FCITX_ASSERT(path.directories(fcitx::StandardPath::Type::PkgConfig) ==
                 std::vector<std::string>{"/TEST/PATH1/mypackage",
                                          "/TEST/PATH2/mypackage"});
    FCITX_ASSERT(path.directories(fcitx::StandardPath::Type::Data) ==
                 std::vector<std::string>{TEST_ADDON_DIR, "/TEST/PATH3"})
        << path.directories(fcitx::StandardPath::Type::Data);
}

int main() {
    test_basic();
    test_nouser();
    test_custom();
    return 0;
}
