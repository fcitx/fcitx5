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

#include "keyboard.h"
#include "config.h"
#include "fcitx-utils/cutf8.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "modules/xcb/xcb_public.h"
#include <libintl.h>
#include <string.h>

const char imNamePrefix[] = "fcitx-keyboard-";
const int imNamePrefixLength = sizeof(imNamePrefix) - 1;
#define INVALID_COMPOSE_RESULT 0xffffffff

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

std::pair<std::string, std::string> layoutFromName(const std::string &s) {
    auto pos = s.find('-', imNamePrefixLength);
    if (pos == std::string::npos) {
        return {s.substr(imNamePrefixLength), ""};
    }
    return {s.substr(imNamePrefixLength, pos - imNamePrefixLength), s.substr(pos + 1)};
}

KeyboardEngine::KeyboardEngine(Instance *instance)
    : m_instance(instance), m_xkbContext(nullptr, &xkb_context_unref),
      m_xkbComposeTable(nullptr, &xkb_compose_table_unref), m_xkbComposeState(nullptr, &xkb_compose_state_unref) {
    m_isoCodes.read(ISOCODES_ISO639_XML, ISOCODES_ISO3166_XML);
    auto xcb = m_instance->addonManager().addon("xcb");
    std::string rule;

    const char *locale = getenv("LC_ALL");
    if (!locale) {
        locale = getenv("LC_CTYPE");
    }
    if (!locale) {
        locale = getenv("LANG");
    }
    if (!locale) {
        locale = "C";
    }
    if (xcb) {
        auto rules = xcb->call<IXCBModule::xkbRulesNames>("");
        if (!rules[0].empty()) {
            rule = rules[0];
            if (rule[0] == '/') {
                rule += ".xml";
            } else {
                rule = XKEYBOARDCONFIG_XKBBASE "/rules/" + rule + ".xml";
            }
            m_ruleName = rule;
        }
    }
    if (rule.empty() || !m_xkbRules.read(rule)) {
        rule = XKEYBOARDCONFIG_XKBBASE "/rules/" DEFAULT_XKB_RULES ".xml";
        m_xkbRules.read(rule);
        m_ruleName = DEFAULT_XKB_RULES;
    }

    m_xkbContext.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
    if (m_xkbContext) {
        xkb_context_set_log_level(m_xkbContext.get(), XKB_LOG_LEVEL_CRITICAL);
        m_xkbComposeTable.reset(
            xkb_compose_table_new_from_locale(m_xkbContext.get(), locale, XKB_COMPOSE_COMPILE_NO_FLAGS));
        if (m_xkbComposeTable) {
            m_xkbComposeState.reset(xkb_compose_state_new(m_xkbComposeTable.get(), XKB_COMPOSE_STATE_NO_FLAGS));
        }
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
        auto uniqueName = imNamePrefix + layoutInfo.name;
        result.emplace_back(std::move(
            InputMethodEntry(uniqueName, description, language, "keyboard").setIcon("kbd").setLabel(layoutInfo.name)));
        for (auto &variantInfo : layoutInfo.variantInfos) {
            auto language =
                findBestLanguage(m_isoCodes, variantInfo.description,
                                 variantInfo.languages.size() ? variantInfo.languages : layoutInfo.languages);
            auto description = stringutils::join({_("Keyboard"), " - ", D_("xkeyboard-config", layoutInfo.description),
                                                  " - ", D_("xkeyboard-config", variantInfo.description)},
                                                 "");
            auto uniqueName = imNamePrefix + layoutInfo.name + "-" + variantInfo.name;
            result.emplace_back(std::move(InputMethodEntry(uniqueName, description, language, "keyboard")
                                              .setIcon("kbd")
                                              .setLabel(layoutInfo.name)));
        }
    }
    return result;
}

uint32_t KeyboardEngine::processCompose(uint32_t keyval, uint32_t state) {
    if (!m_xkbComposeState) {
        return 0;
    }

    enum xkb_compose_feed_result result = xkb_compose_state_feed(m_xkbComposeState.get(), keyval);
    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return 0;
    }

    enum xkb_compose_status status = xkb_compose_state_get_status(m_xkbComposeState.get());
    if (status == XKB_COMPOSE_NOTHING) {
        return 0;
    } else if (status == XKB_COMPOSE_COMPOSED) {
        char buffer[FCITX_UTF8_MAX_LENGTH + 1] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        int length = xkb_compose_state_get_utf8(m_xkbComposeState.get(), buffer, sizeof(buffer));
        xkb_compose_state_reset(m_xkbComposeState.get());
        if (length == 0) {
            return INVALID_COMPOSE_RESULT;
        }

        uint32_t c = 0;
        fcitx_utf8_get_char(buffer, &c);
        return c;
    } else if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(m_xkbComposeState.get());
    }

    return INVALID_COMPOSE_RESULT;
}

void KeyboardEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &event) {
    if (event.isRelease()) {
        return;
    }

    auto sym = event.key().sym();

    if (sym == FcitxKey_Shift_L || sym == FcitxKey_Shift_R || sym == FcitxKey_Alt_L || sym == FcitxKey_Alt_R ||
        sym == FcitxKey_Control_L || sym == FcitxKey_Control_R || sym == FcitxKey_Super_L || sym == FcitxKey_Super_R) {
        return;
    }

    auto compose = processCompose(event.key().sym(), event.key().states());
    if (compose == INVALID_COMPOSE_RESULT) {
        event.accept();
        return;
    }

    if (compose) {
        auto composeString = utf8::UCS4ToUTF8(compose);
        event.accept();
        event.inputContext()->commitString(composeString);
    }
}
}
