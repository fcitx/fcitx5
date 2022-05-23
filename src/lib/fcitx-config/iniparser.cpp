/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "iniparser.h"
#include <fcntl.h>
#include <cstdio>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/unixfd.h"
#include "configuration.h"

namespace fcitx {

void readFromIni(RawConfig &config, int fd) {
    if (fd < 0) {
        return;
    }
    // dup it
    UnixFD unixFD(fd);
    UniqueFilePtr fp = fs::openFD(unixFD, "rb");
    if (!fp) {
        return;
    }
    readFromIni(config, fp.get());
}

bool writeAsIni(const RawConfig &config, int fd) {
    if (fd < 0) {
        return false;
    }
    // dup it
    UnixFD unixFD(fd);
    UniqueFilePtr fp = fs::openFD(unixFD, "wb");
    if (!fp) {
        return false;
    }
    return writeAsIni(config, fp.get());
}

void readFromIni(RawConfig &config, FILE *fin) {
    std::string lineBuf, currentGroup;

    UniqueCPtr<char> clineBuf;
    size_t bufSize = 0;
    unsigned int line = 0;
    while (getline(clineBuf, &bufSize, fin) >= 0) {
        line++;
        lineBuf = clineBuf.get();
        auto pair = stringutils::trimInplace(lineBuf);
        std::string::size_type start = pair.first, end = pair.second;
        if (start == end || lineBuf[start] == '#') {
            continue;
        }

        lineBuf.resize(end);

        std::string::size_type equalPos;

        if (lineBuf[start] == '[' && lineBuf[end - 1] == ']') {
            currentGroup = lineBuf.substr(start + 1, end - start - 2);
            config.visitItemsOnPath(
                [line](RawConfig &config, const std::string &) {
                    if (!config.lineNumber()) {
                        config.setLineNumber(line);
                    }
                },
                currentGroup);
        } else if ((equalPos = lineBuf.find_first_of('=', start)) !=
                   std::string::npos) {
            auto name = lineBuf.substr(start, equalPos - start);
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
                subConfig = config.get(name, true);
            }
            subConfig->setValue(*value);
            subConfig->setLineNumber(line);
        }
    }
}

bool writeAsIni(const RawConfig &root, FILE *fout) {
    std::function<bool(const RawConfig &, const std::string &path)> callback;

    callback = [fout, &callback](const RawConfig &config,
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
            auto valueString = values;
            if (!valueString.empty()) {
                if (!path.empty()) {
                    FCITX_RETURN_IF(fprintf(fout, "[%s]\n", path.c_str()) < 0,
                                    false);
                }
                FCITX_RETURN_IF(fprintf(fout, "%s\n", valueString.c_str()) < 0,
                                false);
            }
        }
        config.visitSubItems(callback, "", false, path);
        return true;
    };

    return callback(root, "");
}

void readAsIni(RawConfig &rawConfig, const std::string &path) {
    return readAsIni(rawConfig, StandardPath::Type::PkgConfig, path);
}

void readAsIni(Configuration &configuration, const std::string &path) {
    return readAsIni(configuration, StandardPath::Type::PkgConfig, path);
}

void readAsIni(RawConfig &rawConfig, StandardPath::Type type,
               const std::string &path) {
    const auto &standardPath = StandardPath::global();
    auto file = standardPath.open(type, path, O_RDONLY);
    readFromIni(rawConfig, file.fd());
}

void readAsIni(Configuration &configuration, StandardPath::Type type,
               const std::string &path) {
    RawConfig config;
    readAsIni(config, type, path);

    configuration.load(config);
}

bool safeSaveAsIni(const RawConfig &config, const std::string &path) {
    return safeSaveAsIni(config, StandardPath::Type::PkgConfig, path);
}

bool safeSaveAsIni(const Configuration &configuration,
                   const std::string &path) {
    return safeSaveAsIni(configuration, StandardPath::Type::PkgConfig, path);
}

bool safeSaveAsIni(const RawConfig &config, StandardPath::Type type,
                   const std::string &path) {
    const auto &standardPath = StandardPath::global();
    return standardPath.safeSave(
        type, path, [&config](int fd) { return writeAsIni(config, fd); });
}

bool safeSaveAsIni(const Configuration &configuration, StandardPath::Type type,
                   const std::string &path) {
    RawConfig config;

    configuration.save(config);
    return safeSaveAsIni(config, type, path);
}
} // namespace fcitx
