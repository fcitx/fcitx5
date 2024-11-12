/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENT_P_H_
#define _FCITX_UTILS_EVENT_P_H_

#include <memory>
#include <fcitx-utils/eventloopinterface.h>

namespace fcitx {

std::unique_ptr<EventLoopInterface> createDefaultEventLoop();

const char *defaultEventLoopImplementation();

} // namespace fcitx

#endif // _FCITX_UTILS_EVENT_P_H_
