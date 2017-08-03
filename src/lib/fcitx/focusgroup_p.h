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
#ifndef _FCITX_FOCUSGROUP_P_H_
#define _FCITX_FOCUSGROUP_P_H_

#include "fcitx-utils/intrusivelist.h"
#include "focusgroup.h"
#include <unordered_set>

namespace fcitx {

class InputContextManager;

class FocusGroupPrivate : public QPtrHolder<FocusGroup> {
public:
    FocusGroupPrivate(FocusGroup *q, const std::string &display,
                      InputContextManager &manager)
        : QPtrHolder(q), display_(display), manager_(manager), focus_(nullptr) {
    }

    std::string display_;
    InputContextManager &manager_;
    InputContext *focus_;
    std::unordered_set<InputContext *> ics_;

    IntrusiveListNode listNode_;
};
}

#endif // _FCITX_FOCUSGROUP_P_H_
