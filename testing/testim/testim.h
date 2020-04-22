//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
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
