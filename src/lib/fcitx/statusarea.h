/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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
