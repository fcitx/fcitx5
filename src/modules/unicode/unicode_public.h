/*
 * SPDX-FileCopyrightText: 2021-2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_UNICODE_UNICODE_PUBLIC_H_
#define _FCITX_MODULES_UNICODE_UNICODE_PUBLIC_H_

#include <functional>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>

FCITX_ADDON_DECLARE_FUNCTION(Unicode, trigger,
                             void(InputContext *ic));

#endif // _FCITX_MODULES_UNICODE_UNICODE_PUBLIC_H_
