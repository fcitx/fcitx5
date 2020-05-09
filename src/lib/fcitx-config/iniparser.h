/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_INIPARSER_H_
#define _FCITX_CONFIG_INIPARSER_H_

#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/standardpath.h>
#include "fcitxconfig_export.h"

namespace fcitx {
class Configuration;
FCITXCONFIG_EXPORT void readFromIni(RawConfig &config, int fd);
FCITXCONFIG_EXPORT bool writeAsIni(const RawConfig &config, int fd);
FCITXCONFIG_EXPORT void readFromIni(RawConfig &config, FILE *fin);
FCITXCONFIG_EXPORT bool writeAsIni(const RawConfig &config, FILE *fout);
FCITXCONFIG_EXPORT void readAsIni(Configuration &, const std::string &name);
FCITXCONFIG_EXPORT void readAsIni(RawConfig &, const std::string &name);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const Configuration &,
                                      const std::string &name);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const RawConfig &,
                                      const std::string &name);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const Configuration &,
                                      StandardPath::Type type,
                                      const std::string &name);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const RawConfig &,
                                      StandardPath::Type type,
                                      const std::string &name);
} // namespace fcitx

#endif // _FCITX_CONFIG_INIPARSER_H_
