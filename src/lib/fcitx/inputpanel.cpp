/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputpanel.h"

namespace fcitx {

class InputPanelPrivate {
public:
    Text auxDown_;
    Text auxUp_;
    Text preedit_;
    Text clientPreedit_;
    std::shared_ptr<CandidateList> candidate_;
    InputContext *ic_;
    CustomInputPanelCallback customCallback_ = nullptr;
    CustomInputPanelCallback customVirtualKeyboardCallback_ = nullptr;
};

InputPanel::InputPanel(InputContext *ic)
    : d_ptr(std::make_unique<InputPanelPrivate>()) {
    FCITX_D();
    d->ic_ = ic;
}

InputPanel::~InputPanel() {}

void InputPanel::setAuxDown(const Text &text) {
    FCITX_D();
    d->auxDown_ = text;
}

void InputPanel::setAuxUp(const Text &text) {
    FCITX_D();
    d->auxUp_ = text;
}

void InputPanel::setCandidateList(std::unique_ptr<CandidateList> candidate) {
    FCITX_D();
    d->candidate_ = std::move(candidate);
}

void InputPanel::setClientPreedit(const Text &clientPreedit) {
    FCITX_D();
    d->clientPreedit_ = clientPreedit.normalize();
    // If it is empty preedit, always set cursor to 0.
    // An empty preedit with hidden cursor would only cause issues.
    if (d->clientPreedit_.empty()) {
        d->clientPreedit_.setCursor(0);
    }
}

void InputPanel::setPreedit(const Text &text) {
    FCITX_D();
    d->preedit_ = text;
}

const Text &InputPanel::auxDown() const {
    FCITX_D();
    return d->auxDown_;
}

const Text &InputPanel::auxUp() const {
    FCITX_D();
    return d->auxUp_;
}

const Text &InputPanel::clientPreedit() const {
    FCITX_D();
    return d->clientPreedit_;
}

const Text &InputPanel::preedit() const {
    FCITX_D();
    return d->preedit_;
}

const CustomInputPanelCallback &InputPanel::customInputPanelCallback() const {
    FCITX_D();
    return d->customCallback_;
}

void InputPanel::setCustomInputPanelCallback(
    CustomInputPanelCallback callback) {
    FCITX_D();
    d->customCallback_ = std::move(callback);
}

const CustomInputPanelCallback &
InputPanel::customVirtualKeyboardCallback() const {
    FCITX_D();
    return d->customVirtualKeyboardCallback_;
}

void InputPanel::setCustomVirtualKeyboardCallback(
    CustomInputPanelCallback callback) {
    FCITX_D();
    d->customVirtualKeyboardCallback_ = std::move(callback);
}

void InputPanel::reset() {
    FCITX_D();
    d->preedit_.clear();
    d->clientPreedit_.clear();
    d->clientPreedit_.setCursor(0);
    d->candidate_.reset();
    d->auxUp_.clear();
    d->auxDown_.clear();
    d->customCallback_ = nullptr;
    d->customVirtualKeyboardCallback_ = nullptr;
}

bool InputPanel::empty() const {
    FCITX_D();
    return d->preedit_.empty() && d->clientPreedit_.empty() &&
           (!d->candidate_ || d->candidate_->size() == 0) &&
           d->auxUp_.empty() && d->auxDown_.empty();
}

std::shared_ptr<CandidateList> InputPanel::candidateList() const {
    FCITX_D();
    return d->candidate_;
}
} // namespace fcitx
