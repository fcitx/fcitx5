/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ACTION_H_
#define _FCITX_ACTION_H_

#include <memory>
#include <fcitx-utils/element.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

namespace fcitx {
class ActionPrivate;
class SimpleActionPrivate;
class Menu;
class UserInterfaceManager;
class InputContext;

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Action class

/**
 * The Action class provides an abstraction for user commands that can be added
 * to user interfaces.
 */
class FCITXCORE_EXPORT Action : public Element {
    friend class UserInterfaceManager;
    friend class UserInterfaceManagerPrivate;

public:
    Action();
    virtual ~Action();

    /**
     * Whether the action is a separator action.
     */
    bool isSeparator() const;

    /**
     * Set whether this action is a separator.
     *
     * How separator is displayed depends on the user interface implementation.
     * The separator may be not displayed at all.
     */
    Action &setSeparator(bool separator);

    /**
     * Whether the action is a checkable action.
     */
    bool isCheckable() const;
    /**
     *  Set whether this action is a checkable action.
     *
     * This property mainly affect how this action is presented. Usually, it
     * will be displayed as a radio button.
     */
    Action &setCheckable(bool checkable);

    /**
     * The action name when this action is registered.
     */
    const std::string &name() const;

    /**
     * Register an action to UserInterfaceManager.
     *
     * All action should have a unique name, but it is suggested that the action
     * using a human readable string for its name. In order to make addons work
     * together, other addon may use this name to lookup a certain action.
     */
    bool registerAction(const std::string &name,
                        UserInterfaceManager *uiManager);

    /**
     * Short description for this action of given input context.
     */
    virtual std::string shortText(InputContext *) const = 0;

    /**
     * Icon name of this action of given input context.
     */
    virtual std::string icon(InputContext *) const = 0;

    /**
     * Return if this action is checked.
     *
     * It is only meaningful when isCheckable is true.
     * @see setCheckable
     */
    virtual bool isChecked(InputContext *) const { return false; }

    /**
     * Return a long description for this action.
     *
     * In some UI implementation, this is not displayed at all. In some other
     * implemented, it is only displayed as tooltip.
     */
    virtual std::string longText(InputContext *) const { return {}; }

    /**
     * Set the sub menu of this action.
     *
     * Some status area implemenation only supports one level of menu, so it is
     * preferred that only one level of menu is used.
     */
    void setMenu(Menu *menu);

    /**
     * Return the sub menu of this action.
     */
    Menu *menu();

    /**
     * Return the unique integer id of action.
     *
     * This id is generated when the action is registered. If it is not
     * registered, the value will be 0.
     */
    int id();

    /**
     * Activate this action.
     *
     * This function may be triggered by user mouse click of the given action.
     */
    virtual void activate(InputContext *) {}

    /**
     * Notify that this action is required to be updated of given input context.
     *
     * @param ic the input context that need to update this action.
     */
    void update(InputContext *ic);

    FCITX_DECLARE_SIGNAL(Action, Update, void(InputContext *));

private:
    void setName(const std::string &name);
    void setId(int id);

    std::unique_ptr<ActionPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Action);
};

class FCITXCORE_EXPORT SimpleAction : public Action {
public:
    SimpleAction();
    ~SimpleAction();

    void setIcon(const std::string &icon);
    void setChecked(bool checked);
    void setShortText(const std::string &text);
    void setLongText(const std::string &text);

    std::string shortText(InputContext *) const override;
    std::string icon(InputContext *) const override;
    bool isChecked(InputContext *) const override;
    std::string longText(InputContext *) const override;

    void activate(fcitx::InputContext *) override;

    FCITX_DECLARE_SIGNAL(SimpleAction, Activated, void(InputContext *));

private:
    std::unique_ptr<SimpleActionPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(SimpleAction);
};
} // namespace fcitx

#endif // _FCITX_ACTION_H_
