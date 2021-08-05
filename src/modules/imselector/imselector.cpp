/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "imselector.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"

namespace fcitx {

namespace {

void selectInputMethod(InputContext *ic, IMSelector *imSelector,
                       const std::string &uniqueName, bool local) {
    auto instance = imSelector->instance();
    auto *state = ic->propertyFor(&imSelector->factory());
    instance->setCurrentInputMethod(ic, uniqueName, local);
    state->reset(ic);
    instance->showInputMethodInformation(ic);
}

bool selectInputMethod(InputContext *ic, IMSelector *imSelector, size_t index,
                       bool local) {
    auto &inputMethodManager = imSelector->instance()->inputMethodManager();
    auto &list = inputMethodManager.currentGroup().inputMethodList();
    if (index >= list.size()) {
        return false;
    }
    selectInputMethod(
        ic, imSelector,
        inputMethodManager.entry(list[index].name())->uniqueName(), local);
    return true;
}

} // namespace

class IMSelectorCandidateWord : public CandidateWord {
public:
    IMSelectorCandidateWord(IMSelector *q, const InputMethodEntry *entry,
                            bool local)
        : CandidateWord(Text(entry->name())), q_(q),
          uniqueName_(entry->uniqueName()), local_(local) {}

    void select(InputContext *ic) const override {
        selectInputMethod(ic, q_, uniqueName_, local_);
    }

    IMSelector *q_;
    std::string uniqueName_;
    bool local_;
};

IMSelector::IMSelector(Instance *instance)
    : instance_(instance),
      factory_([](InputContext &) { return new IMSelectorState; }) {
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            if (keyEvent.key().checkKeyList(config_.triggerKey.value()) &&
                trigger(keyEvent.inputContext(), false)) {
                keyEvent.filterAndAccept();
                return;
            }
            if (keyEvent.key().checkKeyList(config_.triggerKeyLocal.value()) &&
                trigger(keyEvent.inputContext(), true)) {
                keyEvent.filterAndAccept();
                return;
            }
        }));

    // Select input method via hotkey.
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *inputContext = keyEvent.inputContext();
            if (int index =
                    keyEvent.key().keyListIndex(config_.switchKey.value());
                index >= 0 &&
                selectInputMethod(inputContext, this, index, /*local=*/false)) {
                keyEvent.filterAndAccept();
                return;
            }
            if (int index =
                    keyEvent.key().keyListIndex(config_.switchKeyLocal.value());
                index >= 0 &&
                selectInputMethod(inputContext, this, index, /*local=*/true)) {
                keyEvent.filterAndAccept();
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
            keyEvent.filterAndAccept();
            if (keyEvent.isRelease()) {
                return;
            }

            auto candidateList = inputContext->inputPanel().candidateList();
            if (candidateList && !candidateList->empty()) {
                int idx = keyEvent.key().keyListIndex(selectionKeys_);
                if (idx >= 0) {
                    if (idx < candidateList->size()) {
                        keyEvent.accept();
                        candidateList->candidate(idx).select(inputContext);
                        return;
                    }
                }

                if (keyEvent.key().check(FcitxKey_space) ||
                    keyEvent.key().check(FcitxKey_Return) ||
                    keyEvent.key().check(FcitxKey_KP_Enter)) {
                    keyEvent.accept();
                    if (candidateList->cursorIndex() >= 0) {
                        candidateList->candidate(candidateList->cursorIndex())
                            .select(inputContext);
                    }
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevPage())) {
                    keyEvent.filterAndAccept();
                    candidateList->toPageable()->prev();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
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
            if (keyEvent.key().check(FcitxKey_Escape) ||
                keyEvent.key().check(FcitxKey_BackSpace) ||
                keyEvent.key().check(FcitxKey_Delete)) {
                keyEvent.accept();
                state->reset(inputContext);
                return;
            }
        }));

    instance_->inputContextManager().registerProperty("imselector", &factory_);

    std::array<KeySym, 10> syms = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym);
    }

    reloadConfig();
}

void IMSelector::reloadConfig() { readAsIni(config_, "conf/imselector.conf"); }

bool IMSelector::trigger(InputContext *inputContext, bool local) {
    const auto &list =
        instance_->inputMethodManager().currentGroup().inputMethodList();
    if (list.empty()) {
        return false;
    }
    auto *state = inputContext->propertyFor(&factory_);
    state->enabled_ = true;
    inputContext->inputPanel().reset();
    auto currentIM = instance_->inputMethod(inputContext);
    auto candidateList = std::make_unique<CommonCandidateList>();
    candidateList->setPageSize(10);
    int idx = -1;
    for (const auto &item : list) {
        if (const auto *entry =
                instance_->inputMethodManager().entry(item.name())) {
            if (entry->uniqueName() == currentIM) {
                idx = candidateList->totalSize();
            }
            candidateList->append<IMSelectorCandidateWord>(this, entry, local);
        }
    }
    candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
    candidateList->setSelectionKey(selectionKeys_);
    candidateList->setCursorPositionAfterPaging(
        CursorPositionAfterPaging::ResetToFirst);
    if (candidateList->size()) {
        if (idx < 0) {
            candidateList->setGlobalCursorIndex(0);
        } else {
            candidateList->setGlobalCursorIndex(idx);
            candidateList->setPage(idx / candidateList->pageSize());
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

class IMSelectorFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new IMSelector(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::IMSelectorFactory);
