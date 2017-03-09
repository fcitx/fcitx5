/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <stdio.h>

#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/unixfd.h"
#include "iniparser.h"

namespace fcitx {
enum class UnescapeState { NORMAL, ESCAPE };

bool _unescape_string(std::string &str, bool unescapeQuote) {
    if (str.empty()) {
        return true;
    }

    size_t i = 0;
    size_t j = 0;
    UnescapeState state = UnescapeState::NORMAL;
    do {
        switch (state) {
        case UnescapeState::NORMAL:
            if (str[i] == '\\') {
                state = UnescapeState::ESCAPE;
            } else {
                str[j] = str[i];
                j++;
            }
            break;
        case UnescapeState::ESCAPE:
            if (str[i] == '\\') {
                str[j] = '\\';
                j++;
            } else if (str[i] == 'n') {
                str[j] = '\n';
                j++;
            } else if (str[i] == '\"' && unescapeQuote) {
                str[j] = '\"';
                j++;
            } else {
                return false;
            }
            state = UnescapeState::NORMAL;
            break;
        }
    } while (str[i++]);
    str.resize(j - 1);
    return true;
}

typedef std::unique_ptr<FILE, decltype(&fclose)> ScopedFILE;

void readFromIni(RawConfig &config, int fd) {
    if (fd < 0) {
        return;
    }
    // dup it
    UnixFD unixFD(fd);
    FILE *f = fdopen(unixFD.fd(), "r");
    if (!f) {
        return;
    }
    ScopedFILE fp{f, fclose};
    unixFD.release();
    readFromIni(config, fp.get());
}

bool writeAsIni(const RawConfig &config, int fd) {
    if (fd < 0) {
        return false;
    }
    // dup it
    UnixFD unixFD(fd);
    FILE *f = fdopen(unixFD.release(), "w");
    if (!f) {
        return false;
    }
    ScopedFILE fp{f, fclose};
    return writeAsIni(config, fp.get());
}

void readFromIni(RawConfig &config, FILE *fin) {
    std::string lineBuf, currentGroup;

    char *clineBuf = nullptr;
    size_t bufSize = 0;
    unsigned int line = 0;
    while (getline(&clineBuf, &bufSize, fin) >= 0) {
        line++;
        lineBuf = clineBuf;
        std::string::size_type start, end;
        std::tie(start, end) = stringutils::trimInplace(lineBuf);
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
        } else if ((equalPos = lineBuf.find_first_of('=', start)) != std::string::npos) {
            auto name = lineBuf.substr(start, equalPos - start);
            auto valueStart = equalPos + 1;
            auto valueEnd = lineBuf.size();

            bool unescapeQuote = false;
            ;
            // having quote at beginning and end, escape
            if (valueEnd - valueStart >= 2 && lineBuf[valueStart] == '"' && lineBuf[valueEnd - 1] == '"') {
                lineBuf.resize(valueEnd - 1);
                valueStart++;
                unescapeQuote = true;
            }

            auto value = lineBuf.substr(valueStart);
            if (!_unescape_string(value, unescapeQuote)) {
                continue;
            }

            RawConfigPtr subConfig;
            if (!currentGroup.empty()) {
                std::stringstream ss;
                ss << currentGroup << "/" << name;
                subConfig = config.get(ss.str(), true);
            } else {
                subConfig = config.get(name, true);
            }
            subConfig->setValue(value);
            subConfig->setLineNumber(line);
        }
    }

    free(clineBuf);
}

bool writeAsIni(const RawConfig &root, FILE *fout) {
    std::function<bool(const RawConfig &, const std::string &path)> callback;

    callback = [fout, &callback](const RawConfig &config, const std::string &path) {
        if (config.hasSubItems()) {
            std::stringstream valuesout;
            config.visitSubItems(
                [&valuesout](const RawConfig &config, const std::string &) {
                    if (config.hasSubItems() && config.value().empty()) {
                        return true;
                    }

                    if (!config.comment().empty() && config.comment().find('\n') == std::string::npos) {
                        valuesout << "# " << config.comment() << "\n";
                    }

                    auto value = config.value();
                    value = stringutils::replaceAll(value, "\\", "\\\\");
                    value = stringutils::replaceAll(value, "\n", "\\n");

                    bool needQuote = value.find_first_of("\f\r\t\v ") != std::string::npos;

                    if (needQuote) {
                        value = stringutils::replaceAll(value, "\"", "\\\"");
                    }

                    if (needQuote) {
                        valuesout << config.name() << "=\"" << value << "\"\n";
                    } else {
                        valuesout << config.name() << "=" << value << "\n";
                    }
                    return true;
                },
                "", false, path);
            auto valueString = valuesout.str();
            if (!valueString.empty()) {
                if (!path.empty()) {
                    FCITX_RETURN_IF(fprintf(fout, "[%s]\n", path.c_str()) < 0, false);
                }
                FCITX_RETURN_IF(fprintf(fout, "%s\n", valueString.c_str()) < 0, false);
            }
        }
        config.visitSubItems(callback, "", false, path);
        return true;
    };

    return callback(root, "");
}
}
