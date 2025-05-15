/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "iniparser.h"
#include <fcntl.h>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include "fcitx-config/fcitxconfig_export.h"
#include "fcitx-utils/fdstreambuf.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/unixfd.h"
#include "configuration.h"
#include "rawconfig.h"

namespace fcitx {

void readFromIni(RawConfig &config, FILE *fin) {
    if (!fin) {
        return;
    }
    readFromIni(config, fileno(fin));
}

bool writeAsIni(const RawConfig &config, FILE *fout) {
    if (!fout) {
        return false;
    }
    return writeAsIni(config, fileno(fout));
}

void readFromIni(RawConfig &config, int fd) {
    if (fd == -1) {
        return;
    }
    IFDStreamBuf buf(fd);
    std::istream in(&buf);
    readFromIni(config, in);
}

bool writeAsIni(const RawConfig &config, int fd) {
    if (fd == -1) {
        return false;
    }
    OFDStreamBuf buf(fd);
    std::ostream fout(&buf);
    return writeAsIni(config, fout);
}

void readFromIni(RawConfig &config, std::istream &in) {
    std::string currentGroup;

    std::string line;

    unsigned int lineNumber = 0;
    while (std::getline(in, line)) {
        lineNumber++;
        std::string_view lineBuf = stringutils::trimView(line);
        if (lineBuf.empty() || lineBuf.front() == '#') {
            continue;
        }

        if (lineBuf.front() == '[' && lineBuf.back() == ']') {
            currentGroup = lineBuf.substr(1, lineBuf.size() - 2);
            config.visitItemsOnPath(
                [lineNumber](RawConfig &config, const std::string &) {
                    if (!config.lineNumber()) {
                        config.setLineNumber(lineNumber);
                    }
                },
                currentGroup);
        } else if (std::string::size_type equalPos = lineBuf.find_first_of('=');
                   equalPos != std::string::npos) {
            auto name = lineBuf.substr(0, equalPos);
            auto valueStart = equalPos + 1;

            auto value = stringutils::unescapeForValue(
                std::string_view(lineBuf).substr(valueStart));
            if (!value) {
                continue;
            }

            RawConfigPtr subConfig;
            if (!currentGroup.empty()) {
                std::string s = currentGroup;
                s += "/";
                s += name;
                subConfig = config.get(s, true);
            } else {
                subConfig = config.get(std::string(name), true);
            }
            subConfig->setValue(*value);
            subConfig->setLineNumber(lineNumber);
        }
    }
}

bool writeAsIni(const RawConfig &config, std::ostream &out) {
    std::function<bool(const RawConfig &, const std::string &path)> callback;

    callback = [&out, &callback](const RawConfig &config,
                                 const std::string &path) {
        if (config.hasSubItems()) {
            std::string values;
            config.visitSubItems(
                [&values](const RawConfig &config, const std::string &) {
                    if (config.hasSubItems() && config.value().empty()) {
                        return true;
                    }

                    if (!config.comment().empty() &&
                        config.comment().find('\n') == std::string::npos) {
                        values += "# ";
                        values += config.comment();
                        values += "\n";
                    }

                    values += config.name();
                    values += "=";
                    values += stringutils::escapeForValue(config.value());
                    values += "\n";
                    return true;
                },
                "", false, path);
            if (!values.empty()) {
                if (!path.empty()) {
                    out << "[" << path << "]\n";
                    FCITX_RETURN_IF(!out, false);
                }
                out << values << "\n";
                FCITX_RETURN_IF(!out, false);
            }
        }
        config.visitSubItems(callback, "", false, path);
        return true;
    };

    return callback(config, "");
}

void readAsIni(RawConfig &rawConfig, const std::string &path) {
    readAsIni(rawConfig, StandardPathsType::PkgConfig, path);
}

void readAsIni(Configuration &configuration, const std::string &path) {
    readAsIni(configuration, StandardPathsType::PkgConfig, path);
}

bool safeSaveAsIni(const RawConfig &config, const std::string &path) {
    return safeSaveAsIni(config, StandardPathsType::PkgConfig, path);
}

bool safeSaveAsIni(const Configuration &configuration,
                   const std::string &path) {
    return safeSaveAsIni(configuration, StandardPathsType::PkgConfig, path);
}

void readAsIni(RawConfig &rawConfig, StandardPathsType type,
               const std::filesystem::path &path) {
    const auto &standardPath = StandardPaths::global();
    auto file = standardPath.open(type, path);
    readFromIni(rawConfig, file.fd());
}

void readAsIni(Configuration &configuration, StandardPathsType type,
               const std::filesystem::path &path) {
    RawConfig config;
    readAsIni(config, type, path);

    configuration.load(config);
}

bool safeSaveAsIni(const RawConfig &config, StandardPathsType type,
                   const std::filesystem::path &path) {
    const auto &standardPath = StandardPaths::global();
    return standardPath.safeSave(
        type, path, [&config](int fd) { return writeAsIni(config, fd); });
}

bool safeSaveAsIni(const Configuration &configuration, StandardPathsType type,
                   const std::filesystem::path &path) {
    RawConfig config;

    configuration.save(config);
    return safeSaveAsIni(config, type, path);
}

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
