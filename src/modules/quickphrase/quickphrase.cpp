/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphrase.h"

#include <utility>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/inputbuffer.h"
#include "fcitx/addonfactory.h"
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
                int idx = -1;
                if (!selectionKeys_.empty() &&
                    selectionKeys_[0].sym() == FcitxKey_1) {
                    idx = keyEvent.key().digitSelection(selectionModifier_);
                } else {
                    idx = keyEvent.key().keyListIndex(selectionKeys_);
                }
                if (idx >= 0) {
                    if (idx < candidateList->size()) {
                        keyEvent.accept();
                        candidateList->candidate(idx).select(inputContext);
                        return;
                    }
                }

                if (keyEvent.key().check(FcitxKey_space) &&
                    !candidateList->empty()) {
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
                    auto *pageable = candidateList->toPageable();
                    if (!pageable->hasNext()) {
                        if (pageable->usedNextBefore()) {
                            event.accept();
                            return;
                        }
                    } else {
                        event.accept();
                        pageable->next();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                        return;
                    }
                }

                if (!candidateList->empty() &&
                    keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->prevCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (!candidateList->empty() &&
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
            if (keyEvent.key().check(FcitxKey_Return) ||
                keyEvent.key().check(FcitxKey_KP_Enter)) {
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
            if (keyEvent.key().check(FcitxKey_Delete)) {
                if (state->buffer_.empty()) {
                    state->reset(inputContext);
                } else {
                    if (state->buffer_.del()) {
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
            if (!state->buffer_.empty()) {
                const Key &key = keyEvent.key();
                if (key.check(FcitxKey_Home) || key.check(FcitxKey_KP_Home)) {
                    state->buffer_.setCursor(0);
                    keyEvent.accept();
                    return updateUI(inputContext);
                }
                if (key.check(FcitxKey_End) || key.check(FcitxKey_KP_End)) {
                    state->buffer_.setCursor(state->buffer_.size());
                    keyEvent.accept();
                    return updateUI(inputContext);
                }
                if (key.check(FcitxKey_Left) || key.check(FcitxKey_KP_Left)) {
                    auto cursor = state->buffer_.cursor();
                    if (cursor > 0) {
                        state->buffer_.setCursor(cursor - 1);
                    }
                    keyEvent.accept();
                    return updateUI(inputContext);
                }
                if (key.check(FcitxKey_Right) || key.check(FcitxKey_KP_Right)) {
                    auto cursor = state->buffer_.cursor();
                    if (cursor < state->buffer_.size()) {
                        state->buffer_.setCursor(cursor + 1);
                    }
                    keyEvent.accept();
                    return updateUI(inputContext);
                }
            }
            if (!state->typed_ && !state->str_.empty() &&
                state->buffer_.empty() && keyEvent.key().check(state->key_)) {
                keyEvent.accept();
                inputContext->commitString(state->str_);
                state->reset(inputContext);
                return;
            }

            // check compose first.
            auto compose = instance_->processComposeString(
                inputContext, keyEvent.key().sym());

            // compose is invalid, ignore it.
            if (!compose) {
                return event.accept();
            }

            if (!compose->empty()) {
                state->buffer_.type(*compose);
            } else {
                state->buffer_.type(Key::keySymToUnicode(keyEvent.key().sym()));
            }
            state->typed_ = true;
            event.accept();

            updateUI(inputContext);
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextInvokeAction, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            auto &invokeActionEvent = static_cast<InvokeActionEvent &>(event);
            auto *inputContext = invokeActionEvent.inputContext();
            auto *state = inputContext->propertyFor(&factory_);
            if (!state->enabled_) {
                return;
            }
            invokeActionEvent.filter();
            int cursor = invokeActionEvent.cursor() -
                         static_cast<int>(state->prefix_.size());
            if (cursor < 0 ||
                invokeActionEvent.action() !=
                    InvokeActionEvent::Action::LeftClick ||
                !inputContext->capabilityFlags().test(
                    CapabilityFlag::Preedit)) {
                state->reset(inputContext);
                return;
            }
            state->buffer_.setCursor(cursor);
            invokeActionEvent.accept();
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
            inputContext->commitString(commit_);
            state->reset(inputContext);
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
    selectionModifier_ = KeyState::NoState;
    switch (config_.chooseModifier.value()) {
    case QuickPhraseChooseModifier::Alt:
        selectionModifier_ = KeyState::Alt;
        break;
    case QuickPhraseChooseModifier::Control:
        selectionModifier_ = KeyState::Ctrl;
        break;
    case QuickPhraseChooseModifier::Super:
        selectionModifier_ = KeyState::Super;
        break;
    default:
        break;
    }

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, selectionModifier_);
    }
}

void QuickPhrase::updateUI(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
    inputContext->inputPanel().reset();
    if (!state->buffer_.empty()) {
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setCursorPositionAfterPaging(
            CursorPositionAfterPaging::ResetToFirst);
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
        if (!candidateList->empty()) {
            candidateList->setGlobalCursorIndex(0);
        }
        inputContext->inputPanel().setCandidateList(std::move(candidateList));
    }
    Text preedit;
    const bool useClientPreedit =
        inputContext->capabilityFlags().test(CapabilityFlag::Preedit);
    TextFormatFlags format{useClientPreedit ? TextFormatFlag::Underline
                                            : TextFormatFlag::NoFlag};
    if (!state->prefix_.empty()) {
        preedit.append(state->prefix_, format);
    }
    if (!state->buffer_.empty()) {
        preedit.append(state->buffer_.userInput(), format);
    }
    preedit.setCursor(state->prefix_.size() + state->buffer_.cursorByChar());

    Text auxUp(_("Quick Phrase: "));
    if (!state->typed_) {
        auxUp.append(state->text_);
    }
    inputContext->inputPanel().setAuxUp(auxUp);
    if (useClientPreedit) {
        inputContext->inputPanel().setClientPreedit(preedit);
    } else {
        inputContext->inputPanel().setPreedit(preedit);
    }
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
