/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "keyboard.h"
#include <fcntl.h>
#include <cstring>
#include <fmt/format.h>
#include <libintl.h>
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
#include "chardata.h"
#include "config.h"
#include "longpress.h"
#include "notifications_public.h"
#include "quickphrase_public.h"
#include "spell_public.h"

#ifdef ENABLE_X11
#include "xcb_public.h"
#endif

#include "emoji_public.h"

const char imNamePrefix[] = "keyboard-";
#define FCITX_KEYBOARD_MAX_BUFFER 20

namespace fcitx {

namespace {
enum class SpellType { AllLower, Mixed, FirstUpper, AllUpper };

SpellType guessSpellType(const std::string &input) {
    if (input.size() <= 1) {
        if (charutils::isupper(input[0])) {
            return SpellType::FirstUpper;
        }
        return SpellType::AllLower;
    }

    if (std::all_of(input.begin(), input.end(),
                    [](char c) { return charutils::isupper(c); })) {
        return SpellType::AllUpper;
    }

    if (std::all_of(input.begin() + 1, input.end(),
                    [](char c) { return charutils::islower(c); })) {
        if (charutils::isupper(input[0])) {
            return SpellType::FirstUpper;
        }
        return SpellType::AllLower;
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

std::string findBestLanguage(const IsoCodes &isocodes, const std::string &hint,
                             const std::vector<std::string> &languages) {
    /* score:
     * 1 -> first one
     * 2 -> match 2
     * 3 -> match three
     */
    const IsoCodes639Entry *bestEntry = nullptr;
    int bestScore = 0;
    for (const auto &language : languages) {
        const auto *entry = isocodes.entry(language);
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

class LongPressCandidateWord : public CandidateWord {
public:
    LongPressCandidateWord(KeyboardEngine *engine, const std::string &text,
                           int index)
        : CandidateWord(Text(stringutils::concat(text, "\n", index))),
          engine_(engine), text_(text) {}

    const std::string &str() const { return text_; }

    void select(InputContext *inputContext) const override {
        auto state = inputContext->propertyFor(engine_->factory());
        state->mode_ = CandidateMode::Hint;
        // Make sure the candidate is from not from long press mode.
        inputContext->inputPanel().setCandidateList(nullptr);
        if (!engine_->updateBuffer(inputContext, text_)) {
            engine_->commitBuffer(inputContext);
            inputContext->inputPanel().reset();
            engine_->updateUI(inputContext);
            inputContext->commitString(text_);
        }
    }

private:
    KeyboardEngine *engine_;
    std::string text_;
};

} // namespace

KeyboardEngineState::KeyboardEngineState(KeyboardEngine *engine) {
    enableWordHint_ = engine->config().enableHintByDefault.value();
}

KeyboardEngine::KeyboardEngine(Instance *instance) : instance_(instance) {
    setupDefaultLongPressConfig(longPressConfig_);
    registerDomain("xkeyboard-config", XKEYBOARDCONFIG_DATADIR "/locale");
    std::string rule;
#ifdef ENABLE_X11
    auto *xcb = instance_->addonManager().addon("xcb");
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
#endif
    if (rule.empty() || !xkbRules_.read(rule)) {
        rule = XKEYBOARDCONFIG_XKBBASE "/rules/" DEFAULT_XKB_RULES ".xml";
        xkbRules_.read(rule);
        ruleName_ = DEFAULT_XKB_RULES;
    }

    instance_->inputContextManager().registerProperty("keyboardState",
                                                      &factory_);
    reloadConfig();

    deferEvent_ = instance_->eventLoop().addDeferEvent([this](EventSource *) {
        initQuickPhrase();
        deferEvent_.reset();
        return true;
    });
}

KeyboardEngine::~KeyboardEngine() {}

std::vector<InputMethodEntry> KeyboardEngine::listInputMethods() {
    IsoCodes isoCodes;
    isoCodes.read(ISOCODES_ISO639_JSON, ISOCODES_ISO3166_JSON);

    std::vector<InputMethodEntry> result;
    bool usExists = false;
    for (const auto &p : xkbRules_.layoutInfos()) {
        const auto &layoutInfo = p.second;
        auto language = findBestLanguage(isoCodes, layoutInfo.description,
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
                .setLabel(layoutInfo.shortDescription.empty()
                              ? layoutInfo.name
                              : layoutInfo.shortDescription)
                .setIcon("input-keyboard")
                .setConfigurable(true)));
        for (const auto &variantInfo : layoutInfo.variantInfos) {
            auto language = findBestLanguage(isoCodes, variantInfo.description,
                                             !variantInfo.languages.empty()
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
                    .setLabel(variantInfo.shortDescription.empty()
                                  ? layoutInfo.name
                                  : variantInfo.shortDescription)
                    .setIcon("input-keyboard")
                    .setConfigurable(true)));
        }
    }

    if (result.empty()) {
        RawConfig config;
        readAsIni(config, "conf/cached_layouts");
        for (auto &uniqueName : config.subItems()) {
            const auto *desc = config.valueByPath(
                stringutils::joinPath(uniqueName, "Description"));
            const auto *lang = config.valueByPath(
                stringutils::joinPath(uniqueName, "Language"));
            const auto *label =
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
                .setLabel("en")
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
    longPressBlocklistSet_ = decltype(longPressBlocklistSet_)(
        config_.blocklistApplicationForLongPress->begin(),
        config_.blocklistApplicationForLongPress->end());

    readAsIni(longPressConfig_, "conf/keyboard-longpress.conf");
    longPressData_ = longPressData(longPressConfig_);
}

static inline bool isValidSym(const Key &key) {
    if (key.states()) {
        return false;
    }

    return validSyms.count(key.sym());
}

static inline bool isValidCharacter(const std::string &c) {
    if (c.empty()) {
        return false;
    }

    uint32_t code;
    auto iter = utf8::getNextChar(c.begin(), c.end(), &code);

    return iter == c.end() && validChars.count(code);
}

static KeyList FCITX_HYPHEN_APOS = Key::keyListFromString("minus apostrophe");

bool KeyboardEngine::updateBuffer(InputContext *inputContext,
                                  const std::string &chr) {
    auto *entry = instance_->inputMethodEntry(inputContext);
    if (!entry) {
        return false;
    }

    auto *state = inputContext->propertyFor(&factory_);
    const CapabilityFlags noPredictFlag{CapabilityFlag::Password,
                                        CapabilityFlag::NoSpellCheck,
                                        CapabilityFlag::Sensitive};
    // no spell hint enabled or no supported dictionary
    if (!state->hintEnabled() ||
        inputContext->capabilityFlags().testAny(noPredictFlag) ||
        !supportHint(entry->languageCode())) {
        return false;
    }

    auto &buffer = state->buffer_;
    auto preedit = preeditString(inputContext);
    if (preedit != buffer.userInput()) {
        buffer.clear();
        buffer.type(preedit);
    }

    buffer.type(chr);

    if (buffer.size() >= FCITX_KEYBOARD_MAX_BUFFER) {
        commitBuffer(inputContext);
        return true;
    }

    updateCandidate(*entry, inputContext);
    return true;
}

void KeyboardEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &event) {
    FCITX_UNUSED(entry);
    auto *inputContext = event.inputContext();

    // by pass all key release
    if (event.isRelease()) {
        return;
    }

    auto *state = inputContext->propertyFor(&factory_);
    if (state->repeatStarted_ &&
        !event.rawKey().states().test(KeyState::Repeat)) {
        state->repeatStarted_ = false;
    }

    // and by pass all modifier
    if (event.key().isModifier()) {
        return;
    }

    auto &buffer = state->buffer_;
    auto keystr = Key::keySymToUTF8(event.key().sym());
    if (!event.key().states() &&
        event.rawKey().states().test(KeyState::Repeat) &&
        *config_.enableLongPress &&
        !longPressBlocklistSet_.count(inputContext->program())) {
        if (auto results = findValue(longPressData_, keystr)) {
            if (state->repeatStarted_) {
                return event.filterAndAccept();
            }
            state->repeatStarted_ = true;

            state->mode_ = CandidateMode::LongPress;
            state->origKeyString_ = keystr;
            if (buffer.empty()) {
                if (inputContext->capabilityFlags().test(
                        CapabilityFlag::SurroundingText)) {
                    inputContext->deleteSurroundingText(-1, 1);
                } else {
                    inputContext->forwardKey(Key(FcitxKey_BackSpace));
                }
            } else {
                buffer.backspace();
            }

            inputContext->inputPanel().reset();
            auto candidateList = std::make_unique<CommonCandidateList>();
            int i = 1;
            for (const auto &result : *results) {
                candidateList->append<LongPressCandidateWord>(this, result,
                                                              i++);
            }
            candidateList->setPageSize(results->size());
            candidateList->setCursorIncludeUnselected(true);
            inputContext->inputPanel().setCandidateList(
                std::move(candidateList));

            updateUI(inputContext);
            return event.filterAndAccept();
        }
    }

    // check compose first.
    auto composeResult =
        instance_->processComposeString(inputContext, event.key().sym());

    // compose is invalid, ignore it.
    if (!composeResult) {
        return event.filterAndAccept();
    }

    auto compose = *composeResult;

    // check the spell trigger key
    if ((event.key().checkKeyList(config_.hintTrigger.value()) ||
         event.origKey().normalize().checkKeyList(
             config_.hintTrigger.value())) &&
        supportHint(entry.languageCode())) {
        state->enableWordHint_ = !state->enableWordHint_;
        state->oneTimeEnableWordHint_ = false;
        commitBuffer(inputContext);
        showHintNotification(entry, state);
        return event.filterAndAccept();
    }

    // check the spell trigger key
    if ((event.key().checkKeyList(config_.oneTimeHintTrigger.value()) ||
         event.origKey().normalize().checkKeyList(
             config_.oneTimeHintTrigger.value())) &&
        supportHint(entry.languageCode())) {
        bool oldOneTime = state->oneTimeEnableWordHint_;
        state->enableWordHint_ = false;
        commitBuffer(inputContext);
        // This need to be set after commit buffer.
        state->oneTimeEnableWordHint_ = !oldOneTime;
        showHintNotification(entry, state);
        return event.filterAndAccept();
    }

    // check if we can select candidate.
    if (auto candList = inputContext->inputPanel().candidateList()) {
        int idx = event.key().keyListIndex(selectionKeys_);
        if (idx >= 0 && idx < candList->size()) {
            event.filterAndAccept();
            candList->candidate(idx).select(inputContext);
            return;
        }

        auto *movable = candList->toCursorMovable();
        if (movable) {
            if (event.key().checkKeyList(*config_.nextCandidate)) {
                movable->nextCandidate();
                updateUI(inputContext);
                return event.filterAndAccept();
            }
            if (event.key().checkKeyList(*config_.prevCandidate)) {
                movable->prevCandidate();
                updateUI(inputContext);
                return event.filterAndAccept();
            }
        }
    }

    bool validCharacter = isValidCharacter(compose);
    bool validSym = isValidSym(event.key());

    // check for valid character
    if (validCharacter || event.key().isSimple() || validSym) {
        if (validCharacter || event.key().isLAZ() || event.key().isUAZ() ||
            validSym ||
            (!buffer.empty() && event.key().checkKeyList(FCITX_HYPHEN_APOS))) {
            auto text = !compose.empty() ? compose
                                         : Key::keySymToUTF8(event.key().sym());
            if (updateBuffer(inputContext, text)) {
                return event.filterAndAccept();
            }
        }
    } else if (event.key().check(FcitxKey_BackSpace)) {
        if (buffer.backspace()) {
            event.filterAndAccept();
            if (buffer.empty()) {
                return reset(entry, event);
            }
            return updateCandidate(entry, inputContext);
        }
    }

    // if we reach here, just commit and discard buffer.
    commitBuffer(inputContext);
    // and now we want to forward key.
    if (!compose.empty()) {
        event.filterAndAccept();
        inputContext->commitString(compose);
    }
}

void KeyboardEngine::commitBuffer(InputContext *inputContext) {
    auto preedit = preeditString(inputContext);
    if (preedit.empty()) {
        return;
    }
    inputContext->commitString(preedit);
    resetState(inputContext);
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

bool KeyboardEngine::supportHint(const std::string &language) {
    const bool hasSpell = spell() && spell()->call<ISpell::checkDict>(language);
    const bool hasEmoji = *config_.enableEmoji && emoji() &&
                          emoji()->call<IEmoji::check>(language, true);
    return hasSpell || hasEmoji;
}

std::string KeyboardEngine::preeditString(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
    auto candidateList = inputContext->inputPanel().candidateList();
    std::string preedit;
    if (state->mode_ == CandidateMode::Hint) {
        if (candidateList && candidateList->cursorIndex() >= 0) {
            if (const auto *candidate =
                    dynamic_cast<const KeyboardCandidateWord *>(
                        &candidateList->candidate(
                            candidateList->cursorIndex()))) {
                return candidate->text().toString();
            }
        }
        return state->buffer_.userInput();
    } else if (state->mode_ == CandidateMode::LongPress) {
        if (candidateList && candidateList->cursorIndex() >= 0) {
            if (const auto *candidate =
                    dynamic_cast<const LongPressCandidateWord *>(
                        &candidateList->candidate(
                            candidateList->cursorIndex()))) {
                return state->buffer_.userInput() + candidate->str();
            }
        }
        return state->buffer_.userInput() + state->origKeyString_;
    }
    return state->buffer_.userInput();
}

void KeyboardEngine::updateUI(InputContext *inputContext) {
    if (inputContext->capabilityFlags().test(CapabilityFlag::Preedit)) {
        Text preedit(preeditString(inputContext), TextFormatFlag::HighLight);
        inputContext->inputPanel().setClientPreedit(preedit);
    } else {
        Text preedit(preeditString(inputContext));
        if (auto length = preedit.textLength()) {
            preedit.setCursor(length);
        }
        inputContext->inputPanel().setPreedit(preedit);
    }
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void KeyboardEngine::updateCandidate(const InputMethodEntry &entry,
                                     InputContext *inputContext) {
    inputContext->inputPanel().reset();
    auto *state = inputContext->propertyFor(&factory_);
    std::vector<std::string> results;
    if (spell()) {
        results = spell()->call<ISpell::hint>(entry.languageCode(),
                                              state->buffer_.userInput(),
                                              config_.pageSize.value());
    }
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
    candidateList->setPageSize(*config_.pageSize);
    candidateList->setSelectionKey(selectionKeys_);
    candidateList->setCursorIncludeUnselected(true);
    state->mode_ = CandidateMode::Hint;
    inputContext->inputPanel().setCandidateList(std::move(candidateList));

    updateUI(inputContext);
}

void KeyboardEngine::resetState(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
    state->reset();
    instance_->resetCompose(inputContext);
}

void KeyboardEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto *inputContext = event.inputContext();
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

void KeyboardEngine::invokeActionImpl(const InputMethodEntry &,
                                      InvokeActionEvent &event) {
    auto *inputContext = event.inputContext();
    commitBuffer(inputContext);
}

bool KeyboardEngine::foreachLayout(
    const std::function<
        bool(const std::string &variant, const std::string &description,
             const std::vector<std::string> &languages)> &callback) {
    for (const auto &p : xkbRules_.layoutInfos()) {
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
    const auto *info = xkbRules_.findByName(layout);
    for (const auto &p : info->variantInfos) {
        if (!callback(p.name, p.description, p.languages)) {
            return false;
        }
    }
    return true;
}

void KeyboardEngine::initQuickPhrase() {
    auto *qp = quickphrase();
    if (!qp) {
        return;
    }
    quickphraseHandler_ = qp->call<IQuickPhrase::addProvider>(
        [this](InputContext *ic, const std::string &input,
               const QuickPhraseAddCandidateCallback &callback) {
            const auto *im = instance_->inputMethodEntry(ic);
            if (!im || !im->isKeyboard() || !*config_.enableQuickphraseEmoji ||
                !emoji()) {
                return true;
            }
            std::set<std::string> result;
            emoji()->call<IEmoji::prefix>(
                im->languageCode(), input, true,
                [&result, inputSize = input.size()](
                    const std::string &key,
                    const std::vector<std::string> &values) {
                    if (inputSize + 1 < key.size()) {
                        return true;
                    }
                    result.insert(values.begin(), values.end());
                    return true;
                });
            for (const auto &str : result) {
                callback(str, str, QuickPhraseAction::Commit);
            }
            return true;
        });
}

void KeyboardEngine::showHintNotification(const InputMethodEntry &entry,
                                          KeyboardEngineState *state) {
    if (!notifications()) {
        return;
    }
    bool hasSpell =
        spell() && spell()->call<ISpell::checkDict>(entry.languageCode());
    std::string extra;
    if (!hasSpell) {
        extra += " ";
        extra += _("Only emoji support is found. To enable spell "
                   "checking, you may need to install spell check data "
                   "for the language.");
    }
    std::string message;
    if (!state->hintEnabled()) {
        message = _("Completion is disabled.");
    } else if (state->oneTimeEnableWordHint_) {
        message =
            stringutils::concat(_("Completion is enabled temporarily."), extra);
    } else {
        message = stringutils::concat(_("Completion is enabled."), extra);
    }
    notifications()->call<INotifications::showTip>(
        "fcitx-keyboard-hint", _("Input Method"), "tools-check-spelling",
        _("Completion"), message, -1);
}

const Configuration *
KeyboardEngine::getSubConfig(const std::string &path) const {
    if (path == "longpress") {
        return &longPressConfig_;
    }
    return nullptr;
}

void KeyboardEngine::setSubConfig(const std::string &path,
                                  const RawConfig &config) {

    if (path == "longpress") {
        longPressConfig_.load(config, true);
        safeSaveAsIni(longPressConfig_, "conf/keyboard-longpress.conf");
        reloadConfig();
    }
}

} // namespace fcitx
