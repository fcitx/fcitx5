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
    StandardPaths standardPaths("fcitx5", {},
                                StandardPathsOption::SkipSystemPath);

    FCITX_ASSERT(standardPaths.userDirectory(StandardPathsType::Config)
                     .string()
                     .ends_with("/Fcitx5/config"));
    // The order to the path should be kept for their first appearance.
    FCITX_ASSERT(standardPaths.directories(StandardPathsType::Config).front() ==
                 standardPaths.userDirectory(StandardPathsType::Config));
    FCITX_ASSERT(standardPaths.directories(StandardPathsType::Config).back() ==
                 FCITX5_BINARY_DIR "/config");
    FCITX_ASSERT(standardPaths.directories(StandardPathsType::PkgData).back() ==
                 std::filesystem::path(FCITX5_BINARY_DIR) / "data/fcitx5");
}

void test_override() {
    StandardPaths standardPaths("fcitx5",
                                {{"pkgdatadir", {TEST_ADDON_DIR "/fcitx5"}}},
                                StandardPathsOption::SkipSystemPath);
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

int main() {
    test_basic();
    test_override();
    return 0;
}
