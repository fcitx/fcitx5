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
#include "chardata.h"
#include "config.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/cutf8.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "notifications_public.h"
#include "spell_public.h"
#include "xcb_public.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libintl.h>

const char imNamePrefix[] = "fcitx-keyboard-";
const int imNamePrefixLength = sizeof(imNamePrefix) - 1;
#define INVALID_COMPOSE_RESULT 0xffffffff
#define FCITX_KEYBOARD_MAX_BUFFER 20

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

struct KeyboardEngineState : public InputContextProperty {
    KeyboardEngineState(KeyboardEngine *engine) : xkbComposeState_(nullptr, &xkb_compose_state_unref) {
        if (engine->xkbComposeTable()) {
            xkbComposeState_.reset(xkb_compose_state_new(engine->xkbComposeTable(), XKB_COMPOSE_STATE_NO_FLAGS));
        }
    }

    bool enableWordHint_ = false;
    std::string buffer_;
    int cursorPos_ = 0;
    std::unique_ptr<struct xkb_compose_state, decltype(&xkb_compose_state_unref)> xkbComposeState_;

    void reset() {
        buffer_.clear();
        cursorPos_ = 0;
        xkb_compose_state_reset(xkbComposeState_.get());
    }
};

KeyboardEngine::KeyboardEngine(Instance *instance)
    : instance_(instance), xkbContext_(nullptr, &xkb_context_unref),
      xkbComposeTable_(nullptr, &xkb_compose_table_unref) {
    isoCodes_.read(ISOCODES_ISO639_XML, ISOCODES_ISO3166_XML);
    auto xcb = instance_->addonManager().addon("xcb");
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
            ruleName_ = rule;
        }
    }
    if (rule.empty() || !xkbRules_.read(rule)) {
        rule = XKEYBOARDCONFIG_XKBBASE "/rules/" DEFAULT_XKB_RULES ".xml";
        xkbRules_.read(rule);
        ruleName_ = DEFAULT_XKB_RULES;
    }

    xkbContext_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
    if (xkbContext_) {
        xkb_context_set_log_level(xkbContext_.get(), XKB_LOG_LEVEL_CRITICAL);
        xkbComposeTable_.reset(
            xkb_compose_table_new_from_locale(xkbContext_.get(), locale, XKB_COMPOSE_COMPILE_NO_FLAGS));
    }

    instance_->inputContextManager().registerProperty("keyboardState",
                                                      [this](InputContext &) { return new KeyboardEngineState(this); });
    reloadConfig();
}

KeyboardEngine::~KeyboardEngine() { instance_->inputContextManager().unregisterProperty("keyboardState"); }

std::vector<InputMethodEntry> KeyboardEngine::listInputMethods() {
    std::vector<InputMethodEntry> result;
    for (auto &p : xkbRules_.layoutInfos()) {
        auto &layoutInfo = p.second;
        auto language = findBestLanguage(isoCodes_, layoutInfo.description, layoutInfo.languages);
        auto description =
            stringutils::join({_("Keyboard"), " - ", D_("xkeyboard-config", layoutInfo.description)}, "");
        auto uniqueName = imNamePrefix + layoutInfo.name;
        result.emplace_back(std::move(
            InputMethodEntry(uniqueName, description, language, "keyboard").setIcon("kbd").setLabel(layoutInfo.name)));
        for (auto &variantInfo : layoutInfo.variantInfos) {
            auto language =
                findBestLanguage(isoCodes_, variantInfo.description,
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

void KeyboardEngine::reloadConfig() {
    auto &standardPath = StandardPath::global();
    auto file = standardPath.open(StandardPath::Type::Config, "fcitx5/conf/keyboard.conf", O_RDONLY);
    RawConfig config;
    readFromIni(config, file.fd());

    config_.load(config);
    selectionKeys_.clear();
    KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    KeyStates states;
    switch (config_.chooseModifier.value()) {
    case ChooseModifier::Alt:
        states = KeyState::Alt;
        break;
    case ChooseModifier::Control:
        states = KeyState::Ctrl;
        break;
    case ChooseModifier::Super:
        states = KeyState::Super;
        break;
    default:
        break;
    }

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, states);
    }
}

uint32_t KeyboardEngine::processCompose(KeyboardEngineState *state, uint32_t keyval) {
    // FIXME, should we check if state is 0?
    if (!state->xkbComposeState_) {
        return 0;
    }

    enum xkb_compose_feed_result result = xkb_compose_state_feed(state->xkbComposeState_.get(), keyval);
    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return 0;
    }

    enum xkb_compose_status status = xkb_compose_state_get_status(state->xkbComposeState_.get());
    if (status == XKB_COMPOSE_NOTHING) {
        return 0;
    } else if (status == XKB_COMPOSE_COMPOSED) {
        char buffer[FCITX_UTF8_MAX_LENGTH + 1] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        int length = xkb_compose_state_get_utf8(state->xkbComposeState_.get(), buffer, sizeof(buffer));
        xkb_compose_state_reset(state->xkbComposeState_.get());
        if (length == 0) {
            return INVALID_COMPOSE_RESULT;
        }

        uint32_t c = 0;
        fcitx_utf8_get_char(buffer, &c);
        return c;
    } else if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(state->xkbComposeState_.get());
    }

    return INVALID_COMPOSE_RESULT;
}

static inline bool isValidSym(const Key &key) {
    if (key.states())
        return false;

    return std::binary_search(std::begin(validSyms), std::end(validSyms), key.sym());
}

static inline bool isValidCharacter(uint32_t c) {
    if (c == 0 || c == INVALID_COMPOSE_RESULT)
        return false;

    return std::binary_search(std::begin(validChars), std::end(validChars), c);
}

static KeyList FCITX_HYPHEN_APOS = Key::keyListFromString("minus apostrophe");

void KeyboardEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &event) {
    // FIXME use entry to get layout info
    FCITX_UNUSED(entry);

    // by pass all key release
    if (event.isRelease()) {
        return;
    }

    // and by pass all modifier
    if (event.key().isModifier()) {
        return;
    }
    auto inputContext = event.inputContext();
    auto state = inputContext->propertyAs<KeyboardEngineState>("keyboardState");

    // check compose first.
    auto compose = processCompose(state, event.key().sym());

    // compose is invalid, ignore it.
    if (compose == INVALID_COMPOSE_RESULT) {
        return event.filterAndAccept();
    }

    // check the spell trigger key
    if (event.key().checkKeyList(config_.hintTrigger.value()) && spell() &&
        spell()->call<ISpell::checkDict>(entry.languageCode())) {
        state->enableWordHint_ = !state->enableWordHint_;
        commitBuffer(inputContext);
        if (notifications()) {
            notifications()->call<INotifications::showTip>(
                "fcitx-keyboard-hint", "fcitx", "tools-check-spelling", _("Spell hint"),
                state->enableWordHint_ ? _("Spell hint is enabled.") : _("Spell hint is disabled."), -1);
        }
        return event.filterAndAccept();
    }

    do {
        // no spell hint enabled, ignore
        if (!state->enableWordHint_) {
            break;
        }
        // no supported dictionary
        if (!spell() || !spell()->call<ISpell::checkDict>(entry.languageCode())) {
            break;
        }

        // check if we can select candidate.
        if (inputContext->inputPanel().candidateList()) {
            int idx = event.key().keyListIndex(selectionKeys_);
            if (idx >= 0 && idx < inputContext->inputPanel().candidateList()->size()) {
                event.filterAndAccept();
                inputContext->inputPanel().candidateList()->candidate(idx).select(inputContext);
                return;
            }
        }

        std::string &buffer = state->buffer_;
        int &cursorPos = state->cursorPos_;
        bool validCharacter = isValidCharacter(compose);
        bool validSym = isValidSym(event.key());

        // check for valid character
        if (validCharacter || event.key().isSimple() || validSym) {
            if (validCharacter || event.key().isLAZ() || event.key().isUAZ() || validSym ||
                (!buffer.empty() && event.key().checkKeyList(FCITX_HYPHEN_APOS))) {
                char buf[FCITX_UTF8_MAX_LENGTH + 1];
                memset(buf, 0, sizeof(buf));
                if (compose) {
                    fcitx_ucs4_to_utf8(compose, buf);
                } else {
                    fcitx_ucs4_to_utf8(Key::keySymToUnicode(event.key().sym()), buf);
                }
                size_t charlen = strlen(buf);
                buffer.insert(cursorPos, buf);
                cursorPos += charlen;

                event.filterAndAccept();
                if (buffer.size() >= FCITX_KEYBOARD_MAX_BUFFER) {
                    inputContext->commitString(buffer);
                    state->reset();
                    return;
                }

                return updateCandidate(entry, inputContext);
            }
        } else {
            if (event.key().check(FcitxKey_BackSpace)) {
                if (buffer.size()) {
                    if (cursorPos > 0) {
                        size_t len = utf8::length(buffer);
                        int pos = utf8::nthChar(buffer, len - 1);
                        cursorPos = pos;
                        buffer.erase(pos);
                    }
                    event.filterAndAccept();
                    return updateCandidate(entry, inputContext);
                }
            }
        }

        // if we reach here, just commit and discard buffer.
        commitBuffer(inputContext);
    } while (0);

    // and now we want to forward key.
    if (compose) {
        auto composeString = utf8::UCS4ToUTF8(compose);
        event.filterAndAccept();
        inputContext->commitString("时节");
    }
}

void KeyboardEngine::commitBuffer(InputContext *inputContext) {
    auto state = inputContext->propertyAs<KeyboardEngineState>("keyboardState");
    auto &buffer = state->buffer_;
    if (!buffer.empty()) {
        inputContext->commitString(buffer);
        state->reset();
        inputContext->inputPanel().reset();
        inputContext->updatePreedit();
    }
}

class KeyboardCandidateWord : public CandidateWord {
public:
    KeyboardCandidateWord(Text text) : CandidateWord(text) {}

    void select(InputContext *inputContext) const override {
        auto state = inputContext->propertyAs<KeyboardEngineState>("keyboardState");
        inputContext->commitString(text().toString());
        inputContext->inputPanel().reset();
        inputContext->updatePreedit();
        state->reset();
    }
};

void KeyboardEngine::updateCandidate(const InputMethodEntry &entry, InputContext *inputContext) {
    auto state = inputContext->propertyAs<KeyboardEngineState>("keyboardState");
    auto results = spell()->call<ISpell::hint>(entry.languageCode(), state->buffer_, config_.pageSize.value());
    auto candidateList = new CommonCandidateList;
    for (const auto &result : results) {
        candidateList->append(new KeyboardCandidateWord(Text(result)));
    }
    candidateList->setSelectionKey(selectionKeys_);
    Text preedit(state->buffer_);
    preedit.setCursor(state->cursorPos_);
    inputContext->inputPanel().setClientPreedit(std::move(preedit));
    if (inputContext->capabilityFlags()) {
    }
    inputContext->inputPanel().setCandidateList(candidateList);
    inputContext->updatePreedit();
}
void KeyboardEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto inputContext = event.inputContext();
    auto state = inputContext->propertyAs<KeyboardEngineState>("keyboardState");
    state->reset();
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
}
}
