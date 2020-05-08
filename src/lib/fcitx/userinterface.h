/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_USERINTERFACE_H_
#define _FCITX_USERINTERFACE_H_

#include <memory>
#include <fcitx-utils/macros.h>
#include <fcitx/addoninstance.h>

namespace fcitx {

class InputContext;

enum class UserInterfaceComponent {
    InputPanel,
    StatusArea,
};

class FCITXCORE_EXPORT UserInterface : public AddonInstance {
public:
    virtual ~UserInterface();

    virtual void update(UserInterfaceComponent component,
                        InputContext *inputContext) = 0;
    virtual bool available() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
};
}; // namespace fcitx

#endif // _FCITX_USERINTERFACE_H_
