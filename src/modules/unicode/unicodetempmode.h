/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_MODULES_UNICODE_UNICODETEMPMODE_H_
#define _FCITX_MODULES_UNICODE_UNICODETEMPMODE_H_

#include <string_view>
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/key.h"
#include "fcitx/event.h"
#include "fcitx/tempmode.h"

namespace fcitx {

class Unicode;

enum class UnicodeMode {
    Off = 0,
    Search,
    Direct,
};

class UnicodeState {
public:
    UnicodeState();

    void reset(InputContext *inputContext);

    UnicodeMode mode_ = UnicodeMode::Off;
    InputBuffer buffer_;
};

class UnicodeTempMode : public SimpleTempMode<UnicodeState> {
public:
    using Base = SimpleTempMode<UnicodeState>;

    explicit UnicodeTempMode(Unicode *unicode);

    bool trigger(InputContext *inputContext);
    bool triggerTempMode(const KeyEvent &keyEvent) override;
    bool keyEvent(const KeyEvent &keyEvent) override;
    void reset(InputContext *inputContext) override;
    std::string_view name() const override;

    void updateUI(InputContext *inputContext, bool trigger = false);

private:
    bool trigger(InputContext *inputContext, UnicodeMode mode);
    bool handleSearch(const KeyEvent &keyEvent);
    bool handleDirect(const KeyEvent &keyEvent);

    Unicode *unicode_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_UNICODE_UNICODETEMPMODE_H_
