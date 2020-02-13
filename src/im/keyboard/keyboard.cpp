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

#include "keyboard.h"
#include "chardata.h"
#include "config.h"
#include "emoji_public.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/cutf8.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "notifications_public.h"
#include "spell_public.h"
#include "xcb_public.h"
#include <cstring>
#include <fcntl.h>
#include <fmt/format.h>
#include <libintl.h>

const char imNamePrefix[] = "keyboard-";
#define FCITX_KEYBOARD_MAX_BUFFER 20

namespace fcitx {

namespace {

enum class SpellType { AllLower, Mixed, FirstUpper, AllUpper };

SpellType guessSpellType(const std::string &input) {
    if (input.size() <= 1) {
        if (charutils::isupper(input[0])) {
            return SpellType::FirstUpper;
        } else {
            return SpellType::AllLower;
        }
    }

    if (std::all_of(input.begin(), input.end(),
                    [](char c) { return charutils::isupper(c); })) {
        return SpellType::AllUpper;
    }

    if (std::all_of(input.begin() + 1, input.end(),
                    [](char c) { return charutils::islower(c); })) {
        if (charutils::isupper(input[0])) {
            return SpellType::FirstUpper;
        } else {
            return SpellType::AllLower;
        }
    }

    return SpellType::Mixed;
}

std::string formatWord(const std::string &input, SpellType type) {
    if (type == SpellType::Mixed || type == SpellType::AllLower) {
        return input;
    }
    if (guessSpellType(input) != SpellType::AllLower) {
        return input;
    }
    std::string result;
    if (type == SpellType::AllUpper) {
        result.reserve(input.size());
        std::transform(input.begin(), input.end(), std::back_inserter(result),
                       charutils::toupper);
    } else {
        // FirstUpper
        result = input;
        if (!result.empty()) {
            result[0] = charutils::toupper(result[0]);
        }
    }
    return result;
}

static std::string findBestLanguage(const IsoCodes &isocodes,
                                    const std::string &hint,
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
} // namespace

KeyboardEngine::KeyboardEngine(Instance *instance) : instance_(instance) {
    registerDomain("xkeyboard-config", XKEYBOARDCONFIG_DATADIR "/locale");
    isoCodes_.read(ISOCODES_ISO639_JSON, ISOCODES_ISO3166_JSON);
    auto xcb = instance_->addonManager().addon("xcb");
    std::string rule;
    if (xcb) {
        auto rules = xcb->call<IXCBModule::xkbRulesNames>("");
        if (!rules[0].empty()) {
            rule = rules[0];
            if (rule[0] != '/') {
                rule = XKEYBOARDCONFIG_XKBBASE "/rules/" + rule;
            }
            rule += ".xml";
            ruleName_ = rule;
        }
    }
    if (rule.empty() || !xkbRules_.read(rule)) {
        rule = XKEYBOARDCONFIG_XKBBASE "/rules/" DEFAULT_XKB_RULES ".xml";
        xkbRules_.read(rule);
        ruleName_ = DEFAULT_XKB_RULES;
    }

    instance_->inputContextManager().registerProperty("keyboardState",
                                                      &factory_);
    reloadConfig();
}

KeyboardEngine::~KeyboardEngine() {}

std::vector<InputMethodEntry> KeyboardEngine::listInputMethods() {
    std::vector<InputMethodEntry> result;
    bool usExists = false;
    for (auto &p : xkbRules_.layoutInfos()) {
        auto &layoutInfo = p.second;
        auto language = findBestLanguage(isoCodes_, layoutInfo.description,
                                         layoutInfo.languages);
        auto description =
            fmt::format(_("Keyboard - {0}"),
                        D_("xkeyboard-config", layoutInfo.description));
        auto uniqueName = imNamePrefix + layoutInfo.name;
        if (uniqueName == "keyboard-us") {
            usExists = true;
        }
        result.push_back(std::move(
            InputMethodEntry(uniqueName, description, language, "keyboard")
                .setLabel(layoutInfo.name)
                .setIcon("input-keyboard")
                .setConfigurable(true)));
        for (auto &variantInfo : layoutInfo.variantInfos) {
            auto language = findBestLanguage(isoCodes_, variantInfo.description,
                                             variantInfo.languages.size()
                                                 ? variantInfo.languages
                                                 : layoutInfo.languages);
            auto description =
                fmt::format(_("Keyboard - {0} - {1}"),
                            D_("xkeyboard-config", layoutInfo.description),
                            D_("xkeyboard-config", variantInfo.description));
            auto uniqueName = stringutils::concat(imNamePrefix, layoutInfo.name,
                                                  "-", variantInfo.name);
            result.push_back(std::move(
                InputMethodEntry(uniqueName, description, language, "keyboard")
                    .setLabel(layoutInfo.name)
                    .setIcon("input-keyboard")));
        }
    }

    if (result.empty()) {
        RawConfig config;
        readAsIni(config, "conf/cached_layouts");
        for (auto &uniqueName : config.subItems()) {
            auto desc = config.valueByPath(
                stringutils::joinPath(uniqueName, "Description"));
            auto lang = config.valueByPath(
                stringutils::joinPath(uniqueName, "Language"));
            auto label =
                config.valueByPath(stringutils::joinPath(uniqueName, "Label"));
            if (desc && lang && label) {
                if (uniqueName == "keyboard-us") {
                    usExists = true;
                }
                result.push_back(
                    std::move(InputMethodEntry(
                                  uniqueName,
                                  fmt::format(_("{0} (Not Available)"), *desc),
                                  *lang, "keyboard")
                                  .setLabel(*label)
                                  .setIcon("input-keyboard")));
            }
        }
    } else {
        RawConfig config;
        for (auto &item : result) {
            config.setValueByPath(
                stringutils::joinPath(item.uniqueName(), "Description"),
                item.name());
            config.setValueByPath(
                stringutils::joinPath(item.uniqueName(), "Language"),
                item.languageCode());
            config.setValueByPath(
                stringutils::joinPath(item.uniqueName(), "Label"),
                item.label());
        }
        safeSaveAsIni(config, "conf/cached_layouts");
    }
    if (!usExists) {
        result.push_back(std::move(
            InputMethodEntry("keyboard-us", _("Keyboard"), "en", "keyboard")
                .setLabel("us")
                .setIcon("input-keyboard")
                .setConfigurable(true)));
    }
    return result;
}

void KeyboardEngine::reloadConfig() {
    readAsIni(config_, "conf/keyboard.conf");
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

static inline bool isValidSym(const Key &key) {
    if (key.states())
        return false;

    return validSyms.count(key.sym());
}

static inline bool isValidCharacter(uint32_t c) {
    if (c == 0 || c == FCITX_INVALID_COMPOSE_RESULT)
        return false;

    return validChars.count(c);
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
    auto state = inputContext->propertyFor(&factory_);

    // check compose first.
    auto compose = instance_->processCompose(inputContext, event.key().sym());

    // compose is invalid, ignore it.
    if (compose == FCITX_INVALID_COMPOSE_RESULT) {
        return event.filterAndAccept();
    }

    // check the spell trigger key
    if (event.key().checkKeyList(config_.hintTrigger.value()) && spell() &&
        spell()->call<ISpell::checkDict>(entry.languageCode())) {
        state->enableWordHint_ = !state->enableWordHint_;
        commitBuffer(inputContext);
        if (notifications()) {
            notifications()->call<INotifications::showTip>(
                "fcitx-keyboard-hint", "fcitx", "tools-check-spelling",
                _("Spell hint"),
                state->enableWordHint_ ? _("Spell hint is enabled.")
                                       : _("Spell hint is disabled."),
                -1);
        }
        return event.filterAndAccept();
    }

    do {
        // no spell hint enabled, ignore
        if (!state->enableWordHint_) {
            break;
        }
        const CapabilityFlags noPredictFlag{CapabilityFlag::Password,
                                            CapabilityFlag::NoSpellCheck,
                                            CapabilityFlag::Sensitive};
        // no supported dictionary
        if (!spell() ||
            !spell()->call<ISpell::checkDict>(entry.languageCode()) ||
            inputContext->capabilityFlags().testAny(noPredictFlag)) {
            break;
        }

        // check if we can select candidate.
        if (auto candList = inputContext->inputPanel().candidateList()) {
            int idx = event.key().keyListIndex(selectionKeys_);
            if (idx >= 0 && idx < candList->size()) {
                event.filterAndAccept();
                candList->candidate(idx).select(inputContext);
                return;
            }

            auto movable = candList->toCursorMovable();
            if (movable) {
                if (event.key().checkKeyList(*config_.nextCandidate)) {
                    movable->nextCandidate();
                    updateUI(inputContext);
                    return event.filterAndAccept();
                } else if (event.key().checkKeyList(*config_.prevCandidate)) {
                    movable->prevCandidate();
                    updateUI(inputContext);
                    return event.filterAndAccept();
                }
            }
        }

        auto &buffer = state->buffer_;
        bool validCharacter = isValidCharacter(compose);
        bool validSym = isValidSym(event.key());

        // check for valid character
        if (validCharacter || event.key().isSimple() || validSym) {
            if (validCharacter || event.key().isLAZ() || event.key().isUAZ() ||
                validSym ||
                (!buffer.empty() &&
                 event.key().checkKeyList(FCITX_HYPHEN_APOS))) {
                auto preedit = preeditString(inputContext);
                if (preedit != buffer.userInput()) {
                    buffer.clear();
                    buffer.type(preedit);
                }

                if (compose) {
                    buffer.type(compose);
                } else {
                    buffer.type(Key::keySymToUnicode(event.key().sym()));
                }

                event.filterAndAccept();
                if (buffer.size() >= FCITX_KEYBOARD_MAX_BUFFER) {
                    commitBuffer(inputContext);
                    return;
                }

                return updateCandidate(entry, inputContext);
            }
        } else {
            if (event.key().check(FcitxKey_BackSpace)) {
                if (buffer.backspace()) {
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
        inputContext->commitString(composeString);
    }
}

void KeyboardEngine::commitBuffer(InputContext *inputContext) {
    auto state = inputContext->propertyFor(&factory_);
    auto &buffer = state->buffer_;
    if (!buffer.empty()) {
        inputContext->commitString(preeditString(inputContext));
        resetState(inputContext);
        inputContext->inputPanel().reset();
        inputContext->updatePreedit();
        inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
}

class KeyboardCandidateWord : public CandidateWord {
public:
    KeyboardCandidateWord(KeyboardEngine *engine, Text text)
        : CandidateWord(std::move(text)), engine_(engine) {}

    void select(InputContext *inputContext) const override {
        auto commit = text().toString();
        inputContext->inputPanel().reset();
        inputContext->updatePreedit();
        inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
        inputContext->commitString(commit);
        engine_->resetState(inputContext);
    }

private:
    KeyboardEngine *engine_;
};

std::string KeyboardEngine::preeditString(InputContext *inputContext) {
    auto state = inputContext->propertyFor(&factory_);
    auto candidate = inputContext->inputPanel().candidateList();
    std::string preedit;
    if (candidate && candidate->cursorIndex() >= 0) {
        return candidate->candidate(candidate->cursorIndex()).text().toString();
    }
    return state->buffer_.userInput();
}

void KeyboardEngine::updateUI(InputContext *inputContext) {
    Text preedit(preeditString(inputContext), TextFormatFlag::Underline);
    if (auto length = preedit.textLength()) {
        preedit.setCursor(length);
    }
    inputContext->inputPanel().setClientPreedit(preedit);
    if (!inputContext->capabilityFlags().test(CapabilityFlag::Preedit)) {
        inputContext->inputPanel().setPreedit(preedit);
    }
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void KeyboardEngine::updateCandidate(const InputMethodEntry &entry,
                                     InputContext *inputContext) {
    auto state = inputContext->propertyFor(&factory_);
    auto results = spell()->call<ISpell::hint>(entry.languageCode(),
                                               state->buffer_.userInput(),
                                               config_.pageSize.value());
    if (config_.enableEmoji.value() && emoji()) {
        auto emojiResults = emoji()->call<IEmoji::query>(
            entry.languageCode(), state->buffer_.userInput(), true);
        // If we have emoji result and spell result is full, pop one from the
        // original result, because emoji matching is always exact. Which means
        // the spell is right.
        if (!emojiResults.empty() &&
            static_cast<int>(results.size()) == config_.pageSize.value()) {
            results.pop_back();
        }
        size_t i = 0;
        while (i < emojiResults.size() &&
               static_cast<int>(results.size()) < config_.pageSize.value()) {
            results.push_back(emojiResults[i]);
            i++;
        }
    }

    auto candidateList = std::make_unique<CommonCandidateList>();
    auto spellType = guessSpellType(state->buffer_.userInput());
    for (const auto &result : results) {
        candidateList->append<KeyboardCandidateWord>(
            this, Text(formatWord(result, spellType)));
    }
    candidateList->setSelectionKey(selectionKeys_);
    candidateList->setCursorIncludeUnselected(true);
    inputContext->inputPanel().setCandidateList(std::move(candidateList));

    updateUI(inputContext);
}

void KeyboardEngine::resetState(InputContext *inputContext) {
    auto state = inputContext->propertyFor(&factory_);
    state->reset();
    instance_->resetCompose(inputContext);
}

void KeyboardEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto inputContext = event.inputContext();
    // The reason that we do not commit here is we want to force the behavior.
    // When client get unfocused, the framework will try to commit the string.
    if (event.type() != EventType::InputContextFocusOut) {
        commitBuffer(inputContext);
    } else {
        resetState(inputContext);
    }
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

bool KeyboardEngine::foreachLayout(
    const std::function<
        bool(const std::string &variant, const std::string &description,
             const std::vector<std::string> &languages)> &callback) {
    for (auto &p : xkbRules_.layoutInfos()) {
        if (!callback(p.second.name, p.second.description,
                      p.second.languages)) {
            return false;
        }
    }
    return true;
}

bool KeyboardEngine::foreachVariant(
    const std::string &layout,
    const std::function<
        bool(const std::string &variant, const std::string &description,
             const std::vector<std::string> &languages)> &callback) {
    auto info = xkbRules_.findByName(layout);
    for (auto &p : info->variantInfos) {
        if (!callback(p.name, p.description, p.languages)) {
            return false;
        }
    }
    return true;
}
} // namespace fcitx
