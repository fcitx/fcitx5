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
#ifndef _FCITX_STATUSAREA_H_
#define _FCITX_STATUSAREA_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/macros.h>
#include <fcitx/element.h>
#include <memory>
#include <vector>

namespace fcitx {

class Action;
class StatusAreaPrivate;
class InputContext;

enum class StatusGroup {
    BeforeInputMethod,
    InputMethod,
    AfterInputMethod,
};

class FCITXCORE_EXPORT StatusArea : public Element {
public:
    StatusArea(InputContext *ic);
    ~StatusArea();

    void addAction(StatusGroup group, Action *action);
    void removeAction(Action *action);
    void clear();
    std::vector<Action *> actions();

private:
    std::unique_ptr<StatusAreaPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StatusArea);
};
}

#endif // _FCITX_STATUSAREA_H_
