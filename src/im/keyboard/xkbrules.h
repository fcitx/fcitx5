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
#ifndef _FCITX_IM_KEYBOARD_XKBRULES_H_
#define _FCITX_IM_KEYBOARD_XKBRULES_H_

#include "fcitx/misc_p.h"
#include <libxml/parser.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace fcitx {

struct XkbVariantInfo {
    std::string name;
    std::string description;
    std::vector<std::string> languages;
};

struct XkbLayoutInfo {
    std::vector<XkbVariantInfo> variantInfos;
    std::string name;
    std::string description;
    std::vector<std::string> languages;
};

struct XkbModelInfo {
    std::string name;
    std::string description;
    std::string vendor;
};

struct XkbOptionInfo {
    std::string name;
    std::string description;
};

struct XkbOptionGroupInfo {
    std::vector<XkbOptionInfo> optionInfos;
    std::string name;
    std::string description;
    bool exclusive;
};

struct XkbRulesParseState;
class XkbRules {
public:
    friend struct XkbRulesParseState;
    bool read(const std::string &fileName);
    void dump();

    const XkbLayoutInfo *findByName(const std::string &name) const { return findValue(m_layoutInfos, name); }

    void clear() {
        m_layoutInfos.clear();
        m_modelInfos.clear();
        m_optionGroupInfos.clear();
        m_version.clear();
    }

    auto &layoutInfos() const { return m_layoutInfos; }
    auto &modelInfos() const { return m_modelInfos; }
    auto &optionGroupInfos() const { return m_optionGroupInfos; }
    auto &version() const { return m_version; }

private:
    std::unordered_map<std::string, XkbLayoutInfo> m_layoutInfos;
    std::vector<XkbModelInfo> m_modelInfos;
    std::vector<XkbOptionGroupInfo> m_optionGroupInfos;
    std::string m_version;
};
}

#endif // _FCITX_IM_KEYBOARD_XKBRULES_H_
