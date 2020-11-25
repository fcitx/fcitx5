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

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Class for status area in UI.

namespace fcitx {

class Action;
class StatusAreaPrivate;
class InputContext;

enum class StatusGroup {
    /// Action shown before input method group.
    BeforeInputMethod,
    /// Group should be solely used by input method engine.
    /// It will be cleared automatically before InputMethodEngine::activate.
    /// @see InputMethodEngine::activate
    InputMethod,
    /// Action shown after input method group.
    AfterInputMethod,
};

/**
 * Status area represent a list of actions and action may have sub actions.
 *
 * StatusArea can be shown as a floating window, embedded button in panel, or a
 * tray menu. The actual representation is up to UI plugin.
 */
class FCITXCORE_EXPORT StatusArea : public Element {
public:
    /// Construct status area for associated input context.
    StatusArea(InputContext *ic);
    ~StatusArea();

    /// Add an action to given group.
    void addAction(StatusGroup group, Action *action);

    /// Remove an action from given group.
    void removeAction(Action *action);

    /// Clear all the actions, will be called when input context lost focus.
    void clear();

    /// Clear only given status group.
    void clearGroup(StatusGroup group);

    /// Get the associated actions for group.
    std::vector<Action *> actions(StatusGroup group) const;

    /// Get all the associated actions.
    std::vector<Action *> allActions() const;

private:
    std::unique_ptr<StatusAreaPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StatusArea);
};
} // namespace fcitx

#endif // _FCITX_STATUSAREA_H_
