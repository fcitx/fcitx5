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

#include <libintl.h>
#include <string.h>
#include "fcitx/instance.h"
#include "fcitx-utils/stringutils.h"
#include "keyboard.h"
#include "modules/xcb/xcb_public.h"
#include "config.h"
#include "fcitx/misc_p.h"

namespace fcitx {

static std::string findBestLanguage(const IsoCodes &isocodes, const std::string &hint,
                                    const std::vector<std::string> &languages) {
    /* score:
     * 1 -> first one
     * 2 -> match 2
     * 3 -> match three
     */
    const IsoCodes639Entry *bestEntry = nullptr;
    int bestScore = 0;
    for (auto &language : languages) {
        auto entry = isocodes.entry(language);
        if (!entry) {
            continue;
        }

        auto langCode = entry->iso_639_1_code;
        if (langCode.empty()) {
            langCode = entry->iso_639_2T_code;
        }

        if (langCode.empty()) {
            langCode = entry->iso_639_2B_code;
        }

        if (langCode.empty()) {
            continue;
        }

        if (langCode.size() != 2 && langCode.size() != 3) {
            continue;
        }

        int score = 1;
        auto len = langCode.size();
        while (len >= 2) {
            if (strncasecmp(hint.c_str(), langCode.c_str(), len) == 0) {
                score = len;
                break;
            }

            len--;
        }

        if (bestScore < score) {
            bestEntry = entry;
            bestScore = score;
        }
    }
    if (bestEntry) {
        if (!bestEntry->iso_639_1_code.empty()) {
            return bestEntry->iso_639_1_code;
        }
        if (!bestEntry->iso_639_2T_code.empty()) {
            return bestEntry->iso_639_2T_code;
        }
        return bestEntry->iso_639_2B_code;
    }
    return {};
}

KeyboardEngine::KeyboardEngine(Instance *instance) : m_instance(instance) {
    m_isoCodes.read(ISOCODES_ISO639_XML, ISOCODES_ISO3166_XML);
    auto xcb = m_instance->addonManager().addon("xcb");
    std::string rule;
    ;
    if (xcb) {
        auto rules = xcb->call<IXCBModule::xkbRulesNames>("");
        if (!rules[0].empty()) {
            rule = rules[0];
            if (rule[0] == '/') {
                rule += ".xml";
            } else {
                rule = XKEYBOARDCONFIG_XKBBASE "/rules/" + rule + ".xml";
            }
        }
    }
    if (rule.empty() || !m_xkbRules.read(rule)) {
        rule = XKEYBOARDCONFIG_XKBBASE "/rules/" DEFAULT_XKB_RULES ".xml";
        m_xkbRules.read(rule);
    }
}

KeyboardEngine::~KeyboardEngine() {}

std::vector<InputMethodEntry> KeyboardEngine::listInputMethods() {
    std::vector<InputMethodEntry> result;
    for (auto &p : m_xkbRules.layoutInfos()) {
        auto &layoutInfo = p.second;
        auto language = findBestLanguage(m_isoCodes, layoutInfo.description, layoutInfo.languages);
        auto description =
            stringutils::join({_("Keyboard"), " - ", D_("xkeyboard-config", layoutInfo.description)}, "");
        auto uniqueName = "fcitx-keyboard-" + layoutInfo.name;
        result.emplace_back(
            std::move(InputMethodEntry(uniqueName, description, language).setIcon("kbd").setLabel(layoutInfo.name)));
        for (auto &variantInfo : layoutInfo.variantInfos) {
            auto language =
                findBestLanguage(m_isoCodes, variantInfo.description,
                                 variantInfo.languages.size() ? variantInfo.languages : layoutInfo.languages);
            auto description = stringutils::join({_("Keyboard"), " - ", D_("xkeyboard-config", layoutInfo.description),
                                                  " - ", D_("xkeyboard-config", variantInfo.description)},
                                                 "");
            auto uniqueName = "fcitx-keyboard-" + layoutInfo.name + "-" + variantInfo.name;
            result.emplace_back(std::move(
                InputMethodEntry(uniqueName, description, language).setIcon("kbd").setLabel(layoutInfo.name)));
        }
    }
    return result;
}

void KeyboardEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &event) {}
}
