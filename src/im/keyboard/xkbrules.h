/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_IM_KEYBOARD_XKBRULES_H_
#define _FCITX_IM_KEYBOARD_XKBRULES_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "fcitx/misc_p.h"

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
} // namespace fcitx

#endif // _FCITX_IM_KEYBOARD_XKBRULES_H_
