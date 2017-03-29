/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_UI_SIMPLE_SIMPLEUI_H_
#define _FCITX_UI_SIMPLE_SIMPLEUI_H_

#include "fcitx/instance.h"
#include "fcitx/userinterface.h"

namespace fcitx {

class SimpleUI : public UserInterface {
public:
    SimpleUI(Instance *instance);
    ~SimpleUI();

    Instance *instance() { return instance_; }
    void suspend() override;
    void resume() override;
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;

private:
    void printStatusArea(InputContext *inputContext);
    void printInputPanel(InputContext *inputContext);

    Instance *instance_;
};
}

#endif // _FCITX_UI_SIMPLE_SIMPLEUI_H_
