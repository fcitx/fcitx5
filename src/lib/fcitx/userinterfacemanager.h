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
#ifndef _FCITX_USERINTERFACEMANAGER_H_
#define _FCITX_USERINTERFACEMANAGER_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/macros.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterface.h>
#include <memory>

namespace fcitx {

class UserInterfaceManagerPrivate;

class FCITXCORE_EXPORT UserInterfaceManager {
public:
    UserInterfaceManager();
    virtual ~UserInterfaceManager();

    void load(AddonManager *manager, const std::string &ui = {});
    bool registerAction(const std::string &name, Action *action);
    void unregisterAction(Action *action);
    Action *lookupAction(const std::string &name);
    void update(UserInterfaceComponent component, InputContext *inputContext);
    void expire(InputContext *inputContext);
    void flush();

private:
    std::unique_ptr<UserInterfaceManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(UserInterfaceManager);
};
}

#endif // _FCITX_USERINTERFACEMANAGER_H_
