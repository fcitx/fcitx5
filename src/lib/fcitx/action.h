/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_ACTION_H_
#define _FCITX_ACTION_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/element.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals.h>
#include <memory>

namespace fcitx {
class ActionPrivate;
class SimpleActionPrivate;
class Menu;
class UserInterfaceManager;
class InputContext;

class FCITXCORE_EXPORT Action : public Element {
    friend class UserInterfaceManager;

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

private:
    std::unique_ptr<SimpleActionPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(SimpleAction);
};
}

#endif // _FCITX_ACTION_H_
