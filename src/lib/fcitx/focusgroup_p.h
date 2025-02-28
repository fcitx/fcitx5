/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FOCUSGROUP_P_H_
#define _FCITX_FOCUSGROUP_P_H_

#include <string>
#include <unordered_set>
#include <utility>
#include <fcitx-utils/intrusivelist.h>
#include <fcitx/focusgroup.h>
#include "fcitx-utils/macros.h"

namespace fcitx {

class InputContextManager;

class FocusGroupPrivate : public QPtrHolder<FocusGroup> {
public:
    FocusGroupPrivate(FocusGroup *q, std::string display,
                      InputContextManager &manager)
        : QPtrHolder(q), display_(std::move(display)), manager_(manager),
          focus_(nullptr) {}

    std::string display_;
    InputContextManager &manager_;
    InputContext *focus_;
    std::unordered_set<InputContext *> ics_;

    IntrusiveListNode listNode_;
};
} // namespace fcitx

#endif // _FCITX_FOCUSGROUP_P_H_
