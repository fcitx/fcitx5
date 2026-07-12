/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "unicodetempmode.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>
#include <format>
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/textformatflags.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/text.h"
#include "fcitx/userinterface.h"
#include "unicode.h"

namespace fcitx {

namespace {

bool isHexKey(const Key &key) {
    if (key.isDigit()) {
        return true;
    }
    if (key.isUAZ() || key.isLAZ()) {
        return charutils::isxdigit(key.sym());
    }
    return false;
}

bool bufferIsValid(const std::string &input, uint32_t *ret) {
    std::string hex = input;
    std::transform(hex.begin(), hex.end(), hex.begin(), charutils::tolower);
    uint32_t value = 0;
    try {
        value = stoi(hex, nullptr, 16);
    } catch (...) {
        return false;
    }

    const bool valid = utf8::UCS4IsValid(value);
    if (valid && ret) {
        *ret = value;
    }
    return valid;
}

class UnicodeCandidateWord : public CandidateWord {
public:
    UnicodeCandidateWord(UnicodeTempMode *tempMode, Unicode *unicode,
                         uint32_t chr)
        : tempMode_(tempMode) {
        Text text, comment;
        text.append(utf8::UCS4ToUTF8(chr));
        comment.append(unicode->data().name(chr));
        setText(std::move(text));
        setComment(std::move(comment));
    }

    void select(InputContext *inputContext) const override {
        inputContext->commitString(text().stringAt(0));
        tempMode_->reset(inputContext);
    }

private:
    UnicodeTempMode *tempMode_;
};

} // namespace

UnicodeState::UnicodeState() { buffer_.setMaxSize(30); }

void UnicodeState::reset(InputContext *inputContext) {
    mode_ = UnicodeMode::Off;
    buffer_.clear();
    buffer_.shrinkToFit();
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

UnicodeTempMode::UnicodeTempMode(Unicode *unicode) : unicode_(unicode) {}

bool UnicodeTempMode::trigger(InputContext *inputContext) {
    return trigger(inputContext, UnicodeMode::Search);
}

bool UnicodeTempMode::triggerTempMode(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return false;
    }

    UnicodeMode mode;
    if (keyEvent.key().checkKeyList(unicode_->config().triggerKey.value())) {
        mode = UnicodeMode::Search;
    } else if (keyEvent.key().checkKeyList(
                   unicode_->config().directUnicodeKey.value())) {
        mode = UnicodeMode::Direct;
    } else {
        return false;
    }
    return trigger(keyEvent.inputContext(), mode);
}

bool UnicodeTempMode::keyEvent(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return false;
    }
    if (property(keyEvent.inputContext())->mode_ == UnicodeMode::Search) {
        return handleSearch(keyEvent);
    }
    return handleDirect(keyEvent);
}

void UnicodeTempMode::reset(InputContext *inputContext) {
    if (auto *state = property(inputContext)) {
        Base::reset(inputContext);
        state->reset(inputContext);
    }
}

std::string_view UnicodeTempMode::name() const { return "unicodeState"; }

const KeyList &UnicodeTempMode::triggerKeys() const {
    return unicode_->config().triggerKey.value();
}

bool UnicodeTempMode::trigger(InputContext *inputContext, UnicodeMode mode) {
    if (!unicode_->data().load()) {
        return false;
    }
    auto *state = property(inputContext);
    state->mode_ = mode;
    state->setActive(true);
    updateUI(inputContext, true);
    return true;
}

bool UnicodeTempMode::handleSearch(const KeyEvent &keyEvent) {
    auto *inputContext = keyEvent.inputContext();
    auto *state = property(inputContext);
    auto candidateList = inputContext->inputPanel().candidateList();
    if (candidateList) {
        int idx = keyEvent.key().digitSelection(KeyState::Alt);
        if (idx >= 0) {
            if (idx < candidateList->size()) {
                candidateList->candidate(idx).select(inputContext);
            }
            return true;
        }

        if (keyEvent.key().checkKeyList(
                unicode_->instance()->globalConfig().defaultPrevPage())) {
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
                unicode_->instance()->globalConfig().defaultNextPage())) {
            candidateList->toPageable()->next();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(
                unicode_->instance()->globalConfig().defaultPrevCandidate())) {
            candidateList->toCursorMovable()->prevCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(
                unicode_->instance()->globalConfig().defaultNextCandidate())) {
            candidateList->toCursorMovable()->nextCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().check(FcitxKey_Return) ||
            keyEvent.key().check(FcitxKey_KP_Enter)) {
            if (!candidateList->empty() && candidateList->cursorIndex() >= 0) {
                candidateList->candidate(candidateList->cursorIndex())
                    .select(inputContext);
            }
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
    if (keyEvent.key().check(FcitxKey_BackSpace)) {
        if (state->buffer_.empty()) {
            reset(inputContext);
        } else if (state->buffer_.backspace()) {
            if (state->buffer_.empty()) {
                reset(inputContext);
            } else {
                updateUI(inputContext);
            }
        }
        return true;
    }

    auto compose = unicode_->instance()->processComposeString(
        inputContext, keyEvent.key().sym());
    if (!compose) {
        return true;
    }
    if (!compose->empty()) {
        state->buffer_.type(*compose);
    } else {
        state->buffer_.type(Key::keySymToUnicode(keyEvent.key().sym()));
    }
    updateUI(inputContext);
    return true;
}

bool UnicodeTempMode::handleDirect(const KeyEvent &keyEvent) {
    auto *inputContext = keyEvent.inputContext();
    auto *state = property(inputContext);
    if (keyEvent.key().isModifier() || keyEvent.key().hasModifier()) {
        return true;
    }
    if (keyEvent.key().check(FcitxKey_Escape)) {
        reset(inputContext);
        return true;
    }

    if (keyEvent.key().check(FcitxKey_BackSpace)) {
        if (state->buffer_.backspace()) {
            updateUI(inputContext);
        } else {
            reset(inputContext);
        }
        return true;
    }

    if (isHexKey(keyEvent.key())) {
        const auto keyStr = Key::keySymToUTF8(keyEvent.key().sym());
        if (keyStr.empty() || !state->buffer_.type(keyStr)) {
            return true;
        }
        if (bufferIsValid(state->buffer_.userInput(), nullptr)) {
            updateUI(inputContext);
        } else {
            state->buffer_.backspace();
        }
        return true;
    }
    if (keyEvent.key().check(FcitxKey_space) ||
        keyEvent.key().check(FcitxKey_KP_Space) ||
        keyEvent.key().check(FcitxKey_Return) ||
        keyEvent.key().check(FcitxKey_KP_Enter)) {
        if (!state->buffer_.empty()) {
            uint32_t value = 0;
            if (bufferIsValid(state->buffer_.userInput(), &value) && value) {
                inputContext->commitString(utf8::UCS4ToUTF8(value));
            }
            reset(inputContext);
        }
    }
    updateUI(inputContext);
    return true;
}

void UnicodeTempMode::updateUI(InputContext *inputContext, bool trigger) {
    auto *state = property(inputContext);
    inputContext->inputPanel().reset();
    if (state->mode_ == UnicodeMode::Direct) {
        TextFormatFlags format = TextFormatFlag::DontCommit;
        const bool clientPreedit =
            inputContext->capabilityFlags().test(CapabilityFlag::Preedit);
        if (clientPreedit) {
            format = {TextFormatFlag::Underline, TextFormatFlag::DontCommit};
        }
        Text preedit;
        preedit.append("U+", format);
        if (!state->buffer_.empty()) {
            preedit.append(state->buffer_.userInput(), format);
        }
        preedit.setCursor(preedit.textLength());
        if (clientPreedit) {
            inputContext->inputPanel().setClientPreedit(preedit);
        } else {
            inputContext->inputPanel().setPreedit(preedit);
        }
    } else if (state->mode_ == UnicodeMode::Search) {
        if (!state->buffer_.empty()) {
            auto result = unicode_->data().find(state->buffer_.userInput());
            auto candidateList = std::make_unique<CommonCandidateList>();
            candidateList->setPageSize(
                unicode_->instance()->globalConfig().defaultPageSize());
            for (auto chr : result) {
                if (utf8::UCS4IsValid(chr)) {
                    candidateList->append<UnicodeCandidateWord>(this, unicode_,
                                                                chr);
                }
            }
            if (!candidateList->empty()) {
                candidateList->setGlobalCursorIndex(0);
            }
            candidateList->setSelectionKey(unicode_->selectionKeys());
            candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
            inputContext->inputPanel().setCandidateList(
                std::move(candidateList));
        } else if (trigger) {
            auto candidateList = std::make_unique<CommonCandidateList>();
            std::vector<std::string> selectedTexts;
            if (inputContext->capabilityFlags().test(
                    CapabilityFlag::SurroundingText)) {
                if (auto selected =
                        inputContext->surroundingText().selectedText();
                    !selected.empty()) {
                    selectedTexts.push_back(std::move(selected));
                }
            }
            if (unicode_->clipboard()) {
                if (selectedTexts.empty()) {
                    auto primary =
                        unicode_->clipboard()->call<IClipboard::primary>(
                            inputContext);
                    if (std::find(selectedTexts.begin(), selectedTexts.end(),
                                  primary) == selectedTexts.end()) {
                        selectedTexts.push_back(std::move(primary));
                    }
                }
                auto clip = unicode_->clipboard()->call<IClipboard::clipboard>(
                    inputContext);
                if (std::find(selectedTexts.begin(), selectedTexts.end(),
                              clip) == selectedTexts.end()) {
                    selectedTexts.push_back(std::move(clip));
                }
            }
            std::unordered_set<uint32_t> seenChar;
            for (const auto &str : selectedTexts) {
                if (!utf8::validate(str)) {
                    continue;
                }
                constexpr int limit = 20;
                int counter = 0;
                for (auto chr : utf8::MakeUTF8CharRange(str)) {
                    if (!seenChar.insert(chr).second) {
                        continue;
                    }
                    auto name = unicode_->data().name(chr);
                    std::string comment;
                    if (!name.empty()) {
                        comment = std::format("U+{0:04X} {1}", chr, name);
                    } else {
                        comment = std::format("U+{0:04X}", chr);
                    }
                    candidateList->append<DisplayOnlyCandidateWord>(
                        Text(utf8::UCS4ToUTF8(chr)), Text(std::move(comment)));
                    if (counter >= limit) {
                        break;
                    }
                    counter += 1;
                }
            }
            candidateList->setSelectionKey(KeyList(10));
            candidateList->setPageSize(
                unicode_->instance()->globalConfig().defaultPageSize());
            if (!candidateList->empty()) {
                candidateList->setGlobalCursorIndex(0);
            }
            candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
            inputContext->inputPanel().setCandidateList(
                std::move(candidateList));
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
        inputContext->inputPanel().setAuxUp(auxUp);
        inputContext->inputPanel().setPreedit(preedit);
    }
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

} // namespace fcitx
