/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_TESTING_H_
#define _FCITX_UTILS_TESTING_H_

#include <string>
#include <vector>
#include "fcitxutils_export.h"

namespace fcitx {

/**
 * Set corresponding environment variable to make sure fcitx can be run properly
 * for testing.
 *
 * This function will do following things:
 * 1. set SKIP_FCITX_PATH to 1, so StandardPath won't try to resolve it's
 * hardcoded installation path.
 * 2. Setup addonDirs, which should point to all the directories of shared
 * library that need to be loaded.
 * 3. Set user path to invalid path to prevent write user data.
 * 4. Set up the data dirs, in order to find necessary data files, addon's .conf
 * files, and setup the path to find the testing only addons.
 *
 * For example, you want to test a test addon A in current project. And your
 * code directory looks like: src/a a.conf.in test/ testa.cpp build/ : cmake
 * binary dir. Then you need to first create a cmake rule to make sure addon
 * config file follow the fcitx directory structure. Which is
 * $fcitx_data_dir/addon.
 *
 * This can be achieved by:
 * Create test/addon, and have a CMake rule to copy generated a.conf to
 * build/test/addon.
 *
 * Then you can invoke setupTestingEnvironment like:
 * setupTestingEnvironment(PATH_TO_build, {"src/a"}, {"test"});
 * So fcitx_data_dir will include build/test, so build/test/addon/a.conf will
 * loaded. And build/src/a will be used to locate build/src/a.so.
 *
 * @param testBinaryDir base directory for build data
 * @param addonDirs directory of addons
 * @param dataDirs directory of fcitx data
 */
FCITXUTILS_EXPORT void
setupTestingEnvironment(const std::string &testBinaryDir,
                        const std::vector<std::string> &addonDirs,
                        const std::vector<std::string> &dataDirs);

} // namespace fcitx

#endif // _FCITX_UTILS_TESTING_H_
