/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INSTANCE_P_H_
#define _FCITX_INSTANCE_P_H_

#include <memory>
#include <string>
#include <unordered_map>
#include "fcitx-utils/event.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/misc.h"
#include "config.h"
#include "inputcontextproperty.h"
#include "inputmethodmanager.h"
#include "instance.h"
#include "userinterfacemanager.h"

#ifdef ENABLE_KEYBOARD
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>
#endif

namespace fcitx {

class CheckInputMethodChanged;

struct InputState : public InputContextProperty {
    InputState(InstancePrivate *d, InputContext *ic);

    bool needCopy() const override { return true; }
    void copyTo(InputContextProperty *other) override;
    void reset();
    void showInputMethodInformation(const std::string &name);
    void hideInputMethodInfo();

#ifdef ENABLE_KEYBOARD
    xkb_state *customXkbState(bool refresh = false);
    void resetXkbState();

    auto xkbComposeState() { return xkbComposeState_.get(); }
    void setModsAllReleased() { modsAllReleased_ = true; }
    bool isModsAllReleased() const { return modsAllReleased_; }
#endif

    bool isActive() const { return active_; }
    void setActive(bool active);
    void setLocalIM(const std::string &localIM);

    CheckInputMethodChanged *imChanged_ = nullptr;
    int keyReleased_ = -1;
    Key lastKeyPressed_;
    bool totallyReleased_ = true;
    bool firstTrigger_ = false;
    size_t pendingGroupIndex_ = 0;

    std::string lastIM_;

    bool lastIMChangeIsAltTrigger_ = false;

    std::string overrideDeactivateIM_;

    std::string localIM_;

private:
    InstancePrivate *d_ptr;
    InputContext *ic_;

#ifdef ENABLE_KEYBOARD
    UniqueCPtr<xkb_compose_state, xkb_compose_state_unref> xkbComposeState_;
    UniqueCPtr<xkb_state, xkb_state_unref> xkbState_;
    bool modsAllReleased_ = false;
    std::string lastXkbLayout_;
#endif

    std::unique_ptr<EventSourceTime> imInfoTimer_;
    std::string lastInfo_;

    bool active_;
};

class CheckInputMethodChanged {
public:
    CheckInputMethodChanged(InputContext *ic, InstancePrivate *instance);
    ~CheckInputMethodChanged();

    void setReason(InputMethodSwitchedReason reason) { reason_ = reason; }
    void ignore() { ignore_ = true; }

private:
    Instance *instance_;
    InstancePrivate *instancePrivate_;
    TrackableObjectReference<InputContext> ic_;
    std::string inputMethod_;
    InputMethodSwitchedReason reason_;
    bool ignore_ = false;
};

struct InstanceArgument {
    InstanceArgument() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(InstanceArgument)

    void parseOption(int argc, char *argv[]);
    static void printVersion() {
        std::cout << FCITX_VERSION_STRING << std::endl;
    }
    void printUsage();

    int overrideDelay = -1;
    bool tryReplace = false;
    bool quietQuit = false;
    bool runAsDaemon = false;
    bool exitWhenMainDisplayDisconnected = true;
    std::string uiName;
    std::vector<std::string> enableList;
    std::vector<std::string> disableList;
    std::string argv0;
};

class InstancePrivate : public QPtrHolder<Instance> {
#ifdef ANDROID
    static constexpr bool isAndroid = true;
#else
    static constexpr bool isAndroid = false;
#endif
public:
    InstancePrivate(Instance *q);

    std::unique_ptr<HandlerTableEntry<EventHandler>>
    watchEvent(EventType type, EventWatcherPhase phase, EventHandler callback);

#ifdef ENABLE_KEYBOARD
    xkb_keymap *keymap(const std::string &display, const std::string &layout,
                       const std::string &variant);
#endif

    std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>>
    overrideAddons();

    void buildDefaultGroup();

    void showInputMethodInformation(InputContext *ic);

    bool canActivate(InputContext *ic);

    bool canDeactivate(InputContext *ic);

    void navigateGroup(InputContext *ic, bool forward);

    void acceptGroupChange(InputContext *ic);

    InstanceArgument arg_;

    int signalPipe_ = -1;
    bool exit_ = false;
    bool running_ = false;
    InputMethodMode inputMethodMode_ = isAndroid
                                           ? InputMethodMode::OnScreenKeyboard
                                           : InputMethodMode::PhysicalKeyboard;

    EventLoop eventLoop_;
    std::unique_ptr<EventSourceIO> signalPipeEvent_;
    std::unique_ptr<EventSource> preloadInputMethodEvent_;
    std::unique_ptr<EventSource> exitEvent_;
    InputContextManager icManager_;
    AddonManager addonManager_;
    InputMethodManager imManager_{&this->addonManager_};
    UserInterfaceManager uiManager_{&this->addonManager_};
    GlobalConfig globalConfig_;
    std::unordered_map<EventType,
                       std::unordered_map<EventWatcherPhase,
                                          HandlerTable<EventHandler>, EnumHash>,
                       EnumHash>
        eventHandlers_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventWatchers_;
    std::unique_ptr<EventSource> uiUpdateEvent_;

    uint64_t idleStartTimestamp_ = now(CLOCK_MONOTONIC);
    std::unique_ptr<EventSourceTime> periodicalSave_;

    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, CommitFilter);
    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, OutputFilter);
    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, KeyEventResult);
    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, CheckUpdate);

    FactoryFor<InputState> inputStateFactory_{
        [this](InputContext &ic) { return new InputState(this, &ic); }};

#ifdef ENABLE_KEYBOARD
    UniqueCPtr<xkb_context, xkb_context_unref> xkbContext_;
    UniqueCPtr<xkb_compose_table, xkb_compose_table_unref> xkbComposeTable_;
#endif

    std::vector<ScopedConnection> connections_;
    std::unique_ptr<EventSourceTime> imGroupInfoTimer_;
    std::unique_ptr<EventSourceTime> focusInImInfoTimer_;

#ifdef ENABLE_KEYBOARD
    std::unordered_map<
        std::string, std::unordered_map<
                         std::string, UniqueCPtr<xkb_keymap, xkb_keymap_unref>>>
        keymapCache_;
#endif
    std::unordered_map<std::string, std::tuple<uint32_t, uint32_t, uint32_t>>
        stateMask_;
    std::unordered_map<std::string,
                       std::tuple<std::string, std::string, std::string>>
        xkbParams_;

    bool restart_ = false;

    AddonInstance *notifications_ = nullptr;

    std::string lastGroup_;

    const bool inFlatpak_ = fs::isreg("/.flatpak-info");
};

} // namespace fcitx

#endif
