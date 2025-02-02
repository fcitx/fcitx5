/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testing.h"
#include <cstdlib>
#include <string>
#include <vector>
#include "environ.h"
#include "standardpath.h"
#include "stringutils.h"

namespace fcitx {

void setupTestingEnvironment(const std::string &testBinaryDir,
                             const std::vector<std::string> &addonDirs,
                             const std::vector<std::string> &dataDirs) {
    // Skip resolution with fcitxPath
    setEnvironment("SKIP_FCITX_PATH", "1");
    setEnvironment("SKIP_FCITX_USER_PATH", "1");
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

    setEnvironment("FCITX_ADDON_DIRS",
                   stringutils::join(fullAddonDirs, ":").data());
    // Make sure we don't write to user data.
    setEnvironment("FCITX_DATA_HOME", "/Invalid/Path");
    // Make sure we don't write to user data.
    setEnvironment("FCITX_CONFIG_HOME", "/Invalid/Path");
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
    setEnvironment("FCITX_DATA_DIRS",
                   stringutils::join(fullDataDirs, ":").data());
}

} // namespace fcitx
