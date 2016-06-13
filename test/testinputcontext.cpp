/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <assert.h>
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/focusgroup.h"

#define TEST_FOCUS(ARGS...)                                                                                            \
    do {                                                                                                               \
        bool focus_result[] = {ARGS};                                                                                  \
        for (size_t i = 0; i < FCITX_ARRAY_SIZE(focus_result); i++) {                                                  \
            assert(ic[i]->hasFocus() == focus_result[i]);                                                              \
        }                                                                                                              \
    } while (0)

using namespace fcitx;

class TestInputContext : public InputContext {
public:
    TestInputContext(InputContextManager &manager) : InputContext(manager) {}

    void commitStringImpl(const std::string &) override {}
    void deleteSurroundingTextImpl(int, unsigned int) override {}
    void forwardKeyImpl(const KeyEvent &) override {}
    void updatePreeditImpl() override {}
};

int main() {
    InputContextManager manager;

    std::vector<std::unique_ptr<InputContext>> ic;

    for (int i = 0; i < 8; i++) {
        ic.emplace_back(new TestInputContext(manager));
    }

    ic.pop_back();
    ic.emplace_back(new TestInputContext(manager));

    FocusGroup group(manager), group2(manager);
    ic[0]->setFocusGroup(&manager.globalFocusGroup());
    ic[1]->setFocusGroup(&manager.globalFocusGroup());
    ic[2]->setFocusGroup(&group);
    ic[3]->setFocusGroup(&group);
    ic[4]->setFocusGroup(&group2);
    ic[5]->setFocusGroup(&group2);

    TEST_FOCUS(false, false, false, false, false, false, false, false);
    ic[0]->focusIn();
    TEST_FOCUS(true, false, false, false, false, false, false, false);
    ic[0]->focusOut();
    TEST_FOCUS(false, false, false, false, false, false, false, false);
    ic[2]->focusIn();
    TEST_FOCUS(false, false, true, false, false, false, false, false);
    ic[3]->focusIn();
    TEST_FOCUS(false, false, false, true, false, false, false, false);
    ic[4]->focusIn();
    TEST_FOCUS(false, false, false, true, true, false, false, false);
    ic[6]->focusIn();
    TEST_FOCUS(false, false, false, true, true, false, true, false);
    ic[7]->focusIn();
    TEST_FOCUS(false, false, false, true, true, false, true, true);
    ic[1]->focusIn();
    TEST_FOCUS(false, true, false, false, false, false, true, true);
    ic[5]->focusIn();
    TEST_FOCUS(false, false, false, false, false, true, true, true);

    ic[1]->setCapabilityFlags(CapabilityFlag::Digit);
    assert(ic[1]->capabilityFlags() == CapabilityFlag::Digit);

    return 0;
}
