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
#include <fcitx-utils/signals.h>
#include "fcitxcore_export.h"

namespace fcitx {
class ActionPrivate;
class SimpleActionPrivate;
class Menu;
class UserInterfaceManager;
class InputContext;

class FCITXCORE_EXPORT Action : public Element {
    friend class UserInterfaceManager;
    friend class UserInterfaceManagerPrivate;

public:
    Action();
    virtual ~Action();

    bool isSeparator() const;
    Action &setSeparator(bool separator);

    bool isCheckable() const;
    Action &setCheckable(bool checkable);

    const std::string &name() const;
    bool registerAction(const std::string &name,
                        UserInterfaceManager *uiManager);

    virtual std::string shortText(InputContext *) const = 0;
    virtual std::string icon(InputContext *) const = 0;
    virtual bool isChecked(InputContext *) const { return false; }
    virtual std::string longText(InputContext *) const { return {}; }

    void setMenu(Menu *menu);
    Menu *menu();

    int id();

    virtual void activate(InputContext *) {}
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
