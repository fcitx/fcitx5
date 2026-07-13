/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphrasetempmode.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/textformatflags.h"
#include "fcitx/candidatelist.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/text.h"
#include "fcitx/userinterface.h"
#include "quickphrase.h"
#include "quickphrase_public.h"
#include "quickphraseprovider.h"

namespace fcitx {

namespace {

class QuickPhraseCandidateWord : public CandidateWord {
public:
    QuickPhraseCandidateWord(QuickPhraseTempMode *tempMode, std::string commit,
                             const std::string &display,
                             const std::string &comment,
                             QuickPhraseAction action)
        : CandidateWord(Text(display)), tempMode_(tempMode),
          commit_(std::move(commit)), action_(action) {
        setComment(Text(comment));
    }

    void select(InputContext *inputContext) const override {
        auto *state = tempMode_->property(inputContext);
        if (action_ == QuickPhraseAction::TypeToBuffer) {
            state->buffer_.type(commit_);
            state->typed_ = true;
            tempMode_->updateUI(inputContext);
        } else if (action_ == QuickPhraseAction::Commit) {
            inputContext->commitString(commit_);
            tempMode_->reset(inputContext);
        }
    }

private:
    QuickPhraseTempMode *tempMode_;
    std::string commit_;
    QuickPhraseAction action_;
};

} // namespace

QuickPhraseState::QuickPhraseState() { buffer_.setMaxSize(30); }

void QuickPhraseState::reset(InputContext *inputContext) {
    typed_ = false;
    text_.clear();
    buffer_.clear();
    buffer_.shrinkToFit();
    prefix_.clear();
    str_.clear();
    alt_.clear();
    originalBuffer_.clear();
    restoreCallback_ = nullptr;
    key_ = Key(FcitxKey_None);
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

QuickPhraseTempMode::QuickPhraseTempMode(QuickPhrase *quickPhrase)
    : quickPhrase_(quickPhrase) {}

bool QuickPhraseTempMode::triggerTempMode(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease() ||
        !keyEvent.key().checkKeyList(
            quickPhrase_->config().triggerKey.value())) {
        return false;
    }
    trigger(keyEvent.inputContext(), "", "", "", "", Key(FcitxKey_None));
    return true;
}

bool QuickPhraseTempMode::keyEvent(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return false;
    }

    auto *inputContext = keyEvent.inputContext();
    auto *state = property(inputContext);
    auto candidateList = inputContext->inputPanel().candidateList();
    if (candidateList) {
        int idx = -1;
        if (!selectionKeys_.empty() && selectionKeys_[0].sym() == FcitxKey_1) {
            idx = keyEvent.key().digitSelection(selectionModifier_);
        } else {
            idx = keyEvent.key().keyListIndex(selectionKeys_);
        }
        if (idx >= 0 && idx < candidateList->size()) {
            candidateList->candidate(idx).select(inputContext);
            return true;
        }

        if (keyEvent.key().check(FcitxKey_space) && !candidateList->empty()) {
            if (candidateList->cursorIndex() >= 0) {
                candidateList->candidate(candidateList->cursorIndex())
                    .select(inputContext);
            }
            return true;
        }

        if (keyEvent.key().checkKeyList(
                quickPhrase_->instance()->globalConfig().defaultPrevPage())) {
            auto *pageable = candidateList->toPageable();
            if (!pageable->hasPrev()) {
                if (pageable->usedNextBefore()) {
                    return true;
                }
            } else {
                pageable->prev();
                inputContext->updateUserInterface(
                    UserInterfaceComponent::InputPanel);
                return true;
            }
        }

        if (keyEvent.key().checkKeyList(
                quickPhrase_->instance()->globalConfig().defaultNextPage())) {
            auto *pageable = candidateList->toPageable();
            if (!pageable->hasNext()) {
                if (pageable->usedNextBefore()) {
                    return true;
                }
            } else {
                pageable->next();
                inputContext->updateUserInterface(
                    UserInterfaceComponent::InputPanel);
                return true;
            }
        }

        if (!candidateList->empty() &&
            keyEvent.key().checkKeyList(quickPhrase_->instance()
                                            ->globalConfig()
                                            .defaultPrevCandidate())) {
            candidateList->toCursorMovable()->prevCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (!candidateList->empty() &&
            keyEvent.key().checkKeyList(quickPhrase_->instance()
                                            ->globalConfig()
                                            .defaultNextCandidate())) {
            candidateList->toCursorMovable()->nextCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }
    }

    if (keyEvent.key().isModifier() || keyEvent.key().hasModifier()) {
        return false;
    }
    if (keyEvent.key().check(FcitxKey_Escape)) {
        reset(inputContext);
        return true;
    }
    if (keyEvent.key().check(FcitxKey_Return) ||
        keyEvent.key().check(FcitxKey_KP_Enter)) {
        if (!state->typed_ && state->buffer_.empty() && !state->str_.empty() &&
            !state->alt_.empty()) {
            inputContext->commitString(state->alt_);
        } else if (state->buffer_.size() + state->prefix_.size()) {
            inputContext->commitString(state->prefix_ +
                                       state->buffer_.userInput());
        }
        reset(inputContext);
        return true;
    }
    if (keyEvent.key().check(FcitxKey_BackSpace)) {
        if (state->buffer_.empty()) {
            reset(inputContext);
        } else if (state->buffer_.backspace()) {
            if (state->restoreCallback_ &&
                state->buffer_.cursor() == state->buffer_.size() &&
                state->buffer_.userInput() == state->originalBuffer_) {
                auto callback = std::move(state->restoreCallback_);
                auto original = std::move(state->originalBuffer_);
                reset(inputContext);
                callback(inputContext, original);
                return true;
            }
            if (state->buffer_.empty()) {
                reset(inputContext);
            } else {
                updateUI(inputContext);
            }
        }
        return true;
    }
    if (keyEvent.key().check(FcitxKey_Delete)) {
        if (state->buffer_.empty()) {
            reset(inputContext);
        } else if (state->buffer_.del()) {
            if (state->buffer_.empty()) {
                reset(inputContext);
            } else {
                updateUI(inputContext);
            }
        }
        return true;
    }
    if (!state->buffer_.empty()) {
        const Key &key = keyEvent.key();
        if (key.check(FcitxKey_Home) || key.check(FcitxKey_KP_Home)) {
            state->buffer_.setCursor(0);
            updateUI(inputContext);
            return true;
        }
        if (key.check(FcitxKey_End) || key.check(FcitxKey_KP_End)) {
            state->buffer_.setCursor(state->buffer_.size());
            updateUI(inputContext);
            return true;
        }
        if (key.check(FcitxKey_Left) || key.check(FcitxKey_KP_Left)) {
            auto cursor = state->buffer_.cursor();
            if (cursor > 0) {
                state->buffer_.setCursor(cursor - 1);
            }
            updateUI(inputContext);
            return true;
        }
        if (key.check(FcitxKey_Right) || key.check(FcitxKey_KP_Right)) {
            auto cursor = state->buffer_.cursor();
            if (cursor < state->buffer_.size()) {
                state->buffer_.setCursor(cursor + 1);
            }
            updateUI(inputContext);
            return true;
        }
    }
    if (!state->typed_ && !state->str_.empty() && state->buffer_.empty() &&
        keyEvent.key().check(state->key_)) {
        inputContext->commitString(state->str_);
        reset(inputContext);
        return true;
    }

    auto compose = quickPhrase_->instance()->processComposeString(
        inputContext, keyEvent.key().sym());
    if (!compose) {
        return true;
    }
    if (!compose->empty()) {
        state->buffer_.type(*compose);
    } else {
        state->buffer_.type(Key::keySymToUnicode(keyEvent.key().sym()));
    }
    state->typed_ = true;
    updateUI(inputContext);
    return true;
}

void QuickPhraseTempMode::reset(InputContext *inputContext) {
    if (auto *state = property(inputContext)) {
        Base::reset(inputContext);
        state->reset(inputContext);
    }
}

std::string_view QuickPhraseTempMode::name() const {
    return "quickphraseState";
}

void QuickPhraseTempMode::trigger(InputContext *inputContext,
                                  const std::string &text,
                                  const std::string &prefix,
                                  const std::string &str,
                                  const std::string &alt, const Key &key) {
    auto *state = property(inputContext);
    state->typed_ = false;
    state->setActive(true);
    state->text_ = text;
    state->prefix_ = prefix;
    state->str_ = str;
    state->alt_ = alt;
    state->key_ = key;
    state->buffer_.clear();
    updateUI(inputContext);
}

void QuickPhraseTempMode::setBuffer(InputContext *inputContext,
                                    const std::string &text) {
    auto *state = property(inputContext);
    if (!state->isActive()) {
        return;
    }
    state->buffer_.clear();
    state->buffer_.type(text);
    updateUI(inputContext);
}

void QuickPhraseTempMode::setBufferWithRestoreCallback(
    InputContext *inputContext, const std::string &text,
    const std::string &original, QuickPhraseRestoreCallback callback) {
    auto *state = property(inputContext);
    if (!state->isActive()) {
        return;
    }
    state->buffer_.clear();
    state->buffer_.type(text);
    state->originalBuffer_ = original;
    state->restoreCallback_ = std::move(callback);
    updateUI(inputContext);
}

bool QuickPhraseTempMode::invokeAction(InvokeActionEvent &event) {
    auto *inputContext = event.inputContext();
    auto *state = property(inputContext);
    int cursor = event.cursor() - static_cast<int>(state->prefix_.size());
    if (cursor < 0 || event.action() != InvokeActionEvent::Action::LeftClick ||
        !inputContext->capabilityFlags().test(CapabilityFlag::Preedit)) {
        reset(inputContext);
        return true;
    }
    state->buffer_.setCursor(cursor);
    updateUI(inputContext);
    return true;
}

void QuickPhraseTempMode::updateUI(InputContext *inputContext) {
    auto *state = property(inputContext);
    inputContext->inputPanel().reset();
    if (!state->buffer_.empty()) {
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setCursorPositionAfterPaging(
            CursorPositionAfterPaging::ResetToFirst);
        candidateList->setPageSize(
            quickPhrase_->instance()->globalConfig().defaultPageSize());
        QuickPhraseProvider *providers[] = {&quickPhrase_->callbackProvider(),
                                            &quickPhrase_->builtinProvider(),
                                            &quickPhrase_->spellProvider()};
        QuickPhraseAction selectionKeyAction =
            QuickPhraseAction::DigitSelection;
        std::string autoCommit;
        bool autoCommitSet = false;
        for (auto *provider : providers) {
            if (!provider->populate(
                    inputContext, state->buffer_.userInput(),
                    [this, &candidateList, &selectionKeyAction, &autoCommit,
                     &autoCommitSet](
                        const std::string &word, const std::string &aux,
                        const std::string &comment, QuickPhraseAction action) {
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
                                this, word, aux, comment, action);
                        } else if (action ==
                                       QuickPhraseAction::DigitSelection ||
                                   action ==
                                       QuickPhraseAction::AlphaSelection ||
                                   action == QuickPhraseAction::NoneSelection) {
                            selectionKeyAction = action;
                        }
                    })) {
                break;
            }
        }
        if (autoCommitSet) {
            if (!autoCommit.empty()) {
                inputContext->commitString(autoCommit);
            }
            reset(inputContext);
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

void QuickPhraseTempMode::setSelectionKeys(QuickPhraseAction action) {
    std::array<KeySym, 10> syms;
    switch (action) {
    case QuickPhraseAction::AlphaSelection:
        syms = {FcitxKey_a, FcitxKey_b, FcitxKey_c, FcitxKey_e, FcitxKey_f,
                FcitxKey_g, FcitxKey_h, FcitxKey_i, FcitxKey_j, FcitxKey_k};
        break;
    case QuickPhraseAction::NoneSelection:
        syms.fill(FcitxKey_None);
        break;
    default:
        syms = {FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
                FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0};
        break;
    }
    selectionKeys_.clear();
    selectionModifier_ = KeyState::NoState;
    switch (quickPhrase_->config().chooseModifier.value()) {
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

} // namespace fcitx
