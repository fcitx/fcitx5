/*
 * SPDX-FileCopyrightText: 2022-2022 liulinsong <liulinsong@kylinos.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_VIRTUALKEYBOARD_VIRTUALKEYBOARD_H_
#define _FCITX_UI_VIRTUALKEYBOARD_VIRTUALKEYBOARD_H_

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"

namespace fcitx {

class CandidateList;
class VirtualKeyboardBackend;
class VirtualKeyboardService;

class VirtualKeyboard : public VirtualKeyboardUserInterface {
public:
    VirtualKeyboard(Instance *instance);
    ~VirtualKeyboard();

    Instance *instance() { return instance_; }
    void suspend() override;
    void resume() override;
    bool available() override { return available_; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;

    bool isVirtualKeyboardVisible() const override {
        return available_ && visible_;
    }

    void showVirtualKeyboard() override;
    void hideVirtualKeyboard() override;

    void showVirtualKeyboardForcibly();
    void toggleVirtualKeyboard();

    void updateInputPanel(InputContext *inputContext);

private:
    void initVirtualKeyboardService();

    void setAvailable(bool available);

    void setVisible(bool visible);

    int calcPreeditCursor(const fcitx::Text &preedit);
    void updatePreeditCaret(int preeditCursor);
    void updatePreeditArea(const std::string &preeditText);

    std::vector<std::string>
    makeCandidateTextList(InputContext *inputContext,
                          std::shared_ptr<CandidateList> candidateList);
    std::vector<std::string>
    makeBulkCandidateTextList(InputContext *inputContext,
                              std::shared_ptr<CandidateList> candidateList);
    int globalCursorIndex(std::shared_ptr<CandidateList> candidateList) const;
    void updateCandidateArea(const std::vector<std::string> &candidateTextList,
                             bool hasPrev, bool hasNext, int pageIndex,
                             int globalCursorIndex);
    void updateCandidate(InputContext *inputContext);

    void notifyIMActivated(const std::string &uniqueName);
    void notifyIMDeactivated(const std::string &uniqueName);
    void notifyIMListChanged();

    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(notificationitem, instance_->addonManager());

    Instance *instance_;
    dbus::Bus *bus_;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<VirtualKeyboardBackend> proxy_;
    std::unique_ptr<VirtualKeyboardService> service_;
    std::unique_ptr<dbus::ServiceWatcherEntry> entry_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    bool available_ = false;
    bool visible_ = false;
};
} // namespace fcitx

#endif // _FCITX_UI_VIRTUALKEYBOARD_VIRTUALKEYBOARD_H_
