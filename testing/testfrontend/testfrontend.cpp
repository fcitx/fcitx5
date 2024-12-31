/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testfrontend.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"

namespace fcitx {

class TestInputContext : public InputContext {
public:
    TestInputContext(TestFrontend *frontend,
                     InputContextManager &inputContextManager,
                     const std::string &program)
        : InputContext(inputContextManager, program), frontend_(frontend) {
        created();
    }
    ~TestInputContext() { destroy(); }

    const char *frontend() const override { return "testfrontend"; }

    void commitStringImpl(const std::string &text) override {
        FCITX_INFO() << "Commit: " << text;
        frontend_->commitString(text);
    }
    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        FCITX_INFO() << "ForwardKey: " << key.key();
    }
    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        FCITX_INFO() << "DeleteSurrounding: " << offset << " " << size;
    }

    void updatePreeditImpl() override {
        FCITX_INFO() << "Update preedit: "
                     << inputPanel().clientPreedit().toString();
    }

private:
    TestFrontend *frontend_;
};

TestFrontend::TestFrontend(Instance *instance) : instance_(instance) {}

TestFrontend::~TestFrontend() {
    FCITX_ASSERT(commitExpectation_.empty()) << commitExpectation_;
}

ICUUID
TestFrontend::createInputContext(const std::string &program) {
    auto *ic =
        new TestInputContext(this, instance_->inputContextManager(), program);
    return ic->uuid();
}

void TestFrontend::destroyInputContext(ICUUID uuid) {
    auto *ic = instance_->inputContextManager().findByUUID(uuid);
    delete ic;
}

bool TestFrontend::sendKeyEvent(ICUUID uuid, const Key &key, bool isRelease) {
    auto *ic = instance_->inputContextManager().findByUUID(uuid);
    if (!ic) {
        return false;
    }
    if (!ic->hasFocus()) {
        ic->focusIn();
    }
    KeyEvent keyEvent(ic, key, isRelease);
    auto result = ic->keyEvent(keyEvent);
    FCITX_INFO() << "KeyEvent key: " << key.toString()
                 << " isRelease: " << isRelease
                 << " accepted: " << keyEvent.accepted();
    return result;
}

void TestFrontend::keyEvent(ICUUID uuid, const Key &key, bool isRelease) {
    sendKeyEvent(uuid, key, isRelease);
}

void TestFrontend::commitString(const std::string &expect) {
    if (!checkExpectation_) {
        return;
    }
    FCITX_ASSERT(!commitExpectation_.empty() &&
                 expect == commitExpectation_.front())
        << "commitString: " << expect;
    commitExpectation_.pop_front();
}

void TestFrontend::pushCommitExpectation(std::string expect) {
    checkExpectation_ = true;
    commitExpectation_.push_back(std::move(expect));
}

class TestFrontendFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new TestFrontend(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY_V2(testfrontend, fcitx::TestFrontendFactory);
