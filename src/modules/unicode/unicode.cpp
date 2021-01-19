/*
 * SPDX-FileCopyrightText: 2012-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "unicode.h"
#include <fmt/format.h>
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"

namespace fcitx {

class UnicodeState : public InputContextProperty {
public:
    UnicodeState(Unicode *q) : q_(q) { buffer_.setMaxSize(30); }

    bool enabled_ = false;
    InputBuffer buffer_;
    Unicode *q_;

    void reset(InputContext *ic) {
        enabled_ = false;
        buffer_.clear();
        buffer_.shrinkToFit();
        ic->inputPanel().reset();
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
};

class UnicodeCandidateWord : public CandidateWord {
public:
    UnicodeCandidateWord(Unicode *q, int c) : q_(q) {
        Text text;
        text.append(utf8::UCS4ToUTF8(c));
        text.append(" ");
        text.append(q->data().name(c));
        setText(std::move(text));
    }

    void select(InputContext *inputContext) const override {
        auto commit = text().stringAt(0);
        auto *state = inputContext->propertyFor(&q_->factory());
        state->reset(inputContext);
        inputContext->commitString(commit);
    }

    Unicode *q_;
};

Unicode::Unicode(Instance *instance)
    : instance_(instance), toggleKey_(Key("Control+Alt+Shift+U")),
      factory_([this](InputContext &) { return new UnicodeState(this); }) {
    instance_->inputContextManager().registerProperty("unicodeState",
                                                      &factory_);

    KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    KeyStates states = KeyState::Alt;

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, states);
    }
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            if (keyEvent.key().check(toggleKey_) && data_.load()) {
                trigger(keyEvent.inputContext());
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
            keyEvent.filter();
            if (keyEvent.isRelease()) {
                return;
            }

            auto candidateList = inputContext->inputPanel().candidateList();
            if (candidateList) {
                int idx = keyEvent.key().keyListIndex(selectionKeys_);
                if (idx >= 0) {
                    keyEvent.accept();
                    if (idx < candidateList->size()) {
                        candidateList->candidate(idx).select(inputContext);
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

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->prevCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (keyEvent.key().checkKeyList(
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
                if (candidateList->size() > 0 &&
                    candidateList->cursorIndex() >= 0) {
                    candidateList->candidate(candidateList->cursorIndex())
                        .select(inputContext);
                }
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
            event.accept();

            updateUI(inputContext);
        }));
}

Unicode::~Unicode() {}

void Unicode::trigger(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
    state->enabled_ = true;
    updateUI(inputContext, true);
}
void Unicode::updateUI(InputContext *inputContext, bool trigger) {
    auto *state = inputContext->propertyFor(&factory_);
    inputContext->inputPanel().reset();
    if (!state->buffer_.empty()) {
        auto result = data_.find(state->buffer_.userInput());
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setPageSize(instance_->globalConfig().defaultPageSize());
        for (auto c : result) {
            if (utf8::UCS4IsValid(c)) {
                candidateList->append<UnicodeCandidateWord>(this, c);
            }
        }
        if (candidateList->size()) {
            candidateList->setGlobalCursorIndex(0);
        }
        candidateList->setSelectionKey(selectionKeys_);
        candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
        inputContext->inputPanel().setCandidateList(std::move(candidateList));
    } else if (trigger) {
        auto candidateList = std::make_unique<CommonCandidateList>();

        std::vector<std::string> selectedTexts;
        if (inputContext->capabilityFlags().test(
                CapabilityFlag::SurroundingText)) {
            if (auto selected = inputContext->surroundingText().selectedText();
                !selected.empty()) {
                selectedTexts.push_back(std::move(selected));
            }
        }
        if (clipboard()) {
            if (selectedTexts.empty()) {
                auto primary =
                    clipboard()->call<IClipboard::primary>(inputContext);
                if (std::find(selectedTexts.begin(), selectedTexts.end(),
                              primary) == selectedTexts.end()) {
                    selectedTexts.push_back(std::move(primary));
                }
            }
            auto clip = clipboard()->call<IClipboard::clipboard>(inputContext);
            if (std::find(selectedTexts.begin(), selectedTexts.end(), clip) ==
                selectedTexts.end()) {
                selectedTexts.push_back(std::move(clip));
            }
        }
        std::unordered_set<uint32_t> seenChar;
        for (const auto &str : selectedTexts) {
            if (!utf8::validate(str)) {
                continue;
            }
            // Hard limit to prevent do too much lookup.
            constexpr int limit = 20;
            int counter = 0;
            for (auto c : utf8::MakeUTF8CharRange(str)) {
                if (!seenChar.insert(c).second) {
                    continue;
                }
                auto name = data_.name(c);
                std::string display;
                if (!name.empty()) {
                    display = fmt::format("{0} U+{1:04x} {2}",
                                          utf8::UCS4ToUTF8(c), c, name);
                } else {
                    display =
                        fmt::format("{0} U+{1:04x}", utf8::UCS4ToUTF8(c), c);
                }
                candidateList->append<DisplayOnlyCandidateWord>(Text(display));
                if (counter >= limit) {
                    break;
                }
                counter += 1;
            }
        }
        candidateList->setSelectionKey(KeyList(10));
        candidateList->setPageSize(instance_->globalConfig().defaultPageSize());
        if (candidateList->size()) {
            candidateList->setGlobalCursorIndex(0);
        }

        candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
        inputContext->inputPanel().setCandidateList(std::move(candidateList));
    }
    Text preedit;
    preedit.append(state->buffer_.userInput());
    if (!state->buffer_.empty()) {
        preedit.setCursor(state->buffer_.cursorByChar());
    }

    Text auxUp(_("Unicode: "));
    if (trigger) {
        auxUp.append(_("(Type to search unicode by code or description)"));
    }
    // inputContext->inputPanel().setClientPreedit(preedit);
    inputContext->inputPanel().setAuxUp(auxUp);
    inputContext->inputPanel().setPreedit(preedit);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

class UnicodeModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Unicode(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::UnicodeModuleFactory);
