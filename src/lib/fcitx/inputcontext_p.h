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

#include <fcitx-utils/intrusivelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/instance.h>
#include <unordered_map>
#include <uuid/uuid.h>

namespace fcitx {

class InputContextPrivate {
public:
    InputContextPrivate(InputContext *q, InputContextManager &manager, const std::string &program)
        : q_ptr(q), manager_(manager), group_(nullptr), inputPanel_(q), hasFocus_(false), program_(program) {
        uuid_generate(uuid_.data());
    }

    template <typename E>
    bool postEvent(E &&event) {
        if (auto instance = manager_.instance()) {
            return instance->postEvent(event);
        }
        return false;
    }

    template <typename E, typename... Args>
    bool emplaceEvent(Args &&... args) {
        if (auto instance = manager_.instance()) {
            return instance->postEvent(E(std::forward<Args>(args)...));
        }
        return false;
    }

    InputContext *q_ptr;
    InputContextManager &manager_;
    FocusGroup *group_;
    InputPanel inputPanel_;
    bool hasFocus_;
    std::string program_;
    CapabilityFlags capabilityFlags_;
    SurroundingText surroundingText_;
    Rect cursorRect_;

    IntrusiveListNode listNode_;
    ICUUID uuid_;
    std::unordered_map<std::string, std::unique_ptr<InputContextProperty>> properties_;

    FCITX_DECLARE_PUBLIC(InputContext);
};
}

#endif // _FCITX_INPUTCONTEXT_P_H_
