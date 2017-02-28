/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals.h>
#include <fcitx/element.h>
#include <memory>

namespace fcitx {
class ActionPrivate;
class Menu;
class UserInterfaceManager;

class FCITXCORE_EXPORT Action : public Element {
    friend class UserInterfaceManager;

public:
    Action();
    virtual ~Action();

    bool isSeparator() const;
    Action &setSeparator(bool separator);

    const std::string &name() const;
    bool registerAction(const std::string &name, UserInterfaceManager *uiManager);

    const std::string &text() const;
    Action &setText(const std::string &text);
    const std::string &icon() const;
    Action &setIcon(const std::string &icon);

    bool isCheckable() const;
    Action &setCheckable(bool checkable);

    bool isChecked() const;
    Action &setChecked(bool checked);

    bool isEnabled() const;
    Action &setEnabled(bool enabled);

    void setMenu(Menu *menu);
    Menu *menu();

    void activate();

    FCITX_DECLARE_SIGNAL(Action, Activated, void());
    FCITX_DECLARE_SIGNAL(Action, Update, void());

private:
    void setName(const std::string &name);

    std::unique_ptr<ActionPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Action);
};
}

#endif // _FCITX_ACTION_H_
