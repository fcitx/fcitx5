/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MENU_H_
#define _FCITX_MENU_H_

#include <memory>
#include <fcitx-utils/element.h>
#include <fcitx-utils/macros.h>
#include <fcitx/action.h>
#include "fcitxcore_export.h"

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Class for menu in UI.

namespace fcitx {

class MenuPrivate;

/**
 * Menu that contains a list of actions.
 *
 * The user interface may only support one-level of menu, so you
 * may want to avoid multiple level of menu if possible.
 */
class FCITXCORE_EXPORT Menu : public Element {
public:
    friend class Action;
    Menu();
    virtual ~Menu();

    void addAction(Action *action);
    void removeAction(Action *action);
    void insertAction(Action *before, Action *action);
    std::vector<Action *> actions();

    FCITX_DECLARE_SIGNAL(Menu, Update, void());

private:
    std::unique_ptr<MenuPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Menu);
};
} // namespace fcitx

#endif // _FCITX_MENU_H_
