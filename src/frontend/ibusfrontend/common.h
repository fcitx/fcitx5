/*
 * SPDX-FileCopyrightText: 2026~2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_IBUSFRONTEND_COMMON_H_
#define _FCITX_FRONTEND_IBUSFRONTEND_COMMON_H_

#include "fcitx-utils/log.h"

namespace fcitx {

FCITX_DECLARE_LOG_CATEGORY(ibus);

}

#define FCITX_IBUS_DEBUG() FCITX_LOGC(::fcitx::ibus, Debug)
#define FCITX_IBUS_WARN() FCITX_LOGC(::fcitx::ibus, Warn)

#endif // _FCITX_FRONTEND_IBUSFRONTEND_COMMON_H_
