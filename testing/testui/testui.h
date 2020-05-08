/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_TEST_TESTUI_H_
#define _FCITX_UI_TEST_TESTUI_H_

#include "fcitx/instance.h"
#include "fcitx/userinterface.h"

namespace fcitx {

class TestUI : public UserInterface {
public:
    TestUI(Instance *instance);
    ~TestUI();

    Instance *instance() { return instance_; }
    void suspend() override;
    void resume() override;
    bool available() override { return true; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;

private:
    void printStatusArea(InputContext *inputContext);
    void printInputPanel(InputContext *inputContext);

    Instance *instance_;
    bool suspended_ = true;
};
} // namespace fcitx

#endif // _FCITX_UI_TEST_TESTUI_H_
