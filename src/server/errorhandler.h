/*
 * SPDX-FileCopyrightText: 2002-2005 Yuking
 * yuking_net@sohu.com
 * SPDX-FileCopyrightText: 2010-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _ERRORHANDLER_H
#define _ERRORHANDLER_H

/* ***********************************************************
// Consts
// *********************************************************** */
#ifndef SIGUNUSED
#define SIGUNUSED 29
#endif

namespace fcitx {

//
// Set Posix Signal Handler
//
//
void SetMyExceptionHandler(int pipeFd);

} // namespace fcitx

#endif
