/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_MODULES_CLIPBOARD_CLIPBOARDTEMPMODE_H_
#define _FCITX_MODULES_CLIPBOARD_CLIPBOARDTEMPMODE_H_

#include "fcitx/tempmode.h"

namespace fcitx {

class Clipboard;

class ClipboardState {
public:
    void reset(InputContext *inputContext);
};

class ClipboardTempMode : public SimpleTempMode<ClipboardState> {
public:
    using Base = SimpleTempMode<ClipboardState>;

    explicit ClipboardTempMode(Clipboard *clipboard);

    bool triggerTempMode(const KeyEvent &keyEvent) override;
    bool keyEvent(const KeyEvent &keyEvent) override;
    void reset(InputContext *inputContext) override;
    std::string_view name() const override;

protected:
    const KeyList &triggerKeys() const override;

private:
    Clipboard *clipboard_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARDTEMPMODE_H_
