/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_USERINTERFACE_H_
#define _FCITX_USERINTERFACE_H_

#include <fcitx/addoninstance.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Base class for User Interface addon.

namespace fcitx {

class InputContext;

enum class UserInterfaceComponent {
    /**
     * Input Panel component
     * @see InputPanel
     */
    InputPanel,
    /**
     * Status Area component
     * @see StatusArea
     */
    StatusArea,
};

/**
 * @brief ...
 *
 */
class FCITXCORE_EXPORT UserInterface : public AddonInstance {
public:
    virtual ~UserInterface();

    virtual void update(UserInterfaceComponent component,
                        InputContext *inputContext) = 0;
    virtual bool available() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
};

class FCITXCORE_EXPORT VirtualKeyboardUserInterface : public UserInterface {
public:
    ~VirtualKeyboardUserInterface() override;

    virtual bool isVirtualKeyboardVisible() const = 0;

    virtual void showVirtualKeyboard() = 0;

    virtual void hideVirtualKeyboard() = 0;
};
}; // namespace fcitx

#endif // _FCITX_USERINTERFACE_H_
