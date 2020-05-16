/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _TESTFRONTEND_TESTFRONTEND_PUBLIC_H_
#define _TESTFRONTEND_TESTFRONTEND_PUBLIC_H_

#include <string>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>

FCITX_ADDON_DECLARE_FUNCTION(TestFrontend, createInputContext,
                             ICUUID(const std::string &));

FCITX_ADDON_DECLARE_FUNCTION(TestFrontend, destroyInputContext, void(ICUUID));

FCITX_ADDON_DECLARE_FUNCTION(TestFrontend, keyEvent,
                             void(ICUUID, const Key &, bool isRelease));

FCITX_ADDON_DECLARE_FUNCTION(TestFrontend, pushCommitExpectation,
                             void(std::string));

#endif // _TESTFRONTEND_TESTFRONTEND_PUBLIC_H_
