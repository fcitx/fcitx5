/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "imselectortempmode.h"
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx/candidatelist.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/text.h"
#include "fcitx/userinterface.h"
#include "imselector.h"

namespace fcitx {

namespace {

constexpr KeyList emptyKeyList;

void selectInputMethod(InputContext *inputContext, IMSelector *imSelector,
                       const std::string &uniqueName, bool local) {
    auto *instance = imSelector->instance();
    instance->setCurrentInputMethod(inputContext, uniqueName, local);
    imSelector->reset(inputContext);
    instance->showInputMethodInformation(inputContext);
}

class IMSelectorCandidateWord : public CandidateWord {
public:
    IMSelectorCandidateWord(IMSelector *imSelector,
                            const InputMethodEntry *entry, bool local)
        : CandidateWord(Text(entry->name())), imSelector_(imSelector),
          uniqueName_(entry->uniqueName()), local_(local) {}

    void select(InputContext *inputContext) const override {
        selectInputMethod(inputContext, imSelector_, uniqueName_, local_);
    }

private:
    IMSelector *imSelector_;
    std::string uniqueName_;
    bool local_;
};

} // namespace

void IMSelectorState::reset(InputContext *inputContext) {
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

IMSelectorTempMode::IMSelectorTempMode(IMSelector *imSelector)
    : imSelector_(imSelector) {}

bool IMSelectorTempMode::triggerTempMode(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return false;
    }

    bool local;
    if (keyEvent.key().checkKeyList(imSelector_->config().triggerKey.value())) {
        local = false;
    } else if (keyEvent.key().checkKeyList(
                   imSelector_->config().triggerKeyLocal.value())) {
        local = true;
    } else {
        return false;
    }

    if (!trigger(keyEvent.inputContext(), local)) {
        return false;
    }
    property(keyEvent.inputContext())->setActive(true);
    return true;
}

bool IMSelectorTempMode::keyEvent(const KeyEvent &keyEvent) {
    auto *inputContext = keyEvent.inputContext();
    if (keyEvent.isRelease()) {
        return false;
    }

    auto candidateList = inputContext->inputPanel().candidateList();
    if (candidateList && !candidateList->empty()) {
        int idx = keyEvent.key().digitSelection();
        if (idx >= 0 && idx < candidateList->size()) {
            candidateList->candidate(idx).select(inputContext);
            return true;
        }

        if (keyEvent.key().check(FcitxKey_space) ||
            keyEvent.key().check(FcitxKey_Return) ||
            keyEvent.key().check(FcitxKey_KP_Enter)) {
            if (candidateList->cursorIndex() >= 0) {
                candidateList->candidate(candidateList->cursorIndex())
                    .select(inputContext);
            }
            return true;
        }

        if (keyEvent.key().checkKeyList(
                imSelector_->instance()->globalConfig().defaultPrevPage())) {
            candidateList->toPageable()->prev();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(
                imSelector_->instance()->globalConfig().defaultNextPage())) {
            candidateList->toPageable()->next();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(imSelector_->instance()
                                            ->globalConfig()
                                            .defaultPrevCandidate())) {
            candidateList->toCursorMovable()->prevCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(imSelector_->instance()
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
    if (keyEvent.key().check(FcitxKey_Escape) ||
        keyEvent.key().check(FcitxKey_BackSpace) ||
        keyEvent.key().check(FcitxKey_Delete)) {
        reset(inputContext);
    }
    return true;
}

bool IMSelectorTempMode::trigger(InputContext *inputContext, bool local) {
    auto *instance = imSelector_->instance();
    const auto &list =
        instance->inputMethodManager().currentGroup().inputMethodList();
    if (list.empty()) {
        return false;
    }

    inputContext->inputPanel().reset();
    auto currentIM = instance->inputMethod(inputContext);
    auto candidateList = std::make_unique<CommonCandidateList>();
    candidateList->setPageSize(10);
    int index = -1;
    for (const auto &item : list) {
        if (const auto *entry =
                instance->inputMethodManager().entry(item.name())) {
            if (entry->uniqueName() == currentIM) {
                index = candidateList->totalSize();
            }
            candidateList->append<IMSelectorCandidateWord>(imSelector_, entry,
                                                           local);
        }
    }
    candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
    candidateList->setSelectionKey(imSelector_->selectionKeys());
    candidateList->setCursorPositionAfterPaging(
        CursorPositionAfterPaging::ResetToFirst);
    if (candidateList->size()) {
        if (index < 0) {
            candidateList->setGlobalCursorIndex(0);
        } else {
            candidateList->setGlobalCursorIndex(index);
            candidateList->setPage(index / candidateList->pageSize());
        }
        inputContext->inputPanel().setAuxUp(
            Text(local ? _("Select local input method:")
                       : _("Select input method:")));
    }
    inputContext->inputPanel().setCandidateList(std::move(candidateList));
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
    return true;
}

void IMSelectorTempMode::reset(InputContext *inputContext) {
    if (auto *imSelectorState = property(inputContext)) {
        Base::reset(inputContext);
        imSelectorState->reset(inputContext);
    }
}

std::string_view IMSelectorTempMode::name() const { return "imselector"; }

const KeyList &IMSelectorTempMode::triggerKeys() const { return emptyKeyList; }

} // namespace fcitx
