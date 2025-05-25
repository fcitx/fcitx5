/*
 * SPDX-FileCopyrightText: 2025-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <string>
#include "fcitx-config/fcitxconfig_export.h"
#include "fcitx-utils/standardpath.h"
#include "configuration.h"
#include "iniparser.h"
#include "rawconfig.h"

namespace fcitx {

// FIXME: Remove this deprecated API in future releases.
FCITXCONFIG_DEPRECATED_EXPORT void readAsIni(RawConfig &rawConfig,
                                             StandardPath::Type type,
                                             const std::string &path) {
    const auto &standardPath = StandardPath::global();
    auto file = standardPath.open(type, path, O_RDONLY);
    readFromIni(rawConfig, file.fd());
}

FCITXCONFIG_DEPRECATED_EXPORT void readAsIni(Configuration &configuration,
                                             StandardPath::Type type,
                                             const std::string &path) {
    RawConfig config;
    const auto &standardPath = StandardPath::global();
    auto file = standardPath.open(type, path, O_RDONLY);
    readFromIni(config, file.fd());
    configuration.load(config);
}

FCITXCONFIG_DEPRECATED_EXPORT bool safeSaveAsIni(const RawConfig &config,
                                                 StandardPath::Type type,
                                                 const std::string &path) {
    const auto &standardPath = StandardPath::global();
    return standardPath.safeSave(
        type, path, [&config](int fd) { return writeAsIni(config, fd); });
}

FCITXCONFIG_DEPRECATED_EXPORT bool
safeSaveAsIni(const Configuration &configuration, StandardPath::Type type,
              const std::string &path) {
    RawConfig config;

    configuration.save(config);
    const auto &standardPath = StandardPath::global();
    return standardPath.safeSave(
        type, path, [&config](int fd) { return writeAsIni(config, fd); });
}

} // namespace fcitx
