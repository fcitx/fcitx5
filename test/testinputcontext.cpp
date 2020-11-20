/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <vector>
#include "fcitx-utils/log.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputcontextproperty.h"

#define TEST_FOCUS(ARGS...)                                                    \
    do {                                                                       \
        bool focus_result[] = {ARGS};                                          \
        for (size_t i = 0; i < FCITX_ARRAY_SIZE(focus_result); i++) {          \
            FCITX_ASSERT(ic[i]->hasFocus() == focus_result[i]);                \
        }                                                                      \
    } while (0)

using namespace fcitx;

class TestInputContext : public InputContext {
public:
    TestInputContext(InputContextManager &manager,
                     const std::string &program = {})
        : InputContext(manager, program) {}

    ~TestInputContext() { destroy(); }

    const char *frontend() const override { return "test"; }

    void commitStringImpl(const std::string &) override {}
    void deleteSurroundingTextImpl(int, unsigned int) override {}
    void forwardKeyImpl(const ForwardKeyEvent &) override {}
    void updatePreeditImpl() override {}
};

class TestProperty : public InputContextProperty {
public:
    int num() const { return num_; }
    void setNum(int n) { num_ = n; }

protected:
    int num_ = 0;
};

class TestSharedProperty : public TestProperty {
public:
    bool needCopy() const override { return true; }
    void copyTo(InputContextProperty *other_) override {
        auto *other = static_cast<TestSharedProperty *>(other_);
        other->num_ = num_;
    }
};

void test_simple() {
    InputContextManager manager;

    {
        std::vector<std::unique_ptr<InputContext>> ic;
        ic.reserve(8);
        for (int i = 0; i < 8; i++) {
            ic.emplace_back(std::make_unique<TestInputContext>(manager));
        }

        ic.pop_back();
        ic.emplace_back(new TestInputContext(manager));

        FocusGroup group("", manager), group2("", manager);
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
        TEST_FOCUS(false, true, false, true, true, false, true, true);
        ic[5]->focusIn();
        TEST_FOCUS(false, true, false, true, false, true, true, true);

        ic[1]->setCapabilityFlags(CapabilityFlag::Digit);
        FCITX_ASSERT(ic[1]->capabilityFlags() == CapabilityFlag::Digit);
    }

    {
        std::vector<std::unique_ptr<InputContext>> ic;
        ic.emplace_back(new TestInputContext(manager, "Firefox"));
        ic.emplace_back(new TestInputContext(manager, "Firefox"));
        ic.emplace_back(new TestInputContext(manager, "Chrome"));

        SimpleInputContextPropertyFactory<TestSharedProperty> testsharedFactory;
        FactoryFor<TestProperty> testFactory(
            [](InputContext &) { return new TestProperty; });
        FCITX_ASSERT(manager.registerProperty("shared", &testsharedFactory));
        FCITX_ASSERT(manager.registerProperty("property", &testFactory));

        ic.emplace_back(new TestInputContext(manager, "Chrome"));

        std::array<const char *, 2> slot{{"shared", "property"}};
        auto check = [&ic, &slot](auto expect) {
            int idx = 0;
            for (const auto *s : slot) {
                int idx2 = 0;
                for (auto &context : ic) {
                    FCITX_ASSERT(context->propertyAs<TestProperty>(s)->num() ==
                                 expect[idx][idx2]);
                    idx2++;
                }
                idx++;
            }
        };

        {
            int expect[][4] = {
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            };
            check(expect);
        }

        ic[0]->propertyAs<TestProperty>(slot[0])->setNum(1);
        ic[0]->propertyAs<TestProperty>(slot[1])->setNum(2);
        ic[0]->updateProperty(slot[0]);
        ic[0]->updateProperty(slot[1]);
        {
            int expect[][4] = {
                {1, 0, 0, 0},
                {2, 0, 0, 0},
            };
            check(expect);
        }
        manager.setPropertyPropagatePolicy(PropertyPropagatePolicy::Program);
        ic[0]->updateProperty(slot[0]);
        ic[0]->updateProperty(slot[1]);
        {
            int expect[][4] = {
                {1, 1, 0, 0},
                {2, 0, 0, 0},
            };
            check(expect);
        }
        manager.setPropertyPropagatePolicy(PropertyPropagatePolicy::All);
        ic[0]->updateProperty(slot[0]);
        ic[0]->updateProperty(slot[1]);
        {
            int expect[][4] = {
                {1, 1, 1, 1},
                {2, 0, 0, 0},
            };
            check(expect);
        }
        ic.emplace_back(new TestInputContext(manager, "Firefox"));
        FCITX_ASSERT(ic.back()->propertyAs<TestProperty>(slot[0])->num() == 1);
        FCITX_ASSERT(ic.back()->propertyAs<TestProperty>(slot[1])->num() == 0);
        manager.setPropertyPropagatePolicy(PropertyPropagatePolicy::Program);
        {
            ic[3]->propertyAs<TestProperty>(slot[0])->setNum(3);
            ic[3]->updateProperty(slot[0]);
            int expect[][5] = {
                {1, 1, 3, 3, 1},
                {2, 0, 0, 0, 0},
            };
            check(expect);
        }
    }
}

void test_property() {
    InputContextManager manager;
    FactoryFor<TestProperty> testFactory(
        [](InputContext &) { return new TestProperty; });
    manager.registerProperty("test", &testFactory);
    std::vector<std::unique_ptr<InputContext>> ic;
    ic.emplace_back(new TestInputContext(manager, "Firefox"));
    auto *testProperty = ic[0]->propertyFor(&testFactory);
    FCITX_ASSERT(testProperty->num() == 0);
    FCITX_ASSERT(testFactory.registered());
    testFactory.unregister();
    FCITX_ASSERT(!testFactory.registered());

    manager.registerProperty("test", &testFactory);
    auto *testProperty2 = ic[0]->propertyFor(&testFactory);
    FCITX_ASSERT(testProperty2->num() == 0);
}

void test_preedit_override() {
    InputContextManager manager;
    auto ic = std::make_unique<TestInputContext>(manager, "Firefox");
    ic->setCapabilityFlags(CapabilityFlag::Preedit);
    FCITX_ASSERT(ic->capabilityFlags().test(CapabilityFlag::Preedit));
    ic->setEnablePreedit(false);
    FCITX_ASSERT(!ic->capabilityFlags().test(CapabilityFlag::Preedit));
    manager.setPreeditEnabledByDefault(false);
    ic = std::make_unique<TestInputContext>(manager, "Firefox");
    FCITX_ASSERT(!ic->capabilityFlags().test(CapabilityFlag::Preedit));
    ic->setCapabilityFlags(CapabilityFlag::Preedit);
    FCITX_ASSERT(!ic->capabilityFlags().test(CapabilityFlag::Preedit));
    ic->setEnablePreedit(true);
    FCITX_ASSERT(ic->capabilityFlags().test(CapabilityFlag::Preedit));
}

int main() {
    test_simple();
    test_property();
    test_preedit_override();

    return 0;
}
