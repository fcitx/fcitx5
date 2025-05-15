/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <string>
#include <vector>
#include "fcitx-utils/environ.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpaths.h"
#include "testdir.h"

using namespace fcitx;

#define TEST_ADDON_DIR FCITX5_SOURCE_DIR "/test/addon"

void test_basic() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS",
                   "/TEST/PATH1/:/TEST/PATH2:/TEST/PATH2/:/TEST/PATH1");
    setEnvironment("XDG_DATA_HOME", "/TEST//PATH");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    StandardPaths standardPaths("fcitx5", {}, true, false);

    FCITX_ASSERT(standardPaths.userDirectory(StandardPathsType::Config) ==
                 "/TEST/PATH");
    // The order to the path should be kept for their first appearance.
    FCITX_ASSERT(standardPaths.directories(StandardPathsType::Config) ==
                 std::vector<std::filesystem::path>(
                     {"/TEST/PATH", "/TEST/PATH1", "/TEST/PATH2"}));
    FCITX_ASSERT(standardPaths.directories(StandardPathsType::PkgData) ==
                 std::vector<std::filesystem::path>(
                     {"/TEST/PATH/fcitx5",
                      std::filesystem::path(TEST_ADDON_DIR) / "fcitx5"}))
        << standardPaths.directories(StandardPathsType::PkgData);

    {
        auto result = standardPaths.locate(StandardPathsType::PkgData, "addon",
                                           pathfilter::extension(".conf"));
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf",
                                              "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
        }

        FCITX_ASSERT(names == expect_names) << names;
    }

    {
        auto result = standardPaths.locate(StandardPathsType::PkgData, "addon",
                                           pathfilter::extension(".conf"),
                                           StandardPaths::Mode::System);
        std::set<std::string> names;
        std::set<std::string> expect_names = {"testim.conf",
                                              "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
        }

        FCITX_ASSERT(names == expect_names);
    }

    std::filesystem::path filePath;
    auto file =
        standardPaths.open(StandardPathsType::PkgData, "addon/testim.conf",
                           StandardPaths::Mode::Default, &filePath);
    FCITX_ASSERT(filePath == std::filesystem::path(TEST_ADDON_DIR
                                                   "/fcitx5/addon/testim.conf")
                                 .lexically_normal());

    auto file2 = standardPaths.open(StandardPathsType::Data,
                                    "fcitx5/addon/testim2.conf");
    FCITX_ASSERT(!file2.isValid());
}

void test_nouser() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS", "/TEST/PATH1:/TEST/PATH2");
    setEnvironment("XDG_DATA_HOME", "/TEST//PATH");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    StandardPaths standardPaths("fcitx5", {}, true, true);

    std::filesystem::path path;
    auto file =
        standardPaths.open(StandardPathsType::PkgData, "addon/testim.conf",
                           StandardPaths::Mode::Default, &path);
    FCITX_ASSERT(path == std::filesystem::path(TEST_ADDON_DIR) /
                             "fcitx5/addon/testim.conf")
        << path;

    auto file2 = standardPaths.open(StandardPathsType::Data,
                                    "fcitx5/addon/testim2.conf");
    FCITX_ASSERT(file2.fd() == -1);
}

void test_custom() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS", "/TEST/PATH1:/TEST/PATH2");
    setEnvironment("XDG_DATA_HOME", "/TEST//PATH");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    StandardPaths path("mypackage", {{"datadir", "/TEST/PATH3"}}, false, false);
    FCITX_ASSERT(path.directories(fcitx::StandardPathsType::PkgConfig) ==
                 std::vector<std::filesystem::path>{"/TEST/PATH/mypackage",
                                                    "/TEST/PATH1/mypackage",
                                                    "/TEST/PATH2/mypackage"});
    FCITX_ASSERT(path.directories(fcitx::StandardPathsType::Data) ==
                 std::vector<std::filesystem::path>{
                     "/TEST/PATH", TEST_ADDON_DIR, "/TEST/PATH3"})
        << path.directories(fcitx::StandardPathsType::Data);
}

int main() {
    test_basic();
    test_nouser();
    test_custom();
    return 0;
}
