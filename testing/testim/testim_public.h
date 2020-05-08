/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _TESTIM_TESTIM_PUBLIC_H_
#define _TESTIM_TESTIM_PUBLIC_H_

#include <string>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodentry.h>

FCITX_ADDON_DECLARE_FUNCTION(
    TestIM, setHandler,
    void(std::function<void(const InputMethodEntry &entry,
                            KeyEvent &keyEvent)>));

#endif // _TESTIM_TESTIM_PUBLIC_H_
