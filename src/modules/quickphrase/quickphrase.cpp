/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphrase.h"
#include <fcntl.h>

#include <utility>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonmanager.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"

namespace fcitx {

class QuickPhraseState : public InputContextProperty {
public:
    QuickPhraseState(QuickPhrase *q) : q_(q) { buffer_.setMaxSize(30); }

    bool enabled_ = false;
    InputBuffer buffer_;
    QuickPhrase *q_;

    bool typed_ = false;
    std::string text_;
    std::string prefix_;
    std::string str_;
    std::string alt_;
    Key key_;

    void reset(InputContext *ic) {
        enabled_ = false;
        typed_ = false;
        text_.clear();
        buffer_.clear();
        buffer_.shrinkToFit();
        prefix_.clear();
        str_.clear();
        alt_.clear();
        key_ = Key(FcitxKey_None);
        ic->inputPanel().reset();
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
};

QuickPhrase::QuickPhrase(Instance *instance)
    : instance_(instance), spellProvider_(this),
      factory_([this](InputContext &) { return new QuickPhraseState(this); }) {
    instance_->inputContextManager().registerProperty("quickphraseState",
                                                      &factory_);
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            if (keyEvent.key().checkKeyList(config_.triggerKey.value())) {
                trigger(keyEvent.inputContext(), "", "", "", "",
                        Key{FcitxKey_None});
                keyEvent.filterAndAccept();
                updateUI(keyEvent.inputContext());
                return;
            }
        }));

    auto reset = [this](Event &event) {
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto *state = icEvent.inputContext()->propertyFor(&factory_);
        if (state->enabled_) {
            state->reset(icEvent.inputContext());
        }
    };
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusOut, EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextReset, EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(
        instance_->watchEvent(EventType::InputContextSwitchInputMethod,
                              EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *inputContext = keyEvent.inputContext();
            auto *state = inputContext->propertyFor(&factory_);
            if (!state->enabled_) {
                return;
            }

            // make sure no one else will handle it
            keyEvent.filter();
            if (keyEvent.isRelease()) {
                return;
            }

            auto candidateList = inputContext->inputPanel().candidateList();
            if (candidateList) {
                int idx = keyEvent.key().keyListIndex(selectionKeys_);
                if (idx >= 0) {
                    if (idx < candidateList->size()) {
                        keyEvent.accept();
                        candidateList->candidate(idx).select(inputContext);
                        return;
                    }
                }

                if (keyEvent.key().check(FcitxKey_space) &&
                    candidateList->size()) {
                    keyEvent.accept();
                    if (candidateList->cursorIndex() >= 0) {
                        candidateList->candidate(candidateList->cursorIndex())
                            .select(inputContext);
                    }
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevPage())) {
                    auto *pageable = candidateList->toPageable();
                    if (!pageable->hasPrev()) {
                        if (pageable->usedNextBefore()) {
                            event.accept();
                            return;
                        }
                    } else {
                        event.accept();
                        pageable->prev();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                        return;
                    }
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultNextPage())) {
                    keyEvent.filterAndAccept();
                    candidateList->toPageable()->next();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (candidateList->size() &&
                    keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->prevCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (candidateList->size() &&
                    keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultNextCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->nextCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }
            }

            // and by pass all modifier
            if (keyEvent.key().isModifier() || keyEvent.key().hasModifier()) {
                return;
            }
            if (keyEvent.key().check(FcitxKey_Escape)) {
                keyEvent.accept();
                state->reset(inputContext);
                return;
            }
            if (keyEvent.key().check(FcitxKey_Return)) {
                keyEvent.accept();
                if (!state->typed_ && state->buffer_.empty() &&
                    !state->str_.empty() && !state->alt_.empty()) {
                    inputContext->commitString(state->alt_);
                } else {
                    if (state->buffer_.size() + state->prefix_.size()) {
                        inputContext->commitString(state->prefix_ +
                                                   state->buffer_.userInput());
                    }
                }
                state->reset(inputContext);
                return;
            }
            if (keyEvent.key().check(FcitxKey_BackSpace)) {
                if (state->buffer_.empty()) {
                    state->reset(inputContext);
                } else {
                    if (state->buffer_.backspace()) {
                        if (state->buffer_.empty()) {
                            state->reset(inputContext);
                        } else {
                            updateUI(inputContext);
                        }
                    }
                }
                keyEvent.accept();
                return;
            }
            if (!state->typed_ && !state->str_.empty() &&
                state->buffer_.empty() && keyEvent.key().check(state->key_)) {
                keyEvent.accept();
                inputContext->commitString(state->str_);
                state->reset(inputContext);
                return;
            }

            // check compose first.
            auto compose =
                instance_->processCompose(inputContext, keyEvent.key().sym());

            // compose is invalid, ignore it.
            if (compose == FCITX_INVALID_COMPOSE_RESULT) {
                return event.accept();
            }

            if (compose) {
                state->buffer_.type(compose);
            } else {
                state->buffer_.type(Key::keySymToUnicode(keyEvent.key().sym()));
            }
            state->typed_ = true;
            event.accept();

            updateUI(inputContext);
        }));

    reloadConfig();
}

QuickPhrase::~QuickPhrase() {}

class QuickPhraseCandidateWord : public CandidateWord {
public:
    QuickPhraseCandidateWord(QuickPhrase *q, std::string commit,
                             const std::string &display,
                             QuickPhraseAction action)
        : CandidateWord(Text(display)), q_(q), commit_(std::move(commit)),
          action_(action) {}

    void select(InputContext *inputContext) const override {
        auto *state = inputContext->propertyFor(&q_->factory());
        if (action_ == QuickPhraseAction::TypeToBuffer) {
            state->buffer_.type(commit_);
            state->typed_ = true;

            q_->updateUI(inputContext);
        } else if (action_ == QuickPhraseAction::Commit) {
            state->reset(inputContext);
            inputContext->inputPanel().reset();
            inputContext->updatePreedit();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel, true);
            inputContext->commitString(commit_);
        }
        // DoNothing and other values are also handled here.
    }

    QuickPhrase *q_;
    std::string commit_;
    QuickPhraseAction action_;
};

void QuickPhrase::setSelectionKeys(QuickPhraseAction action) {
    std::array<KeySym, 10> syms;
    switch (action) {
    case QuickPhraseAction::AlphaSelection:
        syms = {
            FcitxKey_a, FcitxKey_b, FcitxKey_c, FcitxKey_e, FcitxKey_f,
            FcitxKey_g, FcitxKey_h, FcitxKey_i, FcitxKey_j, FcitxKey_k,
        };
        break;
    case QuickPhraseAction::NoneSelection:
        syms = {
            FcitxKey_None, FcitxKey_None, FcitxKey_None, FcitxKey_None,
            FcitxKey_None, FcitxKey_None, FcitxKey_None, FcitxKey_None,
            FcitxKey_None, FcitxKey_None,
        };
        break;
    default:
        syms = {
            FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
            FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
        };
        break;
    }

    selectionKeys_.clear();

    KeyStates states;
    switch (config_.chooseModifier.value()) {
    case QuickPhraseChooseModifier::Alt:
        states = KeyState::Alt;
        break;
    case QuickPhraseChooseModifier::Control:
        states = KeyState::Ctrl;
        break;
    case QuickPhraseChooseModifier::Super:
        states = KeyState::Super;
        break;
    default:
        break;
    }

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, states);
    }
}

void QuickPhrase::updateUI(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
    inputContext->inputPanel().reset();
    if (!state->buffer_.empty()) {
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setPageSize(instance_->globalConfig().defaultPageSize());
        QuickPhraseProvider *providers[] = {&callbackProvider_,
                                            &builtinProvider_, &spellProvider_};
        QuickPhraseAction selectionKeyAction =
            QuickPhraseAction::DigitSelection;
        std::string autoCommit;
        bool autoCommitSet = false;
        for (auto *provider : providers) {
            if (!provider->populate(
                    inputContext, state->buffer_.userInput(),
                    [this, &candidateList, &selectionKeyAction, &autoCommit,
                     &autoCommitSet](const std::string &word,
                                     const std::string &aux,
                                     QuickPhraseAction action) {
                        if (!autoCommitSet &&
                            action == QuickPhraseAction::AutoCommit) {
                            autoCommit = word;
                            autoCommitSet = true;
                        }

                        if (autoCommitSet) {
                            return;
                        }
                        if (!word.empty()) {
                            candidateList->append<QuickPhraseCandidateWord>(
                                this, word, aux, action);
                        } else {
                            if (action == QuickPhraseAction::DigitSelection ||
                                action == QuickPhraseAction::AlphaSelection ||
                                action == QuickPhraseAction::NoneSelection) {
                                selectionKeyAction = action;
                            }
                        }
                    })) {
                break;
            }
        }

        if (autoCommitSet) {
            if (!autoCommit.empty()) {
                inputContext->commitString(autoCommit);
            }
            state->reset(inputContext);
            return;
        }

        setSelectionKeys(selectionKeyAction);
        candidateList->setSelectionKey(selectionKeys_);
        if (candidateList->size()) {
            candidateList->setGlobalCursorIndex(0);
        }
        inputContext->inputPanel().setCandidateList(std::move(candidateList));
    }
    Text preedit;
    if (!state->prefix_.empty()) {
        preedit.append(state->prefix_);
    }
    preedit.append(state->buffer_.userInput());
    if (!state->buffer_.empty()) {
        preedit.setCursor(state->prefix_.size() +
                          state->buffer_.cursorByChar());
    }

    Text auxUp(_("Quick Phrase: "));
    if (!state->typed_) {
        auxUp.append(state->text_);
    }
    // inputContext->inputPanel().setClientPreedit(preedit);
    inputContext->inputPanel().setAuxUp(auxUp);
    inputContext->inputPanel().setPreedit(preedit);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void QuickPhrase::reloadConfig() {
    builtinProvider_.reloadConfig();
    readAsIni(config_, "conf/quickphrase.conf");
}
std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
QuickPhrase::addProvider(QuickPhraseProviderCallback callback) {
    return callbackProvider_.addCallback(std::move(callback));
}

void QuickPhrase::trigger(InputContext *ic, const std::string &text,
                          const std::string &prefix, const std::string &str,
                          const std::string &alt, const Key &key) {
    auto *state = ic->propertyFor(&factory_);
    state->typed_ = false;
    state->enabled_ = true;
    state->text_ = text;
    state->prefix_ = prefix;
    state->str_ = str;
    state->alt_ = alt;
    state->key_ = key;
    state->buffer_.clear();
    updateUI(ic);
}

void QuickPhrase::setBuffer(InputContext *ic, const std::string &text) {
    auto *state = ic->propertyFor(&factory_);
    if (!state->enabled_) {
        return;
    }
    state->buffer_.clear();
    state->buffer_.type(text);
    updateUI(ic);
}

class QuickPhraseModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new QuickPhrase(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::QuickPhraseModuleFactory)
