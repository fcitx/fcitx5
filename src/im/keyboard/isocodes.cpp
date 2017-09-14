/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#include "xmlparser.h"
#include <cstring>

namespace fcitx {

class IsoCodes639Parser : public XMLParser {
public:
    IsoCodes639Parser(IsoCodes *that) : that_(that) {}

    void startElement(const XML_Char *name, const XML_Char **atts) override {
        if (strcmp(name, "iso_639_entry") == 0) {
            IsoCodes639Entry entry;
            int i = 0;
            while (atts && atts[i * 2] != 0) {
                if (strcmp(atts[i * 2], "iso_639_2B_code") == 0)
                    entry.iso_639_2B_code = atts[i * 2 + 1];
                else if (strcmp(atts[i * 2], "iso_639_2T_code") == 0)
                    entry.iso_639_2T_code = atts[i * 2 + 1];
                else if (strcmp(atts[i * 2], "iso_639_1_code") == 0)
                    entry.iso_639_1_code = atts[i * 2 + 1];
                else if (strcmp(atts[i * 2], "name") == 0)
                    entry.name = atts[i * 2 + 1];
                i++;
            }
            if ((!entry.iso_639_2B_code.empty() ||
                 !entry.iso_639_2T_code.empty()) &&
                !entry.name.empty()) {
                that_->iso639entires.emplace_back(entry);
                if (!entry.iso_639_2B_code.empty()) {
                    that_->iso6392B.emplace(entry.iso_639_2B_code,
                                            that_->iso639entires.size() - 1);
                }
                if (!entry.iso_639_2T_code.empty()) {
                    that_->iso6392T.emplace(entry.iso_639_2T_code,
                                            that_->iso639entires.size() - 1);
                }
            }
        }
    }

private:
    IsoCodes *that_;
};

class IsoCodes3166Parser : public XMLParser {
public:
    IsoCodes3166Parser(IsoCodes *that) : that_(that) {}

    void startElement(const XML_Char *name, const XML_Char **atts) override {
        if (strcmp(name, "iso_3166_entry") == 0) {
            std::string alpha_2_code, name;
            int i = 0;
            while (atts && atts[i * 2] != 0) {
                if (strcmp(atts[i * 2], "alpha_2_code") == 0)
                    alpha_2_code = atts[i * 2 + 1];
                else if (strcmp(atts[i * 2], "name") == 0)
                    name = atts[i * 2 + 1];
                i++;
            }
            if (!name.empty() && !alpha_2_code.empty()) {
                that_->iso3166.emplace(alpha_2_code, name);
            }
        }
    }

private:
    IsoCodes *that_;
};

void IsoCodes::read(const std::string &iso639, const std::string &iso3166) {
    IsoCodes639Parser parser639(this);
    parser639.parse(iso639);
    IsoCodes3166Parser parser3166(this);
    parser3166.parse(iso3166);
}
}
