//
// Copyright (C) 2016~2016 by CSSlayer
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

#include "xkbrules.h"
#include <cstring>
#include <list>
#include "fcitx-utils/stringutils.h"
#include "xmlparser.h"

#ifdef _TEST_XKBRULES
#include <iostream>
#endif

namespace fcitx {

struct XkbRulesParseState : public XMLParser {
    std::vector<std::string> parseStack_;
    XkbRules *rules_;
    std::list<XkbLayoutInfo> layoutInfos_;
    std::list<XkbModelInfo> modelInfos_;
    std::list<XkbOptionGroupInfo> optionGroupInfos_;
    std::string version_;

    bool match(const std::initializer_list<const char *> &array) {
        if (parseStack_.size() < array.size()) {
            return false;
        }
        return std::equal(parseStack_.end() - array.size(), parseStack_.end(),
                          array.begin());
    }

    void startElement(const XML_Char *name, const XML_Char **attrs) override {
        parseStack_.emplace_back(reinterpret_cast<const char *>(name));

        if (match({"layoutList", "layout", "configItem"})) {
            layoutInfos_.emplace_back();
        } else if (match({"layoutList", "layout", "variantList", "variant"})) {
            layoutInfos_.back().variantInfos.emplace_back();
        } else if (match({"modelList", "model"})) {
            modelInfos_.emplace_back();
        } else if (match({"optionList", "group"})) {
            optionGroupInfos_.emplace_back();
            int i = 0;
            while (attrs && attrs[i * 2] != 0) {
                if (strcmp(reinterpret_cast<const char *>(attrs[i * 2]),
                           "allowMultipleSelection") == 0) {
                    optionGroupInfos_.back().exclusive =
                        (strcmp(
                             reinterpret_cast<const char *>(attrs[i * 2 + 1]),
                             "true") != 0);
                }
                i++;
            }
        } else if (match({"optionList", "group", "option"})) {
            optionGroupInfos_.back().optionInfos.emplace_back();
        } else if (match({"xkbConfigRegistry"})) {
            int i = 0;
            while (attrs && attrs[i * 2] != 0) {
                if (strcmp(reinterpret_cast<const char *>(attrs[i * 2]),
                           "version") == 0 &&
                    strlen(reinterpret_cast<const char *>(attrs[i * 2 + 1])) !=
                        0) {
                    version_ = reinterpret_cast<const char *>(attrs[i * 2 + 1]);
                }
                i++;
            }
        }
    }
    void endElement(const XML_Char *) override { parseStack_.pop_back(); }
    void characterData(const XML_Char *ch, int len) override {
        std::string temp(reinterpret_cast<const char *>(ch), len);
        auto pair = stringutils::trimInplace(temp);
        std::string::size_type start = pair.first, end = pair.second;
        if (start != end) {
            std::string text(temp.begin() + start, temp.begin() + end);
            if (match({"layoutList", "layout", "configItem", "name"})) {
                layoutInfos_.back().name = text;
            } else if (match({"layoutList", "layout", "configItem",
                              "description"})) {
                layoutInfos_.back().description = text;
            } else if (match({"layoutList", "layout", "configItem",
                              "languageList", "iso639Id"})) {
                layoutInfos_.back().languages.push_back(text);
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "name"})) {
                layoutInfos_.back().variantInfos.back().name = text;
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "description"})) {
                layoutInfos_.back().variantInfos.back().description = text;
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "languageList", "iso639Id"})) {
                layoutInfos_.back().variantInfos.back().languages.push_back(
                    text);
            } else if (match({"modelList", "model", "configItem", "name"})) {
                modelInfos_.back().name = text;
            } else if (match({"modelList", "model", "configItem",
                              "description"})) {
                modelInfos_.back().description = text;
            } else if (match({"modelList", "model", "configItem", "vendor"})) {
                modelInfos_.back().vendor = text;
            } else if (match({"optionList", "group", "configItem", "name"})) {
                optionGroupInfos_.back().name = text;
            } else if (match({"optionList", "group", "configItem",
                              "description"})) {
                optionGroupInfos_.back().description = text;
            } else if (match({"optionList", "group", "option", "configItem",
                              "name"})) {
                optionGroupInfos_.back().optionInfos.back().name = text;
            } else if (match({"optionList", "group", "option", "configItem",
                              "description"})) {
                optionGroupInfos_.back().optionInfos.back().description = text;
            }
        }
    }

    void merge(XkbRules *rules) {
        if (!version_.empty()) {
            rules->version_ = version_;
        }
        while (!layoutInfos_.empty()) {
            XkbLayoutInfo info(std::move(layoutInfos_.front()));
            layoutInfos_.pop_front();
            auto iter = rules->layoutInfos_.find(info.name);
            if (iter == rules->layoutInfos_.end()) {
                std::string name = info.name;
                rules->layoutInfos_.emplace(std::move(name), std::move(info));
            } else {
                iter->second.languages.insert(
                    iter->second.languages.end(),
                    std::make_move_iterator(info.languages.begin()),
                    std::make_move_iterator(info.languages.end()));
                iter->second.variantInfos.insert(
                    iter->second.variantInfos.end(),
                    std::make_move_iterator(info.variantInfos.begin()),
                    std::make_move_iterator(info.variantInfos.end()));
            }
        }
    }
};

bool XkbRules::read(const std::string &fileName) {
    clear();

    {
        XkbRulesParseState state;
        state.rules_ = this;
        if (!state.parse(fileName)) {
            return false;
        }
        state.merge(this);
    }

    if (stringutils::endsWith(fileName, ".xml")) {
        auto extraFile = fileName.substr(0, fileName.size() - 3);
        extraFile += "extras.xml";
        {
            XkbRulesParseState state;
            state.rules_ = this;
            if (state.parse(extraFile)) {
                state.merge(this);
            }
        }
    }
    return true;
}

#ifdef _TEST_XKBRULES
void XkbRules::dump() {
    std::cout << "Version: " << version_ << std::endl;

    for (auto &p : layoutInfos_) {
        auto &layoutInfo = p.second;
        std::cout << "\tLayout Name: " << layoutInfo.name << std::endl;
        std::cout << "\tLayout Description: " << layoutInfo.description
                  << std::endl;
        std::cout << "\tLayout Languages: "
                  << stringutils::join(layoutInfo.languages.begin(),
                                       layoutInfo.languages.end(), ",")
                  << std::endl;
        for (auto &variantInfo : layoutInfo.variantInfos) {
            std::cout << "\t\tVariant Name: " << variantInfo.name << std::endl;
            std::cout << "\t\tVariant Description: " << variantInfo.description
                      << std::endl;
            std::cout << "\t\tVariant Languages: "
                      << stringutils::join(variantInfo.languages.begin(),
                                           variantInfo.languages.end(), ",")
                      << std::endl;
        }
    }

    for (auto &modelInfo : modelInfos_) {
        std::cout << "\tModel Name: " << modelInfo.name << std::endl;
        std::cout << "\tModel Description: " << modelInfo.description
                  << std::endl;
        std::cout << "\tModel Vendor: " << modelInfo.vendor << std::endl;
    }

    for (auto &optionGroupInfo : optionGroupInfos_) {
        std::cout << "\tOption Group Name: " << optionGroupInfo.name
                  << std::endl;
        std::cout << "\tOption Group Description: "
                  << optionGroupInfo.description << std::endl;
        std::cout << "\tOption Group Exclusive: " << optionGroupInfo.exclusive
                  << std::endl;
        for (auto &optionInfo : optionGroupInfo.optionInfos) {
            std::cout << "\t\tOption Name: " << optionInfo.name << std::endl;
            std::cout << "\t\tOption Description: " << optionInfo.description
                      << std::endl;
        }
    }
}
#endif
} // namespace fcitx
