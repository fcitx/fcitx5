/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "isocodes.h"
#include <cstring>
#include <string>
#include <json-c/json.h>
#include "fcitx-utils/metastring.h"
#include "fcitx-utils/misc.h"

namespace fcitx {

template <typename RootString>
class IsoCodesJsonParser {
public:
    virtual void handle(json_object *entry) = 0;

    void parse(const std::string &filename) {
        UniqueCPtr<json_object, json_object_put> obj;
        obj.reset(json_object_from_file(filename.data()));
        if (!obj) {
            return;
        }
        json_object *root =
            json_object_object_get(obj.get(), RootString::data());
        if (!root || json_object_get_type(root) != json_type_array) {
            return;
        }

        for (size_t i = 0, e = json_object_array_length(root); i < e; i++) {
            json_object *entry = json_object_array_get_idx(root, i);
            handle(entry);
        }
    }
};

class IsoCodes639Parser
    : public IsoCodesJsonParser<fcitxMakeMetaString("639-3")> {
public:
    IsoCodes639Parser(IsoCodes *that) : that_(that) {}

    void handle(json_object *entry) override {
        json_object *alpha2 = json_object_object_get(entry, "alpha_2");
        json_object *alpha3 = json_object_object_get(entry, "alpha_3");
        json_object *bibliographic =
            json_object_object_get(entry, "bibliographic");
        json_object *name = json_object_object_get(entry, "name");
        if (!name || json_object_get_type(name) != json_type_string) {
            return;
        }
        // there must be alpha3
        if (!alpha3 || json_object_get_type(alpha3) != json_type_string) {
            return;
        }

        // alpha2 is optional
        if (alpha2 && json_object_get_type(alpha2) != json_type_string) {
            return;
        }

        // bibliographic is optional
        if (bibliographic &&
            json_object_get_type(bibliographic) != json_type_string) {
            return;
        }
        if (!bibliographic) {
            bibliographic = alpha3;
        }
        IsoCodes639Entry e;
        e.name.assign(json_object_get_string(name),
                      json_object_get_string_len(name));
        if (alpha2) {
            e.iso_639_1_code.assign(json_object_get_string(alpha2),
                                    json_object_get_string_len(alpha2));
        }
        e.iso_639_2B_code.assign(json_object_get_string(bibliographic),
                                 json_object_get_string_len(bibliographic));
        e.iso_639_2T_code.assign(json_object_get_string(alpha3),
                                 json_object_get_string_len(alpha3));
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

class IsoCodes3166Parser
    : public IsoCodesJsonParser<fcitxMakeMetaString("3166-1")> {
public:
    IsoCodes3166Parser(IsoCodes *that) : that_(that) {}

    void handle(json_object *entry) override {
        json_object *alpha2 = json_object_object_get(entry, "alpha_2");
        json_object *nameObj = json_object_object_get(entry, "name");
        if (!nameObj || json_object_get_type(nameObj) != json_type_string) {
            return;
        }
        // there must be alpha3
        if (!alpha2 || json_object_get_type(alpha2) != json_type_string) {
            return;
        }
        std::string alpha_2_code;
        std::string name;
        name.assign(json_object_get_string(nameObj),
                    json_object_get_string_len(nameObj));
        alpha_2_code.assign(json_object_get_string(alpha2),
                            json_object_get_string_len(alpha2));
        if (!name.empty() && !alpha_2_code.empty()) {
            that_->iso3166.emplace(alpha_2_code, name);
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
} // namespace fcitx
