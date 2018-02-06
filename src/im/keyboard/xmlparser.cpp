//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "xmlparser.h"
#include <memory>
#define XML_BUFFER_SIZE 4096

bool fcitx::XMLParser::parse(const std::string &name) {
    std::unique_ptr<XML_ParserStruct, decltype(&XML_ParserFree)> parser(
        XML_ParserCreate(nullptr), XML_ParserFree);
    std::unique_ptr<std::FILE, decltype(&std::fclose)> input(
        std::fopen(name.c_str(), "r"), &std::fclose);
    if (!input) {
        return false;
    }

    XML_SetUserData(parser.get(), this);

    XML_SetElementHandler(
        parser.get(),
        [](void *data, const char *element_name, const char **atts) {
            auto ctx = static_cast<XMLParser *>(data);
            ctx->startElement(element_name, atts);
        },
        [](void *data, const XML_Char *name) {
            auto ctx = static_cast<XMLParser *>(data);
            ctx->endElement(name);
        });
    XML_SetCharacterDataHandler(parser.get(),
                                [](void *data, const XML_Char *s, int len) {
                                    auto ctx = static_cast<XMLParser *>(data);
                                    ctx->characterData(s, len);
                                });
    int len;
    void *buf;
    do {
        buf = XML_GetBuffer(parser.get(), XML_BUFFER_SIZE);
        len = fread(buf, 1, XML_BUFFER_SIZE, input.get());
        if (len < 0) {
            return false;
        }
        if (XML_ParseBuffer(parser.get(), len, len == 0) == 0) {
            return false;
        }
    } while (len > 0);

    return true;
}
