/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "isocodes.h"
#include <libxml/parser.h>
#include <string.h>

namespace fcitx {

class LibXmlInit {
public:
    LibXmlInit() { xmlInitParser(); }
};

static LibXmlInit init;

void IsoCodes::read(const std::string &iso639, const std::string &iso3166) {
    xmlSAXHandler handle;
    memset(&handle, 0, sizeof(xmlSAXHandler));

    handle.startElement = &IsoCodes::handleIsoCodes639StartElement;
    xmlSAXUserParseFile(&handle, this, iso639.c_str());
    handle.startElement = &IsoCodes::handleIsoCodes3166StartElement;
    xmlSAXUserParseFile(&handle, this, iso3166.c_str());
}

void IsoCodes::handleIsoCodes639StartElement(void *ctx, const xmlChar *name, const xmlChar **atts) {
    auto that = static_cast<IsoCodes *>(ctx);
    that->isoCodes639StartElement(name, atts);
}

void IsoCodes::handleIsoCodes3166StartElement(void *ctx, const xmlChar *name, const xmlChar **atts) {
    auto that = static_cast<IsoCodes *>(ctx);
    that->isoCodes3166StartElement(name, atts);
}

void IsoCodes::isoCodes639StartElement(const xmlChar *name, const xmlChar **atts) {
    if (strcmp(reinterpret_cast<const char *>(name), "iso_639_entry") == 0) {
        IsoCodes639Entry entry;
        int i = 0;
        while (atts && atts[i * 2] != 0) {
            if (strcmp(reinterpret_cast<const char *>(atts[i * 2]), "iso_639_2B_code") == 0)
                entry.iso_639_2B_code = reinterpret_cast<const char *>(atts[i * 2 + 1]);
            else if (strcmp(reinterpret_cast<const char *>(atts[i * 2]), "iso_639_2T_code") == 0)
                entry.iso_639_2T_code = reinterpret_cast<const char *>(atts[i * 2 + 1]);
            else if (strcmp(reinterpret_cast<const char *>(atts[i * 2]), "iso_639_1_code") == 0)
                entry.iso_639_1_code = reinterpret_cast<const char *>(atts[i * 2 + 1]);
            else if (strcmp(reinterpret_cast<const char *>(atts[i * 2]), "name") == 0)
                entry.name = reinterpret_cast<const char *>(atts[i * 2 + 1]);
            i++;
        }
        if ((!entry.iso_639_2B_code.empty() || !entry.iso_639_2T_code.empty()) && !entry.name.empty()) {
            iso639entires.emplace_back(entry);
            if (!entry.iso_639_2B_code.empty()) {
                iso6392B.emplace(entry.iso_639_2B_code, iso639entires.size() - 1);
            }
            if (!entry.iso_639_2T_code.empty()) {
                iso6392T.emplace(entry.iso_639_2T_code, iso639entires.size() - 1);
            }
        }
    }
}

void IsoCodes::isoCodes3166StartElement(const xmlChar *name, const xmlChar **atts) {
    if (strcmp(reinterpret_cast<const char *>(name), "iso_3166_entry") == 0) {
        std::string alpha_2_code, name;
        int i = 0;
        while (atts && atts[i * 2] != 0) {
            if (strcmp(reinterpret_cast<const char *>(atts[i * 2]), "alpha_2_code") == 0)
                alpha_2_code = strdup(reinterpret_cast<const char *>(atts[i * 2 + 1]));
            else if (strcmp(reinterpret_cast<const char *>(atts[i * 2]), "name") == 0)
                name = reinterpret_cast<const char *>(atts[i * 2 + 1]);
            i++;
        }
        if (!name.empty() && !alpha_2_code.empty()) {
            iso3166.emplace(alpha_2_code, name);
        }
    }
}
}
