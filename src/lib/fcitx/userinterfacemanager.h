//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_USERINTERFACEMANAGER_H_
#define _FCITX_USERINTERFACEMANAGER_H_

#include <memory>
#include <fcitx-utils/macros.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterface.h>
#include "fcitxcore_export.h"

namespace fcitx {

class UserInterfaceManagerPrivate;

class FCITXCORE_EXPORT UserInterfaceManager {
public:
    UserInterfaceManager(AddonManager *manager);
    virtual ~UserInterfaceManager();

    void load(const std::string &ui = {});
    bool registerAction(const std::string &name, Action *action);
    bool registerAction(Action *action);
    void unregisterAction(Action *action);
    Action *lookupAction(const std::string &name) const;
    Action *lookupActionById(int id) const;
    void update(UserInterfaceComponent component, InputContext *inputContext);
    void expire(InputContext *inputContext);
    void flush();
    void updateAvailability();
    std::string currentUI() const;

private:
    std::unique_ptr<UserInterfaceManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(UserInterfaceManager);
};
} // namespace fcitx

#endif // _FCITX_USERINTERFACEMANAGER_H_
