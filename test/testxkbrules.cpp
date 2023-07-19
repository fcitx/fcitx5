/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "config.h"
#include "im/keyboard/xkbrules.h"

int main() {
    fcitx::XkbRules xkbRules;
    xkbRules.read({XKEYBOARDCONFIG_XKBBASE}, DEFAULT_XKB_RULES, {});
    xkbRules.dump();
    return 0;
}
