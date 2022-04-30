/*
 * SPDX-FileCopyrightText: 2022-2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_IM_KEYBOARD_COMPOSE_H_
#define _FCITX_IM_KEYBOARD_COMPOSE_H_

#include <fcitx-utils/keysym.h>
#include <fcitx/inputcontext.h>
#include <fcitx/instance.h>
#include <deque>

namespace fcitx {
class ComposeState {
public:
    ComposeState(Instance* instance, InputContext *inputContext);
    std::tuple<std::string, bool> type(KeySym sym);
    void backspace();
    std::string preedit() const;
    void reset();
    bool isComposing() const { return !composeBuffer_.empty(); }
private:
    bool typeImpl(KeySym sym, std::string &result);
    Instance *instance_;
    InputContext *inputContext_;
    std::deque<KeySym> composeBuffer_;
};
} // namespace fcitx

#endif
