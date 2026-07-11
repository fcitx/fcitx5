/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX5_MODULES_IMSELECTOR_IMSELECTORTEMPMODE_H_
#define _FCITX5_MODULES_IMSELECTOR_IMSELECTORTEMPMODE_H_

#include "fcitx/tempmode.h"

namespace fcitx {

class IMSelector;

class IMSelectorState {
public:
    void reset(InputContext *inputContext);
};

class IMSelectorTempMode : public SimpleTempMode<IMSelectorState> {
public:
    using Base = SimpleTempMode<IMSelectorState>;

    explicit IMSelectorTempMode(IMSelector *imSelector);

    bool triggerTempMode(const KeyEvent &keyEvent) override;
    bool keyEvent(const KeyEvent &keyEvent) override;
    void reset(InputContext *inputContext) override;
    std::string_view name() const override;

protected:
    const KeyList &triggerKeys() const override;

private:
    bool trigger(InputContext *inputContext, bool local);

    IMSelector *imSelector_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_IMSELECTOR_IMSELECTORTEMPMODE_H_
