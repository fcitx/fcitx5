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
#include "fcitx/inputcontextproperty.h"
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
    TestInputContext(InputContextManager &manager, const std::string &program = {}) : InputContext(manager, program) {}

    void commitStringImpl(const std::string &) override {}
    void deleteSurroundingTextImpl(int, unsigned int) override {}
    void forwardKeyImpl(const ForwardKeyEvent &) override {}
    void updatePreeditImpl() override {}
};

class TestProperty : public InputContextProperty {
public:
    int num() const { return m_num; }
    void setNum(int n) { m_num = n; }

protected:
    int m_num = 0;
};

class TestSharedProperty : public TestProperty {
public:
    bool needCopy() const override { return true; }
    void copyTo(InputContextProperty *other_) override {
        auto other = static_cast<TestSharedProperty *>(other_);
        other->m_num = m_num;
    }
};

int main() {
    InputContextManager manager;

    {
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
    }

    {
        std::vector<std::unique_ptr<InputContext>> ic;
        ic.emplace_back(new TestInputContext(manager, "Firefox"));
        ic.emplace_back(new TestInputContext(manager, "Firefox"));
        ic.emplace_back(new TestInputContext(manager, "Chrome"));

        int slot[] = {manager.registerProperty([](InputContext &) { return new TestSharedProperty; }),
                      manager.registerProperty([](InputContext &) { return new TestProperty; })};

        ic.emplace_back(new TestInputContext(manager, "Chrome"));

        auto check = [&ic, &slot](auto expect) {
            int idx = 0;
            for (auto s : slot) {
                int idx2 = 0;
                for (auto &context : ic) {
                    assert(context->propertyAs<TestProperty>(s)->num() == expect[idx][idx2]);
                    idx2++;
                }
                idx++;
            }
        };

        {
            int expect[][4] = {
                {0, 0, 0, 0}, {0, 0, 0, 0},
            };
            check(expect);
        }

        ic[0]->propertyAs<TestProperty>(slot[0])->setNum(1);
        ic[0]->propertyAs<TestProperty>(slot[1])->setNum(2);
        ic[0]->updateProperty(slot[0]);
        ic[0]->updateProperty(slot[1]);
        {
            int expect[][4] = {
                {1, 0, 0, 0}, {2, 0, 0, 0},
            };
            check(expect);
        }
        manager.setPropertyPropagatePolicy(PropertyPropagatePolicy::Program);
        ic[0]->updateProperty(slot[0]);
        ic[0]->updateProperty(slot[1]);
        {
            int expect[][4] = {
                {1, 1, 0, 0}, {2, 0, 0, 0},
            };
            check(expect);
        }
        manager.setPropertyPropagatePolicy(PropertyPropagatePolicy::All);
        ic[0]->updateProperty(slot[0]);
        ic[0]->updateProperty(slot[1]);
        {
            int expect[][4] = {
                {1, 1, 1, 1}, {2, 0, 0, 0},
            };
            check(expect);
        }
        ic.emplace_back(new TestInputContext(manager, "Firefox"));
        assert(ic.back()->propertyAs<TestProperty>(slot[0])->num() == 1);
        assert(ic.back()->propertyAs<TestProperty>(slot[1])->num() == 0);
        manager.setPropertyPropagatePolicy(PropertyPropagatePolicy::Program);
        {
            ic[3]->propertyAs<TestProperty>(slot[0])->setNum(3);
            ic[3]->updateProperty(slot[0]);
            int expect[][5] = {
                {1, 1, 3, 3, 1}, {2, 0, 0, 0, 0},
            };
            check(expect);
        }
    }

    return 0;
}
