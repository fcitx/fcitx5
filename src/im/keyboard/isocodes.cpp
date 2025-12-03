/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "isocodes.h"
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include "fcitx-utils/metastring.h"

namespace fcitx {

using json = nlohmann::json;

template <typename RootString>
class IsoCodesJsonParser {
public:
    virtual void handle(const json &entry) = 0;

    void parse(const std::string &filename) {
        std::ifstream fin(filename);
        if (!fin.is_open()) {
            return;
        }
        json obj;
        try {
            fin >> obj;
        } catch (const json::parse_error &) {
            return;
        }
        auto it = obj.find(RootString::data());
        if (it == obj.end() || !it->is_array()) {
            return;
        }

        for (const auto &entry : *it) {
            handle(entry);
        }
    }
};

class IsoCodes639Parser
    : public IsoCodesJsonParser<fcitxMakeMetaString("639-3")> {
public:
    IsoCodes639Parser(IsoCodes *that) : that_(that) {}

    void handle(const json &entry) override {
        const auto it_alpha2 = entry.find("alpha_2");
        const auto it_alpha3 = entry.find("alpha_3");
        auto it_bibliographic = entry.find("bibliographic");
        const auto it_name = entry.find("name");
        if (it_name == entry.end() || !it_name->is_string()) {
            return;
        }
        // there must be alpha3
        if (it_alpha3 == entry.end() || !it_alpha3->is_string()) {
            return;
        }

        // alpha2 is optional
        if (it_alpha2 != entry.end() && !it_alpha2->is_string()) {
            return;
        }

        // bibliographic is optional
        if (it_bibliographic == entry.end()) {
            it_bibliographic = it_alpha3;
        } else if (!it_bibliographic->is_string()) {
            return;
        }
        IsoCodes639Entry e;
        e.name = it_name->get<std::string>();
        if (it_alpha2 != entry.end()) {
            e.iso_639_1_code = it_alpha2->get<std::string>();
        }
        e.iso_639_2B_code = it_bibliographic->get<std::string>();
        e.iso_639_2T_code = it_alpha3->get<std::string>();
        if ((!e.iso_639_2B_code.empty() || !e.iso_639_2T_code.empty()) &&
            !e.name.empty()) {
            that_->iso639entires.emplace_back(e);
            if (!e.iso_639_2B_code.empty()) {
                that_->iso6392B.emplace(e.iso_639_2B_code,
                                        that_->iso639entires.size() - 1);
            }
            if (!e.iso_639_2T_code.empty()) {
                that_->iso6392T.emplace(e.iso_639_2T_code,
                                        that_->iso639entires.size() - 1);
            }
        }
    }

private:
    IsoCodes *that_;
};

void IsoCodes::read(const std::string &iso639) {
    IsoCodes639Parser parser639(this);
    parser639.parse(iso639);
}
} // namespace fcitx
