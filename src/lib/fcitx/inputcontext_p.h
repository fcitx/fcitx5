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
#ifndef _FCITX_INPUTCONTEXT_P_H_
#define _FCITX_INPUTCONTEXT_P_H_

#include "fcitx-utils/intrusivelist.h"
#include "inputcontext.h"

namespace fcitx {

class InputContextPrivate {
public:
    InputContextPrivate(InputContext *q, InputContextManager &manager_)
        : q_ptr(q), manager(manager_), group(nullptr), hasFocus(false) {}
    InputContext *q_ptr;
    InputContextManager &manager;
    FocusGroup *group;
    bool hasFocus;
    CapabilityFlags capabilityFlags;
    SurroundingText surroundingText;

    IntrusiveListNode listNode;

    FCITX_DECLARE_PUBLIC(InputContext);
};
}

#endif // _FCITX_INPUTCONTEXT_P_H_
