/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xkbrules.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <list>
#include <string>
#include <utility>
#include <vector>
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
    std::string textBuff_;

    bool match(const std::initializer_list<const char *> &array) {
        if (parseStack_.size() < array.size()) {
            return false;
        }
        return std::equal(parseStack_.end() - array.size(), parseStack_.end(),
                          array.begin());
    }

    void startElement(const char *name, const char **attrs) override {
        parseStack_.emplace_back(std::string(name));
        textBuff_.clear();

        if (match({"layoutList", "layout", "configItem"})) {
            layoutInfos_.emplace_back();
        } else if (match({"layoutList", "layout", "variantList", "variant"})) {
            layoutInfos_.back().variantInfos.emplace_back();
        } else if (match({"modelList", "model"})) {
            modelInfos_.emplace_back();
        } else if (match({"optionList", "group"})) {
            optionGroupInfos_.emplace_back();
            ptrdiff_t i = 0;
            while (attrs && attrs[i * 2] != nullptr) {
                if (strcmp(attrs[i * 2], "allowMultipleSelection") == 0) {
                    optionGroupInfos_.back().exclusive =
                        (strcmp(attrs[(i * 2) + 1], "true") != 0);
                }
                i++;
            }
        } else if (match({"optionList", "group", "option"})) {
            optionGroupInfos_.back().optionInfos.emplace_back();
        } else if (match({"xkbConfigRegistry"})) {
            ptrdiff_t i = 0;
            while (attrs && attrs[i * 2] != nullptr) {
                if (strcmp(attrs[i * 2], "version") == 0 &&
                    strlen(attrs[(i * 2) + 1]) != 0) {
                    version_ = attrs[(i * 2) + 1];
                }
                i++;
            }
        }
    }
    void endElement(const char * /*name*/) override {
        auto text = stringutils::trimView(textBuff_);
        if (!text.empty()) {
            if (match({"layoutList", "layout", "configItem", "name"})) {
                layoutInfos_.back().name = text;
            } else if (match({"layoutList", "layout", "configItem",
                              "description"})) {
                layoutInfos_.back().description = text;
            } else if (match({"layoutList", "layout", "configItem",
                              "shortDescription"})) {
                layoutInfos_.back().shortDescription = text;
            } else if (match({"layoutList", "layout", "configItem",
                              "languageList", "iso639Id"})) {
                layoutInfos_.back().languages.emplace_back(text);
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "name"})) {
                layoutInfos_.back().variantInfos.back().name = text;
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "description"})) {
                layoutInfos_.back().variantInfos.back().description = text;
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "shortDescription"})) {
                layoutInfos_.back().variantInfos.back().shortDescription = text;
            } else if (match({"layoutList", "layout", "variantList", "variant",
                              "configItem", "languageList", "iso639Id"})) {
                layoutInfos_.back().variantInfos.back().languages.emplace_back(
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

        textBuff_.clear();
        parseStack_.pop_back();
    }
    void characterData(const char *ch, int len) override {
        textBuff_.append(ch, len);
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

bool XkbRules::read(const std::vector<std::string> &directories,
                    const std::string &name, const std::string &extraFile) {
    clear();

    bool success = false;
    for (const auto &directory : directories) {
        std::string fileName = stringutils::joinPath(
            directory, "rules", stringutils::concat(name, ".xml"));

        {
            XkbRulesParseState state;
            state.rules_ = this;
            if (state.parse(fileName)) {
                success = true;
                state.merge(this);
            }
        }

        std::string extraFileName = stringutils::joinPath(
            directory, "rules", stringutils::concat(name, ".extras.xml"));
        {
            XkbRulesParseState state;
            state.rules_ = this;
            if (state.parse(extraFileName)) {
                success = true;
                state.merge(this);
            }
        }
    }

    if (!extraFile.empty()) {
        XkbRulesParseState state;
        state.rules_ = this;
        if (state.parse(extraFile)) {
            success = true;
            state.merge(this);
        }
    }
    return success;
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
