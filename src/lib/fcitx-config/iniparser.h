/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_INIPARSER_H_
#define _FCITX_CONFIG_INIPARSER_H_

#include <cstdio>
#include <filesystem>
#include <istream>
#include <ostream>
#include <string>
#include <fcitx-config/fcitxconfig_export.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/standardpaths.h>

namespace fcitx {
class Configuration;
FCITXCONFIG_EXPORT void readFromIni(RawConfig &config, int fd);
FCITXCONFIG_EXPORT bool writeAsIni(const RawConfig &config, int fd);
FCITXCONFIG_EXPORT void readFromIni(RawConfig &config, FILE *fin);
FCITXCONFIG_EXPORT bool writeAsIni(const RawConfig &config, FILE *fout);

/**
 * Read raw config from std::istream
 *
 * @param config raw config
 * @param in input stream
 * @since 5.1.13
 */
FCITXCONFIG_EXPORT void readFromIni(RawConfig &config, std::istream &in);

/**
 * Write raw config to std::istream
 *
 * @param config raw config
 * @param out out stream
 * @since 5.1.13
 */
FCITXCONFIG_EXPORT bool writeAsIni(const RawConfig &config, std::ostream &out);

FCITXCONFIG_EXPORT void readAsIni(Configuration &configuration,
                                  const std::string &path);
FCITXCONFIG_EXPORT void readAsIni(RawConfig &rawConfig,
                                  const std::string &path);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const Configuration &configuration,
                                      const std::string &path);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const RawConfig &rawConfig,
                                      const std::string &path);

FCITXCONFIG_EXPORT void readAsIni(Configuration &configuration,
                                  StandardPathsType type,
                                  const std::filesystem::path &path);
FCITXCONFIG_EXPORT void readAsIni(RawConfig &rawConfig, StandardPathsType type,
                                  const std::filesystem::path &path);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const Configuration &configuration,
                                      StandardPathsType type,
                                      const std::filesystem::path &path);
FCITXCONFIG_EXPORT bool safeSaveAsIni(const RawConfig &rawConfig,
                                      StandardPathsType type,
                                      const std::filesystem::path &path);

// API compatible with old standardpath, while not pull in standardpath.h
template <typename T,
          typename = typename StandardPathsTypeConverter<T>::self_type>
void readAsIni(Configuration &configuration, T type, const std::string &path) {
    readAsIni(configuration, StandardPathsTypeConverter<T>::convert(type),
              path);
}

template <typename T,
          typename = typename StandardPathsTypeConverter<T>::self_type>
void readAsIni(RawConfig &rawConfig, T type, const std::string &path) {
    readAsIni(rawConfig, StandardPathsTypeConverter<T>::convert(type), path);
}

template <typename T,
          typename = typename StandardPathsTypeConverter<T>::self_type>
bool safeSaveAsIni(const Configuration &configuration, T type,
                   const std::string &path) {
    return safeSaveAsIni(configuration,
                         StandardPathsTypeConverter<T>::convert(type), path);
}

template <typename T,
          typename = typename StandardPathsTypeConverter<T>::self_type>
bool safeSaveAsIni(const RawConfig &rawConfig, T type,
                   const std::string &path) {
    return safeSaveAsIni(rawConfig,
                         StandardPathsTypeConverter<T>::convert(type), path);
}

} // namespace fcitx

#endif // _FCITX_CONFIG_INIPARSER_H_
