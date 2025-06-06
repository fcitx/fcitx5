/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testing.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <ranges>
#include "environ.h"
#include "standardpaths.h"
#include "standardpaths_p.h"
#include "stringutils.h"

namespace fcitx {

void setupTestingEnvironment(const std::string &testBinaryDir,
                             const std::vector<std::string> &addonDirs,
                             const std::vector<std::string> &dataDirs) {
    std::vector<std::filesystem::path> addonDirsPath;
    addonDirsPath.assign(addonDirs.begin(), addonDirs.end());
    std::vector<std::filesystem::path> dataDirsPath;
    dataDirsPath.assign(dataDirs.begin(), dataDirs.end());
    setupTestingEnvironmentPath(std::filesystem::path(testBinaryDir),
                                addonDirsPath, dataDirsPath);
}

void setupTestingEnvironmentPath(
    const std::filesystem::path &testBinaryDir,
    const std::vector<std::filesystem::path> &addonDirs,
    const std::vector<std::filesystem::path> &dataDirs) {

    // Path to addon library
    std::vector<std::filesystem::path> fullAddonDirs;
    for (const auto &addonDir : addonDirs) {
        if (addonDir.empty()) {
            continue;
        }
        if (addonDir.is_absolute()) {
            fullAddonDirs.push_back(addonDir);
        } else {
            fullAddonDirs.push_back(testBinaryDir / addonDir);
        }
    }
    // Add built-in path for testing addons.
    fullAddonDirs.push_back(StandardPaths::fcitxPath("addondir"));
    // Make sure we can find addon files.
    // Path to addon library
    std::vector<std::filesystem::path> fullDataDirs;
    for (const auto &dataDir : dataDirs) {
        if (dataDir.empty()) {
            continue;
        }
        if (dataDir.is_absolute()) {
            fullDataDirs.push_back(dataDir);
        } else {
            fullDataDirs.push_back(testBinaryDir / dataDir);
        }
    }
    // Include the three testing only addons.
    fullDataDirs.push_back(StandardPaths::fcitxPath("pkgdatadir", "testing"));

#ifndef _WIN32
    // Skip resolution with fcitxPath
    setEnvironment("SKIP_FCITX_PATH", "1");
    setEnvironment("SKIP_FCITX_USER_PATH", "1");

    setEnvironment(
        "FCITX_ADDON_DIRS",
        stringutils::join(fullAddonDirs |
                              std::views::transform([](const auto &path) {
                                  return path.string();
                              }),
                          ":")
            .data());
    // Make sure we don't write to user data.
    setEnvironment("FCITX_DATA_HOME", "/Invalid/Path");
    // Make sure we don't write to user data.
    setEnvironment("FCITX_CONFIG_HOME", "/Invalid/Path");
    setEnvironment(
        "FCITX_DATA_DIRS",
        stringutils::join(fullDataDirs |
                              std::views::transform([](const auto &path) {
                                  return path.string();
                              }),
                          ":")
            .data());
#endif

    StandardPathsPrivate::setGlobal(std::make_unique<StandardPaths>(
        "fcitx5",
        std::unordered_map<std::string, std::vector<std::filesystem::path>>{
            {"pkgdatadir", fullDataDirs},
            {"addondir", fullAddonDirs},
        },
        StandardPathsOptions{
            StandardPathsOption::SkipUserPath,
            StandardPathsOption::SkipSystemPath,
        }));
}

} // namespace fcitx
