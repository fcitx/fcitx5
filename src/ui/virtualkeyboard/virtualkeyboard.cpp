/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "virtualkeyboard.h"
#include <fcitx/inputmethodengine.h>
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "fcitx/userinterfacemanager.h"
#include "dbus_public.h"

namespace fcitx {

class VirtualKeyboardService
    : public dbus::ObjectVTable<VirtualKeyboardService> {
public:
    explicit VirtualKeyboardService(VirtualKeyboard *parent)
        : parent_(parent) {}

    ~VirtualKeyboardService() = default;

    void showVirtualKeyboard() {
        if (!parent_->available() || parent_->isVirtualKeyboardVisible()) {
            return;
        }

        parent_->instance()->setInputMethodMode(
            InputMethodMode::OnScreenKeyboard);

        parent_->showVirtualKeyboard();
    }

    void hideVirtualKeyboard() {
        if (!parent_->isVirtualKeyboardVisible()) {
            return;
        }

        parent_->hideVirtualKeyboard();
    }

private:
    FCITX_OBJECT_VTABLE_METHOD(showVirtualKeyboard, "ShowVirtualKeyboard", "",
                               "");

    FCITX_OBJECT_VTABLE_METHOD(hideVirtualKeyboard, "HideVirtualKeyboard", "",
                               "");

private:
    VirtualKeyboard *parent_;
};

class VirtualKeyboardProxy : public dbus::ObjectVTable<VirtualKeyboardProxy> {

public:
    VirtualKeyboardProxy(VirtualKeyboard *parent, dbus::Bus *bus)
        : parent_(parent), bus_(bus) {}

    ~VirtualKeyboardProxy() = default;

    void processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state,
                         bool isRelease, uint32_t time);

    void processVisibilityEvent(bool visible) { parent_->setVisible(visible); }

    void selectCandidate(int index) {
        auto *inputContext = parent_->instance()->mostRecentInputContext();
        if (inputContext == nullptr) {
            return;
        }

        if (auto candidateList = inputContext->inputPanel().candidateList()) {
            const auto *candidate =
                nthCandidateIgnorePlaceholder(*candidateList, index);
            if (candidate != nullptr) {
                candidate->select(inputContext);
            }
        }
    }

    std::vector<dbus::DBusStruct<std::string, std::string>>
    getInputMethodList();

    void setCurrentInputMethod(const std::string &name) {
        parent_->instance()->setCurrentInputMethod(name);
    }

    void prevPage();

    void nextPage();

private:
    PageableCandidateList *getPageableCandidateList();

private:
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuubu", "");

    FCITX_OBJECT_VTABLE_METHOD(processVisibilityEvent,
                               "PprocessVisibilityEvent", "b", "");

    FCITX_OBJECT_VTABLE_METHOD(selectCandidate, "SelectCandidate", "i", "");

    FCITX_OBJECT_VTABLE_METHOD(getInputMethodList, "GetInputMethodList", "",
                               "a(ss)");

    FCITX_OBJECT_VTABLE_METHOD(setCurrentInputMethod, "SetCurrentInpputMethod",
                               "s", "");

    FCITX_OBJECT_VTABLE_METHOD(prevPage, "PrevPage", "", "");

    FCITX_OBJECT_VTABLE_METHOD(nextPage, "NextPage", "", "");

    VirtualKeyboard *parent_;
    dbus::Bus *bus_;
};

void VirtualKeyboardProxy::processKeyEvent(uint32_t keyval, uint32_t keycode,
                                           uint32_t state, bool isRelease,
                                           uint32_t time) {
    auto *inputContext = parent_->instance()->mostRecentInputContext();
    if (inputContext == nullptr) {
        return;
    }

    KeyEvent event(inputContext,
                   Key(static_cast<KeySym>(keyval), KeyStates(state), keycode),
                   isRelease, time);
    if (!inputContext->hasFocus()) {
        inputContext->focusIn();
    }

    auto eventConsumed = inputContext->keyEvent(event);
    if (eventConsumed) {
        return;
    }

    inputContext->forwardKey(
        Key(static_cast<KeySym>(keyval), KeyStates(state), keycode), isRelease,
        time);
}

std::vector<dbus::DBusStruct<std::string, std::string>>
VirtualKeyboardProxy::getInputMethodList() {
    auto *instance = parent_->instance();
    auto &imManager = instance->inputMethodManager();
    auto &imList = imManager.currentGroup().inputMethodList();

    std::vector<dbus::DBusStruct<std::string, std::string>> inputMethodList;
    for (const auto &im : imList) {
        inputMethodList.emplace_back(std::make_tuple(im.name(), im.layout()));
    }

    return inputMethodList;
}

void VirtualKeyboardProxy::prevPage() {
    auto *inputContext = parent_->instance()->mostRecentInputContext();
    if (inputContext == nullptr) {
        return;
    }

    auto *pageable = getPageableCandidateList();
    if (pageable == nullptr) {
        return;
    }

    pageable->prev();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void VirtualKeyboardProxy::nextPage() {
    auto *inputContext = parent_->instance()->mostRecentInputContext();
    if (inputContext == nullptr) {
        return;
    }

    auto *pageable = getPageableCandidateList();
    if (pageable == nullptr) {
        return;
    }

    pageable->next();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

PageableCandidateList *VirtualKeyboardProxy::getPageableCandidateList() {
    auto *inputContext = parent_->instance()->mostRecentInputContext();
    if (inputContext == nullptr) {
        return nullptr;
    }

    auto &inputPanel = inputContext->inputPanel();
    auto candidateList = inputPanel.candidateList();
    if (candidateList == nullptr) {
        return nullptr;
    }

    return candidateList->toPageable();
}

VirtualKeyboard::VirtualKeyboard(Instance *instance)
    : instance_(instance), bus_(dbus()->call<IDBusModule::bus>()),
      watcher_(*bus_) {
    entry_ =
        watcher_.watchService("org.fcitx.virtualkeyboard.inputpanel",
                              [this](const std::string &, const std::string &,
                                     const std::string &newOwner) {
                                  FCITX_INFO() << "VirtualKeyboard new owner: "
                                               << newOwner;
                                  setAvailable(!newOwner.empty());
                              });
}

VirtualKeyboard::~VirtualKeyboard() = default;

void VirtualKeyboard::suspend() {
    eventHandlers_.clear();
    proxy_.reset();
    bus_->releaseName("org.fcitx.virtualkeyboard.inputpanel");
}

void VirtualKeyboard::resume() {
    proxy_ = std::make_unique<VirtualKeyboardProxy>(this, bus_);
    bus_->addObjectVTable("/virtualkeyboard",
                          "org.fcitx.virtualkeyboard.inputmethod", *proxy_);
    bus_->requestName(
        "org.fcitx.virtualkeyboard.inputmethod",
        Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                     dbus::RequestNameFlag::Queue});
    bus_->flush();

    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) { notifyIMListChanged(); }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextInputMethodActivated, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &activated = static_cast<InputMethodActivatedEvent &>(event);
            notifyIMActivated(activated.name());
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextInputMethodDeactivated,
        EventWatcherPhase::Default, [this](Event &event) {
            auto &deactivated =
                static_cast<InputMethodDeactivatedEvent &>(event);
            notifyIMDeactivated(deactivated.name());
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            instance_->setInputMethodMode(InputMethodMode::PhysicalKeyboard);
        }));
}

void VirtualKeyboard::update(UserInterfaceComponent component,
                             InputContext *inputContext) {
    if (component == UserInterfaceComponent::InputPanel) {
        updateInputPanel(inputContext);
    }
}

void VirtualKeyboard::showVirtualKeyboard() {
    if (isVirtualKeyboardVisible()) {
        return;
    }

    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "ShowVirtualKeyboard");

    msg.send();
}

void VirtualKeyboard::hideVirtualKeyboard() {
    if (!isVirtualKeyboardVisible()) {
        return;
    }

    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "HideVirtualKeyboard");

    msg.send();
}

void VirtualKeyboard::updateInputPanel(InputContext *inputContext) {
    auto &inputPanel = inputContext->inputPanel();
    auto preedit = instance_->outputFilter(inputContext, inputPanel.preedit());
    auto preeditString = preedit.toString();
    updatePreeditArea(preeditString);

    auto cursorIndex = calcPreeditCursor(preedit);
    updatePreeditCaret(cursorIndex);

    bool hasPrev = false;
    bool hasNext = false;
    auto *pageable = inputPanel.candidateList()->toPageable();
    if (pageable != nullptr) {
        hasPrev = pageable->hasPrev();
        hasNext = pageable->hasNext();
    }

    auto candidateTextList =
        makeCandidateTextList(inputContext, inputPanel.candidateList());
    auto pageIndex = inputPanel.candidateList()->cursorIndex();
    updateCandidateArea(candidateTextList, hasPrev, hasNext, pageIndex);
}

void VirtualKeyboard::startVirtualKeyboardService() {
    service_ = std::make_unique<VirtualKeyboardService>(this);
    bus_->addObjectVTable("/virtualkeyboard",
                          "org.fcitx.virtualkeyboard.service", *service_);
    bus_->requestName(
        "org.fcitx.virtualkeyboard.service",
        Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                     dbus::RequestNameFlag::Queue});
    bus_->flush();
}

void VirtualKeyboard::stopVirtualKeyboardService() {
    service_.reset();

    bus_->releaseName("org.fcitx.virtualkeyboard.service");
}

void VirtualKeyboard::setAvailable(bool available) {
    if (available_ == available) {
        return;
    }

    available_ = available;

    instance()->userInterfaceManager().updateAvailability();

    if (available) {
        startVirtualKeyboardService();
    } else {
        stopVirtualKeyboardService();
    }
}

int VirtualKeyboard::calcPreeditCursor(const Text &preedit) {
    auto preeditString = preedit.toString();
    if (preedit.cursor() < 0 ||
        static_cast<size_t>(preedit.cursor()) > preeditString.size()) {
        return -1;
    }

    auto utf8Cursor = utf8::lengthValidated(
        preeditString.begin(),
        std::next(preeditString.begin(), preedit.cursor()));
    if (utf8Cursor == utf8::INVALID_LENGTH) {
        return 0;
    }

    return utf8Cursor;
}

void VirtualKeyboard::updatePreeditCaret(int preeditCursor) {

    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "UpdatePreeditCaret");
    msg << preeditCursor;
    msg.send();
}

void VirtualKeyboard::updatePreeditArea(const std::string &preeditText) {
    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "UpdatePreeditArea");
    msg << preeditText;
    msg.send();
}

std::vector<std::string> VirtualKeyboard::makeCandidateTextList(
    InputContext *inputContext, std::shared_ptr<CandidateList> candidateList) {
    if (candidateList == nullptr || candidateList->size() == 0) {
        return {};
    }

    std::vector<std::string> candidateTextList;
    for (int index = 0; index < candidateList->size(); index++) {
        const auto &candidate = candidateList->candidate(index);
        if (candidate.isPlaceHolder()) {
            continue;
        }

        auto candidateText =
            instance_->outputFilter(inputContext, candidate.text());
        candidateTextList.push_back(candidateText.toString());
    }

    return candidateTextList;
}

void VirtualKeyboard::updateCandidateArea(
    const std::vector<std::string> &candidateTextList, bool hasPrev,
    bool hasNext, int pageIndex) {
    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "UpdateCandidateArea");
    msg << candidateTextList << hasPrev << hasNext << pageIndex;
    msg.send();
}

void VirtualKeyboard::notifyIMActivated(const std::string &uniqueName) {
    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "NotifyIMActivated");
    msg << uniqueName;
    msg.send();
}

void VirtualKeyboard::notifyIMDeactivated(const std::string &uniqueName) {
    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "NotifyIMDeactivated");
    msg << uniqueName;
    msg.send();
}
void VirtualKeyboard::notifyIMListChanged() {
    auto msg = bus_->createMethodCall("org.fcitx.virtualkeyboard.inputpanel",
                                      "/org/fcitx/virtualkeyboard/impanel",
                                      "org.fcitx.virtualkeyboard.inputpanel",
                                      "NotifyIMListChanged");
    msg.send();
}

class VirtualKeyboardFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new VirtualKeyboard(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::VirtualKeyboardFactory);
