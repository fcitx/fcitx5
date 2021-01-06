/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INSTANCE_H_
#define _FCITX_INSTANCE_H_

#include <memory>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>
#include <fcitx/event.h>
#include <fcitx/globalconfig.h>
#include <fcitx/text.h>
#include "fcitxcore_export.h"

#define FCITX_INVALID_COMPOSE_RESULT 0xffffffff

namespace fcitx {

class InputContext;
class KeyEvent;
class InstancePrivate;
class EventLoop;
class AddonManager;
class InputContextManager;
class InputMethodManager;
class InputMethodEngine;
class InputMethodEntry;
class UserInterfaceManager;
class GlobalConfig;
class FocusGroup;
typedef std::function<void(Event &event)> EventHandler;
enum class EventWatcherPhase {
    PreInputMethod,
    InputMethod,
    PostInputMethod,
    ReservedFirst,
    ReservedLast,
    Default = PostInputMethod
};

struct FCITXCORE_EXPORT InstanceQuietQuit : public std::exception {};

class FCITXCORE_EXPORT Instance : public ConnectableObject {
public:
    Instance(int argc, char *argv[]);
    ~Instance();

    bool initialized() const { return !!d_ptr; }

    void setSignalPipe(int fd);
    int exec();
    bool willTryReplace() const;
    bool exitWhenMainDisplayDisconnected() const;
    bool exiting() const;

    EventLoop &eventLoop();
    AddonManager &addonManager();
    InputContextManager &inputContextManager();
    UserInterfaceManager &userInterfaceManager();
    GlobalConfig &globalConfig();

    bool postEvent(Event &event);
    bool postEvent(Event &&event) { return postEvent(event); }

    FCITX_NODISCARD std::unique_ptr<HandlerTableEntry<EventHandler>>
    watchEvent(EventType type, EventWatcherPhase phase, EventHandler callback);

    std::string inputMethod(InputContext *ic);
    const InputMethodEntry *inputMethodEntry(InputContext *ic);
    InputMethodEngine *inputMethodEngine(InputContext *ic);
    InputMethodEngine *inputMethodEngine(const std::string &name);

    /**
     * Handle current XCompose state.
     *
     * @param ic input context.
     * @param keysym key symbol.
     *
     * @return unicode
     *
     * @see processComposeString
     */
    FCITXCORE_DEPRECATED uint32_t processCompose(InputContext *ic,
                                                 KeySym keysym);

    /**
     * Handle current XCompose state.
     *
     * @param ic input context.
     * @param keysym key symbol.
     *
     * @return the composed string, if it returns nullopt, it means compose is
     * invalid.
     *
     * @see processComposeString
     * @since 5.0.4
     */
    std::optional<std::string> processComposeString(InputContext *ic,
                                                    KeySym keysym);

    /// Reset the compose state.
    void resetCompose(InputContext *inputContext);

    std::string commitFilter(InputContext *inputContext,
                             const std::string &orig);
    Text outputFilter(InputContext *inputContext, const Text &orig);

    FCITX_DECLARE_SIGNAL(Instance, CommitFilter,
                         void(InputContext *inputContext, std::string &orig));
    FCITX_DECLARE_SIGNAL(Instance, OutputFilter,
                         void(InputContext *inputContext, Text &orig));
    FCITX_DECLARE_SIGNAL(Instance, KeyEventResult,
                         void(const KeyEvent &keyEvent));

    /// Return a focused input context.
    InputContext *lastFocusedInputContext();
    /// Return the most recent focused input context. If there isn't such ic,
    /// return the last unfocused input context.
    InputContext *mostRecentInputContext();
    InputMethodManager &inputMethodManager();
    const InputMethodManager &inputMethodManager() const;
    void flushUI();

    // controller
    void exit();
    void restart();
    void configure();
    void configureAddon(const std::string &addon);
    void configureInputMethod(const std::string &imName);
    std::string currentUI();
    std::string addonForInputMethod(const std::string &imName);
    void activate();
    void deactivate();
    void toggle();
    void resetInputMethodList();
    int state();
    /// Reload global config.
    void reloadConfig();
    /// Reload certain addon config.
    void reloadAddonConfig(const std::string &addonName);
    std::string currentInputMethod();
    void setCurrentInputMethod(const std::string &imName);
    void setCurrentInputMethod(InputContext *ic, const std::string &imName,
                               bool local);
    bool enumerateGroup(bool forward);
    void enumerate(bool forward);

    FocusGroup *defaultFocusGroup(const std::string &displayHint = {});

    void setXkbParameters(const std::string &display, const std::string &rule,
                          const std::string &model, const std::string &options);
    void updateXkbStateMask(const std::string &display, uint32_t depressed_mods,
                            uint32_t latched_mods, uint32_t locked_mods);
    void showInputMethodInformation(InputContext *ic);

    static const char *version();

private:
    void initialize();
    void handleSignal();
    void save();

    bool canTrigger() const;
    bool canAltTrigger(InputContext *ic) const;
    bool canEnumerate(InputContext *ic) const;
    bool canChangeGroup() const;
    bool trigger(InputContext *ic, bool totallyReleased);
    bool altTrigger(InputContext *ic);
    bool activate(InputContext *ic);
    bool deactivate(InputContext *ic);
    bool enumerate(InputContext *ic, bool forward);
    bool toggle(InputContext *ic, InputMethodSwitchedReason reason =
                                      InputMethodSwitchedReason::Trigger);

    void activateInputMethod(InputContextEvent &event);
    void deactivateInputMethod(InputContextEvent &event);

    std::unique_ptr<InstancePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Instance);
};
}; // namespace fcitx

#endif // _FCITX_INSTANCE_H_
