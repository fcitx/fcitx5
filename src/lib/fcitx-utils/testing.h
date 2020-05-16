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
