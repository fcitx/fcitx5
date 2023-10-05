/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "virtualkeyboard.h"
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
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "fcitx/userinterfacemanager.h"
#include "dbus_public.h"
#include "notificationitem_public.h"

namespace fcitx {

class VirtualKeyboardService
    : public dbus::ObjectVTable<VirtualKeyboardService> {
public:
    explicit VirtualKeyboardService(VirtualKeyboard *parent)
        : parent_(parent) {}

    ~VirtualKeyboardService() = default;

    void showVirtualKeyboard() { parent_->showVirtualKeyboardForcibly(); }

    void hideVirtualKeyboard() { parent_->hideVirtualKeyboard(); }

    void toggleVirtualKeyboard() { parent_->toggleVirtualKeyboard(); }

private:
    FCITX_OBJECT_VTABLE_METHOD(showVirtualKeyboard, "ShowVirtualKeyboard", "",
                               "");

    FCITX_OBJECT_VTABLE_METHOD(hideVirtualKeyboard, "HideVirtualKeyboard", "",
                               "");

    FCITX_OBJECT_VTABLE_METHOD(toggleVirtualKeyboard, "ToggleVirtualKeyboard",
                               "", "");

private:
    VirtualKeyboard *parent_;
};

static const char VirtualKeyboardBackendName[] =
    "org.fcitx.Fcitx5.VirtualKeyboardBackend";
static const char VirtualKeyboardBackendInterfaceName[] =
    "org.fcitx.Fcitx5.VirtualKeyboardBackend1";
static const char VirtualKeyboardName[] = "org.fcitx.Fcitx5.VirtualKeyboard";
static const char VirtualKeyboardInterfaceName[] =
    "org.fcitx.Fcitx5.VirtualKeyboard1";

class VirtualKeyboardBackend
    : public dbus::ObjectVTable<VirtualKeyboardBackend> {
public:
    VirtualKeyboardBackend(VirtualKeyboard *parent) : parent_(parent) {}

    ~VirtualKeyboardBackend() = default;

    void setVirtualKeyboardFunctionMode(uint32_t mode);

    void processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state,
                         bool isRelease, uint32_t time);

    void processVisibilityEvent(bool /*visible*/) {}

    void selectCandidate(int index) {
        auto *inputContext = parent_->instance()->mostRecentInputContext();
        if (inputContext == nullptr) {
            return;
        }

        const CandidateWord *candidate = nullptr;
        if (auto *bulkCandidateList =
                inputContext->inputPanel().candidateList()->toBulk()) {
            candidate =
                nthBulkCandidateIgnorePlaceholder(*bulkCandidateList, index);

        } else {
            candidate = nthCandidateIgnorePlaceholder(
                *inputContext->inputPanel().candidateList(), index);
        }

        if (candidate != nullptr) {
            candidate->select(inputContext);
        }
    }

    void prevPage();

    void nextPage();

private:
    PageableCandidateList *getPageableCandidateList();

    FCITX_OBJECT_VTABLE_METHOD(setVirtualKeyboardFunctionMode,
                               "SetVirtualKeyboardFunctionMode", "u", "");

    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuubu", "");

    FCITX_OBJECT_VTABLE_METHOD(processVisibilityEvent, "ProcessVisibilityEvent",
                               "b", "");

    FCITX_OBJECT_VTABLE_METHOD(selectCandidate, "SelectCandidate", "i", "");

    FCITX_OBJECT_VTABLE_METHOD(prevPage, "PrevPage", "", "");

    FCITX_OBJECT_VTABLE_METHOD(nextPage, "NextPage", "", "");

    VirtualKeyboard *parent_;
};

void VirtualKeyboardBackend::setVirtualKeyboardFunctionMode(uint32_t mode) {
    if (mode != static_cast<uint32_t>(VirtualKeyboardFunctionMode::Full) &&
        mode != static_cast<uint32_t>(VirtualKeyboardFunctionMode::Limited)) {
        throw dbus::MethodCallError("org.freedesktop.DBus.Error.InvalidArgs",
                                    "The argument mode is invalid.");
    }

    parent_->instance()->setVirtualKeyboardFunctionMode(
        static_cast<VirtualKeyboardFunctionMode>(mode));
}

void VirtualKeyboardBackend::processKeyEvent(uint32_t keyval, uint32_t keycode,
                                             uint32_t state, bool isRelease,
                                             uint32_t time) {
    auto *inputContext = parent_->instance()->mostRecentInputContext();
    if (inputContext == nullptr || !inputContext->hasFocus()) {
        // TODO: when keyboard is shown but no focused ic, send fake key via
        // display server interface.
        return;
    }

    VirtualKeyboardEvent event(inputContext, isRelease, time);
    event.setKey(Key(static_cast<KeySym>(keyval), KeyStates(state), keycode));

    auto eventConsumed = false;
    if (parent_->instance()->virtualKeyboardFunctionMode() ==
        VirtualKeyboardFunctionMode::Limited) {
        eventConsumed = inputContext->virtualKeyboardEvent(event);
    } else {
        eventConsumed = inputContext->keyEvent(*event.toKeyEvent());
    }

    if (eventConsumed) {
        return;
    }

    inputContext->forwardKey(
        Key(static_cast<KeySym>(keyval), KeyStates(state), keycode), isRelease,
        time);
}

void VirtualKeyboardBackend::prevPage() {
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

void VirtualKeyboardBackend::nextPage() {
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

PageableCandidateList *VirtualKeyboardBackend::getPageableCandidateList() {
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
    entry_ = watcher_.watchService(
        VirtualKeyboardName, [this](const std::string &, const std::string &,
                                    const std::string &newOwner) {
            FCITX_INFO() << "VirtualKeyboard new owner: " << newOwner;
            setAvailable(!newOwner.empty());

            setVisible(false);
        });

    initVirtualKeyboardService();
}

VirtualKeyboard::~VirtualKeyboard() = default;

void VirtualKeyboard::suspend() {
    if (auto *sni = notificationitem()) {
        sni->call<INotificationItem::disable>();
    }

    hideVirtualKeyboard();

    eventHandlers_.clear();
    proxy_.reset();
    bus_->releaseName(VirtualKeyboardBackendName);
}

void VirtualKeyboard::resume() {
    if (auto *sni = notificationitem()) {
        sni->call<INotificationItem::enable>();
    }

    proxy_ = std::make_unique<VirtualKeyboardBackend>(this);
    bus_->addObjectVTable("/virtualkeyboard",
                          VirtualKeyboardBackendInterfaceName, *proxy_);
    bus_->requestName(
        VirtualKeyboardBackendName,
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
            const auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.origKey().states().test(KeyState::Virtual)) {
                return;
            }

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
    if (!available_) {
        return;
    }

    setVisible(true);

    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "ShowVirtualKeyboard");

    msg.send();
}

void VirtualKeyboard::hideVirtualKeyboard() {
    if (!available_) {
        return;
    }

    setVisible(false);

    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "HideVirtualKeyboard");

    msg.send();
}

void VirtualKeyboard::showVirtualKeyboardForcibly() {
    if (!available_) {
        return;
    }

    instance_->setInputMethodMode(InputMethodMode::OnScreenKeyboard);

    showVirtualKeyboard();
}

void VirtualKeyboard::toggleVirtualKeyboard() {
    if (!available_) {
        return;
    }

    if (visible_) {
        hideVirtualKeyboard();
    } else {
        showVirtualKeyboardForcibly();
    }
}

void VirtualKeyboard::updateInputPanel(InputContext *inputContext) {
    auto &inputPanel = inputContext->inputPanel();
    auto preedit = instance_->outputFilter(inputContext, inputPanel.preedit());
    auto preeditString = preedit.toString();
    updatePreeditArea(preeditString);

    auto cursorIndex = calcPreeditCursor(preedit);
    updatePreeditCaret(cursorIndex);

    updateCandidate(inputContext);
}

void VirtualKeyboard::initVirtualKeyboardService() {
    service_ = std::make_unique<VirtualKeyboardService>(this);
    bus_->addObjectVTable("/virtualkeyboard",
                          "org.fcitx.Fcitx.VirtualKeyboard1", *service_);
    bus_->flush();
}

void VirtualKeyboard::setAvailable(bool available) {
    if (available_ == available) {
        return;
    }

    available_ = available;

    instance()->userInterfaceManager().updateAvailability();
}

void VirtualKeyboard::setVisible(bool visible) {
    if (visible_ != visible) {
        visible_ = visible;
        instance_->userInterfaceManager().updateVirtualKeyboardVisibility();
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

    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "UpdatePreeditCaret");
    msg << preeditCursor;
    msg.send();
}

void VirtualKeyboard::updatePreeditArea(const std::string &preeditText) {
    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "UpdatePreeditArea");
    msg << preeditText;
    msg.send();
}

std::vector<std::string> VirtualKeyboard::makeCandidateTextList(
    InputContext *inputContext, std::shared_ptr<CandidateList> candidateList) {
    if (candidateList == nullptr || candidateList->empty()) {
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

std::vector<std::string> VirtualKeyboard::makeBulkCandidateTextList(
    InputContext *inputContext, std::shared_ptr<CandidateList> candidateList) {
    if (candidateList == nullptr || candidateList->empty()) {
        return {};
    }

    std::vector<std::string> candidateTextList;

    const auto *bulkCandidateList = candidateList->toBulk();
    auto totalSize = bulkCandidateList->totalSize();
    for (int index = 0; (totalSize < 0) || (index < totalSize); index++) {
        Text candidateText;
        try {
            const auto &candidate = bulkCandidateList->candidateFromAll(index);
            if (candidate.isPlaceHolder()) {
                continue;
            }

            candidateText = candidate.text();
        } catch (...) {
            break;
        }
        candidateText = instance_->outputFilter(inputContext, candidateText);
        candidateTextList.push_back(candidateText.toString());
    }

    return candidateTextList;
}

int VirtualKeyboard::globalCursorIndex(
    std::shared_ptr<CandidateList> candidateList) const {
    auto *commonCandidateList =
        dynamic_cast<CommonCandidateList *>(candidateList.get());
    if (commonCandidateList == nullptr) {
        return -1;
    }

    return commonCandidateList->globalCursorIndex();
}

void VirtualKeyboard::updateCandidateArea(
    const std::vector<std::string> &candidateTextList, bool hasPrev,
    bool hasNext, int pageIndex, int globalCursorIndex) {
    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "UpdateCandidateArea");
    msg << candidateTextList << hasPrev << hasNext << pageIndex
        << globalCursorIndex;
    msg.send();
}

void VirtualKeyboard::updateCandidate(InputContext *inputContext) {
    auto &inputPanel = inputContext->inputPanel();

    if (inputPanel.candidateList() == nullptr ||
        inputPanel.candidateList()->empty()) {
        updateCandidateArea({}, false, false, -1, -1);
        return;
    }

    if (inputPanel.candidateList()->toBulk()) {
        auto candidateTextList =
            makeBulkCandidateTextList(inputContext, inputPanel.candidateList());
        updateCandidateArea(candidateTextList, false, false, -1,
                            globalCursorIndex(inputPanel.candidateList()));
    } else {
        auto candidateTextList =
            makeCandidateTextList(inputContext, inputPanel.candidateList());
        bool hasPrev = false, hasNext = false;
        if (auto *pageable = inputPanel.candidateList()->toPageable()) {
            hasPrev = pageable->hasPrev();
            hasNext = pageable->hasNext();
        }
        updateCandidateArea(candidateTextList, hasPrev, hasNext, -1,
                            inputPanel.candidateList()->cursorIndex());
    }
}

void VirtualKeyboard::notifyIMActivated(const std::string &uniqueName) {
    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "NotifyIMActivated");
    msg << uniqueName;
    msg.send();
}

void VirtualKeyboard::notifyIMDeactivated(const std::string &uniqueName) {
    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "NotifyIMDeactivated");
    msg << uniqueName;
    msg.send();
}
void VirtualKeyboard::notifyIMListChanged() {
    auto msg = bus_->createMethodCall(
        VirtualKeyboardName, "/org/fcitx/virtualkeyboard/impanel",
        VirtualKeyboardInterfaceName, "NotifyIMListChanged");
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
