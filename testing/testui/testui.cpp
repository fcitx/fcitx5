/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "testui.h"
#include <iostream>
#include "fcitx-utils/utf8.h"
#include "fcitx/action.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputpanel.h"
#include "fcitx/statusarea.h"

namespace fcitx {

TestUI::TestUI(Instance *instance) : instance_(instance) {}

TestUI::~TestUI() {}

void TestUI::resume() { suspended_ = false; }

void TestUI::suspend() { suspended_ = true; }

void TestUI::update(UserInterfaceComponent component,
                    InputContext *inputContext) {
    switch (component) {
    case UserInterfaceComponent::StatusArea:
        printStatusArea(inputContext);
        break;
    case UserInterfaceComponent::InputPanel:
        printInputPanel(inputContext);
        break;
    }
}

void TestUI::printStatusArea(InputContext *inputContext) {
    auto &statusArea = inputContext->statusArea();
    for (auto group : {StatusGroup::BeforeInputMethod, StatusGroup::InputMethod,
                       StatusGroup::AfterInputMethod}) {
        for (auto *action : statusArea.actions(group)) {
            std::cout << "Action: " << action->name() << std::endl;
            std::cout << "Text: " << action->shortText(inputContext)
                      << std::endl;
            std::cout << "Icon: " << action->icon(inputContext) << std::endl;
        }
    }
}

void TestUI::printInputPanel(InputContext *inputContext) {
    auto &inputPanel = inputContext->inputPanel();
    auto preedit = instance_->outputFilter(inputContext, inputPanel.preedit());
    auto preeditString = preedit.toString();
    auto auxUp = instance_->outputFilter(inputContext, inputPanel.auxUp());
    auto auxDown = instance_->outputFilter(inputContext, inputPanel.auxDown());
    auto cursor = preedit.cursor();
    if (cursor >= 0) {
        preeditString.insert(cursor, "|");
    }
    FCITX_ASSERT(utf8::validate(preeditString));
    std::cerr << "Preedit: " << preeditString << std::endl;
    FCITX_ASSERT(utf8::validate(auxUp.toString()));
    std::cerr << "AuxUp: " << auxUp.toString() << std::endl;
    FCITX_ASSERT(utf8::validate(auxDown.toString()));
    std::cerr << "AuxDown: " << auxDown.toString() << std::endl;
    std::cerr << "Candidates: " << std::endl;
    if (auto candidateList = inputPanel.candidateList()) {
        for (int i = 0; i < candidateList->size(); i++) {
            auto label =
                instance_->outputFilter(inputContext, candidateList->label(i));
            auto candidate = instance_->outputFilter(
                inputContext, candidateList->candidate(i).text());
            FCITX_ASSERT(utf8::validate(candidate.toString()));
            std::cerr << label.toString() << " " << candidate.toString()
                      << std::endl;
        }
    }
}

class TestUIFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new TestUI(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY_V2(testui, fcitx::TestUIFactory);
