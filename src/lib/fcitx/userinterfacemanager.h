/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_USERINTERFACEMANAGER_H_
#define _FCITX_USERINTERFACEMANAGER_H_

#include <memory>
#include <fcitx-utils/macros.h>
#include <fcitx/addonmanager.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/inputpanel.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterface.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Manager class for user interface.

namespace fcitx {

class UserInterfaceManagerPrivate;

class FCITXCORE_EXPORT UserInterfaceManager {
public:
    UserInterfaceManager(AddonManager *manager);
    virtual ~UserInterfaceManager();

    /**
     * Initialize the UI Addon
     *
     * If passed with a certain UI name, only that UI is treated as the usable
     * UI.
     * @arg ui addon name of the UI to be used.
     */
    void load(const std::string &ui = {});
    /**
     * Register an named action.
     *
     * The action should have a meaningful unique name.
     * A common use case for the name is to allow an input method to enable a
     * feature exposed by other addons.
     * @arg name name of the action.
     * @arg action action to be registered.
     * @return whether action is registered successfully.
     */
    bool registerAction(const std::string &name, Action *action);
    /**
     * Register an anonymous action.
     *
     * @arg action action to be registered.
     * @return whether action is registered successfully.
     */
    bool registerAction(Action *action);
    /**
     * Unregister the action.
     */
    void unregisterAction(Action *action);
    /**
     * Lookup an action by the name.
     *
     * Return null if action is not found.
     * @return action
     */
    Action *lookupAction(const std::string &name) const;
    /**
     * Lookup an action by id.
     *
     * Return null if action is not found.
     * @return action
     */
    Action *lookupActionById(int id) const;
    /**
     * Mark a user interface component to be updated for given input context.
     *
     * The update will not happen immediately.
     * @arg component user interface component
     * @arg inputContext Input Context
     */
    void update(UserInterfaceComponent component, InputContext *inputContext);
    /**
     * Remove all pending updates for a given input context.
     */
    void expire(InputContext *inputContext);
    void flush();
    /**
     * Invoke by user interface addon to notify if there is any avaiability
     * change.
     */
    void updateAvailability();
    /**
     * Return the current active addon ui name.
     */
    std::string currentUI() const;

    /**
     * Return if virtual keyboard is visible.
     */
    bool isVirtualKeyboardVisible() const;
    /**
     * Show the virtual keyboard.
     *
     * This is a no-op if active addon is not a virtual keyboard addon.
     */
    void showVirtualKeyboard() const;
    /**
     * Hide the virtual keyboard.
     *
     * This is a no-op if active addon is not a virtual keyboard addon.
     */
    void hideVirtualKeyboard() const;

    /**
     * Invoke by user interface addon to notify if there is any virtual keyboard
     * visibility change.
     *
     * User interface addon must call this function to make user interface
     * manager update its cached visibility.
     */
    void updateVirtualKeyboardVisibility();

private:
    std::unique_ptr<UserInterfaceManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(UserInterfaceManager);
};
} // namespace fcitx

#endif // _FCITX_USERINTERFACEMANAGER_H_
