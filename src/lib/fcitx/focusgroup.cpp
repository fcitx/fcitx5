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

#include "inputcontext.h"
#include "focusgroup_p.h"
#include "inputcontextmanager.h"

namespace fcitx {

FocusGroup::FocusGroup(InputContextManager &manager)
    : d_ptr(std::make_unique<FocusGroupPrivate>(this, manager)) {
    manager.registerFocusGroup(*this);
}

FocusGroup::~FocusGroup() {
    FCITX_D();
    d->manager.unregisterFocusGroup(*this);
}

void FocusGroup::setFocusedInputContext(InputContext *ic) {
    FCITX_D();
    if (d->focus) {
        d->focus->setHasFocus(false);
    }
    d->focus = ic;
    if (d->focus) {
        d->focus->setHasFocus(true);
    }
}

InputContext *FocusGroup::focusedInputContext() const {
    FCITX_D();
    return d->focus;
}
}
