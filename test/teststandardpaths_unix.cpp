/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <algorithm>
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
    setEnvironment("XDG_CACHE_HOME", "/CACHE/PATH");
    setEnvironment("XDG_RUNTIME_DIR", "/RUNTIME/PATH");
    StandardPaths standardPaths("fcitx5", {},
                                StandardPathsOption::SkipBuiltInPath);

    FCITX_ASSERT(standardPaths.userDirectory(StandardPathsType::Config) ==
                 "/TEST/PATH");
    FCITX_ASSERT(standardPaths.userDirectory(StandardPathsType::Runtime) ==
                 "/RUNTIME/PATH");
    FCITX_ASSERT(standardPaths.userDirectory(StandardPathsType::Cache) ==
                 "/CACHE/PATH");
    // The order to the path should be kept for their first appearance.
    FCITX_ASSERT(
        std::ranges::equal(standardPaths.directories(StandardPathsType::Config),
                           std::vector<std::filesystem::path>(
                               {"/TEST/PATH", "/TEST/PATH1", "/TEST/PATH2"})));
    FCITX_ASSERT(std::ranges::equal(
        standardPaths.directories(StandardPathsType::PkgData),
        std::vector<std::filesystem::path>(
            {"/TEST/PATH/fcitx5",
             std::filesystem::path(TEST_ADDON_DIR) / "fcitx5"})))
        << standardPaths.directories(StandardPathsType::PkgData);

    {
        auto result = standardPaths.locate(StandardPathsType::PkgData, "addon",
                                           pathfilter::extension(".conf"));
        std::set<std::filesystem::path> names;
        std::set<std::filesystem::path> expect_names = {"testim.conf",
                                                        "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
        }

        FCITX_ASSERT(names == expect_names) << names;
    }

    {
        auto result = standardPaths.locate(StandardPathsType::PkgData, "addon",
                                           pathfilter::extension(".conf"),
                                           StandardPathsMode::System);
        std::set<std::filesystem::path> names;
        std::set<std::filesystem::path> expect_names = {"testim.conf",
                                                        "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
        }

        FCITX_ASSERT(names == expect_names);
    }

    {
        std::filesystem::path filePath;
        auto file =
            standardPaths.open(StandardPathsType::PkgData, "addon/testim.conf",
                               StandardPathsMode::Default, &filePath);
        FCITX_ASSERT(filePath == std::filesystem::path(
                                     TEST_ADDON_DIR "/fcitx5/addon/testim.conf")
                                     .lexically_normal());
    }

    {
        auto file2 = standardPaths.open(StandardPathsType::Data,
                                        "fcitx5/addon/testim2.conf");
        FCITX_ASSERT(!file2.isValid());
    }

    {
        std::vector<std::filesystem::path> filePaths;
        auto files = standardPaths.openAll(
            StandardPathsType::PkgData, "addon/testim.conf",
            StandardPathsMode::Default, &filePaths);
        FCITX_ASSERT(filePaths ==
                     std::vector<std::filesystem::path>{
                         std::filesystem::path(TEST_ADDON_DIR
                                               "/fcitx5/addon/testim.conf")
                             .lexically_normal()});
        auto files2 = standardPaths.locateAll(StandardPathsType::PkgData,
                                              "addon/testim.conf",
                                              StandardPathsMode::Default);
        FCITX_ASSERT(filePaths == files2);
    }
}

void test_nouser() {
    setEnvironment("XDG_CONFIG_HOME", "/TEST/PATH");
    setEnvironment("XDG_CONFIG_DIRS", "/TEST/PATH1:/TEST/PATH2");
    setEnvironment("XDG_DATA_HOME", "/TEST//PATH");
    setEnvironment("XDG_DATA_DIRS", TEST_ADDON_DIR);
    setEnvironment("XDG_CACHE_HOME", "/CACHE/PATH");
    setEnvironment("XDG_RUNTIME_DIR", "/RUNTIME/PATH");
    StandardPaths standardPaths(
        "fcitx5", {},
        StandardPathsOptions{StandardPathsOption::SkipUserPath,
                             StandardPathsOption::SkipBuiltInPath});
    FCITX_ASSERT(
        standardPaths.userDirectory(StandardPathsType::Runtime).empty());
    FCITX_ASSERT(standardPaths.userDirectory(StandardPathsType::Cache).empty());

    std::filesystem::path path;
    auto file =
        standardPaths.open(StandardPathsType::PkgData, "addon/testim.conf",
                           StandardPathsMode::Default, &path);
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
    StandardPaths path("mypackage", {{"datadir", {"/TEST/PATH3"}}},
                       StandardPathsOption::SkipBuiltInPath);
    FCITX_ASSERT(std::ranges::equal(
        path.directories(fcitx::StandardPathsType::PkgConfig),
        std::vector<std::filesystem::path>{"/TEST/PATH/mypackage",
                                           "/TEST/PATH1/mypackage",
                                           "/TEST/PATH2/mypackage"}))
        << path.directories(fcitx::StandardPathsType::PkgConfig);
    FCITX_ASSERT(std::ranges::equal(
        path.directories(fcitx::StandardPathsType::Data),
        std::vector<std::filesystem::path>{"/TEST/PATH", TEST_ADDON_DIR}))
        << path.directories(fcitx::StandardPathsType::Data);
}

int main() {
    test_basic();
    test_nouser();
    test_custom();
    return 0;
}
