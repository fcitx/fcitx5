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
#ifdef _TEST_XKBRULES
    void dump();
#endif

    const XkbLayoutInfo *findByName(const std::string &name) const {
        return findValue(layoutInfos_, name);
    }

    void clear() {
        layoutInfos_.clear();
        modelInfos_.clear();
        optionGroupInfos_.clear();
        version_.clear();
    }

    auto &layoutInfos() const { return layoutInfos_; }
    auto &modelInfos() const { return modelInfos_; }
    auto &optionGroupInfos() const { return optionGroupInfos_; }
    auto &version() const { return version_; }

private:
    std::unordered_map<std::string, XkbLayoutInfo> layoutInfos_;
    std::vector<XkbModelInfo> modelInfos_;
    std::vector<XkbOptionGroupInfo> optionGroupInfos_;
    std::string version_;
};
}

#endif // _FCITX_IM_KEYBOARD_XKBRULES_H_
