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

/**
 * The event handling phase of event pipeline.
 */
enum class EventWatcherPhase {
    /**
     * Handler executed before input method.
     *
     * Useful for addons that want to implement an independent mode.
     *
     * A common workflow of such addon is:
     * 1. Check a hotkey in PostInputMethod phase to trigger the mode
     * 2. Handle all the key event in PreInputMethod phase just like regular
     * input method.
     */
    PreInputMethod,
    /**
     * Handlers to be executed right after input method.
     *
     * The input method keyEvent is registered with an internal handler. So all
     * the new handler in this phase will still executed after input method.
     */
    InputMethod,
    /**
     * Handlers to be executed after input method.
     *
     * common use case is when you want to implement a key that triggers a
     * standalone action.
     */
    PostInputMethod,
    /// Internal phase to be executed first
    ReservedFirst,
    /// Internal phase to be executed last
    ReservedLast,
    Default = PostInputMethod
};

struct FCITXCORE_EXPORT InstanceQuietQuit : public std::exception {};

/**
 * An instance represents a standalone Fcitx instance. Usually there is only one
 * of such object.
 *
 * Fcitx Instance provides the access to all the addons and sub components. It
 * also provides a event pipeline for handling input method related event.
 */
class FCITXCORE_EXPORT Instance : public ConnectableObject {
public:
    /**
     * A main function like construct to be used to create Fcitx Instance.
     *
     * For more details, see --help of fcitx5 command.
     *
     * @param argc number of argument
     * @param argv command line arguments
     */
    Instance(int argc, char *argv[]);

    ~Instance();

    bool initialized() const { return !!d_ptr; }

    /**
     * Set the pipe forwarding unix signal information.
     *
     * Fcitx Instance is running within its own thread, usually main thread. In
     * order to make it handle signal correctly in a thread-safe way, it is
     * possible to set a file descriptor that write the signal number received
     * by the signal handler. Usually this is done through a self-pipe. This is
     * already handled by Fcitx default server implementation, normal addon user
     * should not touch this. The common usecase is when you want to embed Fcitx
     * into your own program.
     *
     * @param fd file descriptor
     */
    void setSignalPipe(int fd);

    /**
     * Start the event loop of Fcitx.
     *
     * @return return value that can be used as main function return code.
     */
    int exec();

    /**
     * Check whether command line specify if it will replace an existing fcitx
     * server.
     *
     * This function is only useful if your addon provides a way to replace
     * existing fcitx server. Basically it is checking whether -r is passed to
     * fcitx command line.
     *
     * @return whether to replace existing fcitx server. Default value is false.
     */
    bool willTryReplace() const;

    /**
     * Check whether command line specify whether to keep fcitx running.
     *
     * There could be multiple display server, such as X/Wayland/etc. Fcitx
     * usually will exit when the connection is closed. Command line -k can
     * override this behavior and keep Fcitx running.
     *
     * @return whether to exit after main display is disconnected.
     */
    bool exitWhenMainDisplayDisconnected() const;

    /**
     * Check whether fcitx is in exiting process.
     *
     * @return
     */
    bool exiting() const;

    /// Get the fcitx event loop.
    EventLoop &eventLoop();

    /// Get the addon manager.
    AddonManager &addonManager();

    /// Get the input context manager
    InputContextManager &inputContextManager();

    /// Get the user interface manager
    UserInterfaceManager &userInterfaceManager();

    /// Get the input method manager
    InputMethodManager &inputMethodManager();

    /// Get the input method manager
    const InputMethodManager &inputMethodManager() const;

    /// Get the global config.
    GlobalConfig &globalConfig();

    // TODO: Merge this when we can break API.
    bool postEvent(Event &event);
    bool postEvent(Event &&event) { return postEvent(event); }

    /**
     * Put a event to the event pipe line.
     *
     * @param event Input method event
     * @return return the value of event.accepted()
     */
    bool postEvent(Event &event) const;
    bool postEvent(Event &&event) const { return postEvent(event); }

    /**
     * Add a callback to for certain event type.
     *
     * @param type event type
     * @param phase the stage that callback will be executed.
     * @param callback callback function.
     * @return Handle to the callback, the callback will be removed when it is
     * deleted.
     */
    FCITX_NODISCARD std::unique_ptr<HandlerTableEntry<EventHandler>>
    watchEvent(EventType type, EventWatcherPhase phase, EventHandler callback);

    /// Return the unique name of input method for given input context.
    std::string inputMethod(InputContext *ic);

    /// Return the input method entry for given input context.
    const InputMethodEntry *inputMethodEntry(InputContext *ic);

    /// Return the input method engine object for given input context.
    InputMethodEngine *inputMethodEngine(InputContext *ic);

    /// Return the input method engine object for given unique input method
    /// name.
    InputMethodEngine *inputMethodEngine(const std::string &name);

    /**
     * Return the input method icon for input context.
     *
     * It will fallback to input-keyboard by default if no input method is
     * available.
     *
     * @param ic input context
     * @return icon name.
     *
     * @see InputMethodEngine::subModeIcon
     */
    std::string inputMethodIcon(InputContext *ic);

    /**
     * Return the input method label for input context.
     *
     * @param ic input context
     * @return label.
     *
     * @see InputMethodEngine::subModeLabel
     * @since 5.0.11
     */
    std::string inputMethodLabel(InputContext *ic);

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

    /// Check whether input contex is composing or not.
    bool isComposing(InputContext *inputContext);

    /**
     * Update the commit string to frontend
     *
     * This function should be not be used directly since it is already used
     * internally by InputContext::commitString.
     *
     * @param inputContext input context
     * @param orig original string
     * @return the updated string.
     * @see InputContext::commitString
     */
    std::string commitFilter(InputContext *inputContext,
                             const std::string &orig);
    /**
     * Update the string that will be displayed in user interface.
     *
     * This function should only be used by frontend for client preedit, or user
     * interface, for the other field in input panel.
     *
     * @see InputPanel
     *
     * @param inputContext input context
     * @param orig orig text
     * @return fcitx::Text
     */
    Text outputFilter(InputContext *inputContext, const Text &orig);

    FCITX_DECLARE_SIGNAL(Instance, CommitFilter,
                         void(InputContext *inputContext, std::string &orig));
    FCITX_DECLARE_SIGNAL(Instance, OutputFilter,
                         void(InputContext *inputContext, Text &orig));
    FCITX_DECLARE_SIGNAL(Instance, KeyEventResult,
                         void(const KeyEvent &keyEvent));
    /**
     * \deprecated
     */
    FCITX_DECLARE_SIGNAL(Instance, CheckUpdate, bool());

    /// Return a focused input context.
    InputContext *lastFocusedInputContext();
    /// Return the most recent focused input context. If there isn't such ic,
    /// return the last unfocused input context.
    InputContext *mostRecentInputContext();

    /// All user interface update is batched internally. This function will
    /// flush all the batched UI update immediately.
    void flushUI();

    // controller functions.

    /// Exit the fcitx event loop
    void exit();

    /// Restart fcitx instance, this should only be used within a regular Fcitx
    /// server, not within embedded mode.
    void restart();

    /// Launch configtool
    void configure();

    FCITXCORE_DEPRECATED void configureAddon(const std::string &addon);
    FCITXCORE_DEPRECATED void configureInputMethod(const std::string &imName);

    /// Return the name of current user interface addon.
    std::string currentUI();

    /// Return the addon name of given input method.
    std::string addonForInputMethod(const std::string &imName);

    // Following functions are operations against lastFocusedInputContext

    /// Activate last focused input context. (Switch to the active input method)
    void activate();

    /// Deactivate last focused input context. (Switch to the first input
    /// method)
    void deactivate();

    /// Toggle between the first input method and active input method.
    void toggle();

    /// Reset the input method configuration and recreate based on system
    /// language.
    void resetInputMethodList();

    /// Return a fcitx5-remote compatible value for the state.
    int state();

    /// Reload global config.
    void reloadConfig();
    /// Reload certain addon config.
    void reloadAddonConfig(const std::string &addonName);
    /// Load newly installed input methods and addons.
    void refresh();

    /// Return the current input method of last focused input context.
    std::string currentInputMethod();

    /// Set the input method of last focused input context.
    void setCurrentInputMethod(const std::string &imName);

    /**
     * Set the input method of given input context.
     *
     * The input method need to be within the current group. Local parameter can
     * be used to set the input method only for this input context.
     *
     * @param ic input context
     * @param imName unique name of a input method
     * @param local
     */
    void setCurrentInputMethod(InputContext *ic, const std::string &imName,
                               bool local);

    /// Enumerate input method group
    bool enumerateGroup(bool forward);

    /// Enumerate input method with in current group
    void enumerate(bool forward);

    /**
     * Get the default focus group with given display hint.
     *
     * This function is used by frontend to assign a focus group from an unknown
     * display server.
     *
     * @param displayHint Display server hint, it can something like be x11: /
     * wayland:
     * @return focus group
     */
    FocusGroup *defaultFocusGroup(const std::string &displayHint = {});

    /**
     * Set xkb RLVMO tuple for given display
     *
     * @param display display name
     * @param rule xkb rule name
     * @param model xkb model name
     * @param options xkb option
     */
    void setXkbParameters(const std::string &display, const std::string &rule,
                          const std::string &model, const std::string &options);

    /// Update xkb state mask for given display
    void updateXkbStateMask(const std::string &display, uint32_t depressed_mods,
                            uint32_t latched_mods, uint32_t locked_mods);

    /// Clear xkb state mask for given display
    void clearXkbStateMask(const std::string &display);

    /**
     * Show a small popup with input popup window with current input method
     * information.
     *
     * This is useful for input method that has multiple sub modes. It can be
     * called with switching sub modes within the input method.
     *
     * @param ic input context.
     */
    void showInputMethodInformation(InputContext *ic);

    /**
     * Check if need to invoke Instance::refresh.
     *
     * @return need update
     * @see Instance::refresh
     */
    bool checkUpdate() const;

    /// Return the version string of Fcitx.
    static const char *version();

    /**
     * Save everything including input method profile and addon data.
     *
     * It also reset the idle save timer.
     *
     * @since 5.0.14
     */
    void save();

    /**
     * Initialize fcitx.
     *
     * This is only intended to be used if you want to handle event loop on your
     * own. Otherwise you should use Instance::exec().
     *
     * @since 5.0.14
     */
    void initialize();

    /**
     * Let other know that event loop is already running.
     *
     * This should only be used if you run event loop on your own.
     * @since 5.0.14
     */
    void setRunning(bool running);

    /**
     * Whether event loop is started and still running.
     * @since 5.0.14
     */
    bool isRunning() const;

    /**
     * Whether restart is requested.
     * @since 5.0.18
     */
    bool isRestartRequested() const;

protected:
    // For testing purpose
    InstancePrivate *privateData();

private:
    void handleSignal();

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
