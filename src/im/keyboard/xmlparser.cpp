/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xmlparser.h"
#include <cstdio>
#include <string>
#include <expat.h>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/unixfd.h"
#define XML_BUFFER_SIZE 4096

namespace fcitx {

bool XMLParser::parse(const std::string &name) {
    UniqueCPtr<XML_ParserStruct, XML_ParserFree> parser(
        XML_ParserCreate(nullptr));
    UnixFD input = StandardPaths::openPath(name);
    if (!input.isValid()) {
        return false;
    }

    XML_SetUserData(parser.get(), this);

    XML_SetElementHandler(
        parser.get(),
        [](void *data, const char *element_name, const char **atts) {
            auto *ctx = static_cast<XMLParser *>(data);
            ctx->startElement(element_name, atts);
        },
        [](void *data, const char *name) {
            auto *ctx = static_cast<XMLParser *>(data);
            ctx->endElement(name);
        });
    XML_SetCharacterDataHandler(parser.get(),
                                [](void *data, const char *s, int len) {
                                    auto *ctx = static_cast<XMLParser *>(data);
                                    ctx->characterData(s, len);
                                });
    int len;
    void *buf;
    do {
        buf = XML_GetBuffer(parser.get(), XML_BUFFER_SIZE);
        len = fs::safeRead(input.fd(), buf, XML_BUFFER_SIZE);
        if (len < 0) {
            return false;
        }
        if (XML_ParseBuffer(parser.get(), len, len == 0) == 0) {
            return false;
        }
    } while (len > 0);

    return true;
}

} // namespace fcitx
