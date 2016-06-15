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

#include <list>
#include <string.h>
#include <iostream>
#include "fcitx-utils/stringutils.h"
#include "xkbrules.h"

namespace fcitx {

struct XkbRulesParseState {
    std::vector<std::string> parseStack;
    XkbRules *rules;
    std::list<XkbLayoutInfo> m_layoutInfos;
    std::list<XkbModelInfo> m_modelInfos;
    std::list<XkbOptionGroupInfo> m_optionGroupInfos;
    std::string m_version;

    bool match(const std::initializer_list<const char *> &array) {
        if (parseStack.size() < array.size()) {
            return false;
        }
        return std::equal(parseStack.end() - array.size(), parseStack.end(), array.begin());
    }

    static void handleStartElement(void *ctx, const xmlChar *name, const xmlChar **attrs) {
        auto that = static_cast<XkbRulesParseState *>(ctx);
        that->startElement(name, attrs);
    }
    static void handleEndElement(void *ctx, const xmlChar *name) {
        auto that = static_cast<XkbRulesParseState *>(ctx);
        that->endElement(name);
    }
    static void handleCharacters(void *ctx, const xmlChar *ch, int len) {
        auto that = static_cast<XkbRulesParseState *>(ctx);
        that->characters(ch, len);
    }

    void startElement(const xmlChar *name, const xmlChar **attrs) {
        parseStack.emplace_back(reinterpret_cast<const char *>(name));

        if (match({"layoutList", "layout", "configItem"})) {
            m_layoutInfos.emplace_back();
        } else if (match({"layoutList", "layout", "variantList", "variant"})) {
            m_layoutInfos.back().variantInfos.emplace_back();
        } else if (match({"modelList", "model"})) {
            m_modelInfos.emplace_back();
        } else if (match({"optionList", "group"})) {
            m_optionGroupInfos.emplace_back();
            int i = 0;
            while (attrs && attrs[i * 2] != 0) {
                if (strcmp(reinterpret_cast<const char *>(attrs[i * 2]), "allowMultipleSelection") == 0) {
                    m_optionGroupInfos.back().exclusive =
                        (strcmp(reinterpret_cast<const char *>(attrs[i * 2 + 1]), "true") != 0);
                }
                i++;
            }
        } else if (match({"optionList", "group", "option"})) {
            m_optionGroupInfos.back().optionInfos.emplace_back();
        } else if (match({"xkbConfigRegistry"})) {
            int i = 0;
            while (attrs && attrs[i * 2] != 0) {
                if (strcmp(reinterpret_cast<const char *>(attrs[i * 2]), "version") == 0 &&
                    strlen(reinterpret_cast<const char *>(attrs[i * 2 + 1])) != 0) {
                    m_version = reinterpret_cast<const char *>(attrs[i * 2 + 1]);
                }
                i++;
            }
        }
    }
    void endElement(const xmlChar *) { parseStack.pop_back(); }
    void characters(const xmlChar *ch, int len) {
        std::string temp(reinterpret_cast<const char *>(ch), len);
        std::string::size_type start, end;
        std::tie(start, end) = stringutils::trimInplace(temp);
        if (start != end) {
            std::string text = std::string(temp.begin() + start, temp.begin() + end);
            if (match({"layoutList", "layout", "configItem", "name"})) {
                m_layoutInfos.back().name = text;
            } else if (match({"layoutList", "layout", "configItem", "description"})) {
                m_layoutInfos.back().description = text;
            } else if (match({"layoutList", "layout", "configItem", "languageList", "iso639Id"})) {
                m_layoutInfos.back().languages.push_back(text);
            } else if (match({"layoutList", "layout", "variantList", "variant", "configItem", "name"})) {
                m_layoutInfos.back().variantInfos.back().name = text;
            } else if (match({"layoutList", "layout", "variantList", "variant", "configItem", "description"})) {
                m_layoutInfos.back().variantInfos.back().description = text;
            } else if (match({"layoutList", "layout", "variantList", "variant", "configItem", "languageList",
                              "iso639Id"})) {
                m_layoutInfos.back().variantInfos.back().languages.push_back(text);
            } else if (match({"modelList", "model", "configItem", "name"})) {
                m_modelInfos.back().name = text;
            } else if (match({"modelList", "model", "configItem", "description"})) {
                m_modelInfos.back().description = text;
            } else if (match({"modelList", "model", "configItem", "vendor"})) {
                m_modelInfos.back().vendor = text;
            } else if (match({"optionList", "group", "configItem", "name"})) {
                m_optionGroupInfos.back().name = text;
            } else if (match({"optionList", "group", "configItem", "description"})) {
                m_optionGroupInfos.back().description = text;
            } else if (match({"optionList", "group", "option", "configItem", "name"})) {
                m_optionGroupInfos.back().optionInfos.back().name = text;
            } else if (match({"optionList", "group", "option", "configItem", "description"})) {
                m_optionGroupInfos.back().optionInfos.back().description = text;
            }
        }
    }

    void merge(XkbRules *rules) {
        if (!m_version.empty()) {
            rules->m_version = m_version;
        }
        while (!m_layoutInfos.empty()) {
            XkbLayoutInfo info(std::move(m_layoutInfos.front()));
            m_layoutInfos.pop_front();
            auto iter = rules->m_layoutInfos.find(info.name);
            if (iter == rules->m_layoutInfos.end()) {
                std::string name = info.name;
                rules->m_layoutInfos.emplace(name, std::move(info));
            } else {
                iter->second.languages.insert(iter->second.languages.end(),
                                              std::make_move_iterator(info.languages.begin()),
                                              std::make_move_iterator(info.languages.end()));
                iter->second.variantInfos.insert(iter->second.variantInfos.end(),
                                                 std::make_move_iterator(info.variantInfos.begin()),
                                                 std::make_move_iterator(info.variantInfos.end()));
            }
        }
    }
};

bool XkbRules::read(const std::string &fileName) {
    clear();
    xmlSAXHandler handle;
    memset(&handle, 0, sizeof(xmlSAXHandler));
    handle.startElement = &XkbRulesParseState::handleStartElement;
    handle.endElement = &XkbRulesParseState::handleEndElement;
    handle.characters = &XkbRulesParseState::handleCharacters;

    {
        XkbRulesParseState state;
        state.rules = this;
        if (xmlSAXUserParseFile(&handle, &state, fileName.c_str()) != 0) {
            return false;
        }
        state.merge(this);
    }

    if (stringutils::endsWith(fileName, ".xml")) {
        auto extraFile = fileName.substr(0, fileName.size() - 3) + "extras.xml";
        {
            XkbRulesParseState state;
            state.rules = this;
            if (xmlSAXUserParseFile(&handle, &state, extraFile.c_str()) == 0) {
                state.merge(this);
            }
        }
    }
    return true;
}

void XkbRules::dump() {
    std::cout << "Version: " << m_version << std::endl;

    for (auto &p : m_layoutInfos) {
        auto &layoutInfo = p.second;
        std::cout << "\tLayout Name: " << layoutInfo.name << std::endl;
        std::cout << "\tLayout Description: " << layoutInfo.description << std::endl;
        std::cout << "\tLayout Languages: "
                  << stringutils::join(layoutInfo.languages.begin(), layoutInfo.languages.end(), ",") << std::endl;
        for (auto &variantInfo : layoutInfo.variantInfos) {
            std::cout << "\t\tVariant Name: " << variantInfo.name << std::endl;
            std::cout << "\t\tVariant Description: " << variantInfo.description << std::endl;
            std::cout << "\t\tVariant Languages: "
                      << stringutils::join(variantInfo.languages.begin(), variantInfo.languages.end(), ",")
                      << std::endl;
        }
    }

    for (auto &modelInfo : m_modelInfos) {
        std::cout << "\tModel Name: " << modelInfo.name << std::endl;
        std::cout << "\tModel Description: " << modelInfo.description << std::endl;
        std::cout << "\tModel Vendor: " << modelInfo.vendor << std::endl;
    }

    for (auto &optionGroupInfo : m_optionGroupInfos) {
        std::cout << "\tOption Group Name: " << optionGroupInfo.name << std::endl;
        std::cout << "\tOption Group Description: " << optionGroupInfo.description << std::endl;
        std::cout << "\tOption Group Exclusive: " << optionGroupInfo.exclusive << std::endl;
        for (auto &optionInfo : optionGroupInfo.optionInfos) {
            std::cout << "\t\tOption Name: " << optionInfo.name << std::endl;
            std::cout << "\t\tOption Description: " << optionInfo.description << std::endl;
        }
    }
}
}
