/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testing.h"
#include <cstdlib>
#include "standardpath.h"
#include "stringutils.h"

namespace fcitx {

void setupTestingEnvironment(const std::string &testBinaryDir,
                             const std::vector<std::string> &addonDirs,
                             const std::vector<std::string> &dataDirs) {
    // Skip resolution with fcitxPath
    setenv("SKIP_FCITX_PATH", "1", 1);
    setenv("SKIP_FCITX_USER_PATH", "1", 1);
    // Path to addon library
    std::vector<std::string> fullAddonDirs;
    for (const auto &addonDir : addonDirs) {
        if (addonDir.empty()) {
            continue;
        }
        if (addonDir[0] == '/') {
            fullAddonDirs.push_back(addonDir);
        } else {
            fullAddonDirs.push_back(
                stringutils::joinPath(testBinaryDir, addonDir));
        }
    }
    fullAddonDirs.push_back(StandardPath::fcitxPath("addondir"));

    setenv("FCITX_ADDON_DIRS", stringutils::join(fullAddonDirs, ":").data(), 1);
    // Make sure we don't write to user data.
    setenv("FCITX_DATA_HOME", "/Invalid/Path", 1);
    // Make sure we don't write to user data.
    setenv("FCITX_CONFIG_HOME", "/Invalid/Path", 1);
    // Make sure we can find addon files.
    // Path to addon library
    std::vector<std::string> fullDataDirs;
    for (const auto &dataDir : dataDirs) {
        if (dataDir.empty()) {
            continue;
        }
        if (dataDir[0] == '/') {
            fullDataDirs.push_back(dataDir);
        } else {
            fullDataDirs.push_back(
                stringutils::joinPath(testBinaryDir, dataDir));
        }
    }
    // Include the three testing only addons.
    fullDataDirs.push_back(StandardPath::fcitxPath("pkgdatadir", "testing"));
    setenv("FCITX_DATA_DIRS", stringutils::join(fullDataDirs, ":").data(), 1);
}

} // namespace fcitx