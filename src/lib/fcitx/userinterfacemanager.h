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
#include <fcitx/inputpanel.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterface.h>
#include "fcitxcore_export.h"

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
