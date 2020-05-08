/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _TESTIM_TESTIM_H_
#define _TESTIM_TESTIM_H_

#include <functional>
#include "fcitx/inputmethodengine.h"
#include "fcitx/instance.h"
#include "testim_public.h"

namespace fcitx {

class TestIM : public InputMethodEngine {
public:
    TestIM(Instance *instance);
    ~TestIM();

    Instance *instance() { return instance_; }

    std::vector<InputMethodEntry> listInputMethods() override {
        std::vector<InputMethodEntry> result;
        result.push_back(InputMethodEntry("keyboard-us", "Test IM keyboard",
                                          "en", "testim"));
        result.push_back(InputMethodEntry("testim", "Test IM", "en", "testim"));
        result.push_back(
            InputMethodEntry("testim2", "Test IM", "en", "testim"));
        return result;
    }
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;

private:
    void setHandler(
        std::function<void(const InputMethodEntry &entry, KeyEvent &keyEvent)>
            handler) {
        handler_ = std::move(handler);
    }

    FCITX_ADDON_EXPORT_FUNCTION(TestIM, setHandler);
    Instance *instance_;
    std::function<void(const InputMethodEntry &entry, KeyEvent &keyEvent)>
        handler_;
};
} // namespace fcitx

#endif // _TESTIM_TESTIM_H_
