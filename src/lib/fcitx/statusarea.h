//
// Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_STATUSAREA_H_
#define _FCITX_STATUSAREA_H_

#include <memory>
#include <vector>
#include <fcitx-utils/element.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

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
    void clearGroup(StatusGroup group);
    std::vector<Action *> actions(StatusGroup group) const;
    std::vector<Action *> allActions() const;

private:
    std::unique_ptr<StatusAreaPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StatusArea);
};
} // namespace fcitx

#endif // _FCITX_STATUSAREA_H_
