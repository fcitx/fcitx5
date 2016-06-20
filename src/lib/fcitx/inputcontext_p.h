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
#include "inputcontextproperty.h"
#include "inputcontextmanager.h"
#include "instance.h"
#include <uuid/uuid.h>
#include <unordered_map>

namespace fcitx {

class InputContextPrivate {
public:
    InputContextPrivate(InputContext *q, InputContextManager &manager_, const std::string &program_)
        : q_ptr(q), manager(manager_), group(nullptr), hasFocus(false), program(program_) {
        uuid_generate(uuid.data());
    }

    template <typename E>
    bool postEvent(E &&event) {
        if (auto instance = manager.instance()) {
            return instance->postEvent(event);
        }
        return false;
    }

    template <typename E, typename... Args>
    bool emplaceEvent(Args &&... args) {
        if (auto instance = manager.instance()) {
            return instance->postEvent(E(std::forward<Args>(args)...));
        }
        return false;
    }

    InputContext *q_ptr;
    InputContextManager &manager;
    FocusGroup *group;
    bool hasFocus;
    std::string program;
    CapabilityFlags capabilityFlags;
    SurroundingText surroundingText;
    Text preedit;
    Text clientPreedit;
    Rect cursorRect;

    IntrusiveListNode listNode;
    ICUUID uuid;
    std::unordered_map<int, std::unique_ptr<InputContextProperty>> properties;

    FCITX_DECLARE_PUBLIC(InputContext);
};
}

#endif // _FCITX_INPUTCONTEXT_P_H_
